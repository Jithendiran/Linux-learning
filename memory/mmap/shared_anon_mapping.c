#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>
#include <fcntl.h>

// Reads the physical page frame number for a virtual address
unsigned long get_phys_page(void *vaddr)
{
    uint64_t entry;
    int page_size = getpagesize();
    uint64_t vpn = ((uint64_t)vaddr / page_size) * sizeof(uint64_t) ;

    int fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd < 0)
    {
        perror("open pagemap");
        return 0;
    }

    if (lseek(fd, vpn, SEEK_SET) == (off_t)-1)
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
    printf("Entry\t\t\t:\t%lb\n", entry);
    close(fd);

    if (!(entry & (1ULL << 63))) // page not present
        return 0;

    unsigned long pfn = entry & ((1ULL << 55) - 1);
    return (pfn * page_size) + ((unsigned long)vaddr % page_size);
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
=== Parent Before Write ===

[Parent - Shared] ------ /proc/4282/smaps for address: 0x711d56a93000 ------
711d56a93000-711d56a94000 rw-s 00000000 00:01 9252                       /dev/zero (deleted)
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


[Parent - Private] ------ /proc/4282/smaps for address: 0x711d56a59000 ------
711d56a59000-711d56a5c000 rw-p 00000000 00:00 0 
Size:                 12 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   4 kB         // why RSS before write?
Pss:                   4 kB         
Pss_Dirty:             4 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         4 kB         // why before write?
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

=== Parent Before fork ===

[Parent - Shared] ------ /proc/4282/smaps for address: 0x711d56a93000 ------
711d56a93000-711d56a94000 rw-s 00000000 00:01 9252                       /dev/zero (deleted)
Size:                  4 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   4 kB         // data written
Pss:                   4 kB
Pss_Dirty:             4 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         4 kB         // this is shared memory, why private is set
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


[Parent - Private] ------ /proc/4282/smaps for address: 0x711d56a59000 ------
711d56a59000-711d56a5c000 rw-p 00000000 00:00 0 
Size:                 12 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   8 kB         // 4->8
Pss:                   8 kB
Pss_Dirty:             8 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:         8 kB         // acceptable
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

=== Child Brfore write ===

[Child - Shared] ------ /proc/4292/smaps for address: 0x711d56a93000 ------
711d56a93000-711d56a94000 rw-s 00000000 00:01 9252                       /dev/zero (deleted)
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


[Child - Private] ------ /proc/4292/smaps for address: 0x711d56a59000 ------
711d56a59000-711d56a5c000 rw-p 00000000 00:00 0 
Size:                 12 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   8 kB         // how 8
Pss:                   4 kB
Pss_Dirty:             4 kB
Shared_Clean:          0 kB
Shared_Dirty:          8 kB         // this is private, how shared set?
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

Child: Writing to both mappings...
=== Child After Write ===

[Child - Shared] ------ /proc/4292/smaps for address: 0x711d56a93000 ------
711d56a93000-711d56a94000 rw-s 00000000 00:01 9252                       /dev/zero (deleted)
Size:                  4 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   4 kB
Pss:                   2 kB
Pss_Dirty:             2 kB
Shared_Clean:          0 kB
Shared_Dirty:          4 kB                 // acceptable
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


[Child - Private] ------ /proc/4292/smaps for address: 0x711d56a59000 ------
711d56a59000-711d56a5c000 rw-p 00000000 00:00 0 
Size:                 12 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   8 kB
Pss:                   6 kB
Pss_Dirty:             6 kB
Shared_Clean:          0 kB
Shared_Dirty:          4 kB             // why?
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

=== Parent After child ===

[Parent - Shared] ------ /proc/4282/smaps for address: 0x711d56a93000 ------
711d56a93000-711d56a94000 rw-s 00000000 00:01 9252                       /dev/zero (deleted)
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


[Parent - Private] ------ /proc/4282/smaps for address: 0x711d56a59000 ------
711d56a59000-711d56a5c000 rw-p 00000000 00:00 0 
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

Parent sees shared mmap as: Modified by child (shared)
Parent sees private mmap as: Hello from parent (private)
*/


/*
The child process shares the same physical memory pages as the parent (even for MAP_PRIVATE), using Copy-On-Write (COW).
Meaning: both parent and child point to same physical memory pages until either one writes to a page.

What triggers a real copy?
A write to that page in either process.
Then kernel allocates a new page, copies data from the old one, and assigns it to the writing process.
This is a page-level operation — only the page that is written gets copied, not the entire region.

*/