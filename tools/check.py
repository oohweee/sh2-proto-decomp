#!/usr/bin/env python3
"""Compare a built binary to the original; on mismatch, report the first differing addresses.

Usage: check.py built.bin orig.bin [vram]
"""
import hashlib
import sys
from pathlib import Path



def main():
    built, orig = Path(sys.argv[1]).read_bytes(), Path(sys.argv[2]).read_bytes()
    vram = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0x100000
    if built == orig:
        print(f"OK {sys.argv[1]} sha1 {hashlib.sha1(built).hexdigest()}")
        return 0
    print(f"MISMATCH {sys.argv[1]}: size {len(built):#x} vs {len(orig):#x}")
    shown = 0
    for i in range(0, min(len(built), len(orig)), 4):
        if built[i:i + 4] != orig[i:i + 4]:
            print(f"  0x{vram + i:08X}: built {built[i:i + 4].hex()} orig {orig[i:i + 4].hex()}")
            shown += 1
            if shown == 20:
                break
    return 1


if __name__ == "__main__":
    sys.exit(main())
