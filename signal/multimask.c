#define _GNU_SOURCE
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void printSigset(FILE *of, const char *prefix, const sigset_t *sigset) {
    int sig, cnt = 0;
    for (sig = 1; sig < NSIG; sig++) {
        if (sigismember(sigset, sig)) {
            cnt++;
            fprintf(of, "%s%d (%s)\n", prefix, sig, strsignal(sig));
        }
    }
    if (cnt == 0)
        fprintf(of, "%s<empty signal set>\n", prefix);
}

void handler(int sig) {
    printf("Handler invoked for signal: %d (%s)\n", sig, strsignal(sig));
}

int main() {
    printf("PID: %d\n", getpid());
    sigset_t blockSet, pendingSet;

    // Set custom handler for multiple signals
    signal(SIGUSR2, handler);
    signal(SIGUSR1, handler);
    signal(SIGTERM, handler);

    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGUSR1);
    sigaddset(&blockSet, SIGUSR2);
    sigaddset(&blockSet, SIGTERM);

    printf("cat /proc/%d/status | grep Sig\n",getpid());
    // SigQ:	0/59945
    // SigPnd:	0000000000000000
    // SigBlk:	0000000000000000
    // SigIgn:	0000000000000000
    // SigCgt:	0000000000004a00    // custom handler

    // sleep(10);// status check SigPnd

    // Block all 3 signals
    if (sigprocmask(SIG_BLOCK, &blockSet, NULL) == -1) {
        perror("sigprocmask");
    }
    printf("Signals blocked: SIGUSR2, SIGUSR1, SIGTERM\n");

    printf("cat /proc/%d/status | grep Sig\n",getpid());
    // SigQ:	0/59945
    // SigPnd:	0000000000000000
    // SigBlk:	0000000000004a00 // blocked
    // SigIgn:	0000000000000000
    // SigCgt:	0000000000004a00

    // sleep(10);// status check SigPnd

    // Raise signals in specific order
    raise(SIGUSR2);
    raise(SIGTERM);
    raise(SIGUSR1);
    printf("Raised signals in order: SIGUSR2, SIGTERM, SIGUSR1\n");

    printf("cat /proc/%d/status | grep Sig\n",getpid());
    // sleep(10);// status check SigPnd
    // SigQ:	3/59945             // 3 in queue
    // SigPnd:	0000000000004a00    // pending
    // SigBlk:	0000000000004a00
    // SigIgn:	0000000000000000
    // SigCgt:	0000000000004a00

    // Check pending signals
    if (sigpending(&pendingSet) == -1) {
        perror("sigpending");
    }

    printf("Pending signals before unblocking:\n");
    printSigset(stdout, "  ", &pendingSet);

    // Unblock all
    sigprocmask(SIG_UNBLOCK, &blockSet, NULL);
    printf("Unblocked signals\n");

    sleep(5); // Let signals deliver

    return 0;
}

/*
PID: 25525
cat /proc/25525/status | grep Sig
    // SigQ:	0/59945
    // SigPnd:	0000000000000000
    // SigBlk:	0000000000000000
    // SigIgn:	0000000000000000
    // SigCgt:	0000000000004a00
Signals blocked: SIGUSR2, SIGUSR1, SIGTERM
cat /proc/25525/status | grep Sig
    // SigQ:	0/59945
    // SigPnd:	0000000000000000
    // SigBlk:	0000000000004a00 // blocked
    // SigIgn:	0000000000000000
    // SigCgt:	0000000000004a00
Raised signals in order: SIGUSR2, SIGTERM, SIGUSR1
cat /proc/25525/status | grep Sig
    // SigQ:	3/59945             // 3 in queue
    // SigPnd:	0000000000004a00    // pending
    // SigBlk:	0000000000004a00
    // SigIgn:	0000000000000000
    // SigCgt:	0000000000004a00
Pending signals before unblocking:
  10 (User defined signal 1)
  12 (User defined signal 2)
  15 (Terminated)
Handler invoked for signal: 15 (Terminated)
Handler invoked for signal: 12 (User defined signal 2)
Handler invoked for signal: 10 (User defined signal 1)

*/
// POSIX does not guarantee delivery order, here it deliver in desc order