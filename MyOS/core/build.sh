#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
make clean
make
echo
echo "MyOS ISO: $(pwd)/MyOS.iso"
