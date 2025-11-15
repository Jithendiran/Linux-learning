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


A foundational concept in Linux is that almost everything, from regular files to directories, devices, and running processes, is treated as a file or stream of bytes.


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
| **`/sys`** | A **virtual filesystem** that provides an interface to the kernel's device model and configuration. |