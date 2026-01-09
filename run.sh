#!/bin/bash
mkdir -p build

echo "Compiling Jarvis OS Modules..."

# Assemble all .s files (loader.s and interrupts.s)
for f in src/asm/*.s; do
    nasm -f elf32 "$f" -o "build/$(basename "${f%.s}.o")"
done

# Compile all C files
gcc -m32 -c src/io.c -o build/io.o -Iinclude -ffreestanding -O2
gcc -m32 -c src/screen.c -o build/screen.o -Iinclude -ffreestanding -O2
gcc -m32 -c src/idt.c -o build/idt.o -Iinclude -ffreestanding -O2
gcc -m32 -c src/keyboard.c -o build/keyboard.o -Iinclude -ffreestanding -O2
gcc -m32 -c src/kernel.c -o build/kernel.o -Iinclude -ffreestanding -O2

# Link: Force loader.o to be FIRST to ensure Multiboot header is found
ld -m elf_i386 -T link.ld -o kernel.elf build/loader.o build/io.o build/screen.o build/idt.o build/keyboard.o build/interrupts.o build/kernel.o

echo "Build complete. Running Jarvis..."
# Added -d int to help you see if a fault occurs
qemu-system-i386 -kernel kernel.elf