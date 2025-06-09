# Process 
* fork()
* exit()
* wait()
* copy-on-write (COW)

## 🧠 What I Learned
- How `fork()` creates a child process (cloning)
- Why `wait()` is needed (zombies, orphan)
- When to use `exit()` vs `_exit()`
- How copy-on-write behaves in forked processes

## Todo
* wait() (SIGCHILD)
* clone()
* strace

## explain

process is a executing program in linux

| PID | Process Name       | Role                                 |
| --- | ------------------ | ------------------------------------ |
| 0   | swapper / idle     | Kernel-only process; boot & idle     |
| 1   | `init` / `systemd` | First user-space process             |
| 2   | `kthreadd`         | Spawns kernel threads                |
| ... | ...                | User processes, kernel threads, etc. |


> On multi-core CPUs, each core has its own idle process with PID 0, but only one is the "original" process 0.


## Creation
When you run `ls`, your shell acts as the **parent**, and `ls` is the **child**.

All Linux processes (except PID 0 and 1) are created using the `fork()` system call, which duplicates the parent process.

---

## Termination
To terminate a process *exit(stats)* or *_exit(status)* is used
*exit*
	Which will cleanup the resources and terminate the process
	Resources in the sense, opened descriptor, stack segment, data segment, heap segment
	Flush all the open output stream
	
### When to use 
	inside every process where needed termination with resource cleanup

>[!TIP]  
>**safe** termination: cleans up resources like file descriptors, memory, and flushes output

*_exit*
	It does not perform cleanup task

### When to use
	If a process is forked but failed when executing exec() function family. In this case no resources are created for child process. So at this time exit() is used means  in child process resources like file descriptor are inherited from parent process, it will flush out the buffer  and parent also use exit() it will again flush out, so two times log may print.
	Shared memory state from parent process could lead to wired behaviour

>[!TIP]  
>**raw** termination: skips cleanup

1. 🧪 Example 1: `exec()` in child, use `_exit()` if it fails
```c
pid_t pid = fork();
if (pid == 0) {
    // Child
    execlp("pgm", "pgm", NULL);  // inside pgm exit(0) can be used 
    _exit(1);  // Only called if exec fails
} else {
    // Parent
    wait(NULL);
    exit(0);
}
```
2. 🧪 Example 2: No exec(), so both use exit()
```c
pid_t pid = fork();

if (pid == 0) {
	// Child process
	printf("Child PID: %d\n", getpid());
	// here no exec() calls are used so exit(0) can be used
	exit(0);  // ✅ Child should exit
} else {
	// Parent process
	wait(NULL);  // Wait for child to finish
	printf("Parent PID: %d\n", getpid());
	exit(0);  // ✅ Parent should also exit
}
```


## Copy-On-Write

When a process is duplicated using fork, it will use copy on write methodology. 
*Text segment*
	single text shared segment is used for both parent and child

*Data, heap, stack segment*
	Parents segment is marked as read only, when a process attempts to modify the resource, new private resource will be created for attempted process and that resource will be linked with attempted process, now two process has different resource

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int x = 42;
    int y = 12;
    pid_t pid = fork();

    if (pid == 0) {
        // Child
        printf("Child sees x = %d\n", x);  // 42
        sleep(1);
        printf("Child sees x again = %d\n", x);  // Still 42
        y = 1;
        printf("Child sees y = %d\n", y);  // 1
        // new resource created for y, no new resource created for x
    } else {
        // Parent
        x = 99;  // Parent writes → triggers COW
        printf("Parent changed x to = %d\n", x);  // 99
        printf("Parent changed y to = %d\n", y);  // 12
        wait(NULL);
        // new resource created for x, no new resource created for y
    }

    return 0;
}

```

## File sharing
    Open file descriptors in parent and child share the same open file description. If child made some changes parent could see the changes.


[Process](./Process.c) 