#include "KVSSD.h"

// Function to initialize a KVSSD instance
void init_KVSSD(KVSSD *ssd, uint64_t capacity, int page_size, int slab_size, int threshold) {
    ssd->threshold = threshold;
    ssd->capacity = capacity;
    ssd->page_size = page_size;
    ssd->pages_per_block = 256; 
    ssd->block_size = ssd->page_size * ssd->pages_per_block;
    ssd->tt_blocks = ssd->capacity / ssd->block_size;
    ssd->tt_pages = ssd->tt_blocks * ssd->pages_per_block;
    ssd->address_size = 4;  // Default
    ssd->slab_size = slab_size;
    ssd->l2p_ratio = 1;
    ssd->gmd_len = ssd->tt_pages * ssd->l2p_ratio;

    ssd->gmd = malloc(ssd->gmd_len * sizeof(TranslationPage *));
    if (ssd->gmd == NULL){
        fprintf(stderr, "Failed to allocate memory for GMD\n");
        exit(1); // Or handle error accordingly
    }
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
int gmd_size(KVSSD *kvssd) {
    return (kvssd->tt_pages * kvssd->address_size) / (1024 * 1024);
}

// returns murmurhash 64B version 
uint64_t hash_k(const char *key) {
    uint32_t seed = 42;
    int len = strlen(key); 
    uint64_t hash = MurmurHash3_x64_64(key, len, seed);
    return hash;  
}

// Returns index of translation page
int get_translation_page(KVSSD *ssd, uint64_t key_hash) {
    return key_hash % ssd->gmd_len;
}

bool write(KVSSD *kvssd, const char *key, int val, int klen, int vlen) {
    uint64_t key_hash = hash_k(key);
    //printf("Initial key hash: %llu\n", key_hash);

    for (int i = 0; i < kvssd->max_retry; i++) {
        uint64_t key_hash_retry = key_hash + i * i;
        int t_page_idx = get_translation_page(kvssd, key_hash_retry);
        //printf("Retry %d: Key hash retry: %llu, Translation page index: %zu\n", i, key_hash_retry, t_page_idx); // Debugging the key hash retry and page index

        TranslationPage *t_page = kvssd->gmd[t_page_idx];

        if (t_page == NULL) {
            //printf("Creating new translation page at index %zu\n", t_page_idx); // Indicates a new page is being created
            t_page = create_translation_page(kvssd->page_size, kvssd->slab_size, kvssd->threshold); 
            kvssd->gmd[t_page_idx] = t_page;
        } 
        else {
            //printf("Using existing translation page at index %zu\n", t_page_idx); // Indicates using an existing page
        }

        bool ret = insert(t_page, key_hash_retry, klen, vlen, key, val);

        if (ret) {
            return true;  // Write successful
        }
        
        kvssd->retries++;
        printf("Insert failed, retrying\n");
    }

    printf("Couldn't insert KVP\n");

    kvssd->rejections++;
    return false;  // All retries exhausted, write failed
}

bool read(KVSSD *kvssd, const char *key) {
    uint64_t key_hash = hash_k(key);

    for (int i = 0; i < kvssd->max_retry; i++){
        uint64_t key_hash_retry = key_hash + i * i;
        int t_page_idx = get_translation_page(kvssd, key_hash_retry);
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
        uint64_t key_hash_retry = key_hash + i * i;  
        int t_page_idx = get_translation_page(kvssd, key_hash_retry);  
        TranslationPage *t_page = kvssd->gmd[t_page_idx];  

        if (t_page == NULL) 
            return false; // original (return false)

        if(hashmap_get(t_page->key_hashes, key_hash) != NOT_FOUND){
            bool ret;
            int slab_index = hashmap_get(t_page->key_hashes, key_hash_retry);  // Get the index of the entry in the hash map

            if (slab_index != -1){ 
                ret = delete_dentry(t_page, key_hash_retry); // Delete D-entry
            }
            else {
                ret = delete_ientry(t_page, key_hash_retry); // Delete I-entry
            }
            if (ret){
                return true;
            };
        }
    }

    return false; 
}

// prints the specifics of our kvssd
void get_stats(KVSSD *kvssd) {
    printf("Getting stats\n");
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
        
        tt_d_entry += t_page->dentry_idx;
        tt_i_entry_slab += t_page->i_entry_count;
        //tt_keys += t_page->key_hashes->count; // Assuming hashmap_size() calculates number of keys
        //tt_empty_slab += (t_page->tt_slab - t_page->d_entry_slabs - t_page->i_entry_slabs);

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

    printf("TT_INDEX_PAGES: %d. TT_SLABS: %ld\n", tt_pages, tt_pages * (kvssd->page_size / kvssd->slab_size));
    printf("D-entry: %d, I-entry: %d, Empty slab: %d\n", tt_d_entry, tt_i_entry_slab, tt_empty_slab);
    printf("TT_KEYS: %d, NEW-DENTRY: %d, NEW-IENTRY: %d\n", tt_keys, d_entry, i_entry);
    printf("UPDATE-DENTRY: %d, UPDATE-IENTRY: %d\n", update_d_entry, update_i_entry);
    printf("Retries: %d, Evictions: %d, Rejections: %d\n", tt_retries, tt_evictions, tt_rejections);
    printf("Insert: %d, Update: %d\n", tt_inserts, tt_updates);
    printf("Read_D-entry: %d, Read-I-entry: %d\n", tt_read_d_entry, tt_read_i_entry);
    printf("Read_Retry: %d, Read_Error: %d\n", tt_read_retries, tt_read_errors);
}

int main() {
    KVSSD *ssd = malloc(sizeof(KVSSD));
    if (ssd == NULL) {
        fprintf(stderr, "Failed to allocate memory for KVSSD.\n");
        return 1;
    }

    init_KVSSD(ssd, 4ULL * 1024 * 1024 * 10, 1024, 20, 200);

    for (int i = 1; i < 500001; i++) {
        if(i % 100000 == 0){
            printf("%dth iteration\n", i);
        }
        int klen = 1 + rand() % 20;    // Random key length between 1 and 20
        int vlen = 1 + rand() % 300;
        int val = i;
        char key[klen]; 
        sprintf(key, "%d", i);
        write(ssd, key, val, klen, vlen);
    }

    get_stats(ssd);
    
    /*
    // Looks good!
    printf("SSD Size: %llu\n", ssd->capacity);
    printf("Block size: %d\n", ssd->block_size);
    printf("Total blocks: %d\n", ssd->tt_blocks);
    printf("Total pages: %d\n", ssd->tt_pages);
    printf("GMD length: %d\n\n", ssd->gmd_len);

    // Insert 10 D
    write(ssd, "key1", 1000, 10, 10);
    write(ssd, "key2", 1000, 10, 10);
    write(ssd, "key3", 1000, 10, 10);
    write(ssd, "key4", 1000, 10, 10);
    write(ssd, "key5", 1000, 10, 10);
    write(ssd, "key6", 1000, 10, 10);
    write(ssd, "key7", 1000, 10, 10);
    write(ssd, "key8", 1000, 10, 10);
    write(ssd, "key9", 1000, 10, 10);
    write(ssd, "key10", 1000, 10, 10);

    // Update 3D -> 3I
    write(ssd, "key1", 112, 200, 10);
    write(ssd, "key2", 70, 200, 10);
    write(ssd, "key3", 90, 200, 10);

    // Read I
    read(ssd, "key1");
    read(ssd, "key2");

    // Read D
    read(ssd, "key4");
    read(ssd, "key5");
    read(ssd, "key6");

    get_stats(ssd);
    */
    /*
    write(ssd, "key4", 1000, 10, 10);
    write(ssd, "key5", 1000, 10, 10);
    write(ssd, "key6", 1000, 10, 10);
    write(ssd, "key7", 1000, 10, 10);
    write(ssd, "key8", 1000, 10, 10);
    write(ssd, "key9", 1000, 10, 10);
    write(ssd, "key10", 1000, 10, 10);

    // update 10 d-entries to 10 i entries
    write(ssd, "key4", 3000, 200, 10);
    write(ssd, "key5", 5100, 200, 10);
    write(ssd, "key6", 41000, 200, 10);
    write(ssd, "key7", 14200, 200, 10);
    write(ssd, "key8", 8, 200, 10);
    write(ssd, "key9", 67, 200, 10);
    write(ssd, "key10", 80, 200, 10);
    
    get_stats(ssd);
    */
    


    return 0;
}
