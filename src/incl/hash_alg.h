#if !defined(__HASH_ALG_H__)
#define __HASH_ALG_H__

#include "comm.h"

uint32_t int32_hash(uint32_t val);
uint32_t jenkins_hash(const void *key,size_t length);
uint64_t hash_time33(const char *str);
uint64_t hash_time33_ex(const void *addr, size_t len);

#endif /*__HASH_ALG_H__*/
