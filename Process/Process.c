#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int x = 42;
    int y = 12;
    pid_t pid = fork();

    if (pid == 0) {
        // Child
        printf("Child sees x = %d\n", x);  // 42
        sleep(1);
        printf("Child sees x again = %d\n", x);  // Still 42
        y = 1;
        printf("Child sees y = %d\n", y);  // 1
        // new resource created for y, no new resource created for x
    } else {
        // Parent
        x = 99;  // Parent writes → triggers COW
        printf("Parent changed x to = %d\n", x);  // 99
        printf("Parent changed y to = %d\n", y);  // 12
        wait(NULL);
        // new resource created for x, no new resource created for y
    }

    return 0;
}
// gcc Process.c -o Process.o && strace ./Process.o


// ./Process.o
// execve("./Process.o", ["./Process.o"], 0x7fffe5a7e180 /* 57 vars */) = 0
// brk(NULL)                               = 0x5fbbca26d000
// arch_prctl(0x3001 /* ARCH_??? */, 0x7ffde1012000) = -1 EINVAL (Invalid argument)
// mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x70adf19cc000
// access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
// openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
// newfstatat(3, "", {st_mode=S_IFREG|0644, st_size=86200, ...}, AT_EMPTY_PATH) = 0
// mmap(NULL, 86200, PROT_READ, MAP_PRIVATE, 3, 0) = 0x70adf19b6000
// close(3)                                = 0
// openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
// read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0P\237\2\0\0\0\0\0"..., 832) = 832
// pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
// pread64(3, "\4\0\0\0 \0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0"..., 48, 848) = 48
// pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0\325\31p\226\367\t\200\30)\261\30\257\33|\366c"..., 68, 896) = 68
// newfstatat(3, "", {st_mode=S_IFREG|0755, st_size=2220400, ...}, AT_EMPTY_PATH) = 0
// pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
// mmap(NULL, 2264656, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x70adf1600000
// mprotect(0x70adf1628000, 2023424, PROT_NONE) = 0
// mmap(0x70adf1628000, 1658880, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x70adf1628000
// mmap(0x70adf17bd000, 360448, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1bd000) = 0x70adf17bd000
// mmap(0x70adf1816000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x215000) = 0x70adf1816000
// mmap(0x70adf181c000, 52816, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x70adf181c000
// close(3)                                = 0
// mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x70adf19b3000
// arch_prctl(ARCH_SET_FS, 0x70adf19b3740) = 0
// set_tid_address(0x70adf19b3a10)         = 5902
// set_robust_list(0x70adf19b3a20, 24)     = 0
// rseq(0x70adf19b40e0, 0x20, 0, 0x53053053) = 0
// mprotect(0x70adf1816000, 16384, PROT_READ) = 0
// mprotect(0x5fbb8eee9000, 4096, PROT_READ) = 0
// mprotect(0x70adf1a06000, 8192, PROT_READ) = 0
// prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
// munmap(0x70adf19b6000, 86200)           = 0
// clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x70adf19b3a10) = 5903       // -> this is the fork
// newfstatat(1, "", {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0), ...}, AT_EMPTY_PATH) = 0
// Child sees x = 42
// getrandom("\x26\x10\x7b\xce\x35\x1c\x8c\x98", 8, GRND_NONBLOCK) = 8
// brk(NULL)                               = 0x5fbbca26d000
// brk(0x5fbbca28e000)                     = 0x5fbbca28e000
// write(1, "Parent changed x to = 99\n", 25Parent changed x to = 99
// ) = 25
// write(1, "Parent changed y to = 12\n", 25Parent changed y to = 12
// ) = 25
// wait4(-1, Child sees x again = 42
// Child sees y = 1
// NULL, 0, NULL)                = 5903
// --- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=5903, si_uid=1000, si_status=0, si_utime=0, si_stime=0} ---
// exit_group(0)                           = ?
// +++ exited with 0 +++
