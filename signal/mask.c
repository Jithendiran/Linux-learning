#define _GNU_SOURCE
#include <string.h>
#include <signal.h>
#include <stdio.h>

void /* Print list of signals within a signal set */
printSigset(FILE *of, const char *prefix, const sigset_t *sigset)
{
    int sig, cnt;

    cnt = 0;
    for (sig = 1; sig < NSIG; sig++)
    {
        if (sigismember(sigset, sig))
        {
            cnt++;
            fprintf(of, "%s%d (%s)\n", prefix, sig, strsignal(sig));
        }
    }

    if (cnt == 0)
        fprintf(of, "%s<empty signal set>\n", prefix);
}

void sig_user1_handler(int signal)
{
    printf("Inside handler : %s\n", strsignal(signal));
}

int main()
{
    printf("Process ID: %d\n", getpid());
    sigset_t pendingMask, blockingMask, emptyMask;

    signal(SIGUSR1, sig_user1_handler);

    if (sigemptyset(&blockingMask) == -1)
    {
        perror("Empty signal set");
    }

    if (sigaddset(&blockingMask, SIGUSR1) == -1)
    {
        perror("Assign signal set");
    }

    if (sigprocmask(SIG_BLOCK, &blockingMask, NULL) == -1)
    {
        perror("Mask");
    }
    printf("Blocked SIGUSR1\n");

    sleep(10); // do cat /proc/<pid>/status check SigPnd, SigBlk
// SigQ:	0/59945
// SigPnd:	0000000000000000
// SigBlk:	0000000000000200    // ..200 indicate SIGUSR1 in blocked state
// SigIgn:	0000000000000000
// SigCgt:	0000000000000200    // ..200 indicate SIGUSR1 has custom handler

/*
How 200 is SIGUSR1

Signal	Number	Bit Position
----------------------------
SIGHUP	1	    Bit 0
SIGINT	2	    Bit 1
SIGQUIT	3	    Bit 2
...	    ...	    ...
SIGUSR1	10	    Bit 9
SIGUSR2	12	    Bit 11

For a signal N, it is represented by the bit at position N-1.
Signal Number = 10, Bit Position = 10 - 1 = 9
1 << (10 - 1) = 1 << 9 = 512 = 0x200

how 512?
0000000000000001  (16-bit view)
hifting it left by 9 positions (1 << 9) gives:
0000001000000000 = 2^(9) or (1 << 9 ) = 512 
512 is in decimal form, convert to hexadecimal
512 d -> 200 h
*/


    //-------------------------------

    raise(SIGUSR1);
    printf("Raised SIGUSR1\n");
    if (sigpending(&pendingMask) == -1)
    {
        perror("Retrivel error");
    }

    printf("Checking pending signal\n");
    printSigset(stdout, "\t", &pendingMask);

    sleep(10);// do cat /proc/<pid>/status check SigPnd
// SigQ:	1/59945                 1 signal is in queue 
// SigPnd:	0000000000000200        SIGUSR1 is in queue
// SigBlk:	0000000000000200
// SigIgn:	0000000000000000
// SigCgt:	0000000000000200


    printf("Unblocking all blocked signals\n");
    sigemptyset(&emptyMask);
    /* Unblock all signals */
    if (sigprocmask(SIG_SETMASK, &emptyMask, NULL) == -1)
        perror("sigprocmask");

    printf("Unblocked all blocked signals\n");

    if (sigpending(&pendingMask) == -1)
    {
        perror("Retrivel error");
    }
    printf("Checking pending signal\n");
    printSigset(stdout, "\t", &pendingMask);
    //--------------------------------------------------

    raise(SIGUSR1);
    printf("Raised SIGUSR1\n");
    if (sigpending(&pendingMask) == -1)
    {
        perror("Retrivel error");
    }
    printf("Checking pending signal\n");
    printSigset(stdout, "\t", &pendingMask);
}

/*
Blocked SIGUSR1
Raised SIGUSR1
Checking pending signal
        10 (User defined signal 1)
Unblocking all blocked signals
Inside handler : User defined signal 1
Unblocked all blocked signals
Checking pending signal
        <empty signal set>
Inside handler : User defined signal 1
Raised SIGUSR1
Checking pending signal
        <empty signal set>
*/