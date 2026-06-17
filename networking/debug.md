### Kernel config
```bash
cat <<EOF >> .config
CONFIG_64BIT=y
CONFIG_BLK_DEV_INITRD=y
CONFIG_BINFMT_ELF=y
CONFIG_TTY=y
CONFIG_SERIAL_8250=y
CONFIG_SERIAL_8250_CONSOLE=y
CONFIG_DEBUG_KERNEL=y
CONFIG_DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT=y
CONFIG_GDB_SCRIPTS=y
CONFIG_RANDOMIZE_BASE=n
CONFIG_CC_OPTIMIZE_FOR_PERFORMANCE=n
CONFIG_CC_OPTIMIZE_FOR_DEBUGGING=y
CONFIG_NET=y           # Turns on the core Linux Networking Subsystem
CONFIG_UNIX=y          # Enables AF_UNIX (Local intercom sockets)
CONFIG_INET=y          # Enables AF_INET (Internet / TCP/IP sockets)
EOF
```

### init program

```c
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {
    printf("=== Starting Tiny Socket Test ===\n");

    // 1. Create a local intercom socket
    // Domain: AF_UNIX (local communication)
    // Type: SOCK_STREAM (reliable connection, like TCP)
    // Protocol: 0 (let the kernel pick the default)
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd < 0) {
        printf("ERROR: Socket creation failed! Did you enable CONFIG_NET=y?\n");
    } else {
        printf("SUCCESS: Socket created! File Descriptor number is: %d\n", sock_fd);
        
        // Keep the program alive so you can inspect things in GDB
        printf("Sitting in a loop. Ready for GDB tracing...\n");
        close(sock_fd);
    }

    while(1) {
        sleep(1000);
    }
    return 0;
}
```
```bash
gcc -static /tmp/kernel/init.c -o /tmp/kernel/rootfs/init
```