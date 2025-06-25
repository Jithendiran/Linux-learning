#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void sigchld_handler(int sig) {
    printf("Parent received SIGCHLD (%d)\n", sig);
    // No wait() needed due to SA_NOCLDWAIT
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        printf("Child [%d]: running and exiting...\n", getpid());
        _exit(0);
    } else {
        // Parent process
        printf("Parent [%d]: created child [%d]\n", getpid(), pid);

        // Give child time to exit
        sleep(2);

        printf("\nChecking if child is zombie (should not be):\n");
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "ps -o pid,ppid,stat,cmd -p %d", pid);
        system(cmd);  // Should not show 'Z' state

        printf("\nParent exiting.\n");
    }

    return 0;
}

// gcc zombie_cleanup3.c -o zombie_cleanup3.out && ./zombie_cleanup3.out
/*
Parent [4584]: created child [4585]
Child [4585]: running and exiting...
Parent received SIGCHLD (17)                // this for fork

Checking if child is zombie (should not be):
    PID    PPID STAT CMD
Parent received SIGCHLD (17)            // this for system()

Parent exiting.
*/