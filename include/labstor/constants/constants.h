//
// Created by lukemartinlogan on 11/26/21.
//

#ifndef LABSTOR_CONSTANTS_H
#define LABSTOR_CONSTANTS_H

#define LABSTOR_REGISTRAR_ID 0
#define LABSTOR_UPGRADE_ID 1

enum {
    SHMEM_MODULE_RUNTIME_ID,
    WORKER_MODULE_RUNTIME_ID,
    MQ_DRIVER_RUNTIME_ID,
    BIO_DRIVER_RUNTIME_ID,
    BLKDEV_TABLE_RUNTIME_ID
};

#endif //LABSTOR_CONSTANTS_H