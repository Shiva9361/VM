/**
 * Author: Shivadharshan S
 */
#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stdlib.h> // For malloc, free, exit
#include <string.h> // For memcpy, memset
// #include <stdio.h>  // Removed as FILE* is no longer directly managed by VM_State
// #include <stdarg.h> // For va_list - removed, now DBG_PRINTF takes a single string


// Forward declarations for C-style structs
struct ClassInfo;
struct ObjectFactory_State;

#ifdef VM_DEBUG
void vm_debug_printf(const char *msg); // Now takes a single string
#define DBG_PRINTF vm_debug_printf
#else
#define DBG_PRINTF(msg) // nothing
#endif

enum Opcode { // Changed to enum Opcode and then typedef
    IADD = 0x01,
    ISUB = 0x02,
    IMUL = 0x03,
    IDIV = 0x04,
    INEG = 0x05,
    FADD = 0x06,
    FSUB = 0x07,
    FMUL = 0x08,
    FDIV = 0x09,
    FNEG = 0x0A,
    IMOD = 0x0B,
    PUSH = 0x10,
    POP = 0x11,
    DUP = 0x12,
    FPOP = 0x13,
    FPUSH = 0x14,
    LOAD = 0x20,
    STORE = 0x21,
    LOAD_ARG = 0x22,
    JMP = 0x30,
    JZ = 0x31,
    JNZ = 0x32,
    CALL = 0x33,
    RET = 0x34,
    ICMP_EQ = 0x40,
    ICMP_LT = 0x41,
    ICMP_GT = 0x42,
    FCMP_EQ = 0x43,
    FCMP_LT = 0x44,
    FCMP_GT = 0x45,
    ICMP_GEQ = 0x46,
    ICMP_NEQ = 0x47,
    ICMP_LEQ = 0x48,
    FCMP_GEQ = 0x49,
    FCMP_NEQ = 0x4A,
    FCMP_LEQ = 0x4B,
    NEW = 0x50,
    GETFIELD = 0x51,
    PUTFIELD = 0x52,
    INVOKEVIRTUAL = 0x53,
    INVOKESPECIAL = 0x54,
    SYS_CALL = 0x60,
    NEWARRAY = 0x70,
    ALOAD = 0x71,
    ASTORE = 0x72,
};
typedef enum Opcode Opcode; // Explicit typedef

typedef enum {
    OPEN = 0x01,
    READ = 0x02,
    SBRK = 0x03,
    CLOSE = 0x04,
    FSTAT = 0x05,
    LSEEK = 0x06,
    WRITE = 0x07,
    GETPID = 0x09,
    EXIT = 0x0A,
    TIME = 0x0B,
    STAT = 0x0C,
    SYSTEM = 0x0D,
    GETCWD = 0x0E,
    CHDIR = 0x0F,
    RENAME = 0x10,
    UNLINK = 0x11,
    MKDIR = 0x12,
    ISATTY = 0x13,
} Syscall;

// Union for value representation (int or float)
typedef union {
    int32_t intValue;
    float floatValue;
} Value;

#define VM_STACK_SIZE 1024
#define VM_LOCALS_SIZE 256
#define VM_CONST_POOL_SIZE 256
#define VM_MAX_CODE_SIZE 4096 // Assuming a max code size
#define VM_MAX_CLASSES 32
#define VM_MAX_HEAP_OBJECTS 1024
// #define VM_MAX_FILES 10 // Removed as FILE* is no longer directly managed by VM_State

// VM State structure
typedef struct {
    uint32_t stack[VM_STACK_SIZE];
    int32_t stack_ptr; // Points to the next free slot

    uint32_t locals[VM_LOCALS_SIZE];
    uint32_t constantPool[VM_CONST_POOL_SIZE];
    uint32_t constantPool_size; // Actual number of constants loaded

    // For classes, we'll need a way to store ClassInfo structs
    // This will be an array of pointers to ClassInfo, managed by ObjectFactory
    struct ClassInfo* classes[VM_MAX_CLASSES];
    uint32_t classes_count;

    uint8_t code[VM_MAX_CODE_SIZE];
    uint32_t code_size;

    // FILE* fileData[VM_MAX_FILES]; // Removed
    // uint32_t fileData_count;      // Removed

    uint32_t ip; // Instruction Pointer
    uint32_t fp; // Frame Pointer

    uint16_t args_to_pop;

    struct ObjectFactory_State* objectFactory; // Pointer to ObjectFactory state
    void* heap[VM_MAX_HEAP_OBJECTS]; // Replaces std::vector<void*>
    uint32_t heap_count;

} VM_State;

// Function prototypes for VM operations
void VM_init(VM_State* vm, const uint8_t* filedata_buffer, uint32_t filedata_buffer_size);
void VM_destroy(VM_State* vm);
void VM_loadFromBinary(VM_State* vm, const uint8_t* filedata_buffer, uint32_t filedata_buffer_size);
void VM_run(VM_State* vm);
uint32_t VM_top(const VM_State* vm);

// Stack operations
void VM_push(VM_State* vm, uint32_t v);
uint32_t VM_pop(VM_State* vm);
uint32_t VM_peek(const VM_State* vm);

// Bytecode fetching
uint8_t VM_fetch8(VM_State* vm);
uint16_t VM_fetch16(VM_State* vm);
int32_t VM_fetch32(VM_State* vm);

#endif // VM_H
