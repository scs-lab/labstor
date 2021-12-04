//
// Created by lukemartinlogan on 5/6/21.
//

/*
 * A kernel module that constructs bio and request objects, and submits them to the underlying drivers.
 * */

#include "../request_layer.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/kobject.h>

#include <linux/sbitmap.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/list.h>
#include <linux/cpumask.h>
#include <linux/timer.h>
#include <linux/delay.h>

MODULE_AUTHOR("Luke Logan <llogan@hawk.iit.edu>");
MODULE_DESCRIPTION("A kernel module that performs I/O with underlying storage devices");
MODULE_LICENSE("GPL");
MODULE_ALIAS_FS("request_layer_km");


//Macros
void *virt_mem;
struct request_queue *q;

//Prototypes
static int __init init_request_layer_km(void);
static void __exit exit_request_layer_km(void);


/**
 * Required Kernel Code
 * */

#define BLK_MQ_DISPATCH_BUSY_EWMA_WEIGHT  8
#define BLK_MQ_DISPATCH_BUSY_EWMA_FACTOR  4

enum {
    BLK_MQ_TAG_FAIL		= -1U,
    BLK_MQ_TAG_MIN		= 1,
    BLK_MQ_TAG_MAX		= BLK_MQ_TAG_FAIL - 1,
};

struct blk_mq_tags {
    unsigned int nr_tags;
    unsigned int nr_reserved_tags;

    atomic_t active_queues;

    struct sbitmap_queue bitmap_tags;
    struct sbitmap_queue breserved_tags;

    struct request **rqs;
    struct request **static_rqs;
    struct list_head page_list;
};

struct blk_mq_alloc_data {
    /* input parameter */
    struct request_queue *q;
    blk_mq_req_flags_t flags;
    unsigned int shallow_depth;
    unsigned int cmd_flags;

    /* input & output parameter */
    struct blk_mq_ctx *ctx;
    struct blk_mq_hw_ctx *hctx;
};

struct blk_mq_ctxs {
    struct kobject kobj;
    struct blk_mq_ctx __percpu	*queue_ctx;
};

struct blk_mq_ctx {
    struct {
        spinlock_t		lock;
        struct list_head	rq_lists[HCTX_MAX_TYPES];
    } ____cacheline_aligned_in_smp;

    unsigned int		cpu;
    unsigned short		index_hw[HCTX_MAX_TYPES];
    struct blk_mq_hw_ctx 	*hctxs[HCTX_MAX_TYPES];

    /* incremented at dispatch time */
    unsigned long		rq_dispatched[2];
    unsigned long		rq_merged;

    /* incremented at completion time */
    unsigned long		____cacheline_aligned_in_smp rq_completed[2];

    struct request_queue	*queue;
    struct blk_mq_ctxs      *ctxs;
    struct kobject		kobj;
} ____cacheline_aligned_in_smp;

static inline void blk_rq_bio_prep(struct request *rq, struct bio *bio, unsigned int nr_segs)
{
    rq->nr_phys_segments = nr_segs;
    rq->__data_len = bio->bi_iter.bi_size;
    rq->bio = rq->biotail = bio;
    rq->ioprio = bio_prio(bio);
    if (bio->bi_disk)
        rq->rq_disk = bio->bi_disk;
}

static void hctx_unlock(struct blk_mq_hw_ctx *hctx, int srcu_idx) __releases(hctx->srcu)
{
if (!(hctx->flags & BLK_MQ_F_BLOCKING))
rcu_read_unlock();
else
srcu_read_unlock(hctx->srcu, srcu_idx);
}

static void hctx_lock(struct blk_mq_hw_ctx *hctx, int *srcu_idx) __acquires(hctx->srcu)
{
if (!(hctx->flags & BLK_MQ_F_BLOCKING)) {
*srcu_idx = 0;
rcu_read_lock();
} else
*srcu_idx = srcu_read_lock(hctx->srcu);
}

static inline bool hctx_may_queue(struct blk_mq_hw_ctx *hctx,
                                  struct sbitmap_queue *bt)
{
    unsigned int depth, users;

    if (!hctx || !(hctx->flags & BLK_MQ_F_TAG_SHARED))
        return true;
    if (!test_bit(BLK_MQ_S_TAG_ACTIVE, &hctx->state))
        return true;

    /*
     * Don't try dividing an ant
     */
    if (bt->sb.depth == 1)
        return true;

    users = atomic_read(&hctx->tags->active_queues);
    if (!users)
        return true;

    /*
     * Allow at least some tags
     */
    depth = max((bt->sb.depth + users - 1) / users, 4U);
    return atomic_read(&hctx->nr_active) < depth;
}

static inline bool blk_mq_tag_is_reserved(struct blk_mq_tags *tags,
                                          unsigned int tag) {
    return tag < tags->nr_reserved_tags;
}

static inline bool blk_mq_hctx_stopped(struct blk_mq_hw_ctx *hctx)
{
    return test_bit(BLK_MQ_S_STOPPED, &hctx->state);
}

bool __blk_mq_tag_busy(struct blk_mq_hw_ctx *hctx)
{
    if (!test_bit(BLK_MQ_S_TAG_ACTIVE, &hctx->state) &&
        !test_and_set_bit(BLK_MQ_S_TAG_ACTIVE, &hctx->state))
        atomic_inc(&hctx->tags->active_queues);

    return true;
}

static inline struct blk_mq_tags *blk_mq_tags_from_data(struct blk_mq_alloc_data *data)
{
    if (data->flags & BLK_MQ_REQ_INTERNAL)
        return data->hctx->sched_tags;

    return data->hctx->tags;
}

static void blk_mq_update_dispatch_busy(struct blk_mq_hw_ctx *hctx, bool busy)
{
    unsigned int ewma;

    if (hctx->queue->elevator)
        return;

    ewma = hctx->dispatch_busy;

    if (!ewma && !busy)
        return;

    ewma *= BLK_MQ_DISPATCH_BUSY_EWMA_WEIGHT - 1;
    if (busy)
        ewma += 1 << BLK_MQ_DISPATCH_BUSY_EWMA_FACTOR;
    ewma /= BLK_MQ_DISPATCH_BUSY_EWMA_WEIGHT;

    hctx->dispatch_busy = ewma;
}

static int __blk_mq_get_tag(struct blk_mq_alloc_data *data,
                            struct sbitmap_queue *bt)
{
    if (!(data->flags & BLK_MQ_REQ_INTERNAL) &&
        !hctx_may_queue(data->hctx, bt))
        return -1;
    if (data->shallow_depth)
        return __sbitmap_queue_get_shallow(bt, data->shallow_depth);
    else
        return __sbitmap_queue_get(bt);
}

unsigned int blk_mq_get_tag(struct blk_mq_alloc_data *data)
{
    struct blk_mq_tags *tags = blk_mq_tags_from_data(data);
    struct sbitmap_queue *bt;
    unsigned int tag_offset;
    int tag;

    if (data->flags & BLK_MQ_REQ_RESERVED) {
        if (unlikely(!tags->nr_reserved_tags)) {
            WARN_ON_ONCE(1);
            return BLK_MQ_TAG_FAIL;
        }
        bt = &tags->breserved_tags;
        tag_offset = 0;
    } else {
        bt = &tags->bitmap_tags;
        tag_offset = tags->nr_reserved_tags;
    }

    tag = __blk_mq_get_tag(data, bt);
    if (tag != -1)
        return tag + tag_offset;
    return BLK_MQ_TAG_FAIL;

}

void blk_mq_put_tag(struct blk_mq_hw_ctx *hctx, struct blk_mq_tags *tags,
                    struct blk_mq_ctx *ctx, unsigned int tag)
{
    if (!blk_mq_tag_is_reserved(tags, tag)) {
        const int real_tag = tag - tags->nr_reserved_tags;
        sbitmap_queue_clear(&tags->bitmap_tags, real_tag, ctx->cpu);
    } else {
        sbitmap_queue_clear(&tags->breserved_tags, tag, ctx->cpu);
    }
}

static inline void blk_mq_put_dispatch_budget(struct blk_mq_hw_ctx *hctx)
{
    struct request_queue *q = hctx->queue;

    if (q->mq_ops->put_budget)
        q->mq_ops->put_budget(hctx);
}

static inline bool blk_mq_get_dispatch_budget(struct blk_mq_hw_ctx *hctx)
{
    struct request_queue *q = hctx->queue;

    if (q->mq_ops->get_budget)
        return q->mq_ops->get_budget(hctx);
    return true;
}

static inline void __blk_mq_put_driver_tag(struct blk_mq_hw_ctx *hctx,
                                           struct request *rq)
{
    blk_mq_put_tag(hctx, hctx->tags, rq->mq_ctx, rq->tag);
    rq->tag = -1;

    if (rq->rq_flags & RQF_MQ_INFLIGHT) {
        rq->rq_flags &= ~RQF_MQ_INFLIGHT;
        atomic_dec(&hctx->nr_active);
    }
}

static inline void blk_mq_put_driver_tag(struct request *rq)
{
    if (rq->tag == -1 || rq->internal_tag == -1)
        return;

    __blk_mq_put_driver_tag(rq->mq_hctx, rq);
}

static inline bool blk_mq_tag_busy(struct blk_mq_hw_ctx *hctx)
{
    if (!(hctx->flags & BLK_MQ_F_TAG_SHARED))
        return false;

    return __blk_mq_tag_busy(hctx);
}

bool blk_mq_get_driver_tag(struct request *rq)
{
    struct blk_mq_alloc_data data = {
            .q = rq->q,
            .hctx = rq->mq_hctx,
            .flags = BLK_MQ_REQ_NOWAIT,
            .cmd_flags = rq->cmd_flags,
    };
    bool shared;

    if (rq->tag != -1)
        goto done;

    if (blk_mq_tag_is_reserved(data.hctx->sched_tags, rq->internal_tag))
        data.flags |= BLK_MQ_REQ_RESERVED;

    shared = blk_mq_tag_busy(data.hctx);
    rq->tag = blk_mq_get_tag(&data);
    if (rq->tag >= 0) {
        if (shared) {
            rq->rq_flags |= RQF_MQ_INFLIGHT;
            atomic_inc(&data.hctx->nr_active);
        }
        data.hctx->tags->rqs[rq->tag] = rq;
    }

    done:
    return rq->tag != -1;
}

static blk_status_t __blk_mq_issue_directly(struct blk_mq_hw_ctx *hctx,
                                            struct request *rq,
                                            blk_qc_t *cookie, bool last)
{
    struct request_queue *q = rq->q;
    struct blk_mq_queue_data bd = {
            .rq = rq,
            .last = last,
    };
    blk_qc_t new_cookie;
    blk_status_t ret;

    new_cookie = request_to_qc_t(hctx, rq);

    /*
     * For OK queue, we are done. For error, caller may kill it.
     * Any other error (busy), just add it to our list as we
     * previously would have done.
     */
    ret = q->mq_ops->queue_rq(hctx, &bd);
    switch (ret) {
        case BLK_STS_OK:
            blk_mq_update_dispatch_busy(hctx, false);
            *cookie = new_cookie;
            break;
        case BLK_STS_RESOURCE:
        case BLK_STS_DEV_RESOURCE:
            printk("request_layer_km: NEEDS REQUEUE\n");
            blk_mq_update_dispatch_busy(hctx, false);
            *cookie = BLK_QC_T_NONE;
            break;
        default:
            blk_mq_update_dispatch_busy(hctx, false);
            *cookie = BLK_QC_T_NONE;
            printk("request_layer_km: FAILED\n");
            break;
    }

    return ret;
}

static blk_status_t __blk_mq_try_issue_directly(struct blk_mq_hw_ctx *hctx,
                                                struct request *rq,
                                                blk_qc_t *cookie,
                                                bool bypass_insert, bool last)
{
    struct request_queue *q = rq->q;
    bool run_queue = true;

    if (blk_mq_hctx_stopped(hctx) || blk_queue_quiesced(q)) {
        return -1;
    }

    if (q->elevator && !bypass_insert)
        return -1;

    if (!blk_mq_get_dispatch_budget(hctx))
        return -1;

    if (!blk_mq_get_driver_tag(rq)) {
        blk_mq_put_dispatch_budget(hctx);
        return -1;
    }

    printk("request_layer_km: GET DRIVER TAG: %d\n", rq->tag);
    return __blk_mq_issue_directly(hctx, rq, cookie, last);
}

static void blk_mq_try_issue_directly(struct request *rq, blk_qc_t *cookie)
{
    blk_status_t ret;
    int srcu_idx;
    struct blk_mq_hw_ctx *hctx = rq->mq_hctx;

    might_sleep_if(hctx->flags & BLK_MQ_F_BLOCKING);

    hctx_lock(hctx, &srcu_idx);

    ret = __blk_mq_try_issue_directly(hctx, rq, cookie, true, true); //TODO: May not be last...
    if (ret == BLK_STS_RESOURCE || ret == BLK_STS_DEV_RESOURCE)
        blk_mq_end_request(rq, ret); //TODO: theoretically requeue
    else if (ret != BLK_STS_OK)
        blk_mq_end_request(rq, ret);

    hctx_unlock(hctx, srcu_idx);
}

/**
 * I/O REQUEST FUNCTIONS
 * */

static inline struct bio *create_bio(struct block_device *bdev, struct page **pages, int num_pages, size_t sector, int op)
{
    struct bio *bio;
    int i;

    bio = bio_alloc(GFP_KERNEL, num_pages);
    if(bio == NULL) {
        printk(KERN_INFO "create_bio: Could not allocate bio request (%ld bytes)\n", sizeof(struct bio)+num_pages*sizeof(struct bio_vec));
        return NULL;
    }
    bio_set_dev(bio, bdev);
    //bio_set_op_attrs(bio, op, 0);
    bio->bi_opf = op;
    //bio_set_flag(bio, BIO_USER_MAPPED);
    //bio->bi_flags |= (1U << BIO_USER_MAPPED);
    bio->bi_iter.bi_sector = sector;
    bio->bi_end_io = NULL;
    for(i = 0; i < num_pages; ++i) {
        bio_add_page(bio, pages[i], PAGE_SIZE, 0);
    }
    bio->bi_private = NULL;
    bio->bi_status = BLK_STS_OK;
    bio->bi_ioprio = 0;
    bio->bi_write_hint = 0;

    bio_integrity_prep(bio);
    return bio;
}

struct request *bio_to_rq(struct request_queue *q, int hctx_idx, struct bio *bio, unsigned int nr_segs)
{
    struct request *rq = blk_mq_alloc_request_hctx(q, REQ_OP_WRITE, BLK_MQ_REQ_NOWAIT, hctx_idx);
    rq->rq_flags &= ~RQF_IO_STAT;
    rq->bio = rq->biotail = NULL;
    rq->rq_disk = bio->bi_disk;
    rq->__sector = bio->bi_iter.bi_sector;
    rq->write_hint = bio->bi_write_hint;
    blk_rq_bio_prep(rq, bio, nr_segs);
    blk_mq_get_driver_tag(rq);
    return rq;
}

static inline struct page **convert_usr_buf(void *usr_buf, size_t length, int *num_pagesp)
{
    struct page **pages;
    int num_pages;

    num_pages = length/PAGE_SIZE;
    pages = vmalloc(num_pages*sizeof(struct page *), GFP_KERNEL);
    if(pages == NULL) {
        printk(KERN_INFO "time_linux_driver_io_km: Could not allocate space for %d pages\n", num_pages);
        return NULL;
    }
    get_user_pages_fast((long unsigned)usr_buf, num_pages, 0, pages);

    *num_pagesp = num_pages;
    return pages;
}

blk_qc_t submit_io_rq_layer(char *dev, int op, void *user_buf) {
    struct block_device *bdev;
    int nr_hw_queues;
    struct bio *bio;
    struct request *rq;
    struct page *page;
    blk_qc_t cookie;

    blk_mq_try_issue_directly(rq, &cookie);

    return cookie;
}

inline void submit_req_layer_io(struct queue_pair *qp, struct request_layer_request *req) {
    struct page *pages;
    //Convert user's buffer to pages
    //Get block device associated with semantic label
    bdev = blkdev_get_by_path(dev, BDEV_ACCESS_FLAGS, NULL);
    printk("request_layer_km: Acquried bdev %p %s\n", bdev, bdev->bd_disk->disk_name);
    //Get request queue associated with device
    q = bdev->bd_disk->queue;
    printk("request_layer_km: Queue %p\n", q);
    //Disable stats
    q->queue_flags &= ~QUEUE_FLAG_STATS;
    //Create bio
    bio = create_bio(bdev, &page, 1, 0, REQ_OP_WRITE);
    printk("request_layer_km: Create bio %p\n", bio);
    nr_hw_queues = q->nr_hw_queues;
    printk("request_layer_km: nr_hw_queues %d\n", nr_hw_queues);
    //Create request to hctx
    rq = bio_to_rq(q, 0, bio, 1);
    printk("request_layer_km: bio_to_rq\n");
    //Add monitor request
}

void mq_process_request_fn(struct labstor_queue_pair *qp, struct request_layer_request *rq) {
    switch(rq->header.op_) {
        case MQ_SUBMIT_REQUEST: {
            submit_req_layer_io(qp, rq);
            break;
        }
        default: {
            pr_error("Invalid MQ request: %d\n", rq->header.op_);
        }
    }
}

struct labstor_module worker_module = {
        .module_id = WORKER_MODULE_ID,
        .runtime_id = WORKER_MODULE_RUNTIME_ID,
        .process_request_fn = (process_request_fn_netlink_type)mq_process_request_fn,
        .process_request_fn_netlink = NULL,
};

/**
 * MY FUNCTIONS
 * */


static int __init init_request_layer_km(void) {
    register_labstor_module(&reqest_layer_pkg);
}

static void __exit exit_request_layer_km(void) {
    unregister_labstor_module(&request_layer_pkg);
}

module_init(init_request_layer_km)
module_exit(exit_request_layer_km)