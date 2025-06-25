#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

void handler(int sig) {
    printf("Caught signal %d (%s)\n", sig, strsignal(sig));
}

int main() {
    sigset_t block_mask, suspend_mask, old_mask;

    printf("PID: %d\n", getpid());

    // Step 1: Install handler
    struct sigaction sa = {0};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    // Step 2: Block SIGUSR1
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &block_mask, &old_mask);
    printf("SIGUSR1 blocked\n");

    // Step 3: Prepare suspend mask (unblock SIGUSR1 while waiting)
    suspend_mask = old_mask;
    sigdelset(&suspend_mask, SIGUSR1); // Unblock SIGUSR1 while waiting

    printf("Send SIGUSR1 to PID %d from another terminal\n", getpid());
    printf("Waiting for SIGUSR1 using sigsuspend...\n");
    // in new terminal : kill -USR1 <pid>
    // Step 4: Wait for signal

    // replace the current mask 
    // why we need to block the SIGUSR1 before sigsuspend?
    /*
    This is not required, but using for more controlled manner, whithout  sigprocmask(SIG_BLOCK, &block_mask, &old_mask);. signal may be arrived and handled
    then wait for signal in sigsuspend, it will go into sleep till 2nd signal arrives (which may arrives or maynot) to prevent this we are using block 
    */
    sigsuspend(&suspend_mask);  // Returns -1 and sets errno = EINTR after handler runs
    // Step 5: Back to normal flow
    printf("Back from sigsuspend\n");

    // Restore previous signal mask (optional here)
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    return 0;
}

/*
PID: 5074
SIGUSR1 blocked
Send SIGUSR1 to PID 5074 from another terminal
Waiting for SIGUSR1 using sigsuspend...
                    --------------------------------------- new terminal
                                                            kill -USR1 5074
                    ---------------------------------------
Caught signal 10 (User defined signal 1)
Back from sigsuspend
*/