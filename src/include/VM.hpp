/**
 * Author: Shivadharshan S
 */
#ifndef VM_HPP
#define VM_HPP

#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>

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
    PUSH = 0x10,
    POP = 0x11,
    DUP = 0x12,
    LOAD = 0x20,
    STORE = 0x21,
    JMP = 0x30,
    JZ = 0x31,
    JNZ = 0x32,
    CALL = 0x33,
    RET = 0x34,
    ICMP_EQ = 0x40,
    ICMP_LT = 0x41,
    ICMP_GT = 0x42,
    NEW = 0x50,
    GETFIELD = 0x51,
    PUTFIELD = 0x52,
    INVOKEVIRTUAL = 0x53,
    INVOKESPECIAL = 0x54
};

class VM
{
public:
    VM(const std::vector<uint8_t> &filedata);
    void loadFromBinary(const std::vector<uint8_t> &filedata);

    void run();
    int top() const;

private:
    static constexpr int STACK_SIZE = 1024;
    static constexpr int LOCALS_SIZE = 256;
    static constexpr int CONST_POOL_SIZE = 256;

    std::vector<int> stack;
    std::vector<int> locals;
    std::vector<int> constantPool;
    std::vector<uint8_t> code;
    size_t ip;
    size_t sp;
    size_t fp;

    void push(int v);
    int pop();
    int peek() const;

    uint8_t fetch8();
    uint16_t fetch16();
    int32_t fetch32();
};

#endif // VM_HPP
