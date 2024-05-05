#ifndef MURMURHASH3_H
#define MURMURHASH3_H

#include <stdint.h>

// Declaration of the MurmurHash3 function
void MurmurHash3_x64_128(const void *key, int len, uint32_t seed, void *out);

#endif // MURMURHASH3_H
