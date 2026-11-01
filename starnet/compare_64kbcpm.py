#!/usr/bin/env python3
"""Assemble 64kbcpm.asm / n64kbcpm.asm and compare to the .slv images."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent

def check(src: str, slv: str) -> int:
    out = ROOT / f"._{slv}.bin"
    r = subprocess.run(["z80asm", src, "-o", str(out)], cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr, file=sys.stderr)
        return 1
    a, b = out.read_bytes(), (ROOT / slv).read_bytes()
    out.unlink(missing_ok=True)
    if a == b:
        print(f"OK {src} -> {slv} ({len(a)} bytes)")
        return 0
    diffs = sum(1 for x, y in zip(a, b) if x != y) + abs(len(a) - len(b))
    print(f"FAIL {src} vs {slv}: {diffs} byte differences", file=sys.stderr)
    return 1

sys.exit(check("64kbcpm.asm", "64kbcpm.slv") | check("n64kbcpm.asm", "n64kbcpm.slv"))
