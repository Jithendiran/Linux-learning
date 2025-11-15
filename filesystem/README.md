# Linux File system

The Linux filesystem is a hierarchical structure that organizes all files and directories starting from a single root directory, denoted by the forward slash `(/)`

Linux mounts all storage devices and partitions into this unified (to join separate parts together to make one unit) structure.

## Unified Filesystem Structure

Everything in Linux is accessed starting from the `/` directory.

To access files on a hard drive partition or a USB stick, that device must be mounted. Mounting is the process of attaching a filesystem to a specific directory within the existing unified directory tree. This directory is called a mount point.

**Examples**

* The primary hard drive partition containing the OS is mounted at `/`.
* A CD-ROM or USB drive might be mounted to a directory like `/media`.
* A separate partition for user files might be mounted to `/home`.

When you plug in a USB drive (a new storage device), you don't get a new, separate tree (like getting a new drive letter on windows). Instead, you hang that USB drive (the new filesystem) onto a specific branch of the existing tree (e.g., the branch named `/media`).


A foundational concept in Linux is that almost everything, from regular files to directories, devices, and running processes, is treated as a file.
This philosophy means that the operating system provides a single, unified Application Programming Interface (API)—the same set of functions like `open()`, `read()`, `write()`, and `close()` to interact with all these different objects.

## How data is handled?
Files are the objects where data is stored, and Streams are the flows of data used to access them.

### Files

Files are persistent, static objects stored on a filesystem. They have a name, a fixed location (path), and contain metadata (permissions, size, creation date, etc.).

* Regular Files: `/home/user/document.txt`, `/bin/ls`, $\cdots$ 

    Documents, programs, libraries, and configuration files

* Directories: `/etc/`, `/`, $\cdots$ 

    Special files used to organize other files and directories.

* Device Files: `/dev/sda1`, `/dev/null`, $\cdots$ 

    Special files representing physical or virtual hardware devices.

* Socket/Pipe Files: `/tmp/my_pipe` (name pipe) just a example

    Files that uses for inter-process communication (IPC). They exist on the filesystem but represent a communication channel.

### Streams 

Streams are dynamic, sequential flows of bytes used for input and output (I/O). They represent the process of data transfer, regardless of the source or destination.

* Standard Streams: `stdin`, `stdout`, `stderr`

    The three default I/O channels available to every running program (process) in Linux. They are streams of data.

* Pipes:

    That takes the stdout stream of one command and connects it directly to the stdin stream of another command.

* Network Connections:

    The flow of data across a network (like the internet). Data being sent or received over a TCP or UDP socket.

* Memory Buffers:

    A sequence of data read from a file into a temporary memory location for processing.

1. When a program wants to read a File (e.g., `/etc/hosts`), it opens the file.
2. The OS gives the program a File Descriptor (a number like 3, 4, 5...). 
3. The program uses generic I/O commands (like `read()`) to treat the file descriptor as a Stream of Bytes, flowing sequentially from the beginning to the end.

**Data is stored in files, it is interacted with by streams.**


## key directories

| Directory | Purpose |
| :--- | :--- |
| **`/`** | **The Root Directory** of the entire filesystem. Every other file and directory branches out from here. |
| **`/bin`** | Contains essential **user binaries** (programs) that must be available when the system is booting (e.g., `ls`, `cp`, `mv`). |
| **`/sbin`** | Contains essential **system binaries** (programs) for system administration and maintenance (e.g., `fdisk`, `ifconfig`, `mount`). |
| **`/etc`** | Stores **configuration files** for the system and installed applications (e.g., `/etc/passwd`, `/etc/fstab`). |
| **`/home`** | Contains the **personal directories** for standard users (e.g., `/home/username`). |
| **`/root`** | The **home directory for the superuser (root)**. It's separate from `/home` for security and maintenance. |
| **`/dev`** | Contains **device files** representing hardware devices (e.g., `/dev/sda` for a hard drive, `/dev/null` for a 'black hole'). |
| **`/proc`** | A **virtual filesystem** providing information about **running processes** and kernel state (e.g., `/proc/cpuinfo`). |
| **`/var`** | Stores **variable data** that changes frequently during system operation, such as **logs** (`/var/log`), mail queues, and website data. |
| **`/tmp`** | Stores **temporary files** created by the system and users. Contents are usually deleted on reboot. |
| **`/usr`** | Contains **shareable, read-only data**, including non-essential user programs, libraries, and documentation (e.g., `/usr/bin`, `/usr/lib`). |
| **`/opt`** | Stores **optional software packages** from third-party vendors, often self-contained. |
| **`/boot`** | Contains files needed to **boot the operating system**, including the Linux kernel and boot loader files (e.g., GRUB). |
| **`/mnt`** | A conventional **mount point** for temporarily mounting filesystems (e.g., external drives). |
| **`/media`** | A mount point for **removable media** (e.g., USB drives, CDs) that are automatically mounted. |
| **`[/sys](./sys/README.md)`** | A **virtual filesystem** that provides an interface to the kernel's device model and configuration. |