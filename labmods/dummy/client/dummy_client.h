//
// Created by lukemartinlogan on 11/26/21.
//

#ifndef LABSTOR_DUMMY_CLIENT_H
#define LABSTOR_DUMMY_CLIENT_H

#include <labmods/dummy/dummy.h>
#include <labstor/userspace/client/client.h>
#include <labstor/constants/macros.h>
#include <labstor/constants/constants.h>
#include <labstor/userspace/types/module.h>
#include <labstor/userspace/client/macros.h>
#include <labstor/userspace/client/client.h>
#include <labstor/userspace/client/ipc_manager.h>
#include <labstor/userspace/client/namespace.h>

namespace labstor::test::Dummy {

class Client: public labstor::Module {
private:
    LABSTOR_IPC_MANAGER_T ipc_manager_;
    LABSTOR_NAMESPACE_T namespace_;
public:
    Client() : labstor::Module(LABSTOR_DUMMY_MODULE_ID) {
        ipc_manager_ = LABSTOR_IPC_MANAGER;
        namespace_ = LABSTOR_NAMESPACE;
    }
    void Initialize(labstor::ipc::request *rq) {}
    int GetNamespaceID();
    void Register();
    void GetValue();
};

}

#endif //LABSTOR_DUMMY_CLIENT_H
