# AOS - Complete Technical Documentation

Welcome to the comprehensive technical documentation for **AOS**, a modern 64-bit educational kernel built from scratch for learning OS internals and AI integration.

---

## 📚 Documentation Structure

### [1. Architecture & Boot](/blog_docs/architecture/)

- **Global Descriptor Table (GDT)** - Segmentation and memory protection
- **IDT (Interrupt Descriptor Table)** - Interrupt vector configuration
- **Bootloader & Multiboot** - Kernel loading and initialization
- **Long Mode Transition** - x86_64 mode setup

### [2. Memory Management](/blog_docs/memory/)

- **Physical Memory Manager (PMM)** - Bitmap-based allocation
- **Virtual Memory Manager (VMM)** - 4-level paging and page tables
- **Kernel Memory Allocator (Kmalloc)** - Dynamic heap allocation
- **Memory Mapping** - Higher-half kernel and identity mapping

### [3. Interrupts & I/O](/blog_docs/interrupt_io/)

- **Interrupt Handling** - IRQ routing and vector handling
- **APIC/IO-APIC** - Advanced interrupt controller
- **Timer Subsystem** - Context switching and scheduling triggers
- **I/O Ports** - Hardware communication

### [4. Device Drivers](/blog_docs/drivers/)

- **ATA/IDE Driver** - Hard disk communication
- **AHCI Driver** - SATA support
- **Keyboard Driver** - PS/2 keyboard input handling
- **Screen/VGA Driver** - Text mode output
- **PCI Bus Driver** - Device enumeration

### [5. Filesystem](/blog_docs/filesystem/)

- **FAT32 Filesystem** - Cluster-based file storage
- **GPT/MBR Partitioning** - Disk partition management
- **File Operations** - Reading, writing, directory navigation

### [6. Core Systems](/blog_docs/core_systems/)

- **Task Management** - Process creation and scheduling
- **Shell** - Command-line interface
- **Kernel Entry Point** - Initialization sequence
- **Round-Robin Scheduler** - Multi-tasking support

### [7. Networking](/blog_docs/networking/)

- **lwIP Stack** - Lightweight TCP/IP implementation
- **Ethernet Drivers** - Network interface support
- **RTL8169 NIC Driver** - Realtek network driver
- **E1000 Mock Driver** - Intel interface simulation

### [8. Utilities & Libraries](/blog_docs/#utilities)

- **Standard Library** - String, math, and utility functions
- **Bootimage Tools** - ISO and bootable image generation

---

## 🎯 Quick Navigation

### For Beginners

Start here to understand the fundamentals:

1. [Bootloader & Multiboot](architecture/bootloader.md)
2. [Global Descriptor Table](architecture/gdt.md)
3. [Physical Memory Manager](memory/pmm.md)
4. [Basic Interrupts](interrupt_io/idt.md)

### For Intermediate Developers

Dive deeper into core systems:

1. [Virtual Memory Management](memory/vmm.md)
2. [APIC Interrupt Controller](interrupt_io/apic.md)
3. [Task Management & Scheduling](core_systems/task_management.md)
4. [ATA/IDE Storage Driver](drivers/ata_driver.md)

### For Advanced Users

Explore specialized topics:

1. [FAT32 Filesystem Implementation](filesystem/fat32.md)
2. [lwIP Network Stack](networking/lwip_stack.md)
3. [Kernel Memory Allocator](memory/kmalloc.md)
4. [Shell & Command Processing](core_systems/shell.md)

---

## 🔑 Key Features

✅ **Direct x86_64 Assembly Implementation**

- No dependency on existing kernels
- Hand-crafted GDT, IDT, and paging tables
- Custom interrupt handlers

✅ **Modern Boot Support**

- Multiboot 2 compliance
- UEFI and BIOS compatibility
- Framebuffer support

✅ **Advanced Memory Management**

- Bitmap-based physical memory allocation
- 4-level page tables for 64-bit addressing
- Dynamic kernel heap (scalable to 1GB+)

✅ **Complete I/O Architecture**

- APIC/IO-APIC interrupt routing
- ATA/IDE and AHCI storage drivers
- Keyboard and VGA text drivers

✅ **Functional Filesystem**

- FAT32 with GPT/MBR support
- File reading and directory traversal
- Partition detection and mounting

✅ **Multitasking Support**

- Round-robin process scheduler
- Context switching and task creation
- Basic shell with command execution

✅ **Network Stack Integration**

- Embedded lwIP TCP/IP
- Multiple NIC drivers
- Foundation for networked services

✅ **AI Integration Foundation**

- Tokenization support
- LLM inference engine integration
- Shell with NLP intent mapping

---

## 📖 How to Use This Documentation

Each document follows this structure:

1. **Overview** - High-level concept explanation
2. **Purpose** - What problem it solves
3. **Architecture Diagram** - Visual representation
4. **Implementation Details** - Code walkthroughs
5. **Key Functions** - API reference
6. **Code Examples** - Practical usage
7. **Performance Notes** - Optimization considerations
8. **Further Reading** - Related components

---

## 🛠️ Project Statistics

- **Language**: C + x86_64 Assembly
- **Total Lines**: 15,000+
- **Core Modules**: 30+
- **Drivers**: 5 major drivers
- **Boot Methods**: 2 (BIOS, UEFI)
- **Memory Support**: Up to 256GB (64-bit)
- **Max Processes**: Configurable (tested with 100+)

---

## 📝 File Organization

```
blog_docs/
├── README.md                    ← You are here
├── architecture/
│   ├── gdt.md                  - Segmentation
│   ├── idt.md                  - Interrupt vectors
│   ├── bootloader.md           - Boot process
│   └── long_mode.md            - x86_64 transition
├── memory/
│   ├── pmm.md                  - Physical manager
│   ├── vmm.md                  - Virtual memory
│   └── kmalloc.md              - Heap allocator
├── interrupt_io/
│   ├── idt.md                  - Interrupt handling
│   ├── apic.md                 - APIC controller
│   ├── timer.md                - Timer subsystem
│   └── io_ports.md             - Hardware I/O
├── drivers/
│   ├── ata_driver.md           - ATA/IDE storage
│   ├── ahci_driver.md          - SATA support
│   ├── keyboard_driver.md      - Input handling
│   ├── screen_driver.md        - Text output
│   └── pci_driver.md           - Bus enumeration
├── filesystem/
│   ├── fat32.md                - FAT32 filesystem
│   ├── gpt.md                  - Disk partitioning
│   └── file_operations.md      - I/O operations
├── core_systems/
│   ├── kernel_init.md          - Boot sequence
│   ├── task_management.md      - Processes
│   ├── scheduler.md            - Task scheduling
│   └── shell.md                - Command interface
└── networking/
    ├── lwip_stack.md           - TCP/IP stack
    ├── ethernet.md             - Network layer
    ├── rtl8169_driver.md       - Realtek NIC
    └── e1000_driver.md         - Intel NIC simulator
```

---

## 🚀 Getting Started

### Build & Run

```bash
# Clone repository
cd /home/deadgoat/new_os

# Build the kernel
make

# Run in BIOS mode
./run.sh --run

# Run in UEFI mode
./run.sh --run --uefi
```

### Access Source Code

All source code is referenced in documentation:

- C source: `src/` directory
- Assembly: `src/arch/x86_64/asm/` directory
- Headers: `include/` directory

---

## 📧 Contributing to Docs

When adding new documentation:

1. Follow the same structure as existing docs
2. Include code examples from actual source files
3. Add diagrams where helpful
4. Cross-reference related components
5. Keep technical accuracy

---

## 📜 License

This documentation and the AOS project are open source. See `OPENSOURCE.md` for details.

---

**Last Updated**: April 2026  
**Version**: 1.0  
**Status**: Active Development

Happy learning! 🎓
