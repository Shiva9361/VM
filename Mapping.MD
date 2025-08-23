# VM ISA → ARM Mapping Reference

This table maps each VM instruction to its ARM equivalent.

- **Operand Stack** pointer: `r10`
- **Local Variable Base**: `r9`
- **Temporaries**: `r0–r3`
- **IP (Instruction Pointer)** handled in interpreter loop (C++).

---

## 1. Arithmetic Operations

| VM Opcode | Mnemonic | VM Semantics                         | ARM Equivalent                                                                                     |
|-----------|----------|--------------------------------------|----------------------------------------------------------------------------------------------------|
| 0x01      | IADD     | Pop val1, val2 → push (val1 + val2)  | `LDR r0, [r10], #4`  <br> `LDR r1, [r10], #4`  <br> `ADD r0, r1, r0`  <br> `STR r0, [r10, #-4]!`   |
| 0x02      | ISUB     | Pop val1, val2 → push (val1 - val2)  | `LDR r0, [r10], #4`  <br> `LDR r1, [r10], #4`  <br> `SUB r0, r1, r0`  <br> `STR r0, [r10, #-4]!`   |
| 0x03      | IMUL     | Pop val1, val2 → push (val1 * val2)  | `LDR r0, [r10], #4`  <br> `LDR r1, [r10], #4`  <br> `MUL r0, r1, r0`  <br> `STR r0, [r10, #-4]!`   |
| 0x04      | IDIV     | Pop val1, val2 → push (val1 / val2)  | `LDR r0, [r10], #4`  <br> `LDR r1, [r10], #4`  <br> `SDIV r0, r1, r0`  <br> `STR r0, [r10, #-4]!`  |
| 0x05      | INEG     | Pop val → push (-val)                | `LDR r0, [r10], #4`  <br> `RSB r0, r0, #0`  <br> `STR r0, [r10, #-4]!`                             |

---

## 2. Stack Manipulation

| VM Opcode | Mnemonic     | VM Semantics             | ARM Equivalent                              |
| --------- | ------------ | ------------------------ | --------------------------------------------|
| `0x10`    | `PUSH <val>` | Push constant onto stack | `LDR r0, =<val>` <br> `STR r0, [r10, #-4]!` |
| `0x11`    | `POP`        | Discard top of stack     | `LDR r0, [r10], #4`                         |
| `0x12`    | `DUP`        | Duplicate top of stack   | `LDR r0, [r10]` <br> `STR r0, [r10, #-4]!`  |

---

## 3. Memory Operations (Locals)

| VM Opcode | Mnemonic      | VM Semantics     | ARM Equivalent                                    |
| --------- | ------------- | ---------------- | ------------------------------------------------- |
| `0x20`    | `LOAD <idx>`  | Push local[idx]  | `LDR r0, [r9, #idx*4]` <br> `STR r0, [r10, #-4]!` |
| `0x21`    | `STORE <idx>` | Pop → local[idx] | `LDR r0, [r10], #4` <br> `STR r0, [r9, #idx*4]`   |

---

## 4. Control Flow

| VM Opcode | Mnemonic      | VM Semantics              | ARM Equivalent                                        |
| --------- | ------------- | ------------------------- | ----------------------------------------------------- |
| `0x30`    | `JMP <addr>`  | Jump to addr              | `B <addr>`                                            |
| `0x31`    | `JZ <addr>`   | Pop val; if zero, jump    | `LDR r0, [r10], #4` <br>  `CMP r0, #0<br>BEQ <addr>`  |
| `0x32`    | `JNZ <addr>`  | Pop val; if nonzero, jump | `LDR r0, [r10], #4<br>CMP r0, #0<br>BNE <addr>`       |
| `0x33`    | `CALL <addr>` | Call function             | `BL <addr>`                                           |
| `0x34`    | `RET`         | Return from function      | `BX lr`                                               |

---

## 5. Comparison

| VM Opcode | Mnemonic  | VM Semantics        | ARM Equivalent                                                                                                |
| --------- | --------- | ------------------- | ------------------------------------------------------------------------------------------------------------- |
| `0x40`    | `ICMP_EQ` | Push (val1 == val2) | `LDR r0, [r10], #4` <br> `LDR r1, [r10], #4` <br> `CMP r1, r0<br>MOVEQ r0, #1` <br> `MOVNE r0, #0<br>STR r0, [r10, #-4]!` |
| `0x41`    | `ICMP_LT` | Push (val1 < val2)  | `CMP r1, r0` <br> `MOVLT r0, #1 `<br> `MOVGE r0, #0` <br>`STR r0, [r10, #-4]!`                                           |
| `0x42`    | `ICMP_GT` | Push (val1 > val2)  | `CMP r1, r0` <br> `MOVGT r0, #1` <br> `MOVLE r0, #0`<br>`STR r0, [r10, #-4]!`                                           |

---

## 6. Object-Oriented Ops (via C++ Helpers)

Instead of inline ARM, these invoke **VM runtime helpers** (compiled by GCC).

| VM Opcode | Mnemonic                   | VM Semantics                 | ARM Equivalent (C++ Call)                          |
| --------- | -------------------------- | ---------------------------- | -------------------------------------------------- |
| `0x50`    | `NEW <class_ref>`          | Allocate object              | `BL vm_new  ; r0=class_ref`                        |
| `0x51`    | `GETFIELD <field_ref>`     | Pop obj_ref → push obj.field | `BL vm_getfield  ; args=(obj_ref, field_ref)`      |
| `0x52`    | `PUTFIELD <field_ref>`     | Pop (obj_ref, val) → store   | `BL vm_putfield  ; args=(obj_ref, field_ref, val)` |
| `0x53`    | `INVOKEVIRTUAL <meth_ref>` | Virtual method call          | `BL vm_invokevirtual`                              |
| `0x54`    | `INVOKESPECIAL <meth_ref>` | Constructor/private call     | `BL vm_invokespecial`                              |

---

### Notes

- Helpers (`vm_new`, `vm_getfield`, etc.) are normal C++ functions managing heap layout, vtables, and GC.
- The interpreter loop dispatches these opcodes and emits the snippets above.
- Optimizing JIT could inline simple patterns later, but helpers keep it simple now.
