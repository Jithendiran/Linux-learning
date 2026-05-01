# PCI Hardware Identification and Driver Management
This document is explained for virtual machine network adapter, the process is same for other drivers also

## 1. Hardware Presence Verification
Hardware detection is the first layer of system initialization. Even without a functional driver, the **Peripheral Component Interconnect (PCI) Bus** communicates with hardware during the boot sequence to identify what is physically connected to the motherboard.

### The Significance of PCI Class Codes
The Linux kernel categorizes all PCI devices using a standardized **Class Code**. This code allows the operating system to identify the function of a device (e.g., storage, graphics, or networking) before a specific driver is even loaded.

*   **Mechanism:** The command `grep "" /sys/bus/pci/devices/*/class` reads the identity file for every device on the bus.
*   **Identification Logic:** For networking, the prefix **`0x02`** is the critical identifier.
    *   **`0x020000`**: Specifically identifies an **Ethernet Controller**.
    *   **`0x028000`**: Identifies a generic **Network Controller** (commonly used for Wi-Fi cards).

### Device Addressing
Each device is assigned a unique address in the format `Domain:Bus:Device.Function` (e.g., `0000:00:03.0`). This address serves as the permanent physical location for the OS to send and receive data.


## 2. Analyzing Driver-to-Hardware Binding
A "driver" is a software translator that allows the operating system to send instructions to hardware. In Linux, this relationship is visible through symbolic links in the filesystem.

### Characteristics of a Healthy Bound Device
When a driver is successfully managing a device, a `driver` directory exists within the device's PCI path:
*   **Path:** `/sys/bus/pci/devices/[address]/driver`
*   **Indicator:** This directory is a symbolic link pointing to the specific kernel module in use (e.g., `virtio-pci`).

### Characteristics of a Missing Driver
If hardware is physically present but non-functional, the system will exhibit the following symptoms:
1.  **Missing Symlink:** The `/sys/bus/pci/devices/[address]/driver` file will be absent.
2.  **Empty Logic Layer:** The `ip link` command will not show the interface (e.g., `eth0`), because the OS has no way to translate PCI signals into networking logic.
3.  **Hardware Identification:** The device remains visible via its hardware IDs:
    *   **Vendor ID:** (e.g., `0x1af4` for Red Hat/VirtIO)
    *   **Device ID:** (e.g., `0x1000` for VirtIO Network Device)


## 3. Driver Acquisition and Loading Procedures
When hardware is detected but unbound, the operator must locate and activate the compatible kernel module.

### Search and Selection Logic
The **Vendor ID** and **Device ID** are used as global fingerprints. If the driver name is unknown, these hex codes are cross-referenced against the PCI ID database to determine the required module name (e.g., `virtio_net`).

### Internal Module Retrieval
Linux systems ship with a library of pre-compiled drivers located in `/lib/modules/`. 
*   **Verification:** `find /lib/modules/$(uname -r) -name "[module_name]*"` checks if the driver exists on the local disk.
*   **Activation:** The `modprobe` utility is used to load the driver into the kernel. 
    *   *Logic:* `modprobe` is preferred over `insmod` because it automatically loads "dependencies" (other software pieces the driver needs to function).

### Manual Compilation (Fallback)
If the kernel does not include the driver, the software must be obtained from the manufacturer in source code format (typically a `.tar.gz` file).
1.  **Preparation:** The code is extracted and transformed into an executable binary using the `make` command.
2.  **Installation:** The resulting `.ko` (Kernel Object) file is inserted into the kernel using `insmod`.
3.  **Result:** Successful loading creates the `driver` symlink in `/sys/`, which triggers the creation of the network interface (`eth0`) in the networking stack.