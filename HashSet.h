// HashSet.h
#ifndef HASHSET_H
#define HASHSET_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct SetEntry {
    int key_hash;
    struct SetEntry *next;    
} SetEntry;

typedef struct {
    SetEntry **buckets;        
    int capacity;
    int size;
} HashSet;

unsigned int hash_s(int key, int capacity);
HashSet *hashset_create(int capacity);
void hashset_insert(HashSet *set, int key_hash);
bool hashset_contains(HashSet *set, int key_hash);
void hashset_delete(HashSet *set, int key_hash);

#endif
