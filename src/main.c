/**
 * Author: Shivadharshan S
 */
#include <stdint.h>
#include <stdlib.h> // For malloc, free
#include <string.h> // For memcpy
// #include <stdarg.h> // Removed

// System standard library includes
#include <stdio.h>   // For vsnprintf, fprintf, stderr
// #include <stdlib.h> // Already included above for malloc, free, exit
// #include <string.h>  // Already included above for memcpy, memset
// #include <stdarg.h>  // Removed
#include <sys/stat.h>  // For struct stat

// Include user-space syscall headers from OS project
#include <sys_calls.h> // Changed from relative path
#include <unistd.h>    // Changed from relative path

#include "include/VM.h" // Use the new C header

// Define a simple fprintf-like function using sys_write to stderr (fd 2)
// This is now vm_debug_printf if VM_DEBUG is defined, otherwise a local vm_fprintf_stderr
#ifndef VM_DEBUG
static void vm_fprintf_stderr(const char *msg) { // Simplified to take a single string
    write(2, msg, strlen(msg)); // Use sys_write to stderr (fd 2)
    write(2, "\n", 1); // Add newline
}
#else
#define vm_fprintf_stderr vm_debug_printf
#endif


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        vm_fprintf_stderr("Usage: <vm_binary_file>"); // Simplified message
        _exit(1); // Use syscall exit
    }

    const char *filename = argv[1];
    int fd = -1;
    uint8_t *filedata_buffer = NULL;
    long size = 0;

    // Open file using syscall
    fd = open(filename, O_RDONLY, 0); // O_RDONLY from sys_calls.h
    if (fd < 0)
    {
        vm_fprintf_stderr("Error: Cannot open file"); // Simplified message
        close(fd);
        _exit(1);
    }

    // Get file size using fstat syscall
    struct stat st;
    if (fstat(fd, &st) < 0) {
        vm_fprintf_stderr("Error: Failed to get file status"); // Simplified message
        close(fd);
        _exit(1);
    }
    size = st.st_size;

    if (size <= 0)
    {
        vm_fprintf_stderr("Error: File is empty or invalid size"); // Simplified message
        close(fd);
        _exit(1);
    }

    filedata_buffer = (uint8_t *)malloc(size);
    if (!filedata_buffer) {
        vm_fprintf_stderr("Error: Failed to allocate memory for file data"); // Simplified message
        close(fd);
        _exit(1);
    }

    // Read file using syscall
    if (read(fd, filedata_buffer, size) != size)
    {
        vm_fprintf_stderr("Error: Failed to read file"); // Simplified message
        free(filedata_buffer);
        close(fd);
        _exit(1);
    }
    close(fd); // Close file using syscall

    VM_State vm;
    // Initialize VM and load binary
    VM_init(&vm, filedata_buffer, (uint32_t)size);
    
    // Run the VM
    VM_run(&vm);

    // Clean up
    VM_destroy(&vm);
    free(filedata_buffer);

    _exit(0); // Use syscall exit
    return 0; // Should not be reached
}