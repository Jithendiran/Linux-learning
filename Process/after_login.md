1. Login shell 

    It has controlling terminal
    Now It's PID is 12345
    R/E/S/F UIDS - 0    0   0   0   
    R/E/S/F UIDS - 0    0   0   0
    
    progess group id and session id is 12345


2. Now user 1005 tries to Login


    User success at Login, now Login shell's ID changes are
    R/E/S/F UIDS - 1005    1005   1005   1005   
    R/E/S/F UIDS - 1005    1005   1005   1005

    based on the /etc/passwd file

    PGID and Session ID is 12345

    One interseting thing will happen here, this login program replace it's program with the shell program, this shell program path is specified in the /etc/passwd, form now onwards login program gone, shell program take over the action

    All the ids remain same  as 
    R/E/S/F UIDS - 1005    1005   1005   1005   
    R/E/S/F UIDS - 1005    1005   1005   1005
    PGID and Session ID is 12345

    shell program has the controlling terminal, this is called interactive shell


3. Now user execute some program call a.out
    
    When an interactive shell (like Bash or Zsh) executes a command (a.out), it typically forks a child process.

    child process create it's own process group, not all the process will create it's own process group id

    The child process then uses execve() to load and run the a.out program.

    That child process (a.out) Pid 12346

    PID = 12346
    R/E/S/F UIDS - 1005    1005   1005   1005   
    R/E/S/F UIDS - 1005    1005   1005   1005
    PGID = 12346 and Session ID is 12345

    interactive shell (parent) give control of the terminal to new process group

    now shell process 12345 will wait (background process) until a.out 12346 complete
    till now session id is untouched as 12345

    Now two Process group 12346, 12345 is active

4. after a.out complete
    shell program will get SIGCHILD  
    shell program pid 12345 will get back the controlling terminal
    forground process group 12346 is set back to 12345

    Now 12346 is no more

    the 12346 pid is now free to be assigned to new process

5. now b.out started as backgrond process, it forks 2 child in background
    The shell forks a child, say PID 12346.

    The child (12346) creates a new process group, PGID 12346.

    The child (12346) calls execve("b.out", ...) and becomes b.out.

    b.out forks two children: PIDs 12347 and 12348. They inherit the parent's PGID, so they are all in PGID 12346, these child process won't create new process group

    The shell does NOT change the foreground process group.

    | Process | PID | PGID | SID | State |
    | :--- | :--- | :--- | :--- | :--- |
    | Shell | 12345 | 12345 | 12345 | **Foreground** |
    | `b.out` | 12346 | 12346 | 12345 | Background |
    | Child 1 | 12347 | 12346 | 12345 | Background |
    | Child 2 | 12348 | 12346 | 12345 | Background |

6. after b.out&, immediately c.out started as foreground process, it forked 2 child 

    The shell forks a child, say PID 12349.

    The child (12349) creates a new process group, PGID 12349.

    The child (12349) calls execve("c.out", ...) and becomes c.out.

    c.out forks two children: PIDs 12350 and 12351. They are in PGID 12349.

    The shell make PGID 12349 the foreground process group.

    The shell (12345) waits for PID 12349 to complete.

    | Process | PID | PGID | SID | State |
    | :--- | :--- | :--- | :--- | :--- |
    | Shell | 12345 | 12345 | 12345 | Background (waiting) |
    | `b.out` group | 12346 | 12346 | 12345 | Background |
    | b Child 1 | 12347 | 12346 | 12345 | Background |
    | b Child 2 | 12348 | 12346 | 12345 | Background |
    | `c.out` group | 12349 | 12349 | 12345 | **Foreground** |
    | c Child 1 | 12350 | 12349 | 12345 | **Foreground** |
    | c Child 2 | 12351 | 12349 | 12345 | **Foreground** |