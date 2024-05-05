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

bool delete(KVSSD *kvssd, const char *key) {
    uint64_t key_hash = hash_k(key); 

    for (int i = 0; i < kvssd->max_retry; i++) {
        uint64_t key_hash_retry = key_hash + (uint64_t)i * (uint64_t)i;  
        size_t t_page_idx = get_translation_page(kvssd, key_hash_retry);  
        TranslationPage *t_page = kvssd->gmd[t_page_idx];  

        if (t_page == NULL) 
            continue; // original (return false)

        int slab_index = hashmap_get(t_page->key_hashes, key_hash_retry);  // Get the index of the entry in the hash map

        if (slab_index == NOT_FOUND) // if key_hash in t_page.key_hashes:
            continue;  
        
        bool ret;

        if (slab_index != -1)  
            ret = delete_dentry(t_page, key_hash_retry); // Delete D-entry

        else 
            ret = delete_ientry(t_page, key_hash_retry); // Delete I-entry

        if (ret) return ret;
    }

    return false; 
}

// prints the specifics of our kvssd
void get_stats(KVSSD *kvssd) {
    int tt_d_entry = 0, tt_i_entry_slab = 0, tt_i_entry = 0, tt_empty_slab = 0;
    int tt_keys = 0;
    int d_entry = 0, i_entry = 0;
    int update_d_entry = 0, update_i_entry = 0;
    int tt_retries = 0, tt_evictions = 0;
    int tt_inserts = 0, tt_updates = 0, tt_rejections = 0;
    int tt_read_d_entry = 0, tt_read_i_entry = 0, tt_read_retries = 0, tt_read_errors = 0;
    int tt_pages = 0;

    for (size_t i = 0; i < kvssd->gmd_len; i++) {
        TranslationPage *t_page = kvssd->gmd[i];
        if (t_page == NULL) 
            continue;
        
        tt_d_entry += t_page->d_entry_slabs;
        tt_i_entry_slab += t_page->i_entry_slabs;
        tt_i_entry += t_page->i_entry_count;
        tt_keys += t_page->key_hashes->count; // Assuming hashmap_size() calculates number of keys
        tt_empty_slab += (t_page->tt_slab - t_page->d_entry_slabs - t_page->i_entry_slabs);

        tt_evictions += t_page->evictions;
        tt_inserts += t_page->inserts;
        tt_updates += t_page->updates;
        tt_read_d_entry += t_page->read_d_entry;
        tt_read_i_entry += t_page->read_i_entry;
        i_entry += t_page->new_i_entry;
        d_entry += t_page->new_d_entry;
        update_i_entry += t_page->update_i_entry;
        update_d_entry += t_page->update_d_entry;
        tt_pages++;
    }

    tt_rejections = kvssd->rejections;
    tt_retries = kvssd->retries;
    tt_read_retries = kvssd->read_retries;
    tt_read_errors = kvssd->read_error;

    printf("TT_INDEX_PAGES: %d. TT_SLABS: %ld\n", tt_pages, tt_pages * (kvssd->page_size / kvssd->index_slab_size));
    printf("D-entry: %d, I-entry-slabs: %d, I-entry-count: %d, Empty slab: %d\n", tt_d_entry, tt_i_entry_slab, tt_i_entry, tt_empty_slab);
    printf("TT_KEYS: %d, NEW-DENTRY: %d, NEW-IENTRY: %d\n", tt_keys, d_entry, i_entry);
    printf("UPDATE-DENTRY: %d, UPDATE-IENTRY: %d\n", update_d_entry, update_i_entry);
    printf("Retries: %d, Evictions: %d, Rejections: %d\n", tt_retries, tt_evictions, tt_rejections);
    printf("Insert: %d, Update: %d\n", tt_inserts, tt_updates);
    printf("Read_D-entry: %d, Read-I-entry: %d\n", tt_read_d_entry, tt_read_i_entry);
    printf("Read_Retry: %d, Read_Error: %d\n", tt_read_retries, tt_read_errors);
}


int main() {
    // Initialize the KVSSD instance
    KVSSD *kvssd = malloc(sizeof(KVSSD));
    init_KVSSD(kvssd, (size_t)(4ULL * 1024 * 1024 * 1024), 1024);  // NEED TO CHANGE CAPACITY TO LONG LONG

    // Define key and value lengths
    size_t klen = 4;  // Key length in bytes
    size_t vlen = 4;  // Value length in bytes (assuming it represents the size of the data)

    printf("here");
    // Loop to create and write keys
    for (int i = 0; i < 10; i++) {
        char key[4];  // Buffer to hold the binary representation of the key
        int val = 1;  // Value to write

        // Convert integer i to 4-byte key (similar to to_bytes in Python)
        memcpy(key, &i, sizeof(int));

        // Write the key-value pair to the KVSSD
        if (!write(kvssd, key, klen, val, vlen)) {
            printf("Write failed for key %d\n", i);
        }
    }


    printf("starting");

    get_stats(kvssd);

    printf("done!");

    return 0;
}