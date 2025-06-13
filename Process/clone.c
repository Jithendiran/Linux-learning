#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

// Function that the child process will execute
int child_function(void *arg) {
    printf("Child process: Hello from child!\n");
    return 0;
}

int main() {
    // Allocate stack for child process
    void *child_stack = malloc(1024 * 1024); // 1MB stack
    if (child_stack == NULL) {
        perror("malloc");
        return 1;
    }

    // Create the child process using clone()
    // this is not a syscall, the below clone is libc function. Internally it calls clone syscall
    int child_pid = clone(child_function, (char *)child_stack + 1024 * 1024,
                          CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND, NULL);

    if (child_pid == -1) {
        perror("clone");
        free(child_stack);
        return 1;
    }

    printf("Parent process: Child process created with PID: %d\n", child_pid);

    // Wait for the child process to finish
    wait(NULL);

    printf("Parent process: Child process finished.\n");

    free(child_stack);
    return 0;
}


//  gcc -g clone.c -o clone.o
// strace ./clone.o 
// execve("./clone.o", ["./clone.o"], 0x7ffe036575d0 /* 59 vars */) = 0
// brk(NULL)                               = 0x5eaa81a64000
// arch_prctl(0x3001 /* ARCH_??? */, 0x7ffe536feba0) = -1 EINVAL (Invalid argument)
// mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7c9e954e9000
// access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
// openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
// newfstatat(3, "", {st_mode=S_IFREG|0644, st_size=86200, ...}, AT_EMPTY_PATH) = 0
// mmap(NULL, 86200, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7c9e954d3000
// close(3)                                = 0
// openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
// read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0P\237\2\0\0\0\0\0"..., 832) = 832
// pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
// pread64(3, "\4\0\0\0 \0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0"..., 48, 848) = 48
// pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0\325\31p\226\367\t\200\30)\261\30\257\33|\366c"..., 68, 896) = 68
// newfstatat(3, "", {st_mode=S_IFREG|0755, st_size=2220400, ...}, AT_EMPTY_PATH) = 0
// pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
// mmap(NULL, 2264656, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7c9e95200000
// mprotect(0x7c9e95228000, 2023424, PROT_NONE) = 0
// mmap(0x7c9e95228000, 1658880, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x7c9e95228000
// mmap(0x7c9e953bd000, 360448, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1bd000) = 0x7c9e953bd000
// mmap(0x7c9e95416000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x215000) = 0x7c9e95416000
// mmap(0x7c9e9541c000, 52816, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7c9e9541c000
// close(3)                                = 0
// mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7c9e954d0000
// arch_prctl(ARCH_SET_FS, 0x7c9e954d0740) = 0
// set_tid_address(0x7c9e954d0a10)         = 3580
// set_robust_list(0x7c9e954d0a20, 24)     = 0
// rseq(0x7c9e954d10e0, 0x20, 0, 0x53053053) = 0
// mprotect(0x7c9e95416000, 16384, PROT_READ) = 0
// mprotect(0x5eaa78cb3000, 4096, PROT_READ) = 0
// mprotect(0x7c9e95523000, 8192, PROT_READ) = 0
// prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
// munmap(0x7c9e954d3000, 86200)           = 0
// getrandom("\xc8\x58\xaf\xa2\xd3\x64\x95\x87", 8, GRND_NONBLOCK) = 8
// brk(NULL)                               = 0x5eaa81a64000
// brk(0x5eaa81a85000)                     = 0x5eaa81a85000
// mmap(NULL, 1052672, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7c9e950ff000
// clone(child_stack=0x7c9e951ff000, flags=CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND) = 3581
// newfstatat(1, "", Child process: Hello from child!
// {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x2), ...}, AT_EMPTY_PATH) = 0
// write(1, "Parent process: Child process cr"..., 53Parent process: Child process created with PID: 3581
// ) = 53
// wait4(-1, NULL, 0, NULL)                = -1 ECHILD (No child processes)
// write(1, "Parent process: Child process fi"..., 40Parent process: Child process finished.
// ) = 40
// munmap(0x7c9e950ff000, 1052672)         = 0
// exit_group(0)                           = ?
// +++ exited with 0 +++