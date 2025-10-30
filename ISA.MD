# Stack-Based Virtual Machine: ISA

## 1. Overview of the Stack-Based VM Architecture

This document outlines the Instruction Set Architecture (ISA) for a simple, stack-based virtual machine. In this computational model, all arithmetic and logical operations are performed on values temporarily stored on an operand stack. For instance, to add two numbers, both are pushed onto the stack, an `ADD` instruction is executed which consumes them, and the resulting sum is pushed back onto the stack.

This design simplifies the virtual machine's implementation by avoiding the need for a complex, named register file for operations.

#### Key Components:

- **Operand Stack:** A Last-In, First-Out (LIFO) data structure that holds operands and results for calculations.
- **Instruction Pointer (IP):** A register that points to the next VM bytecode instruction to be executed.
- **Local Variable Store:** An array-like structure for storing local variables within the scope of a function.
- **Constant Pool:** A storage area for constants (e.g., numbers, strings) used by the program.

---

## 2. Instruction Set Architecture (ISA)

The ISA is designed for simplicity, with most instructions represented by a single-byte opcode. Some instructions are followed by one or more bytes that act as arguments, such as an index into the local variable store or a jump address.

#### 2.1. Arithmetic Operations

These instructions pop operands from the stack, perform a calculation, and push the result back onto the stack.

| Opcode | Mnemonic | Description            | Stack Transition                   |
| :----- | :------- | :--------------------- | :--------------------------------- |
| `0x01` | `IADD`   | Integer Addition       | `..., val1, val2` -> `..., result` |
| `0x02` | `ISUB`   | Integer Subtraction    | `..., val1, val2` -> `..., result` |
| `0x03` | `IMUL`   | Integer Multiplication | `..., val1, val2` -> `..., result` |
| `0x04` | `IDIV`   | Integer Division       | `..., val1, val2` -> `..., result` |
| `0x05` | `INEG`   | Integer Negation       | `..., val` -> `..., -val`          |

#### 2.2. Stack Manipulation

| Opcode | Mnemonic     | Description                           | Stack Transition              |
| :----- | :----------- | :------------------------------------ | :---------------------------- |
| `0x10` | `PUSH <val>` | Push a constant value onto the stack. | `...` -> `..., val`           |
| `0x11` | `POP`        | Discard the top value of the stack.   | `..., val` -> `...`           |
| `0x12` | `DUP`        | Duplicate the top value of the stack. | `..., val` -> `..., val, val` |

#### 2.3. Memory Operations (Local Variables)

These instructions facilitate data transfer between the operand stack and the local variable store.

| Opcode | Mnemonic      | Description                                                   | Stack Transition    |
| :----- | :------------ | :------------------------------------------------------------ | :------------------ |
| `0x20` | `LOAD <idx>`  | Load a local variable at index `idx` onto the stack.          | `...` -> `..., val` |
| `0x21` | `STORE <idx>` | Pop the top of the stack and store into local variable `idx`. | `..., val` -> `...` |

#### 2.4. Control Flow Operations

These instructions alter the sequential flow of execution by modifying the Instruction Pointer (IP).

| Opcode | Mnemonic      | Description                                     | Stack Transition    |
| :----- | :------------ | :---------------------------------------------- | :------------------ |
| `0x30` | `JMP <addr>`  | Unconditionally jump to address `addr`.         | No change           |
| `0x31` | `JZ <addr>`   | Pop value; jump to `addr` if value is zero.     | `..., val` -> `...` |
| `0x32` | `JNZ <addr>`  | Pop value; jump to `addr` if value is non-zero. | `..., val` -> `...` |
| `0x33` | `CALL <addr>` | Call a function at address `addr`.              | No change           |
| `0x34` | `RET`         | Return from the current function.               | No change           |

#### 2.5. Comparison Operations

These instructions compare the top two values on the stack and push a boolean result (1 for true, 0 for false).

| Opcode | Mnemonic  | Description          | Stack Transition                          |
| :----- | :-------- | :------------------- | :---------------------------------------- |
| `0x40` | `ICMP_EQ` | Integer Equal        | `..., val1, val2` -> `..., 1` or `..., 0` |
| `0x41` | `ICMP_LT` | Integer Less Than    | `..., val1, val2` -> `..., 1` or `..., 0` |
| `0x42` | `ICMP_GT` | Integer Greater Than | `..., val1, val2` -> `..., 1` or `..., 0` |

#### 2.6. Object-Oriented Operations

These instructions provide the foundation for object-oriented programming, managing object creation, field access, and method calls. Arguments (`<..._ref>`) are typically indices into the constant pool.

| Opcode | Mnemonic                   | Description                             | Stack Transition                              |
| ------ | -------------------------- | --------------------------------------- | --------------------------------------------- |
| 0x50   | `NEW <class_ref>`          | Create a new object of a class.         | `... -> ..., obj_ref`                         |
| 0x51   | `GETFIELD <field_ref>`     | Get a field from an object.             | `..., obj_ref -> ..., value`                  |
| 0x52   | `PUTFIELD <field_ref>`     | Set a field in an object.               | `..., obj_ref, value -> ...`                  |
| 0x53   | `INVOKEVIRTUAL <meth_ref>` | Invoke an instance (virtual) method.    | `..., obj_ref, [arg1, ...] -> ..., [ret_val]` |
| 0x54   | `INVOKESPECIAL <meth_ref>` | Invoke a constructor or private method. | `..., obj_ref, [arg1, ...] -> ...`            |

#### 2.7. Syscall Operations

These instructions facilitate interaction with the host environment, allowing the VM to perform I/O operations and other system-level tasks.

The opcode for syscall is 0x60, followed by a syscall number that specifies the operation to be performed.

| Syscall | Mnemonic | Description                                               | Stack Transition                                           |
| :------ | :------- | :-------------------------------------------------------- | :--------------------------------------------------------- |
| 0x01    | OPEN     | Open a file.                                              | `..., filename, mode` -> `..., file_handle`                |
| 0x02    | READ     | Read from a file. Put a pointer to the buffer at localidx | `..., localidx, size, file_handle` -> `..., bytes_read`    |
| 0x07    | WRITE    | Write to a file. Use the data from buffer at localidx     | `..., localidx, size, file_handle` -> `..., bytes_written` |

#### 2.8. Array Operations

These instructions manage array creation and element access.
| Opcode | Mnemonic | Description | Stack Transition |
| :----- | :-------------- | :-------------------------------------- | :----------------------------------- |
| `0x70` | `NEWARRAY <type>` | Create a new array of a specified type and store ref at localidx | `..., localidx, size` -> `...` |
| `0x71` | `ALOAD ` | Load an array element onto the stack | `..., array_ref, index` -> `..., value` |
| `0x72` | `ASTORE ` | Store a value into an array element | `..., array_ref, index, value` -> `...` |
