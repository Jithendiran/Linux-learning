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
 * readelf -s section_header_debug
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