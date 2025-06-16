#include<stdio.h>
#include<signal.h>
#include<unistd.h>

/*
signal
void ( *signal(int sig, void (*handler)(int)) ) (int);

void ( 
    *signal( int sig, void (*handler)(int) ) 
) (int);

How to read
1.void (*handler)(int) - This is a function pointer :
   * Parameter: int
   * Return : void

2. signal(int sig, void (*handler)(int)) - The signal function takes:
    * Parameter: int ,the signal number
    * Parameter: function pointer ,void (*handler)(int) - a function pointer (the signal handler)
    * Return type: Pointer to a function void (*)(int)

3. void (*signal(...))(int) or void (*)(int) - The signal function returns:
    * A function pointer that takes an int and returns void
*/

/* rewrite as

typedef void (*sighandler_t) (int);
sighandler_t signal(int sig, sighandler_t newHandler);
*/


void newHandler(int signal){
    printf("New signal handler : %d\n",signal);
}

int main(){
    void (*oldhandler)(int);
    oldhandler = signal(SIGINT, newHandler);
    if(oldhandler == SIG_ERR) {
        perror("SIGINT Signal disposition");
    }
    printf("New handler installed - Press Ctrl+C\n");
    sleep(15);  // Give time to test old handler, ctrl+c will interrupt the sleep
    printf("New handler sleep completed\n");
    
    sleep(10);
    // signal(SIGINT, SIG_DFL); // reset to default handler
    // signal(SIGINT, SIG_IGN); // ignore the signal
    if(signal(SIGINT, oldhandler) == SIG_ERR){
        perror("Replacement of old handler error");
    }
    printf("Completed\n");
    return 0;
}

/*
New handler installed - Press Ctrl+C
^C
New signal handler : 2
    -- wait for 10 sec
New handler sleep completed
Completed
*/
/*
New handler installed - Press Ctrl+C
^CNew signal handler : 2
New handler sleep completed
^C
*/

// strace
// jidesh@jidesh-MS-7E26:/tmp$ strace ./signal 
// execve("./signal", ["./signal"], 0x7fff373915d0 /* 48 vars */) = 0
// brk(NULL)                               = 0x64ae81228000
// arch_prctl(0x3001 /* ARCH_??? */, 0x7ffe75f2a880) = -1 EINVAL (Invalid argument)
// mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x75b98d5f9000
// access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
// openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
// newfstatat(3, "", {st_mode=S_IFREG|0644, st_size=86200, ...}, AT_EMPTY_PATH) = 0
// mmap(NULL, 86200, PROT_READ, MAP_PRIVATE, 3, 0) = 0x75b98d5e3000
// close(3)                                = 0
// openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
// read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0P\237\2\0\0\0\0\0"..., 832) = 832
// pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
// pread64(3, "\4\0\0\0 \0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0"..., 48, 848) = 48
// pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0\325\31p\226\367\t\200\30)\261\30\257\33|\366c"..., 68, 896) = 68
// newfstatat(3, "", {st_mode=S_IFREG|0755, st_size=2220400, ...}, AT_EMPTY_PATH) = 0
// pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
// mmap(NULL, 2264656, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x75b98d200000
// mprotect(0x75b98d228000, 2023424, PROT_NONE) = 0
// mmap(0x75b98d228000, 1658880, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x75b98d228000
// mmap(0x75b98d3bd000, 360448, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1bd000) = 0x75b98d3bd000
// mmap(0x75b98d416000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x215000) = 0x75b98d416000
// mmap(0x75b98d41c000, 52816, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x75b98d41c000
// close(3)                                = 0
// mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x75b98d5e0000
// arch_prctl(ARCH_SET_FS, 0x75b98d5e0740) = 0
// set_tid_address(0x75b98d5e0a10)         = 5785
// set_robust_list(0x75b98d5e0a20, 24)     = 0
// rseq(0x75b98d5e10e0, 0x20, 0, 0x53053053) = 0
// mprotect(0x75b98d416000, 16384, PROT_READ) = 0
// mprotect(0x64ae5dd32000, 4096, PROT_READ) = 0
// mprotect(0x75b98d633000, 8192, PROT_READ) = 0
// prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
// munmap(0x75b98d5e3000, 86200)           = 0
// rt_sigaction(SIGINT, {sa_handler=0x64ae5dd301c9, sa_mask=[], sa_flags=SA_RESTORER|SA_INTERRUPT|SA_NODEFER|SA_RESETHAND|0xffffffff00000000, sa_restorer=0x75b98d242520}, {sa_handler=SIG_DFL, sa_mask=[], sa_flags=0}, 8) = 0
// newfstatat(1, "", {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x3), ...}, AT_EMPTY_PATH) = 0
// getrandom("\x8c\x4c\x43\x91\x8b\xa4\xda\x1f", 8, GRND_NONBLOCK) = 8
// brk(NULL)                               = 0x64ae81228000
// brk(0x64ae81249000)                     = 0x64ae81249000
// write(1, "New handler installed - Press Ct"..., 37New handler installed - Press Ctrl+C
// ) = 37
// clock_nanosleep(CLOCK_REALTIME, 0, {tv_sec=15, tv_nsec=0}, 0x7ffe75f2a8e0) = 0
// write(1, "New handler sleep completed\n", 28New handler sleep completed
// ) = 28
// clock_nanosleep(CLOCK_REALTIME, 0, {tv_sec=10, tv_nsec=0}, 0x7ffe75f2a8e0) = 0
// rt_sigaction(SIGINT, {sa_handler=SIG_DFL, sa_mask=[], sa_flags=SA_RESTORER|SA_INTERRUPT|SA_NODEFER|SA_RESETHAND|0xffffffff00000000, sa_restorer=0x75b98d242520}, {sa_handler=0x64ae5dd301c9, sa_mask=[], sa_flags=SA_RESTORER|SA_NODEFER|SA_RESETHAND, sa_restorer=0x75b98d242520}, 8) = 0
// write(1, "Completed\n", 10Completed
// )             = 10
// exit_group(0)                           = ?
// +++ exited with 0 +++