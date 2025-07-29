// gcc -g -O0 -no-pie -fcommon -c -o section_header section_header.c
/*Since GCC 10, the default is no longer to generate COMMON symbols. It uses .bss by default.
To revert to old behavior and get COMMON symbols, compile with: -fcommon
 */
// This code is a simple C program that demonstrates the generation of an ELF file
// with various sections like .text, .data, .bss, and .rodata
// It includes a function and some global variables to illustrate the ELF structure.
#include <stdio.h>
extern int j;
int comm;
int global_var = 42;          // .data
const int const_val = 100;    // .rodata
static int static_var = 10;   // .data (local)
int uninit_var;               // .bss

void func() {                 // .text
    printf("Hello ELF\n");
}

int main() {
    func();
    return 0;
}

/**
RESERVED SECTIONS
---------------
 
readelf -s section_header_debug
Symbol table '.symtab' contains 18 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS section_header.c -> It is not associated with any section, so it is marked as ABS
     2: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 .text
     3: 0000000000000000     0 SECTION LOCAL  DEFAULT    3 .data
     4: 0000000000000000     0 SECTION LOCAL  DEFAULT    5 .rodata
     5: 0000000000000004     4 OBJECT  LOCAL  DEFAULT    3 static_var
     6: 0000000000000000     0 SECTION LOCAL  DEFAULT    6 .debug_info
     7: 0000000000000000     0 SECTION LOCAL  DEFAULT    8 .debug_abbrev
     8: 0000000000000000     0 SECTION LOCAL  DEFAULT   11 .debug_line
     9: 0000000000000000     0 SECTION LOCAL  DEFAULT   13 .debug_str
    10: 0000000000000000     0 SECTION LOCAL  DEFAULT   14 .debug_line_str
    11: 0000000000000004     4 OBJECT  GLOBAL DEFAULT  COM comm
    12: 0000000000000000     4 OBJECT  GLOBAL DEFAULT    3 global_var
    13: 0000000000000000     4 OBJECT  GLOBAL DEFAULT    5 const_val
    14: 0000000000000004     4 OBJECT  GLOBAL DEFAULT  COM uninit_var      -> Declaration without initialization, so it is in .bss/COMMON section
    15: 0000000000000000    26 FUNC    GLOBAL DEFAULT    1 func
    16: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND puts            -> symbol not defined in this file, so it is marked as undefined
    17: 000000000000001a    25 FUNC    GLOBAL DEFAULT    1 main

Ndx
UND -> `SHN_UNDEF`
ABS -> `SHN_ABS`
    These FILE entries help symbol table consumers (like debuggers) to know:
    - Which file the following symbols (functions, variables) came from.
    - They don’t occupy memory or represent code/data — they are just tags.
COM -> `SHN_COMMON`

1,2.. -> hese are indexes into the Section Header Table, which you can see using: `readelf -S section_header_debug` in [Nr]

There are 23 section headers, starting at offset 0xb98:

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .text             PROGBITS         0000000000000000  00000040
       0000000000000033  0000000000000000  AX       0     0     1
  [ 2] .rela.text        RELA             0000000000000000  00000718
       0000000000000048  0000000000000018   I      20     1     8
  [ 3] .data             PROGBITS         0000000000000000  00000074
       0000000000000008  0000000000000000  WA       0     0     4
  [ 4] .bss              NOBITS           0000000000000000  0000007c
       0000000000000000  0000000000000000  WA       0     0     1
  [ 5] .rodata           PROGBITS         0000000000000000  0000007c
       000000000000000e  0000000000000000   A       0     0     4
  [ 6] .debug_info       PROGBITS         0000000000000000  0000008a
       0000000000000115  0000000000000000           0     0     1
  [ 7] .rela.debug_info  RELA             0000000000000000  00000760
       00000000000002a0  0000000000000018   I      20     6     8
  [ 8] .debug_abbrev     PROGBITS         0000000000000000  0000019f
       0000000000000086  0000000000000000           0     0     1
  [ 9] .debug_aranges    PROGBITS         0000000000000000  00000225
       0000000000000030  0000000000000000           0     0     1
  [10] .rela.debug_[...] RELA             0000000000000000  00000a00
       0000000000000030  0000000000000018   I      20     9     8
  [11] .debug_line       PROGBITS         0000000000000000  00000255
       000000000000005d  0000000000000000           0     0     1
  [12] .rela.debug_line  RELA             0000000000000000  00000a30
       0000000000000060  0000000000000018   I      20    11     8
  [13] .debug_str        PROGBITS         0000000000000000  000002b2
       0000000000000139  0000000000000001  MS       0     0     1
  [14] .debug_line_str   PROGBITS         0000000000000000  000003eb
       000000000000007f  0000000000000001  MS       0     0     1
  [15] .comment          PROGBITS         0000000000000000  0000046a
       000000000000002c  0000000000000001  MS       0     0     1
  [16] .note.GNU-stack   PROGBITS         0000000000000000  00000496
       0000000000000000  0000000000000000           0     0     1
  [17] .note.gnu.pr[...] NOTE             0000000000000000  00000498
       0000000000000020  0000000000000000   A       0     0     8
  [18] .eh_frame         PROGBITS         0000000000000000  000004b8
       0000000000000058  0000000000000000   A       0     0     8
  [19] .rela.eh_frame    RELA             0000000000000000  00000a90
       0000000000000030  0000000000000018   I      20    18     8
  [20] .symtab           SYMTAB           0000000000000000  00000510
       00000000000001b0  0000000000000018          21    11     8
  [21] .strtab           STRTAB           0000000000000000  000006c0
       0000000000000051  0000000000000000           0     0     1
  [22] .shstrtab         STRTAB           0000000000000000  00000ac0
       00000000000000d3  0000000000000000           0     0     1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)
*/



/*
SYMBOL TABLE
---------------

gcc -g -O0 -no-pie -fcommon  -o section_header.o section_header.c
readelf -s section_header.o

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

dynsym is produced by the linker and contains dynamic symbols that are used for dynamic linking at runtime.
symtab is produced by the compiler and contains static symbols that are used for linking at compile time

if -c flag is used, the compiler does not produce a dynamic symbol table, so the .dynsym section is not present.
The .symtab section is still present, containing static symbols.
*/


/*
RELOCATION
---------------

jidesh@jidesh-MS-7E26:/media/ssd/Project/Linux-learning/ELF$ gcc -g -O0 -no-pie -fcommon -c -o section_header_c.o section_header.c
jidesh@jidesh-MS-7E26:/media/ssd/Project/Linux-learning/ELF$ readelf -r section_header_c.o

Relocation section '.rela.text' at offset 0x718 contains 3 entries:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
00000000000b  000400000002 R_X86_64_PC32     0000000000000000 .rodata + 0
000000000013  001000000004 R_X86_64_PLT32    0000000000000000 puts - 4
000000000028  000f00000004 R_X86_64_PLT32    0000000000000000 func - 4

Relocation section '.rela.debug_info' at offset 0x760 contains 28 entries:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000000008  00070000000a R_X86_64_32       0000000000000000 .debug_abbrev + 0
00000000000d  00090000000a R_X86_64_32       0000000000000000 .debug_str + 6c
000000000012  000a0000000a R_X86_64_32       0000000000000000 .debug_line_str + 0
000000000016  000a0000000a R_X86_64_32       0000000000000000 .debug_line_str + 11
00000000001a  000200000001 R_X86_64_64       0000000000000000 .text + 0
00000000002a  00080000000a R_X86_64_32       0000000000000000 .debug_line + 0
000000000031  00090000000a R_X86_64_32       0000000000000000 .debug_str + 2e
000000000038  00090000000a R_X86_64_32       0000000000000000 .debug_str + 0
00000000003f  00090000000a R_X86_64_32       0000000000000000 .debug_str + 45
000000000046  00090000000a R_X86_64_32       0000000000000000 .debug_str + 107
00000000004d  00090000000a R_X86_64_32       0000000000000000 .debug_str + 18
000000000054  00090000000a R_X86_64_32       0000000000000000 .debug_str + 124
000000000067  00090000000a R_X86_64_32       0000000000000000 .debug_str + 63
00000000006e  00090000000a R_X86_64_32       0000000000000000 .debug_str + 40
000000000073  00090000000a R_X86_64_32       0000000000000000 .debug_str + 11a
00000000007f  000b00000001 R_X86_64_64       0000000000000004 comm + 0
000000000088  00090000000a R_X86_64_32       0000000000000000 .debug_str + 58
000000000094  000c00000001 R_X86_64_64       0000000000000000 global_var + 0
00000000009d  00090000000a R_X86_64_32       0000000000000000 .debug_str + 24
0000000000a9  000d00000001 R_X86_64_64       0000000000000000 const_val + 0
0000000000b2  00090000000a R_X86_64_32       0000000000000000 .debug_str + 12e
0000000000bf  000300000001 R_X86_64_64       0000000000000000 .data + 4
0000000000c8  00090000000a R_X86_64_32       0000000000000000 .debug_str + d
0000000000d4  000e00000001 R_X86_64_64       0000000000000004 uninit_var + 0
0000000000dd  00090000000a R_X86_64_32       0000000000000000 .debug_str + 53
0000000000e8  000200000001 R_X86_64_64       0000000000000000 .text + 1a
0000000000fb  00090000000a R_X86_64_32       0000000000000000 .debug_str + 11f
000000000102  000200000001 R_X86_64_64       0000000000000000 .text + 0

Relocation section '.rela.debug_aranges' at offset 0xa00 contains 2 entries:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000000006  00060000000a R_X86_64_32       0000000000000000 .debug_info + 0
000000000010  000200000001 R_X86_64_64       0000000000000000 .text + 0

Relocation section '.rela.debug_line' at offset 0xa30 contains 4 entries:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000000022  000a0000000a R_X86_64_32       0000000000000000 .debug_line_str + 37
00000000002c  000a0000000a R_X86_64_32       0000000000000000 .debug_line_str + 5d
000000000031  000a0000000a R_X86_64_32       0000000000000000 .debug_line_str + 6e
00000000003b  000200000001 R_X86_64_64       0000000000000000 .text + 0

Relocation section '.rela.eh_frame' at offset 0xa90 contains 2 entries:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000000020  000200000002 R_X86_64_PC32     0000000000000000 .text + 0
000000000040  000200000002 R_X86_64_PC32     0000000000000000 .text + 1a
*/