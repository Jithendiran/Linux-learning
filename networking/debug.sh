#!/bin/bash

# Exit immediately if any command fails
set -e

# Define variables
WORKSPACE="/tmp/kernel"
KERNEL_VER="6.1.60"
KERNEL_DIR="${WORKSPACE}/linux-6.1.60"
TAR_BALL="linux-${KERNEL_VER}.tar.xz"

echo "=== Step 1: Preparing Workspace and Directories ==="
mkdir -p "${WORKSPACE}/rootfs"

echo "=== Step 2: Creating the Custom Init Program ==="
cat << 'EOF' > "${WORKSPACE}/init.c"
// #include <stdio.h>
// #include <sys/socket.h>
// #include <sys/un.h>
// #include <unistd.h>

// int main() {
//     printf("=== Starting Tiny Socket Test ===\n");

//     // 1. Create a local intercom socket
//     // Domain: AF_UNIX (local communication)
//     // Type: SOCK_STREAM (reliable connection, like TCP)
//     // Protocol: 0 (let the kernel pick the default)
//     int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

//     if (sock_fd < 0) {
//         printf("ERROR: Socket creation failed! Did you enable CONFIG_NET=y?\n");
//     } else {
//         printf("SUCCESS: Socket created! File Descriptor number is: %d\n", sock_fd);
        
//         // Keep the program alive so you can inspect things in GDB
//         printf("Sitting in a loop. Ready for GDB tracing...\n");
//         close(sock_fd);
//     }

//     while(1) {
//         sleep(1000);
//     }
//     return 0;
// }


#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

int main() {
    printf("=== Starting Expanded Socket Test ===\n");

    int sv[2]; // sv[0] will be our "Client", sv[1] will be our "Server"
    char write_buf[] = "Hello from the other side!";
    char read_buf[64];

    // 1. Create a connected pair of local sockets
    // This handles creation and connection all at once for easy debugging.
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        printf("ERROR: Socket pair creation failed!\n");
        return 1;
    }

    printf("SUCCESS: Sockets created!\n");
    printf("Client FD: %d | Server FD: %d\n", sv[0], sv[1]);
    printf("Breakpoint hint: Set a breakpoint on 'write' or 'sys_write'\n\n");

    // 2. WRITE: Client sends data to the server
    printf("[Client] Sending message...\n");
    ssize_t bytes_sent = write(sv[0], write_buf, strlen(write_buf) + 1); // +1 to include null terminator
    if (bytes_sent < 0) {
        printf("ERROR: Write failed!\n");
    } else {
        printf("[Client] Successfully wrote %ld bytes.\n\n", bytes_sent);
    }

    // 3. READ: Server receives data from the client
    memset(read_buf, 0, sizeof(read_buf)); // Clear buffer
    printf("[Server] Attempting to read...\n");
    ssize_t bytes_read = read(sv[1], read_buf, sizeof(read_buf));
    if (bytes_read < 0) {
        printf("ERROR: Read failed!\n");
    } else {
        printf("[Server] Successfully read %ld bytes.\n", bytes_read);
        printf("[Server] Message received: \"%s\"\n\n", read_buf);
    }

    // 4. CLOSE: Clean up the file descriptors
    printf("Closing sockets...\n");
    close(sv[0]);
    close(sv[1]);
    printf("Sockets closed.\n\n");

    // Keep the program alive so you can inspect memory/state in GDB post-execution
    printf("Sitting in an infinite loop. Ready for post-mortem GDB tracing...\n");
    while(1) {
        sleep(1000);
    }

    return 0;
}
EOF

# Create a test file for init to read
echo "Hello, Linux!" > "${WORKSPACE}/rootfs/test.txt"

# Compile init statically
gcc -static "${WORKSPACE}/init.c" -o "${WORKSPACE}/rootfs/init"

echo "=== Step 3: Packing the Initramfs Archive ==="
cd "${WORKSPACE}/rootfs"
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz
cd "${WORKSPACE}"

echo "=== Step 4: Downloading and Extracting Linux Kernel ${KERNEL_VER} ==="
if [ ! -f "${TAR_BALL}" ]; then
    wget "https://cdn.kernel.org/pub/linux/kernel/v6.x/${TAR_BALL}"
fi

if [ ! -d "${KERNEL_DIR}" ]; then
    tar -xf "${TAR_BALL}"
fi

echo "=== Step 5: Configuring Minimal Kernel ==="
cd "${KERNEL_DIR}"
make allnoconfig

# Append debugging and minimal runtime configurations
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

# Ensure the config tool updates structural choices properly
./scripts/config --disable CONFIG_CC_OPTIMIZE_FOR_PERFORMANCE
./scripts/config --enable CONFIG_CC_OPTIMIZE_FOR_DEBUGGING
make olddefconfig

echo "=== Step 6: Compiling the Kernel (This may take a few minutes) ==="
make -j$(nproc)

echo "=== Step 7: Generating VS Code Debug Configuration ==="
mkdir -p "${KERNEL_DIR}/.vscode"
cat << 'EOF' > "${KERNEL_DIR}/.vscode/launch.json"
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Linux Kernel Live Debug (QEMU)",
            "type": "cppdbg",
            "request": "launch",
            "program": "/tmp/kernel/linux-6.1.60/vmlinux",
            "args": [],
            "stopAtEntry": false,
            "cwd": "/tmp/kernel/linux-6.1.60",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "/usr/bin/gdb",
            "miDebuggerServerAddress": "localhost:1234",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description": "Load Linux kernel helper scripts",
                    "text": "add-auto-load-safe-path /tmp/kernel/linux-6.1.60/vmlinux-gdb.py",
                    "ignoreFailures": true
                }
            ],
            "sourceFileMap": {
                "/tmp/kernel/linux-6.1.60": "${workspaceFolder}"
            }
        }
    ]
}
EOF

echo "===================================================="
echo " Setup complete! Here is how to run your environment:"
echo "===================================================="
echo "1. Run QEMU in your terminal:"
echo "   qemu-system-x86_64 -kernel ${KERNEL_DIR}/arch/x86/boot/bzImage -initrd ${WORKSPACE}/initramfs.cpio.gz -append \"console=ttyS0 init=/init panic=-1\" -nographic -s -S"
echo ""
echo "2. Debugging Options:"
echo "   - CLI GDB: Run 'gdb ${KERNEL_DIR}/vmlinux' and type 'target remote :1234'"
echo "   - VS Code: Open '${KERNEL_DIR}' folder in VS Code, go to the Run & Debug tab, and launch 'Linux Kernel Live Debug (QEMU)'."
echo "===================================================="
