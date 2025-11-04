/**
 * Author: Shivadharshan S
 */
#include <fstream>
#include <vector>
using namespace std;

int main()
{
    std::vector<uint8_t> file = {
        // Header (44 bytes) - adjusted for code and class metadata sizes
        0x56,
        0x4D,
        0x00,
        0x01, // Magic
        0x01,
        0x00,
        0x00,
        0x00, // Version =1
        0x00,
        0x00,
        0x00,
        0x00, // EntryPoint =0 (start main)
        0x2C,
        0x00,
        0x00,
        0x00, // ConstPoolOffset =44
        0x00,
        0x00,
        0x00,
        0x00, // ConstPoolSize =0
        0x2C,
        0x00,
        0x00,
        0x00, // CodeOffset =44
        0x28,
        0x00,
        0x00,
        0x00, // CodeSize =37 bytes
        0x2C,
        0x00,
        0x00,
        0x00, // GlobalsOffset =81
        0x00,
        0x00,
        0x00,
        0x00, // GlobalsSize =0
        0x2C,
        0x00,
        0x00,
        0x00, // ClassMetadataOffset =81 (0x51)
        0x00,
        0x00,
        0x00,
        0x00, // ClassMetadataSize =42 bytes

        // Code segment (37 bytes = 0x25)
        // 0x00:
        // 0x50, 0x01, // NEW classRef=1 (Derived)
        // 0x53, 0x00, // INVOKEVIRTUAL method vtable index 0 (calls foo)
        // 0x50, 0x00,
        // 0x53, 0x00,
        // 0x34, // RET
        // 0x10, 0x04, 0x00, 0x00, 0x00, // push local idx
        // 0x10, 0x03, 0x00, 0x00, 0x00, // push size
        // 0x10, 0x00, 0x00, 0x00, 0x00, // push fd
        // 0x60, 0x02,                   // call read
        // 0x10, 0x04, 0x00, 0x00, 0x00, // push local idx
        // 0x10, 0x03, 0x00, 0x00, 0x00, // push size
        // 0x10, 0x01, 0x00, 0x00, 0x00, // push fd
        // 0x60, 0x07,                   // call write
        // 0x34
        0x10,
        0x00,
        0x00,
        0x00,
        0x00, // PUSH local idx 0
        0x10,
        0x0A,
        0x00,
        0x00,
        0x00, // PUSH size 10
        0x70,
        0x03, // NEWARRAY of type FLOAT
        0x10,
        0x00,
        0x00,
        0x00,
        0x00, // PUSH local idx 0
        0x10,
        0x01,
        0x00,
        0x00,
        0x00, // PUSH array index 1
        0x10,
        0x00,
        0x00,
        0x20,
        0x41, // PUSH float 10.0 (bits 0x41200000)
        0x72,
        0x10,
        0x00,
        0x00,
        0x00,
        0x00, // PUSH local idx 0
        0x10,
        0x01,
        0x00,
        0x00,
        0x00, // PUSH array index 1
        0x71,
        0x34

        // // Base::foo at offset 6 (0x06)
        // 0x14, 0x00, 0x00, 0x80, 0x3F, // FPUSH 1.0f (float bits 0x3F800000 little endian)
        // 0x34,                         // RET

        // // Derived::foo at offset 11 (0x0B)
        // 0x14, 0x00, 0x00, 0x00, 0x40, // FPUSH 2.0f (float bits 0x40000000)
        // 0x34,                         // RET

        // Class Metadata (example, simplified):

        // ffset// Number of classes = 2
        // 0x02, 0x00, 0x00, 0x00,

        // // Class 0: Base
        // 0x04, 'B', 'a', 's', 'e', // Class name length and name
        // 0xFF, 0xFF, 0xFF, 0xFF,   // Superclass index = -1 (no superclass)
        // 0x00, 0x00, 0x00, 0x00,   // Fields count = 0
        // 0x01, 0x00, 0x00, 0x00,   // Methods count = 1

        // // Method: foo
        // 0x03, 'f', 'o', 'o',    // Method name length and name
        // 0x09, 0x00, 0x00, 0x00, // Bytecode offset = 6 (Base::foo)

        // // Class 1: Derived
        // 0x07, 'D', 'e', 'r', 'i', 'v', 'e', 'd',
        // 0x00, 0x00, 0x00, 0x00, // Superclass index = 0 (Base)
        // 0x00, 0x00, 0x00, 0x00, // Fields count = 0
        // 0x01, 0x00, 0x00, 0x00, // Methods count = 1

        // // Method: foo override
        // 0x03, 'f', 'o', 'o',
        // 0x0F, 0x00, 0x00, 0x00, // Bytecode o = 11 (Derived::foo)

        // Vtable construction implied during class load:
        // Base.vtable[0] = &Base::foo
        // Derived.vtable[0] = &Derived::foo (override)
    };

    ofstream out("test_array.vm", ios::binary);
    out.write((char *)file.data(), file.size());
}
