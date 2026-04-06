#!/bin/bash
# Jarvis OS - Modern Unified Build & Run Script
# Supports: BIOS (Legacy), UEFI (Modern), QEMU (Headless/GTK)
set -e 

# Configuration
ISO_NAME="jarvis.iso"
DISK_IMG="disk.img"
BUILD_DIR="build"
ISO_DIR="iso"
KERNEL_ELF="kernel.elf"

# 1. Clean and Prepare
echo "--- Step 1: Cleaning & Preparing environment ---"
rm -rf "$BUILD_DIR" "$ISO_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$ISO_DIR/boot/grub"

# 2. Assemble Assembly Files (x86_64)
echo "--- Step 2: Assembling x86_64 ASM ---"
for f in src/arch/x86_64/asm/*.s; do
    if [ -f "$f" ]; then
        echo "  Assembling $f..."
        nasm -f elf64 "$f" -o "$BUILD_DIR/$(basename "${f%.s}.o")"
    fi
done

# 3. Compile C Files
echo "--- Step 3: Compiling C Source ---"
CC_FLAGS="-mno-red-zone -m64 -c -Iinclude -ffreestanding -O2 -fno-stack-protector -nostdlib -fno-builtin -Wall -Wextra"
find src -name "*.c" | while read -r f; do
    echo "  Compiling $f..."
    gcc $CC_FLAGS "$f" -o "$BUILD_DIR/$(basename "${f%.c}.o")"
done

# 4. Linking (64-bit ELF)
echo "--- Step 4: Linking Kernel ---"
ld -m elf_x86_64 -T link.ld -o "$KERNEL_ELF" "$BUILD_DIR"/*.o

# 5. Prepare ISO Structure
echo "--- Step 5: Preparing ISO Structure ---"
cp "$KERNEL_ELF" "$ISO_DIR/boot/$KERNEL_ELF"

# Generate Modern GRUB Config (supports Multiboot2 for UEFI)
# NOTE: module2 loads the ramdisk as physical RAM so the kernel's ATA driver
# can use it as a FAT32 filesystem on hardware with no legacy IDE disk.
cat << EOF > "$ISO_DIR/boot/grub/grub.cfg"
# Load video support (required for UEFI GOP framebuffer)
insmod all_video
insmod gfxterm
insmod font

# Try to set up a graphical console (needed on UEFI systems)
if loadfont /boot/grub/fonts/unicode.pf2; then
    set gfxmode=1024x768,auto
else
    set gfxmode=auto
fi

# Switch terminal to graphical mode so UEFI has a console
terminal_output gfxterm

set timeout=5
set default=0

menuentry "Jarvis OS (UEFI / BIOS - 64bit)" {
    # Keep the framebuffer GRUB set up; pass GOP info via Multiboot2 tag
    set gfxpayload=keep
    multiboot2 /boot/$KERNEL_ELF
    # Load the embedded FAT32 ramdisk so filesystem commands work on all hardware
    module2 /boot/ramdisk.img disk
    boot
}

menuentry "Jarvis OS (Legacy BIOS - Multiboot1)" {
    # Legacy fallback: let BIOS handle video
    set gfxpayload=text
    multiboot /boot/$KERNEL_ELF
    # Also load the ramdisk so filesystem commands work
    module /boot/ramdisk.img disk
    boot
}
EOF

# 6. Create Hybrid ISO (BIOS + UEFI)
echo "--- Step 6: Creating Hybrid ISO ---"
if command -v grub-mkrescue >/dev/null 2>&1; then
    grub-mkrescue -o "$ISO_NAME" "$ISO_DIR/"
else
    echo "ERROR: grub-mkrescue not found. Cannot create ISO."
    exit 1
fi

# 7. Create Embedded FAT32 Ramdisk (64 MB) - loaded into RAM by GRUB at boot
# This is the filesystem the kernel uses for ls/cat/write/mkdir on all hardware.
echo "--- Step 7: Creating Embedded FAT32 Ramdisk (64 MB) ---"
RAMDISK_IMG="ramdisk.img"
dd if=/dev/zero of="$RAMDISK_IMG" bs=1M count=64 status=none
if command -v mkfs.fat >/dev/null 2>&1; then
    mkfs.fat -F 32 -n JARVISFS "$RAMDISK_IMG" >/dev/null
else
    /usr/sbin/mkfs.fat -F 32 -n JARVISFS "$RAMDISK_IMG" >/dev/null
fi

# Pre-populate the ramdisk with some useful files
if command -v mcopy >/dev/null 2>&1; then
    echo "Welcome to Jarvis OS!" > /tmp/README.TXT
    echo "This filesystem lives in RAM - writes survive the session but not reboot." >> /tmp/README.TXT
    mcopy -i "$RAMDISK_IMG" /tmp/README.TXT ::/ >/dev/null
    echo "Jarvis OS System Info" > /tmp/SYSINFO.TXT
    echo "Arch: x86_64  Boot: GRUB Multiboot2" >> /tmp/SYSINFO.TXT
    mcopy -i "$RAMDISK_IMG" /tmp/SYSINFO.TXT ::/ >/dev/null
    rm -f /tmp/README.TXT /tmp/SYSINFO.TXT
fi

# Embed the ramdisk in the ISO so GRUB can load it as a module
cp "$RAMDISK_IMG" "$ISO_DIR/boot/ramdisk.img"

# 8. Create Separate Large FAT32 Disk Image for QEMU testing
echo "--- Step 8: Creating FAT32 Disk Image for QEMU (128 MB) ---"
DISK_IMG="disk.img"
dd if=/dev/zero of="$DISK_IMG" bs=1M count=128 status=none
if command -v mkfs.fat >/dev/null 2>&1; then
    mkfs.fat -F 32 "$DISK_IMG" >/dev/null
else
    /usr/sbin/mkfs.fat -F 32 "$DISK_IMG" >/dev/null
fi

# Inject test files into QEMU disk image
if command -v mcopy >/dev/null 2>&1; then
    mkdir -p staging
    echo "Welcome to Jarvis OS FAT32 (QEMU disk)!" > staging/README.TXT
    echo "This is a test file for QEMU testing." > staging/TEST.TXT
    mcopy -i "$DISK_IMG" staging/README.TXT ::/ >/dev/null
    mcopy -i "$DISK_IMG" staging/TEST.TXT ::/ >/dev/null
    rm -rf staging
fi

echo "-------------------------------------------"
echo "DONE! Jarvis OS is ready for Modern Hardware."
echo "ISO: $ISO_NAME | Disk: $DISK_IMG"
echo "-------------------------------------------"

# Both jarvis.iso and jarvis_uefi.iso are the same hybrid BIOS+UEFI image.
# We keep jarvis_uefi.iso in sync so either file can be flashed to USB.
cp "$ISO_NAME" jarvis_uefi.iso
echo "Also updated jarvis_uefi.iso (identical hybrid image)"

# 9. Optional: Export to Shared Folder (for easy Windows access)
SHARED_DIR="/media/sf_KALI_SHARED/Jarwis"
if [ -d "$SHARED_DIR" ]; then
    echo "--- Step 9: Exporting to Shared Folder ---"
    cp jarvis_uefi.iso "$SHARED_DIR/"
    if [ $? -eq 0 ]; then
        echo "Successfully copied jarvis_uefi.iso to $SHARED_DIR/"
    else
        echo "ERROR: Failed to copy to $SHARED_DIR/ (is it mounted?)"
    fi
else
    echo "Shared folder $SHARED_DIR not found. Skipping exp
    ort."
fi

echo "-------------------------------------------"

# 8. Run in QEMU (Optional)
if [[ "$*" == *"--run"* ]]; then
    QEMU_ARGS="-m 2G -boot d -cdrom $ISO_NAME -drive file=$DISK_IMG,format=raw -serial mon:stdio"
    
    if [[ "$*" == *"--uefi"* ]]; then
        # Find OVMF
        OVMF=""
        for path in "/usr/share/OVMF/OVMF_CODE.fd" "/usr/share/edk2-ovmf/x64/OVMF_CODE.fd" "/usr/share/ovmf/OVMF.fd"; do
            if [ -f "$path" ]; then OVMF="$path"; break; fi
        done
        if [ -n "$OVMF" ]; then
            echo "Booting in UEFI mode..."
            QEMU_ARGS="$QEMU_ARGS -bios $OVMF"
        else
            echo "WARNING: OVMF not found, falling back to BIOS."
        fi
    fi

    if [[ "$*" == *"--nographic"* ]]; then
        QEMU_ARGS="$QEMU_ARGS -nographic"
    else
        QEMU_ARGS="$QEMU_ARGS -device virtio-vga -display gtk,zoom-to-fit=on"
    fi

    qemu-system-x86_64 $QEMU_ARGS
fi