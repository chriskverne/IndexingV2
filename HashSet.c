// HashSet that stores Ientries. (Each Ientry is only storing the key_hash) Ientries =[key_hash1, key_hash2,....]
#include <stdlib.h>  
#include <stdio.h>
#include <stdbool.h>    

typedef struct SetEntry {
    int key_hash; // key_hash of Ientry
    struct SetEntry *next;    
} SetEntry;

typedef struct {
    SetEntry **buckets;        
    int capacity; // max number of Ientries
    int size; // current number of Ientries
} HashSet;

unsigned int hash_s(int key, int capacity) {
    return key % capacity;  
}

HashSet *hashset_create(int capacity) {
    HashSet *set = malloc(sizeof(HashSet));
    if (set == NULL) return NULL;

    set->capacity = capacity;
    set->size = 0;
    set->buckets = malloc(sizeof(SetEntry *) * capacity);
    if (set->buckets == NULL) {
        free(set);
        return NULL;
    }

    for (int i = 0; i < capacity; i++) {
        set->buckets[i] = NULL;
    }

    return set;
}

void hashset_insert(HashSet *set, int key_hash) {
    unsigned int bucketIndex = hash_s(key_hash, set->capacity);
    SetEntry *entry = set->buckets[bucketIndex];

    // Check if the item already exists, prevent duplicate entries
    while (entry != NULL) {
        if (entry->key_hash == key_hash) return;  // Already in the set
        entry = entry->next;
    }

    // Insert new item
    SetEntry *newEntry = malloc(sizeof(SetEntry));
    if (newEntry == NULL) return;

    newEntry->key_hash = key_hash;
    newEntry->next = set->buckets[bucketIndex];
    set->buckets[bucketIndex] = newEntry;
    set->size++;
}

bool hashset_contains(HashSet *set, int key_hash) {
    unsigned int bucketIndex = hash_s(key_hash, set->capacity);
    SetEntry *entry = set->buckets[bucketIndex];

    while (entry != NULL) {
        if (entry->key_hash == key_hash) {
            return true;  // Found
        }
        entry = entry->next;
    }

    return false;  // Not found
}

void hashset_delete(HashSet *set, int key_hash) {
    unsigned int bucketIndex = hash_s(key_hash, set->capacity);
    SetEntry *entry = set->buckets[bucketIndex];
    SetEntry *prev = NULL;

    // Traverse the linked list to find the entry
    while (entry != NULL) {
        if (entry->key_hash == key_hash) {
            if (prev == NULL) {
                // The entry to delete is the first in the list
                set->buckets[bucketIndex] = entry->next;
            } else {
                // The entry to delete is not the first
                prev->next = entry->next;
            }
            free(entry); // Free the memory of the entry
            set->size--; // Decrement the size of the set
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}



