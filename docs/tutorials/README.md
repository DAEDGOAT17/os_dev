# Jarvis OS: Build Your Own x86_64 Operating System

## Complete Tutorial Series

Welcome to the **Jarvis OS** comprehensive tutorial series. This guide teaches you how to build a modern x86_64 operating system kernel **from scratch**, covering bootloader design, memory management, interrupt handling, process scheduling, filesystem implementation, and AI integration.

### 📚 Tutorial Modules

Each module is self-contained but builds progressively on previous concepts:

| Module | Topic | Difficulty | Duration |
|--------|-------|-----------|----------|
| [Phase 1](./01-architecture-and-environment.md) | Architecture & Compiling Environment | Beginner | 30 min |
| [Phase 2](./02-boot-sequence.md) | Boot Sequence (32-bit → 64-bit Long Mode) | Intermediate | 45 min |
| [Phase 3](./03-memory-management.md) | Memory Management (Paging Hierarchy) | Intermediate | 60 min |
| [Phase 4](./04-interrupts-and-apic.md) | Interrupts & APIC Configuration | Advanced | 50 min |
| [Phase 5](./05-task-scheduling.md) | Task Scheduling (Multitasking) | Advanced | 60 min |
| [Phase 6](./06-filesystem-and-drivers.md) | Filesystem & Device Drivers | Advanced | 75 min |
| [Phase 7](./07-nlp-ai-integration.md) | NLP/AI Kernel Integration | Expert | 45 min |

---

## 🎯 What You'll Learn

By completing this tutorial series, you will understand:

✅ **CPU Architecture** – x86_64 protected mode, long mode, privilege rings  
✅ **Boot Process** – Multiboot2 specification, firmware-to-kernel handoff  
✅ **Memory Management** – Physical allocation, virtual addressing, 4-level paging  
✅ **Interrupt Handling** – GDT, IDT, APIC/IO-APIC configuration  
✅ **Multitasking** – Process creation, context switching, round-robin scheduling  
✅ **Device I/O** – Disk drivers, filesystem implementation, block I/O  
✅ **Systems Integration** – Bare-metal shell with AI/NLP capabilities  

---

## 🚀 Prerequisites

Before starting, ensure you have:

- **Systems Knowledge**: Basic understanding of C, x86 assembly, and OS concepts (processes, memory, interrupts)
- **Development Environment**: Linux (or WSL2), GCC, NASM, QEMU
- **Toolchain**: x86_64-elf cross-compiler, GRUB utilities, disk tools
- **Repository**: Clone [DAEDGOAT17/AOS](https://github.com/DAEDGOAT17/AOS)

### Quick Toolchain Setup

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential binutils-common nasm qemu-system-x86 \
  grub-pc-bin grub-mkrescue mtools dosfstools gcc-multilib

# Verify
nasm -version
qemu-system-x86_64 --version
grub-mkrescue --version
```

---

## 📖 How to Use This Guide

1. **Start with Phase 1** – Understand the project structure and build pipeline
2. **Progress sequentially** – Each phase builds on the previous one
3. **Read code carefully** – Every code block has inline comments explaining low-level details
4. **Run experiments** – Build and test after each phase
5. **Reference the repository** – Source code is at `/src` in the main repository

---

## 🔍 Code Conventions

Throughout this series:

- **C code** is syntax-highlighted with detailed inline comments
- **Assembly** (NASM 64-bit) explains register operations and memory layout
- **Diagrams** illustrate memory hierarchies, interrupt flows, and data structures
- **"Educational Breakdown"** sections explain the *why* behind each design
- **⚠️ Developer Notes** highlight critical concepts or common pitfalls

---

## 📋 Table of Contents

### Phase 1: Architecture & Compiling Environment
- Toolchain prerequisites and setup
- Build pipeline (asm → object files → linking → ELF → ISO)
- Linker script fundamentals
- Compilation flags and bare-metal considerations

### Phase 2: Boot Sequence (32-bit → 64-bit Long Mode)
- Multiboot2 header specification
- Protected mode initialization
- Physical Address Extension (PAE) and page table setup
- EFER.LME bit and long mode activation
- Far-jump to 64-bit code
- Kernel entry point (kmain) initialization

### Phase 3: Memory Management (Paging Hierarchy)
- Physical Memory Manager (bitmap-based allocation)
- Virtual address translation and TLB mechanics
- 4-level paging structure: PML4 → PDPT → PD → PT
- Page table entry flags and permissions
- Virtual-to-physical address mapping with examples
- Page fault handling and demand paging

### Phase 4: Interrupts & APIC Configuration
- Global Descriptor Table (GDT) and segments
- Interrupt Descriptor Table (IDT) and gates
- Exception handling (faults, traps, aborts)
- Local APIC and I/O APIC setup
- Interrupt Service Routine (ISR) implementations
- End-of-Interrupt (EOI) signaling

### Phase 5: Task Scheduling (Multitasking)
- Task Control Block (TCB) data structure
- Task creation and initialization
- Context switching (register preservation and restoration)
- Round-robin scheduling algorithm
- Task states (READY, RUNNING, WAITING, DEAD)
- Timer-driven preemption

### Phase 6: Filesystem & Device Drivers
- ATA PIO driver: disk sector read/write
- FAT32 filesystem: boot sector, FAT chain, directory entries
- File operation semantics (open, read, write, seek)
- Block I/O abstraction
- Ramdisk initialization

### Phase 7: NLP/AI Kernel Integration
- Lightweight string processing without stdlib
- Intent mapping and natural language command parsing
- Shell command dispatch
- AI context management in bare-metal environment

---

## 🛠️ Building & Testing

### Clone and Setup

```bash
git clone https://github.com/DAEDGOAT17/AOS.git
cd AOS
```

### Build the Kernel

```bash
./run.sh
```

This will:
1. Assemble bootloader and low-level code
2. Compile all C source files
3. Link into kernel.elf
4. Create bootable ISO image
5. Generate FAT32 ramdisk

### Run in QEMU

```bash
# BIOS mode
./run.sh --run

# UEFI mode
./run.sh --run --uefi

# With serial output for debugging
qemu-system-x86_64 -m 4G -boot d -cdrom jarvis.iso -serial mon:stdio
```

### Debug with GDB

```bash
# Terminal 1: Run QEMU with debug server
qemu-system-x86_64 -m 4G -boot d -cdrom jarvis.iso -gdb tcp::1234 -S

# Terminal 2: Connect GDB
gdb kernel.elf
(gdb) target remote localhost:1234
(gdb) break kmain
(gdb) continue
```

---

## 📚 Learning Resources

### Essential References

- **Intel 64 and IA-32 Architectures Software Developer Manual** – Official x86_64 spec
- **Multiboot 2 Specification** – Bootloader standard (https://www.gnu.org/software/grub/manual/multiboot2/)
- **x86-64 System V ABI** – Calling conventions and data layout
- **OSDev.org** – Community OS development wiki and forums

### Books

- *Understanding the Linux Kernel* – Bovet & Cesati
- *Modern Operating Systems* – Tanenbaum
- *The C Programming Language* – Kernighan & Ritchie

---

## ✅ Checklist: Ready to Start?

- [ ] Installed x86_64-elf-gcc (or equivalent cross-compiler)
- [ ] Have NASM, QEMU, and GRUB tools installed
- [ ] Cloned DAEDGOAT17/AOS repository
- [ ] Can build with `./run.sh` without errors
- [ ] Can run kernel in QEMU with `./run.sh --run`
- [ ] Understand basic C and x86 assembly

If all boxes are checked, **proceed to Phase 1** →

---

## 📝 License

This tutorial series is provided as educational material for the Jarvis OS project.

**Last Updated**: 2026-06-22  
**Target Audience**: Intermediate to Advanced Systems Programmers  
**Estimated Total Time**: 5–6 hours
