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

#### Program
[section_header](./section_header.c)