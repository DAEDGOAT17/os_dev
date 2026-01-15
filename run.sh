#!/bin/bash
set -e 

# 1. Clean up and prepare build environment
rm -rf build
mkdir -p build
mkdir -p iso/boot/grub

echo "--- Step 1: Assembling Assembly Files (src/asm) ---"
# We look specifically in src/asm for your .s files
for f in src/asm/*.s; do
    if [ -f "$f" ]; then
        echo "Assembling $f..."
        nasm -f elf32 "$f" -o "build/$(basename "${f%.s}.o")"
    fi
done

echo "--- Step 2: Compiling C Files (Nested src/ directories) ---"
# -Iinclude tells GCC to look in your include/ folder for headers
# We use 'find' to grab every .c file in any subfolder of src/
CC_FLAGS="-m32 -c -Iinclude -ffreestanding -O2 -fno-stack-protector -nostdlib -fno-builtin"

find src -name "*.c" | while read -r f; do
    echo "Compiling $f..."
    gcc $CC_FLAGS "$f" -o "build/$(basename "${f%.c}.o")"
done

echo "--- Step 3: Linking Everything ---"
# This takes all .o files created in the build/ folder and links them
ld -m elf_i386 -T link.ld -o kernel.elf build/*.o

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

echo "-------------------------------------------"
echo "DONE! Jarvis OS is ready."
#echo "Run: qemu-system-i386 -cdrom jarvis.iso"
echo "-------------------------------------------"
qemu-system-i386 -cdrom jarvis.iso