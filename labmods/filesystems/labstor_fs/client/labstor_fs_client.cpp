//
// Created by lukemartinlogan on 8/20/21.
//

#include <labmods/filesystems/generic_posix/generic_posix.h>
#include <labmods/filesystems/labstor_fs/labstor_fs.h>
#include <labmods/filesystems/labstor_fs/client/labstor_fs_client.h>
#include <labmods/storage_api/generic_block/generic_block.h>
#include <labmods/storage_api/generic_block/client/generic_block_client.h>
#include <labmods/filesystems/labstor_fs/lib/labstor_fs_log.h>
#include <labmods/filesystems/labstor_fs/client/labstor_fs_client.h>

void labstor::LabFS::Client::Register(YAML::Node config) {
    ns_id_ = LABSTOR_REGISTRAR->RegisterInstance(LABFS_MODULE_ID, config["labmod_uuid"].as<std::string>());
    LABSTOR_REGISTRAR->InitializeInstance<register_request>(ns_id_, config["next"].as<std::string>());
    if(config["do_format"].as<bool>()) {
        labstor::GenericBlock::Client *block_dev = namespace_->LoadClientModule<labstor::GenericBlock::Client>(config["device"]);
        if(block_dev == nullptr) {
            throw NOT_YET_IMPLEMENTED.format();
        }
        void *buf = calloc(SMALL_BLOCK_SIZE, 0);
        block_dev->Write(buf, SMALL_BLOCK_SIZE, 0);
    }
}

int labstor::LabFS::Client::Open(int fd, const char *path, int pathlen, int oflag) {
    AUTO_TRACE("")
    labstor::GenericPosix::open_request *client_rq;
    labstor::queue_pair *qp;
    labstor::ipc::qtok_t qtok;

    //Get SERVER QP
    ipc_manager_->GetQueuePair(qp,
                               LABSTOR_QP_SHMEM | LABSTOR_QP_STREAM | LABSTOR_QP_PRIMARY | LABSTOR_QP_ORDERED | LABSTOR_QP_LOW_LATENCY);

    //Create CLIENT -> SERVER message
    client_rq = ipc_manager_->AllocRequest<labstor::GenericPosix::open_request>(qp);
    client_rq->ClientInit(ns_id_, path, oflag, fd);

    //Complete CLIENT -> SERVER interaction
    qp->Enqueue<labstor::GenericPosix::open_request>(client_rq, qtok);
    client_rq = ipc_manager_->Wait<labstor::GenericPosix::open_request>(qtok);
    if(client_rq->GetCode() == LABSTOR_GENERIC_FS_PATH_NOT_FOUND) {
        fd = LABSTOR_GENERIC_FS_PATH_NOT_FOUND;
    }

    //Free requests
    ipc_manager_->FreeRequest<labstor::GenericPosix::open_request>(qtok, client_rq);
    return fd;
}

int labstor::LabFS::Client::Close(int fd) {
    AUTO_TRACE("")
    labstor::GenericPosix::close_request *client_rq;
    labstor::queue_pair *qp;
    labstor::ipc::qtok_t qtok;
    int code;

    //Get SERVER QP
    ipc_manager_->GetQueuePair(qp,
                               LABSTOR_QP_SHMEM | LABSTOR_QP_STREAM | LABSTOR_QP_PRIMARY | LABSTOR_QP_ORDERED | LABSTOR_QP_LOW_LATENCY);

    //Create CLIENT -> SERVER message
    client_rq = ipc_manager_->AllocRequest<labstor::GenericPosix::close_request>(qp);
    client_rq->Start(ns_id_, fd);

    //Complete CLIENT -> SERVER interaction
    qp->Enqueue<labstor::GenericPosix::close_request>(client_rq, qtok);
    client_rq = ipc_manager_->Wait<labstor::GenericPosix::close_request>(qtok);
    code = client_rq->GetCode();

    //Free requests
    ipc_manager_->FreeRequest<labstor::GenericPosix::close_request>(qtok, client_rq);

    return code;
}

labstor::ipc::qtok_t labstor::LabFS::Client::AIO(labstor::GenericPosix::Ops op, int fd, void *buf, size_t size) {
    AUTO_TRACE("")
    labstor::GenericPosix::io_request *client_rq;
    labstor::queue_pair *qp;
    labstor::ipc::qtok_t qtok;
    ssize_t ret;

    //Get SERVER QP
    ipc_manager_->GetQueuePair(qp,
                               LABSTOR_QP_SHMEM | LABSTOR_QP_STREAM | LABSTOR_QP_PRIMARY | LABSTOR_QP_ORDERED | LABSTOR_QP_LOW_LATENCY);

    //Create CLIENT -> SERVER message
    client_rq = ipc_manager_->AllocRequest<labstor::GenericPosix::io_request>(qp);
    client_rq->Start(ns_id_, op, fd, buf, size);

    //Enqueue the message
    qp->Enqueue<labstor::GenericPosix::io_request>(client_rq, qtok);
    return qtok;
}

ssize_t labstor::LabFS::Client::IO(labstor::GenericPosix::Ops op, int fd, void *buf, size_t size) {
    AUTO_TRACE("")
    labstor::GenericPosix::io_request *client_rq;
    labstor::queue_pair *qp;
    labstor::ipc::qtok_t qtok;
    ssize_t ret;
    qtok = AIO(op, fd, buf, size);
    client_rq = ipc_manager_->Wait<labstor::GenericPosix::io_request>(qtok);
    ret = client_rq->GetSize();
    ipc_manager_->FreeRequest<labstor::GenericPosix::io_request>(qtok, client_rq);
    return ret;
}

LABSTOR_MODULE_CONSTRUCT(labstor::LabFS::Client, LABFS_MODULE_ID)