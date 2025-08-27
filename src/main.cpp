/**
 * Author: Shivadharshan S
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <VM.hpp>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <vm_binary_file>" << std::endl;
        return 1;
    }

    const char *filename = argv[1];

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
    {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return 1;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0)
    {
        std::cerr << "Error: File is empty or invalid size" << std::endl;
        return 1;
    }

    std::vector<uint8_t> filedata(size);
    if (!file.read(reinterpret_cast<char *>(filedata.data()), size))
    {
        std::cerr << "Error: Failed to read file " << filename << std::endl;
        return 1;
    }

    try
    {
        VM vm(filedata);
        vm.run();
    }
    catch (const std::exception &ex)
    {
        std::cerr << "VM error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
