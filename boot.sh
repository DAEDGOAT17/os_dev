#!/bin/bash
# Main boot script for Jarvis OS

set -e

if [ ! -f jarvis.iso ]; then
    echo "Building ISO first..."
    bash run.sh
fi

echo ""
echo "=========================================="
echo "      Jarvis OS Boot Menu"
echo "=========================================="
echo ""
echo "1) Boot in BIOS mode (legacy)"
echo "2) Boot in UEFI mode (modern laptop)"
echo "3) Exit"
echo ""
read -p "Select option (1-3): " choice

case $choice in
    1)
        bash boot_bios.sh
        ;;
    2)
        bash boot_uefi.sh
        ;;
    3)
        exit 0
        ;;
    *)
        echo "Invalid option"
        exit 1
        ;;
esac
