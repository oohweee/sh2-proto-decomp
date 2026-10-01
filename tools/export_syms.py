#!/usr/bin/env python3
"""Export main's global symbols from the linked build/main.elf as a linker script for overlays.

Overlays are linked separately at the overlay address and call into main. They resolve main's
symbols from this build's main.elf, so they follow main if it moves (the original Metrowerks
linker linked them against the main executable the same way).

    export_syms.py build/main.elf build/main_syms.ld
"""
import struct
import sys
from pathlib import Path

OVERLAY_AREA = 0x1F01E00  # main's symbols end here; overlay symbols come from the overlays


def main():
    d = Path(sys.argv[1]).read_bytes()
    shoff, = struct.unpack_from("<I", d, 0x20)
    shentsize, shnum = struct.unpack_from("<HH", d, 0x2E)
    secs = [struct.unpack_from("<10I", d, shoff + i * shentsize) for i in range(shnum)]
    symtab = next(s for s in secs if s[1] == 2)
    strs = secs[symtab[6]][4]
    out = {}
    for k in range(symtab[5] // 16):
        n, value, _, info, _, shndx = struct.unpack_from("<IIIBBH", d, symtab[4] + k * 16)
        if info >> 4 != 1 or shndx == 0:  # global and defined
            continue
        name = d[strs + n:d.index(b"\0", strs + n)].decode("latin1")
        if value >= OVERLAY_AREA and not name.startswith("_"):
            continue  # an overlay's own symbol (main names the overlay entry tables)
        if name and not name.startswith("."):
            out.setdefault(name, value)
    Path(sys.argv[2]).write_text("".join(f"{k} = 0x{v:08X};\n" for k, v in sorted(out.items())),
                                 encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
