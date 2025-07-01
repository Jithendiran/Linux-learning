#include <stdio.h>
#include <stdlib.h>

static int data_sa = 2;      // static data (data segment)
int data_a = 1;              // global data (data segment)

static int bss_sa;           // static uninitialized (bss segment)
int bss_a;                   // global uninitialized (bss segment)

extern char etext;           // End of text segment
extern char edata;           // End of initialized data segment
extern char end;             // End of uninitialized data segment (bss)

// text -> data->bss->heap->stack-> (args, environ)
int main()
{
    int l_a = 1;             // local variable (stack)
    int *h_a = malloc(2);    // heap allocation
    
    char cmd[64];
    sprintf(cmd, "cat /proc/%d/maps", getpid());
    system(cmd);

    printf("-------------------------------\n");

    printf("Text segment--------------\n");
    printf("  main()        : %p\n", (void*)main);
    printf("  End of text         : %p\n", (void*)&etext);

    printf("Data segment--------------\n");
    printf("  data_a        : %p\n", (void*)&data_a);
    printf("  data_sa       : %p\n", (void*)&data_sa);
    printf("  end of data         : %p\n", (void*)&edata);

    printf("BSS segment--------------\n");
    printf("  bss_a         : %p\n", (void*)&bss_a);
    printf("  bss_sa        : %p\n", (void*)&bss_sa);
    printf("  end of bss          : %p\n", (void*)&end);

    printf("Heap--------------\n");
    printf("  malloc ptr    : %p\n", (void*)h_a);
    printf("  Heap page break (sbrk(0)) : %p\n", sbrk(0)); // sbrk is end of heap

    printf("Stack--------------\n");
    printf("  l_a           : %p\n", (void*)&l_a);

    free(h_a);
    return 0;
}

/*
gcc -g -Wall -Wextra -std=c99 /media/ssd/Project/Linux-learning/memory/layout.c -o /tmp/layout  && /tmp/layout

653069604000-653069605000 r--p 00000000 103:02 262192                    /tmp/layout
653069605000-653069606000 r-xp 00001000 103:02 262192                    /tmp/layout
653069606000-653069607000 r--p 00002000 103:02 262192                    /tmp/layout
653069607000-653069608000 r--p 00002000 103:02 262192                    /tmp/layout
653069608000-653069609000 rw-p 00003000 103:02 262192                    /tmp/layout
65309522c000-65309524d000 rw-p 00000000 00:00 0                          [heap]
75e8e9c00000-75e8e9c28000 r--p 00000000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
75e8e9c28000-75e8e9dbd000 r-xp 00028000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
75e8e9dbd000-75e8e9e15000 r--p 001bd000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
75e8e9e15000-75e8e9e16000 ---p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
75e8e9e16000-75e8e9e1a000 r--p 00215000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
75e8e9e1a000-75e8e9e1c000 rw-p 00219000 103:02 3157728                   /usr/lib/x86_64-linux-gnu/libc.so.6
75e8e9e1c000-75e8e9e29000 rw-p 00000000 00:00 0 
75e8e9f05000-75e8e9f08000 rw-p 00000000 00:00 0 
75e8e9f1e000-75e8e9f20000 rw-p 00000000 00:00 0 
75e8e9f20000-75e8e9f22000 r--p 00000000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
75e8e9f22000-75e8e9f4c000 r-xp 00002000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
75e8e9f4c000-75e8e9f57000 r--p 0002c000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
75e8e9f58000-75e8e9f5a000 r--p 00037000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
75e8e9f5a000-75e8e9f5c000 rw-p 00039000 103:02 3157720                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffd8c1db000-7ffd8c1fc000 rw-p 00000000 00:00 0                          [stack]
7ffd8c2a2000-7ffd8c2a6000 r--p 00000000 00:00 0                          [vvar]
7ffd8c2a6000-7ffd8c2a8000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
-------------------------------
Text segment--------------
  main()        : 0x653069605249
  End of text         : 0x653069605485
Data segment--------------
  data_a        : 0x653069608014
  data_sa       : 0x653069608010
  end of data         : 0x653069608018
BSS segment--------------
  bss_a         : 0x65306960801c
  bss_sa        : 0x653069608020
  end of bss          : 0x653069608028
Heap--------------
  malloc ptr    : 0x65309522c2a0
  Heap page break (sbrk(0)) : 0x9524d000
Stack--------------
  l_a           : 0x7ffd8c1fa094
*/

/*
Analysis
Text:  653069604000 - 653069609000
Heap:  65309522c000 - 65309524d000
Stack: 7ffd8c1db000 - 7ffd8c1fc000 

Text
------------
main -> 0x653069605249  >  653069604000 && < 653069609000
but end of text as per etext is 0x653069605485, as per maps  653069609000
some gap between actuall end of text and etext, some space are blank (Gaps between segment boundaries and maps regions are due to alignment and loader behavior.)

Data 
-----------
0x653069608014 and 0x653069608010  which are lesser than maps textsegment, but greater than etext
static 0x653069608010

non static 0x653069608014

so data segment is right after the text segment

Bss
------------------
0x65306960801c, 0x653069608020 which are also lesser tha maps text segment, but greater than etext
static 0x653069608020

non static 0x65306960801c

How bss and data arranged
Data
1. 0x653069608010 static data
2. 0x653069608014 non static data
3. 0x65306960801c non static bss
4. 0x653069608020 static bss

data                 edata      bss                  end
static -> non static |          non-static -> static |

Heap
----------------------
Should gt than 0x653069608028
ptr in 0x65309522c2a0,yes it is gt than end

sbrk is end of heap
0x9524d000 is tuncated value actual values is 65309524d000, 6530 is truncated

Stack
------------------------
which will graw down word so Stack: 7ffd8c1db000 - 7ffd8c1fc000  should read as 7ffd8c1fc000 - 7ffd8c1db000
0x7ffd8c1fa094 is less than 7ffd8c1fc000 and greater than 7ffd8c1db000
*/