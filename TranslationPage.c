#include "TranslationPage.h"

void print_tp_stats(TranslationPage *tp){
    printf("\n=== STATS ===\n");
    printf("Total D-Entries: %d\n", tp->dentry_idx);
    printf("Total D-Entry Slabs Used: %d\n", tp->d_entry_slabs);
    printf("Total I-Entries: %d\n", tp->i_entry_count);
    printf("Total Evictions: %d\n", tp->evictions);
    printf("Total Updates: %d\n", tp->updates);
    printf("Total Inserts: %d\n", tp->inserts);
    printf("Total Rejections: %d\n", tp->rejections);
    printf("New I-Entries: %d\n", tp->new_i_entry);
    printf("New D-Entries: %d\n", tp->new_d_entry);
    printf("Updated D-Entries: %d\n", tp->update_d_entry);
    printf("Updated I-Entries: %d\n", tp->update_i_entry);
    printf("Read D-Entries: %d\n", tp->read_d_entry);
    printf("Read I-Entries: %d\n", tp->read_i_entry);
}

// Debugging functions
void print_dentry(DEntry entry) {
    printf("DEntry - key_hash: %llu, key: %s, val: %d, klen: %d, vlen: %d, num_slabs: %d\n", 
           entry.key_hash, 
           entry.key, 
           entry.val, 
           entry.klen, 
           entry.vlen, 
           entry.num_slabs);
}

void print_dentries(TranslationPage *tp){
    printf("\n=== D-Entries ===\n");
    for (int i = 0; i < tp->dentry_idx; i++){
        DEntry entry = tp->d_entries[i];
        print_dentry(entry);
    }
    printf("Total D-Entries: %d\n", tp->dentry_idx);
}

void print_ientries(TranslationPage *tp){
    printf("\n=== I-Entries ===\n");
    for (int i = 0; i < tp->i_entries->size; i++) {
        if (tp->i_entries->table[i].is_occupied) {
            printf("Key Hash: %llu\n", tp->i_entries->table[i].key_hash);
        }
    }
    printf("Total I-Entries: %d\n", tp->i_entry_count);
}

void print_key_hashes(TranslationPage *tp){
    printf("\n=== Key Hash Mappings ===\n");
    for (int i = 0; i < tp->key_hashes->size; i++) {
        if (tp->key_hashes->table[i].is_occupied) {
            printf("Key Hash: %llu, Idx: %d", 
                   tp->key_hashes->table[i].key_hash, 
                   tp->key_hashes->table[i].value);
            // Add clarification for special values
            if (tp->key_hashes->table[i].value == -1) {
                printf(" (I-Entry)");
            } else if (tp->key_hashes->table[i].value >= 0) {
                printf(" (D-Entry)");
            }
            printf("\n");
        }
    }
}

// Create hashmap of size tt_slab
HashMap* create_hashmap(int size) {
    HashMap *map = (HashMap*)malloc(sizeof(HashMap));
    map->table = (HashMapEntry*)malloc(size * sizeof(HashMapEntry));
    map->size = size;

    for (int i = 0; i < size; i++) {
        map->table[i].is_occupied = false; // Mark as not occupied
        map->table[i].key_hash = 0; // Initialize key_hash to 0 (or another invalid value)
        map->table[i].value = NOT_FOUND; // Initialize value to NOT_FOUND (-2 or another special value)
    }

    return map;
}

// Simple hash function
int hash_function_map(uint64_t key_hash, int table_size) {
    return key_hash % table_size;
}

// put method
void hashmap_put(HashMap *map, uint64_t key_hash, int value) {
    int index = hash_function_map(key_hash, map->size);

    while (map->table[index].is_occupied && map->table[index].key_hash != key_hash) {
        index = (index + 1) % map->size;
    }

    // Insert or update the key_hash -> value pair
    map->table[index].key_hash = key_hash;
    map->table[index].value = value;
    map->table[index].is_occupied = true;
}

// Function to get the value associated with a key_hash
// Returns -2 (NOT_FOUND) if the key_hash is not found
int hashmap_get(HashMap *map, uint64_t key_hash) {
    int index = hash_function_map(key_hash, map->size);

    while (map->table[index].is_occupied) {
        if (map->table[index].key_hash == key_hash) {
            return map->table[index].value;  // Found: return the value (either -1 or a valid index)
        }
        index = (index + 1) % map->size;
    }

    // Key not found
    return NOT_FOUND;
}

// Function to delete a key_hash from the hashmap
void hashmap_delete(HashMap *map, uint64_t key_hash) {
    int index = hash_function_map(key_hash, map->size);

    while (map->table[index].is_occupied) {
        if (map->table[index].key_hash == key_hash) {
            // Mark the entry as deleted (set a special value for "deleted")
            map->table[index].is_occupied = false;
            map->table[index].value = NOT_FOUND;
            return;
        }
        index = (index + 1) % map->size;
    }
}

// Function to create a hash set with a dynamic size
HashSet* create_hash_set(int size) {
    HashSet *set = (HashSet*)malloc(sizeof(HashSet));
    set->table = (HashSetEntry*)malloc(size * sizeof(HashSetEntry));
    set->size = size;

    // Initialize all slots as empty
    for (int i = 0; i < size; i++) {
        set->table[i].is_occupied = false;
    }

    return set;
}

// Hash function to map key_hash to an index
int hash_function(uint64_t key_hash, int table_size) {
    return key_hash % table_size;
}

// Function to insert a key_hash into the set
void hash_set_put(HashSet *set, uint64_t key_hash) {
    int index = hash_function(key_hash, set->size);

    // Linear probing in case of collision
    while (set->table[index].is_occupied) {
        if (set->table[index].key_hash == key_hash) {
            // Key already exists in the set
            return;
        }
        index = (index + 1) % set->size;
    }

    // Insert the new key_hash
    set->table[index].key_hash = key_hash;
    set->table[index].is_occupied = true;
}

// Function to check if a key_hash is in the set
bool hash_set_contains(HashSet *set, uint64_t key_hash) {
    int index = hash_function(key_hash, set->size);

    // Linear probing to search for the key
    while (set->table[index].is_occupied) {
        if (set->table[index].key_hash == key_hash) {
            return true;  // Found the key
        }
        index = (index + 1) % set->size;
    }

    return false;  // Key not found
}

// Function to delete a key_hash from the set
void hash_set_delete(HashSet *set, uint64_t key_hash) {
    int index = hash_function(key_hash, set->size);

    // Linear probing to find the key
    while (set->table[index].is_occupied) {
        if (set->table[index].key_hash == key_hash) {
            set->table[index].is_occupied = false;  // Mark the slot as empty
            return;
        }
        index = (index + 1) % set->size;
    }
}

// TranslationPage Constructor:
TranslationPage* create_translation_page(int page_size, int slab_size, int threshold) {
    TranslationPage *tp = malloc(sizeof(TranslationPage));
    if (!tp) {
        fprintf(stderr, "Memory allocation failed for TranslationPage\n");
        return NULL;
    }

    tp->threshold = threshold;
    tp->slab_size = slab_size; 
    tp->tt_slab = page_size / slab_size;

    tp->d_entries = (DEntry*)malloc(tp->tt_slab * sizeof(DEntry));
    tp->i_entries = create_hash_set(tp->tt_slab); 
    tp->key_hashes = create_hashmap(tp->tt_slab);

    tp->dentry_idx = 0; 
    tp->d_entry_slabs = 0;
    tp->i_entry_count = 0;
    tp->evictions = 0;
    tp->updates = 0;
    tp->inserts = 0;
    tp->rejections = 0;
    tp->new_i_entry = 0;
    tp->new_d_entry = 0;
    tp->update_d_entry = 0;
    tp->update_i_entry = 0;
    tp->read_d_entry = 0;
    tp->read_i_entry = 0;

    return tp;
}

DEntry create_dentry(uint64_t key_hash, const char *key, int val, int klen, int vlen, int num_slabs) {
    DEntry new_entry;  // Create a new DEntry object
    new_entry.key_hash = key_hash;
    strncpy(new_entry.key, key, sizeof(new_entry.key) - 1);
    new_entry.key[sizeof(new_entry.key) - 1] = '\0';  
    new_entry.val = val;
    new_entry.klen = klen;
    new_entry.vlen = vlen;
    new_entry.num_slabs = num_slabs;
    return new_entry; 
}

// Updates indexes of key_hashes to reflect change in d_entries array
void update_key_hashes(TranslationPage *tp) {
    //print_dentries(tp);
    
    // Clear the existing key_hashes hashmap before updating
    for (int i = 0; i < tp->key_hashes->size; i++) {
        tp->key_hashes->table[i].is_occupied = false;
        tp->key_hashes->table[i].key_hash = 0;
        tp->key_hashes->table[i].value = NOT_FOUND;
    }

    // Update key_hashes based on the current d_entries
    for (int i = 0; i < tp->dentry_idx; i++) {  // Use dentry_idx instead of tt_slab
        if (tp->d_entries[i].key_hash != 0) {    // Skip empty/deleted entries
            hashmap_put(tp->key_hashes, tp->d_entries[i].key_hash, i);
        }
    }
}

bool check_hash_collision(int idx ,TranslationPage *tp, const char *key){
    // check for hash_collision (doesn't work for I-entry)
    char* oldK;
    if (idx != -1) // Check D-entry collision
        oldK = tp->d_entries[idx].key;

    //else
    // Handle I-entry collision (might not be possible)

    // if kvp to update key != new key (a hash collision has happnend)
    if (oldK != NULL && strcmp(oldK, key) != 0){
        printf("Collision happnened, oldkey: %s, newKey: %s\n", oldK, key);
        return true;
    }    

    return false;
}

bool insert(TranslationPage *tp, uint64_t key_hash, int klen, int vlen, const char *key, int val) {
    int slabs_needed = ceil((double)(klen + vlen) / tp->slab_size);

    // if key_hash exists update
    if(hashmap_get(tp->key_hashes, key_hash) != NOT_FOUND){ 
        int idx = hashmap_get(tp->key_hashes, key_hash);  // returns -2 for not found, -1 for Ientry, Index for Dentry
        
        // if a hash collision happens (we try to update entry with different key)
        if(check_hash_collision(idx, tp, key))
            return false;

        // Update D-entry
        if (idx != -1 && idx != NOT_FOUND && idx < tp->dentry_idx) { 
            // Case 1, Convert D-entry to I-entry
            if (klen + vlen > tp->threshold) {
                //printf("Updating D-entry to I-entry\n");
                delete_dentry(tp, key_hash); // delete current d-entry
                insert_ientry(tp, key_hash); // insert it as new i-entry
                tp->evictions++;
            } 

            // Case 2, same number of slabs needed (simply update klen and vlen)
            else if(slabs_needed == tp->d_entries[idx].num_slabs){
                tp->d_entries[idx].klen = klen;
                tp->d_entries[idx].vlen = vlen;
                tp->d_entries[idx].val = val;
            }

            // Case 3, new d-entry requires fewer slabs
            else if(slabs_needed < tp->d_entries[idx].num_slabs){
                //printf("Updating D-entry to fewer slabs\n");
                int difference = tp->d_entries[idx].num_slabs - slabs_needed;
                tp->d_entries[idx].num_slabs = slabs_needed;
                tp->d_entries[idx].val = val;
                tp->d_entries[idx].klen = klen;
                tp->d_entries[idx].vlen = vlen;
                tp->d_entry_slabs -= difference;
            }

            // Case 4, new d-entry requires more slabs
            else if(slabs_needed > tp->d_entries[idx].num_slabs){
                //printf("Updating D-entry to more slabs\n");
                int difference = slabs_needed - tp->d_entries[idx].num_slabs;
                if (tp->d_entry_slabs + tp->i_entry_count + difference > tp->tt_slab){
                    delete_dentry(tp, key_hash); // delete current d-entry
                    insert_ientry(tp, key_hash); // insert it as i-entry
                    tp->evictions++;
                } else{
                    tp->d_entries[idx].num_slabs = slabs_needed;
                    tp->d_entries[idx].klen = klen;
                    tp->d_entries[idx].vlen = vlen;
                    tp->d_entries[idx].val = val;
                    tp->d_entry_slabs += difference;
                }
            }

            tp->updates++;
            tp->update_d_entry++;
            return true;
        }

        // Update I-entry
        else if (idx == -1){
            // i-entry becomes d-entry
            if (klen + vlen < tp->threshold){
                //printf("Updating I-entry to D-entry\n");
                if(tp->d_entry_slabs + tp->i_entry_count + slabs_needed - 1 >= tp->tt_slab){
                    //printf("Not enough space to Update I-entry to D-entry\n");
                    return true; // not enough space
                }
                delete_ientry(tp, key_hash); // delete current entry
                insert_dentry(tp, key_hash, klen, vlen, key, val); // insert it as d-entry
            } 
            // I-entry becomes a new I-entry (No need to do anything)
            else{
                //printf("Updating I-entry to I-entry\n");
            }
            
            tp->updates++;
            tp->update_i_entry++;
            return true;
        }
    }

    else if(klen + vlen <= tp->threshold){
        // Insert D-entry
        bool ret = insert_dentry(tp, key_hash, klen, vlen, key, val);
        if (ret){
            tp->new_d_entry++;
            return true;
        } else{
            ret = insert_dentry_by_eviction(tp, key_hash, klen, vlen, key, val);
            if(ret){
                return true;
            } else{
                //printf("Couldn't insert by eviction, trying to insert I-entry\n");
                ret = insert_ientry(tp, key_hash);
                if(ret){
                    return true;
                }
            }
        }
    }

    else {
        bool ret = insert_ientry(tp, key_hash);
        if(ret){
            tp->new_i_entry++;
            return true;
        }
    }

    tp->rejections++;
    return false; // couldn't insert
}

// Function to insert a DEntry in an empty slab
bool insert_dentry(TranslationPage *tp, uint64_t key_hash, int klen, int vlen, const char *key, int val) {
    //printf("Inserting new D-entry\n");
    int slabs_needed = ceil((double)(klen + vlen) / tp->slab_size);
    if (tp->d_entry_slabs + tp->i_entry_count + slabs_needed > tp->tt_slab){
        //printf("Not enough space to insert new D-entry\n");
        return false;
    }
    
// Add dentry to d_entries array and update key_hashes
    DEntry new_dentry = create_dentry(key_hash, key, val, klen, vlen, slabs_needed);
    tp->d_entries[tp->dentry_idx] = new_dentry;
    hashmap_put(tp->key_hashes, key_hash, tp->dentry_idx);

    // Update counters
    tp->dentry_idx++;
    tp->d_entry_slabs += slabs_needed;
    tp->inserts++;
    return true;
}

// insert d_entry by eviction
bool insert_dentry_by_eviction(TranslationPage *tp, uint64_t key_hash, int klen, int vlen, const char *key, int val) {
    int slabs_needed = ceil((double)(klen + vlen) / tp->slab_size);
    //printf("Inserting D-entry by evicting other D-entry, New d-entry needs: %d slabs\n", slabs_needed);
    // Look for D-entry of greater size to evict
    for(int i = 0; i < tp->dentry_idx; i++){
        if(tp->d_entries[i].num_slabs > slabs_needed){
            //printf("larger D-entry found, key_hash: %d\n", tp->d_entries[i].key_hash);
            uint64_t evict_key_hash = tp->d_entries[i].key_hash;
            delete_dentry(tp, evict_key_hash);
            insert_dentry(tp, key_hash, klen, vlen, key, val);
            insert_ientry(tp, evict_key_hash);
            tp->evictions++;
            return true;
        }
    }

    print_dentries(tp);
    printf("No entries to evict, tried to insert %d B\n", klen + vlen);
    // no entries of greater size to evict
    return false;
}

// SHOULD BE DONE
bool insert_ientry(TranslationPage *tp, uint64_t key_hash) {
    //printf("Inserting new I-entry\n");
    // Not enough space, can't insert I-entry
    if (tp->d_entry_slabs + tp->i_entry_count >= tp->tt_slab){
        return false;
    }

    hash_set_put(tp->i_entries, key_hash); // insert key_hash in i_entries
    hashmap_put(tp->key_hashes, key_hash, -1); // insert key_hash in key_hashes
    tp->i_entry_count++;
    tp->inserts++;
    return true;
}

// finds value from key_hash
bool find_value_by_key_hash(TranslationPage *tp, uint64_t key_hash, const char *key) {
    if (hashmap_get(tp->key_hashes, key_hash) != NOT_FOUND) {  // if key_hash in self.key_hashes: (key_hash exists)
        int idx = hashmap_get(tp->key_hashes, key_hash);  // Check if key_hash exists
        if (idx != -1 && idx < tp->dentry_idx) {  // It's a D-entry
            tp->read_d_entry += 1;  // Increment D-entry read count
            return true;
        }
        else {  // It's an I-entry
            assert(hash_set_contains(tp->i_entries, key_hash));  // assert(key_hash in self.i_entries)
            tp->read_i_entry += 1;  // Increment I-entry read count
            return true;
        }
    }

    return false;  // key_hash not found
}

// SHOULD BE DONE
bool delete_dentry(TranslationPage *tp, uint64_t key_hash) {
    //printf("Trying to delete d-entry, key_hash: %d", key_hash);
    //print_dentries(tp);
    //print_key_hashes(tp);
    int idx = hashmap_get(tp->key_hashes, key_hash);  // -2 = not found, -1 = ientry
    if (idx == NOT_FOUND || idx == -1) {
        printf("couldn't delete");
        return false;  // Not a Dentry
    }

    int num_slabs = tp->d_entries[idx].num_slabs;

    // Remove key_hash from the hash map
    hashmap_delete(tp->key_hashes, key_hash);  

    // Delete dentry from dentries
    for (int i = idx; i < tp->dentry_idx; i++) 
        tp->d_entries[i] = tp->d_entries[i + 1]; 

    // Clear the last entry (now a duplicate after shifting)
    tp->d_entries[tp->dentry_idx - 1].key_hash = 0;  // Reset key_hash
    tp->d_entries[tp->dentry_idx - 1].val = 0;       // Reset value
    tp->d_entries[tp->dentry_idx - 1].klen = 0;      // Reset key length
    tp->d_entries[tp->dentry_idx - 1].vlen = 0;      // Reset value length
    tp->d_entries[tp->dentry_idx - 1].num_slabs = 0; // Reset slab count

    //printf("\nDone deleting D-entry, d_entru\n");

    // update indexes in key_hashes
    update_key_hashes(tp);

    tp->d_entry_slabs -= num_slabs; 
    tp->dentry_idx--;

    return true;  // Deletion successful
}

// SHOULD BE DONE
bool delete_ientry(TranslationPage *tp, uint64_t key_hash) {
    if (hash_set_contains(tp->i_entries, key_hash)) {  // if key_hash in self.i_entries: (checks if key_hash is in Ientries)
        hash_set_delete(tp->i_entries, key_hash);  // self.i_entries.remove(key_hash) (remove key_hash from i_entries)
        hashmap_delete(tp->key_hashes, key_hash);  // del self.key_hashes[key_hash] (delete key_hash from key_hashes)
        tp->i_entry_count--;  
        return true;  // Deletion successful
    }

    return false;  // Nothing to delete
}

void generate_random_string_tp(char *str, int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < length; i++) {
        int random_index = rand() % (sizeof(charset) - 1);
        str[i] = charset[random_index];
    }
    str[length] = '\0';
}

/*
int main() {
    uint64_t key_hash1 = 1ULL;
    uint64_t key_hash2 = 2ULL;
    uint64_t key_hash3 = 3ULL;
    uint64_t key_hash4 = 4ULL;
    uint64_t key_hash5 = 5ULL;
    uint64_t key_hash6 = 6ULL;
    uint64_t key_hash7 = 7ULL;


    // Test 1 insert D-entry/I-entry and insert D-entry by eviction (Looks good!)
    TranslationPage *tp = create_translation_page(100, 20, 100);
    printf("Insert HashMap:\n");
    print_key_hashes(tp);
    insert(tp, key_hash1, 10, 10, "example_key", 42); //1/5
    insert(tp, key_hash2, 10, 50, "example_key2", 42); // 3/5
    insert(tp, key_hash3, 10, 10, "example_key3", 42); // 1/5
    insert(tp, key_hash4, 10, 10, "example_key4", 42); // (1/5 D), (1/5 I), (1/5 D), (1/5D)
    print_key_hashes(tp);
    print_dentries(tp);
    print_ientries(tp);

    // Test 2) Update D-entry -> same size (Looks good!)
    TranslationPage *tp = create_translation_page(100, 20, 100);
    insert(tp, key_hash1, 10, 10, "example_key", 42);
    print_dentries(tp);
    print_key_hashes(tp);
    insert(tp, key_hash1, 10, 2, "example_key", 100);
    print_dentries(tp);
    print_key_hashes(tp);

    // Test 2) Update D-entry -> smaller size (Looks good!)
    TranslationPage *tp = create_translation_page(100, 20, 100);
    insert(tp, key_hash1, 20, 20, "example_key", 42);
    print_dentries(tp);
    print_key_hashes(tp);
    insert(tp, key_hash1, 10, 10, "example_key", 10);
    print_dentries(tp);
    print_key_hashes(tp);

    // Test 2) Update D-entry -> larger size (with + without enough space) (Looks good!)
    TranslationPage *tp = create_translation_page(100, 20, 100);
    insert(tp, key_hash1, 10, 10, "example_key", 42);
    print_dentries(tp);
    print_key_hashes(tp);

    insert(tp, key_hash1, 20, 20, "example_key", 100);
    print_dentries(tp);
    print_key_hashes(tp);
    
    insert(tp, key_hash2, 10, 10, "example_key2", 100); // Taking 1/5 slabs
    print_dentries(tp);
    print_key_hashes(tp);

    insert(tp, key_hash1, 10, 90, "example_key", 100); // we need 5 but only have 4 available
    print_dentries(tp);
    print_key_hashes(tp);

    // Test 2) Update D-entry -> I-entry (Looks good!)
    TranslationPage *tp = create_translation_page(100, 20, 100);
    insert(tp, key_hash1, 10, 10, "example_key", 42);
    print_dentries(tp);
    print_ientries(tp);
    print_key_hashes(tp);
    insert(tp, key_hash1, 200, 100, "example_key", 42); 
    print_dentries(tp);
    print_ientries(tp);
    print_key_hashes(tp); 

    // Test 3) Update I-entry -> I-entry (Looks good!)
    TranslationPage *tp = create_translation_page(100, 20, 100);
    insert(tp, key_hash1, 200, 100, "example_key", 42); 
    print_dentries(tp);
    print_ientries(tp);
    print_key_hashes(tp); 
    insert(tp, key_hash1, 100, 100, "example_key", 42); 
    print_dentries(tp);
    print_ientries(tp);
    print_key_hashes(tp); 

    // Test 3) Update I-entry -> D-entry (with + without space) (Looks good!)
    TranslationPage *tp = create_translation_page(100, 20, 100);
    insert(tp, key_hash1, 200, 100, "example_key", 42); 
    insert(tp, key_hash2, 200, 100, "example_key2", 42); 
    insert(tp, key_hash3, 200, 100, "example_key3", 42); 
    print_dentries(tp);
    print_ientries(tp);
    print_key_hashes(tp);

    insert(tp, key_hash1, 10, 10, "example_key", 42); 
    print_dentries(tp);
    print_ientries(tp);
    print_key_hashes(tp);

    insert(tp, key_hash2, 30, 30, "example_key2", 42); 
    print_dentries(tp);
    print_ientries(tp);
    print_key_hashes(tp); 
    

    // Test 4) Delete D-entry + Read D-entry (Looks good!)
    TranslationPage *tp = create_translation_page(1000, 20, 100);
    insert(tp, key_hash1, 20, 20, "example_key", 42); 
    insert(tp, key_hash2, 10, 30, "example_key2", 42); 
    insert(tp, key_hash3, 10, 90, "example_key3", 42);  
    insert(tp, key_hash4, 10, 90, "example_key4", 42); 
    insert(tp, key_hash4, 10, 2, "example_key5", 42);

    printf("key_hash1 exist: %d\n",find_value_by_key_hash(tp, key_hash1, "example_key"));
    delete_dentry(tp, key_hash1);
    printf("key_hash1 exist: %d\n",find_value_by_key_hash(tp, key_hash1, "example_key"));
    printf("key_hash2 exist: %d\n",find_value_by_key_hash(tp, key_hash2, "example_key2"));
    print_dentries(tp);
    print_key_hashes(tp);
    

    // Test 4) Delete I-entry + Read I-entry (Looks good!)
    
    TranslationPage *tp = create_translation_page(1000, 20, 100);
    insert(tp, key_hash1, 200, 20, "example_key", 42); 
    insert(tp, key_hash2, 100, 30, "example_key2", 42); 
    insert(tp, key_hash3, 100, 90, "example_key3", 42);  
    insert(tp, key_hash4, 100, 90, "example_key4", 42); 
    insert(tp, key_hash4, 100, 2, "example_key5", 42);
    
    printf("key_hash1 exist: %d\n",find_value_by_key_hash(tp, key_hash1, "example_key"));
    delete_ientry(tp, key_hash1);
    printf("key_hash1 exist: %d\n",find_value_by_key_hash(tp, key_hash1, "example_key"));
    printf("key_hash2 exist: %d\n",find_value_by_key_hash(tp, key_hash2, "example_key2"));
    print_ientries(tp);
    print_key_hashes(tp);
    

    // Tesing everything + Counters
    TranslationPage *tp = create_translation_page(10000, 20, 100);

    for (int i = 0; i < 2000; i++) {
        // we have 500 slabs
        // Generate random key and value sizes, ensuring some are larger than 100B
        int klen = rand() % 50 + 1;  // Random key length between 1 and 50
        int vlen = (i % 2 == 0) ? rand() % 100 + 1 : rand() % 300 + 100;  // Alternate between smaller and larger values

        // Generate random key
        char key[51];
        generate_random_string_tp(key, klen);

        // Generate random key_hash and value
        uint64_t key_hash = rand() % 100000000000ULL;
        int value = rand() % 10000;

        // Insert the key-value pair
        insert(tp, key_hash, klen, vlen, key, value);
    }

    // Print all the stats
    print_tp_stats(tp);

    // Clean up resources before exiting (free memory, etc.)
    free(tp->d_entries);
    free(tp->i_entries->table);
    free(tp->i_entries);
    free(tp->key_hashes->table);
    free(tp->key_hashes);
    free(tp);

    return 0; // Exit status
}
*/