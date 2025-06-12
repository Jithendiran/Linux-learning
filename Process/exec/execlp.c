#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
extern char **environ;

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        char *new_env[] = {
            "PATH=/usr/bin",
            NULL};
        environ = new_env;
        // setenv("PATH", "/usr/bin", 1); or // environ = new_env;

        // The arguments to the executable
        const char *arg0 = "ls"; // argv[0]
        const char *arg1 = "-l"; // argv[1]
        if (execlp("ls", arg0, arg1, (char *)NULL) == -1)
        {
            printf("Error:%d\n", errno);
        }
    }
    else
    {
        printf("Parent Waiting\n");
        wait(NULL);
    }
    return 0;
}