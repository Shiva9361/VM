#ifndef HASH_MAP_H
#define HASH_MAP_H

#include "common.h"
#include "string.h" // For string_t

// Hash map entry
typedef struct hash_map_entry {
    string_t key;
    void* value;
    struct hash_map_entry* next; // For separate chaining
} hash_map_entry_t;

// Hash map structure
typedef struct {
    hash_map_entry_t** buckets;
    size_t capacity;
    size_t size;
    size_t value_size; // Size of the value being stored
} hash_map_t;

// Function declarations
void hash_map_init(hash_map_t* map, size_t value_size);
void hash_map_destroy(hash_map_t* map);
bool hash_map_put(hash_map_t* map, const string_t* key, const void* value);
void* hash_map_get(const hash_map_t* map, const string_t* key);
bool hash_map_remove(hash_map_t* map, const string_t* key);
size_t hash_map_size(const hash_map_t* map);

#endif // HASH_MAP_H
