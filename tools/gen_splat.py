#!/usr/bin/env python3
"""Generate splat configs, symbol lists and relocation lists from the prototype ELF.

Targets: `main` (the ELF's `main` section) and every Metrowerks overlay whose binary is on
the disc (`GX/<NAME>.BIN`, linked at 0x1F01E00). For each target T this writes
config/T.yaml, config/symbols_T.txt and config/relocs_T.txt.

Text is split into one translation unit per original source file, using the DWARF compile
units (Metrowerks emits one CU per function, named after its source file). Inline functions
from headers belong to the enclosing .c file. Code without DWARF (Sony libraries, the C
library, newlib) is split at the zero-size `.text` section symbols the GNU-built library objects
left in the symbol table.

The ELF's relocations are turned into splat reloc entries so every pointer gets its real
symbol instead of a guess.

Data, rodata and bss are single blocks per target for now; they get split per file as
files move to C.
"""
import bisect
import hashlib
import re
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import dwarf1  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "baserom" / "disc" / "SLUS_202.28"
GX_DIR = ROOT / "baserom" / "disc" / "GX"

MAIN_VRAM = 0x100000
TEXT_START = 0x1000C0     # first game function; crt0 is before it
VUTEXT_START = 0x299300   # VU microcode (DMA/VIF packets)
DATA_START = 0x2A7400     # __data_start
RODATA_START = 0x389000
SDATA_START = 0x3C7D50
BSS_START = 0x3C7D80      # .sbss then .bss
BSS_END = 0x1F01E00       # end of main's memsz; overlays load here
GP_VALUE = 0x3CFC70       # set by crt0
OVERLAY_VRAM = 0x1F01E00

SRC_PREFIXES = [
    ("E:\\work\\sh2(CVS全取得)\\src\\", ""),
    ("M:\\select\\sound\\sd0712\\ee\\", "sd0712/ee/"),
    ("D:\\Program Files\\Metrowerks\\CodeWarrior\\PS2 Support\\", "mwcc_runtime/"),
]

R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16 = 2, 4, 5, 6
RELOC_NAMES = {R_MIPS_32: "MIPS_32", R_MIPS_26: "MIPS_26",
               R_MIPS_HI16: "MIPS_HI16", R_MIPS_LO16: "MIPS_LO16"}


@dataclass
class Sym:
    name: str
    value: int
    size: int
    type: int
    bind: int
    section: str
    out: str | None = None


@dataclass
class Elf:
    data: bytes
    sections: dict                      # name -> (offset, size, addr)
    syms: list                          # all Sym with a real section
    by_index: dict                      # symtab index -> Sym
    relocs: dict = field(default_factory=dict)  # target section -> [(offset, info)]


def read_elf(path):
    data = path.read_bytes()
    (_, _, _, _, _, _, shoff, _, _, _, _, shentsize, shnum, shstrndx) = \
        struct.unpack_from("<16sHHIIIIIHHHHHH", data, 0)
    secs = [struct.unpack_from("<10I", data, shoff + i * shentsize) for i in range(shnum)]
    st = secs[shstrndx][4]
    names = [data[st + s[0]:data.index(b"\0", st + s[0])].decode() for s in secs]
    symtab = secs[names.index(".symtab")]
    strs = secs[symtab[6]][4]
    syms, by_index = [], {}
    for i in range(symtab[5] // 16):
        n, value, size, info, _, shndx = struct.unpack_from("<IIIBBH", data, symtab[4] + i * 16)
        if not 0 < shndx < shnum:
            continue
        name = data[strs + n:data.index(b"\0", strs + n)].decode("cp932", "replace")
        sym = Sym(name, value, size, info & 0xF, info >> 4, names[shndx])
        syms.append(sym)
        by_index[i] = sym
    elf = Elf(data, {n: (s[4], s[5], s[3]) for n, s in zip(names, secs)}, syms, by_index)
    for n, s in zip(names, secs):
        if s[1] == 9 and n.startswith(".rel"):
            elf.relocs[n[4:]] = [struct.unpack_from("<II", data, s[4] + i * 8) for i in range(s[5] // 8)]
    return elf


def unit_path(cu_name):
    for prefix, repl in SRC_PREFIXES:
        if cu_name.startswith(prefix):
            rel = repl + cu_name[len(prefix):].replace("\\", "/").replace(" ", "_")
            return re.sub(r"\.c$", "", rel)
    raise ValueError(f"unknown source prefix: {cu_name}")


def dwarf_sources(dw):
    """{(low_pc, function name): [source files]} for every DWARF function.

    Overlays share one load address, so the same key can come from several overlays.
    """
    out = {}
    for cu in dw.roots:
        for child in cu.children:
            if dwarf1.AT_low_pc in child.attrs:
                out.setdefault((child.attrs[dwarf1.AT_low_pc], child.name), []).append(cu.name)
    return out


# Overlay .rodata the splitter can't tell from .data: the Metrowerks linker puts it after .data,
# on the next 0x80 boundary, with no symbol marking it. Offset in the overlay image.
OVERLAY_RODATA = {
    "gx_awx": 0x2B80,  # stg_apart_w1f: TrimColorFilter's jump table
}


def text_units(funcs, lib_starts, sources, lib_prefix):
    """[(vram, unit_name)] for the text TUs of one target, in address order.

    funcs: sorted [(addr, name)]; lib_starts: addresses of library object starts.
    """
    units, current, current_src = [], None, None
    pending = None  # address of header functions seen before the first unit
    for i, (addr, name) in enumerate(funcs):
        cands = sources.get((addr, name), [])
        if len(cands) > 1:
            # Ambiguous (same name and address in several overlays): prefer the file of the
            # surrounding functions.
            nxt = sources.get(funcs[i + 1], []) if i + 1 < len(funcs) else []
            cands = [c for c in cands if c == current_src] or [c for c in cands if c in nxt] or cands
        src = cands[0] if cands else None
        if src and not src.lower().endswith(".h"):
            current_src = src
        if src and src.lower().endswith(".h"):
            unit = current  # inline function from a header: part of the enclosing .c unit
            if current is None and pending is None:
                pending = addr  # ...which starts before the first .c function (overlays)
        elif src:
            unit = unit_path(src)
        elif addr in lib_starts or current is None or not current.startswith(lib_prefix):
            unit = f"{lib_prefix}{name}"
        else:
            unit = current
        if unit != current:
            units.append((addr if current is not None or pending is None else pending, unit))
            current = unit
    # A few library objects start with a label that is not a function symbol.
    for addr in sorted(lib_starts):
        if not any(a == addr for a, _ in units):
            owner = max((a, u) for a, u in units if a < addr)
            units.append((addr, f"{owner[1]}_{addr:X}"))
    units.sort()
    seen = {}
    for addr, unit in units:
        if unit in seen:
            raise SystemExit(f"unit {unit} is not contiguous ({seen[unit]:#x}, {addr:#x})")
        seen[unit] = addr
    return units


def sanitize(name):
    return re.sub(r"[^A-Za-z0-9_.]", "_", name)


def assign_names(syms):
    """Give every FUNC/OBJECT symbol a unique, assembler-safe output name across all targets.

    A global keeps its plain name. Other duplicates get the overlay name (for overlay symbols)
    and/or the address appended.
    """
    groups = {}
    for s in syms:
        base = f"D_{s.value:08X}" if s.name.startswith("@") else sanitize(s.name)
        s.out = base
        groups.setdefault(base, []).append(s)
    for base, group in groups.items():
        if len(group) == 1:
            continue
        globals_ = [s for s in group if s.bind == 1]
        keep = globals_[0] if len(globals_) == 1 else None
        for s in group:
            if s is keep:
                continue
            if s.section != "main":
                s.out = f"{base}_{s.section}"
            else:
                s.out = f"{base}_{s.value:08X}"
    final = {}
    for s in syms:
        if s.out in final and final[s.out] is not s:
            s.out = f"{s.out}_{s.value:08X}"
        final[s.out] = s


def symbol_lines(syms):
    """symbol_addrs lines for one target. Aliases at the same address share one name."""
    kept, lines = {}, []
    for s in sorted(syms, key=lambda s: (s.value, s.type != 2)):
        key = (s.value, s.type)
        if key in kept:
            s.out = kept[key].out
            continue
        kept[key] = s
        attrs = ["type:func"] if s.type == 2 else []
        if 0 < s.size < 0x1000000:
            attrs.append(f"size:0x{s.size:X}")
        lines.append(f"{s.out} = 0x{s.value:08X}; // {' '.join(attrs)}".rstrip(" /"))
    return lines


def reloc_targets(elf, relocs, image, vram):
    """Yield (offset, type, symbol, target) for the HI16/LO16/26/32 relocations inside `image`.

    REL relocations keep the addend in the instruction/word. For HI16/LO16 the full
    target is (hi << 16) + sext(lo): an HI16 pairs with the next LO16 against the same
    symbol, and a LO16 with the closest preceding HI16 against the same symbol.
    Relocations that can't be resolved are yielded with target None.
    """
    end = vram + len(image)
    word = lambda addr: struct.unpack_from("<I", image, addr - vram)[0]
    sext16 = lambda v: v - 0x10000 if v & 0x8000 else v

    lo_imm_for_hi, pending = {}, []
    for off, info in relocs:
        typ, symi = info & 0xFF, info >> 8
        if typ == R_MIPS_HI16:
            pending.append((off, symi))
        elif typ == R_MIPS_LO16:
            for hoff, _ in [p for p in pending if p[1] == symi]:
                lo_imm_for_hi[hoff] = sext16(word(off) & 0xFFFF)
            pending = [p for p in pending if p[1] != symi]

    last_hi = {}
    for off, info in relocs:
        typ, symi = info & 0xFF, info >> 8
        if typ not in RELOC_NAMES or not (vram <= off < end):
            continue
        sym = elf.by_index.get(symi)
        w = word(off)
        target = None
        if typ == R_MIPS_32:
            target = w
        elif typ == R_MIPS_26:
            target = ((w & 0x3FFFFFF) << 2) | ((off + 4) & 0xF0000000)
        elif typ == R_MIPS_HI16:
            last_hi[symi] = w & 0xFFFF
            if off in lo_imm_for_hi:
                target = ((w & 0xFFFF) << 16) + lo_imm_for_hi[off]
        elif symi in last_hi:
            target = (last_hi[symi] << 16) + sext16(w & 0xFFFF)
        if sym is None or sym.value < MAIN_VRAM:
            target = None  # VU/DVP labels and other non-EE addresses
        yield off, typ, sym, None if target is None else target & 0xFFFFFFFF


def reloc_lines(elf, relocs, image, vram, own_syms, main_syms):
    """splat reloc_addrs lines for one target."""
    candidates = sorted(((s.value, s) for s in own_syms + main_syms if s.out), key=lambda t: t[0])
    starts = [v for v, _ in candidates]
    own = {id(s) for s in own_syms} | {id(s) for s in main_syms}

    def name_for(sym, target):
        if id(sym) in own and sym.out:
            return sym.out, target - sym.value
        # Section or unnamed base symbol: use the named symbol containing the target.
        i = bisect.bisect_right(starts, target) - 1
        if i >= 0:
            v, s = candidates[i]
            if target == v or target < v + max(s.size, 1):
                return s.out, target - v
        return None

    out, skipped = [], 0
    for off, typ, sym, target in reloc_targets(elf, relocs, image, vram):
        named = None if target is None else name_for(sym, target)
        if named is None:
            skipped += 1
            continue
        name, addend = named
        line = f"rom:0x{off - vram:X} reloc:{RELOC_NAMES[typ]} symbol:{name}"
        if addend:
            line += f" addend:{'-' if addend < 0 else ''}0x{abs(addend):X}"
        out.append(line)
    return out, skipped


def yaml_text(name, image, vram, bss_subs, bss_end, subsegments, extra_symbol_files, gp=None):
    sub = "\n".join(f"      - {s}" for s in subsegments)
    sub += "".join(f"\n      - {{ type: {t}, vram: 0x{a:X}, name: {n} }}" for a, t, n in bss_subs)
    bss_size = bss_end - bss_subs[0][0]
    symbol_files = ", ".join(extra_symbol_files + [f"config/symbols_{name}.txt"])
    gp_line = f"\n  gp_value: 0x{gp:X}" if gp else ""
    asm_path = f"asm/{name}"
    return f"""# Generated by tools/gen_splat.py -- do not edit by hand.
name: {name}
sha1: {hashlib.sha1(image).hexdigest()}
options:
  basename: {name}
  target_path: build/orig/{name}.bin
  elf_path: build/{name}.elf
  base_path: ..
  platform: ps2
  compiler: MWCCPS2
  build_path: build
  asm_path: {asm_path}
  src_path: src
  asset_path: assets/{name}
  ld_script_path: build/{name}.ld
  symbol_addrs_path: [{symbol_files}]
  reloc_addrs_path: [config/relocs_{name}.txt]
  undefined_funcs_auto_path: build/{name}.undefined_funcs_auto.txt
  undefined_syms_auto_path: build/{name}.undefined_syms_auto.txt
  find_file_boundaries: False{gp_line}
  section_order: [".text", ".data", ".rodata", ".sdata", ".sbss", ".bss"]
  ld_bss_is_noload: True
  emit_subalign: False
  string_encoding: ASCII
  data_string_encoding: ASCII
  allow_data_addends: False
  generate_asm_macros_files: True
  make_full_disasm_for_code: True
  create_undefined_funcs_auto: True
  create_undefined_syms_auto: True

segments:
  - name: {name}
    type: code
    start: 0x0
    vram: 0x{vram:X}
    bss_size: 0x{bss_size:X}
    subsegments:
{sub}
  - [0x{len(image):X}]
"""


def c_units():
    """{target: {unit}} built from C instead of asm (config/c_units.txt: `<target> <unit>` per line)."""
    out = {}
    path = ROOT / "config" / "c_units.txt"
    if path.exists():
        for line in path.read_text(encoding="utf-8").splitlines():
            line = line.split("#")[0].strip()
            if line:
                target, unit = line.split()
                out.setdefault(target, set()).add(unit)
    return out


def main():
    elf = read_elf(ELF)
    in_c = c_units()
    dw = dwarf1.Dwarf1(ELF)
    sources = dwarf_sources(dw)

    overlays = [n for n in elf.sections if re.match(r"g[xyz]_", n)]
    on_disc = [n for n in overlays if (GX_DIR / f"{n[3:].upper()}.BIN").exists() and n.startswith("gx_")]

    named = [s for s in elf.syms if s.type in (1, 2) and
             ((s.section == "main" and MAIN_VRAM <= s.value < BSS_END) or
              (s.section in overlays and s.value >= OVERLAY_VRAM))]
    assign_names(named)
    by_section = {}
    for s in named:
        by_section.setdefault(s.section, []).append(s)
    seg = {s.name: s.value for s in elf.syms if s.type == 0 and re.match(r"_g[xyz]_\w+_(start|end)$", s.name)}

    (ROOT / "config").mkdir(exist_ok=True)
    write = lambda p, text: (ROOT / p).write_text(text, encoding="utf-8", newline="\n")

    # Symbols a target may name but doesn't contain: every overlay's symbols (main refers to
    # overlay-resident stage data by name) and the linker-defined globals (_end, _gp, ...).
    # configure.py turns these into PROVIDE() fallbacks.
    extern, seen_names = [], {s.out for s in named}
    for s in sorted((s for s in named if s.section != "main"), key=lambda s: (s.section, s.value)):
        extern.append(f"{s.out} = 0x{s.value:08X};")
    for s in elf.syms:
        if s.type == 0 and s.bind == 1 and s.value >= MAIN_VRAM and not s.name.startswith(".") \
                and sanitize(s.name) == s.name and s.name not in seen_names:
            seen_names.add(s.name)
            extern.append(f"{s.name} = 0x{s.value:08X};")
    write("config/symbols_extern.txt", "\n".join(extern) + "\n")

    def align_lines(target, entries, asmtu_names, bss_start):
        """Per assembled object section: the alignment that puts it back at its original address.

        entries: [(vram, splat type, name)]. The alignment is the largest power of two dividing
        the start address, so the linker lands on it even after an object without trailing
        padding (MWCC objects end at their last symbol). The bss output section has no fixed
        address and takes the largest input alignment, so bss alignments are capped at the
        alignment of the bss start.
        """
        bss_cap = bss_start & -bss_start
        sect = {"asmtu": ".text", "textbin": ".text", "data": ".data", "rodata": ".rodata",
                "sdata": ".sdata", "bss": ".bss"}
        sect.update({"." + k: v for k, v in list(sect.items())})
        out = []
        for vram, typ, name in entries:
            if name in asmtu_names:
                path = f"asm/{target}/{name}.s"
            elif typ == "textbin":
                path = f"asm/{target}/data/{name}.s"
            else:
                path = f"asm/{target}/data/{name}.{typ.lstrip('.')}.s"
            align = vram & -vram
            if sect[typ] == ".bss":
                align = min(align, bss_cap)
            out.append(f"{path} {sect[typ]} 0x{align:X}")
        return out

    # --- main ---
    off, size, _ = elf.sections["main"]
    image = elf.data[off:off + size]
    main_syms = by_section["main"]
    funcs = sorted((s.value, s.name) for s in main_syms if s.type == 2 and TEXT_START <= s.value < VUTEXT_START)
    lib_starts = {s.value for s in elf.syms if s.section == "main" and s.type == 3 and s.name == ".text"
                  and TEXT_START <= s.value < VUTEXT_START}
    units = text_units(funcs, lib_starts, sources, "lib/")
    rom = lambda v: v - MAIN_VRAM
    import datasplit  # imports this module; keep the import local
    ranges = datasplit.Splitter(elf, dw, units).run().ranges()
    asmtu_names = {"crt0"} | {u for _, u in units}
    # Sections owned by a code unit are dotted: splat folds them into that unit's .s file.
    kind = lambda sect, owner: ("." + sect) if owner in asmtu_names else sect
    entries = [(MAIN_VRAM, "asmtu", "crt0")] + [(a, "asmtu", u) for a, u in units] + [
        (VUTEXT_START, "textbin", "vutext")]
    for sect in ("data", "rodata", "sdata"):
        entries += [(a, kind(sect, owner), owner) for a, _, owner in ranges[sect]]
    ctype = lambda t, n: "c" if t == "asmtu" and n in in_c.get("main", ()) else t
    subs = [f"[0x{rom(a):X}, {ctype(t, n)}, {n}]" for a, t, n in entries]
    bss_subs = [(a, kind("bss", owner), owner) for a, _, owner in ranges["bss"]]
    write("config/main.yaml", yaml_text("main", image, MAIN_VRAM, bss_subs, BSS_END, subs, [], GP_VALUE))
    write("config/align_main.txt", "\n".join(align_lines("main", entries + bss_subs, asmtu_names, BSS_START)) + "\n")
    write("config/symbols_main.txt", "\n".join(symbol_lines(main_syms)) + "\n")
    rl, skipped = reloc_lines(elf, elf.relocs["main"], image, MAIN_VRAM, main_syms, [])
    write("config/relocs_main.txt", "\n".join(rl) + "\n")
    # Every relocated address, of any type (tools/diff_unit.py masks only these).
    write("config/relocated_main.txt", "".join(f"{off:08X}\n" for off, _ in sorted(elf.relocs["main"])))
    print(f"main: {len(units) + 1} units, {len(main_syms)} symbols, {len(rl)} relocs ({skipped} skipped)")

    # --- overlays on disc ---
    for ov in on_disc:
        image = (GX_DIR / f"{ov[3:].upper()}.BIN").read_bytes()
        own = by_section.get(ov, [])
        text_start, data_start = seg[f"_{ov}_text_start"], seg[f"_{ov}_data_start"]
        bss_start, bss_end = seg[f"_{ov}_bss_start"], seg[f"_{ov}_bss_end"]
        if seg[f"_{ov}_data_end"] != OVERLAY_VRAM + len(image):
            raise SystemExit(f"{ov}: BIN size does not end at _data_end")
        funcs = sorted((s.value, s.name) for s in own if s.type == 2 and text_start <= s.value < data_start)
        units = text_units(funcs, set(), sources, f"{ov}_misc/")
        r = lambda v: v - OVERLAY_VRAM
        if len(units) != 1:
            raise SystemExit(f"{ov}: expected one source file, got {len(units)}")
        unit = units[0][1]
        entries = [(OVERLAY_VRAM, "textbin", "header"), (units[0][0], "asmtu", unit), (data_start, ".data", unit)]
        if ov in OVERLAY_RODATA:
            entries.append((OVERLAY_VRAM + OVERLAY_RODATA[ov], ".rodata", unit))
        subs = [f"[0x{r(a):X}, {'c' if t == 'asmtu' and n in in_c.get(ov, ()) else t}, {n}]" for a, t, n in entries]
        bss_subs = [(bss_start, ".bss", unit)]
        write(f"config/{ov}.yaml", yaml_text(ov, image, OVERLAY_VRAM, bss_subs, bss_end, subs,
                                             ["config/symbols_main.txt"], GP_VALUE))
        write(f"config/align_{ov}.txt", "\n".join(align_lines(ov, entries + bss_subs, {unit}, bss_start)) + "\n")
        write(f"config/symbols_{ov}.txt", "\n".join(symbol_lines(own)) + "\n")
        rl, skipped = reloc_lines(elf, elf.relocs[ov], image, OVERLAY_VRAM, own, main_syms)
        write(f"config/relocs_{ov}.txt", "\n".join(rl) + "\n")
        write(f"config/relocated_{ov}.txt", "".join(f"{off:08X}\n" for off, _ in sorted(elf.relocs[ov])))
        print(f"{ov}: {len(units)} units, {len(own)} symbols, {len(rl)} relocs ({skipped} skipped)")


if __name__ == "__main__":
    main()
