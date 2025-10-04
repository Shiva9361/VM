/* src/libuser/user_startup.S */
.syntax unified
.cpu cortex-m0
.thumb

.global _start
.global main

.section .vectors
/* The user application's linker script must define _estack */
.word _estack
.word _start + 1 // Thumb mode

.section .text
.thumb_func
_start:
    // This is the entry point for the user program
    bl main

    // If main ever returns, formally exit the program
    // by calling the exit system call (SVC #0)
exit_loop:
    movs r0, #0 // return code 0
    svc #0      // Call the OS's exit service
    b exit_loop // Should never be reached