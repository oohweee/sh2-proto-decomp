#!/usr/bin/env python3
"""Restore relocations the original has but the split asm lost.

Where the original ELF relocates an instruction but spimdisasm wrote it without a symbol (a
`lui` of a section-relative .bss address written as a constant, a pair addressing a table inside
a function's code), the assembled object has no relocation there and the value would not follow
its target if code or data moved. This rewrites each such line as an explicit relocation to the
symbol that contains the target, plus an offset:

    .reloc ., R_MIPS_HI16, _copyRefImage+0x48
    /* ... */ .word 0x3C0A0000

GNU as puts the addend into the instruction (REL), so the bytes stay the original's. Relocations
of the Metrowerks VU types (inside the VU microcode blobs) address VU memory and are left alone.

    resymbolize.py <target>     # after unsymbolize.py and label_symbols.py
"""
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import dwarf1  # noqa: E402
import gen_splat  # noqa: E402

TYPES = {2: "R_MIPS_32", 4: "R_MIPS_26", 5: "R_MIPS_HI16", 6: "R_MIPS_LO16", 7: "R_MIPS_GPREL16"}
FIELD = {2: 0xFFFFFFFF, 4: 0x03FFFFFF, 5: 0xFFFF, 6: 0xFFFF, 7: 0xFFFF}
LINE = re.compile(r"^(\s*)(/\* (?:[0-9A-F]+ )?([0-9A-F]{8}) ([0-9A-F]{8}) \*/)\s+(.*)$")
SYMBOLIC = re.compile(r"%(?:hi|lo|gp_rel)\(|^(?:jal|j)\s+[A-Za-z_.]|^\.word\s+[A-Za-z_]")


def sext16(x):
    return x - 0x10000 if x & 0x8000 else x


def main():
    target = sys.argv[1]
    vram = 0x100000 if target == "main" else 0x1F01E00
    image = (ROOT / f"build/orig/{target}.bin").read_bytes()
    word = lambda a: struct.unpack_from("<I", image, a - vram)[0]
    elf = gen_splat.read_elf(dwarf1.DEFAULT_ELF)
    relocs = sorted(elf.relocs.get(target, []))
    by_addr = {off: (info & 0xFF, info >> 8) for off, info in relocs}
    # Symbols of this build, to name targets: (start, size, name), from the splitter's list.
    syms = []
    for line in (ROOT / f"config/symbols_{target}.txt").read_text(encoding="utf-8").splitlines():
        m = re.match(r"(\S+) = 0x([0-9A-F]+); // (?:type:\w+ )?size:0x([0-9A-F]+)", line)
        if m:
            syms.append((int(m.group(2), 16), int(m.group(3), 16), m.group(1)))
    syms.sort()

    def target_of(addr, typ, symi):
        w = word(addr)
        if typ == 2:
            return w
        if typ == 4:
            return ((addr + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
        if typ in (5, 6):
            # Pair with the nearest relocation of the other half against the same symbol.
            other = 6 if typ == 5 else 5
            near = [o for o, (t, s) in by_addr.items() if t == other and s == symi and abs(o - addr) < 0x800]
            if not near:
                return None
            o = min(near, key=lambda x: abs(x - addr))
            hi, lo = (w, word(o)) if typ == 5 else (word(o), w)
            return ((hi & 0xFFFF) << 16) + sext16(lo & 0xFFFF)
        return None

    # spimdisasm's own labels (D_XXXXXXXX) in the asm, for targets no named symbol contains.
    labels = sorted({int(x, 16) for p in (ROOT / "asm" / target).rglob("*.s")
                     for x in re.findall(r"^\s*dlabel D_([0-9A-F]{8})\s*$",
                                         p.read_text(encoding="utf-8", errors="replace"), re.M)})

    def name_for(t):
        best = None
        for start, size, name in syms:
            if start > t:
                break
            if start <= t < start + max(size, 1):
                best = (name, t - start)
        if best is None:
            below = [a for a in labels if a <= t and t - a < 0x10000]
            if below:
                best = (f"D_{max(below):08X}", t - max(below))
        return best

    fixed = unresolved = 0
    for path in sorted((ROOT / "asm" / target).rglob("*.s")):
        lines = path.read_text(encoding="utf-8", errors="replace").split("\n")
        out, dirty = [], False
        for line in lines:
            m = LINE.match(line)
            if m:
                indent, comment, addr, raw, insn = m.groups()
                a = int(addr, 16)
                already = out and out[-1].strip().startswith(".reloc")
                if a in by_addr and by_addr[a][0] in TYPES and not SYMBOLIC.search(insn.strip()) and not already:
                    typ, symi = by_addr[a]
                    t = target_of(a, typ, symi)
                    named = name_for(t) if t is not None else None
                    if named is None:
                        unresolved += 1
                        print(f"  {target}: 0x{a:08X} {TYPES[typ]}: no symbol for target "
                              f"{'?' if t is None else hex(t)}")
                    else:
                        value = int.from_bytes(bytes.fromhex(raw), "little") & ~FIELD[typ] & 0xFFFFFFFF
                        name, off = named
                        out.append(f"{indent}.reloc ., {TYPES[typ]}, {name}+0x{off:X}")
                        out.append(f"{indent}{comment} .word 0x{value:08X} /* was: {insn.strip()} */")
                        dirty = True
                        fixed += 1
                        continue
            out.append(line)
        if dirty:
            path.write_text("\n".join(out), encoding="utf-8", newline="\n")
    print(f"{target}: {fixed} lost relocations restored, {unresolved} unresolved")
    return 1 if unresolved else 0


if __name__ == "__main__":
    sys.exit(main())
