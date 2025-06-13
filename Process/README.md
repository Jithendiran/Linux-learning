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
- exec family
- clone basics
- How syscall happens and how to trace system calls


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

`syscall : clone`

    create a new process

Flow
-----

`fork() -> __libc_fork() -> arch_fork() -> INLINE_SYSCALL_CALL (clone, flags, 0, NULL, ctid, 0)`

**User Application** : `fork()`

**libc**:
`__libc_fork()` -> `arch_fork()` -> `INLINE_SYSCALL_CALL (clone, flags, 0, NULL, ctid, 0)` 

**syscall**: `INLINE_SYSCALL_CALL (clone, flags, 0, NULL, ctid, 0)`

visit before going to kernel flow [syscall](../debug/syscall.md)

**kernel flow**

`sys_clone()` -> `kernel_clone()` -> `copy_process()` -> `wake_up_new_task()`  
`copy_process()` is allocating the resource for process  
`wake_up_new_task()` is put the process in action

---
## wait
- `wait(int *status)` wait till any of the child process to terminate or killed
- `waitpid(pid_t pid, int *status, int options)` wait till specified pid to terminate, killed and signal specified in options  

`syscall : wait4`

    Waits for child process, collects exit status

### pid

   *  \> 0: Waits for the child with that specific PID.
   *  = 0: Waits for any child in the same process group.
   * < -1: Waits for any child whose process group ID is equal to the absolute value of pid.
   * -1: Waits for any child process.

### status
 Although it is defined as int, only bottom 2bytes are taken for account
   * The child terminated by `_exit()` or `exit()` specifies and integer 
   * Terminated by unhandled signal
   * Child stopped by signal and `waitpid()` called with `WUNTRACED`
   * Child resumed by `SIGCONT` in `waitpid()`

### options  

   * `WUNTRACED` , return if a child has stopped (not terminated) due to a signal like SIGSTOP or Ctrl+Z.
   * `WCONTINUED`, return if a stopped child has been resumed due to SIGCONT.
   * `WNOHANG` , Do not block (wait); return immediately.

### orphan
when a parent process killed before, child process then child process is become orphan, it will be adapted by init (pid 1) process

### zombie
when child process is completed (exited) it's execution before parent process do `wait` for child process in this state child process become zombie, meaning all of it's resource are collected back, only process table is maintained (table recording (among other things) the child’s process ID, termination status, and resource usage statistics).

If a parent creates a child, but fails to perform a wait(), then an entry for the zombie child will be maintained indefinitely in the kernel’s process table.
the zombies can’t be killed by a signal, the only way to remove them from the system is to kill their parent (or wait for it to exit), at which time the zombies are adopted init, and init process periodically and automatically calls the wait() system call so the zombie process will be reaped.

A zombie process, also known as a "defunct" process

summary  
- Zombie processes are dead but not yet reaped.
- They can't be "killed" by signals because they already terminated.

### SIGCHILD
When a child terminates, the kernel sends SIGCHLD to its parent.
By default, this signal does nothing.

## Termination
To terminate a process *exit(stats)* or *_exit(status)* is used
- `exit(status)` – user-level (libc) termination
- `_exit(status)` – low-level (syscall) termination


### exit
- Calls registered cleanup handlers: `atexit()` or `on_exit()`
- Flushes all open output stream buffers (`stdout`, `stderr`, etc.)
- Finally calls `_exit()` to exit the process

`syscall : exit_group`

    used to terminate an entire process, including all of its threads.
	
#### When to use 
	inside every process where needed termination with resource cleanup

> [!TIP]  
> `exit()` = **safe termination**.  
> Ensures complete libc cleanup, including I/O and `atexit()` routines.

### _exit
- **Bypasses all user-space cleanup**
- Immediately terminates the process
- OS will:
  - Close file descriptors
  - Release file locks
  - Detach System V shared memory
  - Release semaphores
  - Unmap memory regions (`mmap`)
  - Send `SIGHUP` to all foreground processes in the controlling terminal if needed

#### When to use
    simple: no cleanup needed
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

when a fork is called and exec() function is called, in child process all the exit handlers are removed if any registered by `atext()` or `on_exit()`  
if a process is terminated by signals, it don't call's exit handler by default, signal handler have to call it explicitly, however for `SIGKILL` no way to regoster signal handler, so terminate the process with `SIGTERM`

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


## exec 

### 📋 Summary of Differences Between the `exec()` Functions

v → argument array (char *argv[])

l → argument list (const char *arg0, arg1, ..., NULL)

e → explicit envp[] array

p → search PATH environment variable

| Function   | Specification of Program File (`-`, `p`) | Specification of Arguments (`v`, `l`) | Source of Environment (`e`, `-`)     |
|------------|-------------------------------------------|----------------------------------------|---------------------------------------|
| `execve()` | pathname                                  | array                                  | `envp` argument                        |
| `execl()`  | pathname                                  | list                                   | caller’s `environ`                    |
| `execvp()` | filename + `PATH`                         | array                                  | caller’s `environ`                    |
| `execlp()` | filename + `PATH`                         | list                                   | caller’s `environ`                    |
| `execv()`  | pathname                                  | array                                  | caller’s `environ`                    |
| `execle()` | pathname                                  | list                                   | `envp` argument                        |


During the process of exec, a process image is constructed using the segments of executable file, executable file also allows to define the interpreter (`PT_INTERP` in `ELF`). If interpreter is defined kernel constructs the process image from the segments of the specified interpreter executable file. It is then the responsibility of the interpreter to load and execute the file

syscall : execve

    Replaces current process image with a new one

exec family is different from execve syscall, when we call execve in user space application it is called from libc, libc internally calls the syscall

Programs
----------
[execve](./exec/execve.c)  
[execlp](./exec/execlp.c)  

## clone
Allows fine-grained control over what is shared between parent and child through flags

Process/Thread Creation Flags
    `CLONE_VM`: Share memory space  
    `CLONE_FS`: Share filesystem information (root, working dir)  
    `CLONE_FILES`: Share file descriptors  
    `CLONE_SIGHAND`: Share signal handlers  
    `CLONE_PARENT`: Childs parent same as callers parent   
    `CLONE_THREAD`: Place child in same thread group as parent  
    `CLONE_SYSVSEM`: Share System V SEM_UNDO semantics  
    `CLONE_SETTLS`: Set TLS (Thread Local Storage)  
    `CLONE_PARENT_SETTID`: Write thread id of child into ptid
    `CLONE_CHILD_CLEARTID`: Clear TID in child  

Namespace Flags
    `CLONE_NEWIPC`: New IPC namespace  
    `CLONE_NEWNET`: New network namespace  
    `CLONE_NEWNS`: Child get copy of parent's mount namespace   
    `CLONE_NEWPID`: New PID namespace  
    `CLONE_NEWUSER`: New user namespace  
    `CLONE_NEWUTS`: New UTS namespace  

[clone](./clone.c)  