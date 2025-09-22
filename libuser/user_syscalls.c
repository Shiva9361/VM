// src/libuser/user_syscalls.c

// These are the user-space functions that applications will call.
// Their only job is to trigger a Supervisor Call (SVC) exception,
// which transfers control to the OS kernel.

void _exit(int code)
{
    // SVC #0 is EXIT
    // Move the return code into r0 and execute the svc instruction.
    __asm__ volatile("mov r0, %0; svc #0" : : "r"(code));
    // This function never returns.
    while (1)
        ;
}

// Minimal stubs to satisfy the linker for now.
// We will make these real SVC calls later.
int _write(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        char buf[2] = {ptr[i], '\0'};
        register int r0 __asm__("r0") = 0x05; // SH_WRITE
        register void *r1 __asm__("r1") = buf;
        __asm__ volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
    }
    return 0;
}
void *_sbrk(int incr) { return (void *)-1; }
int _close(int file) { return -1; }
int _open(const char *name, int flags, int mode)
{
    (void)name;
    (void)flags;
    (void)mode;
    // errno = ENOSYS;
    return -1;
}
int _fstat(int file, void *st) { return 0; }
int _isatty(int file) { return 1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
int _kill(int pid, int sig) { return -1; }
int _getpid(void) { return 1; }