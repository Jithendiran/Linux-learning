// echo "Original FILE content!!!!!!!" > /tmp/test.txt && private_file_mapping.c
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

int main() {
    pid_t pid = getpid();
    char cmdmaps[120], cmdsmaps[256];
    sprintf(cmdmaps, "cat /proc/%d/maps", pid);

    int fd = open("/tmp/test.txt", O_RDWR);  // Open for reading & writing
    if (fd < 0) { perror("open"); exit(1); }

    // Create mmap with PROT_READ | PROT_WRITE
    char *data = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) { perror("mmap"); exit(1); }

    // Create smaps viewer
    snprintf(cmdsmaps, sizeof(cmdsmaps),
             "awk '/^%lx-/{flag=1} flag && /^[0-9a-f]+-[0-9a-f]+/ && !/^%lx-/ {flag=0} flag' /proc/%d/smaps",
             (unsigned long)data, (unsigned long)data, pid);

    printf("\n--- BEFORE WRITE ---\n");
    printf("Mapped content: %s\n", data); // Mapped content: Original FILE content
    system(cmdsmaps);

    // Write into private mapping
    strcpy(data, "Hello PRIVATE MAPPING!");

    printf("\n--- AFTER WRITE ---\n");
    printf("Mapped content after write: %s\n", data);       // Mapped content after write: Hello PRIVATE MAPPING!

    // Read from original file to verify it's unchanged
    char buf[64] = {0};
    lseek(fd, 0, SEEK_SET);
    read(fd, buf, 30);
    printf("File content after mmap write: %s\n", buf);    // File content after mmap write: Original FILE content

    system(cmdsmaps);  // Check if flags changed (they won't!)

    munmap(data, 4096);
    close(fd);
    return 0;
}

/*
--- BEFORE WRITE ---
Mapped content: Original FILE content

7ffff7ffa000-7ffff7ffb000 rw-p 00000000 103:02 262195                    /tmp/test.txt
Size:                  4 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
Rss:                   4 kB
Pss:                   4 kB
Pss_Dirty:             0 kB
Shared_Clean:          0 kB
Shared_Dirty:          0 kB
Private_Clean:         4 kB
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
VmFlags: rd wr mr mw me ac sd 

--- AFTER WRITE ---
Mapped content after write: Hello PRIVATE MAPPING!
File content after mmap write: Original FILE content

7ffff7ffa000-7ffff7ffb000 rw-p 00000000 103:02 262195                    /tmp/test.txt
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
*/