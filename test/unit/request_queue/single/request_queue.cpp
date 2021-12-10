//
// Created by lukemartinlogan on 9/21/21.
//

#include <labstor/types/data_structures/shmem_request_queue.h>
#include <modules/kernel/secure_shmem/netlink_client/secure_shmem_client_netlink.h>

int main(int argc, char **argv) {
    size_t half_region_size = 4096;
    labstor::ipc::request_queue q;
    labstor::ipc::request *rq;
    void *region = malloc(2*half_region_size);
    labstor::ipc::request *req_region = (labstor::ipc::request*)((char*)region + half_region_size);

    printf("REQUEST QUEUE START!\n");
    q.Init(region, region, half_region_size, 10);
    printf("Max Depth: %d\n", q.GetMaxDepth());
    for(int i = 0; i < 10; ++i) {
        req_region[i].ns_id_ = i+1;
        q.Enqueue(req_region + i);
        printf("ENQUEUED REQUEST[%lu]: %d\n", (size_t)(req_region + i), i);
    }
    printf("\n");

    int i = 0;
    while(i < 10){
        if(!q.Dequeue(rq)) {
            continue;
        }
        if(!rq) {
            continue;
        }
        printf("DEQUEUED REQUEST[%lu]: %d\n", (size_t)(rq), rq->ns_id_);
        ++i;
    }
}