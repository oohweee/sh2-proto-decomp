#!/usr/bin/env python3
"""Turn symbol references the original never relocated back into plain numbers in split asm.

spimdisasm symbolizes any word or %hi/%lo pair whose value looks like an address. Where the
original ELF has no relocation there (config/relocated_<target>.txt), the value is really a
number: a file size, a constant like 0xEEEEE, a coincidental match. As a symbol it assembles to
the same bytes, but it would move with that symbol if the code shifted. This rewrites such a
line as the original's raw word. Branches to local labels (PC-relative, never relocated) are left
alone.

    unsymbolize.py <target>     # rewrite asm/<target>/**/*.s in place; prints a count
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LINE = re.compile(r"^(\s*)/\* (?:[0-9A-F]+ )?([0-9A-F]{8}) ([0-9A-F]{8}) \*/\s+(.*)$")
SYMBOLIC = re.compile(r"%(?:hi|lo|gp_rel)\((?!\.L)[A-Za-z_]|^(?:jal|j)\s+(?!\.L)[A-Za-z_]|^\.word\s+(?!0x)[A-Za-z_]")


def main():
    target = sys.argv[1]
    real = {int(x, 16) for x in (ROOT / f"config/relocated_{target}.txt").read_text().split()}
    changed = 0
    for path in sorted((ROOT / "asm" / target).rglob("*.s")):
        lines = path.read_text(encoding="utf-8", errors="replace").split("\n")
        dirty = False
        for i, line in enumerate(lines):
            m = LINE.match(line)
            if not m:
                continue
            indent, vram, raw, insn = m.groups()
            if int(vram, 16) in real or not SYMBOLIC.search(insn.strip()):
                continue
            value = int.from_bytes(bytes.fromhex(raw), "little")
            lines[i] = f"{indent}/* {vram} {raw} */ .word 0x{value:08X} /* was: {insn.strip()} */"
            dirty = True
            changed += 1
        if dirty:
            path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"{target}: {changed} unrelocated symbol references made numeric")
    return 0


if __name__ == "__main__":
    sys.exit(main())
