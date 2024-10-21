#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    unsigned long long key_hash;
    bool is_occupied;
} HashSetEntry;

typedef struct {
    HashSetEntry *table;
    int size;
} HashSet;

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

/*
// Main function to demonstrate usage
int main() {
    // Assuming page_size and slab_size are defined earlier
    int page_size = 4096;  // Example page size in bytes
    int slab_size = 256;   // Example slab size in bytes

    // Calculate tt_slab as page_size / slab_size
    int tt_slab = page_size / slab_size;

    // Create a hash set with the size equal to tt_slab
    HashSet *set = create_hash_set(tt_slab);

    // Example usage: Insert key_hashes and check their existence
    unsigned long long key_hash1 = 123456789ULL;
    unsigned long long key_hash2 = 987654321ULL;

    hash_set_put(set, key_hash1);
    hash_set_put(set, key_hash2);

    // Check if key_hashes are in the set
    if (hash_set_contains(set, key_hash1)) {
        printf("Key hash 1 found in the set.\n");
    } else {
        printf("Key hash 1 not found in the set.\n");
    }

    if (hash_set_contains(set, key_hash2)) {
        printf("Key hash 2 found in the set.\n");
    }

    // Delete key_hash1 and check again
    hash_set_delete(set, key_hash1);

    if (!hash_set_contains(set, key_hash1)) {
        printf("Key hash 1 successfully deleted.\n");
    }

    printf("Hashset size: %d", set->size);

    // Free allocated memory
    free(set->table);
    free(set);

    return 0;
}
*/