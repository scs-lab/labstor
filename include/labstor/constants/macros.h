//
// Created by lukemartinlogan on 11/18/21.
//

#ifndef LABSTOR_MACROS_H
#define LABSTOR_MACROS_H

#include <labstor/types/basics.h>
#include <labstor/userspace/util/debug.h>
#include <labstor/userspace/util/errors.h>

#ifdef KERNEL_BUILD

#define LABSTOR_REGION_SUB(ptr, region) (labstor_off_t)((size_t) ptr - (size_t) region)
#define LABSTOR_REGION_ADD(off, region) (void*)((char*)region + off)

#endif

#ifdef __cplusplus

#define KERNEL_PID 0
static inline labstor::off_t LABSTOR_REGION_SUB(void *ptr, void *region) {
#ifndef LABSTOR_MEM_DEBUG
    return (labstor::off_t)((size_t) ptr - (size_t) region);
#else
    if ((size_t) ptr >= (size_t) region) {
        return (size_t) ptr - (size_t) region;
    } else {
#ifdef __cplusplus
        throw labstor::INVALID_REGION_SUB.format((size_t)ptr, (size_t)region);
#endif
    }
#endif
}
#define LABSTOR_REGION_ADD(off, region) (void*)((char*)region + off)

#define DEFINE_SINGLETON(NAME) template<> LABSTOR_##NAME##_T LABSTOR_##NAME##_SINGLETON::obj_ = nullptr;

#endif

#endif //LABSTOR_MACROS_H
