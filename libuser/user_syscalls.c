int _write(int file, char *ptr, int len)
{
    int ret;
    __asm__ volatile(
        "mov r0, %1\n"
        "mov r1, %2\n"
        "mov r2, %3\n"
        "svc #1\n"
        "mov %0, r0\n"
        : "=r"(ret)
        : "r"(file), "r"(ptr), "r"(len)
        : "r0", "r1", "r2");
    return ret;
}

int _read(int file, char *ptr, int len)
{
    int ret;
    __asm__ volatile(
        "mov r0, %1\n"
        "mov r1, %2\n"
        "mov r2, %3\n"
        "svc #2\n"
        "mov %0, r0\n"
        : "=r"(ret)
        : "r"(file), "r"(ptr), "r"(len)
        : "r0", "r1", "r2");
    return ret;
}

void *_sbrk(int incr)
{
    void *ret;
    __asm__ volatile(
        "mov r0, %1\n"
        "svc #3\n"
        "mov %0, r0\n"
        : "=r"(ret)
        : "r"(incr)
        : "r0");
    return ret;
}

int _close(int file)
{
    int ret;
    __asm__ volatile(
        "mov r0, %1\n"
        "svc #4\n"
        "mov %0, r0\n"
        : "=r"(ret)
        : "r"(file)
        : "r0");
    return ret;
}

int _fstat(int file, void *st)
{
    int ret;
    __asm__ volatile(
        "mov r0, %1\n"
        "mov r1, %2\n"
        "svc #5\n"
        "mov %0, r0\n"
        : "=r"(ret)
        : "r"(file), "r"(st)
        : "r0", "r1");
    return ret;
}

int _isatty(int file)
{
    int ret;
    __asm__ volatile(
        "mov r0, %1\n"
        "svc #6\n"
        "mov %0, r0\n"
        : "=r"(ret)
        : "r"(file)
        : "r0");
    return ret;
}

int _lseek(int file, int ptr, int dir)
{
    int ret;
    __asm__ volatile(
        "mov r0, %1\n"
        "mov r1, %2\n"
        "mov r2, %3\n"
        "svc #7\n"
        "mov %0, r0\n"
        : "=r"(ret)
        : "r"(file), "r"(ptr), "r"(dir)
        : "r0", "r1", "r2");
    return ret;
}

void _exit(int code)
{
    __asm__ volatile("mov r0, %0; svc #0" : : "r"(code));
    while (1)
        ;
}

int _kill(int pid, int sig)
{
    int ret;
    __asm__ volatile(
        "mov r0, %1\n"
        "mov r1, %2\n"
        "svc #8\n"
        "mov %0, r0\n"
        : "=r"(ret)
        : "r"(pid), "r"(sig)
        : "r0", "r1");
    return ret;
}

int _getpid(void)
{
    int ret;
    __asm__ volatile(
        "svc #9\n"
        "mov %0, r0\n"
        : "=r"(ret)
        :
        : "r0");
    return ret;
}

int _open(const char *name, int flags, int mode)
{
    int ret;
    __asm__ volatile(
        "mov r0, %1\n"
        "mov r1, %2\n"
        "mov r2, %3\n"
        "svc #10\n"
        "mov %0, r0\n"
        : "=r"(ret)
        : "r"(name), "r"(flags), "r"(mode)
        : "r0", "r1", "r2");
    return ret;
}