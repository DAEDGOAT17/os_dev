#!/bin/bash
# Boot Jarvis OS in BIOS mode (legacy)

set -e

if [ ! -f jarvis.iso ]; then
    echo "Error: jarvis.iso not found. Run ./run.sh first"
    exit 1
fi

echo "Booting Jarvis OS in BIOS mode..."
echo ""

qemu-system-i386 \
    -cdrom jarvis.iso \
    -m 256M \
    -vga std \
    -enable-kvm \
    -display gtk,zoom-to-fit=on \
    2>&1 | grep -v "symbol lookup error" || true

echo ""
echo "QEMU closed"
