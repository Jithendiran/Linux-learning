## initramfs

This gives you minimal init file, this is the minimal initramfs.img required to debug kernel from start to first process init

### How to create minimal

1. `$ cd /tmp`
2. `$ vi simple_init.c`
```c
#include <stdio.h>
#include <unistd.h>

void main() {
    printf("Hello! I am the only program running on this computer.\n");
    while(1) {
        sleep(1000); // Prevents CPU burnout and PID 1 from exiting
    }
}
```

3. `$ gcc -static -o init simple_init.c`  
The -static flag is critical because your tiny initramfs will not contain the standard C libraries (libc, etc.) that are normally required for dynamically linked programs.

4. `$ mkdir initrd` 
5. `$ cp init initrd/`
6. `$ cd initrd`
7. `$ find . -print0 | cpio --null -o --format=newc > ../initramfs.img`
8. `$ cd ..`
9. `$ rm -r initrd`
10. This step is to verify the `initramfs.img`

`$ lsinitramfs ./initramfs.img `

```sh
.
init
cpio: premature end of archive
```



### Run

Terminal one
```sh
$ qemu-system-x86_64 \
    -kernel bzImage \ # bzImage usually found in arch/x86/boot/bzImage
    -initrd initramfs.img \ # This is the newely created 
    -nographic \
    -append "console=ttyS0 nokaslr" \
    -s -S # This option required only to connect with debugger
```
Terminal 2
`$ gdb vmlinux`
`(gdb) target remote :1234`
`(gdb) hbreak start_kernel`