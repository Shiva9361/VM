/**
 * Author: Shivadharshan S
 */
#include <VM.hpp>
#include <algorithm>

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

    stack.reserve(STACK_SIZE);
    locals.resize(LOCALS_SIZE, 0);
}

/* Code Added By Mokshith - Start*/
VM::~VM()
{
    for (void *obj_data : heap)
    {
        objectFactory.destroyObject(obj_data);
    }
}
/* Code Added By Mokshith - End*/

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
            int b = pop().intValue, a = pop().intValue;
            push(a + b);
            DBG("IADD, Stack top = " + std::to_string(stack.back().intValue));
            break;
        }
        case Opcode::ISUB:
        {
            int b = pop().intValue, a = pop().intValue;
            push(a - b);
            DBG("ISUB, Stack top = " + std::to_string(stack.back().intValue));
            break;
        }
        case Opcode::IMUL:
        {
            int b = pop().intValue, a = pop().intValue;
            push(a * b);
            DBG("IMUL, Stack top = " + std::to_string(stack.back().intValue));
            break;
        }
        case Opcode::IDIV:
        {
            int b = pop().intValue, a = pop().intValue;
            if (b == 0)
                throw std::runtime_error("Division by zero");
            push(a / b);
            DBG("IDIV, Stack top = " + std::to_string(stack.back().intValue));
            break;
        }
        case Opcode::INEG:
        {
            int a = pop().intValue;
            push(-a);
            DBG("INEG, Stack top = " + std::to_string(stack.back().intValue));
            break;
        }

        case Opcode::FADD:
        {
            float b = pop().floatValue, a = pop().floatValue;
            push(a + b);
            DBG("FADD, Stack top = " + std::to_string(stack.back().floatValue));
            break;
        }
        case Opcode::FSUB:
        {
            float b = pop().floatValue, a = pop().floatValue;
            push(a - b);
            DBG("FSUB, Stack top = " + std::to_string(stack.back().floatValue));
            break;
        }
        case Opcode::FMUL:
        {
            float b = pop().floatValue, a = pop().floatValue;
            push(a * b);
            DBG("FMUL, Stack top = " + std::to_string(stack.back().floatValue));
            break;
        }
        case Opcode::FDIV:
        {
            float b = pop().floatValue, a = pop().floatValue;
            if (b == 0.0f)
                throw std::runtime_error("Division by zero");
            push(a / b);
            DBG("FDIV, Stack top = " + std::to_string(stack.back().floatValue));
            break;
        }

        case Opcode::FNEG:
        {
            float a = pop().floatValue;
            push(-a);
            DBG("FNEG, Stack top = " + std::to_string(stack.back().floatValue));
            break;
        }

        case Opcode::PUSH:
        {
            int32_t val = fetch32();
            push(val);
            DBG("PUSH " + std::to_string(val) + ", Stack top = " + std::to_string(stack.back().intValue));
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
            push(val);
            DBG("FPUSH " + std::to_string(val) + ", Stack top = " + std::to_string(stack.back().floatValue));
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
            Value v = peek();
            push(v);
            DBG("DUP, Stack top = " + std::to_string(stack.back().intValue));
            break;
        }

        case Opcode::LOAD:
        {
            uint8_t idx = fetch8();
            push(locals.at(idx));
            DBG("LOAD " + std::to_string((int)idx) + ", Value = " + std::to_string(locals.at(idx)) + ", Stack top = " + std::to_string(stack.back().intValue));
            break;
        }
        case Opcode::STORE:
        {
            uint8_t idx = fetch8();
            locals.at(idx) = pop().intValue;
            DBG("STORE " + std::to_string((int)idx) + ", Value = " + std::to_string(locals.at(idx)));
            break;
        }

        case Opcode::LOAD_ARG:
        {
            uint8_t argIdx = fetch8();
            Value argVal = stack.at(fp - 2 - argIdx); // arguments are pushed in reverse order :)
            push(argVal);
            DBG("LOAD_ARG " + std::to_string((int)argIdx) + ", Value = " + std::to_string(argVal.intValue) + ", Stack top = " + std::to_string(stack.back().intValue));
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
            if (pop().intValue == 0)
                ip = addr;
            DBG("JZ to " + std::to_string(addr));
            break;
        }
        case Opcode::JNZ:
        {
            uint16_t addr = fetch16();
            if (pop().intValue != 0)
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

            int old_fp = stack[fp].intValue;

            int return_ip = stack[fp - 1].intValue;

            int itemsToPop = static_cast<int>(stack.size()) - (fp - 1);
            Value returnValue = pop();
            for (int i = 0; i < itemsToPop - 1; i++)
            {
                pop();
            }

            fp = old_fp;
            ip = return_ip;
            push(returnValue);

            DBG("RET to ip " << ip << ", restored FP = " << fp);
            break;
        }
        case Opcode::CALL:
        {
            uint32_t methodOffset = fetch32();

            push(static_cast<int>(ip));
            push(static_cast<int>(fp));

            fp = static_cast<int>(stack.size()) - 1;
            ip = methodOffset;

            DBG("CALL to offset " << methodOffset << ", return IP = " << stack[fp - 1].intValue << ", FP = " << fp);
            break;
        }

        case Opcode::ICMP_EQ:
        {
            int b = pop().intValue, a = pop().intValue;
            push(a == b ? 1 : 0);
            DBG("ICMP_EQ, Stack top = " + std::to_string(stack.back().intValue));
            break;
        }
        case Opcode::ICMP_LT:
        {
            int b = pop().intValue, a = pop().intValue;
            push(a < b ? 1 : 0);
            DBG("ICMP_LT, Stack top = " + std::to_string(stack.back().intValue));
            break;
        }
        case Opcode::ICMP_GT:
        {
            int b = pop().intValue, a = pop().intValue;
            push(a > b ? 1 : 0);
            DBG("ICMP_GT, Stack top = " + std::to_string(stack.back().intValue));
            break;
        }

        case Opcode::FCMP_EQ:
        {
            float b = pop().floatValue, a = pop().floatValue;
            push(a == b ? 1 : 0);
            DBG("FCMP_EQ, Stack top = " + std::to_string(stack.back().floatValue));
            break;
        }

        case Opcode::FCMP_LT:
        {
            float b = pop().floatValue, a = pop().floatValue;
            push(a < b ? 1 : 0);
            DBG("FCMP_LT, Stack top = " + std::to_string(stack.back().floatValue));
            break;
        }

        case Opcode::FCMP_GT:
        {
            float b = pop().floatValue, a = pop().floatValue;
            push(a > b ? 1 : 0);
            DBG("FCMP_GT, Stack top = " + std::to_string(stack.back().floatValue));
            break;
        }

        case Opcode::NEW:
        {
            // uint8_t classRef = fetch8();
            // break;

            // Code added by Mokshith - start
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
            // Code added by Mokshith - end
        }
        case Opcode::GETFIELD:
        {
            // uint8_t fieldRef = fetch8();
            // int obj = pop();
            // push(obj + fieldRef); // TODO: dummy behavior
            // break;

            // Code added by Mokshith - start
            uint8_t fieldIndex = fetch8();
            int32_t objRef = pop().intValue;
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
            // Code added by Mokshith - End
        }
        case Opcode::PUTFIELD:
        {
            // uint8_t fieldRef = fetch8();
            // int val = pop();
            // int obj = pop();
            // (void)fieldRef;
            // (void)val;
            // (void)obj; // TODO: stub
            // break;

            // Code added by Mokshith - Start
            uint8_t fieldIndex = fetch8();
            int32_t value = pop().intValue;
            int32_t objRef = pop().intValue;
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
            // Code added by Mokshith - End
        }
        case Opcode::INVOKEVIRTUAL:
        {
            uint32_t methodOffset = fetch8();
            int32_t objRef = pop().intValue;

            if (objRef < 0 || static_cast<size_t>(objRef) >= heap.size())
            {
                throw std::runtime_error("INVOKEVIRTUAL error: Invalid object reference.");
            }
            void *objectData = heap.at(objRef);
            void *rawMemory = static_cast<char *>(objectData) - sizeof(void *); // remove cls metadata header
            const ClassInfo *cls = *static_cast<const ClassInfo **>(rawMemory);

            push(Value(static_cast<int>(ip)));
            push(Value(static_cast<int>(fp)));
            fp = static_cast<int>(stack.size()) - 1;
            ip = cls->vtable[methodOffset]->bytecodeOffset;
            DBG("INVOKEVIRTUAL to offset " + std::to_string(cls->vtable[methodOffset]->bytecodeOffset));
            break;
        }
        case Opcode::INVOKESPECIAL:
        {
            // (void)fetch8(); // TODO create
            // break;

            // Code added by Mokshith - Start

            break;
            // Code added by Mokshith - End
        }

        default:
            throw std::runtime_error("Unknown opcode: " + std::to_string((int)opcode));
        }
    }
}

Value VM::top() const { return peek(); }

void VM::push(Value v)
{
    if (stack.size() >= STACK_SIZE)
        throw std::runtime_error("Stack Overflow");
    stack.push_back(v);
}
Value VM::pop()
{
    if (stack.empty())
        throw std::runtime_error("Stack Underflow");
    Value v = stack.back();
    stack.pop_back();
    return v;
}
Value VM::peek() const
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
