#include "TranslationPage.h" 
#include "HashFunction/MurmurHash3New.h"
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    int threshold;
    unsigned long long capacity;
    int page_size;
    int pages_per_block;
    int block_size;
    int tt_blocks;
    int tt_pages;
    int address_size;
    int slab_size;
    float l2p_ratio;
    int gmd_len;
    TranslationPage **gmd;  
    
    int iteration;

    int max_retry;
    int rejections;
    int retries;
    int read_retries;
    int read_error;
    int i_entry_called;
} KVSSD;

// Function Prototypes
void init_KVSSD(KVSSD *ssd, unsigned long long capacity, int page_size, int slab_size, int threshold);
int gmd_size(KVSSD *kvssd);
unsigned long long hash_k(const char *key);
int get_translation_page(KVSSD *ssd, unsigned long long key_hash);
bool write(KVSSD *kvssd, const char *key, int klen, int val, int vlen);
bool read(KVSSD *kvssd, const char *key);
bool delete(KVSSD *kvssd, const char *key);
void get_stats(KVSSD *kvssd);
