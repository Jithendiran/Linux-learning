#include <stdio.h>

// Provide a strong (global) definition of the same symbol
void hello() {
    printf("Hello from STRONG symbol\n");
}

/*

Symbol table '.symtab' contains 6 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS weak_global.c
     2: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 .text
     3: 0000000000000000     0 SECTION LOCAL  DEFAULT    5 .rodata
     4: 0000000000000000    26 FUNC    GLOBAL DEFAULT    1 hello
     5: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND puts
*/