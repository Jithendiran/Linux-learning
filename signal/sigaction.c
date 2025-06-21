#define _GNU_SOURCE
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

void sigint_handler(int signo, siginfo_t *info, void *context) {
    printf("Caught SIGINT (%d) from PID %d\n", signo, info->si_pid);
}

void sigusr1_handler(int signal){
    printf("Caught SIGUSR1  : %d\n",signal);
}

int main() {
    // Set up SIGINT handler with SA_SIGINFO
    struct sigaction sa_int = {0};
    sa_int.sa_sigaction = sigint_handler;
    sa_int.sa_flags = SA_SIGINFO;
    sigemptyset(&sa_int.sa_mask);
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        return 1;
    }

    // Set up SIGUSR1 handler with SA_SIGINFO
    struct sigaction sa_usr1 = {0};
    sa_usr1.sa_handler = sigusr1_handler;
    sigemptyset(&sa_usr1.sa_mask);
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("sigaction SIGINT");
        return 1;
    }


    // Set up SIGCHLD handler with SA_NOCLDWAIT
    // no need to call wait()
    struct sigaction sa_chld = {0};
    sa_chld.sa_handler = SIG_IGN; // Ignore
    sa_chld.sa_flags = SA_NOCLDWAIT;
    sigemptyset(&sa_chld.sa_mask);
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction SIGCHLD");
        return 1;
    }

    raise(SIGUSR1);

    // Fork a child to demonstrate SA_NOCLDWAIT (no zombie)
    pid_t pid = fork();
    if (pid == 0) {
        printf("Child process (PID %d) exiting\n", getpid());
        _exit(0);
    } else if (pid > 0) {
        printf("Parent process (PID %d), child PID %d\n", getpid(), pid);
    } else {
        perror("fork");
        return 1;
    }

    printf("Press Ctrl+C to trigger SIGINT (from your terminal)\n");
    sleep(10); // Wait for signals

    printf("Parent exiting\n");
    return 0;
}

/**
Caught SIGUSR1  : 10
Parent process (PID 4867), child PID 4869
Press Ctrl+C to trigger SIGINT (from your terminal)
Child process (PID 4869) exiting
^CCaught SIGINT (2) from PID 0
Parent exiting
 */