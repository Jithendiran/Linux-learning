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

## Page table
A page table stores the mapping between virtual memory and physical memory. In linux `/proc/self/pagemap` will give you the page table

On a 64-bit system, the page size is typically 4096 bytes (4KB), and the virtual address space is 64 bits wide.

To find no of possible pages:  
1. Each page is 4096 bytes = 2^(12) bytes. 
    
    For example, addresses 0x0000 to 0x0FFF are in the first page; only the starting address of each page is stored.

2. The total number of unique virtual address is 2^(64).  

3. Number of pages = total address space / page size

    = 2^(64) / 2^(12)
    = 2^(52)

So, there are `2^(52) possible pages in a 64-bit` address space with 4KB pages.

If each page were 1 byte instead of 4096 bytes, there would be 2⁶⁴ pages, but that is not the case.

With 2^(52) pages, the file is extremely large (even if most of it is sparse and not physically stored). 
Most entries are zero because most of the virtual address space is not mapped. 
Catting the whole file will appear to hang or do nothing, because it's outputting mostly zeros (NUL bytes) and is extremely large.  
It's a binary file, not text, so outputting it to a terminal is not useful.  

The correct way to use `/proc/PID/pagemap`:
1. Find the virtual address you care about (e.g., from /proc/PID/maps).
2. calculate the offset (virtual_address / page_size) * 8 :  
    - Divide the virtual address by the page size (e.g., 4096) to get the page number.  (virtual_address / page_size)  
    - Multiply the page number by 8 (the size of each entry) to get the byte offset in the pagemap file.  
3. Seek to that offset and read 8 bytes to get the pagemap entry for that page.  

Example:

Assume page size is 8 bytes (for illustration), and a 64-bit virtual memory address. The table would look like:
        0: [0 - 7]
        1: [8 - 15]
        2: [16 - 23]
        ....
If the virtual address is 2, page size is 8 bytes:  
2 / 8 = 0 → virtual address 2 is in the 0th entry.

If the virtual address is 12, page size is 8 bytes:
12 / 8 = 1 → 1st entry.

To get the starting address, multiply the entry index by the entry size (8 bytes, since each entry is 64 bits).
0th entry: 0 * 8 = 0
1st entry: 1 * 8 = 8

[refer](https://www.kernel.org/doc/Documentation/vm/pagemap.txt)

#### program
[page_table](./pagetable.c)


## Stack memory

Stack memory is mainly used for function calls. When a function is called, a new stack frame is created on the stack for that call. This frame contains the function's arguments, local variables, return address, and saved CPU registers.

When a function calls another function, certain CPU registers are saved in the stack frame. When the call returns, these registers are restored.

The stack starts at a high memory address and grows downward (toward lower addresses) as new frames are added. The stack pointer register (`rsp` on x86_64) tracks the top of the stack.

Each thread in a process has its own stack. The kernel also maintains a separate stack for its own execution for each process or thread.

If the stack grows beyond its allocated region (for example, due to deep or infinite recursion), a stack overflow occurs, typically resulting in a segmentation fault (`SIGSEGV`).

The guard page is an unmapped region placed at the end of the stack to catch stack overflows. Accessing the guard page triggers a page fault that the kernel cannot handle, resulting in a SIGSEGV (segmentation fault).

## Heap memory

When a program needs memory at run time, it uses the heap to allocate memory.
Heap memory starts right after the uninitialized data segment (BSS) and grows or shrinks as memory is allocated and freed. The current limit of the heap is referred to as the `program break`.

Allocating and deallocating heap memory is done by adjusting the program break. When no heap memory is allocated, the program break points to the same location as `extern char *end` (the end of the BSS segment).

After the program break is increased, the process can access addresses in the newly allocated area, but no physical memory pages are created immediately. The kernel will automatically allocate physical pages on the first attempt by the process to access those addresses (this is called demand paging). It can be verified using `cat /proc/self/status | grep Vm`, after dynamic memory allocation `VmSize` will be increased, But `VmRSS` will not increase until you actually write to the memory

```
Field	Meaning
VmPeak	Peak virtual memory size (maximum VmSize reached) (The maximum amount of virtual memory this process has used at any time)
VmSize	Current total virtual memory size (all mapped regions) (This is the current total size of the process's virtual memory.)
VmLck	Locked memory size (pages locked in RAM, not swappable)
VmPin	Pinned memory size (pages that can't be moved, e.g., for DMA)
VmHWM	Peak resident set size ("high water mark" of VmRSS) (The maximum value of VmRSS (resident set size) that your process has ever reached during its lifetime.)
VmRSS	Resident Set Size (actual physical memory currently in RAM)
VmData	Size of data segment (heap + uninitialized data, i.e., malloc/brk/sbrk)
VmStk	Stack size
VmExe	Size of text segment (executable code)
VmLib	Shared library code size
VmPTE	Page Table entries size
VmSwap	Swapped-out virtual memory size
```

system calls used to adjust the program breaks are `brk(void *end_data_seg)` and `sbrk(intptr_t increment)`

brk -> Sets the program break to location specified by end_data_seg, Since virtual memory are allocated as pages, end_data_seg is effectively rounded up to the next boundry

sbrk -> Moves the program break by a specified increment. sbrk(0) returns the current location of the program break.
sbrk(0) will return current location of program break

To know more about mapping region levels `cat /proc/self/smaps`
```
Size 
    Total size of the virtual memory region (not necessarily resident).
    Always a multiple of page size (typically 4 KB).
    Includes any extra padding due to page alignment.

KernelPageSize
    Actual software page size used

MMUPageSize
    Actual hardware page size used

Rss (Resident Set Size)
    Amount of this region currently in physical RAM.
    Pages not accessed yet or swapped out will not be counted here.

Pss (Proportional Set Size)
    Private memory + (Shared memory / number of sharing processes)

Pss_Dirty
    Proportional part of dirty pages (pages modified) from this region.

Shared_Clean / Shared_Dirty
    Memory shared with other processes.
    Clean = not modified since loaded.
    Dirty = modified.

Private_Clean / Private_Dirty
    Private memory — only this process uses it.
    Clean = not modified since loaded.
    Dirty = modified (important for things like Copy-On-Write).

Referenced
    How much of this memory has been recently accessed (read or written).
    The kernel clears this periodically to track active pages.

Anonymous
    Memory not backed by a file — like heap, stack, or MAP_ANONYMOUS.

KSM (Kernel Samepage Merging)
    Pages that were merged using Kernel Samepage Merging (KSM).
    Mainly for VMs or deduplicated pages.

LazyFree
    Pages marked with MADV_FREE — kernel can reclaim if under memory pressure.
    Used by applications like jemalloc.

AnonHugePages
    Portion of anonymous memory backed by huge pages (e.g., 2MB pages).
    Speeds up access and reduces TLB pressure.

ShmemPmdMapped / FilePmdMapped
    Amount of shared memory or file-backed memory using PMD-sized huge pages (2MB).

Shared_Hugetlb / Private_Hugetlb
    Pages using the explicit hugetlbfs interface (very large pages).
    Requires special kernel config & mount.

Swap / SwapPss
    Swap = how much of this region is currently swapped out.
    SwapPss = proportional share (if shared) of swapped memory.

Locked
    Memory locked into RAM using mlock() or similar.
    Not swappable.

THPeligible
    Indicates whether Transparent Huge Pages are allowed.
    1 = eligible, 0 = not eligible.

ProtectionKey
    Memory protection key assigned (used in systems with Intel PKU).

VmFlags

    | Flag | Meaning                                                |
    | ---- | ------------------------------------------------------ |
    | `rd` | Readable                                               |
    | `wr` | Writable                                               |
    | `ex` | Executable                                             |
    | `mr` | May read                                               |
    | `mw` | May write                                              |
    | `me` | May execute                                            |
    | `ac` | Accounted (included in RSS)                            |
    | `sd` | Soft-dirty (page was written to since last checkpoint) |
    | `sh` | Shared mapping                                         |
    | `pr` | Private mapping                                        |
    | `dd` | Don't dump (excluded from core dumps)                  |
    | `mm` | Mixed map (shared/private mix)                         |


```

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

### Program
[lazy_alloc](lazy_alloc.c)

## mmap

`mmap` system call creates a new memory mapping in the calling process's virtual address space.
The mapping can be two types
* **File Mapping or memory mapped file** :
    Maps a region of file directly into the process virtual address. File content is accessed in the byte order in corresponding to the memory region.  

* **Anonymous mapping**
    It doesn't have a file to map, instead the pages are initilized to 0

Mappings can be **private** or **shared**:

* **shared(`MAP_SHARED`)** :
    The mapped memory can be shared between processes. The page table entries of each process point to the same physical pages in RAM. Changes made by one process are visible to others.

* **parivate(`MAP_PRIVATE`)**:
    Modifications are not visible to the other process, the operations are not carried to the underlying files when it is in private

When a process calls fork(), both the parent and child initially share the same memory region if it is MAP_PRIVATE. If either process modifies the memory, a new private copy is created for that process (copy-on-write).  Private region is some times called as `copy-on-write mapping`


There are four types of mapping can be created
1. **private file mapping**:

    The content of the mapping initilized from a file, multiple process mapping the sane file initially share the same physical pages, later copy-on-write is employed and changes are not visible to other process
    [private_file_mapping](./mmap/private_file_mapping.c)

2. **Private anonymous mapping**:

    It creates a distant memory for the process, when subprocess is created using `fork` copy-on-write ensure's changes made by one process is not visible to other. It's conent are initilized to 0
    [mmap](./mmap/mmap.c)

3. **shared file mapping**:

    All process mapped the same region of a file share the same physical pages of memory. Modifications are carried through the file.
    It's uses 
    - memory-mapped IO (File is loaded into a region of the process virtual memory and modifications are automatically written to the file). It is the alternative for `read` and `write`
    - Inter Process Communication for two unrelated process
    [shared_file_mapping.c](./mmap/shared_file_mapping.c)

4. **shared anonymous mapping**:

    It creates a distant memory, when a child process is created using fork it will not employ copy-on-write. The changes made by one process is visible to other
    It's used
    -Inter Process Communication for two related process 
    [shared_anon_mapping.c](./mmap/shared_anon_mapping.c)

> [!TIP]  
> 1. Private anonymous pages: Kernel eagerly allocates zero pages on first access  
> 2. Shared anonymous pages: Kernel delays allocation until actual data write  

### Memory protection for mmap
These are the page protection flags
`PORT_NONE` - The resgion is not accessed.  
              This is used for page gaurd, like before and after of stack. if user trying to access, it cause violation
`PORT_READ` - The content of region can be read
`PORT_WRITE`- The content of region can be modified
`PORT_EXEC` - The content of region can be executed

If write to read only region or, execution of write and read region or access of PORT_NONE,.. cause violation. It raise `SIGSEGV` signal, In some linux `SIGBUS` is used

The memory protection reside in process-private virtual memory tables. Thus different process may map the same region with different protections.  
Memory protection can be changed using `mprotect()`

### munmap
`munmap` system call removing a mapping from the calling process virtual address space.
During munmap, kernel removes any memory locks that the process holds for specified address range.
All the process mapping are unmapped when process terminates or performs an `exec`

To ensure the cotent of the shared file mapping are written to the underlying file, a call to `msync()` should be made before munmap


When a process executes `exec()` mapping are lost

// todo
virtual address supposed to be same at all times of same program execution?
i have written to shared and private, why private's dirty only 8kb not the shared one?


📅 Week 2: mmap(), mprotect(), and File Mapping
| Focus                        | Topics Covered                           |
| ---------------------------- | ---------------------------------------- |
| ✅ mmap()                     | Anonymous, file-backed, shared/private   |
munmap()
| ✅ mprotect()                 | Set RWX on memory    (MAP_PRIVATE, MAP_SHARED, etc.)                     |
| ✅ Access violation & SIGSEGV | Trigger, handle, debug                   |
| ✅ Page fault theory          | Demand paging, page faults               |
| ✅ Tools                      | `strace`, `objdump -h`, `/proc/PID/maps` |
Mini Projects

Map a text file to memory with mmap(), read and modify

Modify it with MAP_SHARED and observe persistence

Show copy-on-write with MAP_PRIVATE

Show mprotect() usage and trigger SIGSEGV

📅 Week 3: ELF Internals & Executable Memory Mapping
| Focus                        | Topics Covered                                             |
| ---------------------------- | ---------------------------------------------------------- |
| ✅ ELF binary layout          | Sections vs Segments (`.text`, `.data`, `.bss`, `.rodata`) |
| ✅ Use `readelf`, `objdump`   | Program headers (for loader), section headers (for linker) |
| ✅ ELF → Memory mapping       | How ELF segments are mapped into memory via `mmap()`       |
| ✅ Linker/Loader basics       | Dynamic linker, PT\_INTERP, ld.so, GOT/PLT                 |
| ✅ Static vs dynamic binaries | Analyze with `ldd`, `readelf -d`, `objdump -R`             |
Mini Projects

Load a small ELF manually using mmap() (read-only)

Trace program startup using strace and LD_DEBUG

Compare static vs dynamic executable size & startup

📅 Week 4: Advanced Topics: Demand Paging, Lazy Mapping, Real-Time SIGSEGV Handling
| Focus                         | Topics Covered                                              |
| ----------------------------- | ----------------------------------------------------------- |
| ✅ Demand mapping via SIGSEGV  | Setup `sigaction()` with `SA_SIGINFO`                       |
| ✅ Manual memory loading       | Allocate empty region, `mprotect`, on SIGSEGV load contents |
| ✅ Reentrant-safe signal usage | Handler safety, `sig_atomic_t`, async-signal-safe functions |
| ✅ Shared memory (intro only)  | Use of `mmap()` with `MAP_SHARED`, not `shmget` yet         |

Mini Projects

Custom loader: Allocate memory and load text file on-demand when SIGSEGV occurs

Dynamically grow stack on access

Watch memory behavior with /proc/self/smaps, and memory pressure with vmstat

 Bonus / Optional (Add if time remains):
🔸 brk() vs mmap() memory allocation

🔸 Heap growth via sbrk() — legacy but useful to understand

🔸 Implement mini malloc with mmap()

🔸 Read /proc/PID/smaps and estimate RSS, shared/private memory

🔸 Understand huge pages (THP)

🧩 1. Process Memory Layout


Identify regions: file-backed vs anonymous

What is ELF’s role in memory mapping


🔍 3. mmap() Internals
What mmap() does at the system level:

File-backed memory mapping (e.g. .so, images)

Anonymous memory mapping (heap-like regions)

Flags & Protections:

MAP_SHARED vs MAP_PRIVATE (COW behavior)

PROT_READ, PROT_WRITE, PROT_EXEC, PROT_NONE

Mapping partial files, aligned memory

munmap() to free memory manually

🧪 4. Memory Protection (mprotect)
How to mark memory regions read-only or inaccessible

Catching segmentation faults with SIGSEGV

Guard pages, memory corruption protection

Security: exploit mitigation via memory protection

🔁 5. Copy-on-Write Behavior
How fork() and mmap(MAP_PRIVATE) use COW

Detecting when a page gets copied (via strace, /proc)

Use cases: performance optimization, snapshotting

⚠️ 6. Segmentation Fault Debugging

Accessing read-only or unmapped memory

Recognize common memory corruption symptoms

🧬 7. Advanced Concepts
Memory-mapped I/O (e.g. /dev/mem)

Allocating large pages (MAP_HUGETLB)

Locking memory in RAM (mlock())

Creating executable memory regions (JIT)

🛠 8. Practical Debugging Tools
strace, pmap, vmstat, top, valgrind

Use perf or page-fault counters

Understand how memory leaks or fragmentation happen

Address Space Layout Randomization (ASLR) 