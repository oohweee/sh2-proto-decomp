#!/usr/bin/env python3
"""DWARF 1 line tables (.line): the source line of every statement, for the original and for us.

The prototype's ELF keeps a .line table per function: for each statement, its source line (and
column) and the offset of its first instruction. Our MWCC writes the same table with `-g`, which
doesn't change the code (docs/toolchain.md). Comparing the two shows where our source splits or
joins statements differently from the original, even in functions whose code matches, and gives
the original line number of every statement (what asserts bake in via __LINE__).

    dwarf_lines.py orig <function> [--file <substring>]   the original's table for a function
    dwarf_lines.py obj <object.o> [<function>]            the table of a -g object
    dwarf_lines.py layout <unit> <function> [--src F]     side by side: original line, our line and
                                                          source text, per statement (compiles with -g;
                                                          run in the build environment)

Entries are (line, column, offset). Column 0xFFFF means "no column". The last entry of a function
is its closing brace.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from dwarf1 import DEFAULT_ELF, TAG_compile_unit, TAG_global_subroutine, TAG_subroutine, Dwarf1  # noqa: E402
from elfobj import ElfObj  # noqa: E402

AT_stmt_list = 0x0106
AT_name = 0x0038


def parse_block(buf, off):
    """(base address, [(line, column, offset), ...]) of the .line block at off."""
    length, base = struct.unpack_from("<II", buf, off)
    entries = []
    p = off + 8
    while p + 10 <= off + length:
        entries.append(struct.unpack_from("<IHI", buf, p))
        p += 10
    return base, entries


def original(elf=DEFAULT_ELF):
    """[(source file, function, low_pc, entries)] for every function of the original, via each
    compile unit's AT_stmt_list (unambiguous also for overlays, whose addresses overlap)."""
    obj = ElfObj(elf)
    line = next(s for s in obj.sections if s.name == ".line")
    buf = bytes(line.data)
    dw = Dwarf1(elf)
    out = []
    for cu in dw.roots:
        if cu.tag != TAG_compile_unit or AT_stmt_list not in cu.attrs:
            continue
        funcs = [c for c in cu.children if c.tag in (TAG_global_subroutine, TAG_subroutine)]
        if not funcs:
            continue
        _, entries = parse_block(buf, cu.attrs[AT_stmt_list])
        src = cu.attrs.get(AT_name, "").replace("\\", "/")
        out.append((src, funcs[0].name, funcs[0].attrs.get(0x0111), entries))
    return out


def find_original(func, file_hint=None, table=None):
    """The original's entries for a function; file_hint (e.g. the unit path) picks between statics
    of the same name."""
    rows = [r for r in (table or original()) if r[1] == func]
    if file_hint:
        hint = file_hint.split(":")[-1].replace("\\", "/")
        narrowed = [r for r in rows if r[0].endswith(hint + ".c") or hint in r[0]]
        rows = narrowed or rows
    return rows[0][3] if rows else None


def from_object(path):
    """{function: entries} of a relocatable object compiled with -g (each .line section's base
    address is relocated against its function's symbol)."""
    obj = ElfObj(path)
    out = {}
    for i, s in enumerate(obj.sections):
        if s.name != ".line":
            continue
        rel = [r for r in obj.relocs.get(i, []) if r[0] == 4]
        if not rel:
            continue
        _, entries = parse_block(bytes(s.data), 0)
        out[rel[0][2].name] = entries
    return out


def compare(ours, orig):
    """Compare two functions' line tables (without the closing-brace entry).

    Returns a dict:
      same       - identical layout relative to the first line
      structure  - [(offset, where, rel_line)]: statement starts only one side has
                   ('orig' = the original starts a statement there, we don't; 'ours' = the reverse)
      spacing    - [(offset, ours_rel, orig_rel)]: same statement start, different line distance
      first      - (our first line, the original's first line)
    """
    def body(entries):
        rows = [(ln, off) for ln, _, off in entries if ln]
        return rows[:-1] if len(rows) > 1 else rows  # the last row is the closing brace

    a, b = body(ours), body(orig)
    if not a or not b:
        return {"same": False, "structure": [], "spacing": [], "first": (None, None)}
    a0, b0 = a[0][0], b[0][0]
    ra = {}
    for ln, off in a:
        ra.setdefault(off, ln - a0)
    rb = {}
    for ln, off in b:
        rb.setdefault(off, ln - b0)
    structure = [(off, "orig", rb[off]) for off in sorted(set(rb) - set(ra))]
    structure += [(off, "ours", ra[off]) for off in sorted(set(ra) - set(rb))]
    spacing = [(off, ra[off], rb[off]) for off in sorted(set(ra) & set(rb)) if ra[off] != rb[off]]
    return {"same": not structure and not spacing, "structure": sorted(structure), "spacing": spacing,
            "first": (a0, b0)}


def logical_lines(text):
    """{logical line number: physical line index} for a C file, honouring `#line N`."""
    out, cur = {}, 1
    for i, line in enumerate(text.split("\n")):
        s = line.strip()
        if s.startswith("#line "):
            cur = int(s.split()[1])
            continue
        out[cur] = i
        cur += 1
    return out


def layout(unit, func, src=None):
    """Lines of text: for each statement start of `func`, the original's line, ours, our source."""
    import subprocess
    import tempfile
    root = Path(__file__).resolve().parent.parent
    sys.path.insert(0, str(root))
    import configure
    cunit = unit.split(":")[-1]
    src = Path(src) if src else root / "src" / f"{cunit}.c"
    with tempfile.TemporaryDirectory() as d:
        obj = Path(d) / "g.o"
        subprocess.run([str(root / configure.WIBO), str(root / configure.MWCC), "-c", "-g",
                        *configure.unit_cflags(cunit), "-nostdinc", "-Iinclude", "-stderr", str(src),
                        "-o", str(obj)], cwd=root, check=True, capture_output=True)
        mine = from_object(obj).get(func)
    orig = find_original(func, cunit)
    if mine is None or orig is None:
        return [f"{func}: no line table ({'ours' if mine is None else 'original'})"]
    text = src.read_text(encoding="utf-8").split("\n")
    where = logical_lines("\n".join(text))
    ours_at = {}
    for ln, _, off in mine:
        if ln:
            ours_at.setdefault(off, ln)
    orig_at = {}
    for ln, _, off in orig:
        if ln:
            orig_at.setdefault(off, ln)
    out = [f"{'offset':>7} {'orig':>5} {'ours':>5}  source (ours)"]
    for off in sorted(set(ours_at) | set(orig_at)):
        o, m = orig_at.get(off), ours_at.get(off)
        code = text[where[m]].strip() if m in where else ""
        mark = "" if (o is None) == (m is None) else "   <-- only " + ("original" if m is None else "ours")
        out.append(f"{off:#7x} {o if o else '':>5} {m if m else '':>5}  {code}{mark}")
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=["orig", "obj", "layout"])
    ap.add_argument("arg")
    ap.add_argument("func", nargs="?")
    ap.add_argument("--file")
    ap.add_argument("--src")
    args = ap.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")
    if args.cmd == "layout":
        print("\n".join(layout(args.arg, args.func, args.src)))
        return
    if args.cmd == "orig":
        rows = [r for r in original() if r[1] == args.arg and (not args.file or args.file in r[0])]
        for src, name, lo, entries in rows:
            print(f"{name} ({src}) at 0x{lo or 0:08X}:")
            for ln, col, off in entries:
                print(f"  line {ln:5d}  col {col if col != 0xFFFF else '-':>4}  +0x{off:x}")
    else:
        for name, entries in from_object(args.arg).items():
            if args.func and name != args.func:
                continue
            print(f"{name}:")
            for ln, col, off in entries:
                print(f"  line {ln:5d}  col {col if col != 0xFFFF else '-':>4}  +0x{off:x}")


if __name__ == "__main__":
    main()
