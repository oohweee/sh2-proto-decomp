#!/usr/bin/env python3
"""Define the ELF's symbols that sit inside asm data and binary blobs as labels in the split asm.

Some original symbols have no object of their own: labels inside the VU microcode blob
(`Shadow_micro_code`, `load_yuvprg0_mpg`...), inside asm data (`FontData`), or crt0's `_start`.
gen_splat.py lists them in config/symbols_extern.txt, and the link used to PROVIDE them at their
original absolute addresses. Defined as labels at their place in the asm instead, they move with
the code around them, so the build stays shiftable.

    label_symbols.py <target>     # rewrite asm/<target>/**/*.s in place
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LINE = re.compile(r"^\s*/\* (?:[0-9A-F]+ )?([0-9A-F]{8}) [0-9A-F]{8} \*/")
INCBIN = re.compile(r'^\s*\.incbin\s+"([^"]+)"\s*$')
DLABEL = re.compile(r"^\s*(?:glabel|dlabel)\s+(D_([0-9A-F]{8}))\s*$")
# Values in the symbol table that are sizes, not addresses.
NOT_ADDRESSES = {"__data_size"}


def label(name):
    return [f".global {name}", f"{name}:"]


def main():
    target = sys.argv[1]
    if target != "main":
        return 0  # overlay symbols are defined by the overlays' own objects
    want = {}
    # splat's own scripts assign addresses it saw referenced but found no label for (a local
    # subroutine in hand-written asm, a spot inside the VU blob): label those too, then drop them.
    auto = [ROOT / f"build/{target}.undefined_funcs_auto.txt", ROOT / f"build/{target}.undefined_syms_auto.txt"]
    sources = [ROOT / "config/symbols_extern.txt"] + [p for p in auto if p.exists()]
    for src in sources:
        for line in src.read_text(encoding="utf-8").splitlines():
            m = re.match(r"(\S+) = 0x([0-9A-Fa-f]+);", line)
            if m and m.group(1) not in NOT_ADDRESSES and 0x100000 <= int(m.group(2), 16) < 0x3C7D80:
                want.setdefault(int(m.group(2), 16), []).append(m.group(1))
    placed = set()
    for path in sorted((ROOT / "asm" / target).rglob("*.s")):
        lines = path.read_text(encoding="utf-8", errors="replace").split("\n")
        out, dirty, blob_addr = [], False, None
        for line in lines:
            d = DLABEL.match(line)
            if d:
                blob_addr = int(d.group(2), 16)
            m = LINE.match(line)
            if m:
                addr = int(m.group(1), 16)
                for name in want.get(addr, []):
                    if name not in placed:
                        out += label(name)
                        placed.add(name)
                        dirty = True
            inc = INCBIN.match(line)
            if inc and blob_addr is not None:
                # Split the blob around the labels that fall inside it.
                blob = ROOT / inc.group(1)
                size = blob.stat().st_size
                cuts = sorted(a for a in want if blob_addr <= a < blob_addr + size
                              and any(n not in placed for n in want[a]))
                if cuts:
                    pos = 0
                    for a in cuts:
                        if a - blob_addr > pos:
                            out.append(f'.incbin "{inc.group(1)}", 0x{pos:X}, 0x{a - blob_addr - pos:X}')
                        for name in want[a]:
                            if name not in placed:
                                out += label(name)
                                placed.add(name)
                        pos = a - blob_addr
                    out.append(f'.incbin "{inc.group(1)}", 0x{pos:X}, 0x{size - pos:X}')
                    dirty = True
                    continue
            out.append(line)
        if dirty:
            path.write_text("\n".join(out), encoding="utf-8", newline="\n")
    for path in auto:
        if path.exists():
            kept = [l for l in path.read_text(encoding="utf-8").splitlines()
                    if l.split(" = ")[0] not in placed]
            path.write_text("".join(l + "\n" for l in kept), encoding="utf-8", newline="\n")
    missing = [n for names in want.values() for n in names if n not in placed]
    print(f"{target}: {len(placed)} symbols labelled in asm; {len(missing)} left to the linker script")
    return 0


if __name__ == "__main__":
    sys.exit(main())
