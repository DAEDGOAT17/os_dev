# Jarvis OS

A custom 32-bit x86 monolithic kernel built from scratch.

Jarvis OS is a "Higher-Half" kernel designed to explore core operating system concepts, including manual memory management, interrupt-driven I/O, and hardware abstraction.

---

##  Key Features

- **Bootloader**: Multiboot-compliant GRUB loading mechanism
- **Memory Management**:
  - **PMM**: Physical Memory Manager using bitmap allocation (~128MB RAM)
  - **VMM**: Virtual Memory Manager with 2-level paging (page directories and tables)
- **Interrupts**: Custom IDT with PIC remapping for hardware IRQs
- **Drivers**:
  - **VGA**: Text-mode driver with hardware cursor control and scrolling
  - **Keyboard**: US-QWERTY driver with Shift and Caps Lock support
- **Shell**: Interactive command-line interface with command buffer

---

## 📂 Project Structure

```
.
├── src/
│   ├── arch/i386/    # GDT, IDT, PIC, and I/O logic
│   ├── asm/          # Assembly entry points
│   ├── drivers/      # Keyboard and VGA drivers
│   └── kernel/       # Core functionality (PMM, VMM, shell)
├── include/          # Header files
├── build/            # Compiled objects (ignored)
├── iso/              # GRUB configuration
├── link.ld           # Linker script
└── run.sh            # Build and emulation script
```

## Getting Started

**Prerequisites:**

- Linux environment with gcc, binutils (32-bit), nasm, grub-mkrescue, xorriso, and qemu-system-i386

**Build and Run:**

```bash
chmod +x run.sh
./run.sh
```

## Architecture

**Memory Layout**: Kernel mapped to the higher half (0xC0000000) with lower memory reserved for user space.

**Interrupt Pipeline**: Hardware IRQs → PIC routing → IDT lookup → assembly stub → C handler → driver response.

## Roadmap

- [x] Global Descriptor Table
- [x] Interrupt Descriptor Table & PIC
- [x] Physical Memory Manager
- [x] Virtual Memory Manager
- [x] Keyboard and VGA drivers
- [x] Shell
- [ ] Heap allocator
- [ ] Multitasking
- [ ] System calls

---

## License

Open-source for educational use.
