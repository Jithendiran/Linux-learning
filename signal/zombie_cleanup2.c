// gcc zombie_cleanup2.c -o zombie_cleanup2.out && ./zombie_cleanup2.out

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
/*
Useful when
* When you don’t care about child exit status.
* Ideal in simple servers or background utilities that launch short-lived helpers.
*/

// caution
/*
If you both ignore SIGCHLD and try to call wait(), wait() will fail with ECHILD
→ Because the child is auto-reaped — it no longer exists.
*/

int main() {
    // Ignore SIGCHLD to auto-reap children
    signal(SIGCHLD, SIG_IGN);

    pid_t pid = fork();
    if (pid == 0) {
        printf("Child PID: %d\n", getpid());
        _exit(0);
    } else {
        printf("Parent PID: %d\n", getpid());
        sleep(5); // Let child exit
        printf("Parent exits\n");
    }
    return 0;
}


// check the child process uaing ps, it won't exists