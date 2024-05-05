#include "TranslationPage.h"  // Include the header for TranslationPage
#include "HashFunction/MurmurHash3.h"
#include <stdint.h>
#include <string.h>
#include <stddef.h>

typedef struct {
    size_t capacity;
    size_t page_size;
    int pages_per_block;

    size_t block_size;
    size_t tt_blocks;
    size_t tt_pages;
    size_t address_size;
    size_t index_slab_size;
    float l2p_ratio;
    size_t gmd_len;
    TranslationPage **gmd;  // Pointer to array of TranslationPage pointers

    int max_retry;
    int rejections;
    int retries;
    int read_retries;
    int read_error;
    int i_entry_called;
} KVSSD;

// Function Prototypes
void init_KVSSD(KVSSD *ssd, size_t capacity, size_t page_size);
size_t gmd_size(KVSSD *kvssd);
uint64_t hash_k(const char *key);
size_t get_translation_page(KVSSD *ssd, int key_hash);
bool write(KVSSD *kvssd, const char *key, size_t klen, int val, size_t vlen);
bool read(KVSSD *kvssd, const char *key);
