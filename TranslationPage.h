#ifndef TRANSLATIONPAGE_H
#define TRANSLATIONPAGE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <stdbool.h> 
#include <assert.h> 

#include "HashSet.c" // Ientries array
#include "HashMap.c" //key_hashes

typedef struct {
    unsigned long long key_hash; 
    char key[256]; 
    int val;
    int klen;
    int vlen;
    int num_slabs;
} DEntry;

typedef struct {
    int threshold;
    int slab_size;
    int tt_slab;

    DEntry *d_entries; //  [ {key_hash1, type1, klen1, vlen1, key1, va1} , {key_hash2, type2, klen2, vlen2, key2, va2} ...]
    HashSet *i_entries;   // [key_hash1, key_hash2, key_hash3....]
    HashMap *key_hashes;  // { (key_hash1 : index1) , (key_hash2 : index2)...}

    int dentry_idx; // Index of D_entry in d_entries (i.e index to insert)

    // Counters
    int d_entry_slabs;
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
} TranslationPage;

// Function Prototypes
TranslationPage* create_translation_page(int page_size, int slab_size, int threshold);

DEntry create_dentry(unsigned long long key_hash, const char *key, int val, int klen, int vlen, int num_slabs);

int compare_dentries(const void *a, const void *b);

void update_key_hashes(TranslationPage *tp);

bool insert(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val);

bool insert_dentry(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val);

bool insert_dentry_by_eviction(TranslationPage *tp, unsigned long long key_hash, int klen, int vlen, const char *key, int val);

bool insert_ientry(TranslationPage *tp, unsigned long long key_hash);

bool find_value_by_key_hash(TranslationPage *tp, unsigned long long key_hash, const char *key);

bool delete_dentry(TranslationPage *tp, unsigned long long key_hash);

bool delete_ientry(TranslationPage *tp, unsigned long long key_hash);

#endif // TRANSLATIONPAGE_H
