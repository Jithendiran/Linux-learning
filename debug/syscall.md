# Syscall

## How to find syscall mapping in linux

There is an entry file in this location `linux/arch/x86/entry/syscalls/syscall_64.tbl`, this entry file has the mapping of syscall 

Eg:
If user application call
`fork() -> __libc_fork() -> arch_fork() -> INLINE_SYSCALL_CALL (clone, flags, 0, NULL, ctid, 0)`

1. User can debug till `INLINE_SYSCALL_CALL`, here syscall is happened so kernel will do the things from here. To identify the syscall in  `INLINE_SYSCALL_CALL` called function is `clone`. We have to find it's kernel entry for clone

2. open `syscall_64.tbl` file search for `clone`. In `clone` case it's entry is `56	common	clone			sys_clone`

3. when user app called `clone`, kernel will receive it's request in `sys_clone`

4. Now search for function definition 

>[!TIP]  
>Instead of debug use strace to find the syscalls


> \# grep -rn "sys_clone(" /media/Linux/linux-6.8 

```bash
/media/Linux/linux-6.8/include/linux/syscalls.h:786:asmlinkage long sys_clone(unsigned long, unsigned long, int __user *, unsigned long,
/media/Linux/linux-6.8/include/linux/syscalls.h:790:asmlinkage long sys_clone(unsigned long, unsigned long, int, int __user *,
/media/Linux/linux-6.8/include/linux/syscalls.h:793:asmlinkage long sys_clone(unsigned long, unsigned long, int __user *,
```
`syscalls.h` contains the reference

6. In `syscalls.h` you may see multiple declation and different parameter for same sys call, to find it's definition use grep like this `SYSCALL_DEFINE{N_no_param(syscall`
    eg: let's take `asmlinkage long sys_clone(unsigned long, unsigned long, int __user *, unsigned long, int __user *);` this one, it has 5 parameters so the grep will be
    > \# grep -rn "SYSCALL_DEFINE5(clone" .

    ```
    ./kernel/fork.c:3013:SYSCALL_DEFINE5(clone, unsigned long, clone_flags, unsigned long, newsp,
    ./kernel/fork.c:3018:SYSCALL_DEFINE5(clone, unsigned long, newsp, unsigned long, clone_flags,
    ./kernel/fork.c:3029:SYSCALL_DEFINE5(clone, unsigned long, clone_flags, unsigned long, newsp,
    ```
    `fork.c` is the file has clone function definition