
//{T_NAME}: The semantic name of the type
//{T}: The type of the array

#ifndef LABSTOR_ARRAY_{T_NAME}_H
#define LABSTOR_ARRAY_{T_NAME}_H

#include <labstor/types/basics.h>
#ifdef __cplusplus
#include <labstor/types/shmem_type.h>
#endif

struct labstor_array_{T_NAME}_header {
    uint32_t length_;
};

#ifdef __cplusplus
struct labstor_array_{T_NAME} : public labstor::shmem_type {
#else
struct labstor_array_{T_NAME} {
#endif
    struct labstor_array_{T_NAME}_header *header_;
    {T} *arr_;

#ifdef __cplusplus
    inline static uint32_t GetSize(uint32_t length);
    inline uint32_t GetSize();
    inline uint32_t GetLength();
    inline void* GetRegion();
    inline void Init(void *region, uint32_t region_size, uint32_t length = 0);
    inline void Attach(void *region);
    inline {T}& operator [] (int i) { return arr_[i]; }
#endif
};

static inline uint32_t labstor_array_{T_NAME}_GetSize_global(uint32_t length) {
    return sizeof(struct labstor_array_{T_NAME}_header) + sizeof({T})*length;
}

static inline uint32_t labstor_array_{T_NAME}_GetSize(struct labstor_array_{T_NAME} *arr) {
    return labstor_array_{T_NAME}_GetSize_global(arr->header_->length_);
}

static inline uint32_t labstor_array_{T_NAME}_GetLength(struct labstor_array_{T_NAME} *arr) {
    return arr->header_->length_;
}

static inline void* labstor_array_{T_NAME}_GetRegion(struct labstor_array_{T_NAME} *arr) {
    return arr->header_;
}

static inline void* labstor_array_{T_NAME}_GetNextSection(struct labstor_array_{T_NAME} *arr) {
    return (char*)arr->header_ + labstor_array_{T_NAME}_GetSize(arr);
}

static inline void labstor_array_{T_NAME}_Init(struct labstor_array_{T_NAME} *arr, void *region, uint32_t region_size, uint32_t length) {
    arr->header_ = (struct labstor_array_{T_NAME}_header*)region;
    if(length) {
        arr->header_->length_ = length;
    } else {
        arr->header_->length_ = (region_size - sizeof(struct labstor_array_{T_NAME}_header*)) / sizeof({T});
    }
    arr->arr_ = ({T}*)(arr->header_ + 1);
}

static inline void labstor_array_{T_NAME}_Attach(struct labstor_array_{T_NAME} *arr, void *region) {
    arr->header_ = (struct labstor_array_{T_NAME}_header*)region;
    arr->arr_ = ({T}*)(arr->header_ + 1);
}

static inline {T} labstor_array_{T_NAME}_Get(struct labstor_array_{T_NAME} *arr, int i) {
    return arr->arr_[i];
}

static inline {T}* labstor_array_{T_NAME}_GetPtr(struct labstor_array_{T_NAME} *arr, int i) {
    return arr->arr_ + i;
}


#ifdef __cplusplus
namespace labstor::ipc {
    typedef labstor_array_{T_NAME} array_{T_NAME};
}
uint32_t labstor_array_{T_NAME}::GetSize(uint32_t length) {
    return labstor_array_{T_NAME}_GetSize_global(length);
}
uint32_t labstor_array_{T_NAME}::GetSize() {
    return labstor_array_{T_NAME}_GetSize(this);
}
uint32_t labstor_array_{T_NAME}::GetLength() {
    return labstor_array_{T_NAME}_GetLength(this);
}
void* labstor_array_{T_NAME}::GetRegion() {
    return labstor_array_{T_NAME}_GetRegion(this);
}
void labstor_array_{T_NAME}::Init(void *region, uint32_t region_size, uint32_t length) {
    labstor_array_{T_NAME}_Init(this, region, region_size, length);
}
void labstor_array_{T_NAME}::Attach(void *region) {
    labstor_array_{T_NAME}_Attach(this, region);
}

#endif

#endif