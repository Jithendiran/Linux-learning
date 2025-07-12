#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

int main() {
    pid_t pid = getpid();
    char cmdmaps[128], cmdsmaps[256];
    sprintf(cmdmaps, "cat /proc/%d/maps", pid);

    // 1. Open file for reading and writing
    int fd = open("test_shared.txt", O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // 2. Write initial content to file
    const char *initial = "This is a shared mmap demo.\n";
    write(fd, initial, strlen(initial));

    // 3. Map file using MAP_SHARED
    size_t length = 4096;
    char *addr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Print initial content from mmap
    printf("\n[BEFORE WRITE]\nMapped content: %.30s\n", addr);

    // Prepare smaps command
    snprintf(cmdsmaps, sizeof(cmdsmaps),
             "awk '/^%lx-/{flag=1} flag && /^[0-9a-f]+-[0-9a-f]+/ && !/^%lx-/ {flag=0} flag' /proc/%d/smaps",
             (unsigned long)addr, (unsigned long)addr, pid);
    system(cmdsmaps);

    // 4. Modify mapped region
    strcpy(addr, "Updated via shared mmap!");

    // 5. msync to ensure changes go to disk
    msync(addr, length, MS_SYNC);

    // 6. Read file to verify update
    lseek(fd, 0, SEEK_SET);
    char buf[100] = {0};
    read(fd, buf, sizeof(buf) - 1);
    printf("\n[AFTER WRITE]\nFile content: %s\n", buf);

    // Print again from mmap
    printf("Mapped content: %s\n", addr);
    system(cmdsmaps);

    // Cleanup
    munmap(addr, length);
    close(fd);

    return 0;
}

/*
[BEFORE WRITE]
Mapped content: This is a shared mmap demo.

71983918d000-71983918e000 rw-s 00000000 08:00 52986919                   /media/ssd/Project/Linux-learning/test_shared.txt
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

[AFTER WRITE]
File content: Updated via shared mmap!
Mapped content: Updated via shared mmap!
71983918d000-71983918e000 rw-s 00000000 08:00 52986919                   /media/ssd/Project/Linux-learning/test_shared.txt
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
VmFlags: rd wr sh mr mw me ms sd 
*/