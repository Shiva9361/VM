#include <string.h> // Must be first for string function declarations
#include "string.h"
#include <stdlib.h> // For malloc, realloc, free
#include <stdio.h>  // For fprintf, stderr

#define INITIAL_STRING_CAPACITY 16

void string_init(string_t* str) {
    str->data = (char*)malloc(INITIAL_STRING_CAPACITY);
    if (!str->data) {
        ERROR_PRINT("Failed to allocate memory for string.\n");
        str->length = 0;
        str->capacity = 0;
        return;
    }
    str->data[0] = '\0';
    str->length = 0;
    str->capacity = INITIAL_STRING_CAPACITY;
}

void string_init_from_cstr(string_t* str, const char* cstr) {
    size_t cstr_len = strlen(cstr);
    str->capacity = cstr_len + 1 > INITIAL_STRING_CAPACITY ? cstr_len + 1 : INITIAL_STRING_CAPACITY;
    str->data = (char*)malloc(str->capacity);
    if (!str->data) {
        ERROR_PRINT("Failed to allocate memory for string from cstr.\n");
        str->length = 0;
        str->capacity = 0;
        return;
    }
    strcpy(str->data, cstr);
    str->length = cstr_len;
}

void string_destroy(string_t* str) {
    if (str->data) {
        free(str->data);
        str->data = NULL;
    }
    str->length = 0;
    str->capacity = 0;
}

static bool string_grow(string_t* str, size_t min_capacity) {
    size_t new_capacity = str->capacity == 0 ? INITIAL_STRING_CAPACITY : str->capacity;
    while (new_capacity < min_capacity) {
        new_capacity *= 2;
    }
    char* new_data = (char*)realloc(str->data, new_capacity);
    if (!new_data) {
        ERROR_PRINT("Failed to reallocate memory for string.\n");
        return false;
    }
    str->data = new_data;
    str->capacity = new_capacity;
    return true;
}

bool string_append_cstr(string_t* str, const char* cstr) {
    size_t cstr_len = strlen(cstr);
    size_t required_capacity = str->length + cstr_len + 1;
    if (required_capacity > str->capacity) {
        if (!string_grow(str, required_capacity)) {
            return false;
        }
    }
    strcat(str->data, cstr);
    str->length += cstr_len;
    return true;
}

bool string_append_char(string_t* str, char c) {
    size_t required_capacity = str->length + 2; // +1 for char, +1 for null terminator
    if (required_capacity > str->capacity) {
        if (!string_grow(str, required_capacity)) {
            return false;
        }
    }
    str->data[str->length] = c;
    str->data[str->length + 1] = '\0';
    str->length++;
    return true;
}

const char* string_cstr(const string_t* str) {
    return str->data;
}

size_t string_length(const string_t* str) {
    return str->length;
}

bool string_compare(const string_t* str1, const string_t* str2) {
    if (str1->length != str2->length) {
        return false;
    }
    return strcmp(str1->data, str2->data) == 0;
}

bool string_compare_cstr(const string_t* str1, const char* cstr2) {
    return strcmp(str1->data, cstr2) == 0;
}

bool string_copy(string_t* dest, const string_t* src) {
    if (dest == src) return true; // Copying to self
    
    size_t required_capacity = src->length + 1;
    if (required_capacity > dest->capacity) {
        if (dest->data) free(dest->data); // Free existing data if capacity needs to change
        dest->data = (char*)malloc(required_capacity);
        if (!dest->data) {
            ERROR_PRINT("Failed to allocate memory for string copy.\n");
            dest->length = 0;
            dest->capacity = 0;
            return false;
        }
        dest->capacity = required_capacity;
    }
    strcpy(dest->data, src->data);
    dest->length = src->length;
    return true;
}

bool string_reserve(string_t* str, size_t new_capacity) {
    if (new_capacity <= str->capacity) {
        return true;
    }
    char* new_data = (char*)realloc(str->data, new_capacity);
    if (!new_data) {
        ERROR_PRINT("Failed to reallocate memory for string reserve.\n");
        return false;
    }
    str->data = new_data;
    str->capacity = new_capacity;
    return true;
}
