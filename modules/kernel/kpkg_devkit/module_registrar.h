//
// Created by lukemartinlogan on 9/17/21.
//

#ifndef LABSTOR_MODULE_REGISTRAR_H
#define LABSTOR_MODULE_REGISTRAR_H

#include <labstor/types/basics.h>
#include <linux/types.h>
#include "request_queue.h"

struct labstor_module {
    struct labstor_id module_id;
    uint32_t runtime_id;
    process_request_fn_type process_request_fn;
    process_request_fn_netlink_type process_request_fn_netlink;
    void* (*get_ops)(void);
    int req_size;
};

void init_labstor_module_registrar(int max_size);
void free_labstor_module_registrar(void);
void labstor_msg_trusted_server(void *serialized_buf, size_t buf_size, int pid);
void register_labstor_module(struct labstor_module *pkg);
void unregister_labstor_module(struct labstor_module *pkg);
struct labstor_module *get_labstor_module(struct labstor_id module_id);
struct labstor_module *get_labstor_module_by_runtime_id(int runtime_id);

#endif //LABSTOR_MODULE_REGISTRAR_H