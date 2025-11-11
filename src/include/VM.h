#ifndef VM_H
#define VM_H

#include "common.h"
#include "vector.h"
#include "object_factory.h" // For ObjectFactory_t and ClassInfo_t

#include <stdio.h>  // For FILE
#include <stdint.h> // For fixed-width integers

#ifdef VM_DEBUG
#define DBG(msg)           \
    do                     \
    {                      \
        printf("%s", msg); \
    } while (0)
#define DBG_UINT(val) printf("%d", val)   // For size_t
#define DBG_UINT32(val) printf("%d", val) // For uint32_t
#define DBG_INT(val) printf("%d", val)
#define DBG_FLOAT(val) printf("%f", val)
#define DBG_STR(val) printf("%s", val)
#define DBG_CHAR(val) printf("%c", val)
#define DBG_NL() printf("\r\n")
#else
#define DBG(msg) // nothing
#define DBG_UINT(val)
#define DBG_UINT32(val)
#define DBG_INT(val)
#define DBG_FLOAT(val)
#define DBG_STR(val)
#define DBG_CHAR(val)
#define DBG_NL()
#endif

typedef enum Opcode_t
{
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
} Opcode_t;

typedef enum Syscall_t
{
    SYS_OPEN = 0x01,
    SYS_READ = 0x02,
    SYS_SBRK = 0x03,
    SYS_CLOSE = 0x04,
    SYS_FSTAT = 0x05,
    SYS_LSEEK = 0x06,
    SYS_WRITE = 0x07,
    SYS_GETPID = 0x09,
    SYS_EXIT = 0x0A,
    SYS_TIME = 0x0B,
    SYS_STAT = 0x0C,
    SYS_SYSTEM = 0x0D,
    SYS_GETCWD = 0x0E,
    SYS_CHDIR = 0x0F,
    SYS_RENAME = 0x10,
    SYS_UNLINK = 0x11,
    SYS_MKDIR = 0x12,
    SYS_ISATTY = 0x13,
} Syscall_t;

typedef union Value_t
{
    int32_t intValue;
    float floatValue;
    // No constructors in C unions, rely on direct assignment
} Value_t;

typedef struct VM_t
{
    vector_t stack;        // vector_t<uint32_t>
    vector_t locals;       // vector_t<uint32_t>
    vector_t constantPool; // vector_t<uint32_t>
    vector_t classes;      // vector_t<ClassInfo_t> - This was std::vector<ClassInfo> in C++
    vector_t code;         // vector_t<uint8_t>
    vector_t fileData;     // vector_t<FILE*>

    uint32_t ip; // Instruction Pointer
    uint32_t sp; // Stack Pointer
    uint32_t fp; // Frame Pointer

    uint16_t args_to_pop;

    ObjectFactory_t objectFactory;
    vector_t heap_objects; // vector_t<void*> - for tracking allocated objects

} VM_t;

// VM_t function declarations
void vm_init(VM_t *vm, const vector_t *filedata);
void vm_destroy(VM_t *vm);
void vm_load_from_binary(VM_t *vm, const vector_t *filedata);
void vm_run(VM_t *vm);
uint32_t vm_top(const VM_t *vm);

// Private helper functions (will be defined in VM.c)
void vm_push(VM_t *vm, uint32_t v);
uint32_t vm_pop(VM_t *vm);
uint32_t vm_peek(const VM_t *vm);

uint8_t vm_fetch8(VM_t *vm);
uint16_t vm_fetch16(VM_t *vm);
int32_t vm_fetch32(VM_t *vm);

#endif // VM_H
