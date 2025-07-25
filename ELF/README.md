The Executable and Linking Format was originally developed and published by UNIX System Laboratories (USL) as part of the Application Binary Interface (ABI). The Tool Interface Standards committee(TIS) has selected the evolving ELF standard as a portable object file format that works on 32-bit Intel Architecture environments for a variety of operating systems.

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
