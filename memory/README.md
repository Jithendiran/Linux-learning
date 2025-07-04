A `program` is a file containing a range of information that describes how to construct a `process` at run time.

Informations in file are:  
**Binary format identifier**:

Format of the program. Widely used formats are in olden days `a.out(assembler output)` and  `COFF(Common Object File Format)`. Now a days `ELF(Executable and Linking Format)`.

`file ./a.out`

**Machine instructions**:

Contains compiled machine code (.text section). CPU executes this code when the process runs.  
 
**Program entry-point address**:

Address where execution starts (usually main() after some runtime startup).

`readelf -h a.out | grep Entry`

**Data**:

Values used to initialize variables and Constants
.data: global/static variables with initial values

.rodata: string literals, consts

.bss: globals/statics set to 0

`readelf -S a.out`


**Symbol and relocation table**:

*Symbol table*:

A symbol is a name associated with an address in your code or data for eg: functions (main, printf,..), variables (int c, ab ,...). This symbol table is maintained by compiler and linker that maps the symbol name and address for that to refer. It is used for Linking, debugging, dynamic loading,..

To view symbol table `nm a.out` or `readelf -s a.out`


*Relocation table*:

Relocation is the process of adjusting addresses in your program after linking or loading, especially when: You use external functions like printf, You use shared libraries, The loader loads your program at a different address

To view relocation table `readelf -r a.out`

**Shared-library and dynamic-linking information**:

List of shared libraries that the program needs at run time and the pathname of the dynamic linker that should load these libraries

`readelf -d a.out` for loader and `readelf -S a.out` for linker

dynamic link info `ldd ./a.out` To list shared libraries the program will load.


**Other information**:
Other information to describe the construction of a process

One program can be used to construct many processes, or many processes may be running the same program.
According to the kernel, a process consists of user-space memory (code, variables, etc.) and kernel-space memory (a range of kernel data structures that maintain the state of the process, such as IDs, virtual memory tables, open file descriptors, signal delivery and handling, etc.).


## Memory layout  
Memory allocated to each process is composed of number of parts, referred as segments.

**Text**

This has the machine instructions of the program.  
It is readonly section  
It is shared segment so that many process can use same text segment

**initialized data segment (data)**  

Contains initialized static and global variable. 
The values are read from executable file when the program is loaded into memory

**uninitialized data segment (bss)** 

Contains uninitialized static and global variable.  
Before starting the program system initilize the values to 0

why initialized and uninitialized are stored differently?

Executable file no need to allocate memory for uninitialized memory. Instead it just record the size and location of the memory. Memory is allocated at run time by loader

**stack**

This is dynamically growing memory. For each function call it will create a stack memory.
Stack memory for a function call consists of function arguments, (automatic variable) local variable and return values.  

**heap**

This is dynamically allocated memory at run time. The top end of the heap is called `program break`.

size command displays the size of text, initialized data and uninitialized segments
`size a.out`

### diagram
![virtual memory](./res/memory_layout.png)

```c
extern char etext;  // etext -> address of end of the program text/ start of the initialized data
extern char edata;  // edata -> end of the initialized data segment
extern char end;    // end   -> end of the uninitilized data segment
```

### program
[layout](layout.c), [stack_overflow](stack_overflow.c)

## Virtual Memory
The aim of virtual memory is to efficiently use both the CPU and RAM.

A virtual memory scheme splits the memory used by each program into small, fixed-size units called `pages` (typically 4096 or 8192 bytes). Physical RAM is divided into a series of `page frames` of the same size.

At any point in time, only some of a program’s pages need to be in RAM; the rest can be kept in swap (disk storage). When RAM is full, the kernel may swap out unused pages to disk to make room for active ones.

When a process tries to access a page that is not in RAM, a `page fault` occurs. The kernel suspends the process, loads the required page from disk into RAM, and then resumes execution.

To support virtual memory, the kernel maintains a `page table` for each process. Each page table entry maps a virtual address to a physical address (or indicates if the page is on disk). If a process accesses a memory address with no valid page table entry, the kernel raises a `SIGSEGV` signal (segmentation fault). The range of valid virtual addresses for a process can change during its lifetime.

Virtual memory pages are contiguous in the virtual address space, but the corresponding physical memory may not be contiguous.

A dedicated hardware component, the PMMU (Paged Memory Management Unit), translates virtual addresses to physical addresses and notifies the kernel of page faults.

### Virtual memory advantage
* Process isolation: Each process is isolated from others, so one program cannot read or modify another’s memory.
* Page protection: Each page can have its own permissions (read, write, execute, or combinations).
* Contiguous virtual space: Processes see a contiguous range of memory, even if physical memory is fragmented.
* Efficient memory usage: Not all parts of a process need to be loaded into RAM at once.
* Shared memory: The kernel can map the same physical page into multiple processes’ address spaces (e.g., shared libraries, shared memory regions).
    - The text segment of a process is typically mapped read-only and can be shared across processes.
    - Processes can use the mmap system call to request shared memory regions.

### Page Fault
* A page fault is a hardware event that occurs when a process tries to access a virtual memory page that is not currently mapped to physical RAM.
* The CPU notifies the kernel (via the MMU) when this happens.
* The kernel then decides what to do:
    - If the access is valid (e.g., the page is just swapped out or needs to be allocated), the kernel loads the page into RAM and resumes the process. This is a handled page fault.
    - If the access is invalid (e.g., accessing an unmapped or protected region), the kernel cannot resolve it.

### SIGSEGV
* SIGSEGV (Segmentation Fault) is a signal sent by the kernel to a process when it tries to access memory in a way that is not allowed (e.g., invalid address, writing to read-only memory, accessing unmapped memory).
* This usually happens when a page fault cannot be handled by the kernel (i.e., the memory access is truly invalid).

#### Summary
* Every SIGSEGV is caused by a page fault, but not every page fault results in a SIGSEGV.
* Only unresolvable page faults (invalid access) cause a SIGSEGV.
* Handled page faults (e.g., demand paging, stack growth) are transparent to the process.

Working: page fault -> if valid memory loads or allocate else raise SIGSEGV

## Stack memory

Stack memory is mainly used for function calls. When a function is called, a new stack frame is created on the stack for that call. This frame contains the function's arguments, local variables, return address, and saved CPU registers.

When a function calls another function, certain CPU registers are saved in the stack frame. When the call returns, these registers are restored.

The stack starts at a high memory address and grows downward (toward lower addresses) as new frames are added. The stack pointer register (`rsp` on x86_64) tracks the top of the stack.

Each thread in a process has its own stack. The kernel also maintains a separate stack for its own execution for each process or thread.

If the stack grows beyond its allocated region (for example, due to deep or infinite recursion), a stack overflow occurs, typically resulting in a segmentation fault (`SIGSEGV`).

## Heap memory

When a program needs memory at run time, it uses the heap to allocate memory.
Heap memory starts right after the uninitialized data segment (BSS) and grows or shrinks as memory is allocated and freed. The current limit of the heap is referred to as the `program break`.

Allocating and deallocating heap memory is done by adjusting the program break. When no heap memory is allocated, the program break points to the same location as `extern char *end` (the end of the BSS segment).

After the program break is increased, the process can access addresses in the newly allocated area, but no physical memory pages are created immediately. The kernel will automatically allocate physical pages on the first attempt by the process to access those addresses (this is called demand paging).

system calls used to adjust the program breaks are `brk(void *end_data_seg)` and `sbrk(intptr_t increment)`

brk -> Sets the program break to location specified by end_data_seg, Since virtual memory are allocated as pages, end_data_seg is effectively rounded up to the next boundry

sbrk -> Moves the program break by a specified increment. sbrk(0) returns the current location of the program break.
sbrk(0) will return current location of program break

### malloca and free

#### malloc

malloc and related functions are used to allocate and deallocate memory on the heap. These have several advantages over brk and sbrk:

- malloc is a standard C library function.
- It can be used to allocate any small or large unit of memory.
- Memory returned by malloc is always aligned on a byte boundary suitable for any C data structure (typically 8 or 16 bytes).

To allocate memory, malloc first checks its free list. If the required memory is not available, it requests more memory from the system (using brk, sbrk).

When memory is allocated, malloc actually allocates extra bytes at the beginning of each block to store the size of the block. The address returned to the caller points just past this length value

#### free
free deallocates a block of memory previously allocated by malloc.
- free does not immediately lower the program break. Instead, it adds the block to a list of free blocks that can be recycled by future allocations.
- Because the block to be freed may be in the middle of the heap, it is not always possible to adjust the program break. If many small contiguous blocks at the top end are freed, they may be combined and the program break lowered.
- free may call sbrk to lower the program break only when the free blocks at the top end are sufficiently large.

free will deallocate the block of memory which is allocated by malloc. free don't lower the page break. Instead it adds the block of memory to a list of free blocks that are recycled by future calls. Because the block of memory to be freed may be in the middle so it is not possible to adjust the page break, if a many small continous blocks are freed then all are combined to form a large single block
free calls sbrk to lower the page break only when the free blocks at the top end is sufficiently large.

When a process terminates, all of its memory (including heap memory) is returned to the system, even if free was not called.

### Dynamic memory in stack

It is possible to allocate dynamic memory at stack during run time using `alloca()` function, This is possible because the calling function is in the top of the stack, just by simply moving stack pointer we can get memory.

We should not call `free` for memory allocated by alloca function. It is not possible to use realloc

We can't use alloca in function arguments `func(x, alloca(size));`, This is wrong

pgms  
view physical memory  
view page table where virtual memory and physical memory is mapped