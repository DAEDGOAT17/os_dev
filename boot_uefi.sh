#!/bin/bash
# Boot Jarvis OS in UEFI mode (simulates modern laptop)

set -e

# Check if ISO exists
if [ ! -f jarvis.iso ]; then
    echo "Error: jarvis.iso not found. Run ./run.sh first"
    exit 1
fi

# Find OVMF firmware
OVMF_PATHS=(
    "/usr/share/ovmf/OVMF.fd"
    "/usr/share/OVMF/OVMF_CODE.fd"
    "/usr/share/edk2-ovmf/x64/OVMF_CODE.fd"
    "/usr/share/qemu/OVMF.fd"
)

OVMF_FD=""
for path in "${OVMF_PATHS[@]}"; do
    if [ -f "$path" ]; then
        OVMF_FD="$path"
        break
    fi
done

if [ -z "$OVMF_FD" ]; then
    echo "OVMF firmware not found. Install it:"
    echo "  Ubuntu/Debian: sudo apt-get install ovmf"
    echo "  Fedora: sudo dnf install edk2-ovmf"
    echo "  Arch: sudo pacman -S edk2-ovmf"
    exit 1
fi

echo "Using OVMF firmware: $OVMF_FD"
echo "Booting Jarvis OS in UEFI mode with serial console..."
echo "Press Enter to boot from GRUB menu"
echo ""

# Boot with UEFI firmware
# Serial console output visible in terminal
qemu-system-x86_64 \
    -bios "$OVMF_FD" \
    -cdrom jarvis.iso \
    -m 256M \
    -serial mon:stdio \
    -display gtk,zoom-to-fit=on \
    2>&1

echo ""
echo "QEMU closed"
