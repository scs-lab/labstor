
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

#include <labstor/userspace/server/server.h>
#include <labstor/userspace/util/errors.h>
#include <labstor/constants/debug.h>
#include <labstor/userspace/types/userspace_daemon.h>
#include <labstor/userspace/server/macros.h>
#include <labstor/userspace/server/work_orchestrator.h>
#include <labstor/userspace/server/worker.h>
#include <labstor/userspace/server/ipc_manager.h>
#include <labstor/userspace/server/server.h>
#include <labstor/userspace/util/partitioner.h>
#include <labstor/kernel/client/kernel_client.h>

#include <labmods/secure_shmem/netlink_client/secure_shmem_client_netlink.h>
#include <labmods/work_orchestrator/netlink_client/work_orchestrator_client_netlink.h>

void labstor::Server::WorkOrchestrator::CreateWorkers() {
    AUTO_TRACE("")
    auto labstor_config_ = LABSTOR_CONFIGURATION_MANAGER;
    auto netlink_client_ = LABSTOR_KERNEL_CLIENT;
    const auto &config = labstor_config_->config_["work_orchestrator"];
    uint32_t queue_depth = config["work_queue_depth"].as<uint32_t>();
    int nworkers;

    //Server worker threads
    nworkers = config["server_workers"].size();
    if(nworkers == 0) {
        throw WORK_ORCHESTRATOR_HAS_NO_WORKERS.format("server");
    }
    worker_pool_.emplace(pid_, std::move(std::vector<std::shared_ptr<labstor::Daemon>>(nworkers)));
    auto &server_workers = worker_pool_[pid_];
    server_workers.resize(nworkers);
    for (const auto &worker_conf : config["server_workers"]) {
        int worker_id = worker_conf["worker_id"].as<int>();
        int cpu_id = worker_conf["cpu_id"].as<int>();
        TRACEPOINT("id", worker_id, "cpu", cpu_id)
        std::shared_ptr<labstor::UserspaceDaemon> worker_daemon = std::shared_ptr<labstor::UserspaceDaemon>(new labstor::UserspaceDaemon());
        std::shared_ptr<labstor::Server::Worker> worker = std::shared_ptr<labstor::Server::Worker>(new labstor::Server::Worker(queue_depth, worker_id));
        server_workers[worker_id] = worker_daemon;
        worker_daemon->SetWorker(worker);
        worker_daemon->Start();
        worker_daemon->SetAffinity(cpu_id);
    }

    //Create kernel work queue region
    labstor::kernel::netlink::ShmemClient shmem;
    nworkers = config["kernel_workers"].size();
    if(nworkers == 0) {
        throw WORK_ORCHESTRATOR_HAS_NO_WORKERS.format("kernel");
    }
    uint32_t region_size = nworkers * labstor::ipc::work_queue::GetSize(queue_depth);
    int region_id = shmem.CreateShmem(region_size, true);
    if(region_id < 0) {
        throw WORK_ORCHESTRATOR_WORK_QUEUE_ALLOC_FAILED.format();
    }
    shmem.GrantPidShmem(getpid(), region_id);
    void *region = (char*)shmem.MapShmem(region_id, region_size);
    if(!region) {
        throw WORK_ORCHESTRATOR_WORK_QUEUE_MMAP_FAILED.format();
    }

    //Tell kernel to create work queues
    LABSTOR_KERNEL_WORK_ORCHESTRATOR_T kernel_work_orchestrator = LABSTOR_KERNEL_WORK_ORCHESTRATOR;
    kernel_work_orchestrator->CreateWorkers(nworkers, region_id, region_size, 0);

    //Worker queues
    worker_pool_.emplace(KERNEL_PID, std::move(std::vector<std::shared_ptr<labstor::Daemon>>(nworkers)));
    auto &kernel_workers = worker_pool_[KERNEL_PID];
    kernel_workers.resize(nworkers);
    for (const auto &worker_conf : config["kernel_workers"]) {
        int worker_id = worker_conf["worker_id"].as<int>();
        int cpu_id = worker_conf["cpu_id"].as<int>();
        TRACEPOINT("id", worker_id, "cpu", cpu_id)
        std::shared_ptr<labstor::kernel::netlink::WorkerClient> worker_daemon =
                std::shared_ptr<labstor::kernel::netlink::WorkerClient>(new labstor::kernel::netlink::WorkerClient(worker_id));
        worker_daemon->SetAffinity(cpu_id);
        kernel_workers[worker_id] = worker_daemon;
    }
}

void labstor::Server::WorkOrchestrator::AssignQueuePair(labstor::ipc::shmem_queue_pair *qp, int worker_id) {
    AUTO_TRACE("")
    LABSTOR_IPC_MANAGER_T ipc_manager_ = LABSTOR_IPC_MANAGER;
    labstor::credentials *creds;
    ipc_manager_->GetRegion(qp, creds);
    auto &server_workers = worker_pool_[pid_];
    if(worker_id < 0) {
        throw NOT_YET_IMPLEMENTED.format("Dynamic work orchestration");
    }
    worker_id = worker_id % server_workers.size();
    TRACEPOINT(worker_id)
    std::shared_ptr<labstor::Server::Worker> worker = std::dynamic_pointer_cast<labstor::Server::Worker>(server_workers[worker_id]->GetWorker());
    worker->AssignQP(qp, creds);
    TRACEPOINT("Depth", worker->GetQueueDepth());
}