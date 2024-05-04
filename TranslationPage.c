#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <stdbool.h> 
#include <assert.h> 

#include "HashSet.c" // Ientries array
#include "HashMap.c" // Key_hashes
#include "Dentry.h" // Dentry defintion

typedef struct {
    int slab_size;
    int tt_slab;
    int *entry_type; // [-1,-1,-1,1-1...]

    DEntry *d_entries; //  [ {key_hash1, type1, klen1, vlen1, key1, va1} , {key_hash2, type2, klen2, vlen2, key2, va2} ...]
    HashSet *i_entries;   // [key_hash1, key_hash2, key_hash3....]
    HashMap *key_hashes;  // { (key_hash1 : index1) , (key_hash2 : index2)...}

    int i_entry_size;
    int effective_slab_size;

    // Counters
    int d_entry_slabs;
    int i_entry_slabs;
    int i_entry_count;
    int evictions;
    int updates;
    int inserts;
    int rejections;
    int new_i_entry;
    int new_d_entry;
    int update_d_entry;
    int update_i_entry;
    int read_d_entry;
    int read_i_entry;
    int i_entry_fn_called;
} TranslationPage;

// Function Prototypes
bool insert_dentry_in_empty_slab(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val);
bool insert_dentry_by_eviction(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val);
bool insert_ientry(TranslationPage *tp, unsigned long long key_hash, bool existing_entry);

// TranslationPage Constructor:
TranslationPage* create_translation_page(int page_size, int slab_size, int i_entry_called) {
    TranslationPage *tp = malloc(sizeof(TranslationPage));
    if (!tp) return NULL;

    tp->slab_size = slab_size; // Currently set to 170
    tp->tt_slab = page_size / slab_size;

    tp->entry_type = malloc(sizeof(int) * tp->tt_slab);
    // Initialize entry_type array
    for (int i = 0; i < tp->tt_slab; i++) 
        tp->entry_type[i] = -1;
    

    tp->d_entries = calloc(tp->tt_slab, sizeof(DEntry));// Allocate or initialize depending on how d_entries is supposed to be structured
    tp->i_entries = hashset_create(tp->tt_slab);  // Same capacity as tt_slab for simplicity
    tp->key_hashes = create_hashmap(tp->tt_slab);

    tp->i_entry_size = 16 + 4;  
    tp->effective_slab_size = slab_size - 1 - 16; 

    tp->d_entry_slabs = 0;
    tp->i_entry_slabs = 0;
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
    tp->i_entry_fn_called = i_entry_called;

    return tp;
}

// Comparison function for sorting DEntry by key_hash
int compare_dentries(const void *a, const void *b) {
    const DEntry *entry1 = (const DEntry *)a;
    const DEntry *entry2 = (const DEntry *)b;
    if (entry1->key_hash < entry2->key_hash) return -1;
    if (entry1->key_hash > entry2->key_hash) return 1;
    return 0;
}

// Updates indexes of key_hashes to reflect change in d_entries array
void update_key_hashes(TranslationPage *tp) {
    for (int i = 0; i < tp->tt_slab; i++) {
        put(tp->key_hashes, tp->d_entries[i].key_hash, i);
    }
}

// Sorts Dentries array and updates key_hashes
void sort_dentries(TranslationPage *tp) {
    qsort(tp->d_entries, tp->tt_slab, sizeof(DEntry), compare_dentries);

    update_key_hashes(tp);
}


bool insert(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val) {
    int slab = get(tp->key_hashes, key_hash);  // returns -2 for not found, -1 for Ientry, Index for Dentry
    if(slab != NOT_FOUND){ // if key_hash in self.key_hashes (checks whether key_hash exists)
        if (slab != -1 && slab < tp->d_entry_slabs) { // if slab != -1 and slab < self.d_entry_slabs (key is a Dentry)
            assert(tp->entry_type[slab] == 0); // Confirm slab is a Dentry

            if (klen + vlen <= tp->effective_slab_size) {
                DEntry *d_entry = &tp->d_entries[slab];

                // Assert conditions
                assert(tp->entry_type[slab] == 0);
                assert(d_entry->type == 0);
                assert(d_entry->key_hash == key_hash);
                assert(strncmp(d_entry->key, key, klen) == 0); // assert(d_entry['key'] == key) (string comparison)

                // Update the d-entry value
                d_entry->val = val;
                tp->updates++;
                tp->update_d_entry++;
                return true;
            } 
            
            // If new KVP size > effective_entry_size, try inserting it as an I-entry and then evict the old D-entry
            else { 
                bool ret = insert_ientry(tp, key_hash, true);  // existing_entry  set to true
                if (ret) {
                    assert(strncmp(tp->d_entries[slab].key, key, klen) == 0);
                    assert(tp->d_entries[slab].key_hash == key_hash);

                    // self.d_entries.pop(slab) (Evict the old D-entry)
                    for (int i = slab; i < tp->d_entry_slabs - 1; i++) 
                        tp->d_entries[i] = tp->d_entries[i + 1];

                    
                    // Sort the d-entries to maintain order and update key_hashes
                    sort_dentries(tp);
                    
                    tp->d_entry_slabs--;
                    tp->entry_type[tp->d_entry_slabs] = -1;  // Mark the last entry type as unused
                    
                    tp->updates++;
                    tp->evictions++;
                    return true;
                }
            }
        }
        else{
            assert(tp->entry_type[slab] == 1);
            assert(hashset_contains(tp->i_entries, key_hash)); // assert(key_hash in self.i_entries)
            tp->updates++;
            tp->update_i_entry++;
            return true;
        }
    }

    else if(klen + vlen <= tp->effective_slab_size){
        bool ret = insert_dentry_in_empty_slab(tp, key_hash, klen, vlen, key, val);
        if (ret) { // New D-entry successfully created
            tp->inserts++;
            tp->new_d_entry++;
            return true;
        } else{
            ret = insert_dentry_by_eviction(tp, key_hash, klen, vlen, key, val);
            if(ret){
                tp->new_d_entry++;
                tp->evictions++;
                tp->inserts++;
                return true;
            } else{
                ret = insert_ientry(tp, key_hash, false);
                if (ret) {
                    tp->inserts++;
                    return true;
                }
            }
        }
    }

    else { // New i-entry. KVP > effective_slab_size
        // Ensure key_hash is not already an I-entry
        assert(!hashset_contains(tp->i_entries, key_hash));

        // Attempt to insert a new I-entry
        bool ret = insert_ientry(tp, key_hash, false); // existing_entry set to false
        if (ret) {
            tp->new_i_entry++; // Increment the count of new I-entries
            tp->inserts++; // Increment the total number of insertions
            return true; // Indicate success
        }
    }

    tp->rejections++;
    return false;
}


// Function to insert a DEntry in an empty slab
bool insert_dentry_in_empty_slab(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val) {
    // Check if there is room to add a new DEntry
    if (tp->d_entry_slabs + tp->i_entry_slabs == tp->tt_slab) 
        return false;

    int slab = tp->d_entry_slabs; // Get new slab index

    // Update the key_hashes HashMap
    put(tp->key_hashes, key_hash, slab);

    // Ensure the DEntry at 'slab' index is initialized 
    DEntry *entry = &tp->d_entries[slab];

    entry->key_hash = key_hash;
    entry->type = 0; // A
    entry->klen = klen;
    entry->vlen = vlen;
    strncpy(entry->key, key, klen); // Copy key to entry, assuming 'key' is char array
    entry->val = -1; //set val to -1 for all Dentries

    // Set entry type for this slab
    tp->entry_type[slab] = 0;

    // Sort d_entries if more than one entry exists
    if (tp->d_entry_slabs > 1) 
        sort_dentries(tp); 

    tp->d_entry_slabs++; // Increment the count of d_entries slabs

    // Assert to check that the total number of slabs doesn't exceed capacity
    assert(tp->d_entry_slabs + tp->i_entry_slabs <= tp->tt_slab);

    return true;
}

// insert d_entry by eviction
bool insert_dentry_by_eviction(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val) {
    // Ensure that we are at capacity
    assert(tp->d_entry_slabs + tp->i_entry_slabs == tp->tt_slab);

    // Try to evict an existing d-entry and reuse its slab for a new d-entry
    for (int i = 0; i < tp->d_entry_slabs; i++) {
        DEntry *d_entry = &tp->d_entries[i];
        
        // Try inserting the existing d_entry's key_hash into an i_entry
        if (insert_ientry(tp, d_entry->key_hash, true)) {
            // Eviction successful, now repurpose this d_entry slab for the new key
            d_entry->key_hash = key_hash;
            strncpy(d_entry->key, key, klen);  
            d_entry->klen = klen;
            d_entry->vlen = vlen;

            // Update the key_hashes map
            put(tp->key_hashes, key_hash, i);
            // Sort d_entries if needed
            sort_dentries(tp);
            // Update the entry_type to mark the old one as no longer a d_entry
            tp->entry_type[tp->d_entry_slabs - 1] = -1;
            tp->evictions++;
            printf("Eviction: New i_entry added\n");
            return true;
        }
    }

    return false;
}

// int PPA and boolean eviction are never used so I didn't include them in my code
bool insert_ientry(TranslationPage *tp, unsigned long long key_hash, bool existing_entry) {
    // Check if the entry is new and assert key_hash not in key_hashes
    if (!existing_entry) 
        assert(get(tp->key_hashes, key_hash) == NOT_FOUND); // assert(key_hash not in self.key_hashes)
    
    int i_entry_per_slab = tp->slab_size / tp->i_entry_size;

    // Check if all i-entry slabs are utilized
    if (tp->i_entry_slabs * i_entry_per_slab == tp->i_entry_count) {
        // Check if all slabs in the page are utilized
        if (tp->d_entry_slabs + tp->i_entry_slabs == tp->tt_slab) {
            return false;
        } else {
            // Allocate new slabs for i-entries
            tp->i_entry_slabs++;
            tp->entry_type[tp->tt_slab - tp->i_entry_slabs] = 1; // Set new slab type to 1
        }
    }

    assert(tp->d_entry_slabs + tp->i_entry_slabs <= tp->tt_slab); // Ensure we don't exceed total slab count

    // Update the key_hashes to indicate that this key_hash is an i_entry (-1)
    put(tp->key_hashes, key_hash, -1);
    // Add key_hash to i_entries set
    hashset_insert(tp->i_entries, key_hash);
    tp->i_entry_count++;

    return true;
}



int main(){
    printf("hello word");

    return 1;
}