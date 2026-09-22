#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." >/dev/null 2>&1 && pwd )"

echo "=== Building MyOS Kernel & Bootable ISO ==="
mkdir -p "$DIR/build"
cd "$DIR/build"

cmake ..
make -j$(nproc)
make iso

echo "=== Build Complete! Bootable ISO generated at: $DIR/build/MyOS.iso ==="
ls -lh "$DIR/build/MyOS.iso"
