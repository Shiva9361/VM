#include <fstream>
#include <vector>
#include <cstdint> 

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
    LOAD_ARG = 0x22,
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

enum class FieldType : uint8_t
{
    INT = 0x01,
    OBJECT = 0x02
};

int main()
{
    std::vector<uint8_t> file = {
        // ========== HEADER (44 bytes) ==========
        // All header values have been recalculated and are correct for this bytecode.
        0x56, 0x4D, 0x00, 0x01, // Magic "VM\x00\x01"
        0x01, 0x00, 0x00, 0x00, // Version = 1
        0x00, 0x00, 0x00, 0x00, // EntryPoint = 0 (start of code)
        0x2C, 0x00, 0x00, 0x00, // ConstPoolOffset = 44
        0x00, 0x00, 0x00, 0x00, // ConstPoolSize = 0
        0x2C, 0x00, 0x00, 0x00, // CodeOffset = 44
        0x20, 0x00, 0x00, 0x00, // CodeSize = 32 bytes
        0x68, 0x00, 0x00, 0x00, // GlobalsOffset = 104
        0x00, 0x00, 0x00, 0x00, // GlobalsSize = 0
        0x4C, 0x00, 0x00, 0x00, // ClassMetadataOffset = 76 (44 + 32)
        0x1C, 0x00, 0x00, 0x00, // ClassMetadataSize = 28 bytes

        // ========== CODE (32 bytes, starts at offset 44) ==========
        // GOAL: Create a Point, set x=10, y=20. Then get x, get y, and add them.
        // Expected final value on stack: 30

        // Create object, store its reference in local variable 0.
        (uint8_t)Opcode::NEW, 0x00,         // Stack: [objRef]
        (uint8_t)Opcode::STORE, 0x00,       // Stack: []

        // Set field 'x' to 10.
        (uint8_t)Opcode::LOAD, 0x00,        // Stack: [objRef]
        (uint8_t)Opcode::PUSH, 0x0A, 0x00, 0x00, 0x00, // Stack: [objRef, 10]
        (uint8_t)Opcode::PUTFIELD, 0x00,    // Stack: []

        // Set field 'y' to 20.
        (uint8_t)Opcode::LOAD, 0x00,        // Stack: [objRef]
        (uint8_t)Opcode::PUSH, 0x14, 0x00, 0x00, 0x00, // Stack: [objRef, 20]
        (uint8_t)Opcode::PUTFIELD, 0x01,    // Stack: []

        // Get field 'x' and push its value (10) to the stack.
        (uint8_t)Opcode::LOAD, 0x00,        // Stack: [objRef]
        (uint8_t)Opcode::GETFIELD, 0x00,    // Stack: [10]

        // Get field 'y' and push its value (20) to the stack.
        (uint8_t)Opcode::LOAD, 0x00,        // Stack: [10, objRef]
        (uint8_t)Opcode::GETFIELD, 0x01,    // Stack: [10, 20]

        // Add the two values.
        (uint8_t)Opcode::IADD,              // Stack: [30]

        // Halt the VM.
        (uint8_t)Opcode::RET,

        // ========== CLASS METADATA (28 bytes, starts at offset 76) ==========
        0x01, 0x00, 0x00, 0x00,       // ClassCount = 1
        // -- Class 0: Point --
        0x05,                         // Name length = 5
        'P', 'o', 'i', 'n', 't',      // "Point"
        0xFF, 0xFF, 0xFF, 0xFF,       // SuperClassIndex = -1 (no superclass)
        0x02, 0x00, 0x00, 0x00,       // FieldCount = 2
            // Field 0: x
            0x01, 'x', (uint8_t)FieldType::INT,
            // Field 1: y
            0x01, 'y', (uint8_t)FieldType::INT,
        0x00, 0x00, 0x00, 0x00        // MethodCount = 0
    };

    // Write the byte vector to a binary file
    std::ofstream out("test_object.vm", std::ios::binary);
    out.write(reinterpret_cast<const char*>(file.data()), file.size());
}