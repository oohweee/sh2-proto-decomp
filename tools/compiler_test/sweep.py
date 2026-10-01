#!/usr/bin/env python3
"""Compile test.c with each candidate MWCC build and flag set; compare each function to the target.

Run under WSL from the repo root (the builds other than the build's own compiler come from
`tools/download_tools.py --sweep`):
    .venv/bin/python tools/compiler_test/sweep.py [source.c] [--opts -O1,-O2] [--show]
--show prints a side-by-side disassembly of every mismatching function.
Relocated fields (HI16/LO16 immediates, 26-bit jump targets) are masked on both sides.
"""
import argparse
import itertools
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TARGET = ROOT / "baserom" / "disc" / "SLUS_202.28"
WIBO = ROOT / "tools" / "wibo"
COMPILERS = ["mwcps2-2.3.3-000906", "mwcps2-2.4-001213", "mwcps2-3.0-011126"]
OPT_LEVELS = ["-O0", "-O1", "-O2", "-O3", "-O4", "-O2,p", "-O3,p", "-O4,p", "-O4,s"]
HERE = Path(__file__).resolve().parent
BASE_FLAGS = ["-sdatathreshold", "0"]

R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16 = 4, 5, 6


def elf_sections(data):
    (_, _, _, _, _, _, shoff, _, _, _, _, shentsize, shnum, shstrndx) = \
        struct.unpack_from("<16sHHIIIIIHHHHHH", data, 0)
    secs = [struct.unpack_from("<10I", data, shoff + i * shentsize) for i in range(shnum)]
    st = secs[shstrndx][4]
    names = [data[st + s[0]:data.index(b"\0", st + s[0])].decode() for s in secs]
    return secs, names


def elf_funcs(data):
    """{name: (section_index, value, size)} for FUNC symbols."""
    secs, _ = elf_sections(data)
    out = {}
    for s in secs:
        if s[1] != 2:  # SHT_SYMTAB
            continue
        strs = secs[s[6]][4]
        for i in range(s[5] // 16):
            n, v, sz, info, _, shndx = struct.unpack_from("<IIIBBH", data, s[4] + i * 16)
            if info & 0xF == 2:
                out[data[strs + n:data.index(b"\0", strs + n)].decode()] = (shndx, v, sz)
    return out


def target_words(data, funcs, name):
    secs, names = elf_sections(data)
    shndx, addr, size = funcs[name]
    sec = secs[names.index("main")]
    off = sec[4] + addr - sec[3]
    return list(struct.unpack_from(f"<{size // 4}I", data, off))


def object_words(data, funcs, name):
    """Instruction words for `name` in a relocatable object, plus {word_index: mask} for relocs."""
    secs, names = elf_sections(data)
    shndx, value, size = funcs[name]
    sec = secs[shndx]
    words = list(struct.unpack_from(f"<{size // 4}I", data, sec[4] + value))
    masks = {}
    for i, s in enumerate(secs):
        if s[1] == 9 and s[7] == shndx:  # SHT_REL targeting this section
            for j in range(s[5] // 8):
                r_off, r_info = struct.unpack_from("<II", data, s[4] + j * 8)
                if value <= r_off < value + size:
                    typ = r_info & 0xFF
                    masks[(r_off - value) // 4] = 0x03FFFFFF if typ == R_MIPS_26 else 0xFFFF
    return words, masks


def compile_once(cc, flags, source, obj):
    cmd = [str(WIBO), str(ROOT / "tools" / "mwcc" / cc / "mwccps2.exe"), "-c", *flags,
           "-nostdinc", "-stderr", str(source), "-o", str(obj)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0 or not obj.exists():
        return r.stdout + r.stderr
    return None


def show(name, ow, tw, masks, vram):
    import rabbitizer
    rabbitizer.config.regNames_gprAbiNames = rabbitizer.Abi.O32
    print(f"    --- {name}: ours | target")
    for i in range(max(len(ow), len(tw))):
        a = rabbitizer.Instruction(ow[i], vram + i * 4).disassemble() if i < len(ow) else ""
        b = rabbitizer.Instruction(tw[i], vram + i * 4).disassemble() if i < len(tw) else ""
        m = masks.get(i, 0)
        same = i < len(ow) and i < len(tw) and (ow[i] & ~m) == (tw[i] & ~m)
        print(f"    {'  ' if same else '>>'} {a:40} | {b}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source", nargs="?", default=str(HERE / "test.c"))
    ap.add_argument("--opts", default=",".join(OPT_LEVELS).replace(",p", ":p").replace(",s", ":s"),
                    help="comma-separated opt flags; write -O2,p as -O2:p")
    ap.add_argument("--cc", default=",".join(COMPILERS))
    ap.add_argument("--show", action="store_true")
    args = ap.parse_args()
    opts = [o.replace(":", ",") for o in args.opts.split(",")]
    target = TARGET.read_bytes()
    tfuncs = elf_funcs(target)
    results = {}
    with tempfile.TemporaryDirectory() as tmp:
        for cc, opt in itertools.product(args.cc.split(","), opts):
            obj = Path(tmp) / f"{cc}{opt.replace(',', '_')}.o"
            err = compile_once(cc, [opt, *BASE_FLAGS], args.source, obj)
            if err:
                print(f"{cc:22} {opt:7} COMPILE FAILED: {err.strip().splitlines()[-1] if err.strip() else '?'}")
                continue
            odata = obj.read_bytes()
            ofuncs = elf_funcs(odata)
            row = []
            for name in ofuncs:
                if name not in tfuncs:
                    continue
                ow, masks = object_words(odata, ofuncs, name)
                tw = target_words(target, tfuncs, name)
                ok = len(ow) == len(tw) and all(
                    (a & ~masks.get(i, 0)) == (b & ~masks.get(i, 0)) for i, (a, b) in enumerate(zip(ow, tw)))
                row.append((name, ok, len(ow), len(tw)))
                if args.show and not ok:
                    show(name, ow, tw, masks, tfuncs[name][1])
                results.setdefault(name, {})[(cc, opt)] = ok
            n_ok = sum(ok for _, ok, _, _ in row)
            detail = " ".join(f"{n}{'' if ok else f'({o}/{t}w)'}{'✓' if ok else '✗'}" for n, ok, o, t in row)
            print(f"{cc:22} {opt:7} {n_ok}/{len(row)}  {detail}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
