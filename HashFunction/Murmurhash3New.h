#include <stdint.h>
#include <string.h>

static inline uint64_t getblock64(const uint64_t *p, int i);
static inline uint64_t fmix64(uint64_t k);
uint64_t MurmurHash3_x64_64(const void *key, int len, uint32_t seed);