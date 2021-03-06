
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

#ifndef LABSTOR_ERRORS_H
#define LABSTOR_ERRORS_H

#ifdef __cplusplus

#include <labstor/userspace/util/error.h>

namespace labstor {
    const Error FILE_NOT_FOUND(0, "File not found at {}");
    const Error INVALID_STORAGE_TYPE(1, "{} is not a valid storage method");
    const Error INVALID_SERIALIZER_TYPE(2, "{} is not a valid serializer type");
    const Error INVALID_TRANSPORT_TYPE(3, "{} is not a valid transport type");
    const Error INVALID_AFFINITY(3, "Could not set CPU affinity of thread: {}");
    const Error MMAP_FAILED(4, "Could not mmap file: {}");
    const Error LAZY_ERROR(5, "Error in function {}");

    const Error DLSYM_MODULE_NOT_FOUND(100, "Module {} was not loaded; error {}");
    const Error DLSYM_MODULE_NO_CONSTRUCTOR(101, "Module {} has no constructor");

    const Error UNIX_SOCKET_FAILED(200, "Failed to create socket: {}");
    const Error UNIX_BIND_FAILED(201, "Failed to bind socket: {}");
    const Error UNIX_CONNECT_FAILED(202, "Failed to connect over socket: {}");
    const Error UNIX_SENDMSG_FAILED(203, "Failed to send message over socket: {}");
    const Error UNIX_RECVMSG_FAILED(204, "Failed to receive message over socket: {}");
    const Error UNIX_SETSOCKOPT_FAILED(205, "Failed to set socket options: {}");
    const Error UNIX_GETSOCKOPT_FAILED(205, "Failed to acquire user credentials: {}");
    const Error UNIX_LISTEN_FAILED(206, "Failed to listen for connections: {}");
    const Error UNIX_ACCEPT_FAILED(207, "Failed accept connections: {}");

    const Error INVALID_RING_BUFFER_SIZE(300, "Failed to allocate ring buffer, {} is too small to support {} requests.");
    const Error INVALID_UNORDERED_MAP_SIZE(301, "Failed to allocate unordered map, {} is too small to support {} requests.");
    const Error INVALID_UNORDERED_MAP_KEY(301, "No such key in map");
    const Error INVALID_QP_QUERY(302, "There is no such queue: pid={}, type={}, flags={}, cnt={}.");
    const Error INVALID_REGION_SUB(303, "The pointer {} exists outside of {}");

    const Error SHMEM_CREATE_FAILED(400, "Failed to allocate SHMEM");

    const Error INVALID_MODULE_ID(500, "Failed to find module {}");
    const Error INVALID_NAMESPACE_ENTRY(501, "Failed to find namespace entry {}");
    const Error WORK_ORCHESTRATOR_WORK_QUEUE_ALLOC_FAILED(502, "Failed to allocate work queue entries");
    const Error WORK_ORCHESTRATOR_WORK_QUEUE_MMAP_FAILED(503, "Failed to mmap work queue entries");
    const Error WORK_ORCHESTRATOR_HAS_NO_WORKERS(504, "Work orchestrator has no {} workers");
    const Error IPC_MANAGER_CANT_REGISTER_QP(505, "IPCManager failed to register qp");
    const Error KERNEL_IPC_MANAGER_FAILED_TO_REGISTER(506, "IPCManager failed to initialize: {}");
    const Error MODULE_DOES_NOT_EXIST(507, "Module does not exist: {}");
    const Error NOT_ENOUGH_REQUEST_MEMORY(508, "{}: {} were allocated. Minimum of {} expected");
    const Error FAILED_TO_ASSIGN_QUEUE(509, "Failed to assign queue {} to worker {}");
    const Error INVALID_QP_FLAGS(510, "Attempted to find QP with invalid flags set: {}");
    const Error INVALID_QP_CNT(511, "Attempted to find QP with invalid index: {}");
    const Error FAILED_TO_SET_NAMESPACE_KEY(512, "Failed to insert {} into the namespace");
    const Error SPDK_CANT_CREATE_QP(513, "Failed to allocate queue {}");
    const Error SPDK_CANT_RESET_ZONE(513, "Failed to reset zone");

    const Error FAILED_TO_ENQUEUE(508, "Failed to enqueue a request");
    const Error FAILED_TO_DEQUEUE(509, "Failed to enqueue a request");
    const Error FAILED_TO_COMPLETE(510, "Failed to enqueue a request");
    const Error QP_WAIT_TIMED_OUT(511, "Failed to wait for a request");

    const Error NOT_YET_IMPLEMENTED(1000, "{} is not yet implemented");
}

#endif

#endif //LABSTOR_ERRORS_H