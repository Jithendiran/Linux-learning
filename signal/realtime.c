#define _GNU_SOURCE
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

void rt_handler(int signo, siginfo_t *info, void *context)
{
    printf("Received realtime signal %d with value %d from PID %d\n",
           signo, info->si_value.sival_int, info->si_pid);
}

int main()
{
    printf("PID: %d\n", getpid());

    struct sigaction sa = {0};
    sa.sa_sigaction = rt_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    int rt_sig = SIGRTMIN; // Use the first realtime signal
    // SIGRTMIN to SIGRTMAX

    // Register handlers for SIGRTMIN, SIGRTMIN+1, SIGRTMIN+2, SIGRTMIN+3
    for (int i = 0; i < 4; ++i)
    {
        if (sigaction(rt_sig + i, &sa, NULL) == -1)
        {
            perror("sigaction");
            exit(1);
        }
    }

    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigemptyset(&old_set);

    // Block SIGRTMIN+1, SIGRTMIN+2, SIGRTMIN+3
    sigaddset(&block_set, rt_sig + 1);
    sigaddset(&block_set, rt_sig + 2);
    sigaddset(&block_set, rt_sig + 3);
    if (sigprocmask(SIG_BLOCK, &block_set, &old_set) == -1)
    {
        perror("sigprocmask block");
        exit(1);
    }
    printf("Blocked SIGRTMIN+1, SIGRTMIN+2, SIGRTMIN+3 \n");

    // Wait for signals
    for (int i = 0; i < 10; ++i)
    {
        pause();
    }

    // Unblock after loop
    if (sigprocmask(SIG_SETMASK, &old_set, NULL) == -1) {
        perror("sigprocmask unblock");
        exit(1);
    }
    printf("Unblocked SIGRTMIN+1, SIGRTMIN+2, SIGRTMIN+3 after loop\n");

    // Wait a bit to handle any pending signals
    sleep(2);

    return 0;
}

// execute
/*
gcc -o realtime.out realtime.c && ./realtime.out

new terminal
gcc -o realtime_signal.out realtime_signal.c
./realtime_signal.out <pid> 100
./realtime_signal.out <pid> 200
... do for 8 more times

---
*/

/*

terminal 1 
 gcc -o realtime.out realtime.c && ./realtime.out
PID: 5363
Blocked SIGRTMIN+1, SIGRTMIN+2, SIGRTMIN+3 
==================================================

Terminal 2
./realtime_signal.out 5363  34 100 
./realtime_signal.out 5363  37 1            # (b)
./realtime_signal.out 5363  35 2            # (b)
./realtime_signal.out 5363  36 3            # (b)
./realtime_signal.out 5363  34 200
./realtime_signal.out 5363  37 4            # (b)
./realtime_signal.out 5363  34 300
./realtime_signal.out 5363  34 400
./realtime_signal.out 5363  34 500
./realtime_signal.out 5363  34 600
./realtime_signal.out 5363  34 700
./realtime_signal.out 5363  34 800
./realtime_signal.out 5363  35 5            # (b)
./realtime_signal.out 5363  34 900
./realtime_signal.out 5363  34 1000
*/

/*OP
PID: 5363
Blocked SIGRTMIN+1, SIGRTMIN+2, SIGRTMIN+3 
Received realtime signal 34 with value 100 from PID 5515
Received realtime signal 34 with value 200 from PID 5649
Received realtime signal 34 with value 300 from PID 5712
Received realtime signal 34 with value 400 from PID 5743
Received realtime signal 34 with value 500 from PID 5773
Received realtime signal 34 with value 600 from PID 5815
Received realtime signal 34 with value 700 from PID 5857
Received realtime signal 34 with value 800 from PID 5899
Received realtime signal 34 with value 900 from PID 5987
Received realtime signal 34 with value 1000 from PID 6017
Received realtime signal 37 with value 1 from PID 5545
Received realtime signal 37 with value 4 from PID 5679
Received realtime signal 36 with value 3 from PID 5634
Received realtime signal 35 with value 2 from PID 5569
Received realtime signal 35 with value 5 from PID 5945
Unblocked SIGRTMIN+1, SIGRTMIN+2, SIGRTMIN+3 after loop
*/