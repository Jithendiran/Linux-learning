The `/sys` directory in Linux is a virtual filesystem (sysfs), that provides a structured interface for the kernel to expose its hardware and system information to user-space programs.

It allows you to view and often modify the real-time configuration and status of the system's devices, drivers, and kernel subsystems.

The files in `/sys` represent live data. For example, you can:
* Check the current CPU frequency.
* Change the I/O scheduler for a block device.
* Change the I/O scheduler for a block device.

1. `/sys/devices`

Kernel's view of the physical hardware hierarchy.
```sh
jidesh@jidesh-MS-7E26:~$ ls -la /sys/devices/
total 0
drwxr-xr-x 22 root root 0 Nov 15 16:18 .
dr-xr-xr-x 13 root root 0 Nov 15 16:18 ..
drwxr-xr-x  5 root root 0 Nov 15 16:18 amd_iommu_0
drwxr-xr-x  3 root root 0 Nov 15 16:18 breakpoint
drwxr-xr-x  6 root root 0 Nov 15 16:18 cpu
drwxr-xr-x  3 root root 0 Nov 15 16:32 faux
drwxr-xr-x  5 root root 0 Nov 15 16:18 ibs_fetch
drwxr-xr-x  5 root root 0 Nov 15 16:18 ibs_op
drwxr-xr-x  3 root root 0 Nov 15 16:32 isa
drwxr-xr-x  4 root root 0 Nov 15 16:18 kprobe
drwxr-xr-x  8 root root 0 Nov 15 16:18 LNXSYSTM:00
drwxr-xr-x  5 root root 0 Nov 15 16:18 msr
drwxr-xr-x 26 root root 0 Nov 15 16:18 pci0000:00 # (PCIe)
drwxr-xr-x 25 root root 0 Nov 15 16:18 platform
drwxr-xr-x  9 root root 0 Nov 15 16:18 pnp0
drwxr-xr-x  5 root root 0 Nov 15 16:18 power
drwxr-xr-x  5 root root 0 Nov 15 16:18 power_core
drwxr-xr-x  3 root root 0 Nov 15 16:18 software
drwxr-xr-x 10 root root 0 Nov 15 16:18 system
drwxr-xr-x  3 root root 0 Nov 15 16:18 tracepoint
drwxr-xr-x  4 root root 0 Nov 15 16:18 uprobe
drwxr-xr-x 24 root root 0 Nov 15 16:18 virtual

$

/sys/devices/breakpoint:
total 0
drwxr-xr-x  3 root root    0 Nov 15 16:18 .
drwxr-xr-x 22 root root    0 Nov 15 16:18 ..
-rw-r--r--  1 root root 4096 Nov 15 16:33 perf_event_mux_interval_ms
drwxr-xr-x  2 root root    0 Nov 15 16:33 power
lrwxrwxrwx  1 root root    0 Nov 15 16:18 subsystem -> ../../bus/event_source
-r--r--r--  1 root root 4096 Nov 15 16:33 type
-rw-r--r--  1 root root 4096 Nov 15 16:18 uevent

/sys/devices/cpu:
total 0
drwxr-xr-x  6 root root    0 Nov 15 16:18 .
drwxr-xr-x 22 root root    0 Nov 15 16:18 ..
drwxr-xr-x  2 root root    0 Nov 15 16:33 caps
drwxr-xr-x  2 root root    0 Nov 15 16:33 events
drwxr-xr-x  2 root root    0 Nov 15 16:33 format
-rw-r--r--  1 root root 4096 Nov 15 16:33 perf_event_mux_interval_ms
drwxr-xr-x  2 root root    0 Nov 15 16:33 power
-rw-------  1 root root 4096 Nov 15 16:33 rdpmc
lrwxrwxrwx  1 root root    0 Nov 15 16:18 subsystem -> ../../bus/event_source
-r--r--r--  1 root root 4096 Nov 15 16:33 type
-rw-r--r--  1 root root 4096 Nov 15 16:18 uevent

```

what is `uevent` ?
`User-space Event Communication`. It allows a user or program to signal the kernel about an action related to the device, or it allows the kernel to trigger a notification to user-space programs (like the udev daemon).

what is `perf_event_mux_interval_ms`?
perf_event_mux_interval_ms is the timer for how often the kernel cycles its limited performance monitoring hardware to track a larger number of requested performance events.

```sh
jidesh@jidesh-MS-7E26:~$ ls -la /sys/devices/amd_iommu_0/subsystem/*
-rw-r--r-- 1 root root 4096 Nov 15 16:51 /sys/devices/amd_iommu_0/subsystem/drivers_autoprobe # Controls whether the kernel should automatically check for and bind drivers to devices on this bus. A value of 1 (usually default) means automatic probing is enabled. Writing 0 can disable it for manual control.
--w------- 1 root root 4096 Nov 15 16:51 /sys/devices/amd_iommu_0/subsystem/drivers_probe # Writing a device ID or driver name to this file manually forces the kernel to attempt to bind a driver to a specific device on this bus, overriding or assisting the automatic process.
--w------- 1 root root 4096 Nov 15 16:18 /sys/devices/amd_iommu_0/subsystem/uevent

/sys/devices/amd_iommu_0/subsystem/devices:
total 0
drwxr-xr-x 2 root root 0 Nov 15 16:18 .
drwxr-xr-x 4 root root 0 Nov 15 16:18 ..
lrwxrwxrwx 1 root root 0 Nov 15 16:18 amd_iommu_0 -> ../../../devices/amd_iommu_0
lrwxrwxrwx 1 root root 0 Nov 15 16:18 breakpoint -> ../../../devices/breakpoint
lrwxrwxrwx 1 root root 0 Nov 15 16:18 cpu -> ../../../devices/cpu
lrwxrwxrwx 1 root root 0 Nov 15 16:18 ibs_fetch -> ../../../devices/ibs_fetch
lrwxrwxrwx 1 root root 0 Nov 15 16:18 ibs_op -> ../../../devices/ibs_op
lrwxrwxrwx 1 root root 0 Nov 15 16:18 kprobe -> ../../../devices/kprobe
lrwxrwxrwx 1 root root 0 Nov 15 16:18 msr -> ../../../devices/msr
lrwxrwxrwx 1 root root 0 Nov 15 16:19 power -> ../../../devices/power
lrwxrwxrwx 1 root root 0 Nov 15 16:19 power_core -> ../../../devices/power_core
lrwxrwxrwx 1 root root 0 Nov 15 16:18 software -> ../../../devices/software
lrwxrwxrwx 1 root root 0 Nov 15 16:18 tracepoint -> ../../../devices/tracepoint
lrwxrwxrwx 1 root root 0 Nov 15 16:18 uprobe -> ../../../devices/uprobe

```


2. `/sys/block`

Logical view of all block devices detected and managed by the Linux kernel.

```sh
jidesh@jidesh-MS-7E26:~$ ls -la /sys/block/
total 0
drwxr-xr-x  2 root root 0 Nov 15 16:18 .
dr-xr-xr-x 13 root root 0 Nov 15 16:18 ..
lrwxrwxrwx  1 root root 0 Nov 15 16:18 loop0 -> ../devices/virtual/block/loop0
lrwxrwxrwx  1 root root 0 Nov 15 16:18 loop1 -> ../devices/virtual/block/loop1
lrwxrwxrwx  1 root root 0 Nov 15 16:18 loop2 -> ../devices/virtual/block/loop2
lrwxrwxrwx  1 root root 0 Nov 15 16:18 loop3 -> ../devices/virtual/block/loop3
lrwxrwxrwx  1 root root 0 Nov 15 16:18 loop4 -> ../devices/virtual/block/loop4
lrwxrwxrwx  1 root root 0 Nov 15 16:18 loop5 -> ../devices/virtual/block/loop5
lrwxrwxrwx  1 root root 0 Nov 15 16:18 loop6 -> ../devices/virtual/block/loop6
lrwxrwxrwx  1 root root 0 Nov 15 16:18 loop7 -> ../devices/virtual/block/loop7
lrwxrwxrwx  1 root root 0 Nov 15 16:18 nvme0n1 -> ../devices/pci0000:00/0000:00:01.2/0000:01:00.0/nvme/nvme0/nvme0n1
lrwxrwxrwx  1 root root 0 Nov 15 16:18 sda -> ../devices/pci0000:00/0000:00:02.1/0000:02:00.0/0000:03:0d.0/0000:0e:00.0/ata2/host1/target1:0:0/1:0:0:0/block/sda
```

3. `/sys/bus`

This directory organizes all devices according to the type of bus (or connection mechanism) they use. It represents a topological view based on communication protocols.

4. `/sys/class`

This directory organizes devices according to their function or class, regardless of their physical connection type. It represents a functional view.


