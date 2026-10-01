#!/usr/bin/env python3
"""Shiftability test: move every game function, relink, check, and optionally make a disc image.

Inserts SHIFT bytes of padding in main's .text right after crt0, so every game function and
everything referring to one moves. main is relinked, then every overlay is relinked against the
shifted main's symbols. The checks:

- outside the moved code, main and the overlays may differ from the original only at words the
  original relocates (a hard-coded address would show up as an unexpected difference);
- every relocated word that referred to moved code now refers to the moved address.

With --iso, the shifted executable and overlays are written into a copy of the disc image (files
keep their size and place on disc), ready to boot in an emulator.

    .venv/bin/python tools/shift_test.py [--shift 0x40] [--iso original.iso]

Run after `ninja` (it reuses the build's objects and linker scripts). Output goes to build/shift/.
The shift must fit the slack before .vutext's alignment (0x58 bytes), so .data, .bss and the
overlay area stay put: the overlays' entry tables are at fixed addresses (docs/toolchain.md).
"""
import argparse
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import dwarf1  # noqa: E402
import gen_splat  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import configure  # noqa: E402
LD = configure.BINUTILS + "ld"
OBJCOPY = configure.BINUTILS + "objcopy"
ANCHOR = "        build/asm/main/crt0.s.o(.text);\n"
MAIN_VRAM, OVL_VRAM = 0x100000, 0x1F01E00
ELF_MAIN_OFFSET, MAIN_SIZE = 0x700, 0x2C7D80


def run(*cmd):
    subprocess.run(cmd, cwd=ROOT, check=True)


def link(target, script, out, extra=()):
    run(LD, "-EL", "-T", script, "-T", f"build/{target}.undefined_syms_auto.txt",
        "-T", f"build/{target}.undefined_funcs_auto.txt", "-T", f"build/{target}.provide.ld", *extra,
        "-Map", str(out.with_suffix(".map")), "--no-check-sections", "-o", str(out))
    run(OBJCOPY, "-O", "binary", str(out), str(out.with_suffix(".bin")))
    return out.with_suffix(".bin").read_bytes()


def relocated(target):
    return {int(x, 16) for x in (ROOT / f"config/relocated_{target}.txt").read_text().split()}


def compare(name, new, orig, vram, real, moved):
    """Words that differ: (expected, unexpected). `moved(addr)` maps an original address of a
    word to its address in the new image (identity outside the moved code)."""
    expected = unexpected = 0
    for off in range(0, len(orig) - 3, 4):
        addr = vram + off
        new_addr = moved(addr)
        if new_addr is None:
            continue
        a = struct.unpack_from("<I", orig, off)[0]
        b = struct.unpack_from("<I", new, new_addr - vram)[0]
        if a != b:
            if addr in real:
                expected += 1
            else:
                unexpected += 1
                if unexpected <= 5:
                    print(f"  {name}: unexpected change at 0x{addr:08X}: {a:08x} -> {b:08x}")
    return expected, unexpected


def iso_files(iso, want):
    """{name: (byte offset, size)} of files in an ISO9660 image, for paths like 'GX/TOI.BIN'."""
    def read(lba, n):
        iso.seek(lba * 2048)
        return iso.read(n)

    pvd = read(16, 2048)
    assert pvd[1:6] == b"CD001", "not an ISO9660 image"
    out = {}

    def walk(lba, size, prefix):
        data = read(lba, size)
        pos = 0
        while pos < len(data):
            n = data[pos]
            if n == 0:
                pos = (pos // 2048 + 1) * 2048
                continue
            ext, length = struct.unpack_from("<I", data, pos + 2)[0], struct.unpack_from("<I", data, pos + 10)[0]
            flags, nlen = data[pos + 25], data[pos + 32]
            name = data[pos + 33:pos + 33 + nlen].decode("latin1").split(";")[0]
            if name not in ("\0", "\1"):
                path = prefix + name
                if flags & 2:
                    walk(ext, length, path + "/")
                elif path in want:
                    out[path] = (ext * 2048, length)
            pos += n

    root = pvd[156:190]
    walk(struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0], "")
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--shift", type=lambda x: int(x, 0), default=0x40)
    ap.add_argument("--iso", help="original disc image to copy and patch")
    args = ap.parse_args()
    out = ROOT / "build/shift"
    out.mkdir(parents=True, exist_ok=True)
    targets = sorted(p.stem for p in (ROOT / "config").glob("gx_*.yaml"))

    text = (ROOT / "build/main.ld").read_text(encoding="utf-8")
    assert text.count(ANCHOR) == 1
    (out / "main.ld").write_text(text.replace(ANCHOR, ANCHOR + f"        . += 0x{args.shift:X}; /* shift test */\n"),
                                 encoding="utf-8", newline="\n")
    main_bin = link("main", "build/shift/main.ld", out / "main.elf")
    run(sys.executable, "tools/export_syms.py", "build/shift/main.elf", "build/shift/main_syms.ld")
    orig_main = (ROOT / "build/orig/main.bin").read_bytes()
    if len(main_bin) != len(orig_main):
        sys.exit(f"main: size 0x{len(main_bin):X}, original 0x{len(orig_main):X} (shift too big?)")

    # The moved range: from the first object after crt0 to the end of the game's .text.
    start = next(int(l.split()[1], 16) for l in (ROOT / "build/main.map").read_text().splitlines()
                 if l.strip().startswith(".text") and "crt0" not in l and len(l.split()) >= 4
                 and int(l.split()[1], 16) > MAIN_VRAM)
    vutext = next(int(l.split()[1], 16) for l in (ROOT / "build/main.map").read_text().splitlines()
                  if "vutext.s.o" in l and l.strip().startswith(".text"))
    moved = lambda a: a + args.shift if start <= a < vutext - args.shift else (None if a >= vutext - args.shift and a < vutext else a)
    exp, unexp = compare("main", main_bin, orig_main, MAIN_VRAM, relocated("main"), moved)
    print(f"main: code from 0x{start:08X} moved by 0x{args.shift:X}; {exp} relocated words changed, "
          f"{unexp} unexpected changes")
    bad = unexp
    # Every relocation of the original (calls, pointers, lui/lo pairs), from its own relocation
    # table: each must now name its target's new address. VU relocation types are skipped (they
    # address VU memory, which doesn't move).
    in_moved = lambda a: start <= a < vutext - args.shift
    word = lambda img, a: struct.unpack_from("<I", img, a - MAIN_VRAM)[0]
    sext = lambda x: x - 0x10000 if x & 0x8000 else x
    elf = gen_splat.read_elf(dwarf1.DEFAULT_ELF)
    rel = {off: (info & 0xFF, info >> 8) for off, info in elf.relocs["main"]}

    def resolve(img, addr, at, typ, symi, where):
        """Target of the relocated field at original address `addr` (read at `where(addr)`)."""
        w = word(img, where(addr))
        if typ == 2:
            return w
        if typ == 4:
            return ((where(addr) + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
        other = 6 if typ == 5 else 5
        near = [o for o in range(addr - 0x800, addr + 0x800, 4)
                if rel.get(o, (None, None)) == (other, symi) and where(o) is not None]
        if not near:
            return None
        o = min(near, key=lambda x: abs(x - addr))
        hi, lo = (w, word(img, where(o))) if typ == 5 else (word(img, where(o)), w)
        return ((hi & 0xFFFF) << 16) + sext(lo & 0xFFFF)

    checked = wrong = 0
    for addr, (typ, symi) in sorted(rel.items()):
        if typ not in (2, 4, 5, 6) or moved(addr) is None:
            continue
        target = resolve(orig_main, addr, addr, typ, symi, lambda a: a)
        got = resolve(main_bin, addr, moved(addr), typ, symi, moved)
        if target is None:
            continue
        # Whether the target moved follows the relocation's symbol (Sd3dData - 0x138800 is data
        # indexed from far below, not code); section symbols go by the target itself.
        sym = elf.by_index.get(symi)
        base = sym.value if sym is not None and sym.type in (1, 2) else target
        want = target + args.shift if in_moved(base) else target
        checked += 1
        if got != want:
            wrong += 1
            if wrong <= 5:
                print(f"  main: relocation type {typ} at 0x{addr:08X} names 0x{got:08X}, expected 0x{want:08X}")
    print(f"main: {checked} relocated fields checked (calls, pointers, lui/lo pairs), {wrong} wrong")
    bad += wrong
    # Every relocation of the original that falls in a linked asm object must be in that object
    # (tools/resymbolize.py restores the ones spimdisasm dropped).
    import diff_unit
    lost = 0
    for line in (ROOT / "build/main.map").read_text(encoding="utf-8").splitlines():
        p = line.split()
        if len(p) < 4 or p[0] not in (".text", ".data", ".rodata", ".sdata") or not p[3].startswith("build/asm/"):
            continue
        a, size = int(p[1], 16), int(p[2], 16)
        if not a or not size:
            continue
        obj = diff_unit.Obj(ROOT / p[3])
        have = {a + off for i, n in enumerate(obj.names) if n == p[0] for off in obj.masks.get(i, {})}
        for x in range(a, a + size, 4):
            if x in rel and rel[x][0] in (2, 4, 5, 6, 7) and x not in have:
                lost += 1
                if lost <= 5:
                    print(f"  main: 0x{x:08X} is relocated in the original but not in {p[3]}")
    print(f"main: {lost} of the original's relocations missing from linked asm objects")
    bad += lost
    bins = {}
    for t in targets:
        ovl = link(t, f"build/{t}.ld", out / f"{t}.elf", ("-T", "build/shift/main_syms.ld"))
        orig = (ROOT / f"build/orig/{t}.bin").read_bytes()
        exp, unexp = compare(t, ovl, orig, OVL_VRAM, relocated(t), lambda a: a)
        print(f"{t}: {exp} relocated words changed, {unexp} unexpected changes")
        bad += unexp + (len(ovl) != len(orig))
        bins[t] = ovl
    if bad:
        sys.exit(f"FAIL: {bad} unexpected differences")
    print("PASS: every change is at a relocated word")

    if args.iso:
        elf = bytearray((ROOT / "baserom/disc/SLUS_202.28").read_bytes())
        elf[ELF_MAIN_OFFSET:ELF_MAIN_OFFSET + MAIN_SIZE] = main_bin
        files = {"SLUS_202.28": bytes(elf)}
        for t in targets:
            files[f"GX/{t[3:].upper()}.BIN"] = bins[t]
        iso_out = out / "sh2_shift.iso"
        print(f"copying {args.iso} -> {iso_out}")
        shutil.copyfile(args.iso, iso_out)
        with open(iso_out, "r+b") as iso:
            where = iso_files(iso, set(files))
            missing = set(files) - set(where)
            if missing:
                sys.exit(f"not found on the disc: {sorted(missing)}")
            for path, data in files.items():
                off, size = where[path]
                if size != len(data):
                    sys.exit(f"{path}: {len(data)} bytes, the disc has {size}")
                iso.seek(off)
                old = iso.read(size)
                iso.seek(off)
                iso.write(data)
                print(f"  {path}: {sum(a != b for a, b in zip(old, data))} bytes changed")
        print(f"wrote {iso_out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
