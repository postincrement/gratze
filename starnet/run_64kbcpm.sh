#!/bin/sh
# Assemble both slaves and verify against the committed .slv images.
set -e
cd "$(dirname "$0")"
command -v z80asm >/dev/null || { echo "error: z80asm required" >&2; exit 2; }
python3 compare_64kbcpm.py
