/**
 * Author: Shivadharshan S
 */
#include <fstream>
#include <vector>
using namespace std;

int main()
{
    std::vector<uint8_t> file = {
        // Header (44 bytes)
        0x56, 0x4D, 0x00, 0x01, // Magic "VM\x00\x01"
        0x01, 0x00, 0x00, 0x00, // Version = 1
        0x00, 0x00, 0x00, 0x00, // EntryPoint = 0 (start of main)
        0x2C, 0x00, 0x00, 0x00, // ConstPoolOffset = 44
        0x00, 0x00, 0x00, 0x00, // ConstPoolSize = 0
        0x2C, 0x00, 0x00, 0x00, // CodeOffset = 44
        0x24, 0x00, 0x00, 0x00, // CodeSize = 36 bytes
        0x44, 0x00, 0x00, 0x00, // GlobalsOffset = 68 (right after code)
        0x00, 0x00, 0x00, 0x00, // GlobalsSize = 0
        0x44, 0x00, 0x00, 0x00, // ClassMetadataOffset = 68
        0x00, 0x00, 0x00, 0x00, // ClassMetadataSize = 0

        // Code segment starting at offset 44:
        // Caller main:
        0x10, 0x03, 0x00, 0x00, 0x00, // PUSH 3  (argument c)
        0x10, 0x07, 0x00, 0x00, 0x00, // PUSH 7  (argument b)
        0x10, 0x05, 0x00, 0x00, 0x00, // PUSH 5  (argument a)
        0x33, 0x1B, 0x00, 0x00, 0x00, // CALL 20 (function offset 0x14)
        0x10, 0x03, 0x00, 0x00, 0x00, // PUSH 3
        0x03,                         // IADD    (result + 3)
        0x34,                         // RET     (end of main)

        // Function sum at offset 20 (0x14)
        0x22, 0x00, // LOAD_ARG 0  (a)
        0x22, 0x01, // LOAD_ARG 1  (b)
        0x01,       // IADD         (a + b)
        0x22, 0x02, // LOAD_ARG 2  (c)
        0x01,       // IADD         ((a+b) + c)
        0x34        // RET         (return sum)
    };

    ofstream out("test3_function_call.vm", ios::binary);
    out.write((char *)file.data(), file.size());
}
