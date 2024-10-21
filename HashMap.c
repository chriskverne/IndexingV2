#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NOT_FOUND -2 // Special marker for a key not found

// entry structure
typedef struct {
    unsigned long long key_hash;
    int value; // -1 or index in d_entries array
    bool is_occupied;
} HashMapEntry;

// Hashmap structure
typedef struct {
    HashMapEntry *table;
    int size;
} HashMap;

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

/*
// Main function to demonstrate usage
int main() {
    // Create a hashmap
    HashMap *map = create_hashmap(5);

    // Example usage: insert key_hash -> value pairs
    unsigned long long key_hash1 = 123456789ULL;
    unsigned long long key_hash2 = 987654321ULL;

    hashmap_put(map, key_hash1, 10);  // 10 is an index in d_entries
    hashmap_put(map, key_hash2, -1);  // -1 indicates an I_entry

    // Retrieve the value for key_hash1
    int value1 = hashmap_get(map, key_hash1);
    if (value1 != NOT_FOUND) {
        printf("Key hash 1 maps to value: %d\n", value1);
    } else {
        printf("Key hash 1 not found.\n");
    }

    // Retrieve the value for key_hash2
    int value2 = hashmap_get(map, key_hash2);
    if (value2 != NOT_FOUND) {
        printf("Key hash 2 maps to value: %d\n", value2);  // Should print -1 for I_entry
    } else {
        printf("Key hash 2 not found.\n");
    }

    // Delete key_hash1 and check again
    hashmap_delete(map, key_hash1);
    if (hashmap_get(map, key_hash1) == NOT_FOUND) {
        printf("Key hash 1 successfully deleted.\n");
    }

    printf("key_hash 1 maps to: %d\n", hashmap_get(map, key_hash1));
    printf("key_hash 2 maps to: %d\n", hashmap_get(map, key_hash2));


    // Free allocated memory
    free(map->table);
    free(map);

    return 0;
}
*/