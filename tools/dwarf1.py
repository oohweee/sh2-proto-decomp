#!/usr/bin/env python3
"""DWARF 1 reader for the SH2 prototype ELF (Metrowerks MIPS C 2.4.1.01).

Prints C declarations recovered from the .debug section:

    dwarf1.py func <name|addr>      prototype, locals, and the types they use
    dwarf1.py var <name|addr>       global/static variable declaration
    dwarf1.py type <name>           struct/union/enum/typedef definition
    dwarf1.py globals <func>        globals the function's CU knows about (by address)

Metrowerks emits one compile unit per function; struct/union/enum DIEs live in
separate type-only units and are referenced across units. No typedef DIEs are
emitted at all, so anonymous aggregates keep MWCC's "@anonN" names and their
typedef names have to be recovered by hand.
"""
import argparse
import struct
import sys
from pathlib import Path

DEFAULT_ELF = Path(__file__).resolve().parent.parent / "baserom" / "disc" / "SLUS_202.28"

# Tags
TAG_array_type = 0x01
TAG_enumeration_type = 0x04
TAG_formal_parameter = 0x05
TAG_global_subroutine = 0x06
TAG_global_variable = 0x07
TAG_lexical_block = 0x0B
TAG_local_variable = 0x0C
TAG_member = 0x0D
TAG_pointer_type = 0x0F
TAG_compile_unit = 0x11
TAG_structure_type = 0x13
TAG_subroutine = 0x14
TAG_subroutine_type = 0x15
TAG_typedef = 0x16
TAG_union_type = 0x17
TAG_unspecified_parameters = 0x18

# Attributes (name << 4 | form)
AT_sibling = 0x0012
AT_location = 0x0023
AT_name = 0x0038
AT_fund_type = 0x0055
AT_mod_fund_type = 0x0063
AT_user_def_type = 0x0072
AT_mod_u_d_type = 0x0083
AT_subscr_data = 0x00A3
AT_byte_size = 0x00B6
AT_bit_offset = 0x00C5
AT_bit_size = 0x00D6
AT_element_list = 0x00F3
AT_element_list4 = 0x00F4
AT_low_pc = 0x0111
AT_high_pc = 0x0121
AT_prototyped = 0x0270

FORM_SIZE = {0x1: 4, 0x2: 4, 0x5: 2, 0x6: 4, 0x7: 8}

FUND = {
    0x0001: "char", 0x0002: "signed char", 0x0003: "unsigned char",
    # MWCC emits FT_signed_* for plain short/int/long.
    0x0004: "short", 0x0005: "short", 0x0006: "unsigned short",
    0x0007: "int", 0x0008: "int", 0x0009: "unsigned int",
    0x000A: "long", 0x000B: "long", 0x000C: "unsigned long",
    0x000D: "void*", 0x000E: "float", 0x000F: "double", 0x0010: "long double",
    0x0014: "void", 0x0015: "bool",
    0x8008: "long long", 0x8108: "signed long long", 0x8208: "unsigned long long",
    # Metrowerks 128-bit integers (16 bytes; Sony's u_long128 / long128)
    0xA410: "long128", 0xA510: "u_long128",
}

MOD_pointer_to = 0x01
MOD_reference_to = 0x02
MOD_const = 0x03
MOD_volatile = 0x04

MIPS_REGS = ["zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
             "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
             "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
             "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"]


class Die:
    __slots__ = ("off", "tag", "attrs", "end", "children")

    def __init__(self, off, tag, attrs, end):
        self.off, self.tag, self.attrs, self.end = off, tag, attrs, end
        self.children = []

    @property
    def name(self):
        return self.attrs.get(AT_name)


class Dwarf1:
    def __init__(self, elf_path):
        data = Path(elf_path).read_bytes()
        (_, _, _, _, _, _, shoff, _, _, _, _, shentsize, shnum, shstrndx) = \
            struct.unpack_from("<16sHHIIIIIHHHHHH", data, 0)
        secs = [struct.unpack_from("<10I", data, shoff + i * shentsize) for i in range(shnum)]
        strtab = secs[shstrndx][4]
        for s in secs:
            nm = data[strtab + s[0]:data.index(b"\0", strtab + s[0])]
            if nm == b".debug":
                self.buf = data[s[4]:s[4] + s[5]]
        self.dies = {}
        self.roots = []
        # Optional {DIE offset: name} for aggregates, used instead of the DWARF name
        # (MWCC names anonymous aggregates "@anonN", numbered per compile unit).
        self.name_override = {}
        self._parse()

    def _parse(self):
        buf = self.buf
        off = 0
        flat = []
        while off < len(buf):
            length = struct.unpack_from("<I", buf, off)[0]
            if length < 8:
                flat.append(None)
                off += max(length, 4)
                continue
            tag = struct.unpack_from("<H", buf, off + 4)[0]
            p, end, attrs = off + 6, off + length, {}
            while p < end:
                at = struct.unpack_from("<H", buf, p)[0]
                p += 2
                form = at & 0xF
                if form == 0x8:
                    e = buf.index(b"\0", p)
                    val = buf[p:e].decode("cp932", "replace")
                    p = e + 1
                elif form == 0x3:
                    n = struct.unpack_from("<H", buf, p)[0]
                    val = buf[p + 2:p + 2 + n]
                    p += 2 + n
                elif form == 0x4:
                    n = struct.unpack_from("<I", buf, p)[0]
                    val = buf[p + 4:p + 4 + n]
                    p += 4 + n
                else:
                    sz = FORM_SIZE[form]
                    val = int.from_bytes(buf[p:p + sz], "little")
                    p += sz
                attrs[at] = val
            die = Die(off, tag, attrs, end)
            self.dies[off] = die
            flat.append(die)
            off = end
        # Build the tree from sibling pointers: children of D are the DIEs
        # between D.end and D's sibling.
        stack = []
        for die in flat:
            if die is None:
                continue
            while stack and die.off >= stack[-1].attrs.get(AT_sibling, stack[-1].end):
                stack.pop()
            if stack:
                stack[-1].children.append(die)
            else:
                self.roots.append(die)
            if die.attrs.get(AT_sibling, die.end) > die.end:
                stack.append(die)
        # Indexes
        self.funcs = {}
        self.funcs_by_addr = {}
        self.types = {}
        self.vars = {}
        self.vars_by_addr = {}
        for cu in self.roots:
            for d in cu.children:
                self._index(d)

    def _index(self, d):
        if d.tag in (TAG_global_subroutine, TAG_subroutine) and AT_low_pc in d.attrs:
            self.funcs.setdefault(d.name, d)
            self.funcs_by_addr.setdefault(d.attrs[AT_low_pc], d)
        elif d.tag in (TAG_structure_type, TAG_union_type, TAG_enumeration_type, TAG_typedef):
            if d.name and (d.tag == TAG_typedef or AT_byte_size in d.attrs):
                self.types.setdefault(d.name, d)
        elif d.tag in (TAG_global_variable, TAG_local_variable):
            addr = self.static_addr(d)
            if addr is not None:
                self.vars.setdefault(d.name, d)
                self.vars_by_addr.setdefault(addr, d)
        for c in d.children:
            if c.tag in (TAG_local_variable, TAG_lexical_block):
                self._index(c)

    # --- locations -------------------------------------------------------

    @staticmethod
    def location(d):
        """Decode a location expression into a short string."""
        loc = d.attrs.get(AT_location)
        if loc is None:
            return None
        out, stack, p = [], [], 0
        while p < len(loc):
            op = loc[p]
            p += 1
            if op in (0x01, 0x02, 0x03, 0x04):
                val = struct.unpack_from("<I", loc, p)[0]
                p += 4
                if op == 0x01:
                    stack.append(("reg", val))
                elif op == 0x02:
                    stack.append(("basereg", val))
                elif op == 0x03:
                    stack.append(("addr", val))
                else:
                    stack.append(("const", val))
            elif op == 0x80:  # Metrowerks extension: floating-point register
                stack.append(("freg", struct.unpack_from("<I", loc, p)[0]))
                p += 4
            elif op == 0x07:
                b = stack.pop()
                a = stack.pop()
                stack.append(("add", a, b))
            else:
                stack.append(("op", op))
        for item in stack:
            out.append(Dwarf1._fmt_loc(item))
        return " ".join(out)

    @staticmethod
    def _fmt_loc(item):
        kind = item[0]
        if kind == "reg":
            return MIPS_REGS[item[1]] if item[1] < 32 else f"reg{item[1]}"
        if kind == "freg":
            return f"f{item[1]}"
        if kind == "basereg":
            return f"[{MIPS_REGS[item[1]] if item[1] < 32 else item[1]}]"
        if kind == "addr":
            return f"0x{item[1]:08X}"
        if kind == "const":
            return f"{item[1]:#x}"
        if kind == "add":
            a, b = item[1], item[2]
            if a[0] == "basereg" and b[0] == "const":
                off = b[1] - (1 << 32) if b[1] & 0x80000000 else b[1]
                return f"{off:#x}({MIPS_REGS[a[1]]})"
            return f"({Dwarf1._fmt_loc(a)} + {Dwarf1._fmt_loc(b)})"
        return f"op{item[1]:#x}"

    @staticmethod
    def static_addr(d):
        loc = d.attrs.get(AT_location)
        if loc and len(loc) == 5 and loc[0] == 0x03:
            return struct.unpack_from("<I", loc, 1)[0]
        return None

    @staticmethod
    def member_offset(d):
        loc = d.attrs.get(AT_location)
        if loc and len(loc) >= 5 and loc[0] == 0x04:
            return struct.unpack_from("<I", loc, 1)[0]
        return None

    # --- types -----------------------------------------------------------

    def type_of(self, d):
        """Return a type node for the DIE's type attributes.

        Nodes: ("fund", code) | ("die", Die) | ("ptr", node) | ("const", node) | ("volatile", node)
        """
        a = d.attrs
        if AT_fund_type in a:
            return ("fund", a[AT_fund_type])
        if AT_user_def_type in a:
            return ("die", self.dies[a[AT_user_def_type]])
        if AT_mod_fund_type in a:
            blk = a[AT_mod_fund_type]
            return self._apply_mods(blk[:-2], ("fund", struct.unpack_from("<H", blk, len(blk) - 2)[0]))
        if AT_mod_u_d_type in a:
            blk = a[AT_mod_u_d_type]
            return self._apply_mods(blk[:-4], ("die", self.dies[struct.unpack_from("<I", blk, len(blk) - 4)[0]]))
        return ("fund", 0x0014)

    @staticmethod
    def _apply_mods(mods, node):
        # Modifiers are listed outermost first, so apply them innermost first.
        for m in reversed(mods):
            if m == MOD_pointer_to:
                node = ("ptr", node)
            elif m == MOD_reference_to:
                node = ("ref", node)
            elif m == MOD_const:
                node = ("const", node)
            elif m == MOD_volatile:
                node = ("volatile", node)
        return node

    def array_dims(self, d):
        """Parse AT_subscr_data: returns (dims, element_type_node)."""
        blk = d.attrs[AT_subscr_data]
        p, dims, elem = 0, [], None
        while p < len(blk):
            fmt = blk[p]
            p += 1
            if fmt == 0x8:  # FMT_ET
                at = struct.unpack_from("<H", blk, p)[0]
                p += 2
                form = at & 0xF
                if form == 0x3:
                    n = struct.unpack_from("<H", blk, p)[0]
                    val = blk[p + 2:p + 2 + n]
                    p += 2 + n
                else:
                    sz = FORM_SIZE[form]
                    val = int.from_bytes(blk[p:p + sz], "little")
                    p += sz
                fake = Die(0, 0, {at: val}, 0)
                elem = self.type_of(fake)
            else:
                idx_is_ut = fmt >= 0x4
                p += 4 if idx_is_ut else 2
                lo_const = not (fmt & 0x2)
                hi_const = not (fmt & 0x1)
                if lo_const:
                    lo = struct.unpack_from("<i", blk, p)[0]
                    p += 4
                else:
                    n = struct.unpack_from("<H", blk, p)[0]
                    p += 2 + n
                    lo = 0
                if hi_const:
                    hi = struct.unpack_from("<i", blk, p)[0]
                    p += 4
                else:
                    n = struct.unpack_from("<H", blk, p)[0]
                    p += 2 + n
                    hi = None
                n = None if hi is None else hi - lo + 1
                dims.append(n if n else None)  # [] for arrays declared without a size
        return dims, elem

    def type_name(self, d):
        """Name used to refer to a user-defined type DIE."""
        kw = {TAG_structure_type: "struct", TAG_union_type: "union",
              TAG_enumeration_type: "enum"}.get(d.tag)
        if d.tag == TAG_typedef:
            return d.name
        if d.off in self.name_override:
            return f"{kw} {self.name_override[d.off]}" if kw else self.name_override[d.off]
        if kw:
            return f"{kw} {d.name}" if d.name else f"{kw} __anon_{d.off:X}"
        return None

    def decl(self, node, inner=""):
        """Render a C declaration of `inner` with type `node`."""
        kind = node[0]
        if kind == "fund":
            base = FUND.get(node[1], f"__fund_{node[1]:#x}")
            if base == "void*":
                return self.decl(("ptr", ("fund", 0x0014)), inner)
            return f"{base} {inner}".rstrip()
        if kind in ("ptr", "ref"):
            sym = "*" if kind == "ptr" else "&"
            return self.decl(node[1], self._wrap(node[1], sym + inner))
        if kind in ("const", "volatile"):
            tgt = node[1]
            if tgt[0] == "ptr":
                return self.decl(tgt[1], self._wrap(tgt[1], f"*{kind} {inner}".rstrip()))
            return f"{kind} " + self.decl(tgt, inner)
        d = node[1]
        if d.tag == TAG_pointer_type:
            return self.decl(("ptr", self.type_of(d)), inner)
        if d.tag == TAG_array_type:
            dims, elem = self.array_dims(d)
            suffix = "".join(f"[{n if n is not None else ''}]" for n in dims)
            if inner.startswith("*") or inner.startswith("&"):
                inner = f"({inner})"
            return self.decl(elem, inner + suffix)
        if d.tag == TAG_subroutine_type:
            if inner.startswith("*"):
                inner = f"({inner})"
            return self.decl(self.type_of(d), f"{inner}({self.params(d)})")
        return f"{self.type_name(d)} {inner}".rstrip()

    @staticmethod
    def _wrap(target, s):
        if target[0] == "die" and target[1].tag in (TAG_array_type, TAG_subroutine_type):
            return f"({s})"
        return s

    def params(self, d):
        ps = []
        for c in d.children:
            if c.tag == TAG_formal_parameter:
                ps.append(self.decl(self.type_of(c), c.name or ""))
            elif c.tag == TAG_unspecified_parameters:
                ps.append("...")
        if not ps:
            return "void"
        return ", ".join(ps)

    def deps(self, node, seen, out):
        """Collect user-defined type DIEs referenced by `node`, definitions first."""
        kind = node[0]
        if kind == "fund":
            return
        if kind in ("ptr", "ref", "const", "volatile"):
            self.deps(node[1], seen, out)
            return
        d = node[1]
        if d.off in seen:
            return
        seen.add(d.off)
        if d.tag in (TAG_pointer_type, TAG_typedef):
            self.deps(self.type_of(d), seen, out)
        elif d.tag == TAG_array_type:
            self.deps(self.array_dims(d)[1], seen, out)
        elif d.tag == TAG_subroutine_type:
            self.deps(self.type_of(d), seen, out)
            for c in d.children:
                if c.tag == TAG_formal_parameter:
                    self.deps(self.type_of(c), seen, out)
        elif d.tag in (TAG_structure_type, TAG_union_type):
            for c in d.children:
                if c.tag == TAG_member:
                    self.deps(self.type_of(c), seen, out)
        if d.tag in (TAG_structure_type, TAG_union_type, TAG_enumeration_type, TAG_typedef):
            out.append(d)

    def definition(self, d):
        if d.tag == TAG_typedef:
            return f"typedef {self.decl(self.type_of(d), d.name)};"
        if d.tag == TAG_enumeration_type:
            blk = d.attrs.get(AT_element_list) or d.attrs.get(AT_element_list4) or b""
            # Each element is a constant of the enum's byte size, then a name.
            vsize = d.attrs.get(AT_byte_size, 4)
            items, p = [], 0
            while p < len(blk):
                val = int.from_bytes(blk[p:p + vsize], "little")
                e = blk.index(b"\0", p + vsize)
                items.append((blk[p + vsize:e].decode("cp932", "replace"), val))
                p = e + 1
            # MWCC lists enumerators in declaration order (DWARF 1 suggests reverse order).
            body = "".join(f"    {n} = {v},\n" for n, v in items)
            return f"{self.type_name(d)} {{ // size {d.attrs.get(AT_byte_size, 0):#x}\n{body}}};"
        lines = [f"{self.type_name(d)} {{ // size {d.attrs.get(AT_byte_size, 0):#x}"]
        for c in d.children:
            if c.tag != TAG_member:
                continue
            off = self.member_offset(c)
            text = self.decl(self.type_of(c), c.name or "")
            if AT_bit_size in c.attrs:
                text += f" : {c.attrs[AT_bit_size]}"
            lines.append(f"    /* 0x{off if off is not None else 0:03X} */ {text};")
        lines.append("};")
        return "\n".join(lines)

    def printable(self, d):
        """Whether a dependency DIE gets its own definition in printed output."""
        if d.tag == TAG_typedef:
            return True
        return bool(d.children) or d.tag == TAG_enumeration_type

    # --- lookup helpers --------------------------------------------------

    def find_func(self, key):
        try:
            return self.funcs_by_addr.get(int(key, 16))
        except ValueError:
            return self.funcs.get(key)

    def find_var(self, key):
        try:
            return self.vars_by_addr.get(int(key, 16))
        except ValueError:
            return self.vars.get(key)

    def func_text(self, f, with_types=True):
        seen, deps = set(), []
        ret = self.type_of(f)
        self.deps(ret, seen, deps)
        for c in f.children:
            if c.tag == TAG_formal_parameter:
                self.deps(self.type_of(c), seen, deps)
        self._walk_locals(f, lambda c: self.deps(self.type_of(c), seen, deps))
        out = []
        if with_types:
            for d in deps:
                if not self.printable(d):
                    continue
                out.append(self.definition(d))
                out.append("")
        storage = "static " if f.tag == TAG_subroutine else ""
        lo, hi = f.attrs[AT_low_pc], f.attrs.get(AT_high_pc, 0)
        # Parameter locations are not printed: MWCC records every parameter as
        # register 2 regardless of where it lives.
        out.append(f"// 0x{lo:08X}-0x{hi:08X} (size {hi - lo:#x})")
        out.append(f"{storage}{self.decl(ret, f'{f.name}({self.params(f)})')} {{")
        self._print_locals(f, out, 1)
        out.append("}")
        return "\n".join(out)

    def _walk_locals(self, d, fn):
        for c in d.children:
            if c.tag == TAG_local_variable:
                fn(c)
            elif c.tag == TAG_lexical_block:
                self._walk_locals(c, fn)

    def _has_locals(self, d):
        return any(c.tag == TAG_local_variable or
                   (c.tag == TAG_lexical_block and self._has_locals(c)) for c in d.children)

    def _print_locals(self, d, out, depth):
        ind = "    " * depth
        blocks = [c for c in d.children if c.tag == TAG_lexical_block]
        # The function body is itself a lexical block; print its contents inline.
        if depth == 1 and len(blocks) == 1 and not any(c.tag == TAG_local_variable for c in d.children):
            self._print_locals(blocks[0], out, depth)
            return
        for c in d.children:
            if c.tag == TAG_local_variable:
                st = "static " if self.static_addr(c) is not None else ""
                out.append(f"{ind}{st}{self.decl(self.type_of(c), c.name or '')}; // {self.location(c)}")
            elif c.tag == TAG_lexical_block and self._has_locals(c):
                out.append(f"{ind}{{")
                self._print_locals(c, out, depth + 1)
                out.append(f"{ind}}}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--elf", default=str(DEFAULT_ELF))
    ap.add_argument("--no-types", action="store_true", help="omit type definitions")
    ap.add_argument("cmd", choices=["func", "var", "type", "globals"])
    ap.add_argument("key")
    args = ap.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")
    dw = Dwarf1(args.elf)

    if args.cmd == "func":
        f = dw.find_func(args.key)
        if not f:
            sys.exit(f"no function {args.key}")
        print(dw.func_text(f, not args.no_types))
    elif args.cmd == "var":
        v = dw.find_var(args.key)
        if not v:
            sys.exit(f"no variable {args.key}")
        seen, deps = set(), []
        dw.deps(dw.type_of(v), seen, deps)
        if not args.no_types:
            for d in deps:
                if dw.printable(d):
                    print(dw.definition(d) + "\n")
        print(f"{dw.decl(dw.type_of(v), v.name)}; // 0x{dw.static_addr(v):08X}")
    elif args.cmd == "type":
        d = dw.types.get(args.key)
        if not d:
            sys.exit(f"no type {args.key}")
        seen, deps = set(), []
        dw.deps(("die", d), seen, deps)
        for t in deps:
            if dw.printable(t):
                print(dw.definition(t) + "\n")
    elif args.cmd == "globals":
        f = dw.find_func(args.key)
        cu = next(r for r in dw.roots if f in r.children)
        for c in cu.children:
            addr = dw.static_addr(c)
            if c.tag == TAG_global_variable and addr is not None:
                print(f"0x{addr:08X} {dw.decl(dw.type_of(c), c.name)};")


if __name__ == "__main__":
    main()
