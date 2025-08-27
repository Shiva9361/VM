/**
 * Author: Shivadharshan S
 */
#include <fstream>
#include <vector>
using namespace std;

int main()
{
    std::vector<uint8_t> file = {
        0x56, 0x4D, 0x00, 0x01, // magic (4 bytes)
        0x01, 0x00, 0x00, 0x00, // version (4 bytes)
        0x00, 0x00, 0x00, 0x00, // entry point (4 bytes)

        0x24, 0x00, 0x00, 0x00, // const pool offset = 36 (header size)
        0x00, 0x00, 0x00, 0x00, // const pool size

        0x24, 0x00, 0x00, 0x00, // code offset (same as const pool offset because const pool empty)
        0x0C, 0x00, 0x00, 0x00, // code size (12 bytes)

        0x30, 0x00, 0x00, 0x00, // globals offset = 48 decimal (36 header + 12 code)
        0x00, 0x00, 0x00, 0x00  // globals size (empty)
    };
    std::vector<uint8_t> code = {
        0x10, 0x00, 0x00, 0x00, 0x03, // PUSH 3
        0x10, 0x00, 0x00, 0x00, 0x07, // PUSH 7
        0x01,                         // IADD
        0x34                          // RET
    };

    file.insert(file.end(), code.begin(), code.end());

    ofstream out("test1.vm", ios::binary);
    out.write((char *)file.data(), file.size());
}
