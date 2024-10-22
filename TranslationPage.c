#include "TranslationPage.h"

// Create hashmap of size tt_slab
HashMap* create_hashmap(int size) {
    HashMap *map = (HashMap*)malloc(sizeof(HashMap));
    map->table = (HashMapEntry*)malloc(size * sizeof(HashMapEntry));
    map->size = size;

    for (int i = 0; i < size; i++) {
        map->table[i].is_occupied = false;
    }

    return map;
}

// Simple hash function
int hash_function_map(unsigned long long key_hash, int table_size) {
    return key_hash % table_size;
}

// put method
void hashmap_put(HashMap *map, unsigned long long key_hash, int value) {
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
int hashmap_get(HashMap *map, unsigned long long key_hash) {
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
void hashmap_delete(HashMap *map, unsigned long long key_hash) {
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
int hash_function(unsigned long long key_hash, int table_size) {
    return key_hash % table_size;
}

// Function to insert a key_hash into the set
void hash_set_put(HashSet *set, unsigned long long key_hash) {
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
bool hash_set_contains(HashSet *set, unsigned long long key_hash) {
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
void hash_set_delete(HashSet *set, unsigned long long key_hash) {
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
    if (!tp) return NULL;

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

DEntry create_dentry(unsigned long long key_hash, const char *key, int val, int klen, int vlen, int num_slabs) {
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
    for (int i = 0; i < tp->tt_slab; i++) {
        hashmap_put(tp->key_hashes, tp->d_entries[i].key_hash, i);
    }
}

bool insert(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val) {
    int slabs_needed = ceil((klen + vlen)/tp->slab_size);

    if(hashmap_get(tp->key_hashes, key_hash) != NOT_FOUND){ // if key_hash in self.key_hashes (checks whether key_hash exists)
        int idx = hashmap_get(tp->key_hashes, key_hash);  // returns -2 for not found, -1 for Ientry, Index for Dentry

        // Update D-entry
        if (idx != -1 && idx != NOT_FOUND && idx < tp->dentry_idx) { 
            // Case 1, Convert D-entry to I-entry
            if (klen + vlen > tp->threshold) {
                delete_dentry(tp, key_hash); // delete current d-entry
                insert_ientry(tp, key_hash); // insert it as new i-entry
                tp->evictions++;
            } 

            // Case 2, same number of slabs needed (simply update klen and vlen)
            else if(slabs_needed == tp->d_entries[idx].num_slabs){
                tp->d_entries[idx].klen = klen;
                tp->d_entries[idx].vlen = vlen;
            }

            // Case 3, new d-entry requires fewer slabs
            else if(slabs_needed < tp->d_entries[idx].num_slabs){
                int difference = tp->d_entries[idx].num_slabs - slabs_needed;
                tp->d_entries[idx].num_slabs = slabs_needed;
                tp->d_entries[idx].klen = klen;
                tp->d_entries[idx].vlen = vlen;
                tp->d_entry_slabs -= difference;
            }

            // Case 4, new d-entry requires more slabs
            else if(slabs_needed > tp->d_entries[idx].num_slabs){
                int difference = tp->d_entries[idx].num_slabs - slabs_needed;
                if (tp->d_entry_slabs + tp->i_entry_count + difference >= tp->tt_slab){
                    delete_dentry(tp, key_hash); // delete current d-entry
                    insert_ientry(tp, key_hash); // insert it as i-entry
                    tp->evictions++;
                } else{
                    tp->d_entries[idx].num_slabs = slabs_needed;
                    tp->d_entries[idx].klen = klen;
                    tp->d_entries[idx].vlen = vlen;
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
                if(tp->d_entry_slabs + tp->i_entry_count + slabs_needed - 1 >= tp->tt_slab){
                    return true; // not enough space
                }
                delete_ientry(tp, key_hash); // delete current entry
                insert_dentry(tp, key_hash, klen, vlen, key, val); // insert it as d-entry
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
        } 
        else{
            ret = insert_dentry_by_eviction(tp, key_hash, klen, vlen, key, val);
            if(ret){
                return true;
            } else{
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
bool insert_dentry(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val) {
    int slabs_needed = ceil((klen + vlen)/tp->slab_size);
    if (tp->d_entry_slabs + tp->i_entry_count + slabs_needed >= tp->tt_slab){
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
bool insert_dentry_by_eviction(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val) {
    int slabs_needed = ceil((klen + vlen)/tp->slab_size);
    // Look for D-entry of greater size to evict
    for(int i = 0; i < tp->dentry_idx; i++){
        if(tp->d_entries[i].num_slabs > slabs_needed){
            unsigned long long evict_key_hash = tp->d_entries[i].key_hash;
            delete_dentry(tp, evict_key_hash);
            insert_dentry(tp, key_hash, klen, vlen, key, val);
            insert_ientry(tp, evict_key_hash);
            tp->evictions++;
            return true;
        }
    }

    // no entries of greater size to evict
    return false;
}

// SHOULD BE DONE
bool insert_ientry(TranslationPage *tp, unsigned long long key_hash) {
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
bool find_value_by_key_hash(TranslationPage *tp, unsigned long long key_hash, const char *key) {
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
bool delete_dentry(TranslationPage *tp, unsigned long long key_hash) {
    int idx = hashmap_get(tp->key_hashes, key_hash);  // -2 = not found, -1 = ientry
    if (idx == NOT_FOUND || idx == -1) {
        return false;  // Not a Dentry
    }

    int num_slabs = tp->d_entries[idx].num_slabs;

    // Delete dentry from dentries
    for (int i = idx; i < tp->d_entry_slabs - 1; i++) 
        tp->d_entries[i] = tp->d_entries[i + 1]; 

    // Remove key_hash from the hash map
    hashmap_delete(tp->key_hashes, key_hash);  

    // update indexes in key_hashes
    update_key_hashes(tp);

    tp->d_entry_slabs -= num_slabs; 
    tp->dentry_idx--;

    return true;  // Deletion successful
}

// SHOULD BE DONE
bool delete_ientry(TranslationPage *tp, unsigned long long key_hash) {
    if (hash_set_contains(tp->i_entries, key_hash)) {  // if key_hash in self.i_entries: (checks if key_hash is in Ientries)
        hash_set_delete(tp->i_entries, key_hash);  // self.i_entries.remove(key_hash) (remove key_hash from i_entries)
        hashmap_delete(tp->key_hashes, key_hash);  // del self.key_hashes[key_hash] (delete key_hash from key_hashes)
        tp->i_entry_count--;  
        return true;  // Deletion successful
    }

    return false;  // Nothing to delete
}


int main() {
    // 25 slabs, 100b threshold
    TranslationPage *tp = create_translation_page(500, 20, 100);

    unsigned long long key_hash1 = 1ULL;
    unsigned long long key_hash2 = 2ULL;
    unsigned long long key_hash3 = 3ULL;
    unsigned long long key_hash4 = 4ULL;
    unsigned long long key_hash5 = 5ULL;


    insert(tp, key_hash1, 10, 20, "example_key", 42);

    printf("%d", tp->d_entries[0].num_slabs);

    // Clean up resources before exiting (free memory, etc.)
    free(tp->d_entries);
    free(tp->i_entries->table);
    free(tp->i_entries);
    free(tp->key_hashes->table);
    free(tp->key_hashes);
    free(tp);

    return 0; // Exit status
}