# ELF

## Introduction

The Executable and Linking Format was originally developed and published by UNIX System Laboratories (USL) as part of the Application Binary Interface (ABI). The Tool Interface Standards committee(TIS) has selected the evolving ELF standard as a portable object file format that works on 32-bit Intel Architecture environments for a variety of operating systems.

## Types

There are three main types of object files:
--------------------------------------------
* **Relocatable file(ET_REL)**
    Contains code and data suitable to link with other object files to create a executable or shared object file.  
    These are object files, generate by compiler without linking.  
    ```c
    // file: hello.c
    #include <stdio.h>
    void hello() {
        printf("Hello from relocatable object!\n");
    }

    ```
    - Compilation:
    `gcc -c hello.c -o hello.o`  
    - Read header:
    `readelf -h hello.o | grep Type`

* **Executable file(ET_EXEC)**
    This is suitable for execution, this specifies how `exec()` create a program's process image.  
    This is final binary produces by linking many object files
    
    ```c
    // file: main.c
    void hello();  // Declaration from hello.o
    int main() {
        hello();
        return 0;
    }
    ```

    - Compilation:
    ```
    gcc -c main.c -o main.o      # relocatable
    gcc main.o hello.o -o main   # link to create executable
    ```
    - Read header:
    `readelf -h main | grep Type`

* **shared object file(ET_DYN)**
    Shared libraries (.so)  
    Contains code and data suitable for linking in two context
    1. (Build time) Link editor (ld) may process it with another relocatable or shared object file to create another object file
    ```c
    // file: util.c
    #include <stdio.h>
    void util() {
        printf("util() from util.o\n");
    }
    ```
    - Compilation: `gcc -c util.c -o util.o` (util.o — a relocatable file)

    ```c
    // file: libcore.c
    #include <stdio.h>
    void core() {
        printf("core() from libcore.so\n");
    }

    ```
    - compilation 
    ```
    gcc -fPIC -c libcore.c -o libcore.o
    gcc -shared -o libcore.so libcore.o
    ```
    libcore.so is a shared object

    Combine `util.o` and `libcore.so` to create another shared object
    `gcc -shared -o libcombined.so util.o libcore.so` 

    2. (Run time) Dynamic linker combines with an executable file and other shared objects to create a process image
    ```c
    // file: main.c
    void util();
    void core();

    int main() {
        util();
        core();
        return 0;
    }
    ```
    - compilation: `gcc -o main main.c -L. -lcombined`
    - execution:
    ```sh
    LD_LIBRARY_PATH=. ./main
    # Output:
    # util() from util.o
    # core() from libcore.so
    ```

    - Read header for shared object: `readelf -h libcore.so | grep Type`

## Structure 

Sections and segments have no specified order. Only the ELF header has a fixed position in the file.

An ELF header resides at the beginning and holds a `road map` describing the file’s organization. 

Sections hold the bulk of object file information for the linking view: instructions, data, symbol table, relocation information, and so on.

### Structure of header

```c
#define EI_NIDENT 16

// ELF32 Type Definitions (Size and Alignment)

#define Elf32_Addr        4   // Elf32_Addr: Unsigned program address
#define Elf32_Half        2   // Elf32_Half: Unsigned medium integer
#define Elf32_Off         4   // Elf32_Off: Unsigned file offset
#define Elf32_Sword       4   // Elf32_Sword: Signed large integer
#define Elf32_Word        4   // Elf32_Word: Unsigned large integer


typedef struct {
    unsigned char e_ident[EI_NIDENT]; // 1. Magic number + metadata (architecture (32/64-bit), endianness, OS ABI, etc.)
    Elf32_Half    e_type;             // 2. Type of ELF (relocatable, executable, shared)
    Elf32_Half    e_machine;          // 3. Target CPU architecture (e.g., x86)
    Elf32_Word    e_version;          // 4. ELF version
    Elf32_Addr    e_entry;            // 5. Entry point address (where execution starts)
    Elf32_Off     e_phoff;            // 6. Offset to Program Header Table, if no header table will have 0
    Elf32_Off     e_shoff;            // 7. Offset to Section Header Table, if no header table will have 0
    Elf32_Word    e_flags;            // 8. Processor-specific flags
    Elf32_Half    e_ehsize;           // 9. Size of this ELF header
    Elf32_Half    e_phentsize;        // 10. Size of each entry in the Program Header Table; All the entries are same size
    Elf32_Half    e_phnum;            // 11. Number of entries in the Program Header Table, if no header table will have 0
    Elf32_Half    e_shentsize;        // 12. Size of each entry in the Section Header Table; All the entries are same size
    Elf32_Half    e_shnum;            // 13. Number of entries in the Section Header Table, if no header table will have 0
    Elf32_Half    e_shstrndx;         // 14. Index of section name string table
} Elf32_Ehdr;

```

**e_ident**

This marks the file as a object file and provide machine-independent data with which to decode and interupt the file's content

| Name         | Value | Meaning                       |
|--------------|-------|-------------------------------|
| EI_MAG0      | 0     | 0x7F                          |
| EI_MAG1      | 1     | 'E'                           |
| EI_MAG2      | 2     | 'L'                           |
| EI_MAG3      | 3     | 'F'                           |
| EI_CLASS     | 4     | 32-bit or 64-bit              |
| EI_DATA      | 5     | Endianness                    |
| EI_VERSION   | 6     | ELF header version            |
| EI_OSABI     | 7     | Operating System / ABI        |
| EI_ABIVERSION| 8     | ABI Version                   |
| EI_PAD       | 9–15  | Unused (set to 0)             |


**e_type**

Identify the object file type

| Name       | Value   | Meaning                 |
|------------|---------|-------------------------|
| ET_NONE    | 0       | No file type            |
| ET_REL     | 1       | Relocatable file        |
| ET_EXEC    | 2       | Executable file         |
| ET_DYN     | 3       | Shared object file      |
| ET_CORE    | 4       | Core file               |
| ET_LOPROC  | 0xff00  | Processor-specific (low)|
| ET_HIPROC  | 0xffff  | Processor-specific (high)|

**e_machine**

| Name          | Value   | Meaning                    |
|---------------|---------|----------------------------|
| EM_NONE       | 0       | No machine                 |
| EM_M32        | 1       | AT&T WE 32100              |
| EM_SPARC      | 2       | SPARC                      |
| EM_386        | 3       | Intel 80386                |
| EM_68K        | 4       | Motorola 68000             |
| EM_88K        | 5       | Motorola 88000             |
| EM_860        | 7       | Intel 80860                |
| EM_MIPS       | 8       | MIPS RS3000                |
| EM_ARM        | 40      | ARM architecture           |
| EM_X86_64     | 62      | AMD x86-64 architecture    |
| EM_AARCH64    | 183     | ARM AArch64                |

**e_version**

| Name        | Value | Meaning            |
|-------------|-------|--------------------|
| EV_NONE     | 0     | Invalid Version    |
| EV_CURRENT  | 1     | Current version    |

**e_phoff**

This member holds the program header table’s file offset in bytes. If the file has no program header table, this member holds zero.

**e_shoff**

This member holds the section header table’s file offset in bytes. If the file has no section header table, this member holds zero.

**e_shstrndx**

Each section in an ELF file has a name — like .text, .data, .rodata, .bss, etc.
These names are not stored directly in the section header but rather as offsets into a string table (a blob of null-terminated strings).  
It gives the index of the section that holds this string table.  
If the file has no section name string table, this member holds the value SHN_UNDEF.

## Section

- **Section header table** tells us where each section is `(.text, .data. .bss,..)`
- Each entry is an `Elf32_Shdr` (or `Elf64_Shdr` in 64-bit).
- Table is an array of `Elf32_Shdr` or `Elf64_Shdr`

### ELF Header
- `e_shoff` Byte offset of the section header, to indicate where the header table is start
- `e_shnum` Number of entries in the table
- `e_shentsize` size of each entries in byte

### Reserved section

| Constant                     | Value               | Meaning                                                                                                |
| ---------------------------- | ------------------- | ------------------------------------------------------------------------------------------------------ |
| `SHN_UNDEF`                  | `0`                 | Symbol is **undefined**. This is common for symbols waiting to be resolved (e.g., external functions). |
| `SHN_LORESERVE`              | `0xff00`            | Start of reserved range (for system use).                                                              |
| `SHN_HIRESERVE`              | `0xffff`            | End of reserved range.                                                                                 |
| `SHN_LOPROC` to `SHN_HIPROC` | `0xff00` – `0xff1f` | Reserved for processor-specific meanings.                                                              |
| `SHN_ABS`                    | `0xfff1`            | Symbol has an **absolute value** — it's not relocated.  (Fixed location)                                                |
| `SHN_COMMON`                 | `0xfff2`            | Symbol is in the **common block** (used in FORTRAN or C uninitialized global vars).                    |


Sometimes, a symbol in the symbol table doesn’t live in a real section. These special constants help communicate:
- "This symbol is external and not defined here" -> `SHN_UNDEF`
- "This symbol's address is absolute and doesn't move" -> `SHN_ABS`
- "This is unallocated common data" -> `SHN_COMMON`

Section contains all information about object except ELF header, section header and program header

- Every section in object should exactly have one section header
- Sections in a file may not overlap
- An object file may have inactive space

### Header

```c
typedef struct {
    Elf32_Word  sh_name;      // Index into section header string table (name of section)
    Elf32_Word  sh_type;      // Type of section (code, data, symbol table, etc.)
    Elf32_Word  sh_flags;     // Attributes (writable, executable, etc.)
    Elf32_Addr  sh_addr;      // Virtual memory address when loaded, if no things to load then it's value will be 0
    Elf32_Off   sh_offset;    // Offset in the file where this section starts
    Elf32_Word  sh_size;      // Size of the section in bytes
    Elf32_Word  sh_link;      // Link to another section (meaning depends on `sh_type`)
    Elf32_Word  sh_info;      // Extra info (meaning depends on `sh_type`)
    Elf32_Word  sh_addralign; // Required alignment (e.g. 4, 8, 16 bytes)
    Elf32_Word  sh_entsize;   // Size of each entry (for sections with fixed-size entries)
} Elf32_Shdr;

```

**sh_type** 
What kind of section? (e.g., SHT_PROGBITS, SHT_SYMTAB, etc.)

| **Name**       | **Value**    | **What it means (Simple explanation)**                                   |
| -------------- | ------------ | ------------------------------------------------------------------------ |
| `SHT_NULL`     | `0`          | Not used. Just a placeholder.                                            |
| `SHT_PROGBITS` | `1`          | Program code or data (like `.text`, `.rodata`).                          |
| `SHT_SYMTAB`   | `2`          | Full symbol table used by the linker (used in `readelf -s`).             |
| `SHT_STRTAB`   | `3`          | String table (holds names of sections, symbols, etc.).                   |
| `SHT_RELA`     | `4`          | Relocation entries (with extra number called *addend*). Used in x86\_64. |
| `SHT_HASH`     | `5`          | Used for hashing symbols (for faster dynamic symbol lookup).             |
| `SHT_DYNAMIC`  | `6`          | Info used by the dynamic linker at runtime (like which `.so` to load).   |
| `SHT_NOTE`     | `7`          | Used for notes like ABI, build ID (e.g., `.note.ABI-tag`).               |
| `SHT_NOBITS`   | `8`          | Takes up memory but no space in file (e.g., `.bss`).                     |
| `SHT_REL`      | `9`          | Relocation entries (without *addend*). Used in 32-bit ELF.               |
| `SHT_SHLIB`    | `10`         | Reserved. Almost never used.                                             |
| `SHT_DYNSYM`   | `11`         | Smaller symbol table used at runtime (used for dynamic linking).         |
| `SHT_LOPROC`   | `0x70000000` | Start of range for CPU-specific sections.                                |
| `SHT_HIPROC`   | `0x7FFFFFFF` | End of CPU-specific section range.                                       |
| `SHT_LOUSER`   | `0x80000000` | Start of range for user-defined (custom) sections.                       |
| `SHT_HIUSER`   | `0xFFFFFFFF` | End of user-defined section range.                                       |

* SHT_SYMTAB is for debugging/compiling
SHT_SYMTAB is used for link editing  (what is link editing)

| Property         | Description                                            |
| ---------------- | ------------------------------------------------------ |
| **Section Type** | `SHT_SYMTAB` (value = 2)                               |
| **Purpose**      | Used by the linker during static linking               |
| **Contents**     | All symbols: functions, variables, local/global/static |
| **Size**         | Usually large                                          |
| **Present in**   | Executables and object files (`.o`)                    |
| **Stripped in**  | Final binaries (often stripped to reduce size)         |

Use Case:

- Used by compilers and linkers to resolve symbols
- Example: Helps match int x = func(); with the actual definition of func()

* SHT_DYNSYM is used for runtime.

| Property         | Description                                                 |
| ---------------- | ----------------------------------------------------------- |
| **Section Type** | `SHT_DYNSYM` (value = 11)                                   |
| **Purpose**      | Used by dynamic linker/loader at runtime                    |
| **Contents**     | Only symbols needed for dynamic linking (e.g., shared libs) |
| **Size**         | Smaller subset of `SHT_SYMTAB`                              |
| **Present in**   | Shared libraries (`.so`) and dynamically linked executables |
| **Retained in**  | Final binaries for runtime linking                          |

Use Case:

- Used by the dynamic loader (ld.so) to resolve functions like printf, malloc at runtime
- Faster, smaller: doesn't contain unnecessary local/internal symbols


* SHT_RELA

- Relocation adjusts addresses in the binary when it's loaded into memory.
- For example, if a function or variable is referenced before its exact address is known (e.g., external function), a relocation entry tells the linker or loader where and how to fix the address.
- Architectures that require a separate addend value (like x86_64)

| Feature             | `SHT_REL`                      | `SHT_RELA`                     |
| ------------------- | ------------------------------ | ------------------------------ |
| Addend stored where | In the section being relocated | In the relocation entry itself |
| Struct used         | `Elf32_Rel` / `Elf64_Rel`      | `Elf32_Rela` / `Elf64_Rela`    |
| Used by             | x86, ARM (mostly)              | x86\_64, PowerPC, etc.         |



#### Program
[section_header](./section_header.c)