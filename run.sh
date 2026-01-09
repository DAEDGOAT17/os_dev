#!/bin/bash
set -e 

# 1. Clean up from previous builds
rm -rf build
mkdir -p build
mkdir -p iso/boot/grub

echo "--- Step 1: Assembling Assembly (src/asm/*.s) ---"
for f in src/asm/*.s; do
    # This creates build/loader.o and build/interrupts.o
    nasm -f elf32 "$f" -o "build/$(basename "${f%.s}.o")"
done

echo "--- Step 2: Compiling C (src/*.c) ---"
CC_FLAGS="-m32 -c -Iinclude -ffreestanding -O2 -fno-stack-protector -nostdlib -fno-builtin"
for f in src/*.c; do
    # This creates build/kernel.o, build/idt.o, build/gdt.o, etc.
    gcc $CC_FLAGS "$f" -o "build/$(basename "${f%.c}.o")"
done

echo "--- Step 3: Linking Everything ---"
# build/*.o ensures idt_load (in loader.o) and init_idt (in idt.o) find each other
ld -m elf_i386 -T link.ld -o kernel.elf build/*.o

echo "--- Step 4: ISO Creation ---"
# Generate a fresh grub.cfg to point to our kernel
cat << EOF > iso/boot/grub/grub.cfg
set timeout=0
set default=0
menuentry "Jarvis OS" {
    multiboot /boot/kernel.elf
    boot
}
EOF

cp kernel.elf iso/boot/kernel.elf
grub-mkrescue -o jarvis.iso iso/

echo "-------------------------------------------"
echo "DONE! Run: qemu-system-i386 -cdrom jarvis.iso"
echo "-------------------------------------------"