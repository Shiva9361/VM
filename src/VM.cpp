/**
 * Author: Shivadharshan S
 */
#include <VM.hpp>
#include <algorithm>
#include <cstring>

VM::VM(const std::vector<uint8_t> &filedata)
    : ip(0), fp(0)
{
    loadFromBinary(filedata);

    /* Code Added By Mokshith - Start*/
    for (const auto &cls : classes)
    {
        objectFactory.registerClass(cls);
    }
    /* Code Added By Mokshith - End*/

    objectFactory.buildAllVTables();

    fileData.resize(10, nullptr); // Support up to 10 open files
    fileData[0] = stdin;
    fileData[1] = stdout;
    fileData[2] = stderr;

    stack.reserve(STACK_SIZE);
    locals.resize(LOCALS_SIZE, 0);
}

VM::~VM()
{
    for (void *obj_data : heap)
    {
        objectFactory.destroyObject(obj_data);
    }
}

void VM::loadFromBinary(const std::vector<uint8_t> &filedata)
{

    if (filedata.size() < 24)
    {
        throw std::runtime_error("File too small to be a valid VM executable\n Expected at least 24 bytes, got " + std::to_string(filedata.size()));
    }

    // Check magic number "VM\x00\x01"
    const uint8_t expected_magic[4] = {0x56, 0x4D, 0x00, 0x01};
    if (!std::equal(filedata.begin(), filedata.begin() + 4, expected_magic))
    {
        throw std::runtime_error("Invalid VM file magic number");
    }

    size_t offset = 4;

    auto read_uint32 = [&](size_t &off) -> uint32_t
    {
        if (off + 4 > filedata.size())
            throw std::runtime_error("Unexpected EOF");
        uint32_t val = filedata[off] | (filedata[off + 1] << 8) | (filedata[off + 2] << 16) | (filedata[off + 3] << 24);
        off += 4;
        return val;
    };

    auto read_uint8 = [&](size_t &off) -> uint8_t
    {
        if (off + 1 > filedata.size())
            throw std::runtime_error("Unexpected EOF");
        return filedata[off++];
    };

    uint16_t version = read_uint32(offset);
    if (version != 1)
    {
        throw std::runtime_error("Unsupported VM version");
    }

    uint32_t entryPoint = read_uint32(offset);
    uint32_t constPoolOffset = read_uint32(offset);
    uint32_t constPoolSize = read_uint32(offset);
    uint32_t codeOffset = read_uint32(offset);
    uint32_t codeSize = read_uint32(offset);
    uint32_t globalsOffset = read_uint32(offset);
    uint32_t globalsSize = read_uint32(offset);
    uint32_t classMetadataOffset = read_uint32(offset);
    uint32_t classMetadataSize = read_uint32(offset);

    DBG("VM Version: " << version);
    DBG("Entry Point: " << entryPoint);
    DBG("Const Pool Offset: " << constPoolOffset << ", Size: " << constPoolSize);
    DBG("Code Offset: " << codeOffset << ", Size: " << codeSize);
    DBG("Globals Offset: " << globalsOffset << ", Size: " << globalsSize);
    DBG("Class Metadata Offset: " << classMetadataOffset << ", Size: " << classMetadataSize);

    if (constPoolOffset + constPoolSize > filedata.size())
    {
        throw std::runtime_error("Constant pool section out of file bounds");
    }
    constantPool.clear();
    // Assuming constant pool entries are 4-byte integers for simplicity:
    if (constPoolSize % 4 != 0)
    {
        throw std::runtime_error("Constant pool size not multiple of 4");
    }
    size_t numConsts = constPoolSize / 4;
    constantPool.reserve(numConsts);

    for (size_t i = 0; i < numConsts; i++)
    {
        size_t pos = constPoolOffset + i * 4;
        int val = filedata[pos] | (filedata[pos + 1] << 8) | (filedata[pos + 2] << 16) | (filedata[pos + 3] << 24);
        constantPool.push_back(val);
    }

    locals.clear();
    if (globalsOffset + globalsSize > filedata.size())
    {
        throw std::runtime_error("Globals section out of file bounds");
    }
    if (globalsSize % 4 != 0)
    {
        throw std::runtime_error("Globals section size not multiple of 4");
    }

    size_t numGlobals = globalsSize / 4;
    locals.reserve(numGlobals);

    for (size_t i = 0; i < numGlobals; i++)
    {
        size_t pos = globalsOffset + i * 4;
        int val = filedata[pos] | (filedata[pos + 1] << 8) | (filedata[pos + 2] << 16) | (filedata[pos + 3] << 24);
        locals.push_back(val);
    }

    if (codeOffset + codeSize > filedata.size())
    {
        throw std::runtime_error("Code section out of file bounds");
    }

    code.clear();
    code.insert(code.end(), filedata.begin() + codeOffset, filedata.begin() + codeOffset + codeSize);

#ifdef VM_CPP_DEBUG
    DBG("Code bytes loaded: ");
    for (auto b : code)
        std::cerr << std::hex << (int)b << ' ';
    std::cerr << std::endl;
#endif

    if (classMetadataOffset + classMetadataSize > filedata.size())
    {
        throw std::runtime_error("Class metadata section out of file bounds");
    }

    size_t classMetaEnd = classMetadataOffset + classMetadataSize;
    size_t classOffset = classMetadataOffset;

    if (classMetadataSize != 0)
    {

        classes.clear();

        uint32_t classCount = read_uint32(classOffset);

        for (uint32_t i = 0; i < classCount; i++)
        {
            ClassInfo cls;

            uint8_t classNameLen = read_uint8(classOffset);
            if (classOffset + classNameLen > classMetaEnd)
                throw std::runtime_error("Class name exceeds metadata bounds");

            cls.name.assign(reinterpret_cast<const char *>(&filedata[classOffset]), classNameLen);
            classOffset += classNameLen;

            cls.superClassIndex = static_cast<int32_t>(read_uint32(classOffset));

            DBG("Class: " << cls.name << ", Superclass Index: " << cls.superClassIndex);

            uint32_t fieldCount = read_uint32(classOffset);
            for (uint32_t f = 0; f < fieldCount; f++)
            {
                FieldInfo field;

                uint8_t fieldNameLen = read_uint8(classOffset);
                if (classOffset + fieldNameLen + 1 > classMetaEnd)
                    throw std::runtime_error("Field info exceeds metadata bounds\n Expected at least " + std::to_string(fieldNameLen + 1) + " bytes, but only " + std::to_string(classMetaEnd - classOffset) + " bytes remain");

                field.name.assign(reinterpret_cast<const char *>(&filedata[classOffset]), fieldNameLen);
                classOffset += fieldNameLen;

                field.type = static_cast<FieldType>(read_uint8(classOffset));

                cls.fields.push_back(field);
                DBG("Field: " << field.name << " Type: " << static_cast<int>(field.type));
            }

            uint32_t methodCount = read_uint32(classOffset);
            for (uint32_t m = 0; m < methodCount; m++)
            {
                MethodInfo method;

                uint8_t methodNameLen = read_uint8(classOffset);

                if (classOffset + methodNameLen + 4 > classMetaEnd)
                    throw std::runtime_error("Method info exceeds metadata bounds \nExpected at least " + std::to_string(methodNameLen + 4) + " bytes, but only " + std::to_string(classMetaEnd - classOffset) + " bytes remain");

                method.name.assign(reinterpret_cast<const char *>(&filedata[classOffset]), methodNameLen);
                classOffset += methodNameLen;

                method.bytecodeOffset = read_uint32(classOffset);

                cls.methods.push_back(method);
                DBG("Method: " << method.name << " Bytecode Offset: " << method.bytecodeOffset);
            }

            classes.push_back(std::move(cls));
        }

        if (classOffset != classMetaEnd)
        {
            throw std::runtime_error("Class metadata size mismatch after parsing");
        }
    }

    if (entryPoint >= code.size())
    {
        throw std::runtime_error("Entry point out of code segment bounds");
    }
    ip = entryPoint;
    DBG("Entry point set to " + std::to_string(ip));

    stack.clear();
}

void VM::run()
{
    while (ip < code.size())
    {
        Opcode opcode = static_cast<Opcode>(fetch8());

        switch (opcode)
        {

        case Opcode::IADD:
        {
            int b = pop(), a = pop();
            push(a + b);
            DBG("IADD, Stack top = " + std::to_string(stack.back()));
            break;
        }
        case Opcode::ISUB:
        {
            int b = pop(), a = pop();
            push(a - b);
            DBG("ISUB, Stack top = " + std::to_string(stack.back()));
            break;
        }
        case Opcode::IMUL:
        {
            int b = pop(), a = pop();
            push(a * b);
            DBG("IMUL, Stack top = " + std::to_string(stack.back()));
            break;
        }
        case Opcode::IDIV:
        {
            int b = pop(), a = pop();
            if (b == 0)
                throw std::runtime_error("Division by zero");
            push(a / b);
            DBG("IDIV, Stack top = " + std::to_string(stack.back()));
            break;
        }
        case Opcode::INEG:
        {
            int a = pop();
            push(-a);
            DBG("INEG, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::IMOD:
        {
            int b = pop(), a = pop();
            if (b == 0)
                throw std::runtime_error("Modulo by zero");
            push(a % b);
            DBG("IMOD, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::FADD:
        {
            uint32_t b = pop(), a = pop();
            float bf, af;
            std::memcpy(&bf, &b, sizeof(float));
            std::memcpy(&af, &a, sizeof(float));
            float cf = af + bf;
            std::memcpy(&a, &cf, sizeof(float));
            push(a);
            DBG("FADD, Stack top = " + std::to_string(cf));
            break;
        }
        case Opcode::FSUB:
        {
            uint32_t b = pop(), a = pop();
            float bf, af;
            std::memcpy(&bf, &b, sizeof(float));
            std::memcpy(&af, &a, sizeof(float));
            float cf = af - bf;
            std::memcpy(&a, &cf, sizeof(float));
            push(a);
            DBG("FSUB, Stack top = " + std::to_string(cf));
            break;
        }
        case Opcode::FMUL:
        {
            uint32_t b = pop(), a = pop();
            float bf, af;
            std::memcpy(&bf, &b, sizeof(float));
            std::memcpy(&af, &a, sizeof(float));
            float cf = af * bf;
            std::memcpy(&a, &cf, sizeof(float));
            push(a);
            DBG("FMUL, Stack top = " + std::to_string(cf));
            break;
        }
        case Opcode::FDIV:
        {
            uint32_t b = pop(), a = pop();
            float bf, af;
            std::memcpy(&bf, &b, sizeof(float));
            std::memcpy(&af, &a, sizeof(float));
            if (bf == 0.0f)
                throw std::runtime_error("Division by zero");
            float cf = af / bf;
            std::memcpy(&a, &cf, sizeof(float));
            push(a);
            DBG("FDIV, Stack top = " + std::to_string(cf));
            break;
        }

        case Opcode::FNEG:
        {
            uint32_t a = pop();
            float af;
            std::memcpy(&af, &a, sizeof(float));
            af = -af;
            std::memcpy(&a, &af, sizeof(float));
            push(a);
            DBG("FNEG, Stack top = " + std::to_string(af));
            break;
        }

        case Opcode::PUSH:
        {
            int32_t val = fetch32();
            push(val);
            DBG("PUSH " + std::to_string(val) + ", Stack top = " + std::to_string(stack.back()));
            break;
        }
        case Opcode::POP:
        {
            pop();
            DBG("POP ,Stack size = " + std::to_string(stack.size()));
            break;
        }

        case Opcode::FPUSH:
        {
            float val;
            uint32_t raw = fetch32();
            std::copy(reinterpret_cast<uint8_t *>(&raw), reinterpret_cast<uint8_t *>(&raw) + 4, reinterpret_cast<uint8_t *>(&val));
            uint32_t value;
            std::memcpy(&value, &val, sizeof(uint32_t));
            push(value);
            DBG("FPUSH " + std::to_string(val) + ", Stack top = " + std::to_string(val));
            break;
        }

        case Opcode::FPOP:
        {
            pop();
            DBG("FPOP ,Stack size = " + std::to_string(stack.size()));
            break;
        }

        case Opcode::DUP:
        {
            push(peek());
            DBG("DUP, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::LOAD:
        {
            uint32_t idx = fetch32();
            push(locals.at(idx));
            DBG("LOAD " + std::to_string((int)idx) + ", Value = " + std::to_string(locals.at(idx)) + ", Stack top = " + std::to_string(stack.back()));
            break;
        }
        case Opcode::STORE:
        {
            uint32_t idx = fetch32();
            locals.at(idx) = pop();
            DBG("STORE " + std::to_string((int)idx) + ", Value = " + std::to_string(locals.at(idx)));
            break;
        }

        case Opcode::LOAD_ARG:
        {
            uint8_t argIdx = fetch8();
            uint32_t argVal = stack.at(fp - 2 - argIdx); // arguments are pushed in reverse order :)
            push(argVal);
            DBG("LOAD_ARG " + std::to_string((int)argIdx) + ", Value = " + std::to_string(argVal) + ", Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::JMP:
        {
            uint16_t addr = fetch16();
            ip = addr;
            DBG("JMP to " + std::to_string(addr));
            break;
        }
        case Opcode::JZ:
        {
            uint16_t addr = fetch16();
            if (pop() == 0)
                ip = addr;
            DBG("JZ to " + std::to_string(addr));
            break;
        }
        case Opcode::JNZ:
        {
            uint16_t addr = fetch16();
            if (pop() != 0)
                ip = addr;
            DBG("JNZ to " + std::to_string(addr));
            break;
        }
        case Opcode::RET:
        {
            if (fp == 0)
            {
                DBG("RET at base frame, halting execution.");
                return;
            }
            if (fp < 1 || stack.size() < 2)
            {
                throw std::runtime_error("Stack underflow on RET");
            }

            uint32_t old_fp = stack[fp];

            uint32_t return_ip = stack[fp - 1];

            uint32_t itemsToPop = static_cast<int>(stack.size()) - (fp - 1);
            uint32_t returnValue = pop();
            for (int i = 0; i < itemsToPop - 1; i++)
            {
                pop();
            }

            fp = old_fp;
            ip = return_ip;

            for (uint8_t i = 0; i < args_to_pop; i++)
            {
                pop();
            }
            args_to_pop = 0;

            push(returnValue);

            DBG("RET to ip " << ip << ", restored FP = " << fp);
            break;
        }
        case Opcode::CALL:
        {
            uint32_t methodOffset = fetch32();
            uint8_t argCount = fetch8();

            args_to_pop = argCount;

            push(static_cast<int>(ip));
            push(static_cast<int>(fp));

            fp = static_cast<int>(stack.size()) - 1;
            ip = methodOffset;

            DBG("CALL to offset " + std::to_string(methodOffset) + ", return IP = " + std::to_string(stack[fp - 1]) + ", FP = " + std::to_string(fp));
            break;
        }

        case Opcode::ICMP_EQ:
        {
            int b = pop(), a = pop();
            push(a == b ? 1 : 0);
            DBG("ICMP_EQ, Stack top = " + std::to_string(stack.back()));
            break;
        }
        case Opcode::ICMP_LT:
        {
            int b = pop(), a = pop();
            push(a < b ? 1 : 0);
            DBG("ICMP_LT, Stack top = " + std::to_string(stack.back()));
            break;
        }
        case Opcode::ICMP_GT:
        {
            int b = pop(), a = pop();
            push(a > b ? 1 : 0);
            DBG("ICMP_GT, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::ICMP_GEQ:
        {
            int b = pop(), a = pop();
            push(a >= b ? 1 : 0);
            DBG("ICMP_GT, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::ICMP_NEQ:
        {
            int b = pop(), a = pop();
            push(a != b ? 1 : 0);
            DBG("ICMP_NEQ, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::ICMP_LEQ:
        {
            int b = pop(), a = pop();
            push(a <= b ? 1 : 0);
            DBG("ICMP_LEQ, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::FCMP_EQ:
        {
            uint32_t b = pop(), a = pop();
            float bf, af;
            std::memcpy(&bf, &b, sizeof(float));
            std::memcpy(&af, &a, sizeof(float));
            push(af == bf ? 1 : 0);
            DBG("FCMP_EQ, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::FCMP_LT:
        {
            uint32_t b = pop(), a = pop();
            float bf, af;
            std::memcpy(&bf, &b, sizeof(float));
            std::memcpy(&af, &a, sizeof(float));
            push(af < bf ? 1 : 0);
            DBG("FCMP_LT, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::FCMP_GT:
        {
            uint32_t b = pop(), a = pop();
            float bf, af;
            std::memcpy(&bf, &b, sizeof(float));
            std::memcpy(&af, &a, sizeof(float));
            push(af > bf ? 1 : 0);
            DBG("FCMP_GT, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::FCMP_GEQ:
        {
            uint32_t b = pop(), a = pop();
            float bf, af;
            std::memcpy(&bf, &b, sizeof(float));
            std::memcpy(&af, &a, sizeof(float));
            push(af >= bf ? 1 : 0);
            DBG("FCMP_GEQ, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::FCMP_NEQ:
        {
            uint32_t b = pop(), a = pop();
            float bf, af;
            std::memcpy(&bf, &b, sizeof(float));
            std::memcpy(&af, &a, sizeof(float));
            push(af != bf ? 1 : 0);
            DBG("FCMP_NEQ, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::FCMP_LEQ:
        {
            uint32_t b = pop(), a = pop();
            float bf, af;
            std::memcpy(&bf, &b, sizeof(float));
            std::memcpy(&af, &a, sizeof(float));
            push(af <= bf ? 1 : 0);
            DBG("FCMP_LEQ, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::NEW:
        {

            uint8_t classIndex = fetch8();
            if (classIndex >= classes.size())
            {
                throw std::runtime_error("NEW error: Invalid class index.");
            }
            const ClassInfo &cls = classes.at(classIndex);
            void *newObjectData = objectFactory.createObject(cls.name);
            heap.push_back(newObjectData);
            int32_t objRef = static_cast<int32_t>(heap.size() - 1);
            push(objRef);
            DBG("NEW " << cls.name << ", ObjRef: " << objRef);
            break;
        }
        case Opcode::GETFIELD:
        {

            uint8_t fieldIndex = fetch8();
            int32_t objRef = pop();
            if (objRef < 0 || static_cast<size_t>(objRef) >= heap.size())
            {
                throw std::runtime_error("GETFIELD error: Invalid object reference.");
            }
            void *objectData = heap.at(objRef);
            void *rawMemory = static_cast<char *>(objectData) - sizeof(void *);
            const ClassInfo *cls = *static_cast<const ClassInfo **>(rawMemory);

            DBG("GETFIELD, requested index: " + std::to_string((int)fieldIndex) + ", fieldCount: " + std::to_string(cls->fields.size()));
            DBG("filedIndex" << fieldIndex << " " << cls->fields.size());
            if (fieldIndex >= cls->fields.size())
            {
                throw std::runtime_error("GETFIELD error: Invalid field index.");
            }
            const FieldInfo &field = cls->fields.at(fieldIndex);
            size_t offset = cls->fieldOffsets.at(field.name);
            char *baseAddress = static_cast<char *>(objectData);
            int32_t value = *reinterpret_cast<int32_t *>(baseAddress + offset);
            push(value);
            DBG("GETFIELD from ObjRef " + std::to_string(objRef) + " (" + cls->name + "." + field.name + "), Value = " + std::to_string(value));
            break;
        }
        case Opcode::PUTFIELD:
        {

            uint8_t fieldIndex = fetch8();
            int32_t value = pop();
            int32_t objRef = pop();
            if (objRef < 0 || static_cast<size_t>(objRef) >= heap.size())
            {
                throw std::runtime_error("PUTFIELD error: Invalid object reference.");
            }
            void *objectData = heap.at(objRef);
            void *rawMemory = static_cast<char *>(objectData) - sizeof(void *);
            const ClassInfo *cls = *static_cast<const ClassInfo **>(rawMemory);
            if (fieldIndex >= cls->fields.size())
            {
                throw std::runtime_error("PUTFIELD error: Invalid field index.");
            }
            const FieldInfo &field = cls->fields.at(fieldIndex);
            size_t offset = cls->fieldOffsets.at(field.name);
            char *baseAddress = static_cast<char *>(objectData);
            *reinterpret_cast<int32_t *>(baseAddress + offset) = value;
            DBG("PUTFIELD on ObjRef " + std::to_string(objRef) + " (" + cls->name + "." + field.name + "), Value = " + std::to_string(value));
            break;
        }
        case Opcode::INVOKEVIRTUAL:
        {
            uint32_t methodOffset = fetch32();
            args_to_pop = fetch8();
            int32_t objRef = pop();

            if (objRef < 0 || static_cast<size_t>(objRef) >= heap.size())
            {
                throw std::runtime_error("INVOKEVIRTUAL error: Invalid object reference.");
            }
            void *objectData = heap.at(objRef);
            void *rawMemory = static_cast<char *>(objectData) - sizeof(void *); // remove cls metadata header
            const ClassInfo *cls = *static_cast<const ClassInfo **>(rawMemory);

            push(ip);
            push(fp);
            fp = static_cast<int>(stack.size()) - 1;
            ip = cls->vtable[methodOffset]->bytecodeOffset;
            DBG("INVOKEVIRTUAL to offset " + std::to_string(cls->vtable[methodOffset]->bytecodeOffset));
            break;
        }
        case Opcode::INVOKESPECIAL:
        {

            break;
        }

        case Opcode::NEWARRAY:
        {
            FieldType type = static_cast<FieldType>(fetch8());
            int size = pop(); // size of the array
            // int localidx = pop(); // local index to store array reference

            int multiplier = 1;
            switch (type)
            {
            case FieldType::INT:
            {
                multiplier = sizeof(int);
                break;
            }
            case FieldType::FLOAT:
            {
                multiplier = sizeof(float);
                break;
            }
            case FieldType::OBJECT:
            {
                multiplier = sizeof(void *);
                break;
            }
            case FieldType::CHAR:
            {
                multiplier = sizeof(char);
                break;
            }

            default:
                throw std::runtime_error("Unsupported array type");
            }

            void *rawarrayData = malloc(size * multiplier + multiplier + sizeof(void *));              // Allocate memory with header ;)
            *static_cast<FieldType *>(rawarrayData) = type;                                            // Store array type at the start
            void *arrayData = static_cast<void *>(static_cast<char *>(rawarrayData) + sizeof(void *)); // Pointer to actual array data

            heap.push_back(arrayData);
            // locals.at(localidx) = heap.size() - 1; // Store array reference in locals
            push(heap.size() - 1);

            DBG("NEWARRAY of type " + std::to_string(static_cast<int>(type)) + ", size " + std::to_string(size) + ", stored with reference " + std::to_string(heap.size() - 1));

            break;
        }

        case Opcode::ALOAD:
        {
            int index = pop();  // array index
            int arrayRef = pop(); // local index where array reference is stored
            // int arrayRef = locals.at(arrIdx);
            if (arrayRef < 0 || static_cast<size_t>(arrayRef) >= heap.size())
            {
                throw std::runtime_error("ALOAD error: Invalid array reference.");
            }
            void *arrayData = heap.at(arrayRef);

            FieldType arrayType = *reinterpret_cast<FieldType *>(static_cast<char *>(arrayData) - sizeof(void *));

            switch (arrayType)
            {
            case FieldType::INT:
            case FieldType::OBJECT:
            {
                int value = *reinterpret_cast<int *>(static_cast<char *>(arrayData) + index * sizeof(int));
                push(value);
                DBG("ALOAD from array ref " + std::to_string(arrayRef) + " at index " + std::to_string(index) + ", Stack top = " + std::to_string(stack.back()));
                break;
            }
            case FieldType::FLOAT:
            {
                float fvalue = *reinterpret_cast<float *>(static_cast<char *>(arrayData) + index * sizeof(float));
                uint32_t raw;
                std::memcpy(&raw, &fvalue, sizeof(uint32_t));
                push(raw);
                DBG("ALOAD from array ref " + std::to_string(arrayRef) + " at index " + std::to_string(index) + ", Stack top = " + std::to_string(fvalue));
                break;
            }
            case FieldType::CHAR:
            {
                char cvalue = *reinterpret_cast<char *>(static_cast<char *>(arrayData) + index * sizeof(char));
                push(static_cast<int>(cvalue));
                break;
            }
            }

            break;
        }

        case Opcode::ASTORE:
        {
            int value = pop();  // value to store
            int index = pop();  // index in the array
            int arrayRef = pop(); // heap index of array
            // int arrayRef = locals.at(arrIdx);
            if (arrayRef < 0 || static_cast<size_t>(arrayRef) >= heap.size())
            {
                throw std::runtime_error("ASTORE error: Invalid array reference.");
            }
            void *arrayData = heap.at(arrayRef);

            FieldType arrayType = *reinterpret_cast<FieldType *>(static_cast<char *>(arrayData) - sizeof(void *));
            switch (arrayType)
            {
            case FieldType::OBJECT:
            case FieldType::INT:
            {
                *reinterpret_cast<int *>(static_cast<char *>(arrayData) + index * sizeof(int)) = value;

                DBG("ASTORE to array ref " + std::to_string(arrayRef) + " at index " + std::to_string(index) + ", Value = " + std::to_string(value));
                break;
            }
            case FieldType::FLOAT:
            {
                float fvalue;
                std::memcpy(&fvalue, &value, sizeof(float));
                *reinterpret_cast<float *>(static_cast<char *>(arrayData) + index * sizeof(float)) = fvalue;
                DBG("ASTORE to array ref " + std::to_string(arrayRef) + " at index " + std::to_string(index) + ", Value = " + std::to_string(fvalue));
                break;
            }
            case FieldType::CHAR:
            {
                *reinterpret_cast<char *>(static_cast<char *>(arrayData) + index * sizeof(char)) = static_cast<char>(value);
                DBG("ASTORE to array ref " + std::to_string(arrayRef) + " at index " + std::to_string(index) + ", Value = " + std::to_string(static_cast<char>(value)));
                break;
            }

            break;
            }

            break;
        }

        case Opcode::SYS_CALL:
        {
            Syscall syscall = static_cast<Syscall>(fetch8());
            switch (syscall)
            {
            case Syscall::READ:
            {
                int fd = pop();                                                                      // Stack: file descriptor
                int size = pop();                                                                    // Stack: buffer size
                int localsIdx = pop();                                                               // Stack: local index to store buffer address
                void *rawbuffer = malloc(size + sizeof(void *));                                     // Allocate buffer
                void *buffer = static_cast<void *>(static_cast<char *>(rawbuffer) + sizeof(void *)); // following heap convention
                heap.push_back(buffer);
                locals.at(localsIdx) = heap.size() - 1; // Store buffer index in locals
                if (fileData.at(fd) == nullptr)
                {
                    throw std::runtime_error("SYS_READ error: Invalid file descriptor " + std::to_string(fd));
                }
                int bytesRead = fread(buffer, 1, size, fileData.at(fd));
                push(bytesRead); // Push number of bytes read onto stack

                DBG("SYS_READ from FD " + std::to_string(fd) + ", Requested Size = " + std::to_string(size) + ", Bytes Read = " + std::to_string(bytesRead));
                break;
            }
            case Syscall::WRITE:
            {
                int fd = pop();                 // Stack: file descriptor
                int size = pop();               // Stack: buffer size
                int locIdx = pop();             // Stack: buffer index
                int bufIdx = locals.at(locIdx); // Get buffer index from locals

                if (bufIdx < 0 || static_cast<size_t>(bufIdx) >= heap.size())
                {
                    throw std::runtime_error("SYS_WRITE error: Invalid buffer index " + std::to_string(bufIdx));
                }
                void *buffer = heap.at(bufIdx);
                if (fileData.at(fd) == nullptr)
                {
                    throw std::runtime_error("SYS_WRITE error: Invalid file descriptor " + std::to_string(fd));
                }
                int bytesWritten = fwrite(buffer, 1, size, fileData.at(fd));
                push(bytesWritten); // Push number of bytes written onto stack
                DBG("SYS_WRITE to FD " + std::to_string(fd) + ", Requested Size = " + std::to_string(size) + ", Bytes Written = " + std::to_string(bytesWritten));
                break;
            }
            case Syscall::OPEN:
            {
                char mode = pop();
                int32_t filenameIdx = pop();
                if (filenameIdx < 0 || static_cast<size_t>(filenameIdx) >= heap.size())
                {
                    throw std::runtime_error("SYS_OPEN error: Invalid filename index " + std::to_string(filenameIdx));
                }
                char *filename = static_cast<char *>(heap.at(filenameIdx));
                char modeStr[] = {mode, '\0'};
                int fd = -1;

                for (size_t i = 3; i < fileData.size(); i++)
                {
                    if (fileData.at(i) == nullptr)
                    {
                        FILE *f = fopen(filename, modeStr);
                        if (f == nullptr)
                        {
                            throw std::runtime_error("SYS_OPEN error: Failed to open file " + std::string(filename));
                        }
                        fileData.at(i) = f;
                        fd = i;
                        break;
                    }
                }

                if (fd == -1)
                {
                    throw std::runtime_error("SYS_OPEN error: Too many open files.");
                }

                push(fd);

                DBG("SYS_OPEN file " + std::string(filename) + " with mode " + mode + ", FD = " + std::to_string(fd));

                break;
            }
            case Syscall::CLOSE:
            {
                int fd = pop();
                if (fd < 0 || static_cast<size_t>(fd) >= fileData.size() || fileData.at(fd) == nullptr)
                {
                    throw std::runtime_error("SYS_CLOSE error: Invalid file descriptor " + std::to_string(fd));
                }
                fclose(fileData.at(fd));
                fileData.at(fd) = nullptr;
                DBG("SYS_CLOSE on FD " + std::to_string(fd));
                break;
            }
            case Syscall::EXIT:
            {
                int exitCode = pop();
                DBG("SYS_EXIT with code " + std::to_string(exitCode) + ", halting execution.");
                exit(exitCode);
            }
            default:
                throw std::runtime_error("Unsupported syscall: " + std::to_string((int)syscall));
            }
            break;
        }

        default:
            throw std::runtime_error("Unknown opcode: " + std::to_string((int)opcode));
        }
    }
}

uint32_t VM::top() const { return peek(); }

void VM::push(uint32_t v)
{
    if (stack.size() >= STACK_SIZE)
        throw std::runtime_error("Stack Overflow");
    stack.push_back(v);
}
uint32_t VM::pop()
{
    if (stack.empty())
        throw std::runtime_error("Stack Underflow");
    uint32_t v = stack.back();
    stack.pop_back();
    return v;
}
uint32_t VM::peek() const
{
    if (stack.empty())
        throw std::runtime_error("Empty stack");
    return stack.back();
}

uint8_t VM::fetch8() { return code.at(ip++); }
uint16_t VM::fetch16()
{
    uint16_t lo = fetch8();
    uint16_t hi = fetch8();
    return (hi << 8) | lo;
}
int32_t VM::fetch32()
{
    int32_t b1 = fetch8();
    int32_t b2 = fetch8();
    int32_t b3 = fetch8();
    int32_t b4 = fetch8();
    return (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
}
