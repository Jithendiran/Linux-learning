#include <stdio.h>
#include <stdlib.h>

void printStatus()
{
    char cmd[64];
    printf("--------------------------------------------\n");
    sprintf(cmd, "cat /proc/%d/status | grep Vm", getpid());
    system(cmd);
    printf("--------------------------------------------\n");
}
int main()
{
    printStatus();
    /*
    --------------------------------------------
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
    VmPTE:        48 kB
    VmSwap:        0 kB
    --------------------------------------------
    */
    int *p = (int *)malloc(100 * 1024 * 1024); // 104857600 bytes
    printStatus();
    /*
    --------------------------------------------
    VmPeak:   105216 kB                         // 105MB
    VmSize:   105180 kB                         // 105MB
    VmLck:         0 kB
    VmPin:         0 kB
    VmHWM:      1408 kB                         // 1.4MB (no change)
    VmRSS:      1408 kB                         // 1.4MB (no change)
    VmData:   102628 kB                         // 102MB
    VmStk:       132 kB
    VmExe:         4 kB
    VmLib:      1796 kB
    VmPTE:        52 kB
    VmSwap:        0 kB
    --------------------------------------------
    */
    for (size_t i = 0; i < (100 * 1024 * 1024) / sizeof(int); i += 1024)
        p[i] = 1;
    printStatus();
    /*
    --------------------------------------------
    VmPeak:   105216 kB                     // no change
    VmSize:   105180 kB                     // no change
    VmLck:         0 kB
    VmPin:         0 kB
    VmHWM:    103680 kB                     // 103MB // physical memory allocated
    VmRSS:    103680 kB                     // 103MB
    VmData:   102628 kB                     // no change
    VmStk:       132 kB
    VmExe:         4 kB
    VmLib:      1796 kB
    VmPTE:       252 kB
    VmSwap:        0 kB
    --------------------------------------------
    */
    free(p);
    printStatus();
    /*
    --------------------------------------------
    VmPeak:   105216 kB                     // no change
    VmSize:     2776 kB                     // 2.7MB
    VmLck:         0 kB
    VmPin:         0 kB
    VmHWM:    103680 kB                     // no change (can be re used, in free list)
    VmRSS:      1408 kB                     // 1.4MB
    VmData:      224 kB
    VmStk:       132 kB
    VmExe:         4 kB
    VmLib:      1796 kB
    VmPTE:        48 kB
    VmSwap:        0 kB
    --------------------------------------------
*/
}

/*
jidesh@jidesh-MS-7E26:/media/ssd/Project/Linux-learning$ ltrace /tmp/lazy_alloc 


puts("--------------------------------"...--------------------------------------------
)                                                                  = 45
getpid()                                                                                                     = 4465
sprintf("cat /proc/4465/status | grep Vm", "cat /proc/%d/status | grep Vm", 4465)                            = 31
system("cat /proc/4465/status | grep Vm"VmPeak:     2816 kB
VmSize:     2780 kB
VmLck:         0 kB
VmPin:         0 kB
VmHWM:      1408 kB
VmRSS:      1408 kB
VmData:      224 kB
VmStk:       136 kB
VmExe:         4 kB
VmLib:      1796 kB
VmPTE:        52 kB
VmSwap:        0 kB
 <no return ...>
--- SIGCHLD (Child exited) ---
<... system resumed> )                                                                                       = 0
puts("--------------------------------"...--------------------------------------------
)                                                                  = 45
malloc(104857600)                                                                                            = 0x799b651ff010
puts("--------------------------------"...--------------------------------------------
)                                                                  = 45
getpid()                                                                                                     = 4465
sprintf("cat /proc/4465/status | grep Vm", "cat /proc/%d/status | grep Vm", 4465)                            = 31
system("cat /proc/4465/status | grep Vm"VmPeak:   105220 kB
VmSize:   105184 kB
VmLck:         0 kB
VmPin:         0 kB
VmHWM:      1408 kB
VmRSS:      1408 kB
VmData:   102628 kB
VmStk:       136 kB
VmExe:         4 kB
VmLib:      1796 kB
VmPTE:        56 kB
VmSwap:        0 kB
 <no return ...>
--- SIGCHLD (Child exited) ---
<... system resumed> )                                                                                       = 0
puts("--------------------------------"...--------------------------------------------
)                                                                  = 45
puts("--------------------------------"...--------------------------------------------
)                                                                  = 45
getpid()                                                                                                     = 4465
sprintf("cat /proc/4465/status | grep Vm", "cat /proc/%d/status | grep Vm", 4465)                            = 31
system("cat /proc/4465/status | grep Vm"VmPeak:   105220 kB
VmSize:   105184 kB
VmLck:         0 kB
VmPin:         0 kB
VmHWM:    103680 kB
VmRSS:    103680 kB
VmData:   102628 kB
VmStk:       136 kB
VmExe:         4 kB
VmLib:      1796 kB
VmPTE:       256 kB
VmSwap:        0 kB
 <no return ...>
--- SIGCHLD (Child exited) ---
<... system resumed> )                                                                                       = 0
puts("--------------------------------"...--------------------------------------------
)                                                                  = 45
free(0x799b651ff010)                                                                                         = <void>
puts("--------------------------------"...--------------------------------------------
)                                                                  = 45
getpid()                                                                                                     = 4465
sprintf("cat /proc/4465/status | grep Vm", "cat /proc/%d/status | grep Vm", 4465)                            = 31
system("cat /proc/4465/status | grep Vm"VmPeak:   105220 kB
VmSize:     2780 kB
VmLck:         0 kB
VmPin:         0 kB
VmHWM:    103680 kB
VmRSS:      1408 kB
VmData:      224 kB
VmStk:       136 kB
VmExe:         4 kB
VmLib:      1796 kB
VmPTE:        52 kB
VmSwap:        0 kB
 <no return ...>
--- SIGCHLD (Child exited) ---
<... system resumed> )                                                                                       = 0
puts("--------------------------------"...--------------------------------------------
)                                                                  = 45
+++ exited (status 0) +++
*/