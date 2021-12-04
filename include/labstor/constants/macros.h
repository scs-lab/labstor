//
// Created by lukemartinlogan on 11/18/21.
//

#ifndef LABSTOR_MACROS_H
#define LABSTOR_MACROS_H

#ifdef KERNEL_BUILD

#define LABSTOR_REGION_SUB_POS(ptr, region) (labstor_off_t)((size_t)ptr - (size_t)region)
#define LABSTOR_REGION_SUB_NEG(ptr, region) -(labstor_off_t)((size_t)region - (size_t)ptr)
#define LABSTOR_REGION_SUB(ptr, region) ((size_t)ptr >= (size_t)region ? LABSTOR_REGION_SUB_POS(ptr,region) : LABSTOR_REGION_SUB_NEG(ptr,region))
#define LABSTOR_REGION_ADD(off, region) (void*)((char*)region + off)

#endif

#ifdef __cplusplus

#define KERNEL_PID 0
#define LABSTOR_REGION_SUB_POS(ptr, region) (labstor::off_t)((size_t)ptr - (size_t)region)
#define LABSTOR_REGION_SUB_NEG(ptr, region) -(labstor::off_t)((size_t)region - (size_t)ptr)
#define LABSTOR_REGION_SUB(ptr, region) ((size_t)ptr >= (size_t)region ? LABSTOR_REGION_SUB_POS(ptr,region) : LABSTOR_REGION_SUB_NEG(ptr,region))
#define LABSTOR_REGION_ADD(off, region) (void*)((char*)region + off)

#define DEFINE_SINGLETON(NAME) template<> LABSTOR_##NAME##_T LABSTOR_##NAME##_SINGLETON::obj_ = nullptr;

#endif

#endif //LABSTOR_MACROS_H