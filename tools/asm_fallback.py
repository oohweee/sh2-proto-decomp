#!/usr/bin/env python3
"""Link the original code for functions whose C doesn't match yet (the project's INCLUDE_ASM).

config/asm_functions.txt lists `[<target>:]<unit> <function>`. The unit's C file keeps its best
C version of each listed function, marked NON_MATCHING: it still compiles, so the unit's data and
literals come out as in the original, and it is the version any non-matching build uses. After compiling,
this replaces the listed function's section with the function's code from the unit's assembled
original (build/asm/<target>/<unit>.s.o), relocations included:

- references to other units keep their symbol names, as in the asm build;
- references into this unit (its data, literals, jump tables, other functions) become
  section-relative relocations against the C object's own sections, located by address;
- data relocations that pointed into the C version (its jump tables) are retargeted to the
  original code's labels.

Nothing is baked in as an absolute address, so the object stays relocatable (shiftable).
tools/diff_unit.py applies the same step and reports such functions as `AS`, not as matched.

    asm_fallback.py file.o [<target>:]<unit>
"""
import os
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LIST = Path(os.environ.get("ASM_FALLBACK_LIST", ROOT / "config/asm_functions.txt"))  # env: for tests
sys.path.insert(0, str(ROOT / "tools"))

R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16, R_MIPS_PC16 = 2, 4, 5, 6, 7, 10


def listed(target, unit):
    """Function names listed for this unit."""
    out = []
    if not LIST.exists():
        return out
    for line in LIST.read_text(encoding="utf-8").splitlines():
        parts = line.split("#")[0].split()
        if len(parts) != 2:
            continue
        t, _, u = parts[0].rpartition(":")
        if (t or "main") == target and u == unit:
            out.append(parts[1])
    return out


def sext16(x):
    return x - 0x10000 if x & 0x8000 else x


def word(buf, off):
    return struct.unpack_from("<I", buf, off)[0]


def put(buf, off, value):
    struct.pack_into("<I", buf, off, value & 0xFFFFFFFF)


def apply(path, target, unit):
    """Replace this unit's listed functions in `path` with the original code. Returns them."""
    names = listed(target, unit)
    if not names:
        return []
    import diff_unit as du
    import elfobj

    starts = du.unit_starts(target, unit)
    text_start = starts[".text"]
    asm_path = ROOT / "build" / "asm" / target / f"{unit}.s.o"
    orig = elfobj.ElfObj(asm_path)
    ours = elfobj.ElfObj(path)
    tf = du.Obj(asm_path).funcs()  # {plain name: (shndx, value, size)}
    bf = du.Obj(path).funcs()

    # Where every section of both objects lands (the unit is linked at its original address).
    orig_base = {i: starts[s.name] for i, s in enumerate(orig.sections) if s.name in starts}
    our_base = {}
    for name, (shndx, value, _) in bf.items():
        if name in tf:
            our_base[shndx] = text_start + tf[name][1] - value
    for sect in (".data", ".rodata", ".sdata", ".bss"):
        if sect in starts:
            our_base.update(du.section_addresses(du.Obj(path), sect, starts[sect]))

    real = du.real_relocs(target)  # addresses the original ELF really relocates
    vram = 0x100000 if target == "main" else 0x1F01E00
    image = (ROOT / "build" / "orig" / f"{target}.bin").read_bytes()
    done = []
    for fn in names:
        if fn not in bf or fn not in tf:
            raise SystemExit(f"asm_fallback: {fn} is not in both the C file and the original {unit}")
        c_idx, c_val, _ = bf[fn]
        if c_val != 0:
            raise SystemExit(f"asm_fallback: {fn} doesn't start its own section")
        o_idx, o_val, o_size = tf[fn]
        code = bytearray(orig.sections[o_idx].data[o_val:o_val + o_size])
        our_sizes = {i: (o_size if i == c_idx else ours.sections[i].size) for i in our_base}

        def locate(addr):
            """(our section index, offset) of a unit address; the section it starts or ends in."""
            hits = [i for i, b in our_base.items() if b <= addr < b + our_sizes[i]]
            hits = hits or [i for i, b in our_base.items() if b <= addr <= b + our_sizes[i]]
            if not hits:
                raise SystemExit(f"asm_fallback: {fn}: no section of the C object holds {addr:#x}")
            i = max(hits, key=lambda k: our_base[k])
            return i, addr - our_base[i]

        def sym_addr(sym):
            return orig_base[sym.shndx] + sym.value if sym.shndx in orig_base else None

        entries = [e for e in orig.relocs.get(o_idx, []) if o_val <= e[0] < o_val + o_size]
        # HI16/LO16 pairs: each LO16 belongs to the latest HI16 of the same symbol; a HI16 takes
        # its target from its first LO16.
        lo_target, pending = {}, {}
        for n, (off, typ, sym) in enumerate(entries):
            if typ == R_MIPS_HI16:
                pending[id(sym)] = n
            elif typ == R_MIPS_LO16 and id(sym) in pending:
                h = pending[id(sym)]
                ahl = (word(code, entries[h][0] - o_val) & 0xFFFF) << 16
                ahl += sext16(word(code, off - o_val) & 0xFFFF)
                lo_target.setdefault(h, ahl)
                lo_target[n] = ahl
        new = []
        for n, (off, typ, sym) in enumerate(entries):
            rel = off - o_val
            if text_start + off not in real:
                # splat guessed a symbol for a plain constant (e.g. lui 0xE of 0xEEEEE): the original
                # has no relocation here, so keep the instruction as it is in the original.
                put(code, rel, word(image, text_start + off - vram))
                continue
            base = sym_addr(sym)
            if base is None:  # another unit's symbol: keep it by name, addend unchanged
                new.append([rel, typ, ours.undefined(sym.name)])
                continue
            w = word(code, rel)
            if typ == R_MIPS_PC16:
                # A branch to a global label (splat makes jump-table targets global). Branches
                # stay inside the function: resolve it here; it is position-independent.
                target_addr = base + (sext16(w & 0xFFFF) << 2) + 4  # GNU as: addend is relative to P
                here = text_start + o_val + rel
                if not text_start + o_val <= target_addr < text_start + o_val + o_size:
                    raise SystemExit(f"asm_fallback: {fn}+{rel:#x}: branch out of the function")
                put(code, rel, (w & 0xFFFF0000) | (((target_addr - here - 4) >> 2) & 0xFFFF))
                continue
            if typ == R_MIPS_32:
                target_addr = base + w
            elif typ == R_MIPS_26:
                target_addr = base + ((w & 0x3FFFFFF) << 2)
            elif typ in (R_MIPS_HI16, R_MIPS_LO16) and n in lo_target:
                target_addr = base + lo_target[n]
            elif typ == R_MIPS_GPREL16:
                target_addr = base + sext16(w & 0xFFFF)
            else:
                raise SystemExit(f"asm_fallback: {fn}+{rel:#x}: unhandled relocation type {typ}")
            idx, addend = locate(target_addr)
            if typ == R_MIPS_32:
                put(code, rel, addend)
            elif typ == R_MIPS_26:
                put(code, rel, (w & 0xFC000000) | ((addend >> 2) & 0x3FFFFFF))
            elif typ == R_MIPS_HI16:
                put(code, rel, (w & 0xFFFF0000) | (((addend + 0x8000) >> 16) & 0xFFFF))
            else:  # LO16, GPREL16
                put(code, rel, (w & 0xFFFF0000) | (addend & 0xFFFF))
            new.append([rel, typ, ours.section_symbol(idx)])

        ours.sections[c_idx].data = code
        ours.relocs[c_idx] = new
        for s in ours.symbols:
            if s.shndx == c_idx and s.name == fn:
                s.size = o_size
        # Data words that pointed into the C version (jump tables, function pointers): point them
        # at the same place in the original code.
        c_sym = ours.section_symbol(c_idx)
        for sec_idx, sec_relocs in ours.relocs.items():
            if sec_idx == c_idx or sec_idx not in our_base or ours.sections[sec_idx].name == ".text":
                continue
            for e in sec_relocs:
                if e[2].shndx == c_idx and e[1] == R_MIPS_32:
                    addr = our_base[sec_idx] + e[0]
                    put(ours.sections[sec_idx].data, e[0], word(image, addr - vram) - our_base[c_idx])
                    e[2] = c_sym
        done.append(fn)
    ours.write(path)
    return done


def main():
    path, arg = sys.argv[1], sys.argv[2]
    target, _, unit = arg.rpartition(":")
    apply(path, target or "main", unit)
    return 0


if __name__ == "__main__":
    sys.exit(main())
