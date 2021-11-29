//
// Created by lukemartinlogan on 11/28/21.
//

#ifndef LABSTOR_KERNEL_CLIENT_MACROS_H
#define LABSTOR_KERNEL_CLIENT_MACROS_H

#include <labstor/util/singleton.h>
#define LABSTOR_KERNEL_CLIENT_CLASS labstor::Kernel::NetlinkClient
#define LABSTOR_KERNEL_CLIENT_T SINGLETON_T(LABSTOR_KERNEL_CLIENT_CLASS)
#define LABSTOR_KERNEL_CLIENT_SINGLETON scs::Singleton<LABSTOR_KERNEL_CLIENT_CLASS>
#define LABSTOR_KERNEL_CLIENT LABSTOR_KERNEL_CLIENT_SINGLETON::GetInstance()

#endif //LABSTOR_KERNEL_CLIENT_MACROS_H
