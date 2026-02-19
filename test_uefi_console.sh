#!/bin/bash
# Test UEFI boot with console output

cd /home/deadgoat/os_dev

echo "Testing UEFI boot with kernel output..."
echo ""

qemu-system-x86_64 \
    -bios /usr/share/ovmf/OVMF.fd \
    -cdrom jarvis.iso \
    -m 256M \
    -nographic \
    -serial mon:stdio \
    2>&1
