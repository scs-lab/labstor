//
// Created by lukemartinlogan on 11/26/21.
//

#include "blkdev_table_server.h"
#include <labstor/constants/constants.h>

void labstor::BlkdevTable::Server::ProcessRequest(labstor::ipc::queue_pair &qp, labstor::ipc::request *request, labstor::credentials *creds) {
    AUTO_TRACE("labstor::BlkdevTable::ProcessRequest", request->op_, request->req_id_)
    switch (static_cast<Ops>(request->op_)) {
        case Ops::kRegisterBdev: {
            RegisterBlkdev(qp, reinterpret_cast<labstor_submit_blkdev_table_register_request *>(request));
            break;
        }
        case Ops::kUnregisterBdev: {
            UnregisterBlkdev(qp, reinterpret_cast<labstor_submit_blkdev_table_register_request *>(request));
            break;
        }
    }
}

void labstor::BlkdevTable::Server::RegisterBlkdev(labstor::ipc::queue_pair &qp, labstor_submit_blkdev_table_register_request *rq_submit) {
    AUTO_TRACE("labstor::BlkdevTable::AddBdev")
    TRACEPOINT("labstor::BlkdevTable::AddBdev", rq_submit->path);
    labstor_submit_blkdev_table_register_request *kern_submit;
    labstor_complete_blkdev_table_register_request *kern_complete;
    labstor::ipc::queue_pair kern_qp;
    labstor::ipc::qtok_t qtok;

    //Get kernel QP
    ipc_manager_->GetQueuePair(kern_qp, LABSTOR_QP_INTERMEDIATE, 0, KERNEL_PID);
    kern_submit = reinterpret_cast<labstor_submit_blkdev_table_register_request *>(
            ipc_manager_->AllocRequest(kern_qp, sizeof(labstor_submit_blkdev_table_register_request)));
    memcpy(kern_submit, rq_submit, sizeof(labstor_submit_blkdev_table_register_request));

    //Send message to kernel
    qtok = kern_qp.Enqueue(reinterpret_cast<labstor::ipc::request*>(kern_submit));
    TRACEPOINT("labstor::BlkdevTable::AddBdev", "Kernel Start")
    kern_complete = reinterpret_cast<labstor_complete_blkdev_table_register_request *>(ipc_manager_->Wait(qtok));

    //Complete request
    TRACEPOINT("labstor::BlkdevTable::AddBdev", "Kernel Complete")
    qp.Complete(
            reinterpret_cast<labstor::ipc::request*>(rq_submit),
            reinterpret_cast<labstor::ipc::request*>(kern_complete));
}

void labstor::BlkdevTable::Server::UnregisterBlkdev(labstor::ipc::queue_pair &qp, labstor_submit_blkdev_table_register_request *rq_submit) {
    AUTO_TRACE("labstor::BlkdevTable::RemoveBdev")
}