// HashMap.h
#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NOT_FOUND -2  // Define constant for not found cases

typedef struct Entry {
    int key_hash;  
    int index; // index in d_entries array or -1 for all i_entries
    struct Entry* next;  
} Entry;

typedef struct {
    Entry** entries;
    int capacity; // Number of buckets in the hash map
    int count; // Number of key-value pairs stored in the hash map
} HashMap;

unsigned int hash(unsigned long long key_hash, int capacity);
HashMap* create_hashmap(int capacity);
void hashmap_put(HashMap* map, unsigned long long key_hash, int index);
int hashmap_get(HashMap* map, unsigned long long key_hash);
bool hashmap_contains(HashMap* map, unsigned long long key_hash);
void hashmap_delete(HashMap* map, unsigned long long key_hash);

#endif
