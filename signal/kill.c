#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/signal.h>  

void signal_handler(int signum) {
    printf("Process %d received signal %d\n", getpid(), signum);
}

int main() {
    // Set up signal handler for SIGUSR1
    if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    // Create child processes
    pid_t child1 = fork();
    if (child1 < 0) {
        perror("fork child1");
        return 1;
    }

    if (child1 == 0) {  // First child
        printf("Child 1 (PID: %d, PGID: %d) running\n", getpid(), getpgrp());
        while(1) sleep(1);  // Keep running
        return 0;
    }

    pid_t child2 = fork();
    if (child2 < 0) {
        perror("fork child2");
        kill(child1, SIGKILL);  // Clean up first child
        return 1;
    }

    if (child2 == 0) {  // Second child
        printf("Child 2 (PID: %d, PGID: %d) running\n", getpid(), getpgrp());
        while(1) sleep(1);  // Keep running
        return 0;
    }

    // Parent process
    printf("Parent (PID: %d, PGID: %d) running\n", getpid(), getpgrp());
    sleep(1);  // Give time for output

    // Demo 1: Send to specific process (pid > 0)
    printf("\nDemo 1: Sending SIGUSR1 to Child 1 (PID: %d)\n", child1);
    if (kill(child1, SIGUSR1) == -1) {
        perror("kill child1");
    }
    sleep(1);

    // Demo 2: Send to process group (pid == 0)
    printf("\nDemo 2: Sending SIGUSR1 to all processes in group\n");
    if (kill(0, SIGUSR1) == -1) {
        perror("kill group");
    }
    sleep(1);

    // Demo 3: Check if process exists (sig == 0)
    printf("\nDemo 3: Checking if PID 99999 exists\n");
    if (kill(99999, 0) == -1) {
        printf("Process 99999 does not exist (errno: %d)\n", errno);
    }
    sleep(1);

    // Demo 4: Send to specific process group (pid < -1)
    // printf("\nDemo 4:  %d\n", getpgrp());
    // if (kill(-1, SIGUSR1) == -1) { 
    //     perror("kill process group");
    // }
    // sleep(1);
    // // Above code will cause the login session crash 

    // Demo 5: Send to specific process group (pid < -1)
    printf("\nDemo 5: Sending SIGUSR1 to process group %d\n", getpgrp());
    if (kill(-getpgrp(), SIGUSR1) == -1) {
        perror("kill process group");
    }
    sleep(1);

    // Clean up children
    printf("\nCleaning up children...\n");
    kill(child1, SIGTERM);
    kill(child2, SIGTERM);
    wait(NULL);
    wait(NULL);

    return 0;
}

/*
Parent (PID: 4275, PGID: 4275) running
Child 2 (PID: 4278, PGID: 4275) running
Child 1 (PID: 4277, PGID: 4275) running

Demo 1: Sending SIGUSR1 to Child 1 (PID: 4277)
Process 4277 received signal 10

Demo 2: Sending SIGUSR1 to all processes in group
Process 4275 received signal 10
Process 4278 received signal 10

Demo 3: Checking if PID 99999 exists
Process 99999 does not exist (errno: 3)

Demo 5: Sending SIGUSR1 to process group 4275
*/