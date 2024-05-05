// Maps key_hash : index (in d_entries array)
#include "HashMap.h"

// Simple hash function to map key_hash to a bucket
unsigned int hash(int key_hash, int capacity) {
    return key_hash % capacity;
}

// Initialize the hashmap
HashMap* create_hashmap(int capacity) {
    HashMap* map = malloc(sizeof(HashMap));
    map->capacity = capacity;
    map->count = 0;

    map->entries = malloc(sizeof(Entry*) * capacity);
    for (int i = 0; i < capacity; i++) {
        map->entries[i] = NULL;
    }
    return map;
}

// Insert or update an entry in the hashmap
void put(HashMap* map, int key_hash, int index) {
    unsigned int bucketIndex = hash(key_hash, map->capacity);
    Entry* entry = map->entries[bucketIndex];

    // Search for existing entry and update
    while (entry != NULL) {
        if (entry->key_hash == key_hash) {
            entry->index = index;
            return;
        }
        entry = entry->next;
    }

    // Create new entry if not found
    Entry* new_entry = malloc(sizeof(Entry));
    new_entry->key_hash = key_hash;
    new_entry->index = index;
    new_entry->next = map->entries[bucketIndex];
    map->entries[bucketIndex] = new_entry;

    map->count++;
}

// Retrieve an index from the hashmap
int get(HashMap* map, int key_hash) {
    unsigned int bucket = hash(key_hash, map->capacity);
    Entry* entry = map->entries[bucket];

    while (entry != NULL) {
        if (entry->key_hash == key_hash) {
            return entry->index;
        }
        entry = entry->next;
    }

    return NOT_FOUND; // returns -2 if not found because -1 is index of Ientry
}

void delete(HashMap* map, int key_hash) {
    unsigned int bucketIndex = hash(key_hash, map->capacity);
    Entry* entry = map->entries[bucketIndex];
    Entry* prev = NULL;

    // Search for the entry to delete
    while (entry != NULL) {
        if (entry->key_hash == key_hash) {
            if (prev == NULL) {
                // The entry to delete is the first in the list
                map->entries[bucketIndex] = entry->next;
            } else {
                // The entry to delete is not the first
                prev->next = entry->next;
            }
            free(entry); // Free the memory of the entry
            map->count--; // Decrement the count of entries in the hashmap
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}