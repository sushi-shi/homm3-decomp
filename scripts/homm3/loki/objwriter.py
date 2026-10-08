"""Write a relocatable ELF32 i386 object holding comparison code sections."""
from __future__ import annotations

from pathlib import Path
import struct

from homm3.loki.cmpobj import CodeSection, canonical_symbol

SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB, SHT_REL = 1, 2, 3, 9
SHF_ALLOC, SHF_EXECINSTR = 2, 4


class _Strings:
    def __init__(self):
        self.data = bytearray(b"\0")
        self.index = {"": 0}

    def add(self, text: str) -> int:
        text = canonical_symbol(text)
        if text not in self.index:
            self.index[text] = len(self.data)
            self.data += text.encode("latin-1") + b"\0"
        return self.index[text]


def write(path: Path, sections: list[CodeSection]) -> None:
    shstr, strtab = _Strings(), _Strings()
    # Section indices: 0 null, 1..n code, then rel per code, symtab, strtab, shstrtab.
    n = len(sections)
    code_index = {i: 1 + i for i in range(n)}
    defined: dict[str, tuple[int, int, int, bool]] = {}
    for i, section in enumerate(sections):
        for function in section.functions:
            defined.setdefault(function.name, (code_index[i], function.offset, function.size, function.bind_global))
    locals_ = [(name, *v) for name, v in defined.items() if not v[3]]
    globals_ = [(name, *v) for name, v in defined.items() if v[3]]
    externs = sorted({r.target for s in sections for r in s.relocs} - set(defined))
    symbols = [(0, 0, 0, 0, 0, 0)]  # name, value, size, info, other, shndx
    for i in range(n):
        symbols.append((0, 0, 0, 3, 0, code_index[i]))  # STB_LOCAL, STT_SECTION
    index_of = {}
    for name, shndx, value, size, _ in locals_:
        index_of[name] = len(symbols)
        symbols.append((strtab.add(name), value, size, 2, 0, shndx))           # LOCAL FUNC
    first_global = len(symbols)
    for name, shndx, value, size, _ in globals_:
        index_of[name] = len(symbols)
        symbols.append((strtab.add(name), value, size, (1 << 4) | 2, 0, shndx))  # GLOBAL FUNC
    for name in externs:
        index_of[name] = len(symbols)
        symbols.append((strtab.add(name), 0, 0, 1 << 4, 0, 0))                  # GLOBAL NOTYPE UND
    blobs = []
    headers = [(0,) * 10]
    offset = 52

    def place(data: bytes, align: int) -> int:
        nonlocal offset
        offset = (offset + align - 1) // align * align
        start = offset
        blobs.append((start, bytes(data)))
        offset += len(data)
        return start

    for section in sections:
        start = place(section.data, 16)
        headers.append((shstr.add(section.name), SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0,
                        start, len(section.data), 0, 0, 16, 0))
    symtab_index = 1 + n + n
    for i, section in enumerate(sections):
        rel = b"".join(struct.pack("<II", r.offset, (index_of[r.target] << 8) | r.type)
                       for r in sorted(section.relocs, key=lambda r: r.offset))
        start = place(rel, 4)
        headers.append((shstr.add(".rel" + section.name), SHT_REL, 0, 0, start, len(rel),
                        symtab_index, code_index[i], 4, 8))
    symtab = b"".join(struct.pack("<IIIBBH", *s) for s in symbols)
    start = place(symtab, 4)
    headers.append((shstr.add(".symtab"), SHT_SYMTAB, 0, 0, start, len(symtab),
                    symtab_index + 1, first_global, 4, 16))
    start = place(strtab.data, 1)
    headers.append((shstr.add(".strtab"), SHT_STRTAB, 0, 0, start, len(strtab.data), 0, 0, 1, 0))
    shstr_name = shstr.add(".shstrtab")
    start = place(shstr.data, 1)
    headers.append((shstr_name, SHT_STRTAB, 0, 0, start, len(shstr.data), 0, 0, 1, 0))
    shoff = (offset + 3) // 4 * 4
    out = bytearray(shoff + 40 * len(headers))
    ident = b"\x7fELF\x01\x01\x01" + bytes(9)
    out[:52] = ident + struct.pack("<HHIIIIIHHHHHH", 1, 3, 1, 0, 0, shoff, 0, 52, 0, 0, 40,
                                   len(headers), len(headers) - 1)
    for start, data in blobs:
        out[start:start + len(data)] = data
    for i, header in enumerate(headers):
        struct.pack_into("<IIIIIIIIII", out, shoff + 40 * i, *header)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(out))
