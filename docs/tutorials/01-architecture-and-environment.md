# Phase 1: Architecture & Compiling Environment

> **Difficulty**: Beginner  
> **Duration**: 30 minutes  
> **Learning Goals**: Understand the build pipeline, toolchain setup, and linker script fundamentals

---

## Overview

Before writing kernel code, you need a **cross-compiler** that targets bare metal (no operating system). Your native GCC assumes Linux; we need a compiler that generates standalone x86_64 binaries that can run without an OS.

This phase covers:
1. Toolchain installation and verification
2. The build pipeline (ASM → C → Linking → ELF → ISO)
3. Linker script fundamentals
4. Compiler flags for bare-metal development
5. Project structure and organization

---

## 1. Toolchain Setup

### What is a Cross-Compiler?

A **cross-compiler** is a compiler that runs on one platform (e.g., x86_64 Linux) but produces code for a *different* platform (e.g., bare-metal x86_64). This is essential for kernel development because:

- **Native GCC** assumes a hosted environment (Linux, Windows, macOS) with a C library
- **Cross-compiler (x86_64-elf-gcc)** generates standalone code with no external dependencies
- **ELF format** (Executable and Linkable Format) is the standard for bare-metal kernels

### Installation

#### Ubuntu/Debian

```bash
# Update package lists
sudo apt-get update

# Install essential build tools
sudo apt-get install -y build-essential binutils-common nasm

# Install cross-compiler for x86_64-elf
sudo apt-get install -y gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu

# Install emulation and bootloader tools
sudo apt-get install -y qemu-system-x86 qemu-utils
sudo apt-get install -y grub-pc-bin grub-mkrescue
sudo apt-get install -y mtools dosfstools

# 32-bit support (for bootloader)
sudo apt-get install -y gcc-multilib libc6-i386
```

#### macOS (via Homebrew)

```bash
brew install nasm qemu
brew tap homebrew/cask
brew install --cask virtualbox
```

#### Fedora/RHEL

```bash
sudo dnf install -y gcc nasm qemu
sudo dnf install -y grub2-tools-extra mtools dosfstools
```

### Verification

```bash
# Verify toolchain
gcc --version           # Native compiler
nasm --version          # Assembler
qemu-system-x86_64 --version  # Emulator
grub-mkrescue --version # ISO creation tool

# Expected output:
# gcc (GCC) 11.x.x ...
# NASM version 2.15.x
# QEMU emulator version 6.x.x
# grub-mkrescue (GRUB) 2.06
```

✅ If all commands succeed, your toolchain is ready.

---

## 2. Project Structure

Jarvis OS follows a standard kernel project layout:

```
AOS/
├── README.md                    # Project overview
├── link.ld                      # Linker script
├── run.sh                       # Build automation script
├── Makefile                     # Build rules (optional)
│
├── include/                     # Header files
│   ├── gdt.h                    # Global Descriptor Table
│   ├── idt.h                    # Interrupt Descriptor Table
│   ├── pmm.h                    # Physical Memory Manager
│   ├── vmm.h                    # Virtual Memory Manager
│   ├── task.h                   # Task scheduler
│   ├── ata.h                    # Disk driver
│   ├── fat32.h                  # Filesystem
│   └── ...
│
├── src/
│   ├── arch/x86_64/
│   │   └── asm/
│   │       ├── loader.s         # Bootloader entry point
│   │       ├── gdt.s            # GDT loading (asm part)
│   │       ├── idt.s            # IDT loading (asm part)
│   │       └── interrupt.s      # Interrupt handlers
│   │
│   ├── core/
│   │   ├── kernel.c             # kmain() - kernel entry
│   │   ├── gdt.c                # GDT initialization
│   │   ├── idt.c                # IDT initialization
│   │   └── apic.c               # APIC setup
│   │
│   ├── mm/
│   │   ├── pmm.c                # Physical memory manager
│   │   └── vmm.c                # Virtual memory manager
│   │
│   ├── drivers/
│   │   ├── ata.c                # ATA disk driver
│   │   ├── keyboard.c           # Keyboard driver
│   │   └── framebuffer.c        # Video display
│   │
│   ├── fs/
│   │   └── fat32.c              # FAT32 filesystem
│   │
│   ├── libc/
│   │   ├── string.c             # String functions (no stdlib)
│   │   └── stdlib.c             # Memory functions
│   │
│   └── shell/
│       └── shell.c              # Kernel shell
│
├── iso/                         # ISO build directory (created at build time)
│   └── boot/
│       ├── grub/
│       │   └── grub.cfg         # GRUB configuration
│       ├── kernel.elf           # Kernel binary
│       └── ramdisk.img          # FAT32 ramdisk
│
└── build/                       # Object files (created at build time)
    ├── loader.o
    ├── kernel.o
    ├── pmm.o
    └── ...
```

---

## 3. Linker Script Fundamentals

The **linker script** (`link.ld`) is the blueprint for how your kernel is assembled. It tells the linker:
- Where each section (.text, .data, .bss) should be placed
- How sections should be aligned
- What symbols (like `_kernel_start`, `_kernel_end`) should be defined

### Jarvis OS Linker Script

```linker script
/* link.ld – Kernel linker script for x86_64 bare-metal */

ENTRY(loader)              /* Entry point: the 'loader' symbol in loader.s */
OUTPUT_FORMAT(elf64-x86-64) /* Output format: 64-bit ELF */

SECTIONS
{
    /* Kernel loaded at 1 MB (0x100000) by bootloader */
    . = 1M;
    _kernel_start = .;      /* Symbol for kernel start address */

    /* ────── Multiboot Headers ────────────────────────────────────────
       MUST be at the very beginning of the file (within first 32 KB).
       Bootloader scans for Multiboot magic number here.
    */
    .boot : {
        KEEP(*(.multiboot2_header))
        KEEP(*(.multiboot_header))
    }

    /* ────── Code Section ──────────────────────────────────────────────
       Executable code. Aligned to 4 KB page boundary.
    */
    .text : ALIGN(4K) {
        *(.text)
    }

    /* ────── Read-Only Data ────────────────────────────────────────────
       Constants, strings. Aligned to 4 KB page boundary.
    */
    .rodata : ALIGN(4K) {
        *(.rodata)
    }

    /* ────── Initialized Data ──────────────────────────────────────────
       Global variables with initial values. Aligned to 4 KB page boundary.
    */
    .data : ALIGN(4K) {
        *(.data)
    }

    /* ────── Uninitialized Data ────────────────────────────────────────
       BSS section: global/static variables without initial values.
       Linker allocates space but doesn't store zeros (saves file size).
       Kernel must zero this region before use.
    */
    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }

    /* Mark end of kernel */
    . = ALIGN(4K);
    _kernel_end = .;
}
```

### Educational Breakdown: Why This Design?

**4 KB Alignment**: x86_64 paging uses 4 KB pages. By aligning sections to page boundaries, the VMM can:
- Map entire sections as contiguous pages
- Set different permissions per section (code: read+execute, data: read+write)
- Reduce fragmentation and improve TLB efficiency

**KEEP() directive**: By default, the linker discards unused sections. Multiboot headers aren't referenced from C code, so we explicitly `KEEP()` them to prevent removal.

**BSS optimization**: Uninitialized data (.bss) isn't stored in the binary file—only the size is recorded. When the kernel loads, it zeros this region. This saves ~100 MB for a kernel with large static arrays.

**Symbol definitions**: `_kernel_start` and `_kernel_end` are defined by the linker, not the source code. The kernel can use these symbols to protect its own memory in the PMM.

---

## 4. Build Pipeline

The `run.sh` script orchestrates the build process:

```bash
#!/bin/bash
set -e  # Exit on any error

# Configuration
ISO_NAME="jarvis.iso"
DISK_IMG="disk.img"
BUILD_DIR="build"
ISO_DIR="iso"
KERNEL_ELF="kernel.elf"

# ════════════════════════════════════════════════════════════════════════════
# STEP 1: Clean and prepare directories
# ════════════════════════════════════════════════════════════════════════════
echo "--- Step 1: Cleaning & Preparing environment ---"
rm -rf "$BUILD_DIR" "$ISO_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$ISO_DIR/boot/grub"

# ════════════════════════════════════════════════════════════════════════════
# STEP 2: Assemble x86_64 Assembly Files
# ════════════════════════════════════════════════════════════════════════════
echo "--- Step 2: Assembling x86_64 ASM ---"
for f in src/arch/x86_64/asm/*.s; do
    if [ -f "$f" ]; then
        echo "  Assembling $f..."
        # -f elf64: Output format (64-bit ELF object files)
        # -o: Output file path
        nasm -f elf64 "$f" -o "$BUILD_DIR/$(basename "${f%.s}.o")"
    fi
done

# ════════════════════════════════════════════════════════════════════════════
# STEP 3: Compile C Source Files
# ════════════════════════════════════════════════════════════════════════════
echo "--- Step 3: Compiling C Source ---"
CC_FLAGS="\
  -mno-red-zone              # x86_64 ABI: no red zone for bare metal
  -m64                       # 64-bit code
  -c                         # Compile only (no linking)
  -Iinclude                  # Include path
  -ffreestanding             # No hosted environment (no libc assumptions)
  -O2                        # Optimization level
  -fno-stack-protector       # No stack canary (bare metal)
  -nostdlib                  # No standard library
  -fno-builtin               # No compiler builtins
  -Wall -Wextra              # Warnings
"

find src -name "*.c" | while read -r f; do
    echo "  Compiling $f..."
    gcc $CC_FLAGS "$f" -o "$BUILD_DIR/$(basename "${f%.c}.o")"
done

# ════════════════════════════════════════════════════════════════════════════
# STEP 4: Link All Object Files
# ════════════════════════════════════════════════════════════════════════════
echo "--- Step 4: Linking Kernel ---"
# ld: GNU linker
# -m elf_x86_64: Target 64-bit x86_64 ELF
# -T link.ld: Use custom linker script
# -o kernel.elf: Output ELF executable
ld -m elf_x86_64 -T link.ld -o "$KERNEL_ELF" "$BUILD_DIR"/*.o

# ════════════════════════════════════════════════════════════════════════════
# STEP 5: Create ISO Structure
# ════════════════════════════════════════════════════════════════════════════
echo "--- Step 5: Preparing ISO Structure ---"
cp "$KERNEL_ELF" "$ISO_DIR/boot/$KERNEL_ELF"

# Create GRUB configuration
cat << 'EOF' > "$ISO_DIR/boot/grub/grub.cfg"
set timeout=5
set default=0

menuentry "Jarvis OS (x86_64 - 64bit)" {
    multiboot2 /boot/kernel.elf
    module2 /boot/ramdisk.img disk
    boot
}
EOF

# ════════════════════════════════════════════════════════════════════════════
# STEP 6: Create FAT32 Ramdisk
# ════════════════════════════════════════════════════════════════════════════
echo "--- Step 6: Creating FAT32 Ramdisk (34 MB) ---"
RAMDISK_IMG="ramdisk.img"

# dd: Create 34 MB image file filled with zeros
dd if=/dev/zero of="$RAMDISK_IMG" bs=1M count=34 status=none

# mkfs.fat: Format as FAT32 filesystem
mkfs.fat -F 32 -n JARVISFS "$RAMDISK_IMG" >/dev/null

# Embed ramdisk in ISO
cp "$RAMDISK_IMG" "$ISO_DIR/boot/ramdisk.img"

# ════════════════════════════════════════════════════════════════════════════
# STEP 7: Create Bootable ISO
# ════════════════════════════════════════════════════════════════════════════
echo "--- Step 7: Creating Hybrid ISO ---"
# grub-mkrescue: Create hybrid ISO (both BIOS and UEFI boot support)
grub-mkrescue -o "$ISO_NAME" "$ISO_DIR/"

echo "-------------------------------------------"
echo "Build complete! ISO: $ISO_NAME"
echo "-------------------------------------------"

# ════════════════════════════════════════════════════════════════════════════
# STEP 8 (Optional): Run in QEMU
# ════════════════════════════════════════════════════════════════════════════
if [[ "$*" == *"--run"* ]]; then
    echo "Booting in QEMU..."
    qemu-system-x86_64 \
        -m 4G                                    # 4 GB RAM
        -boot d                                  # Boot from CD-ROM
        -cdrom "$ISO_NAME"                       # ISO image
        -serial mon:stdio                        # Serial output to console
        -device virtio-vga                       # Graphics
        -display gtk,zoom-to-fit=on              # GUI window
fi
```

### Educational Breakdown: Why This Pipeline?

**Separate assembly/compilation**: Assembly (loader.s) must be processed by NASM (not GCC). C files use GCC for portability and readability.

**Two-stage linking**: We can't create a single binary with mixed languages in one step. Each file is compiled to an object file (.o), then the linker combines them.

**ELF format**: ELF is the industry standard for executable code. It preserves symbol information, relocation data, and debug symbols—essential for debugging and understanding the binary layout.

**Ramdisk creation**: We embed a FAT32 filesystem in the ISO so the kernel can access files without real disk hardware. Bootloader loads it into RAM, and the kernel's ATA driver treats it as a virtual disk.

**Hybrid ISO**: Modern systems boot via UEFI; older systems use BIOS. A hybrid ISO works on both, maximizing compatibility.

---

## 5. Compiler Flags Explained

Each GCC flag has a purpose in bare-metal development:

| Flag | Purpose |
|------|----------|
| `-mno-red-zone` | Disable x86_64 red zone (128 bytes below RSP). Interrupts would corrupt it. |
| `-m64` | Generate 64-bit code (not 32-bit). |
| `-ffreestanding` | Assume no hosted environment (no libc). |
| `-O2` | Optimize for speed (but not so aggressive that it breaks bare metal). |
| `-fno-stack-protector` | Disable stack canary (requires libc). |
| `-nostdlib` | Don't link standard library. |
| `-fno-builtin` | Don't use compiler builtins (which assume libc). |
| `-Wall -Wextra` | Enable all warnings for code quality. |

---

## 6. Building Jarvis OS

Now you're ready to build:

```bash
# Clone repository
git clone https://github.com/DAEDGOAT17/AOS.git
cd AOS

# Build kernel (creates kernel.elf, ramdisk.img, jarvis.iso)
./run.sh

# Build and run in QEMU
./run.sh --run

# Expected output:
# --- Step 1: Cleaning & Preparing environment ---
# --- Step 2: Assembling x86_64 ASM ---
#   Assembling src/arch/x86_64/asm/loader.s...
# --- Step 3: Compiling C Source ---
#   Compiling src/core/kernel.c...
# ... (more files) ...
# --- Step 4: Linking Kernel ---
# --- Step 5: Preparing ISO Structure ---
# --- Step 6: Creating FAT32 Ramdisk (34 MB) ---
# --- Step 7: Creating Hybrid ISO ---
# Build complete! ISO: jarvis.iso
# Booting in QEMU...
```

If the build succeeds, QEMU will launch with the Jarvis OS boot screen!

---

## 7. Troubleshooting

### Build Errors

**Error**: `nasm: command not found`
- **Solution**: `sudo apt-get install nasm`

**Error**: `gcc: fatal error: no input files`
- **Solution**: Ensure source files exist in `src/` directory. Check file paths in run.sh.

**Error**: `ld: cannot find -lc`
- **Solution**: Compiler is trying to link libc. Ensure you use `-nostdlib` flag.

**Error**: Linker script syntax error
- **Solution**: Check that `link.ld` has proper alignment syntax (`ALIGN(4K)` not `ALIGN(0x1000)`)

### Runtime Issues

**QEMU doesn't boot**
- Ensure kernel.elf has proper entry point: `readelf -h kernel.elf | grep Entry`
- Check that Multiboot headers are in the first 32 KB of the binary

**Kernel crashes immediately**
- Likely a page table or GDT issue (covered in Phase 2)
- Enable QEMU serial output: `./run.sh --run | tee qemu.log`

---

## ✅ Summary

In this phase, you learned:

✅ How to set up a cross-compiler toolchain  
✅ The structure of a bare-metal kernel project  
✅ How linker scripts direct binary layout  
✅ The 8-step build pipeline from source to bootable ISO  
✅ Compiler flags for bare-metal development  
✅ How to troubleshoot common build issues  

**Next**: [Phase 2: Boot Sequence](./02-boot-sequence.md) – Learn how the CPU transitions from 32-bit to 64-bit mode and hands control to your kernel.

---

**Exercises**:
1. Build Jarvis OS locally and verify with `./run.sh --run`
2. Inspect the kernel binary: `readelf -l kernel.elf` (shows sections and addresses)
3. Modify a compiler flag in run.sh and observe the build behavior
4. Create a simple .c file in src/ and add it to the build pipeline
