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