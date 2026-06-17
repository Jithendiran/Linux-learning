## Linux Kernel Live Debugging Environment Setup Guide
This guide provides a step-by-step procedure to set up, run, and debug a minimal Linux kernel using QEMU and GDB.

All commands and file operations use the absolute workspace path: `/tmp/kernel`.

### 1. Directory Layout
The workspace contains the following files and directories:
* `/tmp/kernel/init.c` — The user application source code.
* `/tmp/kernel/rootfs/` — The directory containing the target filesystem layout.
* `/tmp/kernel/linux-6.1.60/` — The compiled Linux kernel source directory.
* `/tmp/kernel/initramfs.cpio.gz` — The compressed RAM filesystem image generated from `rootfs` folder.

### 2. Understanding the Filesystem Archive Command
To create the runtime filesystem for the kernel, the following command must be executed from inside the `/tmp/kernel/rootfs` directory:

```bash
cd /tmp/kernel/rootfs
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz
```

**Technical Breakdown of the Pipeline**
1. `find . -print0`: Recursively lists all file and directory paths inside `rootfs`. The `-print0` flag separates each path name using a null byte character (`\0`) instead of a space or newline. This prevents filenames containing spaces from breaking subsequent steps. 
   * Output of this command alone is `../init./test.txt`, it will just list the files inside the folder
2. `cpio --null -ov --format=newc`: Reads the null-separated list of filenames from the previous step and concatenates the actual contents of those files into a single, continuous uncompressed archive format. The `--format=newc` option forces the use of the `SVR4` portable format headers.
3. `gzip -9`: Compresses the incoming uncompressed byte stream from `cpio` using the maximum compression level (`-9`) to minimize file size.
   * `> ../initramfs.cpio.gz`: Redirects the final compressed stream into a file named `initramfs.cpio.gz` inside the parent directory `/tmp/kernel`.
   * gzip is completely option
   * `find . -print0 | cpio --null -o --format=newc > ../initramfs.img`

#### Why This Command Is Required
Operating systems require a root filesystem (/) to load files and execute programs. Because the kernel in this environment is configured without hard disk controller drivers, it cannot read a standard physical hard drive partition. The kernel instead expects a memory-resident filesystem (initramfs) packed in the specific newc format. The kernel extracts this file directly into system RAM during the boot phase.


### 3. Step-by-Step Compilation and Setup
**Step A: Prepare the Program and Root Filesystem**
1. Change into the workspace directory: 
   ```bash
   cd /tmp/kernel
   ```
2. Create the folder structure `rootfs` inside :
   ```bash
    mkdir -p rootfs
    echo "Hello, Linux!" > rootfs/test.txt
    ```
3. Compile `init.c` as a static executable placed directly at the root of the target filesystem:
   ```c
    // init.c
    #include <stdio.h>
    #include <unistd.h>
    #include <fcntl.h>
    void main() {
        printf("Hello! I am the only program running on this computer.\n");

        char buffer[64];
        // Open a dummy file
        int fd = open("/test.txt", O_RDONLY);
        if (fd >= 0) {
            read(fd, buffer, 13);
            close(fd);
        }

        while(1) {
            sleep(1000); // Prevents CPU burnout and PID 1 from exiting
        }
    }
    ```
    ```bash
    gcc -static init.c -o rootfs/init
    ```
4. Build the compressed file archive using the command explained in Section 2:
   ```bash
    cd rootfs
    find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz
    cd ..
    ```
**Step B: Configure and Compile the Minimal Kernel**
1. Clone the kernel and extract it
   ```bash
   cd /tmp/kernel
   wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.1.60.tar.xz
   tar -xf linux-6.1.60.tar.xz
   cd /tmp/kernel/linux-6.1.60
   ```
2. Reset the kernel configuration to the absolute minimum:
   ```bash
   make allnoconfig
   ```
3. Add the required debugging options, RAM file support, and serial console output options to the configuration file:
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
    EOF
    ```
    ```bash
    ./scripts/config --disable CONFIG_CC_OPTIMIZE_FOR_PERFORMANCE
    ./scripts/config --enable CONFIG_CC_OPTIMIZE_FOR_DEBUGGING
    ```
4. Sanitize and apply the new configuration options:
   ```bash
   make olddefconfig
   ```
5. Compile the kernel using all available CPU threads:
   ```bash
   make -j$(nproc)
   ```
This build step produces two critical files:
* `/tmp/kernel/linux-6.1.60/arch/x86/boot/bzImage` — The compressed kernel image used by QEMU.
* `/tmp/kernel/linux-6.1.60/vmlinux` — The uncompressed kernel binary containing the full DWARF debugging symbol tables required by GDB.

### 4. Execution and GDB Connection
**Step A: Launch QEMU in Stopped Mode**
Run the following command to start the virtual machine. The execution will pause immediately before the first CPU instruction due to the execution constraints passed to QEMU.
```bash
qemu-system-x86_64 \
-kernel /tmp/kernel/linux-6.1.60/arch/x86/boot/bzImage \
-initrd /tmp/kernel/initramfs.cpio.gz \
-append "console=ttyS0 init=/init panic=-1" \
-nographic \
-s -S
```
* `-s`: Shorthand for `-gdb tcp::1234`. Opens a backend GDB server on TCP port 1234.
* `-S`: Freezes the CPU at startup.

**Step B: Attach GDB and Trace System Calls**
Open a separate terminal window and launch the debugger while pointing directly to the symbol-rich uncompressed kernel binary:
```bash
gdb /tmp/kernel/linux-6.1.60/vmlinux
(gdb) target remote :1234
(gdb) hbreak start_kernel
(gdb) c
```

### Vscode
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Linux Kernel Live Debug (QEMU)",
            "type": "cppdbg",
            "request": "launch",
            // Points to the uncompressed binary containing DWARF symbols
            "program": "/tmp/kernel/linux-6.1.60/vmlinux",
            "args": [],
            "stopAtEntry": false,
            "cwd": "/tmp/kernel/linux-6.1.60",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "/usr/bin/gdb",
            // Connects to the QEMU backend server
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
            // Source path mapping so VS Code can align the binary with your source code
            "sourceFileMap": {
                "/tmp/kernel/linux-6.1.60": "${workspaceFolder}/linux-6.1.60"
            }
        }
    ]
}
```