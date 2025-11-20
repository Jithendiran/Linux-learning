## Process group
* Process group is a collection of related process
* One or more process share the same Process Group id (PGID) is know as process group.
* Each process group has leader, which process creates the process group, that process id become the process group id
* A process can leave a process group by terminating or joing other process group
* It is not mandatory the ptocess group leader must be the last member of the process group id
* A process group only  exist when the last process belonging to that group either terminates or moves to another process group.

What happen here?
PGID 12345 has 2 process 12345 and 12346, 
1. process 12345 join different process group (12347)
    - the process 12345 have differnt process goup id (12347)
    - the process 12346 is still part of the process group 12345
    - when the process 12346 terminate or join different process group, 12345 process group will no more
2. process 12345 terminate and  the process 12346 alive
    - the process 12346 is still part of the process group 12345
    - when the process 12346 terminate or join different process group, 12345 process group will no more

## session
* Session is a collection of relared process group
* One or more process group share the same session id (SID) is know as session.
* Session has session leader, leader is the process who create new session, and that process id become session id
* All the process in the session share the single control terminal, The control terminal is established when the session leader first open a terminal device, At a given time only one proces group can access the terminal, which ever process group has the terminal is called as `foreground process group`, remaining are `background process group`
* A single Unix-like system (like Linux or macOS) can, and typically does, have many concurrent sessions.

[refer](./after_login.md)

In the context of a typical shell (like Bash), process groups and sessions are used to implement job control. Each command pipeline you run (e.g., cmd1 | cmd2 &) becomes a process group.

