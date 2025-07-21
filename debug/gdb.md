# GDB debug

>[!TIP]  
> To run in debugger we have to compile the program using `-g` flag, ex: gcc -g a.c -o a.out

## GDB run

Run from `gdb`: `gdb a.out`  
If program has core dump: `gdb a.out core`

(gdb) `start` -> To stop at program entry

## List locals

Every command are executed in gdb

List args: `info args`  

List around code: `list`  

List all local variables: `info locals`  

List particular variable: `print a`

Change variable content: `set var a="Hello"`  

## proc

To List all: `info proc all`

To list mapping: `info proc mappings`  

explore other options by giving `tab` from `info proc`

## Frame

Frame represents stack frame

`#0` -> current frame, where breakpoint stops

`#1` -> The function which is called the frame 0 function

```
#0  0x000055555555515d in level3 (s=0x0) at a.c:4
#1  0x00005555555551ab in level2 (flag=1) at a.c:11
#2  0x00005555555551cb in level1 () at a.c:17
#3  0x00005555555551e0 in main () at a.c:21
```

To List all the frames: `bt`  

To go abve frame: `up`, move from frame 0 -> frame 1
To go below frame: `down`, move from frame 1 -> frame 0

## Breakpoints
View breakpoints: `info b`   
Where in the file: `where`
current line info: `info line`

### Why does a single line cover multiple addresses?
gdb `info line`  
`Line 3 of "a.c" starts at address 0x555555555164 <main+27> and ends at 0x555555555178 <main+47>.`  
A single source line (e.g., x = y + z;) may compile into multiple machine instructions, like:

1. Load y into a register.
2. Add z.
3. Store result into x.
All these instructions belong to the same source line, so GDB maps line 3 to this address range.

break *0x555555555164 -> set break point at specific address not at whole line
or to check better, disassemble to machine instructions and set the breakpoints

To disassemble: `disassemble`  
To disassemble a function: `disassemble main`  
```gdb
Dump of assembler code for function main:
   0x0000555555555149 <+0>:	endbr64 
   0x000055555555514d <+4>:	push   %rbp
   0x000055555555514e <+5>:	mov    %rsp,%rbp
   0x0000555555555151 <+8>:	sub    $0x10,%rsp
   0x0000555555555155 <+12>:	mov    %fs:0x28,%rax
   0x000055555555515e <+21>:	mov    %rax,-0x8(%rbp)
   0x0000555555555162 <+25>:	xor    %eax,%eax
=> 0x0000555555555164 <+27>:	lea    -0xd(%rbp),%rax              # this is the current line
   0x0000555555555168 <+31>:	movl   $0x64636261,(%rax)
   0x000055555555516e <+37>:	movw   $0x6665,0x4(%rax)
   0x0000555555555174 <+43>:	movb   $0x0,0x6(%rax)
   0x0000555555555178 <+47>:	mov    $0x0,%eax
   0x000055555555517d <+52>:	mov    -0x8(%rbp),%rdx
   0x0000555555555181 <+56>:	sub    %fs:0x28,%rdx
   0x000055555555518a <+65>:	je     0x555555555191 <main+72>
   0x000055555555518c <+67>:	call   0x555555555050 <__stack_chk_fail@plt>
   0x0000555555555191 <+72>:	leave  
   0x0000555555555192 <+73>:	ret    
End of assembler dump.
```

To go to next instruction: `stepi` // it will go  instruction by instruction

## To get a address of variable
1. p &add
```gdb
(gdb) p &s
$2 = (char (*)[5]) 0x7fffffffdef3
```

## Print value at specific address
`x/<count><format><size> <address>`

```
<count> – How many units to print.

<format> – How to display values (x, d, u, o, t, c, s).
    x – Hexadecimal (default).

    d – Signed decimal.

    u – Unsigned decimal.

    o – Octal.

    t – Binary.

    c – Character.

    s – String.
<size> – Size of each unit (b, h, w, g).
    b – 1 byte.

    h – 2 bytes (halfword).

    w – 4 bytes (word).

    g – 8 bytes (giant word).

<address> – The memory address or variable pointer.
```

eg:  


Print 1 value (4 bytes) at a given address: `x/w 0x7fffffffe0`  
Print 5 integers starting at address 0x7fffffffe0: `x/5dw 0x7fffffffe0`
    `5d → 5 values in decimal, w → each value is 4 bytes (word).`

Print a string pointed by variable: ` x/s s` -> last s is the variable name 

```
(gdb) print s
$3 = "abcde"
(gdb) x/1b 0x7fffffffdef3       -> print one byte at location 0x7fffffffdef3
0x7fffffffdef3:	97
(gdb) 
```

Static array

```
(gdb)  x/5dw static_array
0x555555558010 <static_array>:	1	2	3	4
0x555555558020 <static_array+16>:	5

(gdb)  x/2w &static_array
0x555555558010 <static_array>:	1	2

```

Dynamic array
1. To know address: 
```
(gdb) print dynamic_array
$2 = (int *) 0x5555555592a0
```
To view values
```
(gdb)  x/10dw 0x5555555592a0
0x5555555592a0:	10	20	30	40
0x5555555592b0:	50	0	1041	0
0x5555555592c0:	1667331155	1918967915
(gdb) 

```

`x/10i $pc` -> Prints 10 instructions from the program counter ($pc).

`dump binary memory memdump.bin 0x7fffffffe0 0x7fffffffe0+64`
This dumps 64 bytes to memdump.bin.



## Watch
watch is used to stop execution when the value of an expression changes.

`watch x` GDB will stop execution when the value of x changes.

`watch *ptr` Execution halts when the contents at that location change.

`rwatch <expression>`  Stops when the expression is read.  

`awatch <expression>` Stops when the expression is read or written.

`info watchpoints`


## Threads
` info threads ` to know about no of threads
```
(gdb) info threads 
  Id   Target Id                                Frame 
  1    Thread 0x7ffff7fa2740 (LWP 5920) "a.out" clone3 () at ../sysdeps/unix/sysv/linux/x86_64/clone3.S:62
* 2    Thread 0x7ffff7bff640 (LWP 5953) "a.out" worker1 (arg=0x0) at a.c:6
  3    Thread 0x7ffff73fe640 (LWP 5954) "a.out" clone3 () at ../sysdeps/unix/sysv/linux/x86_64/clone3.S:62
```
`bt` gives the backtrace of current thread
```
(gdb) bt
#0  worker1 (arg=0x0) at a.c:6
#1  0x00007ffff7c94ac3 in start_thread (arg=<optimized out>) at ./nptl/pthread_create.c:442
#2  0x00007ffff7d26850 in clone3 () at ../sysdeps/unix/sysv/linux/x86_64/clone3.S:81

```
`thread apply all bt ` display all thread backtrace
```
(gdb) thread apply all bt

Thread 3 (Thread 0x7ffff73fe640 (LWP 5954) "a.out"):
#0  clone3 () at ../sysdeps/unix/sysv/linux/x86_64/clone3.S:62
#1  0x0000000000000000 in ?? ()

Thread 2 (Thread 0x7ffff7bff640 (LWP 5953) "a.out"):
#0  worker1 (arg=0x0) at a.c:6
#1  0x00007ffff7c94ac3 in start_thread (arg=<optimized out>) at ./nptl/pthread_create.c:442
#2  0x00007ffff7d26850 in clone3 () at ../sysdeps/unix/sysv/linux/x86_64/clone3.S:81

Thread 1 (Thread 0x7ffff7fa2740 (LWP 5920) "a.out"):
#0  clone3 () at ../sysdeps/unix/sysv/linux/x86_64/clone3.S:62
#1  0x00007ffff7d268a1 in __GI___clone_internal (cl_args=cl_args@entry=0x7fffffffdcb0, func=func@entry=0x7ffff7c947d0 <start_thread>, arg=arg@entry=0x7ffff73fe640) at ../sysdeps/unix/sysv/linux/clone-internal.c:54
#2  0x00007ffff7c946d9 in create_thread (pd=pd@entry=0x7ffff73fe640, attr=attr@entry=0x7fffffffddd0, stopped_start=stopped_start@entry=0x7fffffffddce, stackaddr=stackaddr@entry=0x7ffff6bfe000, stacksize=8388352, thread_ran=thread_ran@entry=0x7fffffffddcf) at ./nptl/pthread_create.c:295
#3  0x00007ffff7c95200 in __pthread_create_2_1 (newthread=<optimized out>, attr=<optimized out>, start_routine=<optimized out>, arg=<optimized out>) at ./nptl/pthread_create.c:828
#4  0x00005555555552e7 in main () at a.c:27

```

`set scheduler-locking on`  Makes GDB pause all other threads while stepping through one (helps with race conditions).

`break my_function thread 2`

break at function `my_function` when hitted by thread 2

`thread <n>` -> to switch the thread

## Reverse Debugging
| Command                   | What it does                                    |
| ------------------------- | ----------------------------------------------- |
| `record`                  | Starts recording execution                      |
| `reverse-continue` (`rc`) | Continue **in reverse** direction               |
| `reverse-step`            | Step **backward into** function calls           |
| `reverse-next`            | Step **backward over** function calls           |
| `reverse-finish`          | Run backward until the current function returns |
| `info record`             | Shows recording status                          |
