Every process has a set of associated UIDs and GUIDs, based on these ID's process can access the resources

## Types
* Real User ID and Group ID
* Effective User ID and Group ID
* Saved set User ID and set Group ID
* File system User ID and group ID
* Supplementary group IDs

All these for a process can be checked by this command `cat /proc/self/status | grep -e "id:"`

```sh
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
```

It is arranged in the order of `Real    Effective   Saved    File`

### Real User ID and Real Group ID

As a part of Login process, login shell takes these values from `/etc/passwd` file 3rd and 4th fields, when a new user process is created it inherits from its parent 

Basically it is the id of it's own user and group

Groups are taken from two files `/etc/passwd` and `/etc/group` (see supplementry group)

### Effective User ID and Group ID

Effective user ID and group ID in conjunction with Supplementary group IDs are used to determine the permission granted to a process when it tries to perfom various operations like `system calls`, `files`, `System V IPC`

These id's are used by kernel to determine whether one process can send signal to other process or not

A process whose effective user ID is 0 has all of the privileges of the super user

Usually effective user ID and group ID have the same value as real ID's, but these can be changed

### Saved set User ID and set Group ID

Before Saved set User ID and saved set Group ID we have to see what is set-user-ID and set-group-ID

A set-user-ID and set-group-ID programs allows a process to gain the privilege it does not have

Example

User id is 1005, he created a process from fork  

process perms: R=1005    E=1005    S=1005    F=1005

he yet to run exec* call with program name `pgm`, `pgm` is a file

It's permission is like this

`pgm    -rwsrws---     abc_user    abc_user`

you might notice in the place of `x` you can see `s`, `s` means set user ID or set group ID depends on the context

when the user 1005 execute `pgm` program likeily by exec* call, program loaded into the memory, Kernel will set the effective user as the user id of the executable file, program user id is 1002 -> abc_user process user become 1002, and process effective group id is copied from program's group id is 1002 -> abc_user, meaning it is get the abc_user user permission, now program will execute, 

process perms: R=1005    E=1002    S=1002    F=1005

this is just a temporary permission, it will regain back it's permission

> [!WARNING]
> There is a security risk in this method, because this will work even with the root permission

Setting Set-UID on a non-executable file (like a text file) is generally ignored, as it has no meaning for file access and cannot be run as a program.

On most modern Linux systems, setting the Set-UID bit on a directory is ignored and has no effect

When SGID is set on a directory, it does not affect execution, but rather changes the group ownership of new files created within that directory. This is its most common and important directory-specific function

In the group's execute permission slot (`drwxr**s**r-x`) All new files and subdirectories created inside this directory inherit the group ID of the parent directory, instead of the primary group of the user who created them.

----

saved set user id and saved set group id are designed for use with set-user-id and set-group-id programs, when a program is executed following steps will occur
1. If set-user-id permission bit is enabled on the executable then the effective user id of the process is made same as the owner of the executable, if set-user-id permission bit is not enabled no changes are made
2. The values for saved set user id is copied from effective user id, (saved user id is a kind of backup for effective user id)  

same thing will happen for saved set group id

How permission are determined

1. init is the 1st process, it's ids are hard code as `R=0   E=0   S=0   F=0`, 
2. Login process also have root permission (inherited), when login is success, it is dropping the priviledge to the id's mentioned in `/etc/passwd`
`R=1005 E=1005  S=1005  F=1005`
3. Which ever process we create after login, it get id's from parent login process

-------------------

Doubt:
real user id : 1005, real group id: 1005 
effective user id: 1005, effective group id: 0

file it tries to acces has permission for

user - rwx
group - rwx
others - ---

1. user id = 1005, group id = 1005
2. user id = 1002, group id = 1005
3. user id = 1002, group id = 1002
4. user id = 1002, group id = 1002

---------------------

### File system User ID
In most of the cases EUID and FUID are same, it is in linux for backward compactable, see The Linux programming interface for more info 

### Supplementry Group Id
This is the set of additional groups to which a process belongs. a new process inherit from parent process, login process after successful login takes these values from `/etc/group` file, this is used in conjunction with effective ID for permission checks 

## User and Group ID Types

| ID Type | Purpose | How it is set |
| :--- | :--- | :--- |
| **Real ID (RUID/RGID)** | Identifies the **real user** who started the process. **This ID never changes** and is used for accounting and determining who can send signals to the process. | Inherited from the parent process/login session. |
| **Effective ID (EUID/EGID)** | The ID the **kernel uses to check permissions** for almost all operations (e.g., file access, system calls). | Usually same as Real ID, but is **changed by the Set-UID/Set-GID bits** on an executable file or by a privileged process using `setuid()`. |
| **Saved Set ID (SUID/SGID)** | A **backup copy** of the Effective ID *before* it was changed by a Set-UID/Set-GID executable. **Allows a process to temporarily surrender root privileges and later regain them.** | Inherited from the Effective ID upon program execution (via `exec*`) if the file has Set-UID/Set-GID set. |


