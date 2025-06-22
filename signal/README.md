# Signals

Signal is notification to process. Most of the time kernel will send the signal to process. One process can send signal to another process.

### when process get signal from kernel

* Hardware exception (divide by zero, mouse click)

These happen when the CPU detects an abnormal condition during process execution.
| Condition             | Signal    | Example                      |
| --------------------- | --------- | ---------------------------- |
| Divide by zero        | `SIGFPE`  | `int x = 1 / 0;`             |
| Invalid memory access | `SIGSEGV` | Dereferencing a null pointer |
| Illegal instruction   | `SIGILL`  | Jumping to garbage memory    |
| Bus error             | `SIGBUS`  | Misaligned memory access     |

---
* User typed signal (SIGINT, SIGKILL,..)  
Terminal driver sends signals to the foreground process group:  

| Key Combo  | Signal                           | Description        |
| ---------- | -------------------------------- | ------------------ |
| `Ctrl + C` | `SIGINT`                         | Interrupt          |
| `Ctrl + \` | `SIGQUIT`                        | Quit and core dump |
| `Ctrl + Z` | `SIGTSTP`                        | Terminal stop      |
| `Ctrl + D` | EOF (not signal, but input ends) |                    |

Signals are sent by processes explicitly via syscalls:

| Function Call                      | Description                         |
| ---------------------------------- | ----------------------------------- |
| `kill(pid, sig)`                   | Send signal to another process      |
| `raise(sig)`                       | Send signal to self                 |
| `killpg(pgid, sig)`                | Send to process group               |
| `pthread_kill(tid, sig)`           | Send to a thread in same process    |
| `sigqueue(pid, sig, union sigval)` | Send signal with value (RT signals) |


---
* Software event become available (when a file descriptor is available, window resize, ...)
Kernel can notify processes of asynchronous events:

| Event                             | Signal     | Description                            |
| --------------------------------- | ---------- | -------------------------------------- |
| Child exits                       | `SIGCHLD`  | Sent to parent                         |
| Timer expires (`alarm()`)         | `SIGALRM`  | Sent to process after timer ends       |
| File descriptor ready (async I/O) | `SIGIO`    | When data is ready                     |
| Window size changed (xterm)       | `SIGWINCH` | Window resize event                    |
| Background write to terminal      | `SIGTTOU`  | Background process writing to terminal |

---

| Category         | Who Generates Signal?  | Examples                         |
| ---------------- | ---------------------- | -------------------------------- |
| Hardware Fault   | CPU → Kernel → Process | `SIGFPE`, `SIGSEGV`              |
| Terminal Input   | TTY driver             | `SIGINT`, `SIGQUIT`, `SIGTSTP`   |
| Software Syscall | Process itself         | `kill()`, `raise()`              |
| Kernel Events    | Kernel                 | `SIGCHLD`, `SIGALRM`, `SIGWINCH` |
| Libc / App Code  | Library or app         | `abort()`, `assert()`            |

-----
-----

### Classification

* Traditional or standard signal (1 to 31)
* realtime signal

## Pending signal

Time between signal generation and delivery is state of pending. If a process is currently not running then signal will delivered to the process as soon as it is next scheduled for CPU

## Signal mask

Some times, some part of code should not interrupt by signals, that time we can block the signal by signal mask, when a signal is sent on this time it will be in pending state till signals are unblocked

## Default actions

* Ignored
* Terminated (abnormal process termination)
* Core dump
* Stopped
* Resumed
Instead of accepting the default action, process can disposition the signal with user handler

> [!IMPORTANT]  
> It is not possible to set the disposition of the signal to terminate or core dump, unless it one of these are default signal

## Status
 For the process in `/proc/{id}/status` file has the information about the signals, it is in hexa decimal format. Least bit is 1 and next is 2 like that. These fields are  

SigPng  - Per thread pending signal  
ShdPnd  - Process-wide pending signal  
SigBlk  - Blocked signal  
SigIgn  - Ignored signal  
SigCgt  - Caught signal    

## Types and default action

Listed only few

* SIGABRT    
    When: Sent when a process calls abort().    
    Default: terminates with core dump.    

* SIGALRM  
    When: When expiration of real-time timer  

* SIGCHLD  
    When: Parent receives the signal when one of it is child is terminated (exit() or kill by signal), stopped or resumed  

* SIGCONT  
    When: user specific  
    Default: When on receive of this signal stopped process will resume it's execution, by default this signal is ignored  

* SIGHUP  
    When: Terminal disconnect occurs, this signal sent to controlling process os the terminal  
    Use: For daemons when the signal is sent, daemons are reinitialize themselves  

* SIGINT  
    When: user type Control-c, terminal driver send the signal to foreground process group  
    Default:  To terminate the process  

* SIGKILL  
    This sure kill signal, it can't be blocked, ignored or caught with handler. It always terminates the process  

* SIGPIPE  
    When: when a process tries to write to a pipe, FIFO or socket for which no corresponding reader process. This is normally occurs bacause the reading process closed the file descriptor  

* SIGQUIT  
    When: User type control + \ in terminal.
    To whom: Sent to foreground process group
    Default: It will  terminate the process and cause core dump

> [!NOTE]  
> We can connect core dump with GDB.  

* SIGTERM    
    When: User type control + c in terminal  
    What: This is the standard signal to terminate the process, well designed application have a handler for this signal and do the resource cleanup  
    Default: terminate the process

* SIGUSR1 and SIGUSR2  
    What: This is user defined signals, generally kernel don't send this type of signals   

## Changing Signal disposition

* signal()  
    * This is simple one, no additional feature.
    * It is not possible to retrieve the current disposition of a signal without at the same time changing that disposition

    SYSCALL: `signal() -> __sysv_signal() -> __sigaction() -> __libc_sigaction() -> INLINE_SYSCALL_CALL (rt_sigaction, sig, ... )`  
    **syscall**: `rt_sigaction`  
    **Kernel syscall handler** : `sys_rt_sigaction`  
    **Kernel implementation**: `SYSCALL_DEFINE4(rt_sigaction, int, sig, const struct sigaction __user *, act, struct sigaction __user *, oact, size_t, sigsetsize)`


* sigaction()  
This is complex and feature rich

## kill, raise and killpg

### Kill

`int Kill(pid, sig)` is used to send a signal to a process

* pid > 0, signal sent to the proccess whose pid is matched
* pid == 0, signal sent to every process in the same process group, including the calling proces itself
* pid < -1, signal sent to all process in the process group whose id equals the absolute value of pid
* pid == -1, signal sent to every process for which the calling process has permission to sent (except init). If a privileged process makes this call then all process in the system will be signaled. This is sometimes called as *broadcast*
    -> Affects processes where your UID matches the real or effective UID  
    -> Excludes process 1 (init)  
    -> Can be dangerous if run as root  
* If no process matched `errno` set to `ESRCH`.
* if sig is spefified as 0, no signal is sent instead check process can be signaled. If fails `errno` set to `ESRCH`

**Permission**  
* Privileged `(CAP_KILL)` can send signal to any process.  
* Un-Privileged can send signal to other process whose `real` or `effective user ID` of *sending process* matches `real user ID` or `saved setuser ID` of the *receiving process* 

### raise

`int raise(int sig)` is used to send signal to itself. It is equal to `kill(getpid(), sig)`

### killpg

`int killpg(pgrp, sig)` send signal to all of the members of process group. It is equal to `kill(-pgrp, sig)`
if pgrp == 0 then signal is sent to all process in the same process group as caller. 

### Programs

[signal](signal.c)

## Signal set

Multiple signals are represented using a data structure called a signal set, provided by the system data type sigset_t.

There are functions to init empty signal set, init fill signal set, add a signal to signal set, remove a signal from signal set, is a signal present in signal set

## Mask
Kernel maintain a signal set for each process, each thread to be blocked. Blocking `SIGKILL` and `SIGSTOP` is not possible. All the blocked signal will be in the pending state, when ever signal is unblocked, signals will be delivered. How ever it is actually mask, it don't count how many times the particular signal signaled, It just deliver once

Using `sigprocmask` it is possible to block or unblock signals

`SIG_BLOCK` - To block a set of signals  
`SIG_UNBLOCK` - To unblock set of signals
`SIG_SETMASK` -  To completely replace the current signal mask with a new one

`sigpending` is used to retrieve all the pending signals for a process

### Doubt
if SIGCHILD, SIGUSR1, SIGUSR2 is in pending state, which signal will deliver to process as first.
    Ans: Order is not guarenteed.

## Program
[mask](mask.c), [multimask](multimask.c)

## Sigaction
It is more flexible way of handling the signal. When signal is caught in handler it is automatically added to signal mask and when leaving it will remove from signal mask
[sigaction](./sigaction.c)

## Reentrant
A function is said to be rentrant if it can safely be simultaneoulsy executed by multiple thread of execution in the same process.
A function it employes only local variables is guarenteed to be reentrant. Even use of static variable is consider as non-reentrant

## exit
It is possible to exit the signal handler with `_exit`, performing `goto`, `longjump` when performing jump or goto depends on the methods we are calling the signal is unmasked.

## Alternative stack
When a process attempts to grow it's stack beyond the maximum possible size, the ketnel generates a `SIGSEGV` for the process.
* Allocate a memory for alternative stack
* Use `sigaltstack` syscall to inform kernel about alternative signal stack.
* When establishing a signal handler specify `SA_ONSTACK` flag to tell the kernel, stack for this handler should be created alternatively

## AS_SIGINFO
It is used to obtain additional information when signal is delivered, when this flag is used, handler will looks like `void handler(int sig, siginfo_t *siginfo, void *ucontext)`

## Restart system call
Consider the scenario   
1. Handler is established for signal.  
2. Make a blocking system call, for example `read` from a terminal device. Which blocks till input is supplied.
3. When system call is blocked, signal is delivered to a process. Here disposition handler will be called once the call is returned to main code. System call will be failed with eror code `EINTR`.

* Some times even without custom handler for some signals `(SIGSTOP, SIGSTP,.. SIGCONT)`, blocked system call will throw an `EINTR` error when signal is delivered.
* When read is blocking at the time signal is delivered, here read has partial data here instead of fail with `EINTR`. read will return the so far available data.  

## Reentrancy 

A function is **reentrant** if it can be safely invoked by multiple threads or from signal handlers without causing data corruption.  
It must not:
- use static or global variables
- modify shared memory without synchronization
- rely on standard I/O buffers

Use only **async-signal-safe** functions in handlers. Refer: `man 7 signal-safety`.

TODO  
  
Trigger SIGSEGV and analyze core dump  
int *p = NULL;
*p = 42;  // causes SIGSEGV

sigqueue(), siginfo_t, real-time signals (SIGRTMIN..)  
Explore delivery latency, reliability (standard vs real-time)  

sigqueue() with siginfo_t

Raise core dump with SIGSEGV, inspect with gdb

Test signal queuing with real-time signals (SIGRTMIN+N)