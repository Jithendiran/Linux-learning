A `program` is a file containing a range of information that describes how to construct a `process` at run time.

Informations in file are:  
**Binary format identifier**:

    Format of the program. Widely used formats are in olden days `a.out(assembler output)` and  `COFF(Common Object File Format)`. Now a days `ELF(Executable and Linking Format)`.

    `file ./a.out`
**Machine instructions**:

    Contains compiled machine code (.text section). CPU executes this code when the process runs.  
 
**Program entry-point address**:

    Address where execution starts (usually main() after some runtime startup).

    `readelf -h a.out | grep Entry`

**Data**:

    Values used to initialize variables and Constants
    .data: global/static variables with initial values

    .rodata: string literals, consts

    .bss: globals/statics set to 0

    `readelf -S a.out`


**Symbol and relocation table**:

*Symbol table*:

    A symbol is a name associated with an address in your code or data for eg: functions (main, printf,..), variables (int c, ab ,...). This symbol table is maintained by compiler and linker that maps the symbol name and address for that to refer. It is used for Linking, debugging, dynamic loading,..

    To view symbol table `nm a.out` or `readelf -s a.out`


*Relocation table*:

    Relocation is the process of adjusting addresses in your program after linking or loading, especially when: You use external functions like printf, You use shared libraries, The loader loads your program at a different address

    To view relocation table `readelf -r a.out`

**Shared-library and dynamic-linking information**:

    List of shared libraries that the program needs at run time and the pathname of the dynamic linker that should load these libraries

    `readelf -d a.out` for loader and `readelf -S a.out` for linker

**Other information**:
    Other information to describe the construction of a process

One program can be used to construct many processes, or many processes may be running the same program.
According to the kernel, a process consists of user-space memory (code, variables, etc.) and kernel-space memory (a range of kernel data structures that maintain the state of the process, such as IDs, virtual memory tables, open file descriptors, signal delivery and handling, etc.).