#!/bin/bash
# Boot Jarvis OS in UEFI mode using OVMF firmware
set -e

JARWIS_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$JARWIS_DIR"

# ── Step 1: Rebuild the kernel and ISO ────────────────────────────────────────
echo "=== Rebuilding Jarvis OS ==="

rm -rf build
mkdir -p build
mkdir -p iso/boot/grub

echo "--- Assembling ASM files ---"
for f in src/arch/x86_64/asm/*.s; do
    [ -f "$f" ] || continue
    echo "  nasm: $f"
    nasm -f elf64 "$f" -o "build/$(basename "${f%.s}.o")"
done

echo "--- Compiling C files ---"
CC_FLAGS="-mno-red-zone -m64 -c -Iinclude -ffreestanding -O2 -fno-stack-protector -nostdlib -fno-builtin"
while IFS= read -r f; do
    echo "  gcc: $f"
    gcc $CC_FLAGS "$f" -o "build/$(basename "${f%.c}.o")"
done < <(find src -name "*.c")

echo "--- Linking ---"
ld -A i386 -m elf_x86_64 -T link.ld -o kernel.elf build/*.o

echo "--- Building ISO with UEFI + BIOS support ---"
cp kernel.elf iso/boot/kernel.elf

# Create UEFI-capable ISO: grub-mkrescue auto-includes both i386-pc and x86_64-efi
grub-mkrescue -o jarvis.iso iso/

echo "=== Build complete ==="

# ── Step 2: Locate OVMF firmware ──────────────────────────────────────────────
# Prefer the split CODE+VARS pair (more correct for EFI variable runtime)
OVMF_CODE=""
OVMF_VARS=""

declare -a CODE_PATHS=(
    "/usr/share/OVMF/OVMF_CODE.fd"
    "/usr/share/OVMF/OVMF_CODE_4M.fd"
    "/usr/share/edk2-ovmf/x64/OVMF_CODE.fd"
)
declare -a VARS_PATHS=(
    "/usr/share/OVMF/OVMF_VARS.fd"
    "/usr/share/OVMF/OVMF_VARS_4M.fd"
    "/usr/share/edk2-ovmf/x64/OVMF_VARS.fd"
)

for i in "${!CODE_PATHS[@]}"; do
    if [ -f "${CODE_PATHS[$i]}" ] && [ -f "${VARS_PATHS[$i]}" ]; then
        OVMF_CODE="${CODE_PATHS[$i]}"
        OVMF_VARS="${VARS_PATHS[$i]}"
        break
    fi
done

# Fallback: single combined OVMF.fd
if [ -z "$OVMF_CODE" ]; then
    for path in "/usr/share/ovmf/OVMF.fd" "/usr/share/qemu/OVMF.fd"; do
        if [ -f "$path" ]; then
            OVMF_CODE="$path"
            break
        fi
    done
fi

if [ -z "$OVMF_CODE" ]; then
    echo "ERROR: OVMF firmware not found. Install with:"
    echo "  sudo apt-get install ovmf"
    exit 1
fi

# ── Step 3: Copy VARS to a writable temp file (OVMF_VARS must be writable) ───
if [ -n "$OVMF_VARS" ]; then
    VARS_TMP=$(mktemp /tmp/OVMF_VARS.XXXXXX.fd)
    cp "$OVMF_VARS" "$VARS_TMP"
    echo "Using OVMF CODE: $OVMF_CODE"
    echo "Using OVMF VARS: $VARS_TMP (writable copy)"
    OVMF_ARGS="-drive if=pflash,format=raw,readonly=on,file=${OVMF_CODE} \
               -drive if=pflash,format=raw,file=${VARS_TMP}"
else
    echo "Using combined OVMF: $OVMF_CODE"
    OVMF_ARGS="-bios ${OVMF_CODE}"
fi

# ── Step 4: Launch QEMU in UEFI mode ─────────────────────────────────────────
echo ""
echo "=== Booting Jarvis OS in UEFI mode ==="
echo "    GRUB menu auto-selects after 3 seconds"
echo ""

qemu-system-x86_64 \
    $OVMF_ARGS \
    -cdrom jarvis.iso \
    -drive file=disk.img,format=raw,index=0,media=disk \
    -m 1G \
    -serial mon:stdio \
    -device virtio-vga,xres=1920,yres=1080 \
    -display gtk,zoom-to-fit=on \
    2>&1

if [ -n "$VARS_TMP" ]; then
    rm -f "$VARS_TMP"
fi

echo ""
echo "QEMU closed."
