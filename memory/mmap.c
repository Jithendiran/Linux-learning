#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main()
{
    size_t length = 4096 * 1024;
    pid_t pid = getpid();
    char cmdmaps[120], cmdstatus[120];
    char smaps_cmd[256];
    sprintf(cmdmaps, "cat /proc/%d/maps", pid);
    sprintf(cmdstatus, "cat /proc/%d/status | grep Vm", pid);

    // Before mmap
    printf("%s\n", cmdmaps);
    system(cmdmaps);
    printf("\n");
    printf("%s\n", cmdstatus);
    system(cmdstatus);

    printf("\n-----------------------------------BEFORE------------------------------------------------\n");


    // 1. Anonymous mapping (private)
    char *anon = mmap(NULL, length, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    /*
    We’ve requested 4 MB of virtual address space — and the kernel reserves it in your virtual address space.

    But physical memory is not allocated until you touch (write or read) each page.
    This is called demand paging.
    */
    
    if (anon == MAP_FAILED)
    {
        perror("mmap (anonymous)");
        return 1;
    }

    snprintf(smaps_cmd, sizeof(smaps_cmd),
             "awk '/^%lx-/{flag=1} flag && /^[0-9a-f]+-[0-9a-f]+/ && !/^%lx-/ {flag=0} flag' /proc/%d/smaps",
             (unsigned long)anon, (unsigned long)anon, pid);


    // After mmap
    printf("%s\n", cmdmaps);
    system(cmdmaps);
    printf("\n");
    printf("%s\n", cmdstatus);
    system(cmdstatus);

    printf("\n--- /proc/self/smaps for new mapping (before write) ---\n");
    system(smaps_cmd);

    printf("\n-----------------------------------After MMAP BW------------------------------------------------\n");



    // strcpy(anon, "Hello from anonymous mmap!");
    /*
    That touches only one page (first 4 KB). So:
    Only 1 page (4 KB) is materialized (i.e., backed by physical memory).
    Therefore, only 4 KB is reflected in:
    VmRSS in /proc/<pid>/status
    Rss, Private_Dirty in /proc/<pid>/smaps
    The rest of the 4 MB is still just virtual memory — no physical RAM backing yet.
    */
    printf("Anonymous mapping: %s\n\n", anon); // Anonymous mapping: Hello from anonymous mmap!

    // to see real changes replace the above code with below strcpy

    for (size_t i = 0; i < length; i += 4096)
    anon[i] = 'A';
    // Now you're writing 1 byte into each 4KB page — i.e., touching all pages → causing page faults for each one.


    // After written
    printf("%s\n", cmdmaps);
    system(cmdmaps); 
    printf("\n");
    printf("%s\n", cmdstatus);
    system(cmdstatus);
    printf("\n--- /proc/self/smaps for new mapping (after write) ---\n");
    system(smaps_cmd);

    printf("\n-----------------------------------After Write ------------------------------------------------\n");

    munmap(anon, length);

    // After munmap
    printf("%s\n", cmdmaps);
    system(cmdmaps);
    printf("\n");
    printf("%s\n", cmdstatus);
    system(cmdstatus);
    printf("\n-----------------------------------Unmapped------------------------------------------------\n");

    return 0;
}

/*
cat /proc/3150/maps
5ac5e6810000-5ac5e6811000 r--p 00000000 103:02 262183                    /tmp/mmap
5ac5e6811000-5ac5e6812000 r-xp 00001000 103:02 262183                    /tmp/mmap
5ac5e6812000-5ac5e6813000 r--p 00002000 103:02 262183                    /tmp/mmap
5ac5e6813000-5ac5e6814000 r--p 00002000 103:02 262183                    /tmp/mmap
5ac5e6814000-5ac5e6815000 rw-p 00003000 103:02 262183                    /tmp/mmap
5ac5ea92f000-5ac5ea950000 rw-p 00000000 00:00 0                          [heap]
77a6ad600000-77a6ad628000 r--p 00000000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad628000-77a6ad7bd000 r-xp 00028000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad7bd000-77a6ad815000 r--p 001bd000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad815000-77a6ad816000 ---p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad816000-77a6ad81a000 r--p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad81a000-77a6ad81c000 rw-p 00219000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad81c000-77a6ad829000 rw-p 00000000 00:00 0 
77a6ad900000-77a6ad903000 rw-p 00000000 00:00 0 
77a6ad919000-77a6ad91b000 rw-p 00000000 00:00 0 
77a6ad91b000-77a6ad91d000 r--p 00000000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad91d000-77a6ad947000 r-xp 00002000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad947000-77a6ad952000 r--p 0002c000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad953000-77a6ad955000 r--p 00037000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad955000-77a6ad957000 rw-p 00039000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffdb4844000-7ffdb4865000 rw-p 00000000 00:00 0                          [stack]
7ffdb4993000-7ffdb4997000 r--p 00000000 00:00 0                          [vvar]
7ffdb4997000-7ffdb4999000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]

cat /proc/3150/status | grep Vm
VmPeak:     2812 kB
VmSize:     2776 kB
VmLck:         0 kB
VmPin:         0 kB
VmHWM:      1408 kB
VmRSS:      1408 kB
VmData:      224 kB
VmStk:       132 kB
VmExe:         4 kB
VmLib:      1796 kB
VmPTE:        44 kB
VmSwap:        0 kB

-----------------------------------BEFORE------------------------------------------------
cat /proc/3150/maps
5ac5e6810000-5ac5e6811000 r--p 00000000 103:02 262183                    /tmp/mmap
5ac5e6811000-5ac5e6812000 r-xp 00001000 103:02 262183                    /tmp/mmap
5ac5e6812000-5ac5e6813000 r--p 00002000 103:02 262183                    /tmp/mmap
5ac5e6813000-5ac5e6814000 r--p 00002000 103:02 262183                    /tmp/mmap
5ac5e6814000-5ac5e6815000 rw-p 00003000 103:02 262183                    /tmp/mmap
5ac5ea92f000-5ac5ea950000 rw-p 00000000 00:00 0                          [heap]
77a6ad200000-77a6ad600000 rwxp 00000000 00:00 0                                                                                 // 4MB mapping => 0x77a6ad600000 - 0x77a6ad200000 => = 0x400000 => 0x400000 = 4,194,304 bytes = 4 MB
77a6ad600000-77a6ad628000 r--p 00000000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad628000-77a6ad7bd000 r-xp 00028000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad7bd000-77a6ad815000 r--p 001bd000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad815000-77a6ad816000 ---p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad816000-77a6ad81a000 r--p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad81a000-77a6ad81c000 rw-p 00219000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad81c000-77a6ad829000 rw-p 00000000 00:00 0 
77a6ad900000-77a6ad903000 rw-p 00000000 00:00 0 
77a6ad919000-77a6ad91b000 rw-p 00000000 00:00 0 
77a6ad91b000-77a6ad91d000 r--p 00000000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad91d000-77a6ad947000 r-xp 00002000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad947000-77a6ad952000 r--p 0002c000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad953000-77a6ad955000 r--p 00037000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad955000-77a6ad957000 rw-p 00039000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffdb4844000-7ffdb4865000 rw-p 00000000 00:00 0                          [stack]
7ffdb4993000-7ffdb4997000 r--p 00000000 00:00 0                          [vvar]
7ffdb4997000-7ffdb4999000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]

cat /proc/3150/status | grep Vm
VmPeak:     6908 kB                 // +4mb
VmSize:     6872 kB                 // +4mb
VmLck:         0 kB
VmPin:         0 kB
VmHWM:      1408 kB
VmRSS:      1408 kB
VmData:     4320 kB
VmStk:       132 kB
VmExe:         4 kB
VmLib:      1796 kB
VmPTE:        44 kB
VmSwap:        0 kB

--- /proc/self/smaps for new mapping (before write) ---
77a6ad200000-77a6ad600000 rwxp 00000000 00:00 0 
Size:               4096 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   0 kB         // physical page not allocated
Pss:                   0 kB
Pss_Dirty:             0 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         0 kB
Referenced:            0 kB
Anonymous:             0 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr ex mr mw me ac sd 

-----------------------------------After MMAP BW------------------------------------------------
Anonymous mapping: 

cat /proc/3150/maps
5ac5e6810000-5ac5e6811000 r--p 00000000 103:02 262183                    /tmp/mmap
5ac5e6811000-5ac5e6812000 r-xp 00001000 103:02 262183                    /tmp/mmap
5ac5e6812000-5ac5e6813000 r--p 00002000 103:02 262183                    /tmp/mmap
5ac5e6813000-5ac5e6814000 r--p 00002000 103:02 262183                    /tmp/mmap
5ac5e6814000-5ac5e6815000 rw-p 00003000 103:02 262183                    /tmp/mmap
5ac5ea92f000-5ac5ea950000 rw-p 00000000 00:00 0                          [heap]
77a6ad200000-77a6ad600000 rwxp 00000000 00:00 0 
77a6ad600000-77a6ad628000 r--p 00000000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad628000-77a6ad7bd000 r-xp 00028000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad7bd000-77a6ad815000 r--p 001bd000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad815000-77a6ad816000 ---p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad816000-77a6ad81a000 r--p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad81a000-77a6ad81c000 rw-p 00219000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad81c000-77a6ad829000 rw-p 00000000 00:00 0 
77a6ad900000-77a6ad903000 rw-p 00000000 00:00 0 
77a6ad919000-77a6ad91b000 rw-p 00000000 00:00 0 
77a6ad91b000-77a6ad91d000 r--p 00000000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad91d000-77a6ad947000 r-xp 00002000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad947000-77a6ad952000 r--p 0002c000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad953000-77a6ad955000 r--p 00037000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad955000-77a6ad957000 rw-p 00039000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffdb4844000-7ffdb4865000 rw-p 00000000 00:00 0                          [stack]
7ffdb4993000-7ffdb4997000 r--p 00000000 00:00 0                          [vvar]
7ffdb4997000-7ffdb4999000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]

cat /proc/3150/status | grep Vm
VmPeak:     6908 kB
VmSize:     6872 kB
VmLck:         0 kB
VmPin:         0 kB
VmHWM:      5376 kB             // physical page allocated  1408 + 4096 = 5376 kB
VmRSS:      5376 kB             // physical page allocated  1408 + 4096 = 5376 kB
VmData:     4320 kB
VmStk:       132 kB
VmExe:         4 kB
VmLib:      1796 kB
VmPTE:        52 kB
VmSwap:        0 kB

--- /proc/self/smaps for new mapping (after write) ---
77a6ad200000-77a6ad600000 rwxp 00000000 00:00 0 
Size:               4096 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                4096 kB         // physical page allocated
Pss:                4096 kB
Pss_Dirty:          4096 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:      4096 kB
Referenced:         4096 kB
Anonymous:          4096 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr ex mr mw me ac sd 

-----------------------------------After Write ------------------------------------------------
cat /proc/3150/maps
5ac5e6810000-5ac5e6811000 r--p 00000000 103:02 262183                    /tmp/mmap
5ac5e6811000-5ac5e6812000 r-xp 00001000 103:02 262183                    /tmp/mmap
5ac5e6812000-5ac5e6813000 r--p 00002000 103:02 262183                    /tmp/mmap
5ac5e6813000-5ac5e6814000 r--p 00002000 103:02 262183                    /tmp/mmap
5ac5e6814000-5ac5e6815000 rw-p 00003000 103:02 262183                    /tmp/mmap
5ac5ea92f000-5ac5ea950000 rw-p 00000000 00:00 0                          [heap]
77a6ad600000-77a6ad628000 r--p 00000000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad628000-77a6ad7bd000 r-xp 00028000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad7bd000-77a6ad815000 r--p 001bd000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad815000-77a6ad816000 ---p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad816000-77a6ad81a000 r--p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad81a000-77a6ad81c000 rw-p 00219000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
77a6ad81c000-77a6ad829000 rw-p 00000000 00:00 0 
77a6ad900000-77a6ad903000 rw-p 00000000 00:00 0 
77a6ad919000-77a6ad91b000 rw-p 00000000 00:00 0 
77a6ad91b000-77a6ad91d000 r--p 00000000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad91d000-77a6ad947000 r-xp 00002000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad947000-77a6ad952000 r--p 0002c000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad953000-77a6ad955000 r--p 00037000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
77a6ad955000-77a6ad957000 rw-p 00039000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffdb4844000-7ffdb4865000 rw-p 00000000 00:00 0                          [stack]
7ffdb4993000-7ffdb4997000 r--p 00000000 00:00 0                          [vvar]
7ffdb4997000-7ffdb4999000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]

// region unmapped

cat /proc/3150/status | grep Vm
VmPeak:     6908 kB
VmSize:     2776 kB
VmLck:         0 kB
VmPin:         0 kB
VmHWM:      5376 kB     
VmRSS:      1408 kB     // Since 4MB are continous frame's, size is reduced
VmData:      224 kB
VmStk:       132 kB
VmExe:         4 kB
VmLib:      1796 kB
VmPTE:        44 kB
VmSwap:        0 kB

-----------------------------------Unmapped------------------------------------------------
*/