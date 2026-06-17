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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    printf("Hello! I am the only program running on this computer.\n");

    char buffer[64];
    // Open a dummy file
    int fd = open("/test.txt", O_RDONLY);
    if (fd >= 0) {
        read(fd, buffer, 13);
        printf("Read from test.txt: %s\n", buffer);
        close(fd);
    }

    while(1) {
        sleep(1000); // Prevents CPU burnout and PID 1 from exiting
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
