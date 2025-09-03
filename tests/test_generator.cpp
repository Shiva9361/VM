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
        0x25, 0x00, 0x00, 0x00, // CodeSize = 37 bytes
        0x51, 0x00, 0x00, 0x00, // GlobalsOffset = 81
        0x00, 0x00, 0x00, 0x00, // GlobalsSize = 0
        0x51, 0x00, 0x00, 0x00, // ClassMetadataOffset = 81
        0x00, 0x00, 0x00, 0x00, // ClassMetadataSize = 0

        // Code segment starting at offset 44:
        // Caller main:
        0x14, 0x00, 0x00, 0x60, 0x40, // FPUSH 3.5f
        0x14, 0xCD, 0xCC, 0xE6, 0x40, // FPUSH 7.2f
        0x14, 0xCD, 0xCC, 0xA2, 0x40, // FPUSH 5.1f
        0x33, 0x15, 0x00, 0x00, 0x00, // CALL 21(0x15)
        0x34,                         // RET

        // Function at offset 36(0x24):
        0x22, 0x00, // LOAD_ARG 0 a
        0x22, 0x01, // LOAD_ARG 1 b
        0x06,       // FADD      (a + b)
        0x22, 0x02, // LOAD_ARG 2 c
        0x07,       // FSUB      (a + b - c)
        0x22, 0x02, // LOAD_ARG 2 c
        0x08,       // FMUL      (a + b - c) * c
        0x22, 0x00, // LOAD_ARG 0 a
        0x09,       // FDIV      ((a + b - c) * c) / a
        0x0A,       // FNEG      negate the result
        0x34};

    ofstream out("test_floating_point.vm", ios::binary);
    out.write((char *)file.data(), file.size());
}
