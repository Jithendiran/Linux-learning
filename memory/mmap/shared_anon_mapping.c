#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>
#include <fcntl.h>

/*
Check the analysis part in down, it is more important than understanding it's program output
*/

// Reads the physical page frame number for a virtual address
unsigned long get_phys_page(void *vaddr)
{
    uint64_t entry;
    size_t pagesize = sysconf(_SC_PAGESIZE);
    off_t offset = ((unsigned long)vaddr / pagesize) * sizeof(uint64_t);

    int fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd < 0)
    {
        perror("open pagemap");
        return 0;
    }

    if (lseek(fd, offset, SEEK_SET) == (off_t)-1)
    {
        perror("lseek");
        close(fd);
        return 0;
    }

    if (read(fd, &entry, sizeof(entry)) != sizeof(entry))
    {
        perror("read");
        close(fd);
        return 0;
    }
    
    printf("Entry\t\t\t:\t0x%lx\n", entry);
    printf("  Present bit (63): %s\n", (entry & (1ULL << 63)) ? "Yes" : "No");
    printf("  Swapped bit (62): %s\n", (entry & (1ULL << 62)) ? "Yes" : "No");
    printf("  File-mapped (61): %s\n", (entry & (1ULL << 61)) ? "Yes" : "No");
    
    close(fd);

    if (!(entry & (1ULL << 63))) // page not present
        return 0;

    // Check if this looks like a restricted/special page
    if ((entry & 0xFF00000000000000ULL) == 0x8100000000000000ULL) {
        printf("  WARNING: This looks like a restricted page or insufficient permissions\n");
        printf("  Try running with sudo or check kernel restrictions\n");
    }

    // Extract PFN (bits 0-54 on x86_64)
    unsigned long pfn = entry & 0x7FFFFFFFFFFFFFULL;
    if (pfn == 0) {
        printf("  PFN is zero - likely permission issue or special page\n");
        return 0;
    }
    
    unsigned long phys_addr = (pfn * pagesize) + ((unsigned long)vaddr % pagesize);
    printf("  PFN: 0x%lx\n", pfn);
    printf("Physical address = 0x%lx\n", phys_addr);
    return phys_addr;
}

void print_smaps_region(const char *who, void *addr)
{
    char cmd[256];
    printf("\n[%s] ------ /proc/%d/smaps for address: %p ------\n", who, getpid(), addr);
    snprintf(cmd, sizeof(cmd),
             "awk '/^%lx-/{flag=1} flag && /^[0-9a-f]+-[0-9a-f]+/ && !/^%lx-/ {flag=0} flag' /proc/%d/smaps",
             (unsigned long)addr, (unsigned long)addr, getpid());
    system(cmd);
    printf("------------------------------------------------------------\n\n");

    unsigned long phys = get_phys_page(addr);
    if (phys)
        printf("[%s] Physical address for %p = 0x%lx\n", who, addr, phys);
    else
        printf("[%s] Page not in RAM for %p\n", who, addr);
}

/*
MAP_PRIVATE uses copy-on-write (COW).
Before write, parent and child share physical memory.
After write, kernel makes a new physical page for the writing process.
*/

int main()
{
    size_t length = 4096;

    // Create shared anonymous memory
    char *shared_mem = mmap(NULL, length, PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_mem == MAP_FAILED)
    {
        perror("mmap shared");
        exit(1);
    }

    // Create private anonymous memory
    char *private_mem = mmap(NULL, length, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (private_mem == MAP_FAILED)
    {
        perror("mmap private");
        exit(1);
    }

    printf("=== Parent Before Write ===\n");
    print_smaps_region("Parent - Shared", shared_mem);
    print_smaps_region("Parent - Private", private_mem);

    // Initial writes
    strcpy(shared_mem, "Hello from parent (shared)");
    strcpy(private_mem, "Hello from parent (private)");

    printf("=== Parent Before fork ===\n");
    print_smaps_region("Parent - Shared", shared_mem);
    print_smaps_region("Parent - Private", private_mem);

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        munmap(shared_mem, length);
        munmap(private_mem, length);
        exit(1);
    }

    if (pid == 0)
    {
        sleep(1);
        printf("=== Child Before Write ===\n");
        print_smaps_region("Child - Shared", shared_mem);
        print_smaps_region("Child - Private", private_mem);

        printf("Child: Writing to both mappings...\n");
        strcpy(shared_mem, "Modified by child (shared)");
        strcpy(private_mem, "Modified by child (private)");

        printf("=== Child After Write ===\n");
        print_smaps_region("Child - Shared", shared_mem);
        print_smaps_region("Child - Private", private_mem);

        exit(0);
    }
    else
    {
        wait(NULL);
        printf("=== Parent After child ===\n");
        print_smaps_region("Parent - Shared", shared_mem);
        print_smaps_region("Parent - Private", private_mem);

        printf("Parent sees shared mmap as: %s\n", shared_mem);
        printf("Parent sees private mmap as: %s\n", private_mem);
    }

    munmap(shared_mem, length);
    munmap(private_mem, length);
    return 0;
}


/*
Run as sudo

program output:
=== Parent Before Write ===

[Parent - Shared] ------ /proc/6556/smaps for address: 0x7b2adc763000 ------
7b2adc763000-7b2adc764000 rw-s 00000000 00:01 2140                       /dev/zero (deleted)
Size:                  4 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   0 kB
Pss:                   0 kB
Pss_Dirty:             0 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         0 kB
Referenced:            0 kB
Anonymous:             0 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr sh mr mw me ms sd 
------------------------------------------------------------

Entry                   :       0x80000000000000
  Present bit (63): No
  Swapped bit (62): No
  File-mapped (61): No
[Parent - Shared] Page not in RAM for 0x7b2adc763000

[Parent - Private] ------ /proc/6556/smaps for address: 0x7b2adc729000 ------
7b2adc729000-7b2adc72c000 rw-p 00000000 00:00 0 
Size:                 12 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   4 kB
Pss:                   4 kB
Pss_Dirty:             4 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         4 kB
Referenced:            4 kB
Anonymous:             4 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr mr mw me ac sd 
------------------------------------------------------------

Entry                   :       0x80000000000000
  Present bit (63): No
  Swapped bit (62): No
  File-mapped (61): No
[Parent - Private] Page not in RAM for 0x7b2adc729000

//-----------------------------------------------------------------------------------------------------------
Why private's PSS, RSS are set, not for shared

For private memory
When get_phys_page() calls read(fd, &entry, sizeof(entry)), it's reading from the pagemap file, 
but to calculate the offset, it needs to access private_mem address. 
More importantly, when your function tries to access the virtual address to read the pagemap entry,
it triggers a page fault.

The sequence is:

mmap() creates the VMA (Virtual Memory Area) but no physical pages allocated yet
get_phys_page() accesses the virtual address (even indirectly through pagemap reading)
Page fault occurs → kernel allocates a zero-filled page
Now RSS/PSS/Private_Dirty show 4KB

For Shared Memory
With shared anonymous memory, the kernel is more conservative. 
It doesn't immediately allocate physical pages even on access for reading pagemap.
The page fault will only occur when you actually write to the memo

1. Private anonymous pages: Kernel eagerly allocates zero pages on first access
2. Shared anonymous pages: Kernel delays allocation until actual data write

//-----------------------------------------------------------------------------------------------------------

=== Parent Before fork ===

[Parent - Shared] ------ /proc/6556/smaps for address: 0x7b2adc763000 ------
7b2adc763000-7b2adc764000 rw-s 00000000 00:01 2140                       /dev/zero (deleted)
Size:                  4 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   4 kB
Pss:                   4 kB
Pss_Dirty:             4 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         4 kB             // even it is shared, it is tracking for process wise
Referenced:            4 kB
Anonymous:             0 kB             // this is /dev/zero backed so not an annonyms
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr sh mr mw me ms sd 
------------------------------------------------------------

Entry                   :       0xa180000000327169
  Present bit (63): Yes
  Swapped bit (62): No
  File-mapped (61): Yes
  PFN: 0x327169
Physical address = 0x327169000
[Parent - Shared] Physical address for 0x7b2adc763000 = 0x327169000

[Parent - Private] ------ /proc/6556/smaps for address: 0x7b2adc729000 ------
7b2adc729000-7b2adc72c000 rw-p 00000000 00:00 0 
Size:                 12 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   8 kB             // why 8 ?
Pss:                   8 kB
Pss_Dirty:             8 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         8 kB
Referenced:            8 kB
Anonymous:             8 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr mr mw me ac sd 
------------------------------------------------------------

Entry                   :       0x81800000002a0b6e
  Present bit (63): Yes
  Swapped bit (62): No
  File-mapped (61): No
  WARNING: This looks like a restricted page or insufficient permissions
  Try running with sudo or check kernel restrictions
  PFN: 0x2a0b6e
Physical address = 0x2a0b6e000
[Parent - Private] Physical address for 0x7b2adc729000 = 0x2a0b6e000
=== Child Before Write ===

[Child - Shared] ------ /proc/6565/smaps for address: 0x7b2adc763000 ------
7b2adc763000-7b2adc764000 rw-s 00000000 00:01 2140                       /dev/zero (deleted)
Size:                  4 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   0 kB
Pss:                   0 kB
Pss_Dirty:             0 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         0 kB
Referenced:            0 kB
Anonymous:             0 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr sh mr mw me ms sd 
------------------------------------------------------------

Entry                   :       0x80000000000000
  Present bit (63): No
  Swapped bit (62): No
  File-mapped (61): No
[Child - Shared] Page not in RAM for 0x7b2adc763000

[Child - Private] ------ /proc/6565/smaps for address: 0x7b2adc729000 ------
7b2adc729000-7b2adc72c000 rw-p 00000000 00:00 0 
Size:                 12 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   8 kB                     
Pss:                   4 kB
Pss_Dirty:             4 kB
Shared_Clean:          0 kB
Shared_Dirty:          8 kB       
Private_Clean:         0 kB
Private_Dirty:         0 kB
Referenced:            0 kB
Anonymous:             8 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr mr mw me ac sd 
------------------------------------------------------------

Entry                   :       0x80800000002a0b6e
  Present bit (63): Yes
  Swapped bit (62): No
  File-mapped (61): No
  PFN: 0x2a0b6e
Physical address = 0x2a0b6e000
[Child - Private] Physical address for 0x7b2adc729000 = 0x2a0b6e000
Child: Writing to both mappings...
=== Child After Write ===

[Child - Shared] ------ /proc/6565/smaps for address: 0x7b2adc763000 ------
7b2adc763000-7b2adc764000 rw-s 00000000 00:01 2140                       /dev/zero (deleted)
Size:                  4 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   4 kB
Pss:                   2 kB
Pss_Dirty:             2 kB
Shared_Clean:          0 kB
Shared_Dirty:          4 kB
Private_Clean:         0 kB
Private_Dirty:         0 kB
Referenced:            4 kB
Anonymous:             0 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr sh mr mw me ms sd 
------------------------------------------------------------

Entry                   :       0xa080000000327169
  Present bit (63): Yes
  Swapped bit (62): No
  File-mapped (61): Yes
  PFN: 0x327169
Physical address = 0x327169000
[Child - Shared] Physical address for 0x7b2adc763000 = 0x327169000

[Child - Private] ------ /proc/6565/smaps for address: 0x7b2adc729000 ------
7b2adc729000-7b2adc72c000 rw-p 00000000 00:00 0 
Size:                 12 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   8 kB
Pss:                   6 kB
Pss_Dirty:             6 kB
Shared_Clean:          0 kB
Shared_Dirty:          4 kB
Private_Clean:         0 kB
Private_Dirty:         4 kB
Referenced:            4 kB
Anonymous:             8 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr mr mw me ac sd 
------------------------------------------------------------

Entry                   :       0x8180000000307b1e
  Present bit (63): Yes
  Swapped bit (62): No
  File-mapped (61): No
  WARNING: This looks like a restricted page or insufficient permissions
  Try running with sudo or check kernel restrictions
  PFN: 0x307b1e
Physical address = 0x307b1e000
[Child - Private] Physical address for 0x7b2adc729000 = 0x307b1e000
=== Parent After child ===

[Parent - Shared] ------ /proc/6556/smaps for address: 0x7b2adc763000 ------
7b2adc763000-7b2adc764000 rw-s 00000000 00:01 2140                       /dev/zero (deleted)
Size:                  4 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   4 kB
Pss:                   4 kB
Pss_Dirty:             4 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         4 kB
Referenced:            4 kB
Anonymous:             0 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr sh mr mw me ms sd 
------------------------------------------------------------

Entry                   :       0xa180000000327169
  Present bit (63): Yes
  Swapped bit (62): No
  File-mapped (61): Yes
  PFN: 0x327169
Physical address = 0x327169000
[Parent - Shared] Physical address for 0x7b2adc763000 = 0x327169000

[Parent - Private] ------ /proc/6556/smaps for address: 0x7b2adc729000 ------
7b2adc729000-7b2adc72c000 rw-p 00000000 00:00 0 
Size:                 12 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   8 kB
Pss:                   8 kB
Pss_Dirty:             8 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         8 kB
Referenced:            8 kB
Anonymous:             8 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
THPeligible:           0
ProtectionKey:         0
VmFlags: rd wr mr mw me ac sd 
------------------------------------------------------------

Entry                   :       0x81800000002a0b6e
  Present bit (63): Yes
  Swapped bit (62): No
  File-mapped (61): No
  WARNING: This looks like a restricted page or insufficient permissions
  Try running with sudo or check kernel restrictions
  PFN: 0x2a0b6e
Physical address = 0x2a0b6e000
[Parent - Private] Physical address for 0x7b2adc729000 = 0x2a0b6e000
Parent sees shared mmap as: Modified by child (shared)
Parent sees private mmap as: Hello from parent (private)
*/

//--------------------------------------------------------------------------------------------------------------------------------------------------

// Analysis section

/*
The child process shares the same physical memory pages as the parent (even for MAP_PRIVATE), using Copy-On-Write (COW).
Meaning: both parent and child point to same physical memory pages until either one writes to a page.

What triggers a real copy?
A write to that page in either process.
Then kernel allocates a new page, copies data from the old one, and assigns it to the writing process.
This is a page-level operation — only the page that is written gets copied, not the entire region.

*/

/*
Analysis (COW in action)

sh-shared memory
pri - private memory

v - virtual address
p - private address

=== Parent Before Write ===
sh
v - 0x7b2adc763000
p - 0

pri
v - 0x7b2adc729000
p - 0
---------------------------------------- write done
=== Parent Before fork ===
sh
v - 0x7b2adc763000
p - 0x327169000

pri
v - 0x7b2adc729000
p - 0x2a0b6e000

=== Child Before Write ===
sh
v - 0x7b2adc763000
p - 0                   // why 0? it supposed to be 0x327169000?

                        // This is temporary due to how kernel handles memory mapping visibility:
                        // When you fork(), Linux uses lazy copying and page population.
                        // Child may not yet have page table entry for the shared mapping until it accesses it.
                        // Until then, /proc/<pid>/pagemap shows 0 (no PFN).
                        // Accessing the address in child will trigger the page fault handler, which populates the PTE and page table — without copying (because MAP_SHARED).

pri
v - 0x7b2adc729000
p - 0x2a0b6e000        // before write same physical page, expected

=== Child After Write ===
sh
v - 0x7b2adc763000
p - 0x327169000         // same physical address as child

pri
v - 0x7b2adc729000
p - 0x307b1e000       // due to private, different address (COW)

=== Parent After child ===
sh 
v - 0x7b2adc763000
p - 0x327169000

pri
v - 0x7b2adc729000
p - 0x2a0b6e000         // back to parent physical address
*/