#!/bin/bash

# Ensure script is run with root privileges (required for dd)
if [ "$EUID" -ne 0 ]; then
  echo "Please run this script as root: sudo ./flash_usb.sh"
  exit 1
fi

if [ ! -f "jarvis.iso" ]; then
    echo "Error: jarvis.iso not found! Please run ./run.sh first to build the OS."
    exit 1
fi

echo "============================================="
echo "   Jarvis OS USB Flashing Utility"
echo "============================================="
echo ""
echo "Available removable USB drives on your system:"
# Display a list of attached USB drives
lsblk -d -o NAME,MODEL,SIZE,TRAN | grep -i usb

echo ""
echo "WARNING: Be extremely careful! If you type the wrong drive letter (like your main Linux drive), it will instantly wipe your actual computer!"
read -p "Enter the device name of your USB drive (e.g., sdb, sdc): " usb_dev

if [ -z "$usb_dev" ]; then
    echo "No device entered. Exiting."
    exit 1
fi

# Basic safety override to prevent destroying the main Linux OS drive
if [ "$usb_dev" = "sda" ] || [ "$usb_dev" = "nvme0n1" ]; then
    echo "DANGER: You selected what looks like a primary hard drive ($usb_dev)!"
    echo "Aborting immediately to protect your data."
    exit 1
fi

if [ ! -e "/dev/$usb_dev" ]; then
    echo "Error: Device /dev/$usb_dev does not exist."
    exit 1
fi

echo ""
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "WARNING: All data on /dev/$usb_dev will be DESTROYED!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
read -p "Type exactly 'YES' to format the drive and flash Jarvis OS: " confirm

if [ "$confirm" != "YES" ]; then
    echo "Aborted."
    exit 1
fi

echo ""
echo "Unmounting target drive..."
umount /dev/${usb_dev}* 2>/dev/null || true

echo "Flashing jarvis.iso to /dev/$usb_dev..."
# Use dd to do a direct bit-for-bit flashing of the ISO to the raw USB block device
dd if=jarvis.iso of=/dev/$usb_dev bs=4M status=progress oflag=sync

echo ""
echo "=========================================================="
echo "SUCCESS! Jarvis OS is now flashed to your USB drive."
echo "=========================================================="
echo "You can now unplug it, insert it into your modern UEFI laptop,"
echo "disable Secure Boot in the BIOS, and boot directly into your OS!"
