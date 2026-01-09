# Jarvis OS 🌌

A custom, 32-bit x86 monolithic kernel built from scratch.

Jarvis OS is a "Higher-Half" kernel designed to explore core operating system concepts including manual memory management, interrupt-driven I/O, and hardware abstraction.

---

## 🚀 Key Features

- **Bootloader**: Multiboot-compliant (GRUB) loading mechanism.
- **Memory Management**:
  - **PMM**: Physical Memory Manager using a Bitset (Managing ~128MB RAM).
  - **VMM**: Virtual Memory Manager with 2-level Paging (Page Directories & Tables).
- **Interrupts**: Custom IDT (Interrupt Descriptor Table) with PIC remapping for hardware IRQs.
- **Drivers**:
  - **VGA**: Text-mode driver at `0xB8000` with hardware cursor control and scrolling.
  - **Keyboard**: Full US-QWERTY driver with Shift/Caps Lock state tracking.
- **Interface**: An interactive command-line shell with a command buffer.

---

## 📂 Project Structure

```text
.
├── src/
│   ├── arch/i386/    # GDT, IDT, PIC, and I/O logic
│   ├── asm/          # Assembly entry points (loader, interrupts, paging)
│   ├── drivers/      # Hardware drivers (Keyboard, VGA)
│   └── kernel/       # Core logic (PMM, VMM, Shell, String lib)
├── include/          # Header files (.h)
├── build/            # Compiled object files (Git ignored)
├── iso/              # GRUB configuration and boot files
├── link.ld           # Linker script defining memory layout
└── run.sh            # Automated build and emulation script
```

🛠️ Build & RunPrerequisitesYou need a Linux environment with the following tools installed:gcc & binutils (32-bit support)nasm (Assembly compiler)grub-mkrescue & xorriso (For ISO generation)qemu-system-i386 (For emulation)ExecutionUse the provided run.sh script to compile the source and launch the OS in QEMU:Bashchmod +x run.sh

```
./run.sh
```

## Architecture Overview1. Memory LayoutJarvis OS uses Paging to separate physical memory from virtual addresses. The kernel is designed to be mapped into the "Higher Half" ($0xC0000000$), leaving the lower memory range free for future user-space applications.2. The Interrupt ChainThe kernel handles hardware interaction via a structured interrupt pipeline:Hardware (e.g., Keyboard) triggers an IRQ signal.PIC (Programmable Interrupt Controller) routes the signal to the CPU.IDT looks up the correct assembly stub in interrupts.s.Assembly Stub saves registers and calls the C-based keyboard_handler.Driver processes the scancode and updates the VGA buffer.📝 Roadmap & Progress[x] Global Descriptor Table (GDT)[x] Interrupt Descriptor Table (IDT) & PIC Remapping[x] Physical Memory Manager (Bitmap)[x] Virtual Memory Manager (Paging)[x] Interactive Keyboard & Screen Drivers[x] Basic Command Shell[ ] Heap Allocator (kmalloc)[ ] Multitasking (Task Switching)[ ] System Calls (Software Interrupts)🛡️ LicenseThis project is open-source. Feel free to use it for educational purposes.

### Suggested Next Step

Now that your documentation is ready, would you like to implement the **`mem`** command for your shell? It would use your PMM functions to show the user exactly how much RAM is free versus occupied in a nice table.
