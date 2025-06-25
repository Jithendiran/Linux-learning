#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int main() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

    // Block these signals so they don't interrupt normally
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    // Create the signalfd
    int sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        perror("signalfd");
        exit(EXIT_FAILURE);
    }

    printf("PID: %d — Send SIGUSR1 or SIGUSR2 to see output\n", getpid());

    while (1) {
        struct signalfd_siginfo fdsi;
        ssize_t s = read(sfd, &fdsi, sizeof(fdsi));
        if (s != sizeof(fdsi)) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        printf("Received signal %d from PID %d\n", fdsi.ssi_signo, fdsi.ssi_pid);
    }

    close(sfd);
    return 0;
}

/*
gcc -o filddes.out filddes.c && ./filddes.out

new terminal
kill -USR1 <pid>
kill -USR2 <pid>

*/