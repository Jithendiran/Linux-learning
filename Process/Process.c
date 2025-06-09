#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    int x = 42;
    int y = 12;
    int fd, flags;
    char template[] = "/tmp/testXXXXXX";

    // file sharing
    fd = mkstemp(template);
    if (fd == -1)
        perror("mkstemp");

    printf("File offset before fork(): %lld\n",
           (long long)lseek(fd, 0, SEEK_CUR));

    flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        perror("fcntl - F_GETFL");

    printf("O_APPEND flag before fork() is: %s\n",
           (flags & O_APPEND) ? "on" : "off");
    // file sharing

    pid_t pid = fork();

    if (pid == 0)
    {
        // Child
        printf("Child sees x = %d\n", x); // 42
        sleep(1);
        printf("Child sees x again = %d\n", x); // Still 42
        y = 1;
        printf("Child sees y = %d\n", y); // 1
        // new resource created for y, no new resource created for x

        // file sharing
        if (lseek(fd, 1000, SEEK_SET) == -1)
            perror("lseek");

        flags = fcntl(fd, F_GETFL);
        if (flags == -1)
            perror("fcntl - F_GETFL");

        flags |= O_APPEND;
        /* Turn O_APPEND on */
        if (fcntl(fd, F_SETFL, flags) == -1)
            perror("fcntl - F_SETFL");

        // file sharing
        _exit(EXIT_SUCCESS);
    }
    else
    {
        // Parent
        x = 99;                                  // Parent writes → triggers COW
        printf("Parent changed x to = %d\n", x); // 99
        printf("Parent changed y to = %d\n", y); // 12
        wait(NULL);
        // new resource created for x, no new resource created for y

        // file sharing
        printf("Child has exited\n");
        printf("File offset in parent: %lld\n",
               (long long)lseek(fd, 0, SEEK_CUR));

        flags = fcntl(fd, F_GETFL);
        if (flags == -1)
            perror("fcntl - F_GETFL");

        printf("O_APPEND flag in parent is: %s\n",
               (flags & O_APPEND) ? "on" : "off");
        // file sharing
        exit(EXIT_SUCCESS);
    }

    return 0;
}
// gcc Process.c -o Process.o && strace ./Process.o
/*
File offset before fork(): 0
O_APPEND flag before fork() is: off
Parent changed x to = 99
Parent changed y to = 12
Child sees x = 42
Child sees x again = 42
Child sees y = 1
Child has exited
File offset in parent: 1000
O_APPEND flag in parent is: on
*/


// execve("./Process.o", ["./Process.o"], 0x7ffe37a131a0 /* 56 vars */) = 0
// brk(NULL)                               = 0x5f2b4fbaf000
// arch_prctl(0x3001 /* ARCH_??? */, 0x7fffaac1b170) = -1 EINVAL (Invalid argument)
// mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f011ede0000
// access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
// openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
// newfstatat(3, "", {st_mode=S_IFREG|0644, st_size=86200, ...}, AT_EMPTY_PATH) = 0
// mmap(NULL, 86200, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7f011edca000
// close(3)                                = 0
// openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
// read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0P\237\2\0\0\0\0\0"..., 832) = 832
// pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
// pread64(3, "\4\0\0\0 \0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0"..., 48, 848) = 48
// pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0\325\31p\226\367\t\200\30)\261\30\257\33|\366c"..., 68, 896) = 68
// newfstatat(3, "", {st_mode=S_IFREG|0755, st_size=2220400, ...}, AT_EMPTY_PATH) = 0
// pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
// mmap(NULL, 2264656, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f011ea00000
// mprotect(0x7f011ea28000, 2023424, PROT_NONE) = 0
// mmap(0x7f011ea28000, 1658880, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x7f011ea28000
// mmap(0x7f011ebbd000, 360448, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1bd000) = 0x7f011ebbd000
// mmap(0x7f011ec16000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x215000) = 0x7f011ec16000
// mmap(0x7f011ec1c000, 52816, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7f011ec1c000
// close(3)                                = 0
// mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f011edc7000
// arch_prctl(ARCH_SET_FS, 0x7f011edc7740) = 0
// set_tid_address(0x7f011edc7a10)         = 11163
// set_robust_list(0x7f011edc7a20, 24)     = 0
// rseq(0x7f011edc80e0, 0x20, 0, 0x53053053) = 0
// mprotect(0x7f011ec16000, 16384, PROT_READ) = 0
// mprotect(0x5f2b2137f000, 4096, PROT_READ) = 0
// mprotect(0x7f011ee1a000, 8192, PROT_READ) = 0
// prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
// munmap(0x7f011edca000, 86200)           = 0
// getrandom("\xf1\x24\x13\x05\x9e\x52\x56\x97", 8, GRND_NONBLOCK) = 8
// openat(AT_FDCWD, "/tmp/test5GyvSE", O_RDWR|O_CREAT|O_EXCL, 0600) = 3
// lseek(3, 0, SEEK_CUR)                   = 0
// newfstatat(1, "", {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x1), ...}, AT_EMPTY_PATH) = 0
// getrandom("\x7f\xaa\x86\xe9\x4f\x59\x76\x12", 8, GRND_NONBLOCK) = 8
// brk(NULL)                               = 0x5f2b4fbaf000
// brk(0x5f2b4fbd0000)                     = 0x5f2b4fbd0000
// write(1, "File offset before fork(): 0\n", 29File offset before fork(): 0
// ) = 29
// fcntl(3, F_GETFL)                       = 0x8002 (flags O_RDWR|O_LARGEFILE)
// write(1, "O_APPEND flag before fork() is: "..., 36O_APPEND flag before fork() is: off
// ) = 36
// clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7f011edc7a10) = 11164
// write(1, "Parent changed x to = 99\n", 25Parent changed x to = 99
// ) = 25
// write(1, "Parent changed y to = 12\n", 25Parent changed y to = 12
// ) = 25
// wait4(-1, Child sees x = 42
// Child sees x again = 42
// Child sees y = 1
// NULL, 0, NULL)                = 11164
// --- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=11164, si_uid=1000, si_status=0, si_utime=0, si_stime=0} ---
// write(1, "Child has exited\n", 17Child has exited
// )      = 17
// lseek(3, 0, SEEK_CUR)                   = 1000
// write(1, "File offset in parent: 1000\n", 28File offset in parent: 1000
// ) = 28
// fcntl(3, F_GETFL)                       = 0x8402 (flags O_RDWR|O_APPEND|O_LARGEFILE)
// write(1, "O_APPEND flag in parent is: on\n", 31O_APPEND flag in parent is: on
// ) = 31
// exit_group(0)                           = ?
// +++ exited with 0 +++
