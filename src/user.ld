/* user.ld - linker script for VM user program */
ENTRY(_start)

/* === RAM Layout === */
MEMORY
{
    /* User space: 8MB starting at 8MB offset (matching your OS layout) */
    RAM (rw) : ORIGIN = 0x20800000, LENGTH = 0x800000  /* 8 MB user RAM */
}

/* Kernel symbols expected by syscalls.c */
PROVIDE(_kernel_end = 0x20000000 + 0x800000);  /* 8 MB kernel */
PROVIDE(_estack     = 0x20000000 + 0x1000000); /* top of total RAM */

SECTIONS
{
    /* === Text + RO Data === */
    .text :
    {
        _text_start = .;
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > RAM

    /*
     * ADD THIS SECTION
     * This is the exception index table used for stack unwinding.
     * The linker will define __exidx_start and __exidx_end automatically.
     */
    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > RAM

    /* === Data Section === */
    .data :
    {
        _data_start = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    /* === BSS Section === */
    .bss (NOLOAD) :
    {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > RAM
    /* After your .bss section */
    PROVIDE(__bss_start__ = _bss_start);
    PROVIDE(__bss_end__   = _bss_end);

    /* === Heap Section === */
    .heap (NOLOAD) :
    {
        _heap_start = .;                      /* Start after BSS */
        . = . + 0x2000;                       /* 8 KB heap */
        _heap_end = .;
    } > RAM

    /* === Stack Section === */
    .stack (NOLOAD) :
    {
        /* Reserve 1MB at the end of user space for stack */
        _stack_bottom = ORIGIN(RAM) + LENGTH(RAM) - 0x100000;  /* 1 MB from end */
        _stack_top = ORIGIN(RAM) + LENGTH(RAM);                /* Top of user RAM */
    } > RAM
}