#!/usr/bin/env python3
"""Compile one C unit and compare it function by function against the original.

Run under WSL/Linux from the repo root, after `ninja` has assembled the target objects:

    .venv/bin/python tools/diff_unit.py DBG/dbalocate              # main target
    .venv/bin/python tools/diff_unit.py gx_aee:Event/stage/stg_apart_e3fe
    .venv/bin/python tools/diff_unit.py DBG/dbalocate --show dbAllocatePrintf
    .venv/bin/python tools/diff_unit.py DBG/dbalocate --show-all

The original ("target") object is build/asm/<target>/<unit>.s.o. The C file is compiled to
build/diff/<target>/<unit>.c.o with the build's flags for that unit (configure.unit_cflags:
the project flags plus the unit's config/file_flags.txt entry). Relocated fields (HI16/LO16
immediates, jump targets, pointer words) are masked on both sides, so symbol naming
differences don't count; everything else must match exactly.

Exit status is 0 when every function in the C file matches.
"""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))
import configure  # noqa: E402  (compiler paths and flags)
sys.path.insert(0, str(ROOT / "tools"))
import mwcc_fixup  # noqa: E402
import asm_fallback  # noqa: E402

R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16 = 2, 4, 5, 6, 7
MASKS = {R_MIPS_32: 0xFFFFFFFF, R_MIPS_26: 0x03FFFFFF, R_MIPS_HI16: 0xFFFF, R_MIPS_LO16: 0xFFFF,
         R_MIPS_GPREL16: 0xFFFF}


class Obj:
    """Minimal ELF32 relocatable-object reader."""

    def __init__(self, path):
        d = self.data = Path(path).read_bytes()
        (_, _, _, _, _, _, shoff, _, _, _, _, shentsize, shnum, shstrndx) = \
            struct.unpack_from("<16sHHIIIIIHHHHHH", d, 0)
        self.secs = [struct.unpack_from("<10I", d, shoff + i * shentsize) for i in range(shnum)]
        st = self.secs[shstrndx][4]
        self.names = [d[st + s[0]:d.index(b"\0", st + s[0])].decode() for s in self.secs]
        self.symbols = []  # (name, section index, value, size, type)
        for s in self.secs:
            if s[1] != 2:
                continue
            strs = self.secs[s[6]][4]
            for i in range(s[5] // 16):
                n, v, sz, info, _, shndx = struct.unpack_from("<IIIBBH", d, s[4] + i * 16)
                name = d[strs + n:d.index(b"\0", strs + n)].decode("latin1")
                self.symbols.append((name, shndx, v, sz, info & 0xF))
        # relocation masks per section: {section index: {offset: mask}}
        self.masks = {}
        self.reloc_names = {}  # {section index: {offset: target symbol name}}
        self.reloc_types = {}  # {section index: {offset: relocation type}}
        self.reloc_syms = {}  # {section index: {offset: symbol index}}
        for s in self.secs:
            if s[1] == 9:
                m = self.masks.setdefault(s[7], {})
                names = self.reloc_names.setdefault(s[7], {})
                types = self.reloc_types.setdefault(s[7], {})
                syms = self.reloc_syms.setdefault(s[7], {})
                for i in range(s[5] // 8):
                    off, info = struct.unpack_from("<II", d, s[4] + i * 8)
                    m[off] = MASKS.get(info & 0xFF, 0xFFFFFFFF)
                    types[off] = info & 0xFF
                    symi = info >> 8
                    syms[off] = symi
                    if symi < len(self.symbols):
                        names[off] = self.symbols[symi][0]

    def funcs(self):
        """{name: (section index, value, size)} for function symbols in .text sections.

        Names are normalized: the splitter appends `_XXXXXXXX` or `_gx_<ovl>` to static
        names that occur in several files; the C side uses the plain DWARF name."""
        out = {}
        for name, shndx, v, sz, typ in self.symbols:
            # Zero-size labels (splat's alabel inside hand-written functions) aren't functions.
            if typ == 2 and sz and 0 < shndx < len(self.secs) and self.names[shndx] == ".text":
                out[plain_name(name)] = (shndx, v, sz)
        return out

    def data_symbols(self, section, start):
        """{name: address} for OBJECT symbols in sections called `section`, laid out from
        `start` the way the linker would."""
        addr_of = section_addresses(self, section, start)
        out, plain = {}, {}
        for name, shndx, v, sz, typ in self.symbols:
            if typ == 1 and shndx in addr_of and not name.startswith(("@", "D_")):
                out[name] = addr_of[shndx] + v
                plain.setdefault(plain_name(name), set()).add(name)
        # Also key by the normalized name, unless it's ambiguous (agl_pos_0 / agl_pos_1).
        for p, names in plain.items():
            if len(names) == 1:
                out.setdefault(p, out[next(iter(names))])
        return out

    def words(self, shndx, value, size):
        s = self.secs[shndx]
        n = size // 4
        return list(struct.unpack_from(f"<{n}I", self.data, s[4] + value)), \
            [self.masks.get(shndx, {}).get(value + 4 * i, 0) for i in range(n)]

    def section_sizes(self):
        sizes = {}
        for name, s in zip(self.names, self.secs):
            if name in (".text", ".data", ".rodata", ".sdata", ".bss", ".sbss"):
                sizes[name] = sizes.get(name, 0) + s[5]
        return sizes


def unit_starts(target, unit):
    """{section: original start address} for one unit, from config/<target>.yaml."""
    import re
    vram = 0x100000 if target == "main" else 0x1F01E00
    out = {}
    for line in (ROOT / f"config/{target}.yaml").read_text(encoding="utf-8").splitlines():
        m = re.match(r"\s+- \[0x([0-9A-F]+), (\.?\w+), (.+)\]", line)
        if m and m.group(3) == unit:
            typ = m.group(2)
            out[".text" if typ in ("asmtu", "c") else "." + typ.lstrip(".")] = int(m.group(1), 16) + vram
        m = re.match(r"\s+- \{ type: (\.?\w+), vram: 0x([0-9A-F]+), name: (.+) \}", line)
        if m and m.group(3) == unit:
            out[".bss"] = int(m.group(2), 16)
    return out


def comparable(name):
    """Whether a relocation target name identifies an object across C and asm objects."""
    return not name.startswith(("@", "D_", ".", "jtbl_", "L")) and "$" not in name


def plain_name(name):
    """Strip the splitter's disambiguation suffix and MWCC's `$N` static-local suffix."""
    name = re.sub(r"_(?:[0-9A-F]{8}|g[xyz]_\w+)$", "", name)
    return re.sub(r"[$_]\d+$", "", name) if "$" in name or re.search(r"_\d+$", name) else name


def section_addresses(obj, name, start):
    """{section index: address} for every section called `name`, laid out from `start`."""
    out, addr = {}, start
    for i, (sname, s) in enumerate(zip(obj.names, obj.secs)):
        if sname != name:
            continue
        addr += (-addr) % max(s[8], 1)
        out[i] = addr
        addr += s[5]
    return out


ABSOLUTE = {}  # linker-defined symbols resolved in place by the original linker; set in main()


def layout(obj, name, start):
    """Lay out every section called `name` in file order from `start`, like the linker does.

    Returns (bytes, mask per byte offset -> 4-byte reloc mask, total size)."""
    blob, masks, addr = bytearray(), {}, start
    for i, (sname, s) in enumerate(zip(obj.names, obj.secs)):
        if sname != name:
            continue
        align = max(s[8], 1)
        pad = (-addr) % align
        blob += b"\0" * pad
        addr += pad
        base = len(blob)
        if s[1] != 8:  # not NOBITS
            blob += obj.data[s[4]:s[4] + s[5]]
        else:
            blob += b"\0" * s[5]
        for off, m in obj.masks.get(i, {}).items():
            masks[base + off] = m
            if ABSOLUTE:
                sym = obj.reloc_names.get(i, {}).get(off)
                if sym in ABSOLUTE:
                    w = struct.unpack_from("<I", blob, base + off)[0]
                    struct.pack_into("<I", blob, base + off,
                                     apply_abs(w, obj.reloc_types.get(i, {}).get(off), ABSOLUTE[sym]))
                    del masks[base + off]  # resolved in place: no relocation in the original either
        addr += s[5]
    return bytes(blob), masks, addr - start


def entry_placement(obj, name, start, target):
    """Where the linker will put this unit's first `name` section when the preceding object
    ends where its last symbol ends (MWCC objects have no trailing padding). None if unknown."""
    import re
    prev_end = None
    for line in (ROOT / f"config/symbols_{target}.txt").read_text(encoding="utf-8").splitlines():
        m = re.match(r"\S+ = 0x([0-9A-F]+); // (?:type:func )?size:0x([0-9A-F]+)", line)
        if m:
            a, sz = int(m.group(1), 16), int(m.group(2), 16)
            if a < start and (prev_end is None or a + sz > prev_end) and start - (a + sz) < 0x100:
                prev_end = a + sz
    # (sections emptied by dead-stripping, see tools/mwcc_fixup.py, don't count)
    first = next((s for n_, s in zip(obj.names, obj.secs) if n_ == name and s[5]), None)
    if prev_end is None or first is None or prev_end > start:
        return None
    align = max(first[8], 1)
    if target != "main" and name in (".data", ".rodata", ".bss"):
        align = max(align, 0x80)  # the overlay linker starts every section on 0x80
    pad = configure.OVERLAY_TEXT_PAD.get(target, 0) if name == ".data" else 0
    return prev_end + (-prev_end) % align + pad


def global_addresses(target):
    """{name: address} of every symbol the linker can resolve against (as configure.py PROVIDEs)."""
    out = {}
    for src in (f"config/symbols_{target}.txt", "config/symbols_main.txt", "config/symbols_extern.txt"):
        for line in (ROOT / src).read_text(encoding="utf-8").splitlines():
            m = line.split(" = ")
            if len(m) == 2:
                out.setdefault(m[0].strip(), int(m[1].split(";")[0], 16))
    return out


def check_targets(base, bf, tf, text_start, starts, real, orig, target):
    """Resolve our object's relocations the way the linker will and compare each result with the
    original binary. Relocated fields are masked in the byte compare, so without this a pointer
    to the wrong object (a swapped jump-table entry, the wrong anonymous .bss template) passes.
    Returns [(address, description)]."""
    sec_addr = {}
    for name, (shndx, v, _) in bf.items():
        if name in tf:
            sec_addr[shndx] = text_start + tf[name][1] - v
    for name in (".data", ".rodata", ".sdata", ".bss"):
        if name in starts:
            sec_addr.update(section_addresses(base, name, starts[name]))
    glob = global_addresses(target)
    gp = ABSOLUTE.get("_gp")
    o_vram, image = orig
    bad = []
    for sec, offs in base.reloc_syms.items():
        if sec not in sec_addr:
            continue
        for off, symi in offs.items():
            addr = sec_addr[sec] + off
            if addr not in real or not 0 <= addr - o_vram < len(image) - 3 or symi >= len(base.symbols):
                continue
            name, shndx, value, _, typ = base.symbols[symi]
            if name in ABSOLUTE:
                continue
            if shndx == 0:
                target_addr = glob.get(name)
            else:
                target_addr = sec_addr[shndx] + value if shndx in sec_addr else None
            if target_addr is None:
                continue
            word = struct.unpack_from("<I", base.data, base.secs[sec][4] + off)[0]
            want = struct.unpack_from("<I", image, addr - o_vram)[0]
            rtype = base.reloc_types[sec][off]
            sext = lambda x: x - 0x10000 if x & 0x8000 else x
            if rtype == R_MIPS_32:
                got, want = (target_addr + word) & 0xFFFFFFFF, want
            elif rtype == R_MIPS_26:
                got, want = ((((word & 0x3FFFFFF) << 2) + target_addr) >> 2) & 0x3FFFFFF, want & 0x3FFFFFF
            elif rtype == R_MIPS_LO16:
                got, want = (target_addr + sext(word & 0xFFFF)) & 0xFFFF, want & 0xFFFF
            elif rtype == R_MIPS_GPREL16 and gp is not None:
                got, want = (target_addr + sext(word & 0xFFFF) - gp) & 0xFFFF, want & 0xFFFF
            else:
                continue
            if got != want:
                bad.append((addr, f"relocation at 0x{addr:08X} ({name or 'section'}) resolves to "
                                  f"{got:#x}, original {want:#x}"))
    return sorted(bad)


def absolute_symbols():
    """{name: value} of linker-defined symbols (config/symbols_extern.txt, non-overlay part)."""
    out = {}
    for line in (ROOT / "config/symbols_extern.txt").read_text(encoding="utf-8").splitlines():
        name, _, value = line.partition(" = ")
        if value and name.startswith("_"):
            out[name] = int(value.rstrip(";"), 16)
    return out


def apply_abs(word, typ, value):
    """Fill a HI16/LO16/26/32 field with an absolute symbol's value."""
    if typ == R_MIPS_HI16:
        return (word & 0xFFFF0000) | (((value + 0x8000) >> 16) & 0xFFFF)
    if typ == R_MIPS_LO16:
        return (word & 0xFFFF0000) | (value & 0xFFFF)
    if typ == R_MIPS_26:
        return (word & 0xFC000000) | ((value >> 2) & 0x03FFFFFF)
    if typ == R_MIPS_32:
        return value & 0xFFFFFFFF
    return word


def real_relocs(target):
    """Every address the original ELF relocates, of any type (config/relocated_<target>.txt).
    Only these fields are pointers; anywhere else a symbol in the asm is spimdisasm's guess
    and the bytes must match."""
    text = (ROOT / f"config/relocated_{target}.txt").read_text(encoding="utf-8")
    return {int(x, 16) for x in text.split()}


def compare_section(base, tgt, name, start, real, orig):
    """None if identical (real relocated fields masked), else a description of the first difference.

    The C side is compared against the original binary (`orig`: (vram, bytes)), not the asm
    object: the asm stores spimdisasm's guessed pointers as zero plus a relocation."""
    b, bm, bsize = layout(base, name, start)
    _, tm, tsize = layout(tgt, name, start)
    if name == ".bss":
        t = bytes(tsize)
    else:
        vram, image = orig
        t = image[start - vram:start - vram + tsize]
    # The original unit's range may end with padding the next object's alignment created.
    if bsize > tsize or any(t[bsize:]):
        return f"size {bsize:#x} vs {tsize:#x}"
    for off in range(0, bsize - bsize % 4, 4):
        if bm.get(off) and start + off not in real and name != ".bss":
            # Our data holds a pointer where the original holds a plain value (before linking
            # both can read the same, e.g. NULL vs a function at its section's offset 0).
            return f"relocation at 0x{start + off:08X} where the original has none"
        m = (bm.get(off, 0) | tm.get(off, 0)) if start + off in real else 0
        bw = struct.unpack_from("<I", b, off)[0] & ~m
        tw = struct.unpack_from("<I", t, off)[0] & ~m
        if bw != tw and name != ".bss":
            return f"first difference at 0x{start + off:08X}: {b[off:off + 4].hex()} vs {t[off:off + 4].hex()}"
    if bsize % 4 and b[bsize - bsize % 4:bsize] != t[bsize - bsize % 4:bsize] and name != ".bss":
        return f"difference in the last bytes (at 0x{start + bsize - bsize % 4:08X})"
    return None


def disasm(word, vram):
    import rabbitizer
    return rabbitizer.Instruction(word, vram).disassemble()


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("unit", help="unit path like DBG/dbalocate, optionally prefixed with <target>:")
    ap.add_argument("--show", action="append", default=[], help="print side-by-side disassembly for a function")
    ap.add_argument("--show-all", action="store_true", help="print side-by-side disassembly for every mismatch")
    ap.add_argument("--src", help="compile this file instead of src/<unit>.c (for experiments)")
    ap.add_argument("--cc", help="compiler build under tools/mwcc/ to use instead of the project's (for experiments)")
    ap.add_argument("--cflags", help="extra compiler flags, appended to the unit's (for experiments)")
    ap.add_argument("--lines", action="store_true",
                    help="also compare each function's statement layout with the original's DWARF line table")
    args = ap.parse_args()

    target, unit = args.unit.split(":", 1) if ":" in args.unit else ("main", args.unit)
    src = Path(args.src).resolve() if args.src else ROOT / "src" / f"{unit}.c"
    orig = ROOT / "build" / "asm" / target / f"{unit}.s.o"
    out = ROOT / "build" / "diff" / target / f"{unit}.c.o"
    if args.src or args.cflags or args.cc:  # experiments: keep the object (and result) apart from
        # the build and progress, one per source file and settings, so they can run in parallel
        tag = hashlib.sha1(f"{src}|{args.cflags}|{args.cc}".encode()).hexdigest()[:8]
        out = ROOT / "build" / "diff_src" / target / f"{unit}.{tag}.c.o"
    if not orig.exists():
        sys.exit(f"{orig} missing: run ninja first")
    out.parent.mkdir(parents=True, exist_ok=True)
    mwcc = f"tools/mwcc/{args.cc}/mwccps2.exe" if args.cc else configure.MWCC
    extra = args.cflags.split() if args.cflags else []
    cmd = [str(ROOT / configure.WIBO), str(ROOT / mwcc), "-c", *configure.unit_cflags(unit), *extra,
           "-nostdinc", "-Iinclude", "-stderr", str(src), "-o", str(out)]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout + r.stderr)
        return 2
    mwcc_fixup.fixup(out)  # same post-processing as the build (data alignment, dead-stripping)
    fallback = asm_fallback.apply(out, target, unit)  # functions linked from the original code

    base, tgt = Obj(out), Obj(orig)
    starts = unit_starts(target, unit)
    real = real_relocs(target)
    absolute = absolute_symbols()
    ABSOLUTE.update(absolute)
    orig = (0x100000 if target == "main" else 0x1F01E00,
            (ROOT / "build" / "orig" / f"{target}.bin").read_bytes())
    text_start = starts.get(".text", 0)
    bf, tf = base.funcs(), tgt.funcs()
    ok = 0
    matched = []
    for name in sorted(bf, key=lambda n: tf.get(n, (0, 1 << 30))[1]):
        if name not in tf:
            print(f"  ?? {name}: not in the original unit")
            continue
        bw, bm = base.words(*bf[name])
        t_shndx, t_val, t_size = tf[name]
        tw, tm = tgt.words(t_shndx, t_val, len(bw) * 4 if len(bw) * 4 <= t_size else t_size)
        # Instruction words from the original binary (the asm object holds guessed relocations).
        o_vram, o_image = orig
        f_addr = text_start + t_val
        tw = list(struct.unpack_from(f"<{len(tw)}I", o_image, f_addr - o_vram))
        # Mask only fields the original really relocates. Where the original has no relocation
        # but our object references an absolute linker symbol (_end, _ovl_start_addr, ...), the
        # original linker resolved it in place: patch in its value.
        b_shndx0, b_val0, _ = bf[name]
        for i in range(len(bm)):
            if text_start + t_val + 4 * i not in real:
                if bm[i]:
                    sym = base.reloc_names.get(b_shndx0, {}).get(b_val0 + 4 * i)
                    typ = base.reloc_types.get(b_shndx0, {}).get(b_val0 + 4 * i)
                    if sym in absolute:
                        bw[i] = apply_abs(bw[i], typ, absolute[sym])
                bm[i] = 0
        for i in range(len(tm)):
            if text_start + t_val + 4 * i not in real:
                tm[i] = 0
        diffs = [i for i in range(max(len(bw), len(tw)))
                 if i >= len(bw) or i >= len(tw)
                 or (bw[i] & ~(bm[i] | tm[i])) != (tw[i] & ~(bm[i] | tm[i]))]
        # Relocated fields are masked, so also require the same target symbol where both
        # sides name one (anonymous literals and section symbols can't be compared by name).
        b_shndx, b_val, _ = bf[name]
        bn, tn = base.reloc_names.get(b_shndx, {}), tgt.reloc_names.get(t_shndx, {})
        for i in range(min(len(bw), len(tw))):
            if text_start + t_val + 4 * i not in real:
                continue  # the asm's guessed relocation; the original bytes were compared above
            a, b = bn.get(b_val + 4 * i), tn.get(t_val + 4 * i)
            if a and b and comparable(a) and comparable(b) and plain_name(a) != plain_name(b) and i not in diffs:
                diffs.append(i)
        diffs.sort()
        size_note = "" if len(bw) * 4 == t_size else f" (size {len(bw) * 4:#x} vs {t_size:#x})"
        if not diffs and not size_note:
            ok += 1
            if name in fallback:
                print(f"  AS {name} (original code: config/asm_functions.txt)")
                continue
            matched.append(name)
            print(f"  OK {name}")
            continue
        print(f"  XX {name}: {len(diffs)} differing words{size_note}")
        if args.show_all or name in args.show:
            vram = t_val
            print(f"     {'C (ours)':44} | original")
            for i in range(max(len(bw), len(tw))):
                a = disasm(bw[i], vram + 4 * i) if i < len(bw) else ""
                b = disasm(tw[i], vram + 4 * i) if i < len(tw) else ""
                print(f"   {'>>' if i in diffs else '  '} {a:44} | {b}")
    # Relocation targets: a function whose relocations resolve differently doesn't match.
    bad_targets = check_targets(base, bf, tf, text_start, starts, real, orig, target)
    target_problems = []
    for addr, desc in bad_targets:
        owner = next((n for n, (_, v, sz) in tf.items() if text_start + v <= addr < text_start + v + sz), None)
        if owner:
            if owner in matched or owner in fallback:
                if owner in matched:
                    matched.remove(owner)
                ok -= 1
                print(f"  XX {owner}: {desc}")
        else:
            target_problems.append(desc)
    missing = sorted(set(tf) - set(bf), key=lambda n: tf[n][1])
    for name in missing:
        print(f"  -- {name}: not in the C file yet")
    bs, ts = base.section_sizes(), tgt.section_sizes()
    data = "  ".join(f"{k} {bs.get(k, 0):#x}/{ts.get(k, 0):#x}" for k in (".data", ".rodata", ".sdata", ".bss")
                     if bs.get(k) or ts.get(k))
    as_note = f" (+{len(fallback)} original code)" if fallback else ""
    print(f"{unit}: {ok - len(fallback)}/{len(tf)} functions match{as_note}; data (C/original): {data or 'none'}")

    # Link simulation: every section laid out from the unit's original start address.
    starts = unit_starts(target, unit)
    problems = []
    for name in (".text", ".data", ".rodata", ".sdata", ".bss"):
        has_c = any(n == name for n in base.names)
        if name not in starts:
            if has_c and layout(base, name, 0)[2]:
                problems.append(f"{name}: the C file has {name} but the original unit has none")
            continue
        diff = compare_section(base, tgt, name, starts[name], real, orig)
        if diff:
            problems.append(f"{name}: {diff}")
        if name != ".text":
            placed = entry_placement(base, name, starts[name], target)
            if placed is not None and placed != starts[name]:
                problems.append(f"{name}: after the preceding object this unit would link at "
                                f"0x{placed:08X}, not 0x{starts[name]:08X} (first object needs more alignment?)")
        if name != ".text":
            ours, theirs = base.data_symbols(name, starts[name]), tgt.data_symbols(name, starts[name])
            for sym, addr in sorted(ours.items(), key=lambda kv: kv[1]):
                if sym in theirs and theirs[sym] != addr:
                    problems.append(f"{name}: {sym} at 0x{addr:08X}, original 0x{theirs[sym]:08X}")
                    break
    problems += target_problems[:3]
    for p in problems:
        print(f"  !! {p}")
    linkable = not problems and ok == len(tf) and not missing
    print("LINKABLE: add it to config/c_units.txt" if linkable else "not linkable yet")
    # Recorded for tools/progress.py
    out.with_suffix(".json").write_text(json.dumps({"matched": matched, "total": sorted(tf), "linkable": linkable}),
                                        encoding="utf-8")
    if args.lines:
        print_line_layout(cmd, out, target, unit, sorted(tf, key=lambda n: tf[n][1]))
    return 0 if linkable else 1


def print_line_layout(cmd, out, target, unit, funcs):
    """Compile again with -g (which doesn't change the code) and compare every function's statement
    layout with the original's DWARF line table (tools/dwarf_lines.py)."""
    import dwarf_lines
    gobj = out.with_suffix(".g.o")
    gcmd = [a if a != str(out) else str(gobj) for a in cmd]
    gcmd.insert(gcmd.index("-c") + 1, "-g")
    r = subprocess.run(gcmd, cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"  lines: -g compile failed\n{r.stdout}{r.stderr}")
        return
    ours = dwarf_lines.from_object(gobj)
    table = dwarf_lines.original()
    same = 0
    print("line layout (vs the original's DWARF line table):")
    for name in funcs:
        orig = dwarf_lines.find_original(plain_name(name), unit, table)
        mine = ours.get(plain_name(name))
        if orig is None or mine is None:
            continue
        c = dwarf_lines.compare(mine, orig)
        if c["same"]:
            same += 1
            continue
        a0, b0 = c["first"]
        parts = []
        for off, who, rel in c["structure"]:
            parts.append(f"+0x{off:x} {'only the original' if who == 'orig' else 'only ours'} starts a statement"
                         f" (line +{rel})")
        if c["spacing"]:
            parts.append(f"{len(c['spacing'])} statements at other line distances")
        print(f"  {name} (ours line {a0}, original {b0}): " + "; ".join(parts))
    print(f"  {same}/{len(funcs)} functions have the original's statement layout")


if __name__ == "__main__":
    sys.exit(main())
