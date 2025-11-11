#include <string.h> // Must be first for memcpy declarations
#include "hash_map.h"
#include <stdlib.h> // For malloc, free
#include <stdio.h>  // For fprintf, stderr

#define INITIAL_HASH_MAP_CAPACITY 16
#define LOAD_FACTOR_THRESHOLD 0.75

// Simple string hashing function (DJB2)
static unsigned long hash_string(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

static bool hash_map_resize(hash_map_t* map, size_t new_capacity);

void hash_map_init(hash_map_t* map, size_t value_size) {
    map->buckets = (hash_map_entry_t**)calloc(INITIAL_HASH_MAP_CAPACITY, sizeof(hash_map_entry_t*));
    if (!map->buckets) {
        ERROR_PRINT("Failed to allocate memory for hash map buckets.\n");
        map->capacity = 0;
        map->size = 0;
        map->value_size = 0;
        return;
    }
    map->capacity = INITIAL_HASH_MAP_CAPACITY;
    map->size = 0;
    map->value_size = value_size;
}

void hash_map_destroy(hash_map_t* map) {
    for (size_t i = 0; i < map->capacity; ++i) {
        hash_map_entry_t* entry = map->buckets[i];
        while (entry) {
            hash_map_entry_t* next = entry->next;
            string_destroy(&entry->key);
            free(entry->value); // Free the value data
            free(entry);
            entry = next;
        }
    }
    free(map->buckets);
    map->buckets = NULL;
    map->capacity = 0;
    map->size = 0;
}

static bool hash_map_put_internal(hash_map_t* map, const string_t* key, const void* value, bool resize_check) {
    if (resize_check && (double)map->size / map->capacity > LOAD_FACTOR_THRESHOLD) {
        if (!hash_map_resize(map, map->capacity * 2)) {
            return false;
        }
    }

    unsigned long hash = hash_string(string_cstr(key));
    size_t index = hash % map->capacity;

    hash_map_entry_t* entry = map->buckets[index];
    while (entry) {
        if (string_compare(&entry->key, key)) {
            // Key already exists, update value
            memcpy(entry->value, value, map->value_size);
            return true;
        }
        entry = entry->next;
    }

    // Key not found, create new entry
    hash_map_entry_t* new_entry = (hash_map_entry_t*)malloc(sizeof(hash_map_entry_t));
    if (!new_entry) {
        ERROR_PRINT("Failed to allocate memory for hash map entry.\n");
        return false;
    }
    string_init(&new_entry->key);
    if (!string_copy(&new_entry->key, key)) {
        free(new_entry);
        return false;
    }
    new_entry->value = malloc(map->value_size);
    if (!new_entry->value) {
        ERROR_PRINT("Failed to allocate memory for hash map value.\n");
        string_destroy(&new_entry->key);
        free(new_entry);
        return false;
    }
    memcpy(new_entry->value, value, map->value_size);
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;
    return true;
}

bool hash_map_put(hash_map_t* map, const string_t* key, const void* value) {
    return hash_map_put_internal(map, key, value, true);
}

void* hash_map_get(const hash_map_t* map, const string_t* key) {
    unsigned long hash = hash_string(string_cstr(key));
    size_t index = hash % map->capacity;

    hash_map_entry_t* entry = map->buckets[index];
    while (entry) {
        if (string_compare(&entry->key, key)) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

bool hash_map_remove(hash_map_t* map, const string_t* key) {
    unsigned long hash = hash_string(string_cstr(key));
    size_t index = hash % map->capacity;

    hash_map_entry_t* entry = map->buckets[index];
    hash_map_entry_t* prev = NULL;

    while (entry) {
        if (string_compare(&entry->key, key)) {
            if (prev) {
                prev->next = entry->next;
            } else {
                map->buckets[index] = entry->next;
            }
            string_destroy(&entry->key);
            free(entry->value);
            free(entry);
            map->size--;
            return true;
        }
        prev = entry;
        entry = entry->next;
    }
    return false; // Key not found
}

size_t hash_map_size(const hash_map_t* map) {
    return map->size;
}

static bool hash_map_resize(hash_map_t* map, size_t new_capacity) {
    hash_map_entry_t** new_buckets = (hash_map_entry_t**)calloc(new_capacity, sizeof(hash_map_entry_t*));
    if (!new_buckets) {
        ERROR_PRINT("Failed to allocate memory for hash map resize.\n");
        return false;
    }

    // Store old capacity and buckets
    size_t old_capacity = map->capacity;
    hash_map_entry_t** old_buckets = map->buckets;

    // Temporarily set new capacity and buckets for re-hashing
    map->capacity = new_capacity;
    map->buckets = new_buckets;
    map->size = 0; // Reset size, will be re-counted by put_internal

    // Re-insert all existing entries into new buckets
    for (size_t i = 0; i < old_capacity; ++i) {
        hash_map_entry_t* entry = old_buckets[i];
        while (entry) {
            hash_map_entry_t* next = entry->next;
            // Re-insert without checking for resize again
            hash_map_put_internal(map, &entry->key, entry->value, false);
            string_destroy(&entry->key); // Destroy old key
            free(entry->value); // Free old value
            free(entry); // Free old entry
            entry = next;
        }
    }
    free(old_buckets);
    return true;
}
