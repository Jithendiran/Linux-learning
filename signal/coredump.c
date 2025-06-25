#include <stdlib.h>

int main()
{
    int *p = NULL;
    *p = 42; // causes SIGSEGV
}
// gcc -g coredump.c -o coredump.out && ./coredump.out

// gdb coredump.out /tmp/core.coredump.out.5287.1750825745