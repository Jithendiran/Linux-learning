#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main()
{
    size_t length = 4096;
    pid_t pid = getpid();
    char cmdmaps[120], cmdstatus[120];
    sprintf(cmdmaps, "cat /proc/%d/maps", pid);
    sprintf(cmdstatus, "cat /proc/%d/status | grep Vm", pid);

    // Before mmap
    system(cmdmaps);
    /**
    642d0f7fc000-642d0f7fd000 r--p 00000000 103:02 262183                    /tmp/mmap
    642d0f7fd000-642d0f7fe000 r-xp 00001000 103:02 262183                    /tmp/mmap
    642d0f7fe000-642d0f7ff000 r--p 00002000 103:02 262183                    /tmp/mmap
    642d0f7ff000-642d0f800000 r--p 00002000 103:02 262183                    /tmp/mmap
    642d0f800000-642d0f801000 rw-p 00003000 103:02 262183                    /tmp/mmap
    78dc2aa00000-78dc2aa28000 r--p 00000000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2aa28000-78dc2abbd000 r-xp 00028000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2abbd000-78dc2ac15000 r--p 001bd000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac15000-78dc2ac16000 ---p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac16000-78dc2ac1a000 r--p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac1a000-78dc2ac1c000 rw-p 00219000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac1c000-78dc2ac29000 rw-p 00000000 00:00 0
    78dc2acea000-78dc2aced000 rw-p 00000000 00:00 0
    78dc2ad03000-78dc2ad05000 rw-p 00000000 00:00 0
    78dc2ad05000-78dc2ad07000 r--p 00000000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad07000-78dc2ad31000 r-xp 00002000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad31000-78dc2ad3c000 r--p 0002c000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad3d000-78dc2ad3f000 r--p 00037000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad3f000-78dc2ad41000 rw-p 00039000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    7ffdb4dcf000-7ffdb4df0000 rw-p 00000000 00:00 0                          [stack]
    7ffdb4df6000-7ffdb4dfa000 r--p 00000000 00:00 0                          [vvar]
    7ffdb4dfa000-7ffdb4dfc000 r-xp 00000000 00:00 0                          [vdso]
    ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
    */
    system(cmdstatus);
    /*
    VmPeak:     2732 kB
    VmSize:     2644 kB
    VmLck:         0 kB
    VmPin:         0 kB
    VmHWM:      1408 kB
    VmRSS:      1408 kB
    VmData:       92 kB
    VmStk:       132 kB
    VmExe:         4 kB
    VmLib:      1796 kB
    VmPTE:        44 kB
    VmSwap:        0 kB
   */

    // 1. Anonymous mapping (private)
    char *anon = mmap(NULL, length, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (anon == MAP_FAILED)
    {
        perror("mmap (anonymous)");
        return 1;
    }
    // After mmap
    system(cmdmaps);
    /*
    642d0f7fc000-642d0f7fd000 r--p 00000000 103:02 262183                    /tmp/mmap
    642d0f7fd000-642d0f7fe000 r-xp 00001000 103:02 262183                    /tmp/mmap
    642d0f7fe000-642d0f7ff000 r--p 00002000 103:02 262183                    /tmp/mmap
    642d0f7ff000-642d0f800000 r--p 00002000 103:02 262183                    /tmp/mmap
    642d0f800000-642d0f801000 rw-p 00003000 103:02 262183                    /tmp/mmap
    78dc2aa00000-78dc2aa28000 r--p 00000000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2aa28000-78dc2abbd000 r-xp 00028000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2abbd000-78dc2ac15000 r--p 001bd000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac15000-78dc2ac16000 ---p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac16000-78dc2ac1a000 r--p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac1a000-78dc2ac1c000 rw-p 00219000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac1c000-78dc2ac29000 rw-p 00000000 00:00 0
    78dc2acea000-78dc2aced000 rw-p 00000000 00:00 0
    78dc2ad03000-78dc2ad05000 rw-p 00000000 00:00 0
    78dc2ad05000-78dc2ad07000 r--p 00000000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad07000-78dc2ad31000 r-xp 00002000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad31000-78dc2ad3c000 r--p 0002c000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad3c000-78dc2ad3d000 rwxp 00000000 00:00 0
    78dc2ad3d000-78dc2ad3f000 r--p 00037000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad3f000-78dc2ad41000 rw-p 00039000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    7ffdb4dcf000-7ffdb4df0000 rw-p 00000000 00:00 0                          [stack]
    7ffdb4df6000-7ffdb4dfa000 r--p 00000000 00:00 0                          [vvar]
    7ffdb4dfa000-7ffdb4dfc000 r-xp 00000000 00:00 0                          [vdso]
    ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
    */
    /*
    The region 78dc2ad3c000-78dc2ad3d000 is a memory mapping from address 0x78dc2ad3c000 to 0x78dc2ad3d000.
     To find the size in bytes, subtract the start from the end:
     0x78dc2ad3d000 - 0x78dc2ad3c000 = 0x1000
     0x1000 in hexadecimal is 4096 bytes (4 KB).
    */
    system(cmdstatus);
    /*
    VmPeak:     2732 kB
    VmSize:     2648 kB
    VmLck:         0 kB
    VmPin:         0 kB
    VmHWM:      1408 kB
    VmRSS:      1408 kB
    VmData:       96 kB
    VmStk:       132 kB
    VmExe:         4 kB
    VmLib:      1796 kB
    VmPTE:        44 kB
    VmSwap:        0 kB
    */

    strcpy(anon, "Hello from anonymous mmap!");
    printf("Anonymous mapping: %s\n", anon); // Anonymous mapping: Hello from anonymous mmap!

    // After written
    system(cmdmaps);
    /*
    642d0f7fc000-642d0f7fd000 r--p 00000000 103:02 262183                    /tmp/mmap
    642d0f7fd000-642d0f7fe000 r-xp 00001000 103:02 262183                    /tmp/mmap
    642d0f7fe000-642d0f7ff000 r--p 00002000 103:02 262183                    /tmp/mmap
    642d0f7ff000-642d0f800000 r--p 00002000 103:02 262183                    /tmp/mmap
    642d0f800000-642d0f801000 rw-p 00003000 103:02 262183                    /tmp/mmap
    642d23bac000-642d23bcd000 rw-p 00000000 00:00 0                          [heap]
    78dc2aa00000-78dc2aa28000 r--p 00000000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2aa28000-78dc2abbd000 r-xp 00028000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2abbd000-78dc2ac15000 r--p 001bd000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac15000-78dc2ac16000 ---p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac16000-78dc2ac1a000 r--p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac1a000-78dc2ac1c000 rw-p 00219000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac1c000-78dc2ac29000 rw-p 00000000 00:00 0
    78dc2acea000-78dc2aced000 rw-p 00000000 00:00 0
    78dc2ad03000-78dc2ad05000 rw-p 00000000 00:00 0
    78dc2ad05000-78dc2ad07000 r--p 00000000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad07000-78dc2ad31000 r-xp 00002000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad31000-78dc2ad3c000 r--p 0002c000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad3c000-78dc2ad3d000 rwxp 00000000 00:00 0
    78dc2ad3d000-78dc2ad3f000 r--p 00037000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad3f000-78dc2ad41000 rw-p 00039000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    7ffdb4dcf000-7ffdb4df0000 rw-p 00000000 00:00 0                          [stack]
    7ffdb4df6000-7ffdb4dfa000 r--p 00000000 00:00 0                          [vvar]
    7ffdb4dfa000-7ffdb4dfc000 r-xp 00000000 00:00 0                          [vdso]
    ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
    */
    system(cmdstatus);
    /*
    VmPeak:     2816 kB
    VmSize:     2780 kB
    VmLck:         0 kB
    VmPin:         0 kB
    VmHWM:      1408 kB
    VmRSS:      1408 kB
    VmData:      228 kB
    VmStk:       132 kB
    VmExe:         4 kB
    VmLib:      1796 kB
    VmPTE:        48 kB
    VmSwap:        0 kB
    */
    munmap(anon, length);

    // After munmap
    system(cmdmaps);
    /*
    642d0f7fc000-642d0f7fd000 r--p 00000000 103:02 262183                    /tmp/mmap
    642d0f7fd000-642d0f7fe000 r-xp 00001000 103:02 262183                    /tmp/mmap
    642d0f7fe000-642d0f7ff000 r--p 00002000 103:02 262183                    /tmp/mmap
    642d0f7ff000-642d0f800000 r--p 00002000 103:02 262183                    /tmp/mmap
    642d0f800000-642d0f801000 rw-p 00003000 103:02 262183                    /tmp/mmap
    642d23bac000-642d23bcd000 rw-p 00000000 00:00 0                          [heap]
    78dc2aa00000-78dc2aa28000 r--p 00000000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2aa28000-78dc2abbd000 r-xp 00028000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2abbd000-78dc2ac15000 r--p 001bd000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac15000-78dc2ac16000 ---p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac16000-78dc2ac1a000 r--p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac1a000-78dc2ac1c000 rw-p 00219000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
    78dc2ac1c000-78dc2ac29000 rw-p 00000000 00:00 0
    78dc2acea000-78dc2aced000 rw-p 00000000 00:00 0
    78dc2ad03000-78dc2ad05000 rw-p 00000000 00:00 0
    78dc2ad05000-78dc2ad07000 r--p 00000000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad07000-78dc2ad31000 r-xp 00002000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad31000-78dc2ad3c000 r--p 0002c000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad3d000-78dc2ad3f000 r--p 00037000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    78dc2ad3f000-78dc2ad41000 rw-p 00039000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    7ffdb4dcf000-7ffdb4df0000 rw-p 00000000 00:00 0                          [stack]
    7ffdb4df6000-7ffdb4dfa000 r--p 00000000 00:00 0                          [vvar]
    7ffdb4dfa000-7ffdb4dfc000 r-xp 00000000 00:00 0                          [vdso]
    ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
    */
    /*
    The region 78dc2ad3c000-78dc2ad3d000 is unmapped
    */
    system(cmdstatus);
    /*
    VmPeak:     2816 kB
    VmSize:     2776 kB
    VmLck:         0 kB
    VmPin:         0 kB
    VmHWM:      1408 kB
    VmRSS:      1408 kB
    VmData:      224 kB
    VmStk:       132 kB
    VmExe:         4 kB
    VmLib:      1796 kB
    VmPTE:        48 kB
    VmSwap:        0 kB
    */

    //---------------------------------------------------------------------------------

    return 0;
}