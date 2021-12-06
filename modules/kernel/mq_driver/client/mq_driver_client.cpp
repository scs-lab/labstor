//
// Created by lukemartinlogan on 12/5/21.
//

#include <labstor/userspace/util/debug.h>
#include <modules/registrar/registrar.h>

#include "mq_driver.h"
#include "mq_driver_client.h"

void labstor::MQDriver::Client::Register() {
    AUTO_TRACE("labstor::MQDriver::Client::Register")
    auto registrar = labstor::Registrar::Client();
    ns_id_ = registrar.RegisterInstance(MQ_DRIVER_MODULE_ID, MQ_DRIVER_MODULE_ID);
    TRACEPOINT("labstor::MQDriver::Client::Register::NamespaceID", ns_id_)
}

void labstor::MQDriver::Client::IO(Ops op, int dev_id, void *user_buf, size_t buf_size, size_t sector, int hctx) {
    AUTO_TRACE("labstor::MQDriver::Client::IO", dev_id, buf_size);
    labstor_submit_mq_driver_request *rq_submit;
    labstor_complete_mq_driver_request *rq_complete;
    labstor::ipc::queue_pair *qp;
    labstor::ipc::qtok_t qtok;

    //Get SERVER QP
    ipc_manager_->GetQueuePair(qp,
                               LABSTOR_QP_SHMEM | LABSTOR_QP_STREAM | LABSTOR_QP_PRIMARY | LABSTOR_QP_ORDERED | LABSTOR_QP_LOW_LATENCY);

    //Create CLIENT -> SERVER message
    TRACEPOINT("Creating MQ driver submission request")
    rq_submit = reinterpret_cast<labstor_submit_mq_driver_request *>(
            ipc_manager_->AllocRequest(qp, sizeof(labstor_submit_mq_driver_request)));
    rq_submit->Init(ns_id_, ipc_manager_->GetPid(), op, dev_id, user_buf, buf_size, sector, hctx);

    //Complete CLIENT -> SERVER interaction
    qtok = qp->Enqueue(reinterpret_cast<labstor::ipc::request*>(rq_submit));
    rq_complete = reinterpret_cast<labstor_complete_mq_driver_request *>(ipc_manager_->Wait(qtok));
    TRACEPOINT("labstor::MQDriver::Client::IO", "Complete", (size_t)rq_complete, (int)rq_complete->header_.op_);

    //Free requests
    ipc_manager_->FreeRequest(qtok, reinterpret_cast<labstor::ipc::request*>(rq_complete));
}

LABSTOR_MODULE_CONSTRUCT(labstor::MQDriver::Client);