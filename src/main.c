#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "VM.h"
#include "vector.h" // For vector_t

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <vm_binary_file>\n", argv[0]);
        exit(1);
    }

    printf("VM Starting...\n");
    const char *filename = argv[1];
    printf("Loading file: %s\n", filename);

    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("Error: Cannot open file %s\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0)
    {
        printf("Error: File is empty or invalid size\n");
        fclose(file);
        exit(1);
    }

    vector_t filedata_vec;
    vector_init(&filedata_vec, sizeof(uint8_t));

    // printf("Vector Initialized\r\n");
    if (!vector_resize(&filedata_vec, size))
    {
        printf("Error: Failed to allocate memory for file data\n");
        fclose(file);
        exit(1);
    }

    if (fread(vector_get_ptr(&filedata_vec, 0), 1, size, file) != (size_t)size)
    {
        printf("Error: Failed to read file %s\n", filename);
        fclose(file);
        vector_destroy(&filedata_vec);
        exit(1);
    }
    fclose(file);

    VM_t vm;
    vm_init(&vm, &filedata_vec);
    vm_run(&vm);
    // vm_destroy(&vm);

    // vector_destroy(&filedata_vec);

    exit(0);
}
