# GDB Remote Debugging — Complete Beginner's Reference

**Document Purpose:** A self-sufficient, permanent reference for understanding GDB remote debugging from first principles. A reader returning after one year should need nothing else.

**Environment Used in This Document:**
| Role | Machine | Relevant Path |
|---|---|---|
| **Target** | QEMU aarch64 (the running program lives here) | `/apps/debug/debug-demo` |
| **Build** | QEMU x86_64 Where the binary was compiled | `/module/dev/src/...` |
| **Debug** | x86_64 Where GDB runs / source code lives | `/tmp/debug-learn/src/...` |

---

## Stage 1 — What a Binary Actually Contains

Before understanding any GDB concept, it is essential to understand what a compiled binary file is made of.

### 1.1 The Anatomy of a Compiled Binary (ELF Format)

On Linux, compiled programs are stored in **ELF (Executable and Linkable Format)** files. Every ELF file has multiple sections. The ones critical to debugging are:

```
Binary File (ELF)
├── .text          → Machine code (CPU instructions). This ALWAYS exists.
├── .data          → Global variables with initial values.
├── .bss           → Global variables without initial values.
├── .rodata        → Read-only data (string literals).
│
│   ── The sections below are OPTIONAL (present only in debug builds) ──
│
├── .debug_info    → Maps machine addresses to variable names, types, functions.
├── .debug_line    → Maps machine addresses to SOURCE FILE + LINE NUMBER.
├── .debug_abbrev  → Compression dictionary for .debug_info.
├── .debug_str     → String table (function names, variable names, file paths).
└── .symtab        → Symbol table (function names and their addresses).
```

**Why this matters:** GDB does not magically know that address `0x00401234` corresponds to line 42 of `main.c`. It reads the `.debug_line` and `.debug_info` sections to make that translation. If those sections are absent, GDB cannot show source code or variable names.

### 1.2 The Two Types of Builds

| Build Type | Command | Debug Sections Present | Binary Size |
|---|---|---|---|
| **Debug build** | `gcc -g -o program program.c` | YES (`.debug_*`) | Large |
| **Release/stripped build** | `gcc -O2 -o program program.c` then `strip program` | NO | Small |

**Key fact about this environment:** The binary on the QEMU target (`/apps/debug/debug-demo`) is **stripped**. It has `.text` (machine code) but **no debug sections**. GDB running on the debug machine cannot get debug information from this binary. This is the central problem that the tools in this document solve.

---

## Stage 2 — What GDB Actually Needs to Debug

GDB requires **four distinct pieces of information** to provide a useful debugging experience. Understanding each piece separately is essential.

### The Four Requirements

```
┌─────────────────────────────────────────────────────────┐
│  Requirement 1: CONNECTIVITY                            │
│  GDB must be able to TALK to the running process.       │
│  Solution: gdbserver (runs on target), GDB connects.    │
├─────────────────────────────────────────────────────────┤
│  Requirement 2: DEBUG SYMBOLS                           │
│  GDB must know: address → function/variable/type.       │
│  Source: The UNSTRIPPED binary from the build machine.  │
├─────────────────────────────────────────────────────────┤
│  Requirement 3: SOURCE FILES                            │
│  GDB must find the actual .c/.h files to display them.  │
│  Source: Source code on the debug machine.              │
├─────────────────────────────────────────────────────────┤
│  Requirement 4: SHARED LIBRARY DEBUG INFO  (Optional)   │
│  GDB must also resolve addresses inside libc, libpthread│
│  etc. Those libraries must also be findable.            │
│  Source: The target's root filesystem (sysroot).        │
└─────────────────────────────────────────────────────────┘
```

Every GDB remote debugging problem is a failure to satisfy one or more of these four requirements.

---

## Stage 3 — The Three-Machine Problem

### 3.1 How a Program's Life Cycle Creates Path Mismatches

A program goes through these stages, each on a potentially different machine:

```
Stage A: SOURCE on Debug
  File: /tmp/debug-learn/src/main.c

        │ (Purpose: Only for debugging)
        ▼ (Shared to build machine/or build machine cloned same version separately)

Stage B: BINARY COMPILED on Build Machine
  File: /module/dev/src/main.c  ← compiler records THIS path inside debug info
  Output: /module/dev/build/debug-demo (unstripped)

        │
        ▼ (deployment / stripping)

Stage C: BINARY RUNS on Target (QEMU)
  File: /apps/debug/debug-demo  ← this is the stripped version
```

### Build Machine prepare

**Host Machine Configuration**
1. Create Directories: Create a source and build folder on the host filesystem.
    > `mkdir -p /tmp/debug-learn/src && mkdir -p /tmp/debug-learn/build`

2.  Download Image: Obtain the Alpine Linux Virtual ISO.
    > `cd /tmp` && `wget https://dl-cdn.alpinelinux.org/alpine/latest-stable/releases/x86_64/alpine-virt-3.23.4-x86_64.iso`

**Build Machine Execution**
1. **Launch QEMU:(Host)**
    ```bash
    qemu-system-x86_64 \
    -enable-kvm \
    -m 1G \
    -drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
    -boot d \
    -nographic \
    -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
    -fsdev local,id=src_dev,path=/tmp/debug-learn,security_model=none \
    -device virtio-9p-pci,fsdev=src_dev,mount_tag=host_share
    ```
    *   **`-fsdev`**: Defines a new filesystem device to the Qemu VM. 
        *   **`local`**: Use the local filesystem driver. othet options (`handle` better for race condition, `proxy`: Runs the filesystem driver in a separate process)
        *   **`id=src_dev`**: Assigns a unique identifier for new file system.
        *   **`path=/tmp/debug-learn`**: Specifies the actual directory on the host machine to share.
        *   **`security_model=none`**: This tells QEMU how to handle file permissions (UID/GID). `none` (or `passthrough`) is common for testing; it means the guest can access files based on the host's permissions.
    *   **`-device virtio-9p-pci`**: Adds a virtual hardware device (a PCI controller) to the guest machine so it can "see" the shared folder.
        *   **`fsdev=src_dev`**: Links this virtual hardware to the backend defined above.
        *   **`mount_tag=host_share`**: Creates a label or "tag" for the guest (VM) operating system. The guest kernel uses this tag to identify which share to mount.

2. **Inside QEMU Guest:**
    * **Mount Shared Volume:**
    ```bash
    $ `mkdir -p /module/dev`
    $ `mount -t 9p -o trans=virtio host_share /module/dev`
    ```
    The command `mount -t 9p -o trans=virtio host_share /module/dev` performs the following:

    *   **`-t <file_system>`**: Specifies the filesystem type as Plan 9.
    *   **`-o trans=virtio`**: Sets the transport method. It instructs the guest kernel to use the high-performance VirtIO drivers to move data between the guest and host.
    *   **`host_share`**: Refers to the `mount_tag` defined in the QEMU launch command. This tells the kernel which host directory to access.
    *   **`/module/dev`**: The local directory inside the guest where the host files will become visible.

    **Download tools**
    ```bash
    $ setup-interfaces -a -r && setup-apkrepos -1
    $ echo "http://dl-cdn.alpinelinux.org/alpine/v3.23/community" >> /etc/apk/repositories
    $ apk update
    $ wget https://musl.cc/aarch64-linux-musl-cross.tgz
    $ tar -xf aarch64-linux-musl-cross.tgz
    $ aarch64-linux-musl-cross/bin/aarch64-linux-musl-gcc /module/dev/src/hello.c -o /module/dev/build/hello.o
    ```

## Target Machine prepare
**Host machine prepare**
```bash
$ cd /tmp
$ wget https://dl-cdn.alpinelinux.org/alpine/latest-stable/releases/aarch64/alpine-virt-3.23.4-aarch64.iso
$ wget https://releases.linaro.org/components/kernel/uefi-linaro/16.02/release/qemu64/QEMU_EFI.fd 
$ qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a57 \
    -m 512 \
    -bios /tmp/QEMU_EFI.fd \
    -drive file=alpine-virt-3.23.4-aarch64.iso,media=cdrom \
    -nographic \
    -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
    -fsdev local,id=src_dev,path=/tmp/debug-learn/build,security_model=none \
    -device virtio-9p-pci,fsdev=src_dev,mount_tag=host_share \
    -netdev socket,id=net1,listen=:1234 \
    -device virtio-net-pci,netdev=net1,mac=52:54:00:12:34:10
```
**Qemu config**
```bash
$ mkdir -p /apps/debug/
$ mount -t 9p -o trans=virtio host_share /apps/debug/
$ setup-interfaces -a -r && setup-apkrepos -1
$ echo "http://dl-cdn.alpinelinux.org/alpine/v3.23/community" >> /etc/apk/repositories
$ apk update && apk add gdb
$ ip addr add 192.168.10.1/24 dev eth1
$ ip link set eth1 up
$ gdbserver 192.168.10.1:1234 /apps/debug/hello.o
```

## Debug 
**Host machine prepare**
 ```bash
$ qemu-system-x86_64 \
-enable-kvm \
-m 1G \
-drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
-boot d \
-nographic \
-netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
-fsdev local,id=src_dev,path=/tmp/debug-learn,security_model=none \
-device virtio-9p-pci,fsdev=src_dev,mount_tag=host_share \
-netdev socket,id=net1,connect=127.0.0.1:1234 \
-device virtio-net-pci,netdev=net1,mac=52:54:00:12:34:11
```

**Qemu config**
```bash
$ mkdir -p /tmp/debug-learn/
$ mount -t 9p -o trans=virtio host_share /tmp/debug-learn/
$ setup-interfaces -a -r && setup-apkrepos -1
$ echo "http://dl-cdn.alpinelinux.org/alpine/v3.23/community" >> /etc/apk/repositories
$ apk update && apk add gdb-multiarch
$ ip addr add 192.168.10.2/24 dev eth1
$ ip link set eth1 up
$ gdb-multiarch -ex "target remote 192.168.10.1:1234"
```

### 3.2 The Path Embedding Problem

When `gcc` compiles a file, it **records the absolute path of the source file inside the binary's debug sections**. Specifically, `.debug_line` and `.debug_str` contain entries like:

```
"source file for function main() is at: /module/dev/src/main.c"
```

This path is **hardcoded at compile time** into the binary. It is the path on the **build machine** where compilation happened.

**The problem:** When GDB runs on the **debug machine**, it reads this path from the debug info and tries to open `/module/dev/src/main.c`. But on the debug machine, the source is at `/tmp/debug-learn/src/main.c`. The path `/module/dev/src/main.c` does not exist on the debug. GDB cannot find the source file and shows:

```
(gdb) list
No source file for "/module/dev/src/main.c"
```

This is **Problem 1: Source Path Mismatch**.

### 3.3 The Shared Library Path Problem

When a program runs on the target (QEMU), it uses shared libraries from the **target's filesystem**:

```
Target filesystem:
  /lib/aarch64-linux-gnu/libc.so.6
  /lib/aarch64-linux-gnu/libpthread.so.0
```

GDB on the debug needs to load these same libraries to resolve addresses inside them. But the debug machine's `/lib/` contains libraries for the **debug architecture** (e.g., x86_64), not the target architecture (e.g., ARM/QEMU). Loading the wrong architecture libraries will fail or give wrong results.

This is **Problem 2: Shared Library Mismatch**.

---

## Stage 4 — The Stripped Binary Problem

### 4.1 Why Target Binaries Are Stripped

Binaries deployed to embedded systems or production devices are stripped because:

- **Size**: Debug sections can make a binary 2–10x larger.
- **Security**: Debug symbols expose internal function names, variable names, and structure layouts.
- **Performance**: Loading large binaries takes more time.

### 4.2 The Solution: Unstripped Binary on the debug

The standard workflow is:

```
Build Machine produces TWO artifacts:
  1. debug-demo          (stripped) → deployed to target /apps/debug/debug-demo
  2. debug-demo.debug    (unstripped, kept) → stays on build machine or copied to debug
```

GDB is started on the **debug** and pointed at the **unstripped** binary. GDB uses that local file only for its debug sections. The actual execution happens on the target via gdbserver.

```bash
# On debug, GDB loads the unstripped binary for debug info:
gdb /path/to/unstripped/debug-demo

# Then connects to gdbserver on QEMU:
(gdb) target remote 192.168.x.x:1234
```

GDB uses the local unstripped binary for **symbol information only**. It does not run it. The target runs the stripped binary.

---

## Stage 5 — Shared Libraries: The Hidden Dependency

### 5.1 What a Shared Library Is

A **shared library** (`.so` file — "shared object") is a compiled file containing reusable code (like C standard library functions: `printf`, `malloc`, `pthread_create`). Instead of copying this code into every binary, the binary records a dependency and the OS loads the library at runtime.

```
debug-demo (binary) links against:
  ├── libc.so.6         (C standard library: printf, malloc, etc.)
  ├── libm.so.6         (math library: sin, cos, sqrt)
  └── libpthread.so.0   (threading: pthread_create, mutex)
```

### 5.2 Why GDB Needs Shared Libraries

When a debugged program calls `printf()`, the CPU jumps into the `libc.so.6` library's code. GDB needs to:

1. Resolve what address `printf` is at (inside libc).
2. Show the backtrace correctly (call stack frames inside libc).
3. Step through libc code if needed.

Without the correct shared libraries, GDB shows:

```
(gdb) backtrace
#0  0x0000007fb4a123 in ?? ()   ← GDB cannot resolve this address
#1  0x0000007fb4b456 in ?? ()
```

### 5.3 The Library Architecture Problem

The QEMU target might run an ARM binary. Its libc is compiled for ARM. The debug machine runs x86_64. Its `/lib/` contains x86_64 libc. GDB must use the **target's** libc (ARM version), not the debug's.

This is why a copy of the target's entire library set must be made available to GDB on the debug.

### 5.4 Where Shared Libraries Live on the Target

```
Target (QEMU) filesystem:
  /lib/                           ← core system libraries
  /lib/aarch64-linux-gnu/         ← architecture-specific (if multiarch)
  /usr/lib/                       ← additional libraries
  /usr/lib/aarch64-linux-gnu/
  /lib64/                         ← 64-bit loader
```

GDB needs access to all of these from the debug.

---

## Stage 6 — What Gets Downloaded From the Remote Machine

### 6.1 The Automatic Download Mechanism

When GDB connects to gdbserver on the target, several things happen automatically. Understanding exactly what is downloaded, why, and where it lands is critical.

### 6.2 The Dynamic Linker and Library List

When `target remote` is issued, GDB:

**Step 1:** Reads the remote binary's `.dynamic` section (present even in stripped binaries) to find the list of required shared libraries and the path to the **dynamic linker** (also called the **loader**).

```
The dynamic linker is typically:
  /lib/ld-linux-aarch64.so.1    (ARM 64-bit)
  /lib/ld-linux-armhf.so.3      (ARM 32-bit)
  /lib64/ld-linux-x86-64.so.2   (x86 64-bit)
```

**Step 2:** GDB downloads the dynamic linker from the target **automatically** via gdbserver's file transfer protocol. It downloads this file to resolve library load addresses.

**Step 3:** GDB reads the loaded library list from the target process's memory (the linker maintains this list at runtime).

### 6.3 The Complete List of Automatically Downloaded Files

| What is downloaded | From (target path) | Why | Where it lands on debug |
|---|---|---|---|
| Dynamic linker | `/lib/ld-linux-*.so` | GDB needs it to find where libraries are mapped | Temp cache or sysroot |
| Shared libraries | `/lib/libc.so.6`, etc. | To resolve addresses inside them | Temp cache or sysroot |
| The main binary (sometimes) | `/apps/debug/debug-demo` | GDB may fetch it if not provided locally | Temp cache |

**Important:** GDB will attempt to download these files **only if it cannot find them locally**. The purpose of `sysroot` is to tell GDB: "look in this local directory tree first, before attempting to download from the target."

### 6.4 Where Downloaded Files Land

Without any configuration, GDB caches downloaded files in a temporary location. The exact path depends on the GDB version and configuration but is often:

```
/tmp/gdb-sysroot-cache/
  └── remote/
      ├── lib/
      │   └── libc.so.6
      └── lib/ld-linux-aarch64.so.1
```

With `set sysroot` configured properly, GDB reads from that directory instead of downloading.

---

## Stage 7 — sysroot: The Master Solution

### 7.1 What sysroot Means

**sysroot** = "system root". It is a **directory on the debug machine that contains a copy of the target's entire filesystem** (or at least its library directories).

Conceptually, if the target's filesystem root is `/` and it contains:
```
/lib/libc.so.6
/usr/lib/libm.so.6
/lib/ld-linux-aarch64.so.1
```

Then the sysroot on the debug is a directory, say `/opt/target-sysroot/`, that mirrors this:
```
/opt/target-sysroot/lib/libc.so.6
/opt/target-sysroot/usr/lib/libm.so.6
/opt/target-sysroot/lib/ld-linux-aarch64.so.1
```

### 7.2 Why sysroot Exists

Without sysroot, every time GDB needs a library, it must download it from the target over the network via gdbserver. This is:

- **Slow**: Network transfers are slow, especially for large libraries.
- **Unreliable**: Network interruptions corrupt the debug session.
- **Impossible** without gdbserver file-serving support.

With sysroot, GDB reads libraries from the local disk instantly.

### 7.3 How GDB Uses sysroot

When GDB encounters a shared library path from the target (e.g., `/lib/libc.so.6`), it **prepends the sysroot path** to construct the local path:

```
sysroot = /opt/target-sysroot

Library path from target = /lib/libc.so.6

GDB looks for = /opt/target-sysroot + /lib/libc.so.6
              = /opt/target-sysroot/lib/libc.so.6
```

If that file exists locally, GDB uses it. If not, it falls back to downloading from the target.

### 7.4 The GDB Command

```gdb
(gdb) set sysroot /opt/target-sysroot
```

Or equivalently, using the older alias:
```gdb
(gdb) set solib-absolute-prefix /opt/target-sysroot
```

Both commands are identical. `solib-absolute-prefix` is the old name; `sysroot` is the modern alias.

### 7.5 Special sysroot Values

| Value | Meaning |
|---|---|
| `set sysroot /some/path` | Use local directory as root of target filesystem |
| `set sysroot remote:` | Tell GDB to download everything from target via gdbserver |
| `set sysroot ""` | Disable sysroot; GDB uses debug's `/` (dangerous for cross-debug) |

**`remote:` is a special protocol prefix.** It tells GDB: "the sysroot is the remote target itself." GDB will download files on demand. This is useful when no local copy of the target filesystem exists.

```gdb
(gdb) set sysroot remote:
```

With `remote:`, when GDB needs `/lib/libc.so.6`, it downloads it from the target via gdbserver's file protocol.

### 7.6 How to Create a sysroot

There are several methods to populate a sysroot directory on the debug:

**Method A: rsync from target**
```bash
mkdir -p /opt/target-sysroot
rsync -avz root@192.168.x.x:/lib /opt/target-sysroot/
rsync -avz root@192.168.x.x:/usr/lib /opt/target-sysroot/usr/
```

**Method B: Use the cross-compiler's sysroot**
Cross-compilation toolchains (like Buildroot or Yocto) produce their own sysroot as part of the build:
```bash
# Buildroot example:
ls output/staging/           ← this IS the sysroot

set sysroot /path/to/buildroot/output/staging
```

**Method C: Let GDB download automatically (`remote:`)**
No local preparation needed. GDB downloads what it needs on demand. Slower, but zero setup.

### 7.7 When sysroot Can Be Skipped

`set sysroot` can be skipped (or left at default) when:

1. **Debugging a native program**: GDB is on the same machine as the running process. The target's libraries ARE the debug's libraries.
2. **Statically linked binary**: The binary contains all library code inside itself. No `.so` files are needed at all. (Check with: `file debug-demo` — if it says "statically linked", no sysroot needed.)
3. **Only inspecting crashes (core dumps)** of debug-architecture programs with debug libraries.

For this environment (QEMU ARM target, GDB on debug x86), **sysroot is required**.

---

## Stage 8 — set substitute-path: The Surgical Solution

### 8.1 What substitute-path Solves

`set substitute-path` solves **Problem 1: Source Path Mismatch** (from Stage 3.2). It does NOT help with shared libraries. It only helps GDB find source files.

Recall the problem:
- Debug info inside the binary says source is at: `/module/dev/src/main.c`
- Actual source on the debug is at: `/tmp/debug-learn/src/main.c`

`set substitute-path` tells GDB: "when looking for a file, replace the beginning of the path."

### 8.2 The Command Syntax

```gdb
set substitute-path FROM TO
```

| Parameter | Meaning |
|---|---|
| `FROM` | The path prefix as it appears inside the binary's debug info (the build machine path) |
| `TO` | The actual path on the debug machine where source files exist |

**For this environment:**
```gdb
(gdb) set substitute-path /module/dev/src /tmp/debug-learn/src
```

After this, when GDB internally tries to open `/module/dev/src/main.c`, it substitutes the prefix and opens `/tmp/debug-learn/src/main.c` instead.

### 8.3 How to Find What Path is Embedded in the Binary

The embedded path can be inspected without running GDB by using `readelf` or `objdump`:

```bash
# Method 1: readelf (shows DWARF debug info)
readelf --debug-dump=info /path/to/unstripped/debug-demo | grep DW_AT_comp_dir
# Output: DW_AT_comp_dir: /module/dev/src

# Method 2: strings (simple but finds all strings)
strings /path/to/unstripped/debug-demo | grep "\.c$"

# Method 3: objdump
objdump --dwarf=info /path/to/unstripped/debug-demo | grep DW_AT_name
```

The value of `DW_AT_comp_dir` (compilation directory) is the `FROM` argument for `set substitute-path`.

### 8.4 Multiple substitute-path Rules

Multiple rules can be set if different parts of the source tree have different paths:

```gdb
(gdb) set substitute-path /module/dev/src /tmp/debug-learn/src
(gdb) set substitute-path /module/dev/include /tmp/debug-learn/include
(gdb) set substitute-path /vendor/libs/src /tmp/debug-learn/vendor
```

### 8.5 Viewing and Removing Rules

```gdb
(gdb) show substitute-path        ← list all active rules
(gdb) unset substitute-path FROM  ← remove one rule
(gdb) unset substitute-path       ← remove all rules
```

### 8.6 substitute-path vs. sysroot: What Each Solves

| Problem | Tool | What it affects |
|---|---|---|
| Source files not found (path mismatch) | `set substitute-path` | Only source file lookup (`.c`, `.h` files) |
| Wrong/missing shared libraries | `set sysroot` | Only shared library loading |

They are independent tools solving independent problems. Both may be needed simultaneously.

### 8.7 When substitute-path Can Be Skipped

`set substitute-path` can be skipped when:

1. **Source is compiled in the same directory where GDB runs**: Paths embedded in debug info match the debug paths exactly.
2. **Using `set directories`**: An alternative (see below) that adds search paths without requiring exact prefix knowledge.
3. **Source is irrelevant**: Only inspecting register values or memory, not stepping through code.

### 8.8 Alternative: set directories

`set directories` adds a list of directories where GDB searches for source files. It is less precise than `substitute-path` because it does not perform prefix replacement — it just searches all listed directories for a matching filename.

```gdb
(gdb) set directories /tmp/debug-learn/src:/tmp/debug-learn/include:$cdir:$cwd
```

| Token | Meaning |
|---|---|
| `/tmp/debug-learn/src` | An actual path to search |
| `$cdir` | The compilation directory (from debug info) |
| `$cwd` | GDB's current working directory |

**When to use `set directories` vs `set substitute-path`:**
- Use `set substitute-path` when the exact build-time path prefix is known and source is organized identically.
- Use `set directories` when source files are scattered across multiple unrelated directories and exact prefix mapping is impractical.

---

## Stage 9 — solib-search-path: The Manual Override

### 9.1 What solib-search-path Is

`solib-search-path` is a **colon-separated list of directories** that GDB searches for shared library files. It is a supplementary mechanism that works alongside sysroot.

```gdb
(gdb) set solib-search-path /opt/target-libs:/opt/extra-libs
```

### 9.2 The Search Order GDB Uses

When GDB needs to find a shared library (e.g., `libc.so.6`), it searches in this order:

```
1. sysroot + absolute path from target
   → /opt/target-sysroot/lib/libc.so.6

2. solib-search-path directories (scanned in order)
   → /opt/target-libs/libc.so.6
   → /opt/extra-libs/libc.so.6

3. Debug system paths (dangerous for cross-debug, may load wrong arch)
   → /lib/libc.so.6  (this is the Debug's libc, wrong for cross-debug)

4. Download from target via gdbserver (if sysroot is remote: or fallback)
```

### 9.3 Key Difference From sysroot

| Feature | `set sysroot` | `set solib-search-path` |
|---|---|---|
| Path mapping | Prepends sysroot prefix to absolute paths | Searches listed dirs for filename only |
| Preserves structure | Yes (directory tree mirrors target) | No (flat search by filename) |
| Recommended for | Complete sysroot available | Individual library files are known |

### 9.4 When solib-search-path Is Used

Use `solib-search-path` when:

1. **No complete sysroot is available**, but specific library files have been manually copied to the debug.
2. **The sysroot is missing some libraries** (third-party `.so` files not in the standard path).
3. **Debugging a proprietary library** that is kept separately from the main sysroot.

```bash
# Example: manually copy only the needed libraries
mkdir -p /opt/debug-libs
scp root@target:/lib/libc.so.6 /opt/debug-libs/
scp root@target:/lib/libm.so.6 /opt/debug-libs/
```

```gdb
(gdb) set solib-search-path /opt/debug-libs
```

### 9.5 Debugging solib Issues

```gdb
# Show which shared libraries GDB has loaded and from where:
(gdb) info sharedlibrary

# Output example:
From        To          Syms Read   Shared Object Library
0x7fb4a000  0x7fb6f000  Yes         /opt/target-sysroot/lib/libc.so.6
0x7fb70000  0x7fb72000  No          libbluetooth.so.3   ← "No" = not found
```

`Syms Read: No` means GDB found the library's load address from the target but could not find the library file locally for debug symbols. The path listed is what GDB is still looking for.

---

## Stage 10 — Decision Matrix: Which Tool to Use When

### 10.1 Problem Identification Flowchart

```
START: GDB connected to target but something is wrong
│
├─► GDB shows "No source file for /module/dev/src/main.c"
│     └─► Problem: Source path mismatch
│           └─► SOLUTION: set substitute-path /module/dev/src /tmp/debug-learn/src
│
├─► GDB shows "??" instead of function names in backtrace
│   OR "Cannot find shared object file"
│     ├─► Check: is there a sysroot available?
│     │     YES → set sysroot /path/to/sysroot
│     │     NO  → set sysroot remote:   (download from target)
│     │
│     └─► Still missing specific libraries?
│           └─► SOLUTION: set solib-search-path /path/to/extra/libs
│
├─► GDB shows "No symbol table" or "no debugging symbols found"
│     └─► Problem: GDB is loading the STRIPPED binary
│           └─► SOLUTION: Start GDB with the UNSTRIPPED binary
│                 gdb /path/to/unstripped/debug-demo
│
└─► GDB cannot step into a function (goes to assembly)
      └─► Problem: No debug info for that function (e.g., inside libc)
            └─► Expected behavior for system libraries unless libc-dbg is installed
```

### 10.2 Summary Table

| Scenario | Required Tools | Can Skip |
|---|---|---|
| Native debug (same machine) | None | sysroot, substitute-path, solib-search-path |
| Cross-debug, static binary | substitute-path (if paths differ) | sysroot, solib-search-path |
| Cross-debug, dynamic binary, sysroot available | sysroot + substitute-path | solib-search-path |
| Cross-debug, dynamic binary, no sysroot | `sysroot remote:` + substitute-path | solib-search-path |
| Cross-debug, mixed sysroot + extra libs | sysroot + substitute-path + solib-search-path | Nothing |

---

## Stage 11 — Complete Setup for This Environment

### 11.1 Environment Recap

```
Target (QEMU)     : running /apps/debug/debug-demo (stripped)
Build machine     : compiled from /module/dev/src/
Debug machine     : GDB runs here, source at /tmp/debug-learn/src/
Unstripped binary : assumed available at /tmp/debug-learn/bin/debug-demo.debug
Target IP         : 192.168.1.100 (example)
gdbserver port    : 1234
```

### 11.2 Step 1 — Start gdbserver on Target (QEMU)

```bash
# On the QEMU target machine:
gdbserver :1234 /apps/debug/debug-demo
# Or attach to already-running process:
gdbserver :1234 --attach $(pidof debug-demo)
```

gdbserver output:
```
Process /apps/debug/debug-demo created; pid = 1042
Listening on port 1234
```

### 11.3 Step 2 — Start GDB on debug

```bash
# On the debug machine, use the cross-debugger matching the target architecture:
# For ARM target:
aarch64-linux-gnu-gdb /tmp/debug-learn/bin/debug-demo.debug
# OR if using a generic GDB with multi-arch support:
gdb-multiarch /tmp/debug-learn/bin/debug-demo.debug
```

**Critical:** The binary passed to GDB must be the **unstripped** version. GDB uses it for debug symbols only.

### 11.4 Step 3 — Configure GDB Before Connecting

```gdb
# Set the sysroot (copy of target's filesystem on host):
(gdb) set sysroot /opt/target-sysroot

# If sysroot is not available locally, use remote download:
# (gdb) set sysroot remote:

# Fix source path mismatch:
(gdb) set substitute-path /module/dev/src /tmp/debug-learn/src

# Optional: if any libraries are not in sysroot:
# (gdb) set solib-search-path /opt/extra-libs

# Connect to target:
(gdb) target remote 192.168.1.100:1234
```

### 11.5 Step 4 — Verify Configuration

```gdb
# Confirm shared libraries loaded correctly:
(gdb) info sharedlibrary

# Confirm source is findable:
(gdb) list main

# Confirm symbols are loaded:
(gdb) info functions
```

---

## Stage 12 — Sample Programs for Deliberate Practice

All programs are designed to be compiled on the build machine with debug info, stripped for deployment to the target, and debugged from the host.

### Program  — Shared Library Usage

**File: `/module/dev/src/04_shared_lib.c`**

This program is specifically designed to practice `set sysroot` and `info sharedlibrary`.

```c
/*
 * Program: 04_shared_lib.c
 * Purpose: Uses multiple shared libraries. Practice sysroot configuration.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>    /* For sqrt() */
#include <errno.h>   /* For errno */
#include <string.h>  /* For strerror() */

/* Computes hypotenuse using sqrt() from libm */
double compute_hypotenuse(double a, double b) {
    double sum_squares = (a * a) + (b * b);
    double result = sqrt(sum_squares);
    return result;
}

/* Allocates with error checking — uses errno from libc */
void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        exit(1);
    }
    return ptr;
}

/* Sorts an array using qsort() from libc */
int compare_ints(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

int main(void) {
    /* Test 1: Math library */
    double h = compute_hypotenuse(3.0, 4.0);
    printf("Hypotenuse(3, 4) = %.2f\n", h); 

    /* Test 2: Heap + sorting */
    int count = 6;
    int *arr  = safe_malloc(count * sizeof(int));
    arr[0] = 42; arr[1] = 7; arr[2] = 19;
    arr[3] = 3;  arr[4] = 88; arr[5] = 1;

    qsort(arr, count, sizeof(int), compare_ints);

    printf("Sorted: ");
    for (int i = 0; i < count; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    free(arr);
    return 0;
}
```

**Build with math library:**
```bash
aarch64-linux-musl-gcc -g -O0  04_shared_lib.c -o 04_shared_lib -lm
cp /module/dev/build/04_shared_lib.debug /module/dev/build/04_shared_lib
strip /module/dev/build/04_shared_lib
```

**Practice exercises:**
```gdb
# After connecting to target and setting sysroot:
(gdb) info sharedlibrary        # See which libs are loaded, which have symbols

# Break before library call:
(gdb) break compute_hypotenuse
(gdb) continue
(gdb) step                      # Step INTO sqrt() — what happens without libm debug?
(gdb) finish                    # Return from sqrt()

# Call a library function directly from GDB:
(gdb) call strerror(2)          # Calls strerror(ENOENT) from libc directly
(gdb) call printf("from gdb\n") # Call printf directly
```
## Cross-Debugging Shared Libraries with GDB

### 1. Fundamental Concept: The Sysroot

The **Sysroot** (System Root) is a directory on the host computer that mirrors the filesystem structure of the target device. In cross-platform development, the host (e.g., x86_64) and the target (e.g., AArch64) utilize different instruction sets and library binaries.

**Purpose:**
GDB requires access to target libraries to perform two critical tasks:

* **Symbol Resolution:** Translating memory addresses into human-readable function names (e.g., `0x...08a0` into `sqrt`).
* **Stack Unwinding:** Understanding how a function manages the stack to provide a backtrace (`bt`). Without the library's metadata, GDB cannot determine where one function call ends and another begins.

---

### 2. The Procedure Linkage Table (PLT)

The **PLT** is a small jump-table residing within the executable binary. When a program calls a function in a shared library (like `sqrt` in `libm`), it does not jump to the library directly. Instead, it jumps to a "stub" in the PLT.

**Logical Flow:**

1. **The Call:** The main program calls the PLT stub.
2. **The Resolution:** The stub checks if the address of the library function is already known.
3. **The Jump:** If known, the stub branches to the library address in memory.
4. **The Appearance in GDB:** While the CPU is executing the 3–4 instructions inside the PLT stub, GDB may display `?? ()`. This occurs because the stub is a machine-generated bridge and does not correspond to a specific line of source code.

---

### 3. Absolute vs. Relative Symlinks in Toolchains

In many Linux distributions, and specifically within the **Musl libc** ecosystem, the dynamic linker (`ld-musl-aarch64.so.1`) and the math library (`libm.so`) are often symlinks to the primary C library (`libc.so`).

**The Problem with Absolute Symlinks:**
A symlink pointing to `/lib/libc.so` is an **absolute path**. When GDB encounters this on a host machine, it attempts to follow the link to the host's own `/lib/` directory rather than staying within the designated `sysroot`. This leads to architecture mismatches or "file not found" errors.

**The Solution:**
Symlinks within a toolchain must be **relative**. By pointing `ld-musl-aarch64.so.1` to `libc.so` (without a leading slash), GDB is forced to resolve the file within the current `sysroot` directory.

---

### 4. Musl Libc Architectural Particularity

Unlike **glibc**, which separates functionality into multiple files (`libc.so`, `libm.so`, `libdl.so`), **Musl libc** often consolidates these into a single shared object file.

**Observation in GDB:**
When debugging a Musl-based target, `info sharedlibrary` may only show one entry (the dynamic linker). This is normal behavior because that single file contains the code for standard C functions, math operations, and the dynamic loader itself.

---

### 5. GDB Configuration Commands

| Command | Purpose |
| --- | --- |
| `set sysroot <path>` | Defines the local directory mimicking the target's root. GDB prepends this path to every library the target attempts to load. |
| `set sysroot target:/` | Instructs GDB to download libraries directly from the remote device. This ensures version matching but is significantly slower than local access. |
| `set solib-search-path <path>` | Provides a fallback directory for libraries. If the directory structure in the `sysroot` does not match the target exactly, GDB looks here. |
| `info sharedlibrary` | Displays which libraries are loaded, their memory addresses, and whether symbols (`Syms Read`) were successfully loaded. |
| `sharedlibrary` | Manually triggers GDB to attempt loading symbols for all currently mapped shared objects. |

---

### 6. Troubleshooting the "?? ()" in Backtraces

If a backtrace displays `?? ()` or stops prematurely, the cause is typically a failure in library resolution.

**Logic Check-list:**

1. **Verify Sysroot:** Ensure `show sysroot` points to the directory containing the `lib/` folder.
2. **Check Symlinks:** Ensure library files are not broken absolute links.
3. **Step-In Execution:** Use `si` (Step Instruction) to move past the PLT stub. Once the Program Counter ($PC) enters the library memory space (often starting with `0xffff...`), GDB can associate the address with the library symbols.
4. **Confirm Symbol Loading:** Use `info sharedlibrary`. If `Syms Read` is "No," GDB is ignoring the library, likely due to a path mismatch between the host and target.

## Stage 13 — Deliberate Practice Exercises

Mastery requires progressive, isolated practice. Complete each stage before advancing.

### Stage 13.1 — Verify GDB Setup (Zero Debugging)

**Goal:** Confirm the infrastructure works before debugging real code.

**Exercise:**
1. Start gdbserver on QEMU: `gdbserver :1234 /apps/debug/01_fundamentals`
2. Start GDB on host: `gdb-multiarch /tmp/debug-learn/bin/01_fundamentals.debug`
3. Configure: `set sysroot ...`, `set substitute-path ...`
4. Connect: `target remote 192.168.1.100:1234`
5. Verify: `info sharedlibrary` — all `Syms Read` should be `Yes`
6. Verify: `list main` — source code should appear
7. Verify: `break main` then `continue` — GDB should stop at line with `struct Rectangle rect;`

**Success criteria:** Source visible, libraries resolved, breakpoint works.


### Stage 13.5 — sysroot Verification Experiment

**Isolated skill:** Understand exactly what sysroot does.

**Exercise:**

**Part 1 — Without sysroot:**
```gdb
gdb-multiarch /tmp/debug-learn/bin/04_shared_lib.debug
# Do NOT set sysroot
target remote 192.168.1.100:1234
info sharedlibrary
# Observe: libraries show "No" for Syms Read or show wrong paths
break main
continue
backtrace
# Observe: ?? appears for frames inside libraries
```

**Part 2 — With sysroot:**
```gdb
gdb-multiarch /tmp/debug-learn/bin/04_shared_lib.debug
set sysroot /opt/target-sysroot     # or: set sysroot remote:
target remote 192.168.1.100:1234
info sharedlibrary
# Observe: libraries now show "Yes" for Syms Read
break main
continue
backtrace
# Observe: library function names are now resolved
```

**Compare the `info sharedlibrary` output between Part 1 and Part 2.** The difference demonstrates exactly what sysroot provides.

---

### Stage 13.6 — substitute-path Verification Experiment

**Isolated skill:** Understand source path resolution.

**Exercise:**

**Part 1 — Without substitute-path:**
```gdb
set sysroot /opt/target-sysroot
target remote 192.168.1.100:1234
break main
continue
list
# Expected output: "No source file for /module/dev/src/01_fundamentals.c"
```

**Part 2 — With substitute-path:**
```gdb
set sysroot /opt/target-sysroot
set substitute-path /module/dev/src /tmp/debug-learn/src
target remote 192.168.1.100:1234
break main
continue
list
# Expected output: actual source code lines appear
```

**The difference confirms what substitute-path provides.**

---

## Quick Reference Card

```
 WHICH TOOL SOLVES WHICH PROBLEM?

 "No source file for /build/path/main.c"
   → set substitute-path /build/path /host/path

 "??" in backtrace / library addresses unresolved
   → set sysroot /opt/target-sysroot  OR  set sysroot remote:

 "info sharedlibrary" shows Syms Read: No for one library
   → set solib-search-path /path/containing/that/lib

 "No symbol table is loaded" / "no debugging symbols"
   → GDB is loaded with stripped binary; use unstripped version

 Loaded all the symbol files but still it is showing `addr in ?? ()` then it might be inside `PLT` to find the where the current code is 
 1. run `bt` note down address of showing ??
 2. `info files` match the address between the range 
```
