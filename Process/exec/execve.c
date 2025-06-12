#include <stdio.h>
#include <errno.h> 
#include <unistd.h>
#include <sys/wait.h>
int main() {
    pid_t pid = fork();

    if(pid == 0){
        char *argv[] = {"ls", "-l", "-a", NULL};
        if(execve("/usr/bin/ls", argv, NULL) == -1)
            printf("Error:%d\n",errno);
    } else {
        printf("Parent Waiting\n");
        wait(NULL);
    }
    return 0;
}