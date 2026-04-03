#!/bin/bash
set -e 

# 1. Clean up and prepare build environment
rm -rf build
mkdir -p build
mkdir -p iso/boot/grub

echo "--- Step 1: Assembling Assembly Files (src/arch/x86_64/asm) ---"
# We look specifically in src/arch/x86_64/asm for your .s files
for f in src/arch/x86_64/asm/*.s; do
    if [ -f "$f" ]; then
        echo "Assembling $f..."
        nasm -f elf64 "$f" -o "build/$(basename "${f%.s}.o")"
    fi
done

echo "--- Step 2: Compiling C Files (Nested src/ directories) ---"
# -Iinclude tells GCC to look in your include/ folder for headers
# We use 'find' to grab every .c file in any subfolder of src/
CC_FLAGS="-mno-red-zone -m64 -c -Iinclude -ffreestanding -O2 -fno-stack-protector -nostdlib -fno-builtin"

find src -name "*.c" | while read -r f; do
    echo "Compiling $f..."
    gcc $CC_FLAGS "$f" -o "build/$(basename "${f%.c}.o")"
done

echo "--- Step 3: Linking Everything ---"
# This takes all .o files created in the build/ folder and links them
ld -A i386 -m elf_x86_64 -T link.ld -o kernel.elf build/*.o

echo "--- Step 4: ISO Creation ---"
# Copy the kernel to the ISO staging area
cp kernel.elf iso/boot/kernel.elf

# Ensure grub.cfg is present (using the one you already have or creating a default)
if [ ! -f iso/boot/grub/grub.cfg ]; then
    cat << EOF > iso/boot/grub/grub.cfg
set timeout=0
set default=0
menuentry "Jarvis OS" {
    multiboot /boot/kernel.elf
    boot
}
EOF
fi

grub-mkrescue -o jarvis.iso iso/

# 5. Create Disk Image (always rebuild to ensure brain.bin is present)
echo "--- Step 5: Creating FAT32 Disk Image ---"
DISK_IMG="disk.img"
dd if=/dev/zero of="$DISK_IMG" bs=1M count=128
/usr/sbin/mkfs.fat -F 32 "$DISK_IMG"

# Create test content
mkdir -p staging
echo "Welcome to Jarwis OS FAT32!" > staging/README.TXT
echo "This is a test file." > staging/TEST.TXT
mcopy -i "$DISK_IMG" staging/README.TXT ::/
mcopy -i "$DISK_IMG" staging/TEST.TXT ::/
rm -rf staging

# Inject AI brain weights into the ISO as a boot module
if [ -f "brain.bin" ]; then
    echo "Copying brain.bin to ISO as a boot module..."
    cp brain.bin iso/boot/brain.bin
    echo "brain.bin injected successfully!"
else
    echo "WARNING: brain.bin not found. Run scripts/export_nanogpt.py to generate it."
    echo "  python3 scripts/export_nanogpt.py"
fi

echo "-------------------------------------------"
echo "DONE! Jarvis OS is ready."
#echo "Run: qemu-system-i386 -cdrom jarvis.iso -hda disk.img"
echo "-------------------------------------------"
qemu-system-x86_64 -m 2G -boot d -cdrom jarvis.iso -drive file=disk.img,format=raw \
    -device virtio-vga,xres=1920,yres=1080 \
    -display gtk,zoom-to-fit=on