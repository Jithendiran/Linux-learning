/*
check : realtime.c
*/
#define _GNU_SOURCE
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <pid> <signo> <value>\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);
    int rt_sig = atoi(argv[2]);
    int value = atoi(argv[3]);

    union sigval sval;
    sval.sival_int = value;

    if (sigqueue(pid, rt_sig, sval) == -1) {
        perror("sigqueue");
        return 1;
    }

    printf("Sent SIGRTMIN (%d) with value %d to process %d\n", rt_sig, value, pid);
    return 0;
}

/*
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