Library is a single unit of object, which will contains multiple binary files. It can be shared between programs thus reduce disk and RAM space

> [!TIP]  
> when compiling a library file or to execute linker manually use `-c` in `gcc`. It will not execute linker

## Types of library
* static library
* shared library

### static library
This type of library is know as `archives`. Instead of list long series of compiled object name, we can make a static lib and mention it's name in program.  
When a program is compiled against static lib, each program has it's local copy of static library

#### Creating a shared lib
`ar option archieve obj-file ....`
```
option
r - Insert an object into archive, replace any previous object, using r option we can modify the objects in lib at any time
t - Display a table of content of the archives
v - When v option is combined with t, we can see additional informations
d - delete a named module from archieves
s – create an index (faster symbol lookup)
```

#### Using a shared lib
We can link a program against static library in two types.

##### place the library as a part of linking command

```
cc -g -c prog.c # program 
cc -g -o prog prog.o libdemo.a
```

##### using from directort
**std directory**  
`/usr/lib` is one of the std path for libraries. To mention the library name use `-l` option without `lib`(prefix) ans `.a` (suffix)
```
cc -g -o prog prog.o -ldemo # actual name of the static lib is libdemo.a
```

**custom directory**  
To include custom lib directory mention `-L` flag followed by path  
`cc -g -o prog prog.o -Lmylibdir -ldemo` #or -L./mylibdir

### shared library
Idea here is single copy of shared between multiple programs, The object modules are not copied into the linked executables. So it is possible to make changes to the object modules without requring programs to be relinked

Although the code of the shared library is shared among the multiple process, it's variables are not, Each process that uses library has it's own copies of the global and static variables

Unlike static library we can't add, update or delete individula module.

#### Creation of shared lib
1. `gcc -g -c -fPIC -wall mod1.c mod2.c mod3.c`  
2. `gcc -g -shared -o libfoo.so mod1.o mod2.o mod3.o`  

That `-shared` is used to create shared library from object files. We can combine the 2 steps into single one

1. `gcc -g -fPIC -wall mod1.c mod2.c mod3.c -shared -o libfoo.so`

`-fPIC` means generate a position independent code. (This allows the code to be located at any virtual address at run time)

Shared library prefix with `lib` and suffix with `.so`  

There can be different version of shared library present in the system.

#### Using shared library

1. (Compile phase)  Embedding Library Information
When you link a program with a shared library:  `gcc main.c -L. -lmymath -o myprog`
- The linker (ld) does not copy the code of libmymath.so into myprog.
- Instead, it embeds a reference to the library inside the executable, using the ELF dynamic section (DT_NEEDED tag).
You can see which libraries are needed by your program using: `readelf -d myprog | grep NEEDED`
``` 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]```  
To check the shared library with the system path use `ldd myprog`  
```
        linux-vdso.so.1 (0x00007fffa9db5000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x0000775838a00000)
        /lib64/ld-linux-x86-64.so.2 (0x0000775838d26000)
```
2. Runtime Resolution (Dynamic Loader)
When you run ./myprog, the dynamic loader (ld.so or ld-linux.so) is responsible for:
- Reading the DT_NEEDED entries from the executable’s ELF headers.
- Finding the actual shared library files (libmymath.so, libc.so.6, etc.).
- Mapping them into the process address space using mmap().
- Resolving all external symbols (function and variable addresses).

If a shared library is already loaded (e.g., another program is using it), the loader reuses the existing copy in memory.

The loader searches for libraries in this order:
1. LD_LIBRARY_PATH (environment variable). `export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH`
2. DT_RPATH / DT_RUNPATH in the executable (if set at link time using -Wl,-rpath).
3. Cache: /etc/ld.so.cache (managed by ldconfig).
4. System directories: /lib, /usr/lib, /lib64, etc.
