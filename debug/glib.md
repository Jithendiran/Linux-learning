# Glibc debug

fork(), wait(),.. most of them are implemente din glibc

Symbols
--------

`#` command line
`(gdb)` inside gdb terminal

## Setup

Know version
---------------
> \# ldd --version
    
    ldd (Ubuntu GLIBC 2.35-0ubuntu3.10) 2.35
    Copyright (C) 2022 Free Software Foundation, Inc.
    This is free software; see the source for copying conditions.  There is NO
    warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    Written by Roland McGrath and Ulrich Drepper.

my version is 2.35

### Download the source code for glibc
my PWD `/home/jidesh/glibc-source`  
1. > \# wget https://ftp.gnu.org/gnu/glibc/glibc-2.35.tar.xz
2. > \# tar xf glibc-2.35.tar.xz


## cmd line
1. \# gcc -g Process.c -o Process.out
2. \# gdb ./Process.out
3. `(gdb)` dir /home/jidesh/glibc-source/glibc-2.35
4. `(gdb)` r

```gdb
GNU gdb (Ubuntu 12.1-0ubuntu1~22.04.2) 12.1
Copyright (C) 2022 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<https://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
--Type <RET> for more, q to quit, c to continue without paging--c
Reading symbols from ./Process.out...
(gdb) dir /home/jidesh/glibc-source/glibc-2.35
Source directories searched: /home/jidesh/glibc-source/glibc-2.35:$cdir:$cwd
(gdb) b fork
Breakpoint 1 at 0x1190
(gdb) r
Starting program: /media/ssd/Project/Linux-learning/Process/Process.out 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
File offset before fork(): 0
O_APPEND flag before fork() is: off

Breakpoint 1, __libc_fork () at ./posix/fork.c:41
41      {
(gdb) 
```

# VScode

visit [launch.json](../.vscode/launch.json), [taksk.json](../.vscode/tasks.json)