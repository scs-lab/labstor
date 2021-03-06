
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

#ifndef LABSTOR_GENERIC_QUEUE_SERVER_H
#define LABSTOR_GENERIC_QUEUE_SERVER_H

#include <labstor/userspace/server/server.h>
#include <labstor/userspace/types/module.h>
#include <labmods/generic_block/generic_block.h>
#include "labmods/generic_queue/generic_queue.h"

namespace labstor::GenericQueue {

class Server : public labstor::Module {
public:
    Server(std::string module_id) : labstor::Module(module_id) {}
    virtual bool GetStatistics(labstor::queue_pair *qp, stats_request *rq_submit, labstor::credentials *creds) = 0;
};

}

#endif //LABSTOR_GENERIC_QUEUE_SERVER_H