#include "VM.h"
#include "string.h"   // For string_t operations
#include <stdlib.h>   // For malloc, free, exit
#include <string.h>   // For memcpy, memset
#include <stdio.h>    // For fprintf, stderr, FILE operations
#include <fcntl.h>    // For file flags (O_RDONLY, O_WRONLY, etc.)
#include <sys/stat.h> // For stat, fstat

// Helper function to read uint32 from filedata vector
static uint32_t read_uint32_from_vector(const vector_t *filedata_vec, size_t *offset)
{
    if (*offset + 4 > vector_size(filedata_vec))
    {
        ERROR_PRINT("Unexpected EOF when reading uint32\n");
        exit(1); // Critical error
    }
    uint8_t b1 = *(uint8_t *)vector_get_ptr(filedata_vec, *offset);
    uint8_t b2 = *(uint8_t *)vector_get_ptr(filedata_vec, *offset + 1);
    uint8_t b3 = *(uint8_t *)vector_get_ptr(filedata_vec, *offset + 2);
    uint8_t b4 = *(uint8_t *)vector_get_ptr(filedata_vec, *offset + 3);
    *offset += 4;
    return (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
}

// Helper function to read uint8 from filedata vector
static uint8_t read_uint8_from_vector(const vector_t *filedata_vec, size_t *offset)
{
    if (*offset + 1 > vector_size(filedata_vec))
    {
        ERROR_PRINT("Unexpected EOF when reading uint8\n");
        exit(1); // Critical error
    }
    return *(uint8_t *)vector_get_ptr(filedata_vec, (*offset)++);
}

void vm_init(VM_t *vm, const vector_t *filedata)
{
    vm->ip = 0;
    vm->fp = 0;
    vm->args_to_pop = 0;

    vector_init(&vm->stack, sizeof(uint32_t));
    vector_init(&vm->locals, sizeof(uint32_t));
    vector_init(&vm->constantPool, sizeof(uint32_t));
    vector_init(&vm->classes, sizeof(ClassInfo_t)); // Stores ClassInfo_t objects
    vector_init(&vm->code, sizeof(uint8_t));
    vector_init(&vm->fileData, sizeof(FILE *));
    vector_init(&vm->heap_objects, sizeof(void *));

    object_factory_init(&vm->objectFactory);

    vm_load_from_binary(vm, filedata);

    // Register classes with object factory
    for (size_t i = 0; i < vector_size(&vm->classes); ++i)
    {
        ClassInfo_t *cls = (ClassInfo_t *)vector_get_ptr(&vm->classes, i);
        if (!object_factory_register_class(&vm->objectFactory, cls))
        {
            ERROR_PRINT("Failed to register class %s with object factory\n", string_cstr(&cls->name));
            exit(1);
        }
    }

    object_factory_build_all_vtables(&vm->objectFactory);

    // Initialize fileData (stdin, stdout, stderr)
    vector_resize(&vm->fileData, 10); // Support up to 10 open files
    // if (!stdout)
    // {
    //     stdout = fdopen_simple(1);
    // }
    *(FILE **)vector_get_ptr(&vm->fileData, 0) = stdin;
    *(FILE **)vector_get_ptr(&vm->fileData, 1) = stdout;
    *(FILE **)vector_get_ptr(&vm->fileData, 2) = stderr;

    printf("Size of fdata vector: %d\r\n", vector_size(&vm->fileData));

    vector_reserve(&vm->stack, 1024); // STACK_SIZE
    vector_resize(&vm->locals, 256);  // LOCALS_SIZE, initialized to 0 by vector_resize
    printf("VM Initialized\r\n");
}

void vm_destroy(VM_t *vm)
{
    // Destroy objects on the heap
    for (size_t i = 0; i < vector_size(&vm->heap_objects); ++i)
    {
        void *obj_data = *(void **)vector_get_ptr(&vm->heap_objects, i);
        object_factory_destroy_object(&vm->objectFactory, obj_data);
    }
    vector_destroy(&vm->heap_objects);

    object_factory_destroy(&vm->objectFactory);

    // Destroy ClassInfo_t objects stored in vm->classes
    for (size_t i = 0; i < vector_size(&vm->classes); ++i)
    {
        ClassInfo_t *cls = (ClassInfo_t *)vector_get_ptr(&vm->classes, i);
        class_info_destroy(cls);
    }
    vector_destroy(&vm->classes);

    vector_destroy(&vm->stack);
    vector_destroy(&vm->locals);
    vector_destroy(&vm->constantPool);
    vector_destroy(&vm->code);

    // Close any open files
    for (size_t i = 3; i < vector_size(&vm->fileData); ++i)
    { // Start from 3 to skip stdin, stdout, stderr
        FILE *f = *(FILE **)vector_get_ptr(&vm->fileData, i);
        if (f != NULL)
        {
            fclose(f);
        }
    }
    vector_destroy(&vm->fileData);
}

void vm_load_from_binary(VM_t *vm, const vector_t *filedata_vec)
{
    if (vector_size(filedata_vec) < 24)
    {
        ERROR_PRINT("File too small to be a valid VM executable\n");
        exit(1);
    }

    // Check magic number "VM\x00\x01"
    const uint8_t expected_magic[4] = {0x56, 0x4D, 0x00, 0x01};
    for (int i = 0; i < 4; ++i)
    {
        if (*(uint8_t *)vector_get_ptr(filedata_vec, i) != expected_magic[i])
        {
            ERROR_PRINT("Invalid VM file magic number\n");
            exit(1);
        }
    }

    size_t offset = 4;

    uint32_t version = read_uint32_from_vector(filedata_vec, &offset);
    if (version != 1)
    {
        ERROR_PRINT("Unsupported VM version: %u\n", version);
        exit(1);
    }

    uint32_t entryPoint = read_uint32_from_vector(filedata_vec, &offset);
    uint32_t constPoolOffset = read_uint32_from_vector(filedata_vec, &offset);
    uint32_t constPoolSize = read_uint32_from_vector(filedata_vec, &offset);
    uint32_t codeOffset = read_uint32_from_vector(filedata_vec, &offset);
    uint32_t codeSize = read_uint32_from_vector(filedata_vec, &offset);
    uint32_t globalsOffset = read_uint32_from_vector(filedata_vec, &offset);
    uint32_t globalsSize = read_uint32_from_vector(filedata_vec, &offset);
    uint32_t classMetadataOffset = read_uint32_from_vector(filedata_vec, &offset);
    uint32_t classMetadataSize = read_uint32_from_vector(filedata_vec, &offset);

    DBG("VM Version: ");
    DBG_UINT32(version);
    DBG_NL();
    DBG("Entry Point: ");
    DBG_UINT32(entryPoint);
    DBG_NL();
    DBG("Const Pool Offset: ");
    DBG_UINT32(constPoolOffset);
    DBG(" Size: ");
    DBG_UINT32(constPoolSize);
    DBG_NL();
    DBG("Code Offset: ");
    DBG_UINT32(codeOffset);
    DBG(" Size: ");
    DBG_UINT32(codeSize);
    DBG_NL();
    DBG("Globals Offset: ");
    DBG_UINT32(globalsOffset);
    DBG(" Size: ");
    DBG_UINT32(globalsSize);
    DBG_NL();
    DBG("Class Metadata Offset: ");
    DBG_UINT32(classMetadataOffset);
    DBG(" Size: ");
    DBG_UINT32(classMetadataSize);
    DBG_NL();

    if (constPoolOffset + constPoolSize > vector_size(filedata_vec))
    {
        ERROR_PRINT("Constant pool section out of file bounds\n");
        exit(1);
    }
    vector_clear(&vm->constantPool);
    if (constPoolSize % 4 != 0)
    {
        ERROR_PRINT("Constant pool size not multiple of 4\n");
        exit(1);
    }
    size_t numConsts = constPoolSize / 4;
    vector_reserve(&vm->constantPool, numConsts);

    for (size_t i = 0; i < numConsts; i++)
    {
        size_t pos = constPoolOffset + i * 4;
        uint32_t val = read_uint32_from_vector(filedata_vec, &pos); // Use pos directly
        vector_push_back(&vm->constantPool, &val);
    }

    vector_clear(&vm->locals);
    if (globalsOffset + globalsSize > vector_size(filedata_vec))
    {
        ERROR_PRINT("Globals section out of file bounds\n");
        exit(1);
    }
    if (globalsSize % 4 != 0)
    {
        ERROR_PRINT("Globals section size not multiple of 4\n");
        exit(1);
    }

    size_t numGlobals = globalsSize / 4;
    vector_reserve(&vm->locals, numGlobals);

    for (size_t i = 0; i < numGlobals; i++)
    {
        size_t pos = globalsOffset + i * 4;
        uint32_t val = read_uint32_from_vector(filedata_vec, &pos); // Use pos directly
        vector_push_back(&vm->locals, &val);
    }

    if (codeOffset + codeSize > vector_size(filedata_vec))
    {
        ERROR_PRINT("Code section out of file bounds\n");
        exit(1);
    }

    vector_clear(&vm->code);
    vector_reserve(&vm->code, codeSize);
    for (size_t i = 0; i < codeSize; ++i)
    {
        uint8_t byte = *(uint8_t *)vector_get_ptr(filedata_vec, codeOffset + i);
        vector_push_back(&vm->code, &byte);
    }

#ifdef VM_DEBUG
    printf("[VM DEBUG] Code bytes loaded: ");
    for (size_t i = 0; i < vector_size(&vm->code); ++i)
    {
        printf("%d ", *(uint8_t *)vector_get_ptr(&vm->code, i));
    }
    printf("\n");
#endif

    if (classMetadataOffset + classMetadataSize > vector_size(filedata_vec))
    {
        ERROR_PRINT("Class metadata section out of file bounds\n");
        exit(1);
    }

    size_t classMetaEnd = classMetadataOffset + classMetadataSize;
    size_t classOffset = classMetadataOffset;

    if (classMetadataSize != 0)
    {
        vector_clear(&vm->classes);

        uint32_t classCount = read_uint32_from_vector(filedata_vec, &classOffset);

        for (uint32_t i = 0; i < classCount; i++)
        {
            ClassInfo_t cls;
            class_info_init(&cls);

            uint8_t classNameLen = read_uint8_from_vector(filedata_vec, &classOffset);
            if (classOffset + classNameLen > classMetaEnd)
            {
                ERROR_PRINT("Class name exceeds metadata bounds\n");
                exit(1);
            }

            string_t temp_name;
            string_init(&temp_name);
            for (int j = 0; j < classNameLen; ++j)
            {
                string_append_char(&temp_name, *(char *)vector_get_ptr(filedata_vec, classOffset + j));
            }
            string_copy(&cls.name, &temp_name);
            string_destroy(&temp_name);
            classOffset += classNameLen;

            cls.superClassIndex = (int32_t)read_uint32_from_vector(filedata_vec, &classOffset);

            DBG("Class: ");
            DBG_STR(string_cstr(&cls.name));
            DBG(", Superclass Index: ");
            DBG_INT(cls.superClassIndex);
            DBG_NL();

            uint32_t fieldCount = read_uint32_from_vector(filedata_vec, &classOffset);
            for (uint32_t f = 0; f < fieldCount; f++)
            {
                FieldInfo_t field;
                string_init(&field.name);

                uint8_t fieldNameLen = read_uint8_from_vector(filedata_vec, &classOffset);
                if (classOffset + fieldNameLen + 1 > classMetaEnd)
                {
                    ERROR_PRINT("Field info exceeds metadata bounds\n");
                    exit(1);
                }

                string_t temp_field_name;
                string_init(&temp_field_name);
                for (int j = 0; j < fieldNameLen; ++j)
                {
                    string_append_char(&temp_field_name, *(char *)vector_get_ptr(filedata_vec, classOffset + j));
                }
                string_copy(&field.name, &temp_field_name);
                string_destroy(&temp_field_name);
                classOffset += fieldNameLen;

                field.type = (FieldType_t)read_uint8_from_vector(filedata_vec, &classOffset);

                vector_push_back(&cls.fields, &field);
                //  // field is copied into vector, destroy local copy
                DBG("Field: ");
                DBG_STR(string_cstr(&field.name));
                DBG(" Type: ");
                DBG_INT(field.type);
                DBG_NL();
                string_destroy(&field.name);
            }

            uint32_t methodCount = read_uint32_from_vector(filedata_vec, &classOffset);
            for (uint32_t m = 0; m < methodCount; m++)
            {
                MethodInfo_t method;
                string_init(&method.name);

                uint8_t methodNameLen = read_uint8_from_vector(filedata_vec, &classOffset);

                if (classOffset + methodNameLen + 4 > classMetaEnd)
                {
                    ERROR_PRINT("Method info exceeds metadata bounds\n");
                    exit(1);
                }

                string_t temp_method_name;
                string_init(&temp_method_name);
                for (int j = 0; j < methodNameLen; ++j)
                {
                    string_append_char(&temp_method_name, *(char *)vector_get_ptr(filedata_vec, classOffset + j));
                }
                string_copy(&method.name, &temp_method_name);
                string_destroy(&temp_method_name);
                classOffset += methodNameLen;

                method.bytecodeOffset = read_uint32_from_vector(filedata_vec, &classOffset);
                method.isVirtual = true; // Default as in C++

                vector_push_back(&cls.methods, &method);
                //  // method is copied into vector, destroy local copy
                DBG("Method: ");
                DBG_STR(string_cstr(&method.name));
                DBG(" Bytecode Offset: ");
                DBG_UINT32(method.bytecodeOffset);
                DBG_NL();
                string_destroy(&method.name);
            }

            vector_push_back(&vm->classes, &cls);
            // class_info_destroy(&cls); // cls is copied into vector, destroy local copy
        }

        if (classOffset != classMetaEnd)
        {
            ERROR_PRINT("Class metadata size mismatch after parsing\n");
            exit(1);
        }
    }

    if (entryPoint >= vector_size(&vm->code))
    {
        ERROR_PRINT("Entry point out of code segment bounds\n");
        exit(1);
    }
    vm->ip = entryPoint;
    DBG("Entry point set to ");
    DBG_UINT32(vm->ip);
    DBG_NL();

    vector_clear(&vm->stack);
}

void vm_run(VM_t *vm)
{
    while (vm->ip < vector_size(&vm->code))
    {
        Opcode_t opcode = (Opcode_t)vm_fetch8(vm);

        switch (opcode)
        {
        case IADD:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            int32_t result = a + b;
            vm_push(vm, result);
            DBG("IADD, Stack top = ");
            DBG_INT(result);
            DBG_NL();
            break;
        }
        case ISUB:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            int32_t result = a - b;
            vm_push(vm, result);
            DBG("ISUB, Stack top = ");
            DBG_INT(result);
            DBG_NL();
            break;
        }
        case IMUL:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            int32_t result = a * b;
            vm_push(vm, result);
            DBG("IMUL, Stack top = ");
            DBG_INT(result);
            DBG_NL();
            break;
        }
        case IDIV:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            if (b == 0)
            {
                ERROR_PRINT("Division by zero\n");
                exit(1);
            }
            int32_t result = a / b;
            vm_push(vm, result);
            DBG("IDIV, Stack top = ");
            DBG_INT(result);
            DBG_NL();
            break;
        }
        case INEG:
        {
            int32_t a = vm_pop(vm);
            int32_t result = -a;
            vm_push(vm, result);
            DBG("INEG, Stack top = ");
            DBG_INT(result);
            DBG_NL();
            break;
        }
        case IMOD:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            if (b == 0)
            {
                ERROR_PRINT("Modulo by zero\n");
                exit(1);
            }
            int32_t result = a % b;
            vm_push(vm, result);
            DBG("IMOD, Stack top = ");
            DBG_INT(result);
            DBG_NL();
            break;
        }
        case FADD:
        {
            uint32_t b_raw = vm_pop(vm);
            uint32_t a_raw = vm_pop(vm);
            float bf, af;
            memcpy(&bf, &b_raw, sizeof(float));
            memcpy(&af, &a_raw, sizeof(float));
            float cf = af + bf;
            uint32_t result_raw;
            memcpy(&result_raw, &cf, sizeof(float));
            vm_push(vm, result_raw);
            DBG("FADD, Stack top = ");
            DBG_FLOAT(cf);
            DBG_NL();
            break;
        }
        case FSUB:
        {
            uint32_t b_raw = vm_pop(vm);
            uint32_t a_raw = vm_pop(vm);
            float bf, af;
            memcpy(&bf, &b_raw, sizeof(float));
            memcpy(&af, &a_raw, sizeof(float));
            float cf = af - bf;
            uint32_t result_raw;
            memcpy(&result_raw, &cf, sizeof(float));
            vm_push(vm, result_raw);
            DBG("FSUB, Stack top = ");
            DBG_FLOAT(cf);
            DBG_NL();
            break;
        }
        case FMUL:
        {
            uint32_t b_raw = vm_pop(vm);
            uint32_t a_raw = vm_pop(vm);
            float bf, af;
            memcpy(&bf, &b_raw, sizeof(float));
            memcpy(&af, &a_raw, sizeof(float));
            float cf = af * bf;
            uint32_t result_raw;
            memcpy(&result_raw, &cf, sizeof(float));
            vm_push(vm, result_raw);
            DBG("FMUL, Stack top = ");
            DBG_FLOAT(cf);
            DBG_NL();
            break;
        }
        case FDIV:
        {
            uint32_t b_raw = vm_pop(vm);
            uint32_t a_raw = vm_pop(vm);
            float bf, af;
            memcpy(&bf, &b_raw, sizeof(float));
            memcpy(&af, &a_raw, sizeof(float));
            if (bf == 0.0f)
            {
                ERROR_PRINT("Division by zero (float)\n");
                exit(1);
            }
            float cf = af / bf;
            uint32_t result_raw;
            memcpy(&result_raw, &cf, sizeof(float));
            vm_push(vm, result_raw);
            DBG("FDIV, Stack top = ");
            DBG_FLOAT(cf);
            DBG_NL();
            break;
        }
        case FNEG:
        {
            uint32_t a_raw = vm_pop(vm);
            float af;
            memcpy(&af, &a_raw, sizeof(float));
            af = -af;
            uint32_t result_raw;
            memcpy(&result_raw, &af, sizeof(float));
            vm_push(vm, result_raw);
            DBG("FNEG, Stack top = ");
            DBG_FLOAT(af);
            DBG_NL();
            break;
        }
        case PUSH:
        {
            int32_t val = vm_fetch32(vm);
            vm_push(vm, val);
            DBG("PUSH ");
            DBG_INT(val);
            DBG(", Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case POP:
        {
            vm_pop(vm);
            DBG("POP ,Stack size = ");
            DBG_UINT(vector_size(&vm->stack));
            DBG_NL();
            break;
        }
        case FPUSH:
        {
            float val;
            uint32_t raw = vm_fetch32(vm);
            memcpy(&val, &raw, sizeof(float));
            vm_push(vm, raw); // Push raw bits
            DBG("FPUSH ");
            DBG_FLOAT(val);
            DBG(", Stack top = ");
            DBG_FLOAT(val);
            DBG_NL();
            break;
        }
        case FPOP:
        {
            vm_pop(vm);
            DBG("FPOP ,Stack size = ");
            DBG_UINT(vector_size(&vm->stack));
            DBG_NL();
            break;
        }
        case DUP:
        {
            uint32_t val = vm_peek(vm);
            vm_push(vm, val);
            DBG("DUP, Stack top = ");
            DBG_UINT32(vm_peek(vm));
            DBG_NL();
            break;
        }
        case LOAD:
        {
            uint32_t idx = vm_fetch32(vm);
            if (idx >= vector_size(&vm->locals))
            {
                ERROR_PRINT("LOAD error: Local index out of bounds: %u\n", idx);
                exit(1);
            }
            uint32_t val = *(uint32_t *)vector_get_ptr(&vm->locals, idx);
            vm_push(vm, val);
            DBG("LOAD ");
            DBG_UINT32(idx);
            DBG(", Value = ");
            DBG_UINT32(val);
            DBG(", Stack top = ");
            DBG_UINT32(vm_peek(vm));
            DBG_NL();
            break;
        }
        case STORE:
        {
            uint32_t idx = vm_fetch32(vm);
            if (idx >= vector_size(&vm->locals))
            {
                ERROR_PRINT("STORE error: Local index out of bounds: %u\n", idx);
                exit(1);
            }
            uint32_t val = vm_pop(vm);
            *(uint32_t *)vector_get_ptr(&vm->locals, idx) = val;
            DBG("STORE ");
            DBG_UINT32(idx);
            DBG(", Value = ");
            DBG_UINT32(val);
            DBG_NL();
            break;
        }
        case LOAD_ARG:
        {
            uint8_t argIdx = vm_fetch8(vm);
            if (vm->fp < 2 + argIdx || vm->fp - 2 - argIdx >= vector_size(&vm->stack))
            {
                ERROR_PRINT("LOAD_ARG error: Argument index out of bounds: %u\n", argIdx);
                exit(1);
            }
            uint32_t argVal = *(uint32_t *)vector_get_ptr(&vm->stack, vm->fp - 2 - argIdx);
            vm_push(vm, argVal);
            DBG("LOAD_ARG ");
            DBG_UINT32(argIdx);
            DBG(", Value = ");
            DBG_UINT32(argVal);
            DBG(", Stack top = ");
            DBG_UINT32(vm_peek(vm));
            DBG_NL();
            break;
        }
        case JMP:
        {
            uint16_t addr = vm_fetch16(vm);
            vm->ip = addr;
            DBG("JMP to ");
            DBG_UINT32(addr);
            DBG_NL();
            break;
        }
        case JZ:
        {
            uint16_t addr = vm_fetch16(vm);
            if (vm_pop(vm) == 0)
                vm->ip = addr;
            DBG("JZ to ");
            DBG_UINT32(addr);
            DBG_NL();
            break;
        }
        case JNZ:
        {
            uint16_t addr = vm_fetch16(vm);
            if (vm_pop(vm) != 0)
                vm->ip = addr;
            DBG("JNZ to ");
            DBG_UINT32(addr);
            DBG_NL();
            break;
        }
        case RET:
        {
            if (vm->fp == 0)
            {
                DBG("RET at base frame, halting execution.");
                DBG_NL();
                return;
            }
            if (vm->fp < 1 || vector_size(&vm->stack) < 2)
            {
                ERROR_PRINT("Stack underflow on RET\n");
                DBG_NL();
                exit(1);
            }

            uint32_t old_fp = *(uint32_t *)vector_get_ptr(&vm->stack, vm->fp);
            uint32_t return_ip = *(uint32_t *)vector_get_ptr(&vm->stack, vm->fp - 1);

            uint32_t itemsToPop = (uint32_t)vector_size(&vm->stack) - (vm->fp - 1);
            uint32_t returnValue = vm_pop(vm);
            for (uint32_t i = 0; i < itemsToPop - 1; i++)
            {
                vm_pop(vm);
            }

            vm->fp = old_fp;
            vm->ip = return_ip;

            for (uint8_t i = 0; i < vm->args_to_pop; i++)
            {
                vm_pop(vm);
            }
            vm->args_to_pop = 0;

            vm_push(vm, returnValue);

            DBG("RET to ip ");
            DBG_UINT32(vm->ip);
            DBG(", restored FP = ");
            DBG_UINT32(vm->fp);
            DBG_NL();
            break;
        }
        case CALL:
        {
            uint32_t methodOffset = vm_fetch32(vm);
            uint8_t argCount = vm_fetch8(vm);

            vm->args_to_pop = argCount;

            vm_push(vm, vm->ip);
            vm_push(vm, vm->fp);

            vm->fp = (uint32_t)vector_size(&vm->stack) - 1;
            vm->ip = methodOffset;

            DBG("CALL to offset ");
            DBG_UINT32(methodOffset);
            DBG(", return IP = ");
            DBG_UINT32(*(uint32_t *)vector_get_ptr(&vm->stack, vm->fp - 1));
            DBG(", FP = ");
            DBG_UINT32(vm->fp);
            DBG_NL();
            break;
        }
        case ICMP_EQ:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            vm_push(vm, (a == b ? 1 : 0));
            DBG("ICMP_EQ, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case ICMP_LT:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            vm_push(vm, (a < b ? 1 : 0));
            DBG("ICMP_LT, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case ICMP_GT:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            vm_push(vm, (a > b ? 1 : 0));
            DBG("ICMP_GT, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case ICMP_GEQ:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            vm_push(vm, (a >= b ? 1 : 0));
            DBG("ICMP_GEQ, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case ICMP_NEQ:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            vm_push(vm, (a != b ? 1 : 0));
            DBG("ICMP_NEQ, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case ICMP_LEQ:
        {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            vm_push(vm, (a <= b ? 1 : 0));
            DBG("ICMP_LEQ, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case FCMP_EQ:
        {
            uint32_t b_raw = vm_pop(vm);
            uint32_t a_raw = vm_pop(vm);
            float bf, af;
            memcpy(&bf, &b_raw, sizeof(float));
            memcpy(&af, &a_raw, sizeof(float));
            vm_push(vm, (af == bf ? 1 : 0));
            DBG("FCMP_EQ, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case FCMP_LT:
        {
            uint32_t b_raw = vm_pop(vm);
            uint32_t a_raw = vm_pop(vm);
            float bf, af;
            memcpy(&bf, &b_raw, sizeof(float));
            memcpy(&af, &a_raw, sizeof(float));
            vm_push(vm, (af < bf ? 1 : 0));
            DBG("FCMP_LT, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case FCMP_GT:
        {
            uint32_t b_raw = vm_pop(vm);
            uint32_t a_raw = vm_pop(vm);
            float bf, af;
            memcpy(&bf, &b_raw, sizeof(float));
            memcpy(&af, &a_raw, sizeof(float));
            vm_push(vm, (af > bf ? 1 : 0));
            DBG("FCMP_GT, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case FCMP_GEQ:
        {
            uint32_t b_raw = vm_pop(vm);
            uint32_t a_raw = vm_pop(vm);
            float bf, af;
            memcpy(&bf, &b_raw, sizeof(float));
            memcpy(&af, &a_raw, sizeof(float));
            vm_push(vm, (af >= bf ? 1 : 0));
            DBG("FCMP_GEQ, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case FCMP_NEQ:
        {
            uint32_t b_raw = vm_pop(vm);
            uint32_t a_raw = vm_pop(vm);
            float bf, af;
            memcpy(&bf, &b_raw, sizeof(float));
            memcpy(&af, &a_raw, sizeof(float));
            vm_push(vm, (af != bf ? 1 : 0));
            DBG("FCMP_NEQ, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case FCMP_LEQ:
        {
            uint32_t b_raw = vm_pop(vm);
            uint32_t a_raw = vm_pop(vm);
            float bf, af;
            memcpy(&bf, &b_raw, sizeof(float));
            memcpy(&af, &a_raw, sizeof(float));
            vm_push(vm, (af <= bf ? 1 : 0));
            DBG("FCMP_LEQ, Stack top = ");
            DBG_INT(vm_peek(vm));
            DBG_NL();
            break;
        }
        case NEW:
        {
            uint8_t classIndex = vm_fetch8(vm);
            if (classIndex >= vector_size(&vm->classes))
            {
                ERROR_PRINT("NEW error: Invalid class index: %u\n", classIndex);
                exit(1);
            }
            ClassInfo_t *cls = (ClassInfo_t *)vector_get_ptr(&vm->classes, classIndex);
            Object_t *newObjectData = object_factory_create_object(&vm->objectFactory, &cls->name);
            if (!newObjectData)
            {
                ERROR_PRINT("NEW error: Failed to create object for class %s\n", string_cstr(&cls->name));
                exit(1);
            }
            if (!vector_push_back(&vm->heap_objects, &newObjectData))
            {
                ERROR_PRINT("NEW error: Failed to track new object on heap\n");
                object_factory_destroy_object(&vm->objectFactory, newObjectData); // Clean up
                exit(1);
            }
            int32_t objRef = (int32_t)vector_size(&vm->heap_objects) - 1;
            vm_push(vm, objRef);
            DBG("NEW ");
            DBG_STR(string_cstr(&cls->name));
            DBG(", ObjRef: ");
            DBG_INT(objRef);
            DBG_NL();
            break;
        }
        case GETFIELD:
        {
            uint8_t fieldIndex = vm_fetch8(vm);
            int32_t objRef = vm_pop(vm);
            if (objRef < 0 || (size_t)objRef >= vector_size(&vm->heap_objects))
            {
                ERROR_PRINT("GETFIELD error: Invalid object reference: %d\n", objRef);
                exit(1);
            }
            void *objectData = *(void **)vector_get_ptr(&vm->heap_objects, objRef);
            void *rawMemory = (char *)objectData - sizeof(ClassInfo_t *);
            const ClassInfo_t *cls = *(const ClassInfo_t **)rawMemory;

            DBG("GETFIELD, requested index: ");
            DBG_INT(fieldIndex);
            DBG(", fieldCount: ");
            DBG_UINT(vector_size(&cls->fields));
            if (fieldIndex >= vector_size(&cls->fields))
            {
                ERROR_PRINT("GETFIELD error: Invalid field index: %u\n", fieldIndex);
                exit(1);
            }
            FieldInfo_t *field = (FieldInfo_t *)vector_get_ptr(&cls->fields, fieldIndex);

            // Retrieve offset from hash map
            size_t *offset_ptr = (size_t *)hash_map_get(&cls->fieldOffsets, &field->name);
            if (!offset_ptr)
            {
                ERROR_PRINT("GETFIELD error: Field offset not found for field %s\n", string_cstr(&field->name));
                exit(1);
            }
            size_t offset = *offset_ptr;

            char *baseAddress = (char *)objectData;
            int32_t value = *(int32_t *)(baseAddress + offset);
            vm_push(vm, value);
            DBG("GETFIELD from ObjRef ");
            DBG_INT(objRef);
            DBG(" (");
            DBG_STR(string_cstr(&cls->name));
            DBG(".");
            DBG_STR(string_cstr(&field->name));
            DBG("), Value = ");
            DBG_INT(value);
            DBG_NL();
            break;
        }
        case PUTFIELD:
        {
            uint8_t fieldIndex = vm_fetch8(vm);
            int32_t value = vm_pop(vm);
            int32_t objRef = vm_pop(vm);
            if (objRef < 0 || (size_t)objRef >= vector_size(&vm->heap_objects))
            {
                ERROR_PRINT("PUTFIELD error: Invalid object reference: %d\n", objRef);
                exit(1);
            }
            void *objectData = *(void **)vector_get_ptr(&vm->heap_objects, objRef);
            void *rawMemory = (char *)objectData - sizeof(ClassInfo_t *);
            const ClassInfo_t *cls = *(const ClassInfo_t **)rawMemory;
            if (fieldIndex >= vector_size(&cls->fields))
            {
                ERROR_PRINT("PUTFIELD error: Invalid field index: %u\n", fieldIndex);
                exit(1);
            }
            FieldInfo_t *field = (FieldInfo_t *)vector_get_ptr(&cls->fields, fieldIndex);

            // Retrieve offset from hash map
            size_t *offset_ptr = (size_t *)hash_map_get(&cls->fieldOffsets, &field->name);
            if (!offset_ptr)
            {
                ERROR_PRINT("PUTFIELD error: Field offset not found for field %s\n", string_cstr(&field->name));
                exit(1);
            }
            size_t offset = *offset_ptr;

            char *baseAddress = (char *)objectData;
            *(int32_t *)(baseAddress + offset) = value;
            DBG("PUTFIELD on ObjRef ");
            DBG_INT(objRef);
            DBG(" (");
            DBG_STR(string_cstr(&cls->name));
            DBG(".");
            DBG_STR(string_cstr(&field->name));
            DBG("), Value = ");
            DBG_INT(value);
            DBG_NL();
            break;
        }
        case INVOKEVIRTUAL:
        {
            uint32_t methodIndex = vm_fetch32(vm); // This is an index into the vtable, not bytecode offset
            vm->args_to_pop = vm_fetch8(vm);
            int32_t objRef = vm_pop(vm);

            if (objRef < 0 || (size_t)objRef >= vector_size(&vm->heap_objects))
            {
                ERROR_PRINT("INVOKEVIRTUAL error: Invalid object reference: %d\n", objRef);
                exit(1);
            }
            void *objectData = *(void **)vector_get_ptr(&vm->heap_objects, objRef);
            void *rawMemory = (char *)objectData - sizeof(ClassInfo_t *);
            const ClassInfo_t *cls = *(const ClassInfo_t **)rawMemory;

            if (methodIndex >= vector_size(&cls->vtable))
            {
                ERROR_PRINT("INVOKEVIRTUAL error: Invalid method index in vtable: %u\n", methodIndex);
                exit(1);
            }
            MethodInfo_t *method_to_call = *(MethodInfo_t **)vector_get_ptr(&cls->vtable, methodIndex);

            vm_push(vm, vm->ip);
            vm_push(vm, vm->fp);
            vm->fp = (uint32_t)vector_size(&vm->stack) - 1;
            vm->ip = method_to_call->bytecodeOffset;
            DBG("INVOKEVIRTUAL to offset ");
            DBG_UINT32(vm->ip);
            DBG(" (method: ");
            DBG_STR(string_cstr(&method_to_call->name));
            DBG(")");
            DBG_NL();
            break;
        }
        case INVOKESPECIAL:
        {
            // Not implemented in C++ version, so placeholder here
            ERROR_PRINT("INVOKESPECIAL opcode not implemented\n");
            exit(1);
            DBG_NL();
            break;
        }
        case NEWARRAY:
        {
            FieldType_t type = (FieldType_t)vm_fetch8(vm);
            int32_t size = vm_pop(vm);

            size_t element_size = 0;
            switch (type)
            {
            case FIELD_TYPE_INT:
            case FIELD_TYPE_OBJECT:
                element_size = sizeof(int32_t); // Object references are int32_t
                DBG_NL();
                break;
            case FIELD_TYPE_FLOAT:
                element_size = sizeof(float);
                DBG_NL();
                break;
            case FIELD_TYPE_CHAR:
                element_size = sizeof(char);
                DBG_NL();
                break;
            default:
                ERROR_PRINT("Unsupported array type: %u\n", type);
                exit(1);
            }

            // Allocate memory with header for array type
            void *rawarrayData = malloc(size * element_size + sizeof(FieldType_t));
            if (!rawarrayData)
            {
                ERROR_PRINT("NEWARRAY error: Failed to allocate memory for array\n");
                exit(1);
            }
            *(FieldType_t *)rawarrayData = type;                          // Store array type at the start
            void *arrayData = (char *)rawarrayData + sizeof(FieldType_t); // Pointer to actual array data

            // Zero initialize array elements
            memset(arrayData, 0, size * element_size);

            if (!vector_push_back(&vm->heap_objects, &arrayData))
            {
                ERROR_PRINT("NEWARRAY error: Failed to track new array on heap\n");
                free(rawarrayData); // Clean up
                exit(1);
            }
            int32_t arrayRef = (int32_t)vector_size(&vm->heap_objects) - 1;
            vm_push(vm, arrayRef);

            DBG("NEWARRAY of type ");
            DBG_INT(type);
            DBG(", size ");
            DBG_INT(size);
            DBG(", stored with reference ");
            DBG_INT(arrayRef);
            DBG_NL();
            break;
        }
        case ALOAD:
        {
            int32_t index = vm_pop(vm);
            int32_t arrayRef = vm_pop(vm);

            if (arrayRef < 0 || (size_t)arrayRef >= vector_size(&vm->heap_objects))
            {
                ERROR_PRINT("ALOAD error: Invalid array reference: %d\n", arrayRef);
                exit(1);
            }
            void *arrayData = *(void **)vector_get_ptr(&vm->heap_objects, arrayRef);
            void *rawarrayData = (char *)arrayData - sizeof(FieldType_t);
            FieldType_t arrayType = *(FieldType_t *)rawarrayData;

            switch (arrayType)
            {
            case FIELD_TYPE_INT:
            case FIELD_TYPE_OBJECT:
            {
                int32_t value = *(int32_t *)((char *)arrayData + index * sizeof(int32_t));
                vm_push(vm, value);
                DBG("ALOAD from array ref ");
                DBG_INT(arrayRef);
                DBG(" at index ");
                DBG_INT(index);
                DBG(", Stack top = ");
                DBG_INT(vm_peek(vm));
                DBG_NL();
                break;
            }
            case FIELD_TYPE_FLOAT:
            {
                float fvalue = *(float *)((char *)arrayData + index * sizeof(float));
                uint32_t raw;
                memcpy(&raw, &fvalue, sizeof(uint32_t));
                vm_push(vm, raw);
                DBG("ALOAD from array ref ");
                DBG_INT(arrayRef);
                DBG(" at index ");
                DBG_INT(index);
                DBG(", Stack top = ");
                DBG_FLOAT(fvalue);
                DBG_NL();
                break;
            }
            case FIELD_TYPE_CHAR:
            {
                char cvalue = *(char *)((char *)arrayData + index * sizeof(char));
                vm_push(vm, (uint32_t)cvalue);
                DBG("ALOAD from array ref ");
                DBG_INT(arrayRef);
                DBG(" at index ");
                DBG_INT(index);
                DBG(", Stack top = ");
                DBG_INT(cvalue);
                DBG_NL();
                break;
            }
            default:
                ERROR_PRINT("ALOAD error: Unsupported array type: %u\n", arrayType);
                exit(1);
            }
            DBG_NL();
            break;
        }
        case ASTORE:
        {
            int32_t value = vm_pop(vm);
            int32_t index = vm_pop(vm);
            int32_t arrayRef = vm_pop(vm);

            if (arrayRef < 0 || (size_t)arrayRef >= vector_size(&vm->heap_objects))
            {
                ERROR_PRINT("ASTORE error: Invalid array reference: %d\n", arrayRef);
                exit(1);
            }
            void *arrayData = *(void **)vector_get_ptr(&vm->heap_objects, arrayRef);
            void *rawarrayData = (char *)arrayData - sizeof(FieldType_t);
            FieldType_t arrayType = *(FieldType_t *)rawarrayData;

            switch (arrayType)
            {
            case FIELD_TYPE_OBJECT:
            case FIELD_TYPE_INT:
            {
                *(int32_t *)((char *)arrayData + index * sizeof(int32_t)) = value;
                DBG("ASTORE to array ref ");
                DBG_INT(arrayRef);
                DBG(" at index ");
                DBG_INT(index);
                DBG(", Value = ");
                DBG_INT(value);
                DBG_NL();
                break;
            }
            case FIELD_TYPE_FLOAT:
            {
                float fvalue;
                memcpy(&fvalue, &value, sizeof(float));
                *(float *)((char *)arrayData + index * sizeof(float)) = fvalue;
                DBG("ASTORE to array ref ");
                DBG_INT(arrayRef);
                DBG(" at index ");
                DBG_INT(index);
                DBG(", Value = ");
                DBG_FLOAT(fvalue);
                DBG_NL();
                break;
            }
            case FIELD_TYPE_CHAR:
            {
                *(char *)((char *)arrayData + index * sizeof(char)) = (char)value;
                DBG("ASTORE to array ref ");
                DBG_INT(arrayRef);
                DBG(" at index ");
                DBG_INT(index);
                DBG(", Value = ");
                DBG_INT((char)value);
                DBG_NL();
                break;
            }
            default:
                ERROR_PRINT("ASTORE error: Unsupported array type: %u\n", arrayType);
                exit(1);
            }
            DBG_NL();
            break;
        }
        case SYS_CALL:
        {
            Syscall_t syscall_type = (Syscall_t)vm_fetch8(vm);
            switch (syscall_type)
            {
            case SYS_READ:
            {
                int32_t fd = vm_pop(vm);
                int32_t size = vm_pop(vm);
                int32_t localsIdx = vm_pop(vm);

                void *rawbuffer = malloc(size + sizeof(FieldType_t)); // Allocate memory with header for type
                if (!rawbuffer)
                {
                    ERROR_PRINT("SYS_READ error: Failed to allocate buffer\n");
                    exit(1);
                }
                *(FieldType_t *)rawbuffer = FIELD_TYPE_CHAR; // Mark as char array for consistency
                void *buffer = (char *)rawbuffer + sizeof(FieldType_t);

                if (!vector_push_back(&vm->heap_objects, &buffer))
                {
                    ERROR_PRINT("SYS_READ error: Failed to track buffer on heap\n");
                    free(rawbuffer);
                    exit(1);
                }
                int32_t bufRef = (int32_t)vector_size(&vm->heap_objects) - 1;

                if (localsIdx >= vector_size(&vm->locals))
                {
                    ERROR_PRINT("SYS_READ error: Local index out of bounds for buffer storage: %d\n", localsIdx);
                    free(rawbuffer);
                    exit(1);
                }
                *(uint32_t *)vector_get_ptr(&vm->locals, localsIdx) = bufRef;

                if (fd < 0 || (size_t)fd >= vector_size(&vm->fileData) || *(FILE **)vector_get_ptr(&vm->fileData, fd) == NULL)
                {
                    ERROR_PRINT("SYS_READ error: Invalid file descriptor %d\n", fd);
                    free(rawbuffer);
                    exit(1);
                }
                FILE *f = *(FILE **)vector_get_ptr(&vm->fileData, fd);
                int32_t bytesRead = (int32_t)fread(buffer, 1, (size_t)size, f);
                vm_push(vm, bytesRead);

                DBG("SYS_READ from FD ");
                DBG_INT(fd);
                DBG(", Requested Size = ");
                DBG_INT(size);
                DBG(", Bytes Read = ");
                DBG_INT(bytesRead);
                DBG_NL();
                break;
            }
            case SYS_WRITE:
            {
                int32_t fd = vm_pop(vm);
                int32_t size = vm_pop(vm);
                int32_t local_idx = vm_pop(vm);
                if (local_idx >= vector_size(&vm->locals))
                {
                    ERROR_PRINT("SYS_WRITE error: Local index out of bounds for buffer reference: %d\n", local_idx);
                    exit(1);
                }
                int32_t bufRef = *(int32_t *)vector_get_ptr(&vm->locals, local_idx);

                if (bufRef < 0 || (size_t)bufRef >= vector_size(&vm->heap_objects))
                {
                    ERROR_PRINT("SYS_WRITE error: Invalid buffer reference %d\n", bufRef);
                    exit(1);
                }
                void *buffer = *(void **)vector_get_ptr(&vm->heap_objects, bufRef);

                if (fd < 0 || (size_t)fd >= vector_size(&vm->fileData) || *(FILE **)vector_get_ptr(&vm->fileData, fd) == NULL)
                {
                    ERROR_PRINT("SYS_WRITE error: Invalid file descriptor %d\n", fd);
                    exit(1);
                }
                FILE *f = *(FILE **)vector_get_ptr(&vm->fileData, fd);
                int32_t bytesWritten = (int32_t)fwrite(buffer, 1, (size_t)size, f);
                // flush to ensure data is written
                fflush(f);
                vm_push(vm, bytesWritten);
                DBG("SYS_WRITE to FD ");
                DBG_INT(fd);
                DBG(", Requested Size = ");
                DBG_INT(size);
                DBG(", Bytes Written = ");
                DBG_INT(bytesWritten);
                DBG_NL();
                break;
            }
            case SYS_OPEN:
            {
                char mode_char = (char)vm_pop(vm);
                int32_t filenameRef = vm_pop(vm);
                if (filenameRef < 0 || (size_t)filenameRef >= vector_size(&vm->heap_objects))
                {
                    ERROR_PRINT("SYS_OPEN error: Invalid filename reference %d\n", filenameRef);
                    exit(1);
                }
                char *filename = (char *)*(void **)vector_get_ptr(&vm->heap_objects, filenameRef);
                char modeStr[] = {mode_char, '\0'};
                int32_t fd = -1;

                for (size_t i = 3; i < vector_size(&vm->fileData); i++)
                {
                    FILE *f_ptr = *(FILE **)vector_get_ptr(&vm->fileData, i);
                    if (f_ptr == NULL)
                    {
                        FILE *f = fopen(filename, modeStr);
                        if (f == NULL)
                        {
                            ERROR_PRINT("SYS_OPEN error: Failed to open file %s with mode %s\n", filename, modeStr);
                            vm_push(vm, -1);  // Push -1 for error
                            goto syscall_end; // Jump to end of syscall switch
                        }
                        *(FILE **)vector_get_ptr(&vm->fileData, i) = f;
                        fd = (int32_t)i;
                        DBG_NL();
                        break;
                    }
                }

                if (fd == -1)
                {
                    ERROR_PRINT("SYS_OPEN error: Too many open files.\n");
                    vm_push(vm, -1); // Push -1 for error
                }
                else
                {
                    vm_push(vm, fd);
                }

                DBG("SYS_OPEN file ");
                DBG_STR(filename);
                DBG(" with mode ");
                DBG_CHAR(mode_char);
                DBG(", FD = ");
                DBG_INT(fd);
            syscall_end:; // Label for goto
                DBG_NL();
                break;
            }
            case SYS_CLOSE:
            {
                int32_t fd = vm_pop(vm);
                if (fd < 0 || (size_t)fd >= vector_size(&vm->fileData) || *(FILE **)vector_get_ptr(&vm->fileData, fd) == NULL)
                {
                    ERROR_PRINT("SYS_CLOSE error: Invalid file descriptor %d\n", fd);
                    exit(1);
                }
                FILE *f = *(FILE **)vector_get_ptr(&vm->fileData, fd);
                fclose(f);
                *(FILE **)vector_get_ptr(&vm->fileData, fd) = NULL;
                DBG("SYS_CLOSE on FD ");
                DBG_INT(fd);
                DBG_NL();
                break;
            }
            case SYS_EXIT:
            {
                int32_t exitCode = vm_pop(vm);
                DBG("SYS_EXIT with code ");
                DBG_INT(exitCode);
                DBG(", halting execution.");
                exit(exitCode);
            }
            default:
                ERROR_PRINT("Unsupported syscall: %u\n", syscall_type);
                exit(1);
            }
            DBG_NL();
            break;
        }
        default:
            ERROR_PRINT("Unknown opcode: %u\n", opcode);
            exit(1);
        }
    }
}

uint32_t vm_top(const VM_t *vm)
{
    return vm_peek(vm);
}

void vm_push(VM_t *vm, uint32_t v)
{
    if (vector_size(&vm->stack) >= 1024)
    { // STACK_SIZE
        ERROR_PRINT("Stack Overflow\n");
        exit(1);
    }
    vector_push_back(&vm->stack, &v);
}

uint32_t vm_pop(VM_t *vm)
{
    if (vector_size(&vm->stack) == 0)
    {
        ERROR_PRINT("Stack Underflow\n");
        exit(1);
    }
    uint32_t v;
    vector_pop_back(&vm->stack, &v);
    return v;
}

uint32_t vm_peek(const VM_t *vm)
{
    if (vector_size(&vm->stack) == 0)
    {
        ERROR_PRINT("Empty stack\n");
        exit(1);
    }
    return *(uint32_t *)vector_get_ptr(&vm->stack, vector_size(&vm->stack) - 1);
}

uint8_t vm_fetch8(VM_t *vm)
{
    if (vm->ip >= vector_size(&vm->code))
    {
        ERROR_PRINT("Fetch8 error: Instruction pointer out of bounds\n");
        exit(1);
    }
    return *(uint8_t *)vector_get_ptr(&vm->code, vm->ip++);
}

uint16_t vm_fetch16(VM_t *vm)
{
    uint16_t lo = vm_fetch8(vm);
    uint16_t hi = vm_fetch8(vm);
    return (hi << 8) | lo;
}

int32_t vm_fetch32(VM_t *vm)
{
    int32_t b1 = vm_fetch8(vm);
    int32_t b2 = vm_fetch8(vm);
    int32_t b3 = vm_fetch8(vm);
    int32_t b4 = vm_fetch8(vm);
    return (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
}
