/* src/user.ld - Linker script for user-space applications */
ENTRY(_start)

MEMORY { RAM (rwx) : ORIGIN = 0x20008000, LENGTH = 32K }

/* The user_startup.S file provides this symbol */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    .text : {
        KEEP(*(.vectors)) /* Keep the vector table from user_startup.S */
        *(.text*)
        *(.rodata*)
    } > RAM

    .data : { *(.data*) } > RAM
    .bss : { *(.bss*) } > RAM
}