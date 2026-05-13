# GDB Remote Debugging — Permanent Reference
## Cross-Architecture Stripped Binary + Shared Library Resolution

**Document Objective:**
Connect GDB (running on the Debug machine) to a **stripped** binary executing on a remote
Target machine, and correctly resolve all shared library symbols.
The practice program is `04_shared_lib.c`.

**Environment Used in This Document:**

| Role | Machine | Architecture | Relevant Path |
|---|---|---|---|
| **Build** | QEMU x86\_64 | x86\_64 host, aarch64 cross-compiler | `/module/dev/src/`, `/module/dev/build/` |
| **Target** | QEMU aarch64 | aarch64 | `/apps/debug/` |
| **Debug** | QEMU x86\_64 | x86\_64 | `/tmp/debug-learn/` |

**Host machine** (the real physical computer running all three QEMU instances):
Path `/tmp/debug-learn/` is shared into both the Build machine and the Debug machine via VirtIO 9P.

---

## Stage 1 — What a Binary Actually Contains

Before any GDB concept can be understood, the structure of a compiled binary must be clear.

### 1.1 The ELF File Format

On Linux, every compiled program is stored as an **ELF (Executable and Linkable Format)** file.
An ELF file is divided into named **sections**. Each section holds a specific category of data.

```
Binary File (ELF)
│
├── .text          → Machine code (CPU instructions). Always present.
├── .data          → Global variables that have an initial value.
├── .bss           → Global variables with no initial value (zero-initialized).
├── .rodata        → Read-only data (string literals such as "Result: %d\n").
├── .dynamic       → List of shared libraries this binary depends on. Present in
│                    dynamic executables even when stripped.
│
│   ── Sections below exist ONLY in debug builds (compiled with -g) ──
│
├── .debug_info    → Maps machine addresses to: function names, variable names, types.
├── .debug_line    → Maps machine addresses to: SOURCE FILE NAME + LINE NUMBER.
├── .debug_abbrev  → Compression dictionary used by .debug_info.
└── .debug_str     → String table containing: function names, variable names, file paths.
```

**Why this matters for remote debugging:**
GDB cannot know that address `0x00401234` means "line 14 of `compute_hypotenuse`" on its own.
It reads `.debug_line` and `.debug_info` to make that translation.
If those sections are absent, GDB cannot display source code, variable names, or function names.

### 1.2 The Two Binary Types

| Type | How Created | Debug Sections | Binary Size |
|---|---|---|---|
| **Debug build** (unstripped) | `gcc -g -O0 -o program program.c` | Present | Large |
| **Release build** (stripped) | compile then `strip program` | Absent | Small |

**Key fact about `.dynamic`:**
Even a stripped binary retains the `.dynamic` section.
That section lists which `.so` files the binary needs at runtime.
GDB reads this section during `target remote` to discover what shared libraries to locate.

---

## Stage 2 — The Stripped Binary Problem

### 2.1 Why Target Binaries Are Stripped

Binaries deployed to embedded or production systems are stripped because:

- **Size**: Debug sections can make a binary 3–10× larger than needed.
- **Security**: Debug symbols expose internal function names, variable layouts, and logic.
- **Startup time**: The OS loads smaller files faster.

### 2.2 The Standard Solution: Two Separate Artifacts

The Build machine produces **two files** from one compile run:

```
Source: /module/dev/src/04_shared_lib.c
         │
         ▼ aarch64-linux-musl-gcc -g -O0
         │
         ├── /module/dev/build/debug/04_shared_lib.debug   ← unstripped, stays on host
         └── /module/dev/build/bin/04_shared_lib            ← stripped, deployed to target
```

GDB on the Debug machine is pointed at the **unstripped** file.
GDB uses that file **for debug information only** — it does not run it.
The **stripped** file is what the Target machine actually executes.

### 2.3 The Compile-Time Path Embedding Problem

When `aarch64-linux-musl-gcc` compiles `04_shared_lib.c`, it records the **absolute path
of the source file** inside the `.debug_info` and `.debug_line` sections.

```
Compilation happens on: Build machine
Source file is at:       /module/dev/src/04_shared_lib.c
Path recorded in binary: /module/dev/src/04_shared_lib.c  ← hardcoded at compile time
```

When GDB on the Debug machine reads this binary and tries to display source code, it opens:
`/module/dev/src/04_shared_lib.c`

But on the Debug machine, the source file is at:
`/tmp/debug-learn/src/04_shared_lib.c`

GDB tries a path that does not exist on the Debug machine.
This is the **Source Path Problem** — solved by `set substitute-path`.

---

## Stage 3 — The Three-Machine Architecture

### 3.1 How the Three Machines Relate

```
┌─────────────────────────────────────────────────────────────┐
│                    HOST MACHINE                             │
│          (Physical computer, runs all QEMU instances)       │
│          /tmp/debug-learn/ on host disk                     │
└───────────────────┬─────────────────────┬───────────────────┘
                    │                     │
                    │  VirtIO 9P share    │  VirtIO 9P share
                    │  → /module/dev/     │  → /tmp/debug-learn/
                    ▼                     ▼
   ┌─────────────────────┐   ┌─────────────────────────────-┐
   │    BUILD MACHINE    │   │       DEBUG MACHINE          │
   │    QEMU x86_64      │   │       QEMU x86_64            │
   │                     │   │                              │
   │  Cross-compiles src │   │  Runs GDB                    │
   │  Puts artifacts in  │   │  Loads .debug file           │
   │  /module/dev/build/ │   │  Source at /tmp/debug-learn/ │
   └─────────────────────┘   └────────────-─────────────────┘
             |        unstripped    ^      ^
             |----------------------|      │
             | cross compile and           │ TCP socket port 1234
             | copy stripped to            │ GDB Remote Protocol
             | target machine,             |
             | copy unstripped to          |
             | debug machine               |
             |            ┌───────────────────────────────-─┐
             |  stripped  │        TARGET MACHINE           │
             |----------->│        QEMU aarch64             │
                          │                                 │
                          │  Runs stripped binary           │
                          │  /apps/debug/04_shared_lib      │
                          │  Runs gdbserver on port 1234    │
                          └───────────────────────────────-─┘
```

### 3.2 What Each Machine Holds

| Machine | What it holds relevant to debugging |
|---|---|
| **Build** | Source code, cross-compiler toolchain (including target's sysroot), unstripped `.debug` file |
| **Target** | Stripped binary, target's actual shared libraries (`libc.so`, `libm.so`, etc.) |
| **Debug** | GDB, unstripped `.debug` file (via shared mount), source code (via shared mount) |

---

## Stage 4 — What Happens When `target remote` is Issued

This stage explains every automatic operation GDB performs on connection.
Understanding this sequence is the foundation for understanding why `set sysroot` matters.

### 4.1 The Connection Sequence

```
(gdb) target remote 192.168.10.1:1234
```

**Step 1: Handshake**
GDB and gdbserver exchange capability information.
gdbserver reports the target architecture (aarch64) and process ID.

**Step 2: Reading the `.dynamic` section**
GDB reads the `.dynamic` section of the binary it was started with (the `.debug` file).
This section lists every shared library the program depends on:

```
NEEDED   libc.so                    ← shared library filename
NEEDED   libm.so                    ← (with Musl: same file as libc)
INTERP   /lib/ld-musl-aarch64.so.1  ← path to the dynamic linker
```

**Step 3: Locating the dynamic linker**
GDB must find the **dynamic linker** (also called the loader).
The dynamic linker is itself a shared library. It is the first piece of code that runs when a
dynamic executable starts, and it loads all other `.so` files into memory.

```
For aarch64 Musl-based systems, the dynamic linker is:
  /lib/ld-musl-aarch64.so.1
```

GDB needs this file to understand where all other libraries are mapped in memory.

**Step 4: Searching for libraries locally**
For each required file (dynamic linker + all `.so` dependencies), GDB searches in this order:

```
1. sysroot directory + the library's absolute path from the target
2. solib-search-path directories (by filename only, as a fallback)
3. The Debug machine's own system paths (wrong for cross-debug — wrong architecture)
4. Download from the Target via gdbserver's file transfer protocol (slow, last resort)
```

**Step 5: Reading symbols from found libraries**
GDB reads the symbol tables from each library file it located.
This enables GDB to translate a raw address like `0x0000ffff9a001234` into the
human-readable name `sqrt` or `printf`.

### 4.2 Files That Get Downloaded or Needed

| File | Target Path | Why GDB Needs It | Required? |
|---|---|---|---|
| Dynamic linker | `/lib/ld-musl-aarch64.so.1` | Starting point for all library resolution | Yes |
| C library | `/lib/libc.so` | Resolves `printf`, `malloc`, `strerror`, `qsort` | Yes |
| Math library | `/lib/libm.so` | Resolves `sqrt` — on Musl, same file as libc | Yes |
| Main binary | `/apps/debug/04_shared_lib` | Sometimes fetched if not provided locally | No (provided locally via .debug file) |

**Key point about "downloading":**
If `sysroot` is configured correctly, GDB reads all library files from the local disk.
No network transfer occurs for libraries.
Network transfer only happens if a library is not found locally (slow fallback).

---

## Stage 5 — sysroot: The Master Solution for Shared Libraries

### 5.1 The Core Problem sysroot Solves

The Target machine runs **aarch64** libraries.
The Debug machine has its own `/lib/` which contains **x86_64** libraries.

If GDB tries to open the Debug machine's own `/lib/libc.so` to resolve symbols in the
Target's program, the file is for the wrong CPU architecture.
The addresses inside it are wrong. The symbol names do not match.
The result is garbage backtraces and unresolved addresses shown as `?? ()`.

### 5.2 What sysroot Is

**sysroot** (short for: system root) is a directory on the Debug machine that
**mirrors the Target machine's filesystem structure**, containing the Target's actual
library files.

```
Target filesystem:               sysroot directory on Debug:
/                                /tmp/debug-learn/build/toolchain/aarch64-linux-musl/
├── lib/                         ├── lib/
│   ├── libc.so          ──────► │   ├── libc.so          (same file, correct arch)
│   └── ld-musl-aarch64.so.1     │   └── ld-musl-aarch64.so.1
└── usr/                         └── usr/
    └── lib/                         └── lib/
        └── libm.so      ──────►         └── libm.so
```

### 5.3 How GDB Uses sysroot

When GDB needs to open a library whose path it read from the Target
(for example: `/lib/ld-musl-aarch64.so.1`), it **prepends the sysroot path**:

```
sysroot = /tmp/debug-learn/build/toolchain/aarch64-linux-musl

Library path from target = /lib/ld-musl-aarch64.so.1

GDB opens = /tmp/debug-learn/build/toolchain/aarch64-linux-musl/lib/ld-musl-aarch64.so.1
                      ▲                                                    ▲
                 (sysroot)                                       (target library path)
```

If that path exists and contains the correct architecture binary, GDB reads it successfully.
No download from the Target is needed.

### 5.4 Where the sysroot Comes From in This Environment

The cross-compiler toolchain (`aarch64-linux-musl-cross`) downloaded during preparation
**already contains a complete sysroot** for the target architecture.

```
/tmp/debug-learn/build/toolchain/
└── aarch64-linux-musl/          ← this directory IS the sysroot
    ├── lib/
    │   ├── libc.so
    │   └── ld-musl-aarch64.so.1
    └── usr/
        └── lib/
```

The same toolchain used to cross-compile the binary contains the exact library files
the binary was linked against. This is the most reliable sysroot source.

### 5.5 The GDB Command

```gdb
(gdb) set sysroot /tmp/debug-learn/build/toolchain/aarch64-linux-musl
```

This must be set **before** `target remote` is issued.
After connection, GDB uses this path when resolving every library request.

### 5.6 The `target:/` Alternative

```gdb
(gdb) set sysroot target:/
```

This tells GDB: "the sysroot is the remote Target itself."
GDB downloads every library from the Target via gdbserver's file transfer protocol on demand.

| `set sysroot /local/path` | `set sysroot target:/` |
|---|---|
| GDB reads from local disk | GDB downloads from Target over network |
| Fast | Slow |
| Requires local copy of target libraries | No local copy needed |
| Reliable | Depends on network stability |

For this environment, the local toolchain sysroot is preferred.

### 5.7 When sysroot Can Be Skipped

`set sysroot` is not needed when:

1. The binary is **statically linked** (all library code is inside the binary itself).
   Verify: `file 04_shared_lib` — if output says "statically linked", no sysroot is needed.

2. **Native debugging**: GDB and the Target run on the same machine with the same architecture.
   The Debug machine's own `/lib/` is correct.

For `04_shared_lib.c`, sysroot **is required** — the binary uses `libm` (for `sqrt`) and `libc`
(for `printf`, `malloc`, `qsort`), and the Target is aarch64 while the Debug is x86_64.

---

## Stage 6 — The Musl Libc Architectural Peculiarity

This stage is specific to this environment. The cross-compiler uses **Musl libc**,
which behaves differently from **glibc** (the standard Linux C library).
This difference silently breaks library resolution if not understood.

### 6.1 Musl vs glibc Library Structure

**glibc** separates functionality into multiple `.so` files:
```
libc.so.6       → core C functions (printf, malloc, etc.)
libm.so.6       → math functions (sqrt, sin, cos)
libpthread.so.0 → threading
libdl.so.2      → dynamic loading
```

**Musl libc** consolidates everything into a **single file**:
```
libc.so         → contains ALL of: core C, math, threading, dynamic loading
```

`libm.so` in a Musl installation is a **symlink** that points to `libc.so`.
`ld-musl-aarch64.so.1` (the dynamic linker) is also a symlink that points to `libc.so`.

### 6.2 The Symlink Problem: Absolute vs Relative

Inside the toolchain sysroot directory, inspect the symlinks:

```bash
ls -la /tmp/debug-learn/build/toolchain/aarch64-linux-musl/lib/
```

**Correct output (relative symlinks):**
```
-rwxr-xr-x  libc.so
lrwxrwxrwx  ld-musl-aarch64.so.1 -> libc.so    ← relative: stays inside sysroot
lrwxrwxrwx  libm.so              -> libc.so    ← relative: stays inside sysroot
```

**Broken output (absolute symlinks):**
```
lrwxrwxrwx  ld-musl-aarch64.so.1 -> /lib/libc.so    ← absolute: escapes sysroot!
```

**Why absolute symlinks break everything:**
When GDB follows `ld-musl-aarch64.so.1 → /lib/libc.so`, it goes to the **Debug machine's
own** `/lib/libc.so`. That file is compiled for x86_64 — the wrong architecture.
GDB either fails to read it or reads wrong symbols. The result is `Syms Read: No` in
`info sharedlibrary` and `?? ()` in backtraces, even though the sysroot path is correct.

### 6.3 How to Verify and Fix Symlinks

```bash
# On the host machine, check the symlinks:
ls -la /tmp/debug-learn/build/toolchain/aarch64-linux-musl/lib/

# If any symlink shows -> /lib/... (absolute), fix it with a relative symlink:
cd /tmp/debug-learn/build/toolchain/aarch64-linux-musl/lib/
ln -sf libc.so ld-musl-aarch64.so.1
ln -sf libc.so libm.so
```

### 6.4 What GDB Shows When Musl Is Working Correctly

```gdb
(gdb) info sharedlibrary
From            To              Syms Read   Shared Object Library
0x0000ffff9a000 0x0000ffff9b000 Yes         /tmp/debug-learn/build/toolchain/aarch64-linux-musl/lib/libc.so
```

Only **one entry** appears. This is normal for Musl.
That one file contains the code for `printf`, `sqrt`, `malloc`, and the dynamic linker itself.
`Syms Read: Yes` confirms GDB successfully loaded its symbols.

---

## Stage 7 — solib-search-path: The Fallback Search

### 7.1 What solib-search-path Does

`solib-search-path` provides a **list of extra directories** GDB searches when a library
is not found via sysroot.

It is a fallback mechanism. sysroot is always searched first.
If sysroot contains all required libraries, solib-search-path is not needed.

### 7.2 The Key Difference from sysroot

| | `set sysroot` | `set solib-search-path` |
|---|---|---|
| **How it maps paths** | Prepends sysroot to the library's full path from Target | Searches directories by filename only |
| **Directory structure** | Must mirror Target's structure exactly | Flat — any `.so` file placed in the listed dirs is found |
| **Search order** | First | Second (fallback) |
| **Best used when** | Complete Target filesystem mirror is available | Only a few `.so` files are available locally |

### 7.3 The GDB Command

```gdb
(gdb) set solib-search-path /opt/extra-libs:/opt/vendor-libs
```

Multiple directories are separated by colons on Linux.

### 7.4 When solib-search-path Is Needed

- The sysroot is missing a custom or third-party library.
- A library on the Target is in a non-standard location (e.g., `/opt/myapp/lib/libcustom.so`)
  not mirrored in the sysroot directory structure.

For `04_shared_lib.c` using Musl with the toolchain sysroot correctly configured,
`solib-search-path` is not required.

---

## Stage 8 — substitute-path: Fixing the Source Path Mismatch

### 8.1 What substitute-path Solves

`substitute-path` is independent of shared library resolution.
It only affects one thing: GDB's ability to **display source code**.

The binary records: `/module/dev/src/04_shared_lib.c` (the Build machine path).
Source exists at: `/tmp/debug-learn/src/04_shared_lib.c` (the Debug machine path).

GDB cannot find the source file without this setting.

### 8.2 The GDB Command

```gdb
(gdb) set substitute-path /module/dev/src /tmp/debug-learn/src
```

**Syntax:** `set substitute-path [FROM] [TO]`

| Argument | Value | Meaning |
|---|---|---|
| `FROM` | `/module/dev/src` | The path prefix embedded in the binary at compile time |
| `TO` | `/tmp/debug-learn/src` | The actual path on the Debug machine |

### 8.3 How to Find the Embedded Path

```bash
# Run this on the host before starting GDB:
readelf --debug-dump=info /tmp/debug-learn/build/debug/04_shared_lib.debug \
    | grep DW_AT_comp_dir

# Output:
# DW_AT_comp_dir : /module/dev/src
```

The value of `DW_AT_comp_dir` is exactly what goes in the `FROM` argument.

### 8.4 Relationship to Shared Library Resolution

`substitute-path` and `sysroot` solve **completely separate problems**:

| Problem | Command | Affects |
|---|---|---|
| Source file not found | `set substitute-path` | Display of `.c`/`.h` source files only |
| Wrong/missing shared libraries | `set sysroot` | Library loading and symbol resolution |

Both may be needed simultaneously. Neither replaces the other.

---

## Stage 9 — The Practice Program

### 9.1 Source Code

**File path on Build machine:** `/module/dev/src/04_shared_lib.c`
**File path on Debug machine (via shared mount):** `/tmp/debug-learn/src/04_shared_lib.c`

```c
/*
 * Program  : 04_shared_lib.c
 * Purpose  : Uses multiple shared library functions.
 *            Designed to practice sysroot configuration and
 *            shared library symbol resolution via GDB.
 *
 * Libraries used:
 *   stdio.h  -> printf()                      from libc
 *   stdlib.h -> malloc(), free(), qsort(), exit()  from libc
 *   math.h   -> sqrt()                        from libm (Musl: same as libc)
 *   errno.h  -> errno                         from libc
 *   string.h -> strerror()                    from libc
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>

/* Uses sqrt() from libm */
double compute_hypotenuse(double a, double b) {
    double sum_squares = (a * a) + (b * b);
    double result = sqrt(sum_squares);
    return result;
}

/* Uses malloc() and errno/strerror() from libc */
void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        exit(1);
    }
    return ptr;
}

/* Comparator for qsort() */
int compare_ints(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

int main(void) {
    /* Exercise 1: sqrt() from libm */
    double h = compute_hypotenuse(3.0, 4.0);
    printf("Hypotenuse(3, 4) = %.2f\n", h);   /* expected: 5.00 */

    /* Exercise 2: malloc() + qsort() from libc */
    int count = 6;
    int *arr  = safe_malloc(count * sizeof(int));
    arr[0] = 42; arr[1] = 7;  arr[2] = 19;
    arr[3] = 3;  arr[4] = 88; arr[5] = 1;

    qsort(arr, count, sizeof(int), compare_ints);

    printf("Sorted: ");
    for (int i = 0; i < count; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");    /* expected: 1 3 7 19 42 88 */

    free(arr);
    return 0;
}
```

### 9.2 What Each Function Tests in GDB

| Function | Library | What resolving it proves |
|---|---|---|
| `sqrt()` | `libm.so` / Musl `libc.so` | `libm` is correctly loaded from sysroot |
| `printf()` | `libc.so` | Core C library is resolved |
| `malloc()` / `free()` | `libc.so` | Heap functions are resolved |
| `qsort()` | `libc.so` | Standard algorithm functions resolved |
| `strerror()` | `libc.so` | String/error functions resolved |

---

## Stage 10 — Complete Step-by-Step Setup

### 10.1 Step 1: Compile on the Build Machine

**On the host**, start the Build machine QEMU:

```bash
qemu-system-x86_64 \
    -enable-kvm \
    -m 1G \
    -drive file=/tmp/alpine-virt-3.23.4-x86_64.iso,media=cdrom \
    -boot d \
    -nographic \
    -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
    -fsdev local,id=src_dev,path=/tmp/debug-learn,security_model=none \
    -device virtio-9p-pci,fsdev=src_dev,mount_tag=host_share
```

**Inside the Build machine QEMU:**

```bash
# Mount the host shared directory
mkdir -p /module/dev
mount -t 9p -o trans=virtio host_share /module/dev

# Add cross-compiler to PATH
export PATH=/module/dev/build/toolchain/bin:$PATH

# Create output directories
mkdir -p /module/dev/build/debug /module/dev/build/bin

# Compile: -g embeds debug info, -O0 disables optimizations (easier to debug)
aarch64-linux-musl-gcc -g -O0 \
    /module/dev/src/04_shared_lib.c \
    -o /tmp/04_shared_lib_full \
    -lm

# Separate debug info into its own file (keeps symbols, no machine code duplication)
aarch64-linux-musl-objcopy \
    --only-keep-debug \
    /tmp/04_shared_lib_full \
    /module/dev/build/debug/04_shared_lib.debug

# Create the stripped binary for the Target (no debug sections)
aarch64-linux-musl-strip \
    --strip-debug --strip-unneeded \
    /tmp/04_shared_lib_full \
    -o /module/dev/build/bin/04_shared_lib

# Optional: embed a link from stripped binary to debug file
# GDB can auto-discover the .debug file using this link
aarch64-linux-musl-objcopy \
    --add-gnu-debuglink=/module/dev/build/debug/04_shared_lib.debug \
    /module/dev/build/bin/04_shared_lib
```

**Verify the output:**

```bash
# Confirm stripped binary has no debug sections:
file /module/dev/build/bin/04_shared_lib
# Expected: ELF 64-bit LSB executable, ARM aarch64, stripped

# Confirm debug file has symbols:
file /module/dev/build/debug/04_shared_lib.debug
# Expected: ELF 64-bit LSB executable, ARM aarch64, not stripped

# Inspect what source path was embedded in the binary:
aarch64-linux-musl-readelf \
    --debug-dump=info \
    /module/dev/build/debug/04_shared_lib.debug \
    | grep DW_AT_comp_dir
# Expected: DW_AT_comp_dir : /module/dev/src
```

Both output files are now available on the host machine at:
- `/tmp/debug-learn/build/debug/04_shared_lib.debug`
- `/tmp/debug-learn/build/bin/04_shared_lib`

### 10.2 Step 2: Start the Target Machine

**On the host**, open a **separate terminal** and start the Target QEMU:

```bash
qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a57 \
    -m 512 \
    -bios /tmp/QEMU_EFI.fd \
    -drive file=/tmp/alpine-virt-3.23.4-aarch64.iso,media=cdrom \
    -nographic \
    -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
    -fsdev local,id=src_dev,path=/tmp/debug-learn/build/bin,security_model=none \
    -device virtio-9p-pci,fsdev=src_dev,mount_tag=host_share \
    -netdev socket,id=net1,listen=:1234 \
    -device virtio-net-pci,netdev=net1,mac=52:54:00:12:34:10
```

**Inside the Target machine QEMU:**

```bash
# Mount the stripped binary directory
mkdir -p /apps/debug/
mount -t 9p -o trans=virtio host_share /apps/debug/

# Install gdbserver
setup-interfaces -a -r && setup-apkrepos -1
echo "http://dl-cdn.alpinelinux.org/alpine/v3.23/community" >> /etc/apk/repositories
apk update && apk add gdb

# Configure the private network interface (connects to Debug machine)
ip addr add 192.168.10.1/24 dev eth1
ip link set eth1 up

# Start gdbserver — pauses the process before main() and waits for GDB
gdbserver 192.168.10.1:1234 /apps/debug/04_shared_lib
```

Expected gdbserver output:
```
Process /apps/debug/04_shared_lib created; pid = 142
Listening on port 1234
```

The Target machine process is now paused before `main()` begins.
gdbserver is waiting for GDB to connect.

### 10.3 Step 3: Start the Debug Machine

**On the host**, open a **separate terminal** and start the Debug QEMU:

```bash
qemu-system-x86_64 \
    -enable-kvm \
    -m 1G \
    -drive file=/tmp/alpine-virt-3.23.4-x86_64.iso,media=cdrom \
    -boot d \
    -nographic \
    -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
    -fsdev local,id=src_dev,path=/tmp/debug-learn,security_model=none \
    -device virtio-9p-pci,fsdev=src_dev,mount_tag=host_share \
    -netdev socket,id=net1,connect=127.0.0.1:1234 \
    -device virtio-net-pci,netdev=net1,mac=52:54:00:12:34:11
```

**Inside the Debug machine QEMU:**

```bash
# Mount the host shared directory
mkdir -p /tmp/debug-learn/
mount -t 9p -o trans=virtio host_share /tmp/debug-learn/

# Install GDB with multi-architecture support
setup-interfaces -a -r && setup-apkrepos -1
echo "http://dl-cdn.alpinelinux.org/alpine/v3.23/community" >> /etc/apk/repositories
apk update && apk add gdb-multiarch

# Configure the private network interface (connects to Target machine)
ip addr add 192.168.10.2/24 dev eth1
ip link set eth1 up
```

### 10.4 Step 4: Verify Sysroot Symlinks Before Starting GDB

This check must happen before GDB is started. Broken symlinks cause silent failures.

```bash
ls -la /tmp/debug-learn/build/toolchain/aarch64-linux-musl/lib/
```

**Correct (relative symlinks):**
```
-rwxr-xr-x  libc.so
lrwxrwxrwx  ld-musl-aarch64.so.1 -> libc.so
lrwxrwxrwx  libm.so -> libc.so
```

**If any symlink shows an absolute path**, fix it:

```bash
cd /tmp/debug-learn/build/toolchain/aarch64-linux-musl/lib/
ln -sf libc.so ld-musl-aarch64.so.1
ln -sf libc.so libm.so
```

### 10.5 Step 5: Launch and Configure GDB

**Inside the Debug machine QEMU:**

```bash
gdb-multiarch /tmp/debug-learn/build/debug/04_shared_lib.debug
```

**Why the `.debug` file is passed, not the stripped binary:**
GDB reads debug information from whatever file it is given at startup.
The `.debug` file contains the complete DWARF data (symbol table, source mappings).
The stripped binary contains none of that.

**Inside the GDB prompt, enter these commands in order:**

```gdb
# 1. Define where GDB finds the Target's shared libraries locally.
#    The toolchain sysroot contains the exact aarch64 libraries the binary was linked against.
(gdb) set sysroot /tmp/debug-learn/build/toolchain/aarch64-linux-musl

# 2. Rewrite the build-time source path to the Debug machine's source path.
#    FROM = what the binary recorded at compile time (DW_AT_comp_dir value)
#    TO   = where the source actually lives on the Debug machine
(gdb) set substitute-path /module/dev/src /tmp/debug-learn/src

# 3. Connect to gdbserver on the Target.
#    After this command, GDB loads and resolves all shared libraries.
(gdb) target remote 192.168.10.1:1234
```

**Expected output after `target remote`:**

```
Remote debugging using 192.168.10.1:1234
Reading /lib/ld-musl-aarch64.so.1 from remote target...
Reading symbols from /tmp/debug-learn/build/toolchain/aarch64-linux-musl/lib/libc.so...
0x0000ffff9ab50000 in ?? ()
```

The message `Reading ... from remote target` appears only for files GDB cannot find locally.
Subsequent libraries are read from the local sysroot.
The final `?? ()` is normal — the process is paused inside the dynamic linker, before
`main()` is reached. The dynamic linker itself does not have DWARF debug info.

---

## Stage 11 — Verification: Confirming Everything Resolved

Run these verification commands before any debugging.
They confirm the entire setup is correct.

### 11.1 Verify Shared Library Resolution

```gdb
(gdb) info sharedlibrary
```

**Correct output (Musl — single library):**
```
From                To                  Syms Read   Shared Object Library
0x0000ffff9a000000  0x0000ffff9a0f0000  Yes         /tmp/debug-learn/build/toolchain/aarch64-linux-musl/lib/libc.so
```

**What each column means:**

| Column | Meaning |
|---|---|
| `From` / `To` | Memory address range where this library is loaded on the Target |
| `Syms Read: Yes` | GDB successfully found and read the symbol table from this file |
| `Syms Read: No` | GDB found where the library is in memory, but cannot find the file locally |
| Path shown | The local file GDB actually opened — confirms sysroot is being used |

**If `Syms Read` shows `No`:**
1. Check `show sysroot` — verify the path is correct.
2. Check symlinks inside sysroot — an absolute symlink causes this.
3. Check if the library exists at the expected path within the sysroot.

### 11.2 Verify Source File Resolution

```gdb
(gdb) break main
(gdb) continue
(gdb) list
```

**Correct output:**
```
25          double h = compute_hypotenuse(3.0, 4.0);
26          printf("Hypotenuse(3, 4) = %.2f\n", h);
27
```

**If `list` shows "No source file for /module/dev/src/...":**
`substitute-path` is not set correctly, or the source file does not exist at the `TO` path.
Verify: `show substitute-path`

### 11.3 Verify Symbol Table

```gdb
(gdb) info functions
```

**Correct output includes:**
```
All defined functions:

File /tmp/debug-learn/src/04_shared_lib.c:
14: double compute_hypotenuse(double, double);
24: void *safe_malloc(size_t);
32: int compare_ints(const void *, const void *);
39: int main(void);
```

If only addresses without names appear, GDB is reading the stripped binary by mistake.

---

## Stage 12 — Deliberate Practice Experiments

Each experiment isolates one variable. Complete Part 1 (broken state) and observe the failure.
Then complete Part 2 (fixed state) and observe the resolution.
The contrast between the two states teaches what each command actually provides.

### Experiment A — The sysroot Effect (Isolated)

**Isolated skill:** Understand exactly what sysroot provides.

**Part 1 — Without sysroot:**

```bash
gdb-multiarch /tmp/debug-learn/build/debug/04_shared_lib.debug
```

```gdb
# Do NOT set sysroot
(gdb) target remote 192.168.10.1:1234
(gdb) info sharedlibrary
```

**Observe:** `Syms Read: No`, or libraries show wrong paths, or GDB reports downloading from remote.

```gdb
(gdb) break compute_hypotenuse
(gdb) continue
(gdb) backtrace
```

**Observe:** Frames inside library functions show `?? ()`.

**Part 2 — With sysroot:**

```bash
gdb-multiarch /tmp/debug-learn/build/debug/04_shared_lib.debug
```

```gdb
(gdb) set sysroot /tmp/debug-learn/build/toolchain/aarch64-linux-musl
(gdb) target remote 192.168.10.1:1234
(gdb) info sharedlibrary
```

**Observe:** `Syms Read: Yes`, path shows the local sysroot file.

```gdb
(gdb) break compute_hypotenuse
(gdb) continue
(gdb) backtrace
```

**Observe:** All frames are named. Library function names are resolved.

**Gold Standard:** `info sharedlibrary` shows `Syms Read: Yes` for every library,
with paths inside the local sysroot directory.

---

### Experiment B — The substitute-path Effect (Isolated)

**Isolated skill:** Understand what substitute-path provides, independent of sysroot.

**Part 1 — Without substitute-path:**

```gdb
(gdb) set sysroot /tmp/debug-learn/build/toolchain/aarch64-linux-musl
(gdb) target remote 192.168.10.1:1234
(gdb) break main
(gdb) continue
(gdb) list
```

**Observe:** `No source file for "/module/dev/src/04_shared_lib.c"`
Shared libraries are resolved correctly (sysroot is set). Only source display fails.
This confirms sysroot and substitute-path are completely independent.

**Part 2 — With substitute-path:**

```gdb
(gdb) set sysroot /tmp/debug-learn/build/toolchain/aarch64-linux-musl
(gdb) set substitute-path /module/dev/src /tmp/debug-learn/src
(gdb) target remote 192.168.10.1:1234
(gdb) break main
(gdb) continue
(gdb) list
```

**Observe:** Source code lines appear. Libraries remain resolved as before.

---

### Experiment C — Stepping Into a Library Function (Understanding the PLT)

**Isolated skill:** Understand what `?? ()` means when stepping into a library.

```gdb
(gdb) set sysroot /tmp/debug-learn/build/toolchain/aarch64-linux-musl
(gdb) set substitute-path /module/dev/src /tmp/debug-learn/src
(gdb) target remote 192.168.10.1:1234
(gdb) break compute_hypotenuse
(gdb) continue
```

GDB stops at `compute_hypotenuse`. Step to the line calling `sqrt()`:

```gdb
(gdb) next       # advance to the sqrt() call line
(gdb) step       # step INTO sqrt()
```

**Observe:** GDB may momentarily show `?? ()` or an unmapped address.
This is the **PLT stub** — a 3–4 machine instruction bridge that jumps from the binary
into the library. The PLT stub has no source file and no DWARF entry.

```gdb
(gdb) stepi    # step one machine instruction at a time through the PLT stub
(gdb) stepi    # keep stepping — the stub is only a few instructions
```

Once the Program Counter enters the library's memory address range:
```
0x0000ffff9a010234 in sqrt () from /tmp/.../libc.so
```

The `?? ()` disappears. GDB now shows the library function name.
This `?? ()` during PLT traversal is expected behavior, not a configuration error.

**To confirm which address range belongs to which library:**
```gdb
(gdb) info files
```
Match the current `$pc` value against the address ranges shown for each loaded file.

---

## Stage 13 — Troubleshooting Reference

### Problem → Cause → Solution

**`?? ()` in backtrace**
```
#0  0x0000ffff9a001234 in ?? ()
```

| Cause | Fix |
|---|---|
| sysroot not set or wrong path | `set sysroot /tmp/debug-learn/build/toolchain/aarch64-linux-musl` |
| Symlink inside sysroot is absolute | `ln -sf libc.so ld-musl-aarch64.so.1` in the sysroot lib dir |
| Currently inside PLT stub (transient) | Use `stepi` to advance past the stub |

---

**`No source file for /module/dev/src/...`**

| Cause | Fix |
|---|---|
| `substitute-path` not set | `set substitute-path /module/dev/src /tmp/debug-learn/src` |
| `FROM` argument wrong | Re-check: `readelf --debug-dump=info file.debug \| grep DW_AT_comp_dir` |
| Source file missing at `TO` path | Verify file exists: `ls /tmp/debug-learn/src/04_shared_lib.c` |

---

**`info sharedlibrary` shows `Syms Read: No`**

| Cause | Fix |
|---|---|
| Library not found at sysroot path | `show sysroot` — verify path is correct |
| Absolute symlink inside sysroot | Make symlinks relative |
| Library in non-standard location | `set solib-search-path /path/containing/the/lib` |

---

**`no debugging symbols found`**

| Cause | Fix |
|---|---|
| GDB started with stripped binary | Always use `.debug` file: `gdb-multiarch .../04_shared_lib.debug` |

---

**GDB hangs or very slow after `target remote`**

| Cause | Fix |
|---|---|
| sysroot not set — GDB downloading all libs from Target | `set sysroot /tmp/debug-learn/build/toolchain/aarch64-linux-musl` |

---

## Quick Reference Card

```
 WHICH COMMAND SOLVES WHICH PROBLEM?
 ──────────────────────────────────────────────────────────────────────────
 PROBLEM                                COMMAND
 ──────────────────────────────────────────────────────────────────────────
 ?? in backtrace / library symbols      set sysroot /tmp/debug-learn/build/
 unresolved                                 toolchain/aarch64-linux-musl

 No source file for /module/dev/...     set substitute-path /module/dev/src
                                            /tmp/debug-learn/src

 info sharedlibrary: Syms Read No       set solib-search-path /path/to/lib/
 for one specific library

 no debugging symbols found             Use .debug file at GDB startup:
                                        gdb-multiarch .../04_shared_lib.debug

 Slow connection, downloading           sysroot not set — set it to local path
 from remote target                     set sysroot /tmp/.../aarch64-linux-musl

 ?? while stepping into sqrt()          Normal PLT stub — use stepi to advance

 ──────────────────────────────────────────────────────────────────────────
 INSPECT COMMANDS
 ──────────────────────────────────────────────────────────────────────────
 See loaded libraries + status          info sharedlibrary
 See current sysroot value              show sysroot
 See active path substitutions          show substitute-path
 See embedded compile path              readelf --debug-dump=info file.debug
                                            | grep DW_AT_comp_dir
 See what symbol $pc belongs to         info symbol $pc
 See all file address ranges            info files
 Force reload of library symbols        sharedlibrary
```

---

## Summary: The Minimum Required Setup

For this environment (aarch64 Target, x86\_64 Debug, Musl toolchain, stripped binary):

```bash
# Start GDB with the unstripped debug file — not the stripped binary:
gdb-multiarch /tmp/debug-learn/build/debug/04_shared_lib.debug
```

```gdb
# Set sysroot — solves the library architecture mismatch:
(gdb) set sysroot /tmp/debug-learn/build/toolchain/aarch64-linux-musl

# Set substitute-path — solves the source file path mismatch:
(gdb) set substitute-path /module/dev/src /tmp/debug-learn/src

# Connect:
(gdb) target remote 192.168.10.1:1234

# Verify — both checks must pass before debugging begins:
(gdb) info sharedlibrary    # Syms Read: Yes for all libraries
(gdb) list main             # Source code lines appear
```

These four actions address the four core problems introduced by the three-machine architecture:

| Problem | Introduced by | Solved by |
|---|---|---|
| No debug symbols at runtime | Stripped binary on Target | Loading `.debug` file in GDB |
| Wrong library architecture | Target is aarch64, Debug is x86\_64 | `set sysroot` |
| Source path not found | Compiled on Build machine, different path on Debug | `set substitute-path` |
| Missing non-standard libraries | Vendor/custom libs not in sysroot | `set solib-search-path` |