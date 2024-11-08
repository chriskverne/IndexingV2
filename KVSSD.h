#include "TranslationPage.h" 
#include "HashFunction/MurmurHash3New.h"
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

typedef struct {\
    int curr_iteration;
    int max_iterations;
    int *kvp_sizes; 
    int threshold;
    uint64_t capacity;
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
    int max_retry;
    int rejections;
    int retries;
    int read_retries;
    int read_error;
    int i_entry_called;
} KVSSD;

// Function Prototypes
void init_KVSSD(KVSSD *ssd, uint64_t capacity, int page_size, int slab_size, int threshold);
int gmd_size(KVSSD *kvssd);
uint64_t hash_k(const char *key);
int get_translation_page(KVSSD *ssd, uint64_t key_hash);
bool write(KVSSD *kvssd, const char *key, int klen, int val, int vlen);
bool read(KVSSD *kvssd, const char *key);
bool delete(KVSSD *kvssd, const char *key);
double get_avg_kv(KVSSD *kvssd);
void update_threshold(KVSSD *kvssd);
void get_stats(KVSSD *kvssd);
