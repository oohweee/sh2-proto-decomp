#!/usr/bin/env python3
"""Compare the debug info of our functions with the original's: how faithful matched C is beyond
its bytes.

Every C unit is compiled with `-g` (same code, see docs/toolchain.md) and, per function that also
exists in the original, its locals and parameters are compared with the original's DWARF:

- locals:    locals the original declares that ours doesn't, or the reverse (a missing local
             usually means the original computed something through a variable we inline);
- order:     the same locals in a different declaration order (the DWARF lists them in reverse);
- types:     a different declared type (anonymous `@anonN` types aren't compared);
- registers: a different register where both sides give a real one. Stack slots aren't compared.
- location:  one side gives `v0` and the other a real register. MWCC writes `v0` for a variable
             that has no register of its own even when the code keeps its value in one: a local
             that only copies another value (typically an inline function's result), or one
             whose stores are dead or constant.
             The code can be identical either way, so these are hints about how the value was
             obtained, not errors; they are counted as their own kind.

    python tools/dwarf_compare.py                # writes docs/dwarf-fidelity.md, prints a summary
    python tools/dwarf_compare.py Font/font      # one unit, details on stdout
    UNIT_SRC=v.c python tools/dwarf_compare.py Font/font   # the same for a variant of the unit

Differences don't change the bytes; they are where the C still differs from the original's
source, and the evidence to fix it (see STYLE.md, "Evidence decides").
"""
import os
import re
import struct
import subprocess
import sys
import tempfile
from collections import Counter
from multiprocessing import Pool
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT))
import dwarf1  # noqa: E402
from dwarf1 import (AT_low_pc, TAG_compile_unit, TAG_formal_parameter, TAG_global_subroutine,  # noqa: E402
                    TAG_lexical_block, TAG_local_variable, TAG_subroutine, Dwarf1)
from elfobj import ElfObj  # noqa: E402

AT_name = 0x0038


class ObjDwarf(Dwarf1):
    """Dwarf1 over one of our relocatable objects: MWCC writes a .debug section per function and
    relocates its addresses, so the sections are joined and the R_MIPS_32 relocations applied."""

    def __init__(self, path):
        obj = ElfObj(path)
        base, parts, pos = {}, [], 0
        for i, s in enumerate(obj.sections):
            if s.name == ".debug":
                base[i] = pos
                parts.append((i, bytearray(s.data)))
                pos += len(s.data)
        for i, buf in parts:
            for off, typ, sym in obj.relocs.get(i, []):
                if typ == 2:
                    v = struct.unpack_from("<I", buf, off)[0]
                    struct.pack_into("<I", buf, off, (v + sym.value + base.get(sym.shndx, 0)) & 0xFFFFFFFF)
        self.buf = b"".join(bytes(b) for _, b in parts)
        self.dies, self.roots, self.name_override = {}, [], {}
        self._parse()


def variables(dw, f):
    """[(kind, name, declaration, location)] of a function, locals of inner blocks included, in
    DWARF order."""
    out = []

    def rec(d):
        for c in d.children:
            if c.tag == TAG_lexical_block:
                rec(c)
            elif c.tag in (TAG_local_variable, TAG_formal_parameter):
                try:
                    decl = dw.decl(dw.type_of(c), c.name or "")
                except Exception:  # noqa: BLE001
                    decl = c.name or ""
                kind = "param" if c.tag == TAG_formal_parameter else "local"
                out.append((kind, c.name or "", decl, dw.location(c) or ""))
    rec(f)
    return out


def is_reg(loc):
    return bool(loc) and "(sp)" not in loc and not loc.startswith("0x") and loc != "v0"


def compare(orig, ours):
    """List of difference strings between two variables() lists."""
    diffs = []
    on = [(k, n) for k, n, _, _ in orig]
    un = [(k, n) for k, n, _, _ in ours]
    oc, uc = Counter(on), Counter(un)
    missing = sorted(n for (k, n) in (oc - uc).elements())
    extra = sorted(n for (k, n) in (uc - oc).elements())
    if missing:
        diffs.append("missing " + ", ".join(missing))
    if extra:
        diffs.append("extra " + ", ".join(extra))
    if not missing and not extra and on != un:
        diffs.append("order")
    ot = {(k, n): (d, l) for k, n, d, l in orig}
    ut = {(k, n): (d, l) for k, n, d, l in ours}
    for key in [k for k in ot if k in ut]:  # the original's order, so the report is deterministic
        (od, ol), (ud, ul) = ot[key], ut[key]
        if "@anon" not in od and od.replace(" ", "") != ud.replace(" ", ""):
            diffs.append(f"type {key[1]}: `{od}` vs `{ud}`")
        if is_reg(ol) and is_reg(ul) and ol != ul:
            diffs.append(f"register {key[1]}: {ol} vs {ul}")
        elif (ol == "v0") != (ul == "v0") and (is_reg(ol) or is_reg(ul)):
            diffs.append(f"location {key[1]}: {ol} vs {ul}")
    return diffs


def originals():
    dw = Dwarf1(dwarf1.DEFAULT_ELF)
    out = {}
    for cu in dw.roots:
        if cu.tag != TAG_compile_unit:
            continue
        src = str(cu.attrs.get(AT_name, "")).replace("\\", "/")
        for c in cu.children:
            if c.tag in (TAG_global_subroutine, TAG_subroutine) and AT_low_pc in c.attrs:
                out.setdefault(c.name, []).append((src, variables(dw, c)))
    return out


ORIG = None


def unit_source(cunit):
    """src/<unit>.c, or $UNIT_SRC: a variant of the one unit being checked (see the usage above)."""
    return Path(os.environ["UNIT_SRC"]) if os.environ.get("UNIT_SRC") else ROOT / "src" / f"{cunit}.c"


def unit_list():
    units = re.findall(r"(?m)^  unit = (\S+)$", (ROOT / "build.ninja").read_text())
    return sorted(set(units))


def compile_g(unit):
    import configure
    cunit = unit.split(":")[-1]
    src = unit_source(cunit)
    obj = Path(tempfile.mkdtemp()) / "g.o"
    r = subprocess.run([str(ROOT / configure.WIBO), str(ROOT / configure.MWCC), "-c", "-g",
                        *configure.unit_cflags(cunit), "-nostdinc", "-Iinclude", "-stderr", str(src), "-o", str(obj)],
                       cwd=ROOT, capture_output=True)
    return obj if r.returncode == 0 else None


def work(unit):
    global ORIG
    if ORIG is None:
        ORIG = originals()
    obj = compile_g(unit)
    if obj is None:
        return unit, None
    dw = ObjDwarf(obj)
    cunit = unit.split(":")[-1]
    res = []
    for name, f in dw.funcs.items():
        cands = ORIG.get(name, [])
        hit = [v for s, v in cands if cunit.split("/")[-1] + ".c" in s.split("/")[-1]] or [v for s, v in cands]
        if not hit or name.startswith("__stripped"):
            continue
        res.append((name, compare(hit[0], variables(dw, f))))
    return unit, res


def fallback():
    out = set()
    for line in (ROOT / "config/asm_functions.txt").read_text(encoding="utf-8").splitlines():
        p = line.split("#")[0].split()
        if len(p) == 2:
            out.add((p[0], p[1]))
    return out


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    only = sys.argv[1] if len(sys.argv) > 1 else None
    if os.environ.get("UNIT_SRC") and not only:
        sys.exit("UNIT_SRC needs the unit it is a variant of")
    units = [u for u in unit_list() if not only or u.split(":")[-1] == only or u == only]
    nonmatching = fallback()
    with Pool(int(os.environ.get("J", "8"))) as pool:
        results = pool.map(work, units)
    kinds = Counter()
    rows, n_funcs, n_same = [], 0, 0
    for unit, res in results:
        if res is None:
            rows.append((unit, "(compile error)", []))
            continue
        cunit = unit.split(":")[-1]
        for name, diffs in sorted(res):
            if (cunit, name) in nonmatching or (unit, name) in nonmatching:
                continue
            n_funcs += 1
            if not diffs:
                n_same += 1
                continue
            for d in diffs:
                kinds[d.split(" ")[0]] += 1
            rows.append((unit, name, diffs))
    if only:
        for unit, name, diffs in rows:
            print(f"{unit} {name}: " + "; ".join(diffs))
        print(f"{n_same}/{n_funcs} matched functions match the original's DWARF")
        return 0
    lines = ["# DWARF fidelity of matched functions", "",
             "Generated by `tools/dwarf_compare.py`. For every matched function, the locals and parameters",
             "of our `-g` compile compared with the original's DWARF (see the tool for what is compared).",
             "The bytes match either way; these are the places where the C still differs from the",
             "original's source, with the evidence to bring it closer.", "",
             f"{n_same} of {n_funcs} matched functions match the original's DWARF. Differences: "
             + ", ".join(f"{kinds[k]} {k}" for k in ("missing", "extra", "order", "type", "register", "location")
                    if kinds[k])
             + ".", "",
             "| unit | function | differences |", "|---|---|---|"]
    for unit, name, diffs in rows:
        lines.append(f"| `{unit}` | `{name}` | " + "; ".join(d.replace("|", "\\|") for d in diffs) + " |")
    (ROOT / "docs" / "dwarf-fidelity.md").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    print(f"{n_same}/{n_funcs} matched functions match the original's DWARF; "
          + ", ".join(f"{kinds[k]} {k}" for k in kinds))
    return 0


if __name__ == "__main__":
    sys.exit(main())
