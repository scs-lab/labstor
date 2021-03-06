
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

#ifndef LABSTOR_IPC_TEST_H
#define LABSTOR_IPC_TEST_H

#define IPC_TEST_MODULE_ID "IPCTest"

#include <labstor/types/basics.h>
#include <labstor/types/data_structures/shmem_request.h>
//#include <labstor/types/data_structures/shmem_poll.h>
#include "labstor/types/data_structures/c/shmem_queue_pair.h"
#include <labstor/types/data_structures/shmem_qtok.h>

enum IPCTestOps {
    LABSTOR_START_IPC_TEST,
    LABSTOR_COMPLETE_IPC_TEST
};

#ifdef __cplusplus
namespace labstor::IPCTest {
enum class Ops {
    kStartIPCTest,
    kCompleteIPCTest
};
struct ipc_test_request : public labstor::ipc::request {
    struct labstor_qtok_t kern_qtok_;
    int nonce_;
    inline void IPCClientStart(int ns_id, int nonce) {
        ns_id_ = ns_id;
        op_ = static_cast<int>(labstor::IPCTest::Ops::kStartIPCTest);
        code_ = 0;
        nonce_ = nonce;
    }
    inline void IPCKernelStart(int ns_id) {
        ns_id_ = ns_id;
        op_ = static_cast<int>(labstor::IPCTest::Ops::kStartIPCTest);
    }
    inline void Copy(ipc_test_request *rq) {
        SetCode(rq->GetCode());
        nonce_ = rq->nonce_;
    }
};
}
#endif

struct labstor_ipc_test_request {
    struct labstor_request header_;
    struct labstor_qtok_t kern_qtok_;
    int nonce_;
};

#endif //LABSTOR_IPC_TEST_H