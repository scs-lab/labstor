//
// Created by lukemartinlogan on 12/3/21.
//

#ifndef LABSTOR_SHMEM_STRING_MAP_H
#define LABSTOR_SHMEM_STRING_MAP_H

#include <labstor/types/data_structures/unordered_map/constants.h>
#include "shmem_string.h"

struct labstor_string_map_bucket {
    labstor::off_t off_;
    uint32_t value_;
};

static inline void labstor_string_map_bucket_Init(struct labstor_string_map_bucket *bucket, labstor::ipc::string key, uint32_t value, void *region);
static inline uint32_t labstor_string_map_bucket_GetValue(struct labstor_string_map_bucket *bucket, void *region);
static inline labstor::ipc::string labstor_string_map_bucket_GetKey(struct labstor_string_map_bucket *bucket, void *region);
static inline uint32_t labstor_string_map_bucket_GetAtomicValue(struct labstor_string_map_bucket *bucket);
static inline labstor_off_t labstor_string_map_bucket_GetAtomicKey(struct labstor_string_map_bucket *bucket);
static inline uint32_t labstor_string_map_bucket_hash(const uint32_t qtok, void *region);
static inline bool labstor_string_map_bucket_IsMarked(struct labstor_string_map_bucket *bucket);
static inline bool labstor_string_map_bucket_IsNull(struct labstor_string_map_bucket *bucket);
static inline labstor_off_t labstor_string_map_bucket_GetMarkedAtomicKey(struct labstor_string_map_bucket *bucket);
static inline labstor_off_t labstor_string_map_bucket_NullKey(void);

inline void labstor_string_map_bucket_Init(struct labstor_string_map_bucket *bucket, labstor::ipc::string key, uint32_t value, void *region) {
    bucket->off_ = LABSTOR_REGION_SUB(key.GetRegion(), region);
    bucket->value_ = value;
}
inline uint32_t labstor_string_map_bucket_GetValue(struct labstor_string_map_bucket *bucket, void *region) {
    return bucket->value_;
}
inline labstor::ipc::string labstor_string_map_bucket_GetKey(struct labstor_string_map_bucket *bucket, void *region) {
    labstor::ipc::string key;
    if(labstor_string_map_bucket_IsNull(bucket)) { return key; }
    key.Attach(LABSTOR_REGION_ADD(bucket->off_, region));
    return key;
}
inline uint32_t labstor_string_map_bucket_GetAtomicValue(struct labstor_string_map_bucket *bucket) {
    return bucket->value_;
}
inline uint32_t* labstor_string_map_bucket_GetAtomicValueRef(struct labstor_string_map_bucket *bucket) {
    return &bucket->value_;
}
inline labstor::off_t labstor_string_map_bucket_GetAtomicKey(struct labstor_string_map_bucket *bucket) {
    return bucket->off_;
}
inline labstor::off_t* labstor_string_map_bucket_GetAtomicKeyRef(struct labstor_string_map_bucket *bucket) {
    return &bucket->off_;
}
inline static uint32_t labstor_string_map_bucket_hash(const labstor::ipc::string key, void *region) {
    return labstor::ipc::string::hash(key.c_str(), key.size());
}
inline bool labstor_string_map_bucket_IsMarked(struct labstor_string_map_bucket *bucket) {
    return labstor_string_map_bucket_GetAtomicKey(bucket) & null0_mark;
}
inline bool labstor_string_map_bucket_IsNull(struct labstor_string_map_bucket *bucket) {
    return labstor_string_map_bucket_GetAtomicKey(bucket) == null0_null;
}
inline labstor_off_t labstor_string_map_bucket_GetMarkedAtomicKey(struct labstor_string_map_bucket *bucket) {
    return labstor_string_map_bucket_GetAtomicKey(bucket) | null0_mark;
}
inline static labstor_off_t labstor_string_map_bucket_NullKey() {
    return null0_null;
}
inline static bool labstor_string_map_bucket_KeyCompare(labstor::ipc::string key1, labstor::ipc::string key2) {
    return key1 == key2;
}

#include <labstor/types/data_structures/unordered_map/shmem_unordered_map_labstor_string_uint32_t_impl.h>

#ifdef __cplusplus
#include <labstor/types/shmem_type.h>
namespace labstor::ipc {

    class string_map : public unordered_map_labstor_string_uint32_t {
    public:
        inline bool Set(labstor::ipc::string key, uint32_t value) {
            labstor_string_map_bucket bucket;
            labstor_string_map_bucket_Init(&bucket, key, value, GetRegion());
            return unordered_map_labstor_string_uint32_t::Set(bucket);
        }
    };

}
#endif

#endif //LABSTOR_SHMEM_STRING_MAP_H