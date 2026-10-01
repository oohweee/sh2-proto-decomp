#!/usr/bin/env python3
"""Post-process an MWCC object: give every data section at least 8-byte alignment.

The original compiler (MW MIPS C 2.4.1.01) starts every data object on an 8-byte boundary,
and every data object of 16 bytes or more on a 16-byte boundary: in the prototype, all 3,147
game .data objects, all 433 game .bss objects and all 702 game .rodata objects are 8-aligned,
and all objects >= 16 bytes are 16-aligned (the only exceptions are GCC-built Sony objects). The public 2.4 build 0017 we compile with gives 4-byte
variables 4-byte alignment, and neither a compiler option nor the Metrowerks linker changes
that (tested). This models the 2.4.1.01 behavior by raising sh_addralign of each .data,
.rodata, .sdata, .bss and .sbss section to at least 8, or 16 for sections of 16 bytes or more,
in place. Larger alignments are kept.

It also models the Metrowerks linker's dead-stripping. A function the original linker removed
(nothing called it) left no code behind, but its string literals stayed in .rodata, where the
compiler emitted them. Such functions are written in the C source and listed in
config/stripped_functions.txt; their .text section (and its relocations) is emptied here, so
GNU ld places everything else exactly as the original linker did.

    mwcc_fixup.py file.o [...]
"""
import struct
import sys
from pathlib import Path

DATA_SECTIONS = {b".data", b".rodata", b".sdata", b".bss", b".sbss"}
MIN_ALIGN = 8
BIG_ALIGN, BIG_SIZE = 16, 16
STRIPPED_LIST = Path(__file__).resolve().parent.parent / "config/stripped_functions.txt"


def stripped_functions():
    """Names of functions the original linker dead-stripped (config/stripped_functions.txt)."""
    if not STRIPPED_LIST.exists():
        return set()
    return {line.split("#")[0].strip() for line in STRIPPED_LIST.read_text(encoding="utf-8").splitlines()} - {""}


def listed(name, names):
    """Whether `name` is in the stripped list; an entry ending in `*` is a prefix."""
    return name in names or any(n.endswith("*") and name.startswith(n[:-1]) for n in names)


def strip(data, shoff, shentsize, shnum, names):
    """Dead-strip the functions in `names`, like the Metrowerks linker: empty their .text
    sections, then every data section that only they referenced (their own string literals;
    literals pooled with a surviving function stay). Returns the number of sections emptied."""
    secs = [struct.unpack_from("<10I", data, shoff + i * shentsize) for i in range(shnum)]
    symtab = next((s for s in secs if s[1] == 2), None)  # SHT_SYMTAB
    if symtab is None:
        return 0
    strbase = secs[symtab[6]][4]
    syms = []  # (name, shndx, bind, file offset)
    for k in range(symtab[5] // 16):
        so = symtab[4] + k * 16
        name_off, _, _, info, _, shndx = struct.unpack_from("<IIIBBH", data, so)
        name = bytes(data[strbase + name_off:data.index(b"\0", strbase + name_off)]).decode()
        syms.append((name, shndx, info, so))
    dead = {shndx for name, shndx, info, _ in syms
            if info & 0xF == 2 and 0 < shndx < shnum and listed(name, names)}  # STT_FUNC
    if not dead:
        return 0
    # refs[s]: sections that section s's relocations point at.
    refs, rel_of = {}, {}
    for i, s in enumerate(secs):
        if s[1] in (4, 9):  # SHT_RELA/SHT_REL
            rel_of.setdefault(s[7], []).append(i)
            size = 12 if s[1] == 4 else 8
            for k in range(s[5] // size):
                symi = struct.unpack_from("<I", data, s[4] + k * size + 4)[0] >> 8
                if symi < len(syms):
                    refs.setdefault(s[7], set()).add(syms[symi][1])
    has_global = {shndx for _, shndx, info, _ in syms if info >> 4 == 1}
    # SHF_ALLOC: non-alloc sections (.mwcats) keep nothing alive. .bss (NOBITS) is never stripped:
    # a stripped function's zero-initializer template survives in the original (Chacter/anime).
    alloc = {i for i, s in enumerate(secs) if s[2] & 2 and s[1] != 8}
    candidates = set().union(*(refs.get(d, set()) for d in dead)) - has_global
    changed = True
    while changed:
        live_refs = set().union(*(refs.get(i, set()) for i in alloc if i not in dead))
        new = {c for c in candidates - dead if c in alloc and c not in live_refs}
        changed = bool(new)
        dead |= new
        candidates |= set().union(*(refs.get(d, set()) for d in new)) - has_global
    for shndx in dead:
        struct.pack_into("<I", data, shoff + shndx * shentsize + 20, 0)  # sh_size
        struct.pack_into("<I", data, shoff + shndx * shentsize + 32, 1)  # sh_addralign
        for i in rel_of.get(shndx, []):
            struct.pack_into("<I", data, shoff + i * shentsize + 20, 0)
    for _, shndx, _, so in syms:
        if shndx in dead:
            struct.pack_into("<I", data, so + 8, 0)  # st_size
    return len(dead)


def fixup(path):
    data = bytearray(Path(path).read_bytes())
    (_, _, _, _, _, _, shoff, _, _, _, _, shentsize, shnum, shstrndx) = \
        struct.unpack_from("<16sHHIIIIIHHHHHH", data, 0)
    strtab = struct.unpack_from("<10I", data, shoff + shstrndx * shentsize)[4]
    changed = 0
    for i in range(shnum):
        off = shoff + i * shentsize
        sh = struct.unpack_from("<10I", data, off)
        name = bytes(data[strtab + sh[0]:data.index(b"\0", strtab + sh[0])])
        if name in DATA_SECTIONS:
            want = BIG_ALIGN if sh[5] >= BIG_SIZE else MIN_ALIGN  # sh_size
            if sh[8] < want:
                struct.pack_into("<I", data, off + 32, want)  # sh_addralign
                changed += 1
    names = stripped_functions()
    if names:
        changed += strip(data, shoff, shentsize, shnum, names)
    if changed:
        Path(path).write_bytes(data)
    return changed


def main():
    for p in sys.argv[1:]:
        fixup(p)
    return 0


if __name__ == "__main__":
    sys.exit(main())
