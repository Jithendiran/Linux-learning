## What is Firmware?

Firmware is a type of permanent software program that is embedded directly into a hardware device. It acts as the "software for hardware," providing the low-level control and instructions needed for a piece of hardware to function, start up, and communicate with the operating system (like the kernel) and other devices.

**Examples of devices that rely on firmware:** Motherboards (BIOS/UEFI), hard drives/SSDs, optical drives, network cards (Wi-Fi/Bluetooth), graphics cards, routers, smart TVs, and controllers in appliances.

If we want to compile the kernel we need device specific pre compiled firmware binary provided by the hardware manufacturer

* **The Kernel's Role:** The Linux kernel includes drivers that know how to talk to hardware (e.g., your Wi-Fi card).

* **The Firmware's Role**: The hardware itself (e.g., the Wi-Fi card's chip) often needs a separate, proprietary piece of software—the firmware—to be loaded onto it to actually operate.

* **The Interaction:** The kernel's driver is responsible for reading this firmware file from the system's storage (usually /lib/firmware on Linux) and loading it into the device's internal memory when the device is initialized.

### if firmware is included inside the wifi card or any hardware storage, why do we need to download firmware separately?

The simplest answer is that modern Wi-Fi cards use a combination of both internal and external storage for their firmware, and the need to download it separately is primarily for flexibility, updates, and compatibility with the operating system.

Wi-Fi card (or Bluetooth card,...any) actually has **two different kinds of firmware** storage:

| Storage Location | What it Stores | Why it's Stored There |
| :--- | :--- | :--- |
| **Internal (On the Chip)** | **The Boot ROM (Low-Level Firmware)**: A very small, unchangeable, initial program. | This code tells the chip's internal CPU how to start up and how to talk to the host computer (the CPU/OS) well enough to receive the main operational firmware. This is essential, permanent, and cannot be updated. |
| **External (On Your Hard Drive/SSD)** | **The Operational Firmware (The "Binary Blob"):** The large, complex program that contains all the logic for Wi-Fi protocols (802.11a/b/g/n/ac/ax), security, power management, and radio control. | This is the file you download. It is easily updatable, saving the manufacturer the cost and complexity of putting a large, flashable memory chip on every card. |

Almost all of the hardware (keyboard, mouse, wifi card,..) have the their own RAM 

### How the Firmware is Loaded

1.  **System Boot:** The computer starts, and the Linux kernel loads the **driver** for your Wi-Fi card.
2.  **Driver Request:** The driver (e.g., `iwlwifi` for Intel cards) determines which specific firmware file the chip needs.
3.  **Firmware Load:** The driver looks for that specific binary file (e.g., `iwlwifi-cc-a0-62.ucode`) in the system's firmware directory (usually `/lib/firmware`).
4.  **Transfer and Execute:** The driver reads this file from your hard drive/SSD and **transfers it** into the Wi-Fi card's on-board **RAM**, where the card's internal processor executes it.

## What is device tree?

In the past, especially with embedded systems built around ARM processors, the Linux kernel had to be hardcoded with all the physical details of a specific board. This was done in "board files" (C code).

- If a hardware detail changed (e.g., an I2C sensor was moved to a different pin), the entire kernel had to be recompiled.
- Supporting a new board required adding a completely new, massive C file to the kernel source. This was inefficient and led to huge, complicated kernels.

The Device Tree provides a unified, standardized, and portable way to describe the hardware, allowing the same compiled Linux kernel image to run on different boards within the same CPU family.

- The Device Tree is a hierarchical, tree-like data structure that acts as a map of the hardware on a system-on-a-chip (SoC) or a custom circuit board. It moves all the hardware-specific details out of the Linux kernel's core code and into a separate, external file.

**standard desktop or laptop PC (running the x86 or x64 architecture) typically does not use a Device Tree (DT) to describe its hardware.**

Instead of the Device Tree, modern PCs rely on a much older and more complex standard for hardware description and management: ACPI.

### PC Architecture uses ACPI

On the vast majority of personal computers running Windows, macOS, or Linux on an Intel or AMD processor, the system uses the **Advanced Configuration and Power Interface (ACPI)**.

| Feature | Device Tree (DT) | ACPI (Advanced Configuration and Power Interface) |
| :--- | :--- | :--- |
| **Primary Use** | Embedded systems (e.g., Raspberry Pi, Android, routers, ARM-based systems). | Personal computers (x86/x64 architecture). |
| **Data Format** | A static, declarative, hierarchical text file (`.dts`) compiled into a **Binary Blob (`.dtb`)**. | A set of binary **ACPI Tables** (e.g., DSDT, FADT) provided by the system's firmware (UEFI/BIOS). |
| **Key Function** | Describes *non-discoverable* devices (like I2C/SPI pins, memory regions). | Handles both hardware configuration and complex **power management** ($S$ and $C$ states). |
| **"Intelligence"** | Purely a static data map of the hardware layout. | Contains an embedded programming language (**AML - ACPI Machine Language**) that the OS kernel can execute to control hardware functions. |

### Why PCs Don't Need DT

The PC architecture relies heavily on **bus enumeration** (also known as Plug and Play) for most of its devices:

1.  **PCI Express (PCIe) and USB:** When your PC boots, the operating system can simply *ask* the PCIe or USB buses what devices are connected, who manufactured them, and what resources they need. This information is provided directly by the devices themselves.
2.  **ACPI fills the gaps:** ACPI is primarily used to describe devices that are **not** on an enumerable bus, such as:
    * The system's thermal zones and fan controllers.
    * Integrated peripherals on the motherboard (clocks, timers).
    * Complex power management logic.

While the **purpose** of both DT and ACPI is similar (to tell the operating system about the hardware), they are distinct standards used on different platforms:

* **You'll use a DTB file if you are building Linux for an ARM-based Single Board Computer (SBC) or an embedded device.**
* **Your PC relies on ACPI tables built into its UEFI firmware.**