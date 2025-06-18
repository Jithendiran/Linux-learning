# Signals

Signal is notification to process. Most of the time kernel will send the signal to process. One process can send signal to another process.

### when process get signal from kernel

* Hardware exception (divide by zero, mouse click)
* User typed signal (SIGINT, SIGKILL,..)
* Software event become available (when a file descriptor is available, window resize, ...)


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

* SIGCHILD  
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
    When: when a process tries to write to a pipe, FIFO or socker for which no corresponding reader process. This is normally occurs bacause the reading process closed the file descriptor  

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
* If no process matched `errno` set to `ESRCH`.
* if sig is spefified as 0, no signal is sent instead check process can be signaled. If fails `errno` set to `ESRCH`

**Permission**  
* Privileged `(CAP_KILL)` can send signal to any process.  
* Un-Privileged can send signal to other process whose `real` or `effective user ID` of *sending process* matches `real user ID` or `saved setuser ID` of the *receiving process* 

### raise
`int raise(int sig)` is used to send signal to itself. It is equal to `kill(getpid(), sig)`

## killpg
`int killpg(pgrp, sig)` send signal to all of the members of process group. It is equal to `kill(-pgrp, sig)`
if pgrp == 0 then signal is sent to all process in the same process group as caller. 

## Programs
[signal](signal.c)
