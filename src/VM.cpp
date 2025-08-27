/**
 * Author: Shivadharshan S
 */
#include <VM.hpp>
#include <algorithm>

VM::VM(const std::vector<uint8_t> &filedata)
    : ip(0)
{
    loadFromBinary(filedata);

    stack.reserve(STACK_SIZE);
    locals.resize(LOCALS_SIZE, 0);
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

    DBG("VM Version: " << version);
    DBG("Entry Point: " << entryPoint);
    DBG("Const Pool Offset: " << constPoolOffset << ", Size: " << constPoolSize);
    DBG("Code Offset: " << codeOffset << ", Size: " << codeSize);
    DBG("Globals Offset: " << globalsOffset << ", Size: " << globalsSize);

    if (constPoolOffset + constPoolSize > filedata.size())
    {
        throw std::runtime_error("Constant pool section out of file bounds");
    }
    constantPool.clear();
    // Assuming constant pool entries are 4-byte integers for simplicity:
    if (constPoolSize % 4 != 0)
        throw std::runtime_error("Constant pool size not multiple of 4");

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
        throw std::runtime_error("Globals section out of file bounds");

    if (globalsSize % 4 != 0)
        throw std::runtime_error("Globals section size not multiple of 4");

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
        case Opcode::DUP:
        {
            int v = peek();
            push(v);
            DBG("DUP, Stack top = " + std::to_string(stack.back()));
            break;
        }

        case Opcode::LOAD:
        {
            uint8_t idx = fetch8();
            push(locals.at(idx));
            DBG("LOAD " + std::to_string((int)idx) + ", Value = " + std::to_string(locals.at(idx)) + ", Stack top = " + std::to_string(stack.back()));
            break;
        }
        case Opcode::STORE:
        {
            uint8_t idx = fetch8();
            locals.at(idx) = pop();
            DBG("STORE " + std::to_string((int)idx) + ", Value = " + std::to_string(locals.at(idx)));
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
            return;

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

        case Opcode::NEW:
        {
            uint8_t classRef = fetch8();
            push(classRef * 1000); // TODO: dummy handle
            break;
        }
        case Opcode::GETFIELD:
        {
            uint8_t fieldRef = fetch8();
            int obj = pop();
            push(obj + fieldRef); // TODO: dummy behavior
            break;
        }
        case Opcode::PUTFIELD:
        {
            uint8_t fieldRef = fetch8();
            int val = pop();
            int obj = pop();
            (void)fieldRef;
            (void)val;
            (void)obj; // TODO: stub
            break;
        }
        case Opcode::INVOKEVIRTUAL:
        {
            uint8_t methRef = fetch8();
            push(methRef * 111); // stub return
            break;
        }
        case Opcode::INVOKESPECIAL:
        {
            (void)fetch8(); // TODO create
            break;
        }

        default:
            throw std::runtime_error("Unknown opcode: " + std::to_string((int)opcode));
        }
    }
}

int VM::top() const { return peek(); }

void VM::push(int v)
{
    if (stack.size() >= STACK_SIZE)
        throw std::runtime_error("Stack Overflow");
    stack.push_back(v);
}
int VM::pop()
{
    if (stack.empty())
        throw std::runtime_error("Stack Underflow");
    int v = stack.back();
    stack.pop_back();
    return v;
}
int VM::peek() const
{
    if (stack.empty())
        throw std::runtime_error("Empty stack");
    return stack.back();
}

uint8_t VM::fetch8() { return code.at(ip++); }
uint16_t VM::fetch16()
{
    uint16_t hi = fetch8();
    uint16_t lo = fetch8();
    return (hi << 8) | lo;
}
int32_t VM::fetch32()
{
    int32_t b1 = fetch8();
    int32_t b2 = fetch8();
    int32_t b3 = fetch8();
    int32_t b4 = fetch8();
    return (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
}
