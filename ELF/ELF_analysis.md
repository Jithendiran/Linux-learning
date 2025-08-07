ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00            // Metadata, architecture(arm, x86,..), endians,..
  Class:                             ELF64                            // 32bit or 64 bit
  Data:                              2's complement, little endian    // endian
  Version:                           1 (current)                      
  OS/ABI:                            UNIX - System V                   
  ABI Version:                       0
  Type:                              EXEC (Executable file)           // Type of the ELF
  Machine:                           Advanced Micro Devices X86-64    // target architecture
  Version:                           0x1
  Entry point address:               0x401050
  Start of program headers:          64 (bytes into file)             // segment start
  Start of section headers:          15088 (bytes into file)          // section start
  Flags:                             0x0            
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         13
  Size of section headers:           64 (bytes)
  Number of section headers:         37
  Section header string table index: 36

// This table helps tools like the compiler, linker, and kernel understand how to use the ELF file.
// Think of an ELF file as a book: the section headers are like the table of contents, telling you what each chapter (section) contains, where it starts, and how it’s used (e.g., read-only, executable).

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .interp           PROGBITS         0000000000400318  00000318       // Specifies the path to the dynamic linker for dynamically linked executables.
       000000000000001c  0000000000000000   A       0     0     1
  [ 2] .note.gnu.pr[...] NOTE             0000000000400338  00000338
       0000000000000030  0000000000000000   A       0     0     8
  [ 3] .note.gnu.bu[...] NOTE             0000000000400368  00000368
       0000000000000024  0000000000000000   A       0     0     4
  [ 4] .note.ABI-tag     NOTE             000000000040038c  0000038c
       0000000000000020  0000000000000000   A       0     0     4
  [ 5] .gnu.hash         GNU_HASH         00000000004003b0  000003b0
       000000000000001c  0000000000000000   A       6     0     8
  [ 6] .dynsym           DYNSYM           00000000004003d0  000003d0       // .dynsym holds symbols (e.g., function names) for dynamic linking; Used by the dynamic linker to resolve function/variable references at runtime
       0000000000000060  0000000000000018   A       7     1     8
  [ 7] .dynstr           STRTAB           0000000000400430  00000430       // .dynstr stores their string names
       0000000000000048  0000000000000000   A       0     0     1
  [ 8] .gnu.version      VERSYM           0000000000400478  00000478
       0000000000000008  0000000000000002   A       6     0     2
  [ 9] .gnu.version_r    VERNEED          0000000000400480  00000480
       0000000000000030  0000000000000000   A       7     1     8
  [10] .rela.dyn         RELA             00000000004004b0  000004b0       // Relocation entries for dynamic linking. Advanced but useful for understanding how addresses are resolved.
       0000000000000030  0000000000000018   A       6     0     8
  [11] .rela.plt         RELA             00000000004004e0  000004e0
       0000000000000018  0000000000000018  AI       6    24     8
  [12] .init             PROGBITS         0000000000401000  00001000       // Code for program initialization, before main
       000000000000001b  0000000000000000  AX       0     0     4
  [13] .plt              PROGBITS         0000000000401020  00001020       // Procedure Linkage Table, contains code stubs for calling functions in shared libraries. Facilitates dynamic linking by providing a way to call external functions. Allows programs to call library functions without knowing their addresses at compile time.
       0000000000000020  0000000000000010  AX       0     0     16
  [14] .plt.sec          PROGBITS         0000000000401040  00001040
       0000000000000010  0000000000000010  AX       0     0     16
  [15] .text             PROGBITS         0000000000401050  00001050       // This contains the program’s executable code
       0000000000000119  0000000000000000  AX       0     0     16
  [16] .fini             PROGBITS         000000000040116c  0000116c       // Code for program cleanup, after main
       000000000000000d  0000000000000000  AX       0     0     4
  [17] .rodata           PROGBITS         0000000000402000  00002000       // Stores read-only data, like string literals (e.g., "Hello, World!").
       0000000000000012  0000000000000000   A       0     0     4
  [18] .eh_frame_hdr     PROGBITS         0000000000402014  00002014       // Exception handling data. Relevant for C++ or advanced debugging.
       000000000000003c  0000000000000000   A       0     0     4
  [19] .eh_frame         PROGBITS         0000000000402050  00002050
       00000000000000c4  0000000000000000   A       0     0     8
  [20] .init_array       INIT_ARRAY       0000000000403e10  00002e10
       0000000000000008  0000000000000008  WA       0     0     8
  [21] .fini_array       FINI_ARRAY       0000000000403e18  00002e18
       0000000000000008  0000000000000008  WA       0     0     8
  [22] .dynamic          DYNAMIC          0000000000403e20  00002e20       // Contains metadata for dynamic linking (e.g., library dependencies, relocation info). Guides the dynamic linker on how to load and link the program.
       00000000000001d0  0000000000000010  WA       7     0     8
  [23] .got              PROGBITS         0000000000403ff0  00002ff0       // Global Offset Table (.got) and Procedure Linkage Table (.got.plt) store addresses for dynamic linking. They help the dynamic linker resolve function and variable addresses at runtime.
       0000000000000010  0000000000000008  WA       0     0     8
  [24] .got.plt          PROGBITS         0000000000404000  00003000
       0000000000000020  0000000000000008  WA       0     0     8
  [25] .data             PROGBITS         0000000000404020  00003020       // Stores initialized global/static variables
       0000000000000018  0000000000000000  WA       0     0     8
  [26] .bss              NOBITS           0000000000404038  00003038       // Stores uninitialized global/static variable, Takes no space in the file (type NOBITS) but reserves space in memory.
       0000000000000010  0000000000000000  WA       0     0     4
  [27] .comment          PROGBITS         0000000000000000  00003038
       000000000000002b  0000000000000001  MS       0     0     1
  [28] .debug_aranges    PROGBITS         0000000000000000  00003063
       0000000000000030  0000000000000000           0     0     1
  [29] .debug_info       PROGBITS         0000000000000000  00003093       // Debugging info (e.g., .debug_info, .debug_line) for tools like gdb. elps map machine code to source code during debugging.
       0000000000000115  0000000000000000           0     0     1 
  [30] .debug_abbrev     PROGBITS         0000000000000000  000031a8
       0000000000000086  0000000000000000           0     0     1
  [31] .debug_line       PROGBITS         0000000000000000  0000322e
       000000000000005d  0000000000000000           0     0     1
  [32] .debug_str        PROGBITS         0000000000000000  0000328b
       000000000000011b  0000000000000001  MS       0     0     1
  [33] .debug_line_str   PROGBITS         0000000000000000  000033a6
       0000000000000037  0000000000000001  MS       0     0     1
  [34] .symtab           SYMTAB           0000000000000000  000033e0       // .symtab holds all symbols (functions, variables); .strtab stores their names. Unlike .dynsym, these are used for static linking and debugging. Critical for debugging and contributing to projects requiring symbol analysis.
       00000000000003c0  0000000000000018          35    19     8
  [35] .strtab           STRTAB           0000000000000000  000037a0
       00000000000001df  0000000000000000           0     0     1
  [36] .shstrtab         STRTAB           0000000000000000  0000397f
       000000000000016f  0000000000000000           0     0     1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),          // A -> Alloc (loaded into memory)
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)

There are no section groups in this file.

// Segmants
// It describes how to create a image in memory
Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  PHDR           0x0000000000000040 0x0000000000400040 0x0000000000400040
                 0x00000000000002d8 0x00000000000002d8  R      0x8

------------------------------------------------------------------------------
// INTERP -> Identifies the runtime linker for dynamic linking
  INTERP         0x0000000000000318 0x0000000000400318 0x0000000000400318
                 0x000000000000001c 0x000000000000001c  R      0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]

------------------------------------------------------------------------------------------
// LOAD -> Describes memory areas for code, data, and BSS, with zero-filling
  LOAD           0x0000000000000000 0x0000000000400000 0x0000000000400000 
                 0x00000000000004f8 0x00000000000004f8  R      0x1000

  LOAD           0x0000000000001000 0x0000000000401000 0x0000000000401000
                 0x0000000000000179 0x0000000000000179  R E    0x1000

  LOAD           0x0000000000002000 0x0000000000402000 0x0000000000402000
                 0x0000000000000114 0x0000000000000114  R      0x1000

  LOAD           0x0000000000002e10 0x0000000000403e10 0x0000000000403e10
                 0x0000000000000228 0x0000000000000238  RW     0x1000

-----------------------------------------------------------------------------------------------
  DYNAMIC        0x0000000000002e20 0x0000000000403e20 0x0000000000403e20
                 0x00000000000001d0 0x00000000000001d0  RW     0x8
  NOTE           0x0000000000000338 0x0000000000400338 0x0000000000400338
                 0x0000000000000030 0x0000000000000030  R      0x8
  NOTE           0x0000000000000368 0x0000000000400368 0x0000000000400368
                 0x0000000000000044 0x0000000000000044  R      0x4
  GNU_PROPERTY   0x0000000000000338 0x0000000000400338 0x0000000000400338
                 0x0000000000000030 0x0000000000000030  R      0x8
  GNU_EH_FRAME   0x0000000000002014 0x0000000000402014 0x0000000000402014
                 0x000000000000003c 0x000000000000003c  R      0x4

----------------------------------------------------------------------------------------------------
// GNU_STACK -> Indicates stack executability, important for security.
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10

------------------------------------------------------------------------------------------------------

  GNU_RELRO      0x0000000000002e10 0x0000000000403e10 0x0000000000403e10
                 0x00000000000001f0 0x00000000000001f0  R      0x1

 Section to Segment mapping:
  Segment Sections...
   00     
   01     .interp 
   02     .interp .note.gnu.property .note.gnu.build-id .note.ABI-tag .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.dyn .rela.plt 
   03     .init .plt .plt.sec .text .fini 
   04     .rodata .eh_frame_hdr .eh_frame 
   05     .init_array .fini_array .dynamic .got .got.plt .data .bss 
   06     .dynamic 
   07     .note.gnu.property 
   08     .note.gnu.build-id .note.ABI-tag 
   09     .note.gnu.property 
   10     .eh_frame_hdr 
   11     
   12     .init_array .fini_array .dynamic .got 

Dynamic section at offset 0x2e20 contains 24 entries:
  Tag        Type                         Name/Value
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
 0x000000000000000c (INIT)               0x401000
 0x000000000000000d (FINI)               0x40116c
 0x0000000000000019 (INIT_ARRAY)         0x403e10
 0x000000000000001b (INIT_ARRAYSZ)       8 (bytes)
 0x000000000000001a (FINI_ARRAY)         0x403e18
 0x000000000000001c (FINI_ARRAYSZ)       8 (bytes)
 0x000000006ffffef5 (GNU_HASH)           0x4003b0
 0x0000000000000005 (STRTAB)             0x400430
 0x0000000000000006 (SYMTAB)             0x4003d0
 0x000000000000000a (STRSZ)              72 (bytes)
 0x000000000000000b (SYMENT)             24 (bytes)
 0x0000000000000015 (DEBUG)              0x0
 0x0000000000000003 (PLTGOT)             0x404000
 0x0000000000000002 (PLTRELSZ)           24 (bytes)
 0x0000000000000014 (PLTREL)             RELA
 0x0000000000000017 (JMPREL)             0x4004e0
 0x0000000000000007 (RELA)               0x4004b0
 0x0000000000000008 (RELASZ)             48 (bytes)
 0x0000000000000009 (RELAENT)            24 (bytes)
 0x000000006ffffffe (VERNEED)            0x400480
 0x000000006fffffff (VERNEEDNUM)         1
 0x000000006ffffff0 (VERSYM)             0x400478
 0x0000000000000000 (NULL)               0x0

Relocation section '.rela.dyn' at offset 0x4b0 contains 2 entries:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000403ff0  000100000006 R_X86_64_GLOB_DAT 0000000000000000 __libc_start_main@GLIBC_2.34 + 0
000000403ff8  000300000006 R_X86_64_GLOB_DAT 0000000000000000 __gmon_start__ + 0

Relocation section '.rela.plt' at offset 0x4e0 contains 1 entry:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000404018  000200000007 R_X86_64_JUMP_SLO 0000000000000000 puts@GLIBC_2.2.5 + 0
No processor specific unwind information to decode

Symbol table '.dynsym' contains 4 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND _[...]@GLIBC_2.34 (2)
     2: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND puts@GLIBC_2.2.5 (3)
     3: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND __gmon_start__

Symbol table '.symtab' contains 40 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS crt1.o
     2: 000000000040038c    32 OBJECT  LOCAL  DEFAULT    4 __abi_tag
     3: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS crtstuff.c
     4: 0000000000401090     0 FUNC    LOCAL  DEFAULT   15 deregister_tm_clones
     5: 00000000004010c0     0 FUNC    LOCAL  DEFAULT   15 register_tm_clones
     6: 0000000000401100     0 FUNC    LOCAL  DEFAULT   15 __do_global_dtors_aux
     7: 0000000000404038     1 OBJECT  LOCAL  DEFAULT   26 completed.0
     8: 0000000000403e18     0 OBJECT  LOCAL  DEFAULT   21 __do_global_dtor[...]
     9: 0000000000401130     0 FUNC    LOCAL  DEFAULT   15 frame_dummy
    10: 0000000000403e10     0 OBJECT  LOCAL  DEFAULT   20 __frame_dummy_in[...]
    11: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS section_header.c
    12: 0000000000404034     4 OBJECT  LOCAL  DEFAULT   25 static_var
    13: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS crtstuff.c
    14: 0000000000402110     0 OBJECT  LOCAL  DEFAULT   19 __FRAME_END__
    15: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS 
    16: 0000000000403e20     0 OBJECT  LOCAL  DEFAULT   22 _DYNAMIC
    17: 0000000000402014     0 NOTYPE  LOCAL  DEFAULT   18 __GNU_EH_FRAME_HDR
    18: 0000000000404000     0 OBJECT  LOCAL  DEFAULT   24 _GLOBAL_OFFSET_TABLE_
    19: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND __libc_start_mai[...]
    20: 0000000000404020     0 NOTYPE  WEAK   DEFAULT   25 data_start
    21: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND puts@GLIBC_2.2.5
    22: 000000000040403c     4 OBJECT  GLOBAL DEFAULT   26 uninit_var
    23: 0000000000404038     0 NOTYPE  GLOBAL DEFAULT   25 _edata
    24: 000000000040116c     0 FUNC    GLOBAL HIDDEN    16 _fini
    25: 0000000000404030     4 OBJECT  GLOBAL DEFAULT   25 global_var
    26: 0000000000404020     0 NOTYPE  GLOBAL DEFAULT   25 __data_start
    27: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND __gmon_start__
    28: 0000000000404028     0 OBJECT  GLOBAL HIDDEN    25 __dso_handle
    29: 0000000000402000     4 OBJECT  GLOBAL DEFAULT   17 _IO_stdin_used
    30: 0000000000401136    26 FUNC    GLOBAL DEFAULT   15 func
    31: 0000000000404040     4 OBJECT  GLOBAL DEFAULT   26 comm
    32: 0000000000404048     0 NOTYPE  GLOBAL DEFAULT   26 _end
    33: 0000000000401080     5 FUNC    GLOBAL HIDDEN    15 _dl_relocate_sta[...]
    34: 0000000000401050    38 FUNC    GLOBAL DEFAULT   15 _start
    35: 0000000000404038     0 NOTYPE  GLOBAL DEFAULT   26 __bss_start
    36: 0000000000401150    25 FUNC    GLOBAL DEFAULT   15 main
    37: 0000000000402004     4 OBJECT  GLOBAL DEFAULT   17 const_val
    38: 0000000000404038     0 OBJECT  GLOBAL HIDDEN    25 __TMC_END__
    39: 0000000000401000     0 FUNC    GLOBAL HIDDEN    12 _init

Version symbols section '.gnu.version' contains 4 entries:
 Addr: 0x0000000000400478  Offset: 0x000478  Link: 6 (.dynsym)
  000:   0 (*local*)       2 (GLIBC_2.34)    3 (GLIBC_2.2.5)   1 (*global*)   

Version needs section '.gnu.version_r' contains 1 entry:
 Addr: 0x0000000000400480  Offset: 0x000480  Link: 7 (.dynstr)
  000000: Version: 1  File: libc.so.6  Cnt: 2
  0x0010:   Name: GLIBC_2.2.5  Flags: none  Version: 3
  0x0020:   Name: GLIBC_2.34  Flags: none  Version: 2

Displaying notes found in: .note.gnu.property
  Owner                Data size        Description
  GNU                  0x00000020       NT_GNU_PROPERTY_TYPE_0
      Properties: x86 feature: IBT, SHSTK
        x86 ISA needed: x86-64-baseline

Displaying notes found in: .note.gnu.build-id
  Owner                Data size        Description
  GNU                  0x00000014       NT_GNU_BUILD_ID (unique build ID bitstring)
    Build ID: 03fd14f6480a11aed36011f8f8518288606ea227

Displaying notes found in: .note.ABI-tag
  Owner                Data size        Description
  GNU                  0x00000010       NT_GNU_ABI_TAG (ABI version tag)
    OS: Linux, ABI: 3.2.0