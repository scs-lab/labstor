//
// Created by lukemartinlogan on 11/18/21.
//

#ifndef LABSTOR_MACROS_H
#define LABSTOR_MACROS_H

#include <labstor/types/basics.h>
#include <labstor/userspace/util/debug.h>
#include <labstor/userspace/util/errors.h>

/*KERNEL_PID*/
#define KERNEL_PID 0

/*LABSTOR_REGION_ADD AND LABSTOR_REGION_SUB*/
#ifdef LABSTOR_MEM_DEBUG
static inline labstor::off_t LABSTOR_REGION_SUB(void *ptr, void *region) {
    if ((size_t) ptr >= (size_t) region) {
        return (size_t) ptr - (size_t) region;
    } else {
#ifdef __cplusplus
        throw labstor::INVALID_REGION_SUB.format((size_t)ptr, (size_t)region);
#elif KERNEL_BUILD
        pr_err("LABSTOR_REGION_SUB, ptr should never be smaller than region!");
        return (size_t) ptr - (size_t) region;
#endif
    }
}
#else
#define LABSTOR_REGION_SUB(ptr,region) (labstor_off_t)((size_t) ptr - (size_t) region)
#endif
#define LABSTOR_REGION_ADD(off, region) (void*)((char*)region + off)

/*YIELD*/
#ifdef KERNEL_BUILD
#include <linux/sched.h>
#define LABSTOR_YIELD() yield()
#elif __cplusplus
#include <sched.h>
#define LABSTOR_YIELD() sched_yield()
#endif

/*BUSY WAIT*/
#define MAX_YIELDS 1000
#define SPINS_PER_USEC 100000
#define MAX_SPINS (SPINS_PER_USEC * 1000)

#define LABSTOR_TIMED_SPINWAIT_START(i,j) \
    for(i = 0; i < MAX_YIELDS; ++i) {\
        for(j = 0; j < MAX_SPINS; ++j) {
#define LABSTOR_TIMED_SPINWAIT_END() } LABSTOR_YIELD();}

#define LABSTOR_SPINWAIT_START(i,j) \
    while(1) {  LABSTOR_TIMED_SPINWAIT_START(i,j)
#define LABSTOR_SPINWAIT_END()\
    LABSTOR_TIMED_SPINWAIT_END() }

#define LABSTOR_SPINWAIT_TIMED_OUT(i,j) (i==MAX_YIELDS || j == MAX_SPINS)

/*SPINLOCK*/
static inline int LABSTOR_SIMPLE_LOCK_TRYLOCK(uint16_t *lockptr) {
    uint16_t unlocked = 0, is_locked = 1;
    return __atomic_compare_exchange_n(lockptr, &unlocked, is_locked, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
}
#define LABSTOR_SIMPLE_LOCK_ACQUIRE(lockptr, i, j)\
    LABSTOR_SPINWAIT_START(i,j)\
    if(LABSTOR_SIMPLE_LOCK_TRYLOCK(lockptr)) { break; }\
    LABSTOR_SPINWAIT_END()
#define LABSTOR_SIMPLE_LOCK_RELEASE(lockptr) \
    *lockptr = 0;

/*SINGLETON*/
#ifdef __cplusplus
#define DEFINE_SINGLETON(NAME) template<> LABSTOR_##NAME##_T LABSTOR_##NAME##_SINGLETON::obj_ = nullptr;
#endif

#endif //LABSTOR_MACROS_H
