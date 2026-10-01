#!/usr/bin/env python3
"""Assign main's data/rodata/sdata/bss to translation units.

The Metrowerks linker lays out every section in the same object order as .text, so each
section is a sequence of per-object ranges in link order. Evidence for who owns an atom
(a symbol-delimited piece of a section):

- relocations from a code unit: a file-local symbol (LOCAL binding, `@N` literals,
  `name$N` statics) is only referenced by its own unit (strong); a referenced global is
  weak evidence for each referencing unit;
- DWARF: a global variable is listed in the compile unit of every file that uses it (weak);
  if only code-less files (data tables like data_bg_*.c) claim it, it belongs to one of them;
- relocations from data: an atom referenced only from another atom's data follows it.

Code units get the best monotonic (link order) assignment by dynamic programming. Atoms
claimed only by code-less files are "foreign" to code units and form blocks named after
their most likely file. Atoms without evidence follow the preceding atom.

This is a best estimate. It gets verified per unit when the unit moves to C: the compiled
object's section sizes must fit the assigned ranges exactly.

    datasplit.py            print a summary
"""
import bisect
import collections
import sys
from array import array
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import dwarf1  # noqa: E402
import gen_splat as g  # noqa: E402

SECTIONS = [
    ("data", g.DATA_START, g.RODATA_START),
    ("rodata", g.RODATA_START, g.SDATA_START),
    ("sdata", g.SDATA_START, g.BSS_START),
    ("bss", g.BSS_START, g.BSS_END),
]
STRONG, WEAK = 10, 1

# Assembler-built objects with sized section symbols (or VU data at the end of .data):
# exact ranges that belong to no C unit.
FIXED_RANGES = [
    (0x2AE310, 0x2AE310 + 0x80B0, "asmobj/data_2AE310"),
    (0x2BAF48, 0x2BAF48 + 0x802D8, "asmobj/fontdata"),
    (0x37DB80, g.RODATA_START, "asmobj/vudata"),
    (0x116E980, 0x116E980 + 0x144, "asmobj/bss_116E980"),
]


def is_local(sym):
    return sym.bind == 0 or sym.name.startswith("@") or "$" in sym.name


def dwarf_var_files(dw):
    """{address: {source file}} for every DWARF variable with a static address."""
    out = collections.defaultdict(set)

    def walk(cu, d):
        for c in d.children:
            if c.tag in (dwarf1.TAG_global_variable, dwarf1.TAG_local_variable):
                a = dw.static_addr(c)
                if a is not None:
                    out[a].add(cu.name)
            if c.tag in (dwarf1.TAG_global_subroutine, dwarf1.TAG_subroutine, dwarf1.TAG_lexical_block):
                walk(cu, c)

    for cu in dw.roots:
        walk(cu, cu)
    return out


def main_units(elf, dw):
    sources = g.dwarf_sources(dw)
    funcs = sorted((s.value, s.name) for s in elf.syms
                   if s.section == "main" and s.type == 2 and g.TEXT_START <= s.value < g.VUTEXT_START)
    lib_starts = {s.value for s in elf.syms if s.section == "main" and s.type == 3 and s.name == ".text"
                  and g.TEXT_START <= s.value < g.VUTEXT_START}
    return g.text_units(funcs, lib_starts, sources, "lib/")


class Splitter:
    def __init__(self, elf, dw, units):
        self.elf = elf
        # Consecutive library units become one block: their object boundaries are incomplete
        # (hand-written asm objects have no section symbols) and they stay asm anyway.
        merged = []
        for addr, u in units:
            if u.startswith("lib/") and merged and merged[-1][1].startswith("lib/"):
                continue
            merged.append((addr, u))
        self.units = [u for _, u in merged]
        self.unit_start = [a for a, _ in merged]
        self.index = {u: i for i, u in enumerate(self.units)}
        self.var_files = dwarf_var_files(dw)
        code_files = {cu.name for cu in dw.roots if any(dwarf1.AT_low_pc in c.attrs for c in cu.children)}
        self.codeless = {cu.name for cu in dw.roots
                         if cu.name and cu.name.lower().endswith(".c") and cu.name not in code_files}

    def unit_of_text(self, addr):
        i = bisect.bisect_right(self.unit_start, addr) - 1
        return i if i >= 0 and addr < g.VUTEXT_START else None

    def file_unit(self, src):
        if src.lower().endswith(".h"):
            return None
        try:
            return self.index.get(g.unit_path(src))
        except ValueError:
            return None

    def run(self):
        elf = self.elf
        off, size, _ = elf.sections["main"]
        image = elf.data[off:off + size]
        objs = [s for s in elf.syms if s.section == "main" and s.type == 1 and g.DATA_START <= s.value < g.BSS_END]
        secsyms = {s.value for s in elf.syms if s.section == "main" and s.type == 3 and s.value >= g.DATA_START
                   and s.name in (".data", ".rodata", ".bss", ".sdata", ".sbss")}
        starts = sorted({s.value for s in objs} | secsyms | {lo for _, lo, _ in SECTIONS}
                        | {lo for lo, _, _ in FIXED_RANGES} | {hi for _, hi, _ in FIXED_RANGES if hi < g.BSS_END})
        local_atoms = {s.value for s in objs if is_local(s)}

        def atom_of(addr):
            return starts[bisect.bisect_right(starts, addr) - 1]

        ev = collections.defaultdict(collections.Counter)
        refs = collections.defaultdict(set)
        data_refs = collections.defaultdict(set)
        static_funcs = {s.value for s in elf.syms if s.section == "main" and s.type == 2 and s.bind == 0}
        for off_, typ, sym, target in g.reloc_targets(elf, elf.relocs["main"], image, g.MAIN_VRAM):
            if target is None:
                continue
            # A table in data that points at a file's static function belongs to that file.
            if g.DATA_START <= off_ < g.BSS_START and target in static_funcs:
                u = self.unit_of_text(target)
                if u is not None:
                    ev[atom_of(off_)][u] += STRONG
                continue
            if not (g.DATA_START <= target < g.BSS_END):
                continue
            a = atom_of(target)
            u = self.unit_of_text(off_)
            if u is not None:
                refs[a].add(u)
            elif g.DATA_START <= off_ < g.BSS_START:
                data_refs[a].add(atom_of(off_))
        for a, us in refs.items():
            for u in us:
                ev[a][u] += STRONG if a in local_atoms else WEAK

        codeless_claims = collections.defaultdict(collections.Counter)
        for a, files in self.var_files.items():
            if not (g.DATA_START <= a < g.BSS_END):
                continue
            atom = atom_of(a)
            for f in files:
                u = self.file_unit(f)
                if u is not None:
                    ev[atom][u] += WEAK
                elif f in self.codeless:
                    codeless_claims[atom][f] += 1

        # Foreign atoms: claimed by code-less files and not referenced as file-local by code.
        foreign = {}
        for a, files in codeless_claims.items():
            if not any(w >= STRONG for w in ev[a].values()):
                foreign[a] = files
        # Atoms referenced only from foreign data are foreign too.
        for _ in range(3):
            for a, rs in data_refs.items():
                if a not in foreign and not ev[a] and rs and all(r in foreign for r in rs):
                    merged = collections.Counter()
                    for r in rs:
                        merged.update(foreign[r])
                    foreign[a] = merged
                elif a not in foreign and not ev[a] and rs:
                    for r in rs:
                        ev[a].update(ev[r])

        self.atoms, self.owner = {}, {}
        for name, lo, hi in SECTIONS:
            atoms = [a for a in starts if lo <= a < hi]
            self.atoms[name] = atoms
            fixed = {a: n for a in atoms for lo_, hi_, n in FIXED_RANGES if lo_ <= a < hi_}
            code_atoms = [a for a in atoms if a not in foreign and a not in fixed and ev[a]]
            chosen = self.monotone(code_atoms, ev)
            # A global the DP gave to a unit with no evidence for it is only referenced by
            # code (defined elsewhere): leave it unowned so it can join a code-less block.
            for a in code_atoms:
                if ev[a][chosen[a]] == 0:
                    del chosen[a]
            kind = dict(fixed)
            for a in atoms:
                if a in fixed:
                    continue
                if a in chosen:
                    kind[a] = self.units[chosen[a]]
                elif a in foreign:
                    kind[a] = "codeless:" + foreign[a].most_common(1)[0][0]
            # Unowned atoms next to a code-less block join it; the rest follow the previous owner.
            changed = True
            while changed:
                changed = False
                for i, a in enumerate(atoms):
                    if a in kind:
                        continue
                    for j in (i - 1, i + 1):
                        if 0 <= j < len(atoms) and str(kind.get(atoms[j], "")).startswith("codeless:"):
                            if a in ev and ev[a] or j == i + 1:
                                kind[a] = kind[atoms[j]]
                                changed = True
                                break
            # A code-less file can't sit inside one code unit's data: foreign atoms between
            # two atoms of the same code unit belong to that unit.
            code_kind = [(i, kind[a]) for i, a in enumerate(atoms)
                         if a in kind and not str(kind[a]).startswith(("codeless:", "asmobj/"))]
            for (i, u1), (j, u2) in zip(code_kind, code_kind[1:]):
                if u1 == u2 and j > i + 1:
                    for a in atoms[i + 1:j]:
                        if str(kind.get(a, "")).startswith("codeless:"):
                            kind[a] = u1
            owner, prev = {}, None
            for a in atoms:
                prev = kind.get(a, prev)
                owner[a] = prev
            first = next((owner[a] for a in atoms if owner[a] is not None), f"asmobj/{name}")
            for a in atoms:
                if owner[a] is None:
                    owner[a] = first
            self.owner[name] = owner
        self.ev, self.foreign = ev, foreign
        return self

    def monotone(self, atoms, ev):
        """Best non-decreasing unit index per atom (dynamic program over units)."""
        n = len(self.units)
        if not atoms:
            return {}
        prev = array("l", [0] * n)
        back = []
        for a in atoms:
            cur = array("l", [0] * n)
            arg = array("H", [0] * n)
            best, best_u = -1, 0
            e = ev[a]
            for u in range(n):
                if prev[u] > best:
                    best, best_u = prev[u], u
                cur[u] = best + e.get(u, 0)
                arg[u] = best_u
            back.append(arg)
            prev = cur
        u = max(range(n), key=lambda x: prev[x])
        chosen = {}
        for i in range(len(atoms) - 1, -1, -1):
            chosen[atoms[i]] = u
            u = back[i][u]
        return chosen

    def ranges(self):
        """{section: [(start, end, owner)]} with consecutive atoms of one owner merged.

        Owners are unit names; consecutive code-less atoms become one block named
        `codeless/<section>_<ADDR>` (their per-file attribution is only a guess).
        """
        out = {}
        for name, lo, hi in SECTIONS:
            runs = []
            atoms = self.atoms[name]
            for i, a in enumerate(atoms):
                end = atoms[i + 1] if i + 1 < len(atoms) else hi
                o = self.owner[name][a]
                if o.startswith("codeless:"):
                    o = "codeless"
                if runs and runs[-1][2] == o:
                    runs[-1][1] = end
                else:
                    runs.append([a, end, o])
            out[name] = [(a, e, f"codeless/{name}_{a:X}" if o == "codeless" else o) for a, e, o in runs]
        return out


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    elf = g.read_elf(g.ELF)
    dw = dwarf1.Dwarf1(g.ELF)
    sp = Splitter(elf, dw, main_units(elf, dw)).run()
    ranges = sp.ranges()
    for name, _, _ in SECTIONS:
        rs = ranges[name]
        owners = collections.Counter(o for _, _, o in rs)
        repeated = [o for o, c in owners.items() if c > 1]
        code = sum(1 for o in owners if o and not o.startswith(("codeless", "asmobj")))
        print(f"{name:7} atoms {len(sp.atoms[name]):6} runs {len(rs):5} code units {code:4} "
              f"code-less blocks {len(owners) - code:4} owners split into >1 run {len(repeated)}")
        for o in repeated[:6]:
            print(f"    {o}: {[(hex(s), hex(e)) for s, e, x in rs if x == o][:4]}")


if __name__ == "__main__":
    main()
