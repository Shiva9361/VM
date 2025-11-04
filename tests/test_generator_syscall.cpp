
#include <fstream>
#include <vector>
#include <string>

using namespace std;

// Function to append a 32-bit integer to the bytecode vector
void emit_i32(vector<uint8_t> &bytecode, int32_t value)
{
    bytecode.push_back(value & 0xFF);
    bytecode.push_back((value >> 8) & 0xFF);
    bytecode.push_back((value >> 16) & 0xFF);
    bytecode.push_back((value >> 24) & 0xFF);
}

// Function to append an opcode to the bytecode vector
void emit_opcode(vector<uint8_t> &bytecode, uint8_t opcode)
{
    bytecode.push_back(opcode);
}

int main()
{
    vector<uint8_t> bytecode;

    // Header
    emit_i32(bytecode, 0x01004D56);
    emit_i32(bytecode, 1);  // Version
    emit_i32(bytecode, 0);  // EntryPoint
    emit_i32(bytecode, 44); // ConstPoolOffset
    emit_i32(bytecode, 0);  // ConstPoolSize
    emit_i32(bytecode, 44); // CodeOffset
    emit_i32(bytecode, 0);  // CodeSize (will be updated later)
    emit_i32(bytecode, 0);  // GlobalsOffset
    emit_i32(bytecode, 0);  // GlobalsSize
    emit_i32(bytecode, 0);  // ClassMetadataOffset
    emit_i32(bytecode, 0);  // ClassMetadataSize

    vector<uint8_t> code;

    // 1. Create a file named "test.txt"
    // Push filename to heap (as a char array)
    string filename = "test.txt";
    emit_opcode(code, 0x10);             // PUSH
    emit_i32(code, 0);                   // local index to store array ref
    emit_opcode(code, 0x10);             // PUSH
    emit_i32(code, filename.size() + 1); // size of the array
    emit_opcode(code, 0x70);             // NEWARRAY
    emit_opcode(code, 0x04);             // of type CHAR

    for (size_t i = 0; i < filename.size(); ++i)
    {
        emit_opcode(code, 0x10);     // PUSH
        emit_i32(code, 0);           // local index
        emit_opcode(code, 0x10);     // PUSH
        emit_i32(code, i);           // index in array
        emit_opcode(code, 0x10);     // PUSH
        emit_i32(code, filename[i]); // value
        emit_opcode(code, 0x72);     // ASTORE
    }
    emit_opcode(code, 0x10);         // PUSH
    emit_i32(code, 0);               // local index
    emit_opcode(code, 0x10);         // PUSH
    emit_i32(code, filename.size()); // index in array
    emit_opcode(code, 0x10);         // PUSH
    emit_i32(code, 0);               // null terminator
    emit_opcode(code, 0x72);         // ASTORE

    // Open the file for writing
    emit_opcode(code, 0x10); // PUSH
    emit_i32(code, 0);       // filename index on heap
    emit_opcode(code, 0x10); // PUSH
    emit_i32(code, 'w');     // mode
    emit_opcode(code, 0x60); // SYS_CALL
    emit_opcode(code, 0x01); // OPEN
    emit_opcode(code, 0x21); // STORE
    emit_i32(code, 3);       // local index 3 for fd

    // 2. Write "Hello, World!" to the file
    string message = "Hello, World!";
    // Create a buffer for the message
    emit_opcode(code, 0x10);        // PUSH
    emit_i32(code, 1);              // local index to store array ref
    emit_opcode(code, 0x10);        // PUSH
    emit_i32(code, message.size()); // size of the array
    emit_opcode(code, 0x70);        // NEWARRAY
    emit_opcode(code, 0x04);        // of type CHAR

    for (size_t i = 0; i < message.size(); ++i)
    {
        emit_opcode(code, 0x10);    // PUSH
        emit_i32(code, 1);          // local index
        emit_opcode(code, 0x10);    // PUSH
        emit_i32(code, i);          // index in array
        emit_opcode(code, 0x10);    // PUSH
        emit_i32(code, message[i]); // value
        emit_opcode(code, 0x72);    // ASTORE
    }

    // Write to the file

    emit_opcode(code, 0x10);        // PUSH
    emit_i32(code, 1);              // local index of buffer
    emit_opcode(code, 0x10);        // PUSH
    emit_i32(code, message.size()); // size
    emit_opcode(code, 0x20);        // LOAD
    emit_i32(code, 3);              // fd from local index 3
    emit_opcode(code, 0x60);        // SYS_CALL
    emit_opcode(code, 0x07);        // WRITE

    // 3. Close the file
    emit_opcode(code, 0x20); // LOAD
    emit_i32(code, 3);       // fd from local index 3
    emit_opcode(code, 0x60); // SYS_CALL
    emit_opcode(code, 0x04); // CLOSE

    // 4. Re-open the file for reading
    emit_opcode(code, 0x10); // PUSH
    emit_i32(code, 0);       // filename index on heap
    emit_opcode(code, 0x10); // PUSH
    emit_i32(code, 'r');     // mode
    emit_opcode(code, 0x60); // SYS_CALL
    emit_opcode(code, 0x01); // OPEN
    emit_opcode(code, 0x21); // STORE
    emit_i32(code, 3);       // local index 3 for fd

    // 5. Read the contents of the file
    // Read from the file

    emit_opcode(code, 0x10);        // PUSH
    emit_i32(code, 2);              // local index to store array ref
    emit_opcode(code, 0x10);        // PUSH
    emit_i32(code, message.size()); // size
    emit_opcode(code, 0x20);        // LOAD
    emit_i32(code, 3);              // fd from local index 3
    emit_opcode(code, 0x60);        // SYS_CALL
    emit_opcode(code, 0x02);        // READ

    // 6. Write the contents to standard output

    emit_opcode(code, 0x10);        // PUSH
    emit_i32(code, 2);              // local index of buffer
    emit_opcode(code, 0x10);        // PUSH
    emit_i32(code, message.size()); // size
    emit_opcode(code, 0x10);        // PUSH
    emit_i32(code, 1);              // fd (stdout)
    emit_opcode(code, 0x60);        // SYS_CALL
    emit_opcode(code, 0x07);        // WRITE

    // Close the file
    emit_opcode(code, 0x20); // LOAD
    emit_i32(code, 3);       // fd from local index 3
    emit_opcode(code, 0x60); // SYS_CALL
    emit_opcode(code, 0x04); // CLOSE

    // 7. Exit the VM
    emit_opcode(code, 0x10); // PUSH
    emit_i32(code, 0);       // exit code
    emit_opcode(code, 0x60); // SYS_CALL
    emit_opcode(code, 0x0A); // EXIT

    // Update CodeSize in the header
    int32_t code_size = code.size();
    bytecode[24] = code_size & 0xFF;
    bytecode[25] = (code_size >> 8) & 0xFF;
    bytecode[26] = (code_size >> 16) & 0xFF;
    bytecode[27] = (code_size >> 24) & 0xFF;

    // Append code to bytecode
    bytecode.insert(bytecode.end(), code.begin(), code.end());

    // Write to file
    ofstream out("test_syscall.vm", ios::binary);
    out.write((char *)bytecode.data(), bytecode.size());

    return 0;
}
