#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." >/dev/null 2>&1 && pwd )"

ISO="$DIR/build/MyOS.iso"

if [ ! -f "$ISO" ]; then
    echo "MyOS.iso not found! Building now..."
    "$DIR/scripts/build.sh"
fi

echo "Launching MyOS in QEMU (x86_64, 512MB RAM, VBE graphics, serial output)..."
qemu-system-x86_64 \
    -cdrom "$ISO" \
    -m 512M \
    -vga std \
    -serial stdio \
    -boot d
