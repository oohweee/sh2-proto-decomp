#!/usr/bin/env python3
"""Write config/min_align_<target>.txt: the smallest alignments that reproduce the layout.

gen_splat.py gives each asm object's section the largest power of two dividing its original
address (config/align_<target>.txt): that always lands it in place, but it isn't the object's real
alignment, and once code moves those huge alignments insert huge padding. This reads the link map
of a matching build and keeps, for each asm section, the smallest power of two (at least MIN) that
still puts it where it is after the preceding input section ends. Where the original had padding
before an object, that padding is the evidence for the alignment kept. configure.py prefers these
values.

    .venv/bin/python tools/min_align.py <target>     # after a matching `ninja`
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MIN = {".text": 8, ".data": 8, ".rodata": 8, ".sdata": 8, ".bss": 8}
ENTRY = re.compile(r"^ (\.\w+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)$")


def main():
    target = sys.argv[1]
    placed = {}  # (object path, section) -> required alignment
    prev_end = {}
    for line in (ROOT / f"build/{target}.map").read_text(encoding="utf-8").splitlines():
        m = ENTRY.match(line)
        if not m:
            continue
        sect, addr, size, obj = m.group(1), int(m.group(2), 16), int(m.group(3), 16), m.group(4)
        if sect not in MIN or addr == 0:  # discarded sections are listed at 0
            continue
        key = ".bss" if sect == ".bss" else "loaded"  # one running end through text, data, rodata, sdata
        end = prev_end.get(key)
        if obj.startswith("build/asm/") and obj.endswith(".s.o") and size:  # empty ones keep theirs
            align = MIN[sect]
            if end is not None:
                while (end + align - 1) & -align != addr:
                    align *= 2
                    if align > 0x10000:
                        raise SystemExit(f"{obj} {sect}: can't reach 0x{addr:X} from 0x{end:X}")
            src = obj[len("build/"):-len(".o")]
            placed[(src, sect)] = max(placed.get((src, sect), 0), align)
        if size:
            prev_end[key] = addr + size
    path = ROOT / f"config/min_align_{target}.txt"
    lines = [f"{src} {sect} 0x{value:X}" for (src, sect), value in sorted(placed.items())]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    big = sorted(((v, k) for k, v in placed.items() if v > 0x10), reverse=True)
    print(f"{target}: {len(placed)} asm sections; alignments above 16: {len(big)}")
    for v, (src, sect) in big[:12]:
        print(f"  0x{v:X} {src} {sect}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
