# GDB Remote Debugging — Complete Beginner's Reference

**Document Purpose:** A self-sufficient, permanent reference for understanding GDB remote debugging from first principles. A reader returning after one year should need nothing else.

**Environment Used in This Document:**
| Role | Machine | Relevant Path |
|---|---|---|
| **Target** | QEMU (the running program lives here) | `/apps/debug/debug-demo` |
| **Build** | Where the binary was compiled | `/module/dev/src/...` |
| **Host** | Where GDB runs / source code lives | `/tmp/debug-learn/src/...` |

---

## Table of Contents

1. [Stage 1 — What a Binary Actually Contains](#stage-1--what-a-binary-actually-contains)
2. [Stage 2 — What GDB Actually Needs to Debug](#stage-2--what-gdb-actually-needs-to-debug)
3. [Stage 3 — The Three-Machine Problem](#stage-3--the-three-machine-problem)
4. [Stage 4 — The Stripped Binary Problem](#stage-4--the-stripped-binary-problem)
5. [Stage 5 — Shared Libraries: The Hidden Dependency](#stage-5--shared-libraries-the-hidden-dependency)
6. [Stage 6 — What Gets Downloaded From the Remote Machine](#stage-6--what-gets-downloaded-from-the-remote-machine)
7. [Stage 7 — sysroot: The Master Solution](#stage-7--sysroot-the-master-solution)
8. [Stage 8 — set substitute-path: The Surgical Solution](#stage-8--set-substitute-path-the-surgical-solution)
9. [Stage 9 — solib-search-path: The Manual Override](#stage-9--solib-search-path-the-manual-override)
10. [Stage 10 — Decision Matrix: Which Tool to Use When](#stage-10--decision-matrix-which-tool-to-use-when)
11. [Stage 11 — Complete Setup for This Environment](#stage-11--complete-setup-for-this-environment)
12. [Stage 12 — Sample Programs for Deliberate Practice](#stage-12--sample-programs-for-deliberate-practice)
13. [Stage 13 — Deliberate Practice Exercises](#stage-13--deliberate-practice-exercises)
14. [Quick Reference Card](#quick-reference-card)

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

**Key fact about this environment:** The binary on the QEMU target (`/apps/debug/debug-demo`) is **stripped**. It has `.text` (machine code) but **no debug sections**. GDB running on the host machine cannot get debug information from this binary. This is the central problem that the tools in this document solve.

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
│  Source: Source code on the host machine.               │
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
Stage A: SOURCE on Host
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
    $ apk update && apk add gcc-aarch64-none-elf binutils-aarch64-none-elf
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
    -fsdev local,id=src_dev,path=/tmp/debug-learn/build,security_model=none \
    -device virtio-9p-pci,fsdev=src_dev,mount_tag=host_share
```

### 3.2 The Path Embedding Problem

When `gcc` compiles a file, it **records the absolute path of the source file inside the binary's debug sections**. Specifically, `.debug_line` and `.debug_str` contain entries like:

```
"source file for function main() is at: /module/dev/src/main.c"
```

This path is **hardcoded at compile time** into the binary. It is the path on the **build machine** where compilation happened.

**The problem:** When GDB runs on the **host machine**, it reads this path from the debug info and tries to open `/module/dev/src/main.c`. But on the host machine, the source is at `/tmp/debug-learn/src/main.c`. The path `/module/dev/src/main.c` does not exist on the host. GDB cannot find the source file and shows:

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

GDB on the host needs to load these same libraries to resolve addresses inside them. But the host machine's `/lib/` contains libraries for the **host architecture** (e.g., x86_64), not the target architecture (e.g., ARM/QEMU). Loading the wrong architecture libraries will fail or give wrong results.

This is **Problem 2: Shared Library Mismatch**.

---

## Stage 4 — The Stripped Binary Problem

### 4.1 Why Target Binaries Are Stripped

Binaries deployed to embedded systems or production devices are stripped because:

- **Size**: Debug sections can make a binary 2–10x larger.
- **Security**: Debug symbols expose internal function names, variable names, and structure layouts.
- **Performance**: Loading large binaries takes more time.

### 4.2 The Solution: Unstripped Binary on the Host

The standard workflow is:

```
Build Machine produces TWO artifacts:
  1. debug-demo          (stripped) → deployed to target /apps/debug/debug-demo
  2. debug-demo.debug    (unstripped, kept) → stays on build machine or copied to host
```

GDB is started on the **host** and pointed at the **unstripped** binary. GDB uses that local file only for its debug sections. The actual execution happens on the target via gdbserver.

```bash
# On host, GDB loads the unstripped binary for debug info:
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

The QEMU target might run an ARM binary. Its libc is compiled for ARM. The host machine runs x86_64. Its `/lib/` contains x86_64 libc. GDB must use the **target's** libc (ARM version), not the host's.

This is why a copy of the target's entire library set must be made available to GDB on the host.

### 5.4 Where Shared Libraries Live on the Target

```
Target (QEMU) filesystem:
  /lib/                           ← core system libraries
  /lib/aarch64-linux-gnu/         ← architecture-specific (if multiarch)
  /usr/lib/                       ← additional libraries
  /usr/lib/aarch64-linux-gnu/
  /lib64/                         ← 64-bit loader
```

GDB needs access to all of these from the host.

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

| What is downloaded | From (target path) | Why | Where it lands on host |
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

**sysroot** = "system root". It is a **directory on the host machine that contains a copy of the target's entire filesystem** (or at least its library directories).

Conceptually, if the target's filesystem root is `/` and it contains:
```
/lib/libc.so.6
/usr/lib/libm.so.6
/lib/ld-linux-aarch64.so.1
```

Then the sysroot on the host is a directory, say `/opt/target-sysroot/`, that mirrors this:
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
| `set sysroot ""` | Disable sysroot; GDB uses host's `/` (dangerous for cross-debug) |

**`remote:` is a special protocol prefix.** It tells GDB: "the sysroot is the remote target itself." GDB will download files on demand. This is useful when no local copy of the target filesystem exists.

```gdb
(gdb) set sysroot remote:
```

With `remote:`, when GDB needs `/lib/libc.so.6`, it downloads it from the target via gdbserver's file protocol.

### 7.6 How to Create a sysroot

There are several methods to populate a sysroot directory on the host:

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

1. **Debugging a native program**: GDB is on the same machine as the running process. The target's libraries ARE the host's libraries.
2. **Statically linked binary**: The binary contains all library code inside itself. No `.so` files are needed at all. (Check with: `file debug-demo` — if it says "statically linked", no sysroot needed.)
3. **Only inspecting crashes (core dumps)** of host-architecture programs with host libraries.

For this environment (QEMU ARM target, GDB on host x86), **sysroot is required**.

---

## Stage 8 — set substitute-path: The Surgical Solution

### 8.1 What substitute-path Solves

`set substitute-path` solves **Problem 1: Source Path Mismatch** (from Stage 3.2). It does NOT help with shared libraries. It only helps GDB find source files.

Recall the problem:
- Debug info inside the binary says source is at: `/module/dev/src/main.c`
- Actual source on the host is at: `/tmp/debug-learn/src/main.c`

`set substitute-path` tells GDB: "when looking for a file, replace the beginning of the path."

### 8.2 The Command Syntax

```gdb
set substitute-path FROM TO
```

| Parameter | Meaning |
|---|---|
| `FROM` | The path prefix as it appears inside the binary's debug info (the build machine path) |
| `TO` | The actual path on the host machine where source files exist |

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

1. **Source is compiled in the same directory where GDB runs**: Paths embedded in debug info match the host paths exactly.
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

3. Host system paths (dangerous for cross-debug, may load wrong arch)
   → /lib/libc.so.6  (this is the HOST's libc, wrong for cross-debug)

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

1. **No complete sysroot is available**, but specific library files have been manually copied to the host.
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
Host machine      : GDB runs here, source at /tmp/debug-learn/src/
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

### 11.3 Step 2 — Start GDB on Host

```bash
# On the host machine, use the cross-debugger matching the target architecture:
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

### 11.6 Saving Configuration to a .gdbinit File

Instead of typing these commands every session, save them to a `.gdbinit` file in the project directory:

```gdb
# File: /tmp/debug-learn/.gdbinit

set sysroot /opt/target-sysroot
set substitute-path /module/dev/src /tmp/debug-learn/src
set solib-search-path /opt/extra-libs
target remote 192.168.1.100:1234
break main
continue
```

Start GDB with this init file:
```bash
gdb -x /tmp/debug-learn/.gdbinit /tmp/debug-learn/bin/debug-demo.debug
```

Or GDB automatically loads `.gdbinit` from the current directory if `set auto-load safe-path` allows it.

---

## Stage 12 — Sample Programs for Deliberate Practice

All programs are designed to be compiled on the build machine with debug info, stripped for deployment to the target, and debugged from the host.

### 12.1 Program A — Fundamentals: Functions and Variables

**File: `/module/dev/src/01_fundamentals.c`**

This program practices: breakpoints, stepping, printing variables, inspecting the call stack.

```c
/*
 * Program: 01_fundamentals.c
 * Purpose: GDB basics — variables, functions, call stack
 * Concepts covered: break, next, step, print, backtrace, finish
 */

#include <stdio.h>

/* A simple structure to practice inspecting complex types */
struct Rectangle {
    int width;
    int height;
};

/* Compute area. Practice: set breakpoint here, print arguments */
int compute_area(struct Rectangle r) {
    int area = r.width * r.height;
    return area;
}

/* Compute perimeter. Practice: inspect 'r' fields individually */
int compute_perimeter(struct Rectangle r) {
    int perimeter = 2 * (r.width + r.height);
    return perimeter;
}

/* Display function. Practice: step INTO this from main */
void display_result(const char *label, int value) {
    printf("%s: %d\n", label, value);
}

int main(void) {
    /* Practice: break here, use 'next' to step through */
    struct Rectangle rect;
    rect.width  = 10;
    rect.height = 5;

    int area      = compute_area(rect);
    int perimeter = compute_perimeter(rect);

    display_result("Area",      area);
    display_result("Perimeter", perimeter);

    /* Practice: modify 'area' at runtime using:
     *   (gdb) set variable area = 999
     * Then continue and observe the changed output */

    printf("Done.\n");
    return 0;
}
```

**Build commands:**
```bash
# On build machine, from /module/dev/src/
# Unstripped (keep for GDB):
gcc -g -O0 -o /module/dev/build/01_fundamentals.debug 01_fundamentals.c

# Stripped (deploy to target):
cp /module/dev/build/01_fundamentals.debug /module/dev/build/01_fundamentals
strip /module/dev/build/01_fundamentals

# Deploy stripped binary to target:
scp /module/dev/build/01_fundamentals root@192.168.1.100:/apps/debug/

# Copy unstripped binary to host for GDB:
cp /module/dev/build/01_fundamentals.debug /tmp/debug-learn/bin/
```

**Practice exercises for this program:**

```gdb
(gdb) break main                    # Stop at entry of main
(gdb) break compute_area            # Stop when compute_area is called
(gdb) run                           # Start (or 'continue' after target remote)
(gdb) next                          # Execute one line, don't enter functions
(gdb) step                          # Execute one line, ENTER functions
(gdb) print rect                    # Print the entire struct
(gdb) print rect.width              # Print one field
(gdb) print area                    # Print a variable
(gdb) backtrace                     # Show call stack
(gdb) frame 1                       # Switch to frame 1 (caller of current)
(gdb) finish                        # Run until current function returns
(gdb) set variable area = 999       # Modify a variable at runtime
(gdb) continue                      # Resume execution
```

---

### 12.2 Program B — Pointers and Memory

**File: `/module/dev/src/02_pointers.c`**

This program practices: examining memory, pointer dereferencing, watching memory changes.

```c
/*
 * Program: 02_pointers.c
 * Purpose: Understand pointer behavior, memory layout, watchpoints
 * Concepts covered: examine (x), print *, watch, display
 */

#include 
#include 
#include 

#define BUFFER_SIZE 64

/* Fills buffer with a repeated character */
void fill_buffer(char *buf, int size, char ch) {
    for (int i = 0; i < size - 1; i++) {
        buf[i] = ch;
    }
    buf[size - 1] = '\0';
}

/* Reverses a string in-place */
void reverse_string(char *str) {
    int len = strlen(str);
    int left  = 0;
    int right = len - 1;
    while (left < right) {
        char temp  = str[left];
        str[left]  = str[right];
        str[right] = temp;
        left++;
        right--;
    }
}

/* Dynamically allocates and returns an integer array */
int *create_array(int size, int start_value) {
    int *arr = malloc(size * sizeof(int));
    if (arr == NULL) {
        return NULL;
    }
    for (int i = 0; i < size; i++) {
        arr[i] = start_value + i;
    }
    return arr;
}

int main(void) {
    /* --- Section 1: Stack buffer ---
     * Practice: examine raw memory with 'x' command */
    char buffer[BUFFER_SIZE];
    fill_buffer(buffer, BUFFER_SIZE, 'A');

    /* Practice: x/64cb buffer   → examine 64 bytes as chars */
    /* Practice: x/16xb buffer   → examine 16 bytes as hex   */

    /* --- Section 2: Pointer arithmetic ---
     * Practice: print ptr, print *ptr, print ptr[3] */
    char *ptr = buffer;
    ptr += 10;   /* advance pointer by 10 bytes */

    /* --- Section 3: String reversal ---
     * Practice: watch buffer  → GDB stops when buffer changes */
    char word[16];
    strncpy(word, "hello", sizeof(word));
    reverse_string(word);

    /* --- Section 4: Heap allocation ---
     * Practice: print arr[0]@5 → print 5 elements from arr   */
    int *arr = create_array(5, 100);
    if (arr == NULL) {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    for (int i = 0; i < 5; i++) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }

    free(arr);
    printf("buffer (first 10): %.10s\n", buffer);
    printf("reversed word: %s\n", word);

    return 0;
}
```

**Key GDB commands to practice:**

```gdb
(gdb) break fill_buffer
(gdb) continue
(gdb) x/64cb buffer             # Examine 64 bytes as characters
(gdb) x/16xb buffer             # Examine 16 bytes in hex

(gdb) break reverse_string
(gdb) continue
(gdb) watch str[0]              # Stop whenever str[0] changes

(gdb) break create_array
(gdb) continue
(gdb) next                      # Step through allocation
(gdb) print arr                 # Print pointer address
(gdb) print *arr                # Dereference: print first element
(gdb) print arr[0]@5            # Print 5 elements starting at arr[0]
(gdb) display arr[0]@5          # Auto-print these on every stop
```

---

### 12.3 Program C — Crash Analysis (Segfault)

**File: `/module/dev/src/03_crash.c`**

This program practices: post-crash analysis, core dumps, finding the root cause.

```c
/*
 * Program: 03_crash.c
 * Purpose: Deliberate crash for debugging practice.
 * Concepts covered: backtrace after crash, info locals, find root cause
 *
 * WARNING: This program INTENTIONALLY crashes. That is the point.
 */

#include 
#include 
#include 

typedef struct Node {
    int value;
    struct Node *next;
} Node;

/* Inserts a value into a linked list */
Node *insert(Node *head, int value) {
    Node *new_node = malloc(sizeof(Node));
    new_node->value = value;
    new_node->next  = head;
    return new_node;
}

/* Sums all values — contains a deliberate bug:
 * Does not check for NULL before dereferencing */
int sum_list(Node *head) {
    int total = 0;
    /* BUG: 'current' is never checked against NULL before access
     * This causes a crash when the list is empty or a node is corrupt */
    Node *current = head;
    while (current->value != -1) {    /* BUG: crashes if current == NULL */
        total   += current->value;
        current  = current->next;
    }
    return total;
}

/* Processes data — calls sum_list with potentially bad input */
int process_data(int *values, int count) {
    Node *list = NULL;

    for (int i = 0; i < count; i++) {
        list = insert(list, values[i]);
    }

    /* BUG: The list has no -1 sentinel node.
     * sum_list will walk past the end and dereference NULL. */
    int result = sum_list(list);

    /* Note: list is never freed — memory leak. Practice: find it. */
    return result;
}

int main(void) {
    int data[] = {10, 20, 30, 40, 50};
    int count  = sizeof(data) / sizeof(data[0]);

    printf("Processing %d values...\n", count);
    int result = process_data(data, count);
    printf("Result: %d\n", result);   /* This line will not be reached */

    return 0;
}
```

**Practice exercises:**

```gdb
# Run the program — it will crash
(gdb) continue
# Program received signal SIGSEGV, Segmentation fault.

# Now investigate:
(gdb) backtrace             # Which function crashed? At what line?
(gdb) frame 0               # Go to crash frame
(gdb) info locals           # What were the local variables?
(gdb) print current         # Print the pointer that was NULL
(gdb) frame 1               # Go one level up
(gdb) info locals           # Inspect the caller's state
(gdb) list                  # Show source around the crash point
```

**What the backtrace will show:**
```
#0  sum_list (head=0x...) at 03_crash.c:32     ← crash here
#1  process_data (values=0x..., count=5) at 03_crash.c:52
#2  main () at 03_crash.c:62
```

---

### 12.4 Program D — Shared Library Usage

**File: `/module/dev/src/04_shared_lib.c`**

This program is specifically designed to practice `set sysroot` and `info sharedlibrary`.

```c
/*
 * Program: 04_shared_lib.c
 * Purpose: Uses multiple shared libraries. Practice sysroot configuration.
 * Concepts covered: info sharedlibrary, stepping into library code,
 *                   understanding resolved vs. unresolved library addresses.
 */

#include 
#include 
#include        /* libm — requires linking with -lm */
#include 
#include 

/* Computes hypotenuse using sqrt() from libm */
double compute_hypotenuse(double a, double b) {
    /* Practice: step into sqrt(). With correct sysroot/debug libs,
     * GDB can show libm source. Without, it shows assembly. */
    double sum_squares = (a * a) + (b * b);
    double result = sqrt(sum_squares);
    return result;
}

/* Allocates with error checking — uses errno from libc */
void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        /* Practice: when malloc fails, inspect errno:
         *   (gdb) print errno
         *   (gdb) call strerror(errno)   */
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
    printf("Hypotenuse(3, 4) = %.2f\n", h);   /* Expected: 5.00 */

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
gcc -g -O0 -o /module/dev/build/04_shared_lib.debug 04_shared_lib.c -lm
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

---

### 12.5 Program E — Multi-threaded Program

**File: `/module/dev/src/05_threads.c`**

```c
/*
 * Program: 05_threads.c
 * Purpose: Multi-threaded debugging practice.
 * Concepts covered: info threads, thread N, break with thread condition,
 *                   detecting race conditions with GDB.
 */

#include 
#include 
#include 
#include 

#define NUM_THREADS 3
#define ITERATIONS  5

/* Shared counter — intentionally NOT protected by mutex.
 * This is a deliberate race condition for educational purposes. */
static int shared_counter = 0;

/* Protected counter with mutex for comparison */
static int protected_counter = 0;
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int thread_id;
    int increment_by;
} ThreadArgs;

/* Worker function for unprotected counter */
void *worker_unprotected(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        /* RACE CONDITION: read-modify-write is not atomic */
        int current = shared_counter;           /* read  */
        current += args->increment_by;           /* modify */
        shared_counter = current;               /* write  */

        /* Sleep to make race condition more visible */
        usleep(1000);
    }

    printf("Thread %d finished. Incremented by %d, %d times.\n",
           args->thread_id, args->increment_by, ITERATIONS);
    return NULL;
}

/* Worker function for protected counter */
void *worker_protected(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&counter_mutex);
        protected_counter += args->increment_by;
        pthread_mutex_unlock(&counter_mutex);
        usleep(1000);
    }

    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];

    printf("Starting %d threads...\n", NUM_THREADS);

    /* Launch unprotected threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id    = i + 1;
        args[i].increment_by = (i + 1) * 10;   /* 10, 20, 30 */
        pthread_create(&threads[i], NULL, worker_unprotected, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Expected: (10+20+30) * 5 = 300
     * Actual may differ due to race condition */
    printf("Unprotected counter: %d (expected 300)\n", shared_counter);

    /* Reset and test protected version */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_protected, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Protected counter:   %d (expected 300)\n", protected_counter);

    pthread_mutex_destroy(&counter_mutex);
    return 0;
}
```

**Build:**
```bash
gcc -g -O0 -o /module/dev/build/05_threads.debug 05_threads.c -lpthread
```

**Practice exercises:**
```gdb
(gdb) break worker_unprotected
(gdb) continue

# After hitting breakpoint:
(gdb) info threads              # List all threads
(gdb) thread 2                  # Switch to thread 2
(gdb) backtrace                 # See thread 2's call stack

# Set a breakpoint that triggers only in thread 3:
(gdb) break worker_unprotected thread 3

# Watch the shared variable:
(gdb) watch shared_counter
# GDB will stop every time shared_counter changes and show which thread changed it

# Print thread-local variables:
(gdb) thread apply all print args  # Print 'args' in all threads
(gdb) thread apply all backtrace   # Backtrace all threads simultaneously
```

---

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

---

### Stage 13.2 — Breakpoints and Stepping (Program A)

**Isolated skill:** Control flow in GDB.

| Command | What it does | When to use |
|---|---|---|
| `break LOCATION` | Set a breakpoint | Before running |
| `continue` (c) | Resume until next break | After a break |
| `next` (n) | Next line, stay in current function | Don't want to enter called functions |
| `step` (s) | Next line, enter called functions | Want to trace into sub-functions |
| `finish` | Run until current function returns | Inside a function, want to get back to caller |
| `until LINE` | Run until specific line | Skip a loop quickly |

**Exercise sequence:**
```gdb
break main
continue
next              ← observe: rect.width = 10 executes
next              ← observe: rect.height = 5 executes
step              ← observe: GDB enters compute_area()
print r           ← confirm: r = {width=10, height=5}
print r.width     ← confirm: $1 = 10
finish            ← return to main
print area        ← confirm: area = 50
```

---

### Stage 13.3 — Memory Inspection (Program B)

**Isolated skill:** The `x` (examine) command.

**Syntax:** `x/NFU ADDRESS`

| Letter | Meaning | Options |
|---|---|---|
| N | Count (how many units) | Any integer |
| F | Format | `x` (hex), `d` (decimal), `c` (char), `s` (string), `i` (instruction) |
| U | Unit size | `b` (byte), `h` (halfword/2B), `w` (word/4B), `g` (giant/8B) |

**Exercise sequence:**
```gdb
break fill_buffer
continue
finish                     ← let fill_buffer complete
x/10cb buffer              ← examine 10 chars: should show 'A' ten times
x/10xb buffer              ← same data in hex: 0x41 = 'A'
x/1s  buffer               ← examine as string
print &buffer              ← print address of buffer
print sizeof(buffer)       ← print compile-time size
```

---

### Stage 13.4 — Crash Analysis (Program C)

**Isolated skill:** Post-crash investigation.

**Exercise sequence:**
```gdb
break main
continue
continue              ← let program run until crash
                      ← SIGSEGV will occur

backtrace             ← identify crash location
frame 0               ← go to crash frame
info locals           ← what variables exist here?
print current         ← what is the bad pointer?

frame 1               ← go to caller
info locals           ← inspect caller's state
list                  ← see source around the bug

# Key question to answer: why is 'current' NULL?
# Trace back: how was the list constructed? Did it end with a -1 sentinel?
```

---

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
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

 GDB REMOTE DEBUGGING — QUICK REFERENCE

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

 STARTUP SEQUENCE
 ────────────────
 [target]  gdbserver :1234 /path/to/stripped-binary
 [host]    gdb-multiarch /path/to/unstripped-binary.debug

 CONFIGURATION (before 'target remote')
 ───────────────────────────────────────
 set sysroot /opt/target-sysroot       ← fix library paths
 set sysroot remote:                   ← download libs from target
 set substitute-path /build/src /host/src  ← fix source paths
 set solib-search-path /extra/libs     ← extra library search dirs
 set directories /host/src:$cdir:$cwd  ← alternative source search

 CONNECT
 ───────
 target remote HOST:PORT

 BREAKPOINTS
 ───────────
 break main                  break at function
 break file.c:42             break at file + line
 break *0x401234             break at address
 watch variable              stop when variable changes
 catch syscall               stop at system call
 info breakpoints            list all breakpoints
 delete N                    delete breakpoint N

 EXECUTION
 ─────────
 continue (c)                resume
 next (n)                    next line, no entry
 step (s)                    next line, with entry
 finish                      run to function return
 until LINE                  run to line

 INSPECTION
 ──────────
 print EXPR                  print value
 print *ptr                  dereference pointer
 print arr@N                 print N elements of array
 display EXPR                auto-print at every stop
 x/NFU ADDR                  examine raw memory
 info locals                 all local variables
 info args                   function arguments
 backtrace (bt)              call stack
 frame N                     switch to frame N
 info registers              all CPU registers
 info sharedlibrary          shared library status

 MODIFICATION
 ────────────
 set variable NAME = VAL     change a variable
 call FUNCTION(ARGS)         call any function

 THREADS
 ───────
 info threads                list threads
 thread N                    switch to thread N
 thread apply all bt         backtrace all threads

 DIAGNOSTICS
 ───────────
 show sysroot                current sysroot
 show substitute-path        active path rules
 info sharedlibrary          library load status
 maintenance info sections   all ELF sections

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

 WHICH TOOL SOLVES WHICH PROBLEM?

 "No source file for /build/path/main.c"
   → set substitute-path /build/path /host/path

 "??" in backtrace / library addresses unresolved
   → set sysroot /opt/target-sysroot  OR  set sysroot remote:

 "info sharedlibrary" shows Syms Read: No for one library
   → set solib-search-path /path/containing/that/lib

 "No symbol table is loaded" / "no debugging symbols"
   → GDB is loaded with stripped binary; use unstripped version

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
