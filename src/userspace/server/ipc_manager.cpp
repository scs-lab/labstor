
/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <memory>

#include <labstor/userspace/server/server.h>
#include <labstor/userspace/util/errors.h>
#include <labstor/constants/debug.h>
#include <labstor/types/basics.h>
#include <labstor/userspace/types/socket.h>
#include <labstor/userspace/server/server.h>
#include <labstor/userspace/server/ipc_manager.h>
#include <labstor/userspace/server/work_orchestrator.h>
#include <labstor/kernel/client/kernel_client.h>
#include <labstor/userspace/types/messages.h>
#include <labmods/ipc_manager/netlink_client/ipc_manager_client_netlink.h>
#include <labmods/work_orchestrator/netlink_client/work_orchestrator_client_netlink.h>

LABSTOR_WORK_ORCHESTRATOR_T work_orchestrator_ = LABSTOR_WORK_ORCHESTRATOR;

void labstor::Server::IPCManager::LoadMemoryConfig(std::string pid_type, MemoryConfig &memconf) {
    memconf.region_size = labstor_config_->config_["ipc_manager"][pid_type]["max_region_size_kb"].as<uint32_t>() * SizeType::KB;
    memconf.request_unit = labstor_config_->config_["ipc_manager"][pid_type]["request_unit_bytes"].as<uint32_t>() * SizeType::BYTES;
    memconf.min_request_region = labstor_config_->config_["ipc_manager"][pid_type]["min_request_region_kb"].as<uint32_t>() * SizeType::KB;
    memconf.queue_depth = labstor_config_->config_["ipc_manager"][pid_type]["queue_depth"].as<uint32_t>();
    memconf.num_queues = labstor_config_->config_["ipc_manager"][pid_type]["num_queues"].as<uint32_t>();
    memconf.queue_region_size = memconf.num_queues * labstor::ipc::shmem_queue_pair::GetSize(memconf.queue_depth);
    memconf.request_region_size = memconf.region_size - memconf.queue_region_size;
    if(memconf.queue_region_size >= memconf.region_size) {
        throw NOT_ENOUGH_REQUEST_MEMORY.format(pid_type,
                                               SizeType(memconf.queue_region_size, SizeType::KB).ToString(),
                                               SizeType(memconf.region_size, SizeType::KB).ToString());
    }
    if(memconf.request_region_size < memconf.min_request_region) {
        throw NOT_ENOUGH_REQUEST_MEMORY.format(pid_type,
               SizeType(memconf.request_region_size, SizeType::KB).ToString(),
               SizeType(memconf.min_request_region, SizeType::KB).ToString());
    }
    memconf.request_queue_size = labstor::ipc::request_queue::GetSize(memconf.queue_depth);
    memconf.request_map_size = labstor::ipc::request_map::GetSize(memconf.queue_depth);
}

void labstor::Server::IPCManager::InitializeKernelIPCManager() {
    AUTO_TRACE("")
    MemoryConfig memconf;
    LoadMemoryConfig("kernel", memconf);

    //Create new IPC for the kernel
    PerProcessIPC *client_ipc = RegisterIPC(KERNEL_PID);

    //Create SHMEM region
    LABSTOR_KERNEL_SHMEM_ALLOC_T shmem = LABSTOR_KERNEL_SHMEM_ALLOC;
    int region_id = shmem->CreateShmem(memconf.region_size, true);
    if(region_id < 0) {
        throw WORK_ORCHESTRATOR_WORK_QUEUE_ALLOC_FAILED.format();
    }
    shmem->GrantPidShmem(getpid(), region_id);
    void *region = (char*)shmem->MapShmem(region_id, memconf.region_size);
    if(!region) {
        throw WORK_ORCHESTRATOR_WORK_QUEUE_MMAP_FAILED.format();
    }

    //Initialize request allocator (returns userspace addresses)
    labstor::ipc::shmem_allocator *kernel_alloc;
    kernel_alloc = new labstor::ipc::shmem_allocator();
    kernel_alloc->Init(region, region, memconf.request_region_size, memconf.request_unit);
    client_ipc->SetShmemAlloc(kernel_alloc);
    TRACEPOINT("Kernel request allocator created")

    //Create kernel IPC manager
    LABSTOR_KERNEL_IPC_MANAGER_T kernel_ipc_manager = LABSTOR_KERNEL_IPC_MANAGER;
    kern_base_region_ = kernel_ipc_manager->Register(region_id);

    //Initialize kernel queue allocator (returns userspace addresses)
    TRACEPOINT("Kernel queue allocator created", (size_t)kern_base_region_)
    labstor::segment_allocator *qp_alloc = new labstor::segment_allocator();
    qp_alloc->Init(
            LABSTOR_REGION_ADD(memconf.request_region_size, client_ipc->GetRegion()), //Back of the SHMEM region
            memconf.queue_region_size);
    client_ipc->SetQueueAlloc(qp_alloc);
}

void labstor::Server::IPCManager::CreateKernelQueues() {
    AUTO_TRACE("")
    //Load config options
    MemoryConfig memconf;
    LoadMemoryConfig("kernel", memconf);
    labstor::ipc::queue_pair_ptr ptr;
    std::vector<labstor_assign_qp_request> assign_qp_vec;

    //Get Kernel IPC region
    PerProcessIPC *client_ipc = pid_to_ipc_[KERNEL_PID];

    //Allocate & register SHMEM queues for the kernel
    LABSTOR_KERNEL_WORK_ORCHESTRATOR_T kernel_work_orchestrator = LABSTOR_KERNEL_WORK_ORCHESTRATOR;
    labstor_qid_flags_t flags = LABSTOR_QP_SHMEM | LABSTOR_QP_STREAM | LABSTOR_QP_INTERMEDIATE | LABSTOR_QP_ORDERED | LABSTOR_QP_LOW_LATENCY;
    client_ipc->ReserveQueues(0, flags, memconf.num_queues);
    for(int i = 0; i < memconf.num_queues; ++i) {
        //Initialize kernel QP (userspace address)
        labstor_queue_pair *remote_qp = client_ipc->AllocShmemQueue<labstor_queue_pair>(sizeof(labstor_queue_pair));
        labstor::ipc::qid_t qid = labstor::queue_pair::GetQID(
                0,
                LABSTOR_QP_SHMEM | LABSTOR_QP_STREAM | LABSTOR_QP_INTERMEDIATE | LABSTOR_QP_ORDERED | LABSTOR_QP_LOW_LATENCY,
                i,
                memconf.num_queues,
                KERNEL_PID);
        void *sq_region = client_ipc->AllocShmemQueue(memconf.request_queue_size);
        void *cq_region = client_ipc->AllocShmemQueue(memconf.request_map_size);
        remote_qp->Init(qid, client_ipc->GetRegion(), sq_region, memconf.request_queue_size, cq_region, memconf.request_map_size);
        TRACEPOINT("qid", remote_qp->GetQID().Hash(), "depth", remote_qp->GetDepth(),
                   "offset2", LABSTOR_REGION_SUB(remote_qp->cq_.GetRegion(), client_ipc->GetRegion()));
        remote_qp->GetPointer(ptr, client_ipc->GetRegion());

        //Make a new QP that holds userspace addresses & store internally
        labstor::ipc::shmem_queue_pair *qp = new labstor::ipc::shmem_queue_pair();
        qp->Attach(ptr, client_ipc->GetRegion());
        if(!RegisterQueuePair(qp)) {
            throw IPC_MANAGER_CANT_REGISTER_QP.format();
        }
        TRACEPOINT("RemoteAttach3", (size_t)remote_qp->sq_.base_region_, (size_t)remote_qp->sq_.header_);

        //Add to netlink message
        assign_qp_vec.emplace_back(qp, ptr);
    }

    kernel_work_orchestrator->AssignQueuePairs(assign_qp_vec);
}

void labstor::Server::IPCManager::CreatePrivateQueues() {
    AUTO_TRACE("")
    //Load config options
    MemoryConfig memconf;
    LoadMemoryConfig("private", memconf);

    //Create new IPC for private queues
    PerProcessIPC *client_ipc = RegisterIPC(pid_);

    //Allocator internal memory
    private_mem_ = malloc(memconf.region_size);

    //Initialize request allocator
    labstor::ipc::shmem_allocator *private_alloc;
    private_alloc = new labstor::ipc::shmem_allocator();
    private_alloc->Init(private_mem_, private_mem_, memconf.request_region_size, memconf.request_unit);
    client_ipc->SetShmemAlloc(private_alloc);
    client_ipc->SetPrivateAlloc(private_alloc);
    private_alloc_ = private_alloc;
    TRACEPOINT("Private allocator", (size_t)private_alloc_)

    //Initialize queue allocator
    labstor::segment_allocator *qp_alloc = new labstor::segment_allocator();
    qp_alloc->Init(LABSTOR_REGION_ADD(memconf.request_region_size, client_ipc->GetRegion()),
                    memconf.queue_region_size);
    client_ipc->SetQueueAlloc(qp_alloc);

    //Allocate & register PRIVATE intermediate streaming queues for modules to communicate internally
    labstor_qid_flags_t flags = LABSTOR_QP_PRIVATE | LABSTOR_QP_STREAM | LABSTOR_QP_INTERMEDIATE | LABSTOR_QP_ORDERED | LABSTOR_QP_LOW_LATENCY;
    client_ipc->ReserveQueues(0, flags, memconf.num_queues);
    for(int i = 0; i < memconf.num_queues; ++i) {
        //Initialize QP
        labstor::ipc::shmem_queue_pair *qp = new labstor::ipc::shmem_queue_pair();
        labstor::ipc::qid_t qid = labstor::queue_pair::GetQID(
                0,
                flags,
                i,
                memconf.num_queues,
                pid_);
        void *sq_region = client_ipc->AllocShmemQueue(memconf.request_queue_size);
        void *cq_region = client_ipc->AllocShmemQueue(memconf.request_map_size);
        qp->Init(qid, private_alloc_->GetRegion(), sq_region, memconf.request_queue_size, cq_region, memconf.request_map_size);
        TRACEPOINT("pid", qid.pid_, "pid", qid.type_, "flags", qid.flags_, "cnt", qid.cnt_)
        TRACEPOINT("pid", qp->GetQID().pid_, "pid", qp->GetQID().type_, "flags", qp->GetQID().flags_, "cnt", qp->GetQID().cnt_)

        //Store QP internally
        if(!RegisterQueuePair(qp)) {
            throw IPC_MANAGER_CANT_REGISTER_QP.format();
        }

        //Schedule queue pair
        work_orchestrator_->AssignQueuePair(qp, i);
    }
}

void labstor::Server::IPCManager::RegisterClient(int client_fd, labstor::credentials &creds) {
    AUTO_TRACE(client_fd)
    void *region;
    uint32_t lpid;
    MemoryConfig memconf;
    LoadMemoryConfig("client", memconf);

    //Create new IPC
    PerProcessIPC *client_ipc = RegisterIPC(client_fd, creds);

    //Create shared memory
    LABSTOR_KERNEL_SHMEM_ALLOC_T shmem = LABSTOR_KERNEL_SHMEM_ALLOC; 
    client_ipc->region_id_ = shmem->CreateShmem(memconf.region_size, true);
    if(client_ipc->region_id_ < 0) {
        throw SHMEM_CREATE_FAILED.format();
    }
    shmem->GrantPidShmem(getpid(), client_ipc->region_id_);
    shmem->GrantPidShmem(creds.pid_, client_ipc->region_id_);
    region = shmem->MapShmem(client_ipc->region_id_, memconf.region_size);
    if(!region) {
        throw MMAP_FAILED.format(strerror(errno));
    }

    //Send shared memory to client
    labstor::ipc::setup_reply reply;
    reply.region_id_ = client_ipc->region_id_;
    reply.region_size_ = memconf.region_size;
    reply.request_unit_ = memconf.request_unit;
    reply.request_region_size_ = memconf.request_region_size;
    reply.queue_region_size_ = memconf.queue_region_size;
    reply.queue_depth_ = memconf.queue_depth;
    reply.num_queues_ = memconf.num_queues;
    LABSTOR_NAMESPACE->GetSharedRegion(reply.namespace_region_id_, reply.namespace_region_size_, reply.namespace_max_entries_);
    TRACEPOINT("Registering", reply.region_id_, reply.region_size_, reply.request_unit_)
    client_ipc->GetSocket().SendMSG(&reply, sizeof(reply));
    labstor::kernel::netlink::ShmemClient().GrantPidShmem(creds.pid_, reply.namespace_region_id_);

    //Receive and register client QPs
    RegisterClientQP(client_ipc, region);
}

void labstor::Server::IPCManager::RegisterClientQP(PerProcessIPC *client_ipc, void *region) {
    AUTO_TRACE("")
    //Receive SHMEM queue offsets
    labstor::ipc::register_qp_request request;
    client_ipc->GetSocket().RecvMSG((void*)&request, sizeof(labstor::ipc::register_qp_request));
    uint32_t size = request.GetQueueArrayLength();
    labstor::ipc::queue_pair_ptr *ptrs = (labstor::ipc::queue_pair_ptr*)malloc(size);
    client_ipc->GetSocket().RecvMSG((void*)ptrs, size);
    TRACEPOINT("count", request.count_);

    //Attach request allocator
    labstor::ipc::shmem_allocator *alloc = new labstor::ipc::shmem_allocator();
    alloc->Attach(region, region);
    client_ipc->SetShmemAlloc(alloc);

    //Schedule QP with the work orchestrator
    for(int i = 0; i < request.count_; ++i) {
        labstor::ipc::shmem_queue_pair *qp = new labstor::ipc::shmem_queue_pair();
        qp->Attach(ptrs[i], client_ipc->GetRegion());
        if(i == 0) {
            labstor_qid_flags_t flags = qp->GetQID().flags_;
            client_ipc->ReserveQueues(0, flags, request.count_);
        }
        TRACEPOINT("pid", qp->GetQID().pid_, "pid", qp->GetQID().type_, "flags", qp->GetQID().flags_, "cnt", qp->GetQID().cnt_)
        if(!RegisterQueuePair(qp)) {
            free(ptrs);
            throw IPC_MANAGER_CANT_REGISTER_QP.format();
        }
        work_orchestrator_->AssignQueuePair(qp, i);
    }
    free(ptrs);

    //Reply success
    labstor::ipc::register_qp_reply reply(0);
    client_ipc->GetSocket().SendMSG((void*)&reply, sizeof(labstor::ipc::register_qp_reply));
}

void labstor::Server::IPCManager::PauseQueues() {
}

void labstor::Server::IPCManager::WaitForPause() {
    int num_unpaused;
    do {
        num_unpaused = 0;
    } while(num_unpaused);
}

void labstor::Server::IPCManager::ResumeQueues() {
}