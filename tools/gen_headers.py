#!/usr/bin/env python3
"""Generate include/sh2/{types,functions,variables}.h and include/sh2/api/ from the prototype's DWARF.

- types.h:     every struct/union/enum, one definition per type, in dependency order, each
               with a compile-time size check. Anonymous aggregates (MWCC's per-CU "@anonN")
               get stable names from a hash of their layout, `anon_<size>_<hash>`; give them
               real names in config/type_names.txt (`<anon name> <new name>` per line). A named
               type with a different layout in different units is only declared.
- api/<dir>.h: prototypes for every global function with DWARF, one header per source directory,
               by unit in code order; config/prototype_overrides.txt corrects them. A prototype
               that a unit must not see (`local` lines there; functions that are also another
               unit's static) is wrapped in #ifndef SH2_LOCAL_<function>.
- functions.h: includes every api/ header.
- variables.h: extern declarations for every global variable.

    python3 tools/gen_headers.py
"""
import collections
import hashlib
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import dwarf1 as dw1  # noqa: E402
import gen_splat as g  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "include" / "sh2"
AGG = (dw1.TAG_structure_type, dw1.TAG_union_type, dw1.TAG_enumeration_type)


class Canon:
    """Layout signatures that are identical across compile units."""

    def __init__(self, dw):
        self.dw = dw
        self.memo = {}

    def agg(self, d, stack=()):
        if d.off in self.memo:
            return self.memo[d.off]
        if d.tag == dw1.TAG_enumeration_type:
            sig = ("E", d.attrs.get(dw1.AT_byte_size), bytes(d.attrs.get(dw1.AT_element_list, b"")))
        else:
            members = []
            for c in d.children:
                if c.tag == dw1.TAG_member:
                    members.append((c.name, self.dw.member_offset(c), c.attrs.get(dw1.AT_bit_size),
                                    c.attrs.get(dw1.AT_bit_offset), self.node(self.dw.type_of(c), stack + (d.off,))))
            sig = (d.tag, d.attrs.get(dw1.AT_byte_size), tuple(members))
        self.memo[d.off] = sig
        return sig

    def node(self, n, stack):
        kind = n[0]
        if kind == "fund":
            return ("F", n[1])
        if kind in ("ptr", "ref", "const", "volatile"):
            return (kind, self.node(n[1], stack))
        d = n[1]
        if d.tag in AGG:
            if d.name and not d.name.startswith("@"):
                return ("N", d.tag, d.name)
            if d.off in stack:
                return ("R",)
            return self.agg(d, stack)
        if d.tag == dw1.TAG_array_type:
            dims, elem = self.dw.array_dims(d)
            return ("A", tuple(dims), self.node(elem, stack))
        if d.tag == dw1.TAG_pointer_type:
            return ("P", self.node(self.dw.type_of(d), stack))
        if d.tag == dw1.TAG_subroutine_type:
            params = tuple(self.node(self.dw.type_of(c), stack) for c in d.children if c.tag == dw1.TAG_formal_parameter)
            return ("S", self.node(self.dw.type_of(d), stack), params)
        return ("?", d.tag)


def type_deps(dw, node, by_value, out):
    """Collect aggregate DIEs that `node` needs defined (by_value) or only declared."""
    kind = node[0]
    if kind == "fund":
        return
    if kind in ("ptr", "ref"):
        type_deps(dw, node[1], False, out)
        return
    if kind in ("const", "volatile"):
        type_deps(dw, node[1], by_value, out)
        return
    d = node[1]
    if d.tag in AGG:
        out.append((d, by_value))
    elif d.tag == dw1.TAG_array_type:
        type_deps(dw, dw.array_dims(d)[1], by_value, out)
    elif d.tag == dw1.TAG_pointer_type:
        type_deps(dw, dw.type_of(d), False, out)
    elif d.tag == dw1.TAG_subroutine_type:
        type_deps(dw, dw.type_of(d), False, out)
        for c in d.children:
            if c.tag == dw1.TAG_formal_parameter:
                type_deps(dw, dw.type_of(c), False, out)


# (size, alignment) of fundamental types under MWCC for the EE (long is 64-bit).
FUND_LAYOUT = {
    0x0001: (1, 1), 0x0002: (1, 1), 0x0003: (1, 1), 0x0004: (2, 2), 0x0005: (2, 2), 0x0006: (2, 2),
    0x0007: (4, 4), 0x0008: (4, 4), 0x0009: (4, 4), 0x000A: (8, 8), 0x000B: (8, 8), 0x000C: (8, 8),
    0x000D: (4, 4), 0x000E: (4, 4), 0x000F: (8, 8), 0x0010: (8, 8), 0x0014: (0, 1), 0x0015: (1, 1),
    0x8008: (8, 8), 0x8108: (8, 8), 0x8208: (8, 8), 0xA410: (16, 16), 0xA510: (16, 16),
}


def align_up(v, a):
    return (v + a - 1) // a * a


class Layout:
    """Reproduces the original struct layouts in C.

    The DWARF gives member offsets and sizes but not the typedefs that carried alignment
    (e.g. Sony's `float[4] __attribute__((aligned(16)))` vector types). Simulating C layout
    shows where a member sits later than natural alignment puts it (-> member alignment
    attribute) or where the struct is bigger than its natural size (-> struct alignment).
    """

    def __init__(self, dw, key_of):
        self.dw, self.key_of = dw, key_of
        self.member_align = {}   # key -> {member index: alignment}
        self.member_pad = {}     # key -> {member index: bytes of explicit padding before it}
        self.struct_align = {}   # key -> alignment attribute
        self.tail_pad = {}       # key -> bytes of explicit trailing padding
        self.align = {}          # key -> resulting alignment
        self.problems = []

    def size_align(self, node):
        kind = node[0]
        if kind == "fund":
            return FUND_LAYOUT.get(node[1], (4, 4))
        if kind in ("ptr", "ref"):
            return (4, 4)
        if kind in ("const", "volatile"):
            return self.size_align(node[1])
        d = node[1]
        if d.tag == dw1.TAG_enumeration_type:
            sz = d.attrs.get(dw1.AT_byte_size, 4)
            return (sz, sz)
        if d.tag in (dw1.TAG_structure_type, dw1.TAG_union_type):
            key = self.key_of.get(d.off)
            return (d.attrs.get(dw1.AT_byte_size, 0), self.align.get(key, 4))
        if d.tag == dw1.TAG_array_type:
            dims, elem = self.dw.array_dims(d)
            es, ea = self.size_align(elem)
            n = 1
            for x in dims:
                n *= x or 0
            return (es * n, ea)
        if d.tag == dw1.TAG_pointer_type:
            return (4, 4)
        return (0, 1)

    def slots(self, d):
        """Members grouped into layout slots: consecutive non-bitfield members at the same
        offset are an anonymous union in the source (the DWARF flattens it)."""
        members = [c for c in d.children if c.tag == dw1.TAG_member]
        out = []
        for c in members:
            dwo = self.dw.member_offset(c) or 0
            if (d.tag == dw1.TAG_structure_type and out and dw1.AT_bit_size not in c.attrs
                    and dw1.AT_bit_size not in out[-1][0].attrs
                    and (self.dw.member_offset(out[-1][0]) or 0) == dwo):
                out[-1].append(c)
            else:
                out.append([c])
        return out

    def decide(self, key, d):
        slots = self.slots(d)
        size = d.attrs.get(dw1.AT_byte_size, 0)
        m_align, m_pad = {}, {}
        off, align = 0, 1
        union = d.tag == dw1.TAG_union_type
        for i, slot in enumerate(slots):
            c = slot[0]
            sa = [self.size_align(self.dw.type_of(m)) for m in slot]
            s, a = max(x[0] for x in sa), max(x[1] for x in sa)
            dwo = self.dw.member_offset(c) or 0
            if dw1.AT_bit_size in c.attrs:
                off = max(off, dwo + s)  # bitfields: trust the DWARF container offset
                align = max(align, a)
                continue
            if union:
                align = max(align, a)
                off = max(off, s)
                continue
            nat = align_up(off, a)
            if dwo > nat:
                p = a
                while p <= 128 and align_up(off, p) != dwo:
                    p *= 2
                if p <= 128 and align_up(off, p) == dwo:
                    m_align[i] = p
                    a = p
                else:
                    m_pad[i] = dwo - off  # a gap no alignment explains: explicit padding
            elif dwo < nat:
                self.problems.append(f"{key[1]}.{c.name}: DWARF offset {dwo:#x} < natural {nat:#x}")
            align = max(align, a)
            off = dwo + s
        s_align, tail = None, 0
        if align_up(off, align) < size:
            p = align
            while p <= 128 and align_up(off, p) != size:
                p *= 2
            if p <= 128 and align_up(off, p) == size:
                s_align, align = p, p
            else:
                tail = size - align_up(off, align)
        elif align_up(off, align) > size:
            self.problems.append(f"{key[1]}: natural size {align_up(off, align):#x} > DWARF size {size:#x}")
        self.member_align[key], self.member_pad[key] = m_align, m_pad
        self.struct_align[key], self.tail_pad[key] = s_align, tail
        self.align[key] = align

    def render(self, key, d):
        kw = "struct" if d.tag == dw1.TAG_structure_type else "union"
        size = d.attrs.get(dw1.AT_byte_size, 0)
        lines = [f"{kw} {key[1]} {{ /* size 0x{size:X} */"]
        for i, slot in enumerate(self.slots(d)):
            off = self.dw.member_offset(slot[0]) or 0
            if i in self.member_pad[key]:
                lines.append(f"    unsigned char pad_{off - self.member_pad[key][i]:X}[0x{self.member_pad[key][i]:X}];")
            texts = []
            for j, c in enumerate(slot):
                text = self.dw.decl(self.dw.type_of(c), c.name or f"unk_{off:X}")
                if dw1.AT_bit_size in c.attrs:
                    text += f" : {c.attrs[dw1.AT_bit_size]}"
                if j == 0 and i in self.member_align[key]:
                    text += f" __attribute__((aligned({self.member_align[key][i]})))"
                texts.append(text)
            if len(slot) == 1:
                lines.append(f"    /* 0x{off:03X} */ {texts[0]};")
            else:
                lines.append(f"    /* 0x{off:03X} */ union {{")
                lines += [f"        {t};" for t in texts]
                lines.append("    };")
        if self.tail_pad[key]:
            lines.append(f"    unsigned char pad_tail[0x{self.tail_pad[key]:X}];")
        attr = f" __attribute__((aligned({self.struct_align[key]})))" if self.struct_align[key] else ""
        lines.append(f"}}{attr};")
        return "\n".join(lines)


# Compile units outside the game's src/ tree, named as in config/main.yaml.
OUTSIDE_PATHS = {"/PS2 Support/": "mwcc_runtime/", "/sound/sd0712/": "sd0712/"}
OUTSIDE = {"mwcc_runtime": "in the Metrowerks runtime (not in src/)",
           "sd0712": "in the sound driver's EE side (sd0712, not in src/)"}


def unit_path(cu):
    """A compile unit's source path: under src/, or as config/main.yaml names the others."""
    name = (cu.name or "?").replace("\\", "/")
    if "/src/" in name:
        return name.split("/src/")[-1]
    for marker, prefix in OUTSIDE_PATHS.items():
        if marker in name:
            return prefix + name.split(marker)[-1]
    return name


def unit_dir(unit):
    """The api/ header of a unit's functions: its directory (the top one outside src/)."""
    top = unit.split("/")[0]
    if top in OUTSIDE:
        return top
    return unit.rpartition("/")[0] or "top"


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    elf = g.read_elf(g.ELF)
    dw = dw1.Dwarf1(g.ELF)
    canon = Canon(dw)

    renames = {}
    names_file = ROOT / "config" / "type_names.txt"
    if names_file.exists():
        for line in names_file.read_text(encoding="utf-8").splitlines():
            line = line.split("#")[0].split()
            if len(line) == 2:
                renames[line[0]] = line[1]

    # One representative DIE per type; anonymous ones keyed by layout.
    reps, key_of, conflicts = {}, {}, []
    for d in dw.dies.values():
        if d.tag not in AGG or not (d.children or d.tag == dw1.TAG_enumeration_type):
            continue
        sig = canon.agg(d)
        if d.name and not d.name.startswith("@"):
            key = ("named", d.name)
            if key in reps and canon.agg(reps[key]) != sig:
                conflicts.append(d.name)
                continue
        else:
            h = hashlib.sha1(repr(sig).encode()).hexdigest()[:8]
            size = d.attrs.get(dw1.AT_byte_size, 0)
            auto = f"anon_{size:X}_{h}"
            key = ("anon", renames.get(auto, auto))
        reps.setdefault(key, d)
        key_of[d.off] = key
    for off, key in key_of.items():
        if key[0] == "anon":
            dw.name_override[off] = key[1]

    # Named types defined per unit: one layout in each unit that has the name, but not the same one
    # in all of them, so each file defined its own (Vertex_Infomeation_List). Only a forward
    # declaration is generated; the units define them. (A name with several layouts inside one unit,
    # like model3's block-scope `Data`s, keeps its first layout.)
    cu_of = {}
    for cu in dw.roots:
        stack = list(cu.children)
        while stack:
            d = stack.pop()
            cu_of[d.off] = cu.name
            stack.extend(d.children)
    per_cu = collections.defaultdict(lambda: collections.defaultdict(set))
    for d in dw.dies.values():
        if d.tag in AGG and d.name and ("named", d.name) in reps and d.children:
            per_cu[d.name][cu_of.get(d.off)].add(canon.agg(d))
    unit_types = {name for name, cus in per_cu.items()
                  if len(set().union(*cus.values())) > 1 and all(len(s) == 1 for s in cus.values())}

    # Order: enums first, then aggregates with by-value dependencies before their users.
    order, state = [], {}

    def visit(key):
        if state.get(key) == 2:
            return
        if state.get(key) == 1:
            return  # cycle through pointers only; forward declarations cover it
        state[key] = 1
        d = reps[key]
        deps = []
        for c in d.children:
            if c.tag == dw1.TAG_member:
                type_deps(dw, dw.type_of(c), True, deps)
        for dd, by_value in deps:
            k = key_of.get(dd.off)
            if k is not None and by_value and k != key:
                visit(k)
        state[key] = 2
        order.append(key)

    for key in sorted(reps, key=lambda k: (reps[k].tag != dw1.TAG_enumeration_type, k[1])):
        visit(key)

    layout = Layout(dw, key_of)
    for key in order:
        if reps[key].tag != dw1.TAG_enumeration_type:
            layout.decide(key, reps[key])

    enum_consts = set()
    lines = ["/* Generated by tools/gen_headers.py from the prototype's DWARF. Do not edit. */",
             "#ifndef SH2_TYPES_H", "#define SH2_TYPES_H", "", '#include "common.h"', "",
             "#define SH2_CAT2(a, b) a##b",
             "#define SH2_CAT(a, b) SH2_CAT2(a, b)",
             "/* Fails to compile if a type's size differs from the original. */",
             "#define SH2_SIZE_CHECK(type, size) typedef char SH2_CAT(sh2_size_check_, __LINE__)[(sizeof(type) == (size)) ? 1 : -1]",
             "", "/* Forward declarations */"]
    for key in order:
        d = reps[key]
        kw = {dw1.TAG_structure_type: "struct", dw1.TAG_union_type: "union"}.get(d.tag)
        if not kw:
            continue
        if key[0] == "named":
            lines.append(f"{kw} {key[1]};")
        else:
            lines.append(f"typedef {kw} {key[1]} {key[1]};")
    lines.append("")
    for key in order:
        d = reps[key]
        size = d.attrs.get(dw1.AT_byte_size, 0)
        if d.tag == dw1.TAG_enumeration_type:
            text = dw.definition(d)
            body = []
            for ln in text.splitlines():
                name = ln.strip().split(" = ")[0] if " = " in ln else None
                if name and name in enum_consts:
                    body.append(f"    /* duplicate enumerator {ln.strip()} */")
                    continue
                if name:
                    enum_consts.add(name)
                body.append(ln)
            lines += body + [""]
            continue
        kw = "struct" if d.tag == dw1.TAG_structure_type else "union"
        if key[0] == "named" and key[1] in unit_types:
            lines += [f"/* {kw} {key[1]}: defined by each unit that uses it (the layouts differ). */", ""]
            continue
        lines.append(layout.render(key, d))
        tname = f"{kw} {key[1]}" if key[0] == "named" else key[1]
        lines.append(f"SH2_SIZE_CHECK({tname}, 0x{size:X});")
        lines.append("")
    lines += ["#endif", ""]
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "types.h").write_text("\n".join(lines), encoding="utf-8", newline="\n")

    # Globals present in the ELF symbol table, to skip file statics.
    # GLOBAL and WEAK bindings (e.g. the execEnv_* settings are weak).
    global_funcs = {s.name for s in elf.syms if s.type == 2 and s.bind in (1, 2)}
    global_objs = {s.name for s in elf.syms if s.type == 1 and s.bind in (1, 2)}
    # Prototypes the DWARF gets wrong (MWCC drops `...`): config/prototype_overrides.txt,
    # one full prototype per line, keyed by function name.
    # `extern` lines override a global variable's declaration the same way.
    # `local <function> <unit> <reason>` lines: the unit declares the function itself (a call without
    # a prototype, or another signature); the prototype is wrapped in #ifndef SH2_LOCAL_<function>,
    # which that unit defines before including sh2.h.
    overrides, var_overrides, local = {}, {}, collections.defaultdict(list)
    ov_file = ROOT / "config" / "prototype_overrides.txt"
    if ov_file.exists():
        for line in ov_file.read_text(encoding="utf-8").splitlines():
            line = line.split("//")[0].strip()
            if line.startswith("local "):
                _, name, unit, reason = line.split(None, 3)
                local[name].append(f"{unit}.c declares it itself: {reason}")
            elif line.startswith("extern "):
                name = line.rstrip(";").split("[")[0].split()[-1].lstrip("*")
                var_overrides[name] = line if line.endswith(";") else line + ";"
            elif line:
                name = line.split("(")[0].split()[-1].lstrip("*")
                overrides[name] = line if line.endswith(";") else line + ";"

    # A global function whose name is also a static function of another unit: that unit hides the
    # prototype the same way (found in the DWARF, no configuration).
    for cu in dw.roots:
        unit = unit_path(cu)
        for c in cu.children:
            if c.tag == dw1.TAG_subroutine and dw1.AT_low_pc in c.attrs and c.name in global_funcs:
                local[c.name].append(f"{unit} has a static {c.name} of its own")

    # Prototypes by the directory and unit that define them (api/<dir>.h), in source order.
    protos, seen = collections.defaultdict(lambda: collections.defaultdict(list)), set()
    externs, seen_v = [], set()
    for cu in dw.roots:
        unit = unit_path(cu)
        for c in cu.children:
            if c.tag == dw1.TAG_global_subroutine and dw1.AT_low_pc in c.attrs and c.name in global_funcs \
                    and c.name not in seen:
                seen.add(c.name)
                proto = overrides.get(c.name) or f"{dw.decl(dw.type_of(c), f'{c.name}({dw.params(c)})')};"
                if c.name in local:
                    proto = f"/* {'; '.join(local[c.name])}. */\n#ifndef SH2_LOCAL_{c.name}\n{proto}\n#endif"
                protos[unit_dir(unit)][unit].append(proto)
            elif c.tag == dw1.TAG_global_variable and c.name in global_objs and c.name not in seen_v \
                    and dw.static_addr(c) is not None:
                seen_v.add(c.name)
                externs.append(var_overrides.get(c.name) or f"extern {dw.decl(dw.type_of(c), c.name)};")
    gen = "/* Generated by tools/gen_headers.py from the prototype's DWARF. Do not edit. */"
    api = OUT / "api"
    for old in sorted(api.rglob("*.h")) if api.exists() else []:
        old.unlink()
    includes = []
    for d in sorted(protos, key=str.lower):
        guard = "SH2_API_" + "".join(ch if ch.isalnum() else "_" for ch in d.upper()) + "_H"
        what = OUTSIDE.get(d) or ("directly in src/" if d == "top" else f"in src/{d}/")
        body = [gen, f"/* The global functions defined {what}, by unit, in the order of their code. */",
                f"#ifndef {guard}", f"#define {guard}", "", '#include "sh2/types.h"']
        for unit in sorted(protos[d], key=str.lower):
            body += ["", f"/* {unit} */"] + protos[d][unit]
        path = api / f"{d}.h"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("\n".join(body + ["", "#endif", ""]), encoding="utf-8", newline="\n")
        includes.append(f'#include "sh2/api/{d}.h"')
    (OUT / "functions.h").write_text("\n".join(
        [gen, "/* Prototypes of all the game's global functions: one header per source directory, api/<dir>.h",
         " * (functions defined directly in src/ are in api/top.h). */",
         "#ifndef SH2_FUNCTIONS_H", "#define SH2_FUNCTIONS_H", ""] + includes + ["", "#endif", ""]),
        encoding="utf-8", newline="\n")
    (OUT / "variables.h").write_text("\n".join(
        [gen, "#ifndef SH2_VARIABLES_H", "#define SH2_VARIABLES_H", "", '#include "sh2/types.h"', ""]
        + sorted(externs) + ["", "#endif", ""]), encoding="utf-8", newline="\n")
    (ROOT / "include" / "sh2.h").write_text(
        "/* All DWARF-derived declarations. */\n#ifndef SH2_H\n#define SH2_H\n\n"
        '#include "common.h"\n#include "sh2/types.h"\n#include "sh2/functions.h"\n#include "sh2/variables.h"\n\n#endif\n',
        encoding="utf-8", newline="\n")
    for pr in layout.problems[:10]:
        print("layout problem:", pr)
    print(f"member alignments: {sum(len(v) for v in layout.member_align.values())}, "
          f"struct alignments: {sum(1 for v in layout.struct_align.values() if v)}, "
          f"explicit paddings: {sum(len(v) for v in layout.member_pad.values()) + sum(1 for v in layout.tail_pad.values() if v)}")
    print(f"types: {len(order)} ({sum(1 for k in order if k[0] == 'anon')} anonymous), "
          f"conflicting named types skipped: {len(set(conflicts))} {sorted(set(conflicts))[:8]}; "
          f"functions: {len(seen)} in {len(protos)} api headers; variables: {len(externs)}")


if __name__ == "__main__":
    main()
