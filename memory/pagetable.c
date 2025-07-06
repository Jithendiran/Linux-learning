#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int main() {
    printf("Pid : %d\n", getpid());
    // Allocate a page-aligned buffer
    size_t pagesize = sysconf(_SC_PAGESIZE);
    printf("Page size : %ld\n",pagesize); // Page size : 4096

    int buf = 'A';
    // Get virtual address

    unsigned long vaddr = (unsigned long)&buf;
    printf("Virtual address: 0x%lx\n", vaddr);

    // Open pagemap
    int fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd < 0) { perror("open pagemap"); return 1; }

    // Calculate offset in pagemap
    off_t offset = (vaddr / pagesize) * sizeof(uint64_t);
    printf("Offset in page table: %ld\n", offset);

    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) { 
        perror("lseek"); 
        return 1; 
    }

    // Read entry
    uint64_t entry;
    if (read(fd, &entry, sizeof(entry)) != sizeof(entry)) { 
        perror("read"); 
        return 1;
    }
    close(fd);
    printf("Entry\t\t\t:\t%lb\n", entry);

    // ULL -> unsigned long long (ULL), which is a 64-bit integer.
    // 1ULL << 63 -> Shift it left by 63 bits.
    printf("Left shift\t\t:\t%lb\n", (1ULL << 63)); 
    // like 1  0  0  0  0  0  0  ...
    //      63 62 61 60 59 58 57 ...
    // in hexa decimal 0x8000000000000000
    // It is little endian, The least significant byte is first (00), and the most significant byte (80) is last.

    // 64th bit is set if page present
    // Bits 0-54  page frame number (PFN) if present
    printf("Is page present\t\t:\t%lb\n", entry & (1ULL << 63));
    printf("1ULL << 55 is\t\t:\t%lb\n", (1ULL << 55));
    printf("Bitmask for pfn\t\t:\t%lb\n", (1ULL << 55) - 1); // -1 is used to set all the bits
    // Check if page is present
    if (entry & (1ULL << 63)) {
        unsigned long pfn = entry & ((1ULL << 55) - 1);
        printf("Physical frame number\t:\t%lb\n", pfn);
        /*
        pfn * pagesize
        Gives the base physical address of the page frame.

        vaddr % pagesize
        Gives the offset within the page (how far into the page your variable is).

        Add them together:
        This gives the exact physical address that the virtual address maps to.
        */
        printf("Physical address\t:\t%lb\n", (pfn * pagesize) + (vaddr % pagesize));
    } else {
        printf("Page not present in RAM\n");
    }
    return 0;
}
// gcc -g pagetable.c -o pagetable.o && sudo ./pagetable.o
/*
Pid : 7489
Page size : 4096
Virtual address: 0x7fff93e207e8
Offset in page table: 274874364160
Entry                   :       1000000110000000000000000000000000000000001001100111100100110001
Left shift              :       1000000000000000000000000000000000000000000000000000000000000000
Is page present         :       1000000000000000000000000000000000000000000000000000000000000000
1ULL << 55 is           :       10000000000000000000000000000000000000000000000000000000
Bitmask for pfn         :       1111111111111111111111111111111111111111111111111111111
Physical frame number   :       1001100111100100110001
Physical address        :       1001100111100100110001011111101000
*/