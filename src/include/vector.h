#ifndef VECTOR_H
#define VECTOR_H

#include "common.h" // For bool, size_t

typedef struct {
    void* data;
    size_t element_size;
    size_t size;
    size_t capacity;
} vector_t;

// Function declarations
void vector_init(vector_t* vec, size_t element_size);
void vector_destroy(vector_t* vec);
bool vector_push_back(vector_t* vec, const void* value);
bool vector_pop_back(vector_t* vec, void* out_value); // Optional: retrieves popped value
void* vector_get(const vector_t* vec, size_t index);
void* vector_get_ptr(const vector_t* vec, size_t index); // Returns pointer to element
bool vector_set(vector_t* vec, size_t index, const void* value);
size_t vector_size(const vector_t* vec);
bool vector_reserve(vector_t* vec, size_t new_capacity);
bool vector_resize(vector_t* vec, size_t new_size);
void vector_clear(vector_t* vec);

#endif // VECTOR_H
