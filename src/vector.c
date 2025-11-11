#include <string.h> // Must be first for memcpy/memset declarations
#include "vector.h"
#include <stdlib.h> // For malloc, realloc, free
#include <stdio.h>  // For fprintf, stderr

#define INITIAL_CAPACITY 4

void vector_init(vector_t *vec, size_t element_size)
{
    vec->data = NULL;
    vec->element_size = element_size;
    vec->size = 0;
    vec->capacity = 0;
}

void vector_destroy(vector_t *vec)
{
    if (vec->data)
    {
        free(vec->data);
        vec->data = NULL;
    }
    vec->size = 0;
    vec->capacity = 0;
}

static bool vector_grow(vector_t *vec)
{
    size_t new_capacity = (vec->capacity == 0) ? INITIAL_CAPACITY : vec->capacity * 2;
    void *new_data = realloc(vec->data, new_capacity * vec->element_size);
    if (!new_data)
    {
        ERROR_PRINT("Failed to reallocate memory for vector.\n");
        return false;
    }
    vec->data = new_data;
    vec->capacity = new_capacity;
    return true;
}

bool vector_push_back(vector_t *vec, const void *value)
{
    if (vec->size == vec->capacity)
    {
        if (!vector_grow(vec))
        {
            return false;
        }
    }
    memcpy((char *)vec->data + vec->size * vec->element_size, value, vec->element_size);
    vec->size++;
    return true;
}

bool vector_pop_back(vector_t *vec, void *out_value)
{
    if (vec->size == 0)
    {
        return false;
    }
    vec->size--;
    if (out_value)
    {
        memcpy(out_value, (char *)vec->data + vec->size * vec->element_size, vec->element_size);
    }
    return true;
}

void *vector_get(const vector_t *vec, size_t index)
{
    if (index >= vec->size)
    {
        ERROR_PRINT("Vector index out of bounds: %zu (size: %zu)\n", index, vec->size);
        return NULL;
    }
    // Return a copy of the element
    void *temp = malloc(vec->element_size);
    if (temp)
    {
        memcpy(temp, (char *)vec->data + index * vec->element_size, vec->element_size);
    }
    return temp;
}

void *vector_get_ptr(const vector_t *vec, size_t index)
{
    if (index >= vec->size)
    {
        ERROR_PRINT("Vector index out of bounds: %zu (size: %zu)\n", index, vec->size);
        return NULL;
    }
    return (char *)vec->data + index * vec->element_size;
}

bool vector_set(vector_t *vec, size_t index, const void *value)
{
    if (index >= vec->size)
    {
        ERROR_PRINT("Vector index out of bounds: %zu (size: %zu)\n", index, vec->size);
        return false;
    }
    memcpy((char *)vec->data + index * vec->element_size, value, vec->element_size);
    return true;
}

size_t vector_size(const vector_t *vec)
{
    return vec->size;
}

bool vector_reserve(vector_t *vec, size_t new_capacity)
{
    if (new_capacity <= vec->capacity)
    {
        return true;
    }
    void *new_data = realloc(vec->data, new_capacity * vec->element_size);
    if (!new_data)
    {
        ERROR_PRINT("Failed to reallocate memory for vector reserve.\n");
        return false;
    }
    vec->data = new_data;
    vec->capacity = new_capacity;
    return true;
}

bool vector_resize(vector_t *vec, size_t new_size)
{
    if (new_size == vec->size)
    {
        return true;
    }
    if (new_size > vec->capacity)
    {
        if (!vector_reserve(vec, new_size))
        {
            return false;
        }
    }
    if (new_size > vec->size)
    {
        // Initialize new elements to zero
        // memset((char*)vec->data + vec->size * vec->element_size, 0, (new_size - vec->size) * vec->element_size);
    }
    vec->size = new_size;
    // printf("Vector resized inside");
    return true;
}

void vector_clear(vector_t *vec)
{
    vec->size = 0;
}
