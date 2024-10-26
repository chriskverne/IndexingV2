#include "Murmurhash3New.h"

#define ROTL64(x,r) ((x << r) | (x >> (64 - r)))
#define BIG_CONSTANT(x) (x##LLU)

// Get block function to properly handle unaligned reads
static inline uint64_t getblock64(const uint64_t *p, int i) {
    const uint8_t *bytep = (const uint8_t *)p;
    uint64_t val;
    memcpy(&val, bytep + (i * sizeof(uint64_t)), sizeof(uint64_t));
    return val;
}

static inline uint64_t fmix64(uint64_t k) {
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xff51afd7ed558ccd);
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
    k ^= k >> 33;
    return k;
}

uint64_t MurmurHash3_x64_64(const void *key, int len, uint32_t seed) {
    const uint8_t *data = (const uint8_t *)key;
    const int nblocks = len / 8;  // Changed from 16 to 8 for 64-bit version
    
    uint64_t h1 = seed;
    
    const uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
    const uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);
    
    // Body
    const uint64_t *blocks = (const uint64_t *)(data);
    
    for (int i = 0; i < nblocks; i++) {
        uint64_t k1 = getblock64(blocks, i);
        
        k1 *= c1;
        k1 = ROTL64(k1, 31);
        k1 *= c2;
        
        h1 ^= k1;
        h1 = ROTL64(h1, 27);
        h1 = h1 * 5 + 0x52dce729;
    }
    
    // Tail
    const uint8_t *tail = (const uint8_t *)(data + nblocks * 8);
    uint64_t k1 = 0;
    
    switch (len & 7) {
        case 7: k1 ^= ((uint64_t)tail[6]) << 48;
        case 6: k1 ^= ((uint64_t)tail[5]) << 40;
        case 5: k1 ^= ((uint64_t)tail[4]) << 32;
        case 4: k1 ^= ((uint64_t)tail[3]) << 24;
        case 3: k1 ^= ((uint64_t)tail[2]) << 16;
        case 2: k1 ^= ((uint64_t)tail[1]) << 8;
        case 1: k1 ^= ((uint64_t)tail[0]) << 0;
               k1 *= c1;
               k1 = ROTL64(k1, 31);
               k1 *= c2;
               h1 ^= k1;
    }
    
    // Finalization
    h1 ^= len;
    h1 = fmix64(h1);
    
    return h1;
}
