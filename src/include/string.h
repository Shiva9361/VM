#ifndef STRING_H
#define STRING_H

#include "common.h" // For size_t

typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} string_t;

// Function declarations
void string_init(string_t* str);
void string_init_from_cstr(string_t* str, const char* cstr);
void string_destroy(string_t* str);
bool string_append_cstr(string_t* str, const char* cstr);
bool string_append_char(string_t* str, char c);
const char* string_cstr(const string_t* str);
size_t string_length(const string_t* str);
bool string_compare(const string_t* str1, const string_t* str2);
bool string_compare_cstr(const string_t* str1, const char* cstr2);
bool string_copy(string_t* dest, const string_t* src);
bool string_reserve(string_t* str, size_t new_capacity);

#endif // STRING_H
