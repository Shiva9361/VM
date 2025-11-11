#ifndef COMMON_H
#define COMMON_H

#include <stddef.h> // For size_t
#include <stdint.h> // For fixed-width integers

// Define boolean type for C
typedef enum
{
    false = 0,
    true = 1
} bool;

// Basic error handling macro
#define ERROR_PRINT(...) printf(__VA_ARGS__)

#endif // COMMON_H
