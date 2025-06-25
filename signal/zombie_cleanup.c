#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Reaped child process %d\n", pid);
    }
}

int main() {
    printf("Parent PID: %d\n", getpid());

    struct sigaction sa = {0};
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    pid_t pid = fork();
    if (pid == 0) {
        // Child
        printf("Child PID: %d, sleeping for 3 sec\n", getpid());
        sleep(3);
        printf("Child PID: %d exiting\n", getpid());
        _exit(0);
    } else {
        // Parent
        printf("Parent waiting... (child will exit soon)\n");
        sleep(5); // Let SIGCHLD be delivered
        printf("Parent exiting\n");
    }

    return 0;
}

//gcc zombie_cleanup.c -o zombie_cleanup.out && ./zombie_cleanup.out

/*
Parent PID: 3560
Parent waiting... (child will exit soon)
Child PID: 3561, sleeping for 3 sec
Child PID: 3561 exiting
Reaped child process 3561
Parent exiting
*/