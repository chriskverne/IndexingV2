#include "KVSSD.h"


// Function to initialize a KVSSD instance
void init_KVSSD(KVSSD *ssd, size_t capacity, size_t page_size) {
    ssd->capacity = capacity;
    ssd->page_size = page_size;
    ssd->pages_per_block = 256; // Default

    ssd->block_size = ssd->page_size * ssd->pages_per_block;
    ssd->tt_blocks = ssd->capacity / ssd->block_size;
    ssd->tt_pages = ssd->tt_blocks * ssd->pages_per_block;
    ssd->address_size = 4;  // Default
    ssd->index_slab_size = 200;
    ssd->l2p_ratio = 1;
    ssd->gmd_len = (size_t)(ssd->tt_pages * ssd->l2p_ratio);

    // Allocate memory for the GMD array
    ssd->gmd = malloc(ssd->gmd_len * sizeof(TranslationPage *));
    for (size_t i = 0; i < ssd->gmd_len; i++) 
        ssd->gmd[i] = NULL; // self.gmd = [None] * self.gmd_len
    
    ssd->max_retry = 8;
    ssd->rejections = 0;
    ssd->retries = 0;
    ssd->read_retries = 0;
    ssd->read_error = 0;
    ssd->i_entry_called = 0;
}

// returns size of gmd in MB
size_t gmd_size(KVSSD *kvssd) {
    return (kvssd->tt_pages * kvssd->address_size) / (1024 * 1024);
}

uint64_t hash_k(const char *key) {
    uint64_t out[2];  // To store the 128-bit hash
    uint32_t seed = 42;
    int len = strlen(key);  // Calculate the length of the key (ASSUMING strlen(key) IS EQUAL TO KLEN)

    // Call MurmurHash3 function
    MurmurHash3_x64_128(key, len, seed, &out);

    return out[0];  // Return the first 64 bits of the hash
}

// Returns index of translation page
size_t get_translation_page(KVSSD *ssd, int key_hash) {
    return key_hash % ssd->gmd_len;
}

bool write(KVSSD *kvssd, const char *key, size_t klen, int val, size_t vlen) {
    uint64_t key_hash = hash_k(key);

    for (int i = 0; i < kvssd->max_retry; i++) {
        uint64_t key_hash_retry = key_hash + (uint64_t)i * (uint64_t)i;

        size_t t_page_idx = get_translation_page(kvssd, key_hash_retry);
        TranslationPage *t_page = kvssd->gmd[t_page_idx];

        if (t_page == NULL) {
            t_page = create_translation_page(kvssd->page_size, kvssd->index_slab_size, kvssd->i_entry_called); // i_entry called = 0
            kvssd->gmd[t_page_idx] = t_page;
        }

        bool ret = insert(t_page, key_hash_retry, klen, vlen, key, val);
        if (ret) 
            return true;  // Write successful
        
        kvssd->retries++;
    }

    kvssd->rejections++;
    return false;  // All retries exhausted, write failed
}

bool read(KVSSD *kvssd, const char *key) {
    uint64_t key_hash = hash_k(key);

    for (int i = 0; i < kvssd->max_retry; i++){
        uint64_t key_hash_retry = key_hash + (uint64_t)i * (uint64_t)i;
        size_t t_page_idx = get_translation_page(kvssd, key_hash_retry);
        TranslationPage *t_page = kvssd->gmd[t_page_idx];

        if(t_page == NULL)
            continue;

        bool ret = find_value_by_key_hash(t_page, key_hash_retry, key);
        if (ret)
            return ret;
        
        kvssd->read_retries++;
    }

    kvssd->read_error++;
    return false;
}