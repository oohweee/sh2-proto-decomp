"""A small reader/writer for 32-bit little-endian MIPS relocatable ELF objects.

Enough to edit an object and write it back: section contents, the symbol table (kept in the
required locals-first order, with relocations renumbered), and SHT_REL relocations.
"""
import struct
from pathlib import Path

SHT_SYMTAB, SHT_STRTAB, SHT_REL, SHT_NOBITS = 2, 3, 9, 8
STB_LOCAL, STB_GLOBAL = 0, 1
STT_NOTYPE, STT_OBJECT, STT_FUNC, STT_SECTION = 0, 1, 2, 3


class Symbol:
    def __init__(self, name, value, size, bind, typ, shndx, other=0):
        self.name, self.value, self.size = name, value, size
        self.bind, self.type, self.shndx, self.other = bind, typ, shndx, other

    def __repr__(self):
        return f"Symbol({self.name!r}, {self.value:#x}, shndx={self.shndx})"


class Section:
    def __init__(self, name, header, data):
        (self.name_off, self.type, self.flags, self.addr, _, _, self.link, self.info,
         self.addralign, self.entsize) = header
        self.name, self.data = name, bytearray(data)
        self.size_override = None  # NOBITS sections have a size but no data

    @property
    def size(self):
        return self.size_override if self.type == SHT_NOBITS else len(self.data)


class ElfObj:
    def __init__(self, path):
        d = Path(path).read_bytes()
        self.ident_rest = d[:0x34]
        (_, self.etype, self.machine, self.version, self.entry, _, shoff, self.flags, _, _, _,
         shentsize, shnum, shstrndx) = struct.unpack_from("<16sHHIIIIIHHHHHH", d, 0)
        headers = [struct.unpack_from("<10I", d, shoff + i * shentsize) for i in range(shnum)]
        shstr = headers[shstrndx]
        self.shstrndx = shstrndx
        self.sections = []
        for h in headers:
            name = d[shstr[4] + h[0]:d.index(b"\0", shstr[4] + h[0])].decode()
            data = b"" if h[1] == SHT_NOBITS else d[h[4]:h[4] + h[5]]
            s = Section(name, (h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7], h[8], h[9]), data)
            if h[1] == SHT_NOBITS:
                s.size_override = h[5]
            self.sections.append(s)
        self.symtab_index = next(i for i, s in enumerate(self.sections) if s.type == SHT_SYMTAB)
        st = self.sections[self.symtab_index]
        strs = self.sections[st.link].data
        self.symbols = []
        for k in range(len(st.data) // 16):
            n, v, sz, info, other, shndx = struct.unpack_from("<IIIBBH", st.data, k * 16)
            name = strs[n:strs.index(b"\0", n)].decode("latin1")
            self.symbols.append(Symbol(name, v, sz, info >> 4, info & 0xF, shndx, other))
        # relocations: {target section index: [[offset, type, Symbol]]}
        self.relocs = {}
        self.rel_index = {}
        for i, s in enumerate(self.sections):
            if s.type == SHT_REL:
                self.rel_index[s.info] = i
                self.relocs[s.info] = [[o, info & 0xFF, self.symbols[info >> 8]]
                                       for o, info in struct.iter_unpack("<II", bytes(s.data))]

    # --- queries -------------------------------------------------------------------------------
    def find_symbol(self, name, defined=True):
        for s in self.symbols:
            if s.name == name and (s.shndx != 0) == defined:
                return s
        return None

    def section_symbol(self, shndx):
        """The STT_SECTION symbol for a section, created if missing."""
        for s in self.symbols:
            if s.type == STT_SECTION and s.shndx == shndx:
                return s
        sym = Symbol("", 0, 0, STB_LOCAL, STT_SECTION, shndx)
        self.symbols.append(sym)
        return sym

    def undefined(self, name):
        """A global undefined symbol `name`, created if missing."""
        s = self.find_symbol(name, defined=False)
        if s is None:
            s = Symbol(name, 0, 0, STB_GLOBAL, STT_NOTYPE, 0)
            self.symbols.append(s)
        return s

    # --- output --------------------------------------------------------------------------------
    def write(self, path):
        # Symbol table: null symbol, locals, then globals (sh_info = first global).
        null = self.symbols[0]
        rest = [s for s in self.symbols[1:]]
        ordered = [null] + [s for s in rest if s.bind == STB_LOCAL] + [s for s in rest if s.bind != STB_LOCAL]
        index = {id(s): i for i, s in enumerate(ordered)}
        st = self.sections[self.symtab_index]
        strtab = self.sections[st.link]
        strs = bytearray(b"\0")
        offsets = {}
        symdata = bytearray()
        for s in ordered:
            if s.name and s.name not in offsets:
                offsets[s.name] = len(strs)
                strs += s.name.encode("latin1") + b"\0"
            symdata += struct.pack("<IIIBBH", offsets.get(s.name, 0), s.value, s.size,
                                   (s.bind << 4) | s.type, s.other, s.shndx)
        strtab.data = strs
        st.data = symdata
        st.info = 1 + sum(1 for s in rest if s.bind == STB_LOCAL)
        for target, entries in self.relocs.items():
            if target not in self.rel_index:  # a section that had no relocations before
                if not entries:
                    continue
                rel = Section(".rel" + self.sections[target].name,
                              (0, SHT_REL, 0, 0, 0, 0, self.symtab_index, target, 4, 8), b"")
                self.rel_index[target] = len(self.sections)
                self.sections.append(rel)
            rel = self.sections[self.rel_index[target]]
            rel.data = bytearray(b"".join(struct.pack("<II", o, (index[id(sym)] << 8) | t)
                                          for o, t, sym in entries))
        # Section name table.
        shstr = self.sections[self.shstrndx]
        names = bytearray(b"\0")
        name_offs = []
        for s in self.sections:
            if s.name:
                name_offs.append(len(names))
                names += s.name.encode() + b"\0"
            else:
                name_offs.append(0)
        shstr.data = names
        # Lay out: header, section contents, section headers.
        out = bytearray(0x34)
        offs = []
        for s in self.sections:
            align = max(s.addralign, 4) if s.type != SHT_NOBITS else 1
            out += b"\0" * (-len(out) % align)
            offs.append(len(out))
            if s.type != SHT_NOBITS:
                out += s.data
        out += b"\0" * (-len(out) % 4)
        shoff = len(out)
        for s, no, off in zip(self.sections, name_offs, offs):
            size = s.size
            out += struct.pack("<10I", no, s.type, s.flags, s.addr, off if s.type or size else 0,
                               size, s.link, s.info, s.addralign, s.entsize)
        out[0:0x34] = self.ident_rest
        struct.pack_into("<IHHHHHH", out, 0x20, shoff, 0, 0, 0, 0, 0, 0)  # placeholder, fixed below
        struct.pack_into("<I", out, 0x20, shoff)
        struct.pack_into("<IHHHHHH", out, 0x24, self.flags, 0x34, 0, 0, 40, len(self.sections),
                         self.shstrndx)
        Path(path).write_bytes(out)
