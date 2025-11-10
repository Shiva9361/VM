/**
 * Author: Shivadharshan S
 */
// System standard library includes
#include <stdio.h> // System's stdio.h for vsnprintf, fprintf, stderr
#include <stdlib.h> // System's stdlib.h for malloc, free, exit
#include <string.h> // System's string.h for memcpy, memset
#include <assert.h>
// #include <stdarg.h> // For va_list - removed, now vm_debug_printf takes a single string
#include <sys/stat.h>  // For struct stat

// Include user-space syscall headers from OS project
#include <sys_calls.h> // Changed from relative path
#include <unistd.h>    // Changed from relative path

#include "include/VM.h"
#include "include/object_factory.h"

#ifdef VM_DEBUG
void vm_debug_printf(const char *msg) { // Now takes a single string
    write(2, "[VM DEBUG] ", strlen("[VM DEBUG] "));
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
}
#endif

// Define a simple fprintf-like function using sys_write to stderr (fd 2)
static void vm_fprintf_stderr(const char *msg) { // Simplified to take a single string
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
}


// Helper to read uint32 from buffer
static uint32_t read_uint32_from_buffer(const uint8_t* buffer, uint32_t* offset) {
    uint32_t val = buffer[*offset] | (buffer[*offset + 1] << 8) | (buffer[*offset + 2] << 16) | (buffer[*offset + 3] << 24);
    *offset += 4;
    return val;
}

// Helper to read uint8 from buffer
static uint8_t read_uint8_from_buffer(const uint8_t* buffer, uint32_t* offset) {
    return buffer[(*offset)++];
}

void VM_init(VM_State* vm, const uint8_t* filedata_buffer, uint32_t filedata_buffer_size) {
    memset(vm, 0, sizeof(VM_State)); // Zero-initialize the VM state

    vm->stack_ptr = 0;
    vm->constantPool_size = 0;
    vm->classes_count = 0;
    vm->code_size = 0;
    // vm->fileData_count = 0; // Removed
    vm->ip = 0;
    vm->fp = 0;
    vm->args_to_pop = 0;
    vm->heap_count = 0;

    // fileData will be managed by syscalls, so no need to initialize stdin/out/err here
    // The syscalls will directly use fd 0, 1, 2.

    // Initialize ObjectFactory
    vm->objectFactory = (ObjectFactory_State*)malloc(sizeof(ObjectFactory_State));
    if (!vm->objectFactory) {
        vm_fprintf_stderr("Error: Failed to allocate ObjectFactory_State");
        _exit(1);
    }
    ObjectFactory_init(vm->objectFactory);

    VM_loadFromBinary(vm, filedata_buffer, filedata_buffer_size);

    // Register classes with the object factory and build vtables
    for (uint32_t i = 0; i < vm->classes_count; ++i) {
        ObjectFactory_registerClass(vm->objectFactory, vm->classes[i]);
    }
    ObjectFactory_buildAllVTables(vm->objectFactory);
}

void VM_destroy(VM_State* vm) {
    // Free heap objects
    for (uint32_t i = 0; i < vm->heap_count; ++i) {
        ObjectFactory_heapFree(vm->heap[i]);
    }

    // Free ClassInfo structs
    for (uint32_t i = 0; i < vm->classes_count; ++i) {
        free(vm->classes[i]);
    }

    // Free ObjectFactory state
    free(vm->objectFactory);
}

void VM_loadFromBinary(VM_State* vm, const uint8_t* filedata_buffer, uint32_t filedata_buffer_size) {
    uint32_t offset = 0;

    if (filedata_buffer_size < 24) {
        vm_fprintf_stderr("File too small to be a valid VM executable");
        _exit(1);
    }

    // Check magic number "VM\x00\x01"
    const uint8_t expected_magic[4] = {0x56, 0x4D, 0x00, 0x01};
    if (memcmp(filedata_buffer, expected_magic, 4) != 0) {
        vm_fprintf_stderr("Invalid VM file magic number");
        _exit(1);
    }
    offset += 4;

    uint32_t version = read_uint32_from_buffer(filedata_buffer, &offset);
    if (version != 1) {
        vm_fprintf_stderr("Unsupported VM version");
        _exit(1);
    }

    uint32_t entryPoint = read_uint32_from_buffer(filedata_buffer, &offset);
    uint32_t constPoolOffset = read_uint32_from_buffer(filedata_buffer, &offset);
    uint32_t constPoolSize = read_uint32_from_buffer(filedata_buffer, &offset);
    uint32_t codeOffset = read_uint32_from_buffer(filedata_buffer, &offset);
    uint32_t codeSize = read_uint32_from_buffer(filedata_buffer, &offset);
    uint32_t globalsOffset = read_uint32_from_buffer(filedata_buffer, &offset);
    uint32_t globalsSize = read_uint32_from_buffer(filedata_buffer, &offset);
    uint32_t classMetadataOffset = read_uint32_from_buffer(filedata_buffer, &offset);
    uint32_t classMetadataSize = read_uint32_from_buffer(filedata_buffer, &offset);

    DBG_PRINTF("VM Version: %u"); // Simplified debug message
    DBG_PRINTF("Entry Point: %u");
    DBG_PRINTF("Const Pool Offset: %u, Size: %u");
    DBG_PRINTF("Code Offset: %u, Size: %u");
    DBG_PRINTF("Globals Offset: %u, Size: %u");
    DBG_PRINTF("Class Metadata Offset: %u, Size: %u");

    // Load Constant Pool
    if (constPoolOffset + constPoolSize > filedata_buffer_size) {
        vm_fprintf_stderr("Constant pool section out of file bounds");
        _exit(1);
    }
    if (constPoolSize % 4 != 0) {
        vm_fprintf_stderr("Constant pool size not multiple of 4");
        _exit(1);
    }
    vm->constantPool_size = constPoolSize / 4;
    for (uint32_t i = 0; i < vm->constantPool_size; ++i) {
        uint32_t pos = constPoolOffset + i * 4;
        vm->constantPool[i] = filedata_buffer[pos] | (filedata_buffer[pos + 1] << 8) | (filedata_buffer[pos + 2] << 16) | (filedata_buffer[pos + 3] << 24);
    }

    // Load Globals (into locals for now)
    if (globalsOffset + globalsSize > filedata_buffer_size) {
        vm_fprintf_stderr("Globals section out of file bounds");
        _exit(1);
    }
    if (globalsSize % 4 != 0) {
        vm_fprintf_stderr("Globals section size not multiple of 4");
        _exit(1);
    }
    uint32_t numGlobals = globalsSize / 4;
    for (uint32_t i = 0; i < numGlobals; ++i) {
        uint32_t pos = globalsOffset + i * 4;
        vm->locals[i] = filedata_buffer[pos] | (filedata_buffer[pos + 1] << 8) | (filedata_buffer[pos + 2] << 16) | (filedata_buffer[pos + 3] << 24);
    }

    // Load Code
    if (codeOffset + codeSize > filedata_buffer_size) {
        vm_fprintf_stderr("Code section out of file bounds");
        _exit(1);
    }
    if (codeSize > VM_MAX_CODE_SIZE) {
        vm_fprintf_stderr("Code size exceeds VM_MAX_CODE_SIZE");
        _exit(1);
    }
    memcpy(vm->code, filedata_buffer + codeOffset, codeSize);
    vm->code_size = codeSize;

    // Load Class Metadata
    if (classMetadataOffset + classMetadataSize > filedata_buffer_size) {
        vm_fprintf_stderr("Class metadata section out of file bounds");
        _exit(1);
    }
    uint32_t classMetaEnd = classMetadataOffset + classMetadataSize;
    uint32_t class_offset_cursor = classMetadataOffset;

    if (classMetadataSize != 0) {
        uint32_t classCount = read_uint32_from_buffer(filedata_buffer, &class_offset_cursor);
        if (classCount > VM_MAX_CLASSES) {
            vm_fprintf_stderr("Error: Too many classes for VM_MAX_CLASSES");
            _exit(1);
        }
        vm->classes_count = classCount;

        for (uint32_t i = 0; i < classCount; ++i) {
            ClassInfo* cls = (ClassInfo*)malloc(sizeof(ClassInfo));
            if (!cls) {
                vm_fprintf_stderr("Error: Failed to allocate ClassInfo");
                _exit(1);
            }
            memset(cls, 0, sizeof(ClassInfo)); // Zero-initialize

            uint8_t classNameLen = read_uint8_from_buffer(filedata_buffer, &class_offset_cursor);
            if (class_offset_cursor + classNameLen > classMetaEnd || classNameLen >= MAX_NAME_LEN) {
                vm_fprintf_stderr("Class name exceeds metadata bounds or too long");
                _exit(1);
            }
            memcpy(cls->name, filedata_buffer + class_offset_cursor, classNameLen);
            cls->name[classNameLen] = '\0';
            class_offset_cursor += classNameLen;

            cls->superClassIndex = (int32_t)read_uint32_from_buffer(filedata_buffer, &class_offset_cursor);

            DBG_PRINTF("Class: %s, Superclass Index: %d"); // Simplified debug message

            uint32_t fieldCount = read_uint32_from_buffer(filedata_buffer, &class_offset_cursor);
            if (fieldCount > MAX_FIELDS_PER_CLASS) {
                vm_fprintf_stderr("Error: Too many fields for class");
                _exit(1);
            }
            cls->field_count = fieldCount;
            for (uint32_t f = 0; f < fieldCount; ++f) {
                uint8_t fieldNameLen = read_uint8_from_buffer(filedata_buffer, &class_offset_cursor);
                if (class_offset_cursor + fieldNameLen + 1 > classMetaEnd || fieldNameLen >= MAX_NAME_LEN) {
                    vm_fprintf_stderr("Field info exceeds metadata bounds or too long");
                    _exit(1);
                }
                memcpy(cls->fields[f].name, filedata_buffer + class_offset_cursor, fieldNameLen);
                cls->fields[f].name[fieldNameLen] = '\0';
                class_offset_cursor += fieldNameLen;

                cls->fields[f].type = (FieldType)read_uint8_from_buffer(filedata_buffer, &class_offset_cursor);
                DBG_PRINTF("Field: %s Type: %d");
            }

            uint32_t methodCount = read_uint32_from_buffer(filedata_buffer, &class_offset_cursor);
            if (methodCount > MAX_METHODS_PER_CLASS) {
                vm_fprintf_stderr("Error: Too many methods for class");
                _exit(1);
            }
            cls->method_count = methodCount;
            for (uint32_t m = 0; m < methodCount; ++m) {
                uint8_t methodNameLen = read_uint8_from_buffer(filedata_buffer, &class_offset_cursor);
                if (class_offset_cursor + methodNameLen + 4 > classMetaEnd || methodNameLen >= MAX_NAME_LEN) {
                    vm_fprintf_stderr("Method info exceeds metadata bounds or too long");
                    _exit(1);
                }
                memcpy(cls->methods[m].name, filedata_buffer + class_offset_cursor, methodNameLen);
                cls->methods[m].name[methodNameLen] = '\0';
                class_offset_cursor += methodNameLen;

                cls->methods[m].bytecodeOffset = read_uint32_from_buffer(filedata_buffer, &class_offset_cursor);
                cls->methods[m].isVirtual = 1; // Default to virtual for now
                DBG_PRINTF("Method: %s Bytecode Offset: %u");
            }
            vm->classes[i] = cls;
        }

        if (class_offset_cursor != classMetaEnd) {
            vm_fprintf_stderr("Class metadata size mismatch after parsing");
            _exit(1);
        }
    }

    if (entryPoint >= vm->code_size) {
        vm_fprintf_stderr("Entry point out of code segment bounds");
        _exit(1);
    }
    vm->ip = entryPoint;
    DBG_PRINTF("Entry point set to %u");
}

void VM_run(VM_State* vm) {
    while (vm->ip < vm->code_size) {
        Opcode opcode = (Opcode)VM_fetch8(vm);

        switch (opcode) {
            case IADD: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                VM_push(vm, a + b);
                DBG_PRINTF("IADD, Stack top = %d");
                break;
            }
            case ISUB: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                VM_push(vm, a - b);
                DBG_PRINTF("ISUB, Stack top = %d");
                break;
            }
            case IMUL: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                VM_push(vm, a * b);
                DBG_PRINTF("IMUL, Stack top = %d");
                break;
            }
            case IDIV: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                if (b == 0) {
                    vm_fprintf_stderr("Division by zero");
                    _exit(1);
                }
                VM_push(vm, a / b);
                DBG_PRINTF("IDIV, Stack top = %d");
                break;
            }
            case INEG: {
                int32_t a = VM_pop(vm);
                VM_push(vm, -a);
                DBG_PRINTF("INEG, Stack top = %d");
                break;
            }
            case IMOD: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                if (b == 0) {
                    vm_fprintf_stderr("Modulo by zero");
                    _exit(1);
                }
                VM_push(vm, a % b);
                DBG_PRINTF("IMOD, Stack top = %d");
                break;
            }

            case FADD: {
                uint32_t b_raw = VM_pop(vm);
                uint32_t a_raw = VM_pop(vm);
                float bf, af;
                memcpy(&bf, &b_raw, sizeof(float));
                memcpy(&af, &a_raw, sizeof(float));
                float cf = af + bf;
                uint32_t c_raw;
                memcpy(&c_raw, &cf, sizeof(float));
                VM_push(vm, c_raw);
                DBG_PRINTF("FADD, Stack top = %f");
                break;
            }
            case FSUB: {
                uint32_t b_raw = VM_pop(vm);
                uint32_t a_raw = VM_pop(vm);
                float bf, af;
                memcpy(&bf, &b_raw, sizeof(float));
                memcpy(&af, &a_raw, sizeof(float));
                float cf = af - bf;
                uint32_t c_raw;
                memcpy(&c_raw, &cf, sizeof(float));
                VM_push(vm, c_raw);
                DBG_PRINTF("FSUB, Stack top = %f");
                break;
            }
            case FMUL: {
                uint32_t b_raw = VM_pop(vm);
                uint32_t a_raw = VM_pop(vm);
                float bf, af;
                memcpy(&bf, &b_raw, sizeof(float));
                memcpy(&af, &a_raw, sizeof(float));
                float cf = af * bf;
                uint32_t c_raw;
                memcpy(&c_raw, &cf, sizeof(float));
                VM_push(vm, c_raw);
                DBG_PRINTF("FMUL, Stack top = %f");
                break;
            }
            case FDIV: {
                uint32_t b_raw = VM_pop(vm);
                uint32_t a_raw = VM_pop(vm);
                float bf, af;
                memcpy(&bf, &b_raw, sizeof(float));
                memcpy(&af, &a_raw, sizeof(float));
                if (bf == 0.0f) {
                    vm_fprintf_stderr("Division by zero (float)");
                    _exit(1);
                }
                float cf = af / bf;
                uint32_t c_raw;
                memcpy(&c_raw, &cf, sizeof(float));
                VM_push(vm, c_raw);
                DBG_PRINTF("FDIV, Stack top = %f");
                break;
            }
            case FNEG: {
                uint32_t a_raw = VM_pop(vm);
                float af;
                memcpy(&af, &a_raw, sizeof(float));
                af = -af;
                uint32_t a_new_raw;
                memcpy(&a_new_raw, &af, sizeof(float));
                VM_push(vm, a_new_raw);
                DBG_PRINTF("FNEG, Stack top = %f");
                break;
            }

            case PUSH: {
                int32_t val = VM_fetch32(vm);
                VM_push(vm, val);
                DBG_PRINTF("PUSH %d, Stack top = %d");
                break;
            }
            case POP: {
                VM_pop(vm);
                DBG_PRINTF("POP, Stack size = %d");
                break;
            }
            case FPUSH: {
                uint32_t raw_val = VM_fetch32(vm);
                float val;
                memcpy(&val, &raw_val, sizeof(float));
                VM_push(vm, raw_val);
                DBG_PRINTF("FPUSH %f, Stack top = %f");
                break;
            }
            case FPOP: {
                VM_pop(vm);
                DBG_PRINTF("FPOP, Stack size = %d");
                break;
            }
            case DUP: {
                VM_push(vm, VM_peek(vm));
                DBG_PRINTF("DUP, Stack top = %d");
                break;
            }

            case LOAD: {
                uint32_t idx = VM_fetch32(vm);
                if (idx >= VM_LOCALS_SIZE) {
                    vm_fprintf_stderr("LOAD error: Local index out of bounds");
                    _exit(1);
                }
                VM_push(vm, vm->locals[idx]);
                DBG_PRINTF("LOAD %u, Value = %u, Stack top = %u");
                break;
            }
            case STORE: {
                uint32_t idx = VM_fetch32(vm);
                if (idx >= VM_LOCALS_SIZE) {
                    vm_fprintf_stderr("STORE error: Local index out of bounds");
                    _exit(1);
                }
                vm->locals[idx] = VM_pop(vm);
                DBG_PRINTF("STORE %u, Value = %u");
                break;
            }
            case LOAD_ARG: {
                uint8_t argIdx = VM_fetch8(vm);
                // arguments are pushed in reverse order relative to fp
                uint32_t argVal = vm->stack[vm->fp - 2 - argIdx]; 
                VM_push(vm, argVal);
                DBG_PRINTF("LOAD_ARG %u, Value = %u, Stack top = %u");
                break;
            }

            case JMP: {
                uint16_t addr = VM_fetch16(vm);
                vm->ip = addr;
                DBG_PRINTF("JMP to %u");
                break;
            }
            case JZ: {
                uint16_t addr = VM_fetch16(vm);
                if (VM_pop(vm) == 0) {
                    vm->ip = addr;
                }
                DBG_PRINTF("JZ to %u");
                break;
            }
            case JNZ: {
                uint16_t addr = VM_fetch16(vm);
                if (VM_pop(vm) != 0) {
                    vm->ip = addr;
                }
                DBG_PRINTF("JNZ to %u");
                break;
            }
            case RET: {
                if (vm->fp == 0) {
                    DBG_PRINTF("RET at base frame, halting execution.");
                    return;
                }
                if (vm->fp < 1 || vm->stack_ptr < 2) {
                    vm_fprintf_stderr("Stack underflow on RET");
                    _exit(1);
                }

                uint32_t old_fp = vm->stack[vm->fp];
                uint32_t return_ip = vm->stack[vm->fp - 1];

                // Pop return value
                uint32_t returnValue = VM_pop(vm);
                
                // Pop old_fp and return_ip
                VM_pop(vm); // old_fp
                VM_pop(vm); // return_ip

                // Pop arguments
                for (uint8_t i = 0; i < vm->args_to_pop; ++i) {
                    VM_pop(vm);
                }
                vm->args_to_pop = 0;

                vm->fp = old_fp;
                vm->ip = return_ip;

                VM_push(vm, returnValue); // Push return value back

                DBG_PRINTF("RET to ip %u, restored FP = %u");
                break;
            }
            case CALL: {
                uint32_t methodOffset = VM_fetch32(vm);
                uint8_t argCount = VM_fetch8(vm);

                vm->args_to_pop = argCount;

                VM_push(vm, vm->ip); // Save current IP
                VM_push(vm, vm->fp); // Save current FP
                VM_push(vm, 0); // Placeholder for 'this' pointer if not object method

                vm->fp = vm->stack_ptr - 1; // New frame pointer
                vm->ip = methodOffset;       // Jump to method

                DBG_PRINTF("CALL to offset %u, return IP = %u, FP = %u");
                break;
            }

            case ICMP_EQ: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                VM_push(vm, a == b ? 1 : 0);
                DBG_PRINTF("ICMP_EQ, Stack top = %d");
                break;
            }
            case ICMP_LT: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                VM_push(vm, a < b ? 1 : 0);
                DBG_PRINTF("ICMP_LT, Stack top = %d");
                break;
            }
            case ICMP_GT: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                VM_push(vm, a > b ? 1 : 0);
                DBG_PRINTF("ICMP_GT, Stack top = %d");
                break;
            }
            case ICMP_GEQ: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                VM_push(vm, a >= b ? 1 : 0);
                DBG_PRINTF("ICMP_GEQ, Stack top = %d");
                break;
            }
            case ICMP_NEQ: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                VM_push(vm, a != b ? 1 : 0);
                DBG_PRINTF("ICMP_NEQ, Stack top = %d");
                break;
            }
            case ICMP_LEQ: {
                int32_t b = VM_pop(vm);
                int32_t a = VM_pop(vm);
                VM_push(vm, a <= b ? 1 : 0);
                DBG_PRINTF("ICMP_LEQ, Stack top = %d");
                break;
            }

            case FCMP_EQ: {
                uint32_t b_raw = VM_pop(vm);
                uint32_t a_raw = VM_pop(vm);
                float bf, af;
                memcpy(&bf, &b_raw, sizeof(float));
                memcpy(&af, &a_raw, sizeof(float));
                VM_push(vm, af == bf ? 1 : 0);
                DBG_PRINTF("FCMP_EQ, Stack top = %d");
                break;
            }
            case FCMP_LT: {
                uint32_t b_raw = VM_pop(vm);
                uint32_t a_raw = VM_pop(vm);
                float bf, af;
                memcpy(&bf, &b_raw, sizeof(float));
                memcpy(&af, &a_raw, sizeof(float));
                VM_push(vm, af < bf ? 1 : 0);
                DBG_PRINTF("FCMP_LT, Stack top = %d");
                break;
            }
            case FCMP_GT: {
                uint32_t b_raw = VM_pop(vm);
                uint32_t a_raw = VM_pop(vm);
                float bf, af;
                memcpy(&bf, &b_raw, sizeof(float));
                memcpy(&af, &a_raw, sizeof(float));
                VM_push(vm, af > bf ? 1 : 0);
                DBG_PRINTF("FCMP_GT, Stack top = %d");
                break;
            }
            case FCMP_GEQ: {
                uint32_t b_raw = VM_pop(vm);
                uint32_t a_raw = VM_pop(vm);
                float bf, af;
                memcpy(&bf, &b_raw, sizeof(float));
                memcpy(&af, &a_raw, sizeof(float));
                VM_push(vm, af >= bf ? 1 : 0);
                DBG_PRINTF("FCMP_GEQ, Stack top = %d");
                break;
            }
            case FCMP_NEQ: {
                uint32_t b_raw = VM_pop(vm);
                uint32_t a_raw = VM_pop(vm);
                float bf, af;
                memcpy(&bf, &b_raw, sizeof(float));
                memcpy(&af, &a_raw, sizeof(float));
                VM_push(vm, af != bf ? 1 : 0);
                DBG_PRINTF("FCMP_NEQ, Stack top = %d");
                break;
            }
            case FCMP_LEQ: {
                uint32_t b_raw = VM_pop(vm);
                uint32_t a_raw = VM_pop(vm);
                float bf, af;
                memcpy(&bf, &b_raw, sizeof(float));
                memcpy(&af, &a_raw, sizeof(float));
                VM_push(vm, af <= bf ? 1 : 0);
                DBG_PRINTF("FCMP_LEQ, Stack top = %d");
                break;
            }

            case NEW: {
                uint8_t classIndex = VM_fetch8(vm);
                if (classIndex >= vm->classes_count) {
                    vm_fprintf_stderr("NEW error: Invalid class index");
                    _exit(1);
                }
                const ClassInfo* cls = vm->classes[classIndex];
                void* newObjectData = ObjectFactory_createObject(vm->objectFactory, cls->name);
                if (!newObjectData) {
                    vm_fprintf_stderr("NEW error: Failed to create object for class");
                    _exit(1);
                }
                if (vm->heap_count >= VM_MAX_HEAP_OBJECTS) {
                    vm_fprintf_stderr("NEW error: Heap full");
                    _exit(1);
                }
                vm->heap[vm->heap_count++] = newObjectData;
                int32_t objRef = (int32_t)(vm->heap_count - 1);
                VM_push(vm, objRef);
                DBG_PRINTF("NEW %s, ObjRef: %d");
                break;
            }
            case GETFIELD: {
                uint8_t fieldIndex = VM_fetch8(vm);
                int32_t objRef = VM_pop(vm);
                if (objRef < 0 || (uint32_t)objRef >= vm->heap_count) {
                    vm_fprintf_stderr("GETFIELD error: Invalid object reference");
                    _exit(1);
                }
                void* objectData = vm->heap[objRef];
                void* rawMemory = (char*)objectData - sizeof(void*);
                const ClassInfo* cls = *(const ClassInfo**)rawMemory;

                DBG_PRINTF("GETFIELD, requested index: %u, fieldCount: %u");
                if (fieldIndex >= cls->field_count) {
                    vm_fprintf_stderr("GETFIELD error: Invalid field index for class");
                    _exit(1);
                }
                
                size_t offset = cls->fieldOffsets[fieldIndex];
                char* baseAddress = (char*)objectData;
                int32_t value = *(int32_t*)(baseAddress + offset);
                VM_push(vm, value);
                DBG_PRINTF("GETFIELD from ObjRef %d (%s.%s), Value = %d");
                break;
            }
            case PUTFIELD: {
                uint8_t fieldIndex = VM_fetch8(vm);
                int32_t value = VM_pop(vm);
                int32_t objRef = VM_pop(vm);
                if (objRef < 0 || (uint32_t)objRef >= vm->heap_count) {
                    vm_fprintf_stderr("PUTFIELD error: Invalid object reference");
                    _exit(1);
                }
                void* objectData = vm->heap[objRef];
                void* rawMemory = (char*)objectData - sizeof(void*);
                const ClassInfo* cls = *(const ClassInfo**)rawMemory;
                if (fieldIndex >= cls->field_count) {
                    vm_fprintf_stderr("PUTFIELD error: Invalid field index for class");
                    _exit(1);
                }
                
                size_t offset = cls->fieldOffsets[fieldIndex];
                char* baseAddress = (char*)objectData;
                *(int32_t*)(baseAddress + offset) = value;
                DBG_PRINTF("PUTFIELD on ObjRef %d (%s.%s), Value = %d");
                break;
            }
            case INVOKEVIRTUAL: {
                uint32_t methodIndex = VM_fetch32(vm); // This is now an index into the vtable
                vm->args_to_pop = VM_fetch8(vm);
                int32_t objRef = VM_pop(vm);

                if (objRef < 0 || (uint32_t)objRef >= vm->heap_count) {
                    vm_fprintf_stderr("INVOKEVIRTUAL error: Invalid object reference");
                    _exit(1);
                }
                void* objectData = vm->heap[objRef];
                void* rawMemory = (char*)objectData - sizeof(void*);
                const ClassInfo* cls = *(const ClassInfo**)rawMemory;

                if (methodIndex >= cls->vtable_size) {
                    vm_fprintf_stderr("INVOKEVIRTUAL error: Invalid vtable index for class");
                    _exit(1);
                }
                MethodInfo* method = cls->vtable[methodIndex];

                VM_push(vm, vm->ip); // Save current IP
                VM_push(vm, vm->fp); // Save current FP
                VM_push(vm, objRef); // Push 'this' pointer (object reference)

                vm->fp = vm->stack_ptr - 1; // New frame pointer
                vm->ip = method->bytecodeOffset;       // Jump to method

                DBG_PRINTF("INVOKEVIRTUAL to offset %u (method %s), return IP = %u, FP = %u");
                break;
            }
            case INVOKESPECIAL: {
                // Similar to CALL, but for constructors/private methods, no virtual dispatch
                uint32_t methodOffset = VM_fetch32(vm);
                vm->args_to_pop = VM_fetch8(vm);
                int32_t objRef = VM_pop(vm); // Pop 'this' pointer

                VM_push(vm, vm->ip); // Save current IP
                VM_push(vm, vm->fp); // Save current FP
                VM_push(vm, objRef); // Push 'this' pointer (object reference)

                vm->fp = vm->stack_ptr - 1; // New frame pointer
                vm->ip = methodOffset;       // Jump to method

                DBG_PRINTF("INVOKESPECIAL to offset %u, return IP = %u, FP = %u");
                break;
            }

            case NEWARRAY: {
                FieldType type = (FieldType)VM_fetch8(vm);
                int32_t size = VM_pop(vm); // size of the array

                int32_t multiplier = 1;
                switch (type) {
                    case INT:
                    case OBJECT:
                        multiplier = sizeof(int32_t);
                        break;
                    case FLOAT:
                        multiplier = sizeof(float);
                        break;
                    case CHAR:
                        multiplier = sizeof(char);
                        break;
                    default:
                        vm_fprintf_stderr("Unsupported array type");
                        _exit(1);
                }

                // Allocate memory with header for type info
                void* rawarrayData = malloc(size * multiplier + sizeof(FieldType));
                if (!rawarrayData) {
                    vm_fprintf_stderr("NEWARRAY error: Failed to allocate memory");
                    _exit(1);
                }
                *(FieldType*)rawarrayData = type; // Store array type at the start
                void* arrayData = (void*)((char*)rawarrayData + sizeof(FieldType));
                memset(arrayData, 0, size * multiplier); // Zero-initialize array content

                if (vm->heap_count >= VM_MAX_HEAP_OBJECTS) {
                    vm_fprintf_stderr("NEWARRAY error: Heap full");
                    _exit(1);
                }
                vm->heap[vm->heap_count++] = rawarrayData; // Store raw pointer to free later
                int32_t arrayRef = (int32_t)(vm->heap_count - 1);
                VM_push(vm, arrayRef);

                DBG_PRINTF("NEWARRAY of type %d, size %d, stored with reference %d");
                break;
            }

            case ALOAD: {
                int32_t index = VM_pop(vm);  // array index
                int32_t arrayRef = VM_pop(vm); // heap index of array

                if (arrayRef < 0 || (uint32_t)arrayRef >= vm->heap_count) {
                    vm_fprintf_stderr("ALOAD error: Invalid array reference");
                    _exit(1);
                }
                void* rawarrayData = vm->heap[arrayRef];
                FieldType arrayType = *(FieldType*)rawarrayData;
                void* arrayData = (void*)((char*)rawarrayData + sizeof(FieldType));

                switch (arrayType) {
                    case INT:
                    case OBJECT: {
                        int32_t value = *(int32_t*)((char*)arrayData + index * sizeof(int32_t));
                        VM_push(vm, value);
                        DBG_PRINTF("ALOAD from array ref %d at index %d, Stack top = %d");
                        break;
                    }
                    case FLOAT: {
                        float fvalue = *(float*)((char*)arrayData + index * sizeof(float));
                        uint32_t raw;
                        memcpy(&raw, &fvalue, sizeof(uint32_t));
                        VM_push(vm, raw);
                        DBG_PRINTF("ALOAD from array ref %d at index %d, Stack top = %f");
                        break;
                    }
                    case CHAR: {
                        char cvalue = *(char*)((char*)arrayData + index * sizeof(char));
                        VM_push(vm, (uint32_t)cvalue);
                        DBG_PRINTF("ALOAD from array ref %d at index %d, Stack top = %c");
                        break;
                    }
                    default:
                        vm_fprintf_stderr("ALOAD error: Unsupported array type");
                        _exit(1);
                }
                break;
            }

            case ASTORE: {
                int32_t value = VM_pop(vm);  // value to store
                int32_t index = VM_pop(vm);  // index in the array
                int32_t arrayRef = VM_pop(vm); // heap index of array

                if (arrayRef < 0 || (uint32_t)arrayRef >= vm->heap_count) {
                    vm_fprintf_stderr("ASTORE error: Invalid array reference");
                    _exit(1);
                }
                void* rawarrayData = vm->heap[arrayRef];
                FieldType arrayType = *(FieldType*)rawarrayData;
                void* arrayData = (void*)((char*)rawarrayData + sizeof(FieldType));

                switch (arrayType) {
                    case INT:
                    case OBJECT: {
                        *(int32_t*)((char*)arrayData + index * sizeof(int32_t)) = value;
                        DBG_PRINTF("ASTORE to array ref %d at index %d, Value = %d");
                        break;
                    }
                    case FLOAT: {
                        float fvalue;
                        memcpy(&fvalue, &value, sizeof(float));
                        *(float*)((char*)arrayData + index * sizeof(float)) = fvalue;
                        DBG_PRINTF("ASTORE to array ref %d at index %d, Value = %f");
                        break;
                    }
                    case CHAR: {
                        *(char*)((char*)arrayData + index * sizeof(char)) = (char)value;
                        DBG_PRINTF("ASTORE to array ref %d at index %d, Value = %c");
                        break;
                    }
                    default:
                        vm_fprintf_stderr("ASTORE error: Unsupported array type");
                        _exit(1);
                }
                break;
            }

            case SYS_CALL: {
                Syscall syscall_num = (Syscall)VM_fetch8(vm);
                switch (syscall_num) {
                    case READ: {
                        int32_t fd = VM_pop(vm);
                        int32_t size = VM_pop(vm);
                        int32_t localsIdx = VM_pop(vm);

                        ssize_t bytesRead = read(fd, NULL, 0); // Check if fd is valid
                        if (bytesRead < 0) {
                            vm_fprintf_stderr("SYS_READ error: Invalid file descriptor");
                            _exit(1);
                        }

                        void* raw_buffer = malloc(size + sizeof(FieldType)); // Allocate with type header
                        if (!raw_buffer) {
                            vm_fprintf_stderr("SYS_READ error: Failed to allocate buffer");
                            _exit(1);
                        }
                        *(FieldType*)raw_buffer = CHAR; // Assume char array for file data
                        void* buffer = (char*)raw_buffer + sizeof(FieldType);
                        
                        if (vm->heap_count >= VM_MAX_HEAP_OBJECTS) {
                            vm_fprintf_stderr("SYS_READ error: Heap full");
                            free(raw_buffer);
                            _exit(1);
                        }
                        vm->heap[vm->heap_count++] = raw_buffer;
                        vm->locals[localsIdx] = vm->heap_count - 1; // Store heap index in locals

                        bytesRead = read(fd, buffer, size); // Use sys_read
                        if (bytesRead < 0) {
                            vm_fprintf_stderr("SYS_READ error: Syscall read failed for fd");
                            _exit(1);
                        }
                        VM_push(vm, (uint32_t)bytesRead);
                        DBG_PRINTF("SYS_READ from FD %d, Requested Size = %d, Bytes Read = %zd");
                        break;
                    }
                    case WRITE: {
                        int32_t fd = VM_pop(vm);
                        int32_t size = VM_pop(vm);
                        int32_t locIdx = VM_pop(vm);
                        int32_t bufHeapIdx = vm->locals[locIdx];

                        if (bufHeapIdx < 0 || (uint32_t)bufHeapIdx >= vm->heap_count) {
                            vm_fprintf_stderr("SYS_WRITE error: Invalid buffer heap index");
                            _exit(1);
                        }
                        void* raw_buffer = vm->heap[bufHeapIdx];
                        void* buffer = (char*)raw_buffer + sizeof(FieldType);

                        ssize_t bytesWritten = write(fd, buffer, size); // Use sys_write
                        if (bytesWritten < 0) {
                            vm_fprintf_stderr("SYS_WRITE error: Syscall write failed for fd");
                            _exit(1);
                        }
                        VM_push(vm, (uint32_t)bytesWritten);
                        DBG_PRINTF("SYS_WRITE to FD %d, Requested Size = %d, Bytes Written = %zd");
                        break;
                    }
                    case OPEN: {
                        char mode_char = (char)VM_pop(vm);
                        int32_t filenameHeapIdx = VM_pop(vm);

                        if (filenameHeapIdx < 0 || (uint32_t)filenameHeapIdx >= vm->heap_count) {
                            vm_fprintf_stderr("SYS_OPEN error: Invalid filename heap index");
                            _exit(1);
                        }
                        void* raw_filename_ptr = vm->heap[filenameHeapIdx];
                        char* filename = (char*)raw_filename_ptr + sizeof(FieldType); // Assuming char array

                        int flags = 0;
                        if (mode_char == 'r') flags = O_RDONLY;
                        else if (mode_char == 'w') flags = O_WRONLY | O_CREAT | O_TRUNC;
                        else if (mode_char == 'a') flags = O_WRONLY | O_CREAT | O_APPEND;
                        else if (mode_char == 'b') flags = O_RDONLY; // Binary mode, treat as read-only for simplicity
                        else {
                            vm_fprintf_stderr("SYS_OPEN error: Unsupported mode character");
                            VM_push(vm, -1); // Push error code
                            goto syscall_end;
                        }

                        int32_t fd = open(filename, flags, 0); // Use sys_open
                        if (fd < 0) {
                            vm_fprintf_stderr("SYS_OPEN error: Syscall open failed for file with mode");
                            VM_push(vm, -1); // Push error code
                            goto syscall_end;
                        }
                        VM_push(vm, fd);
                        DBG_PRINTF("SYS_OPEN file %s with mode %c, FD = %d");
                        break;
                    }
                    case CLOSE: {
                        int32_t fd = VM_pop(vm);
                        int32_t ret = close(fd); // Use sys_close
                        if (ret < 0) {
                            vm_fprintf_stderr("SYS_CLOSE error: Syscall close failed for fd");
                            _exit(1);
                        }
                        VM_push(vm, 0); // Success
                        DBG_PRINTF("SYS_CLOSE on FD %d");
                        break;
                    }
                    case EXIT: {
                        int32_t exitCode = VM_pop(vm);
                        DBG_PRINTF("SYS_EXIT with code %d, halting execution.");
                        _exit(exitCode);
                    }
                    default:
                        vm_fprintf_stderr("Unsupported syscall");
                        _exit(1);
                }
                syscall_end:; // Label for goto
                break;
            }

            default:
                vm_fprintf_stderr("Unknown opcode at IP");
                _exit(1);
        }
    }
}

uint32_t VM_top(const VM_State* vm) {
    return VM_peek(vm);
}

void VM_push(VM_State* vm, uint32_t v) {
    if (vm->stack_ptr >= VM_STACK_SIZE) {
        vm_fprintf_stderr("Stack Overflow");
        _exit(1);
    }
    vm->stack[vm->stack_ptr++] = v;
}

uint32_t VM_pop(VM_State* vm) {
    if (vm->stack_ptr == 0) {
        vm_fprintf_stderr("Stack Underflow");
        _exit(1);
    }
    return vm->stack[--vm->stack_ptr];
}

uint32_t VM_peek(const VM_State* vm) {
    if (vm->stack_ptr == 0) {
        vm_fprintf_stderr("Empty stack");
        _exit(1);
    }
    return vm->stack[vm->stack_ptr - 1];
}

uint8_t VM_fetch8(VM_State* vm) {
    if (vm->ip >= vm->code_size) {
        vm_fprintf_stderr("Attempt to fetch beyond code segment");
        _exit(1);
    }
    return vm->code[vm->ip++];
}

uint16_t VM_fetch16(VM_State* vm) {
    uint16_t lo = VM_fetch8(vm);
    uint16_t hi = VM_fetch8(vm);
    return (hi << 8) | lo;
}

int32_t VM_fetch32(VM_State* vm) {
    int32_t b1 = VM_fetch8(vm);
    int32_t b2 = VM_fetch8(vm);
    int32_t b3 = VM_fetch8(vm);
    int32_t b4 = VM_fetch8(vm);
    return (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
}