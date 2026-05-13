## VM preparation

### Prepare

```bash
$ mkdir -p /tmp/debug-learn/src && mkdir -p /tmp/debug-learn/build/toolchain && mkdir -p /tmp/debug-learn/build/debug && mkdir -p /tmp/debug-learn/build/bin
$ cd /tmp
$ wget https://dl-cdn.alpinelinux.org/alpine/latest-stable/releases/x86_64/alpine-virt-3.23.4-x86_64.iso
$ wget https://dl-cdn.alpinelinux.org/alpine/latest-stable/releases/aarch64/alpine-virt-3.23.4-aarch64.iso
$ wget https://releases.linaro.org/components/kernel/uefi-linaro/16.02/release/qemu64/QEMU_EFI.fd 
$ wget -O- https://musl.cc/aarch64-linux-musl-cross.tgz | tar -xzvC /tmp/debug-learn/build/toolchain --strip-components=1
```

### Build machine
#### Host
```bash
    $ qemu-system-x86_64 \
    -enable-kvm \
    -m 1G \
    -drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
    -boot d \
    -nographic \
    -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
    -fsdev local,id=src_dev,path=/tmp/debug-learn,security_model=none \
    -device virtio-9p-pci,fsdev=src_dev,mount_tag=host_share
```
#### VM
```bash
$ mkdir -p /module/dev
$ mount -t 9p -o trans=virtio host_share /module/dev
$ export PATH=/module/dev/build/toolchain/bin:$PATH
$ aarch64-linux-musl-gcc -g -O0 /module/dev/src/hello.c -o /tmp/hello.o
$ aarch64-linux-musl-objcopy --only-keep-debug /tmp/hello.o /module/dev/build/debug/hello.debug
$ aarch64-linux-musl-strip --strip-debug --strip-unneeded /tmp/hello.o -o /module/dev/build/bin/hello_stripped
$ aarch64-linux-musl-objcopy --add-gnu-debuglink=/module/dev/build/debug/hello.debug /module/dev/build/bin/hello_stripped # optional (auto discovery)
```
##### Auto discovery
Where GDB looks for the .debug file
If your stripped binary is at /module/dev/build/bin/hello_stripped, GDB will search these three locations in order:

1. The same directory: /module/dev/build/bin/hello.debug

2. A subdirectory named .debug: /module/dev/build/bin/.debug/hello.debug

3. The global debug directories: Usually /usr/lib/debug/module/dev/build/bin/hello.debug

set `set debug-file-directory /module/dev/build/debug`
or manual load
`file /module/dev/build/bin/hello_stripped`
`symbol-file /module/dev/build/debug/hello.debug`

### Target machine
#### Host
```bash
$ qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a57 \
    -m 512 \
    -bios /tmp/QEMU_EFI.fd \
    -drive file=alpine-virt-3.23.4-aarch64.iso,media=cdrom \
    -nographic \
    -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
    -fsdev local,id=src_dev,path=/tmp/debug-learn/build/bin,security_model=none \
    -device virtio-9p-pci,fsdev=src_dev,mount_tag=host_share \
    -netdev socket,id=net1,listen=:1234 \
    -device virtio-net-pci,netdev=net1,mac=52:54:00:12:34:10
```
#### VM
```bash
$ mkdir -p /apps/debug/
$ mount -t 9p -o trans=virtio host_share /apps/debug/
$ setup-interfaces -a -r && setup-apkrepos -1
$ echo "http://dl-cdn.alpinelinux.org/alpine/v3.23/community" >> /etc/apk/repositories
$ apk update && apk add gdb
$ ip addr add 192.168.10.1/24 dev eth1
$ ip link set eth1 up
$ gdbserver 192.168.10.1:1234 /apps/debug/hello_stripped
```

### Debug Machine
#### Host
```bash
$ qemu-system-x86_64 \
-enable-kvm \
-m 1G \
-drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
-boot d \
-nographic \
-netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
-fsdev local,id=src_dev,path=/tmp/debug-learn,security_model=none \
-device virtio-9p-pci,fsdev=src_dev,mount_tag=host_share \
-netdev socket,id=net1,connect=127.0.0.1:1234 \
-device virtio-net-pci,netdev=net1,mac=52:54:00:12:34:11
```
#### VM
```bash
$ mkdir -p /tmp/debug-learn/
$ mount -t 9p -o trans=virtio host_share /tmp/debug-learn/
$ setup-interfaces -a -r && setup-apkrepos -1
$ echo "http://dl-cdn.alpinelinux.org/alpine/v3.23/community" >> /etc/apk/repositories
$ apk update && apk add gdb-multiarch
$ ip addr add 192.168.10.2/24 dev eth1
$ ip link set eth1 up
$ gdb-multiarch /tmp/debug-learn/build/bin/hello_stripped # or gdb-multiarch /tmp/debug-learn/build/debug/hello.debug 
```

```gdb
(gdb) set sysroot /tmp/debug-learn/build/toolchain/aarch64-linux-musl/
(gdb) symbol-file /tmp/debug-learn/build/debug/hello.debug 
(gdb) set substitute-path /module/dev/src  /tmp/debug-learn/src
(gdb) target remote 192.168.10.1:1234
```