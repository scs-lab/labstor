//
// Created by lukemartinlogan on 11/25/21.
//

#include <labstor/userspace/client/macros.h>
#include <labstor/userspace/client/ipc_manager.h>
#include <modules/iosched/manual/client/manual_client.h>

int main() {
    LABSTOR_IPC_MANAGER_T ipc_manager_ = LABSTOR_IPC_MANAGER;
    labstor::iosched::Manual::Manual iosched;

    iosched.Init();
}