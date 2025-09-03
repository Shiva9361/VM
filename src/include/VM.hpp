/**
 * Author: Shivadharshan S
 */
#ifndef VM_HPP
#define VM_HPP

#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <object_factory.hpp>

#ifdef VM_DEBUG
#define VM_CPP_DEBUG // Enable debug output in VM.cpp
#define DBG(msg)                                        \
    do                                                  \
    {                                                   \
        std::cerr << "[VM DEBUG] " << msg << std::endl; \
    } while (0)
#else
#define DBG(msg) // nothing
#endif

enum class Opcode : uint8_t
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
    NEW = 0x50,
    GETFIELD = 0x51,
    PUTFIELD = 0x52,
    INVOKEVIRTUAL = 0x53,
    INVOKESPECIAL = 0x54
};

union Value
{
    int32_t intValue;
    float floatValue;
    Value() : intValue(0) {}
    Value(int v) : intValue(v) {}
    Value(float v) : floatValue(v) {}
};

class VM
{
public:
    VM(const std::vector<uint8_t> &filedata);
    ~VM(); // Added by Mokshith
    void loadFromBinary(const std::vector<uint8_t> &filedata);

    void run();
    Value top() const;

private:
    static constexpr int STACK_SIZE = 1024;
    static constexpr int LOCALS_SIZE = 256;
    static constexpr int CONST_POOL_SIZE = 256;

    std::vector<Value> stack;
    std::vector<int> locals;
    std::vector<int> constantPool;
    std::vector<ClassInfo> classes;
    std::vector<uint8_t> code;
    u_int ip;
    u_int sp;
    u_int fp;

    ObjectFactory objectFactory; // Added by Mokshith
    std::vector<void *> heap;    // Added by Mokshith

    void push(Value v);
    Value pop();
    Value peek() const;

    uint8_t fetch8();
    uint16_t fetch16();
    int32_t fetch32();
};

#endif // VM_HPP
