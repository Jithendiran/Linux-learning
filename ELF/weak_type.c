/*
They’re useful when:

- You want default behavior, but allow others to override it.

- You’re writing code that should work even if some functions are missing (like optional libraries or features).
*/

/*
gcc -c weak_type.c
gcc -c weak_global.c

gcc weak_type.o weak_global.o -o weak.o

*/
#include <stdio.h>

int uninit_var; // symbold table Ndx's common type
int int_var = 34;

// Declare a weak symbol using GCC attribute
__attribute__((weak)) void hello() {
    printf("Hello from WEAK symbol\n");
}

__attribute__((weak)) void hello2() {
    printf("Hello2 from WEAK symbol\n");
}

extern void feature() __attribute__((weak));

// global variant is undefined
__attribute__((weak)) int uninit_var = 2234;

// global varient is defined
// __attribute__((weak)) int int_var = 1234; // error: redefinition of ‘int_var’

// multiple weak init
__attribute__((weak)) int int_var2 = 89;
// __attribute__((weak)) int int_var2 = 69; // : error: redefinition of ‘int_var2’


__attribute__((weak)) int local = 2233;

int main() {
    int local = 4567;

    printf("local = %d\n", local); // local = 4567
    // precedence of local is higher

    hello(); //Hello from STRONG symbol
    // precedence of Global is higher

    hello2();  // Hello2 from WEAK symbol
    

     if (feature)
        feature();  // only calls if present
    
    printf("Common Type : %d\n", uninit_var);   // Common Type : 2234
    return 0;
}


/*
Symbol table '.symtab' contains 14 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS weak_type.c
     2: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 .text
     3: 0000000000000000     0 SECTION LOCAL  DEFAULT    5 .rodata
     4: 0000000000000000     4 OBJECT  WEAK   DEFAULT    3 uninit_var
     5: 0000000000000004     4 OBJECT  GLOBAL DEFAULT    3 int_var
     6: 0000000000000000    26 FUNC    WEAK   DEFAULT    1 hello
     7: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND puts
     8: 000000000000001a    26 FUNC    WEAK   DEFAULT    1 hello2
     9: 0000000000000008     4 OBJECT  WEAK   DEFAULT    3 int_var2
    10: 0000000000000034    85 FUNC    GLOBAL DEFAULT    1 main
    11: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND _GLOBAL_OFFSET_TABLE_
    12: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND feature
    13: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND printf
*/