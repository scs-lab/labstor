
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

#include <mpi.h>
#include <labstor/types/allocator/shmem_allocator.h>
#include "labstor/types/data_structures/c/shmem_request_queue.h"
#include <labmods/secure_shmem/netlink_client/secure_shmem_client_netlink.h>

struct simple_request : public labstor::ipc::request {
    int hi;
};

int main(int argc, char **argv) {
    int rank, region_id;
    MPI_Init(&argc, &argv);
    uint32_t region_size = (1<<20);
    uint32_t alloc_region_size = (1<<19);
    uint32_t queue_region_size = (1<<19);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    labstor::ipc::request_queue q;
    labstor::ipc::qtok_t qtok;
    labstor::ipc::shmem_allocator alloc;
    auto netlink_client_ = LABSTOR_KERNEL_CLIENT;
    labstor::kernel::netlink::ShmemClient shmem_netlink;

    //Create SHMEM region
    netlink_client_->Connect();
    if(rank == 0) {
        region_id = shmem_netlink.CreateShmem(region_size, true);
        if(region_id < 0) {
            printf("Failed to allocate SHMEM!\n");
            exit(1);
        }
        printf("Sending ID: %d\n", region_id);
        MPI_Send(&region_id, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    //Acquire region ID from rank 0
    if(rank == 1) {
        MPI_Recv(&region_id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Receiving Region ID: %d\n", region_id);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    //Grant MPI rank access to region
    shmem_netlink.GrantPidShmem(getpid(), region_id);
    void *region = shmem_netlink.MapShmem(region_id, region_size);
    if(region == NULL) {
        printf("Failed to map shmem\n");
        exit(1);
    }
    printf("Mapped Region ID (rank=%d): %d %p\n", rank, region_id, region);

    //Initialize memory allocator and queue
    if(rank == 0) {
        alloc.Init(region, region, alloc_region_size, 256);
        region = alloc.GetNextSection();
        q.Init(region, region, queue_region_size, 1);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank != 0) {
        alloc.Attach(region, region);
        region = alloc.GetNextSection();
        q.Attach(region, region);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    //Place requests in the queue
    if(rank == 0) {
        for(int i = 0; i < 10; ++i) {
            simple_request* rq = (simple_request*)alloc.Alloc(sizeof(simple_request), 0);
            rq->hi = 12341 + i;
            printf("ENQUEUEING REQUEST: %d\n", rq->hi);
            q.Enqueue(rq, qtok);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);


    //Dequeue request and print its value
    if(rank != 0) {
        int i = 0;
        while(i < 10) {
            labstor::ipc::request *rq_uncast;
            simple_request *rq;
            if(!q.Dequeue(rq_uncast)) { continue; }
            rq = (simple_request*)rq_uncast;
            printf("RECEIVED REQUEST: %d\n", rq->hi);
            ++i;
        }
    }

    //Delete SHMEM region
    shmem_netlink.FreeShmem(region_id);

    MPI_Finalize();
}