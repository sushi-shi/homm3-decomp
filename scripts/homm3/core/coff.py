"""homm3.core.coff - i386 COFF object symbol tables.

The authority oracle: extraction admits a clang-proposed name only if cl's own
obj carries that symbol (native proposes, era disposes). Replaces llvm-nm and
the old tree's three hand-rolled parsers with one reader.
"""

from __future__ import annotations

import struct
from pathlib import Path

IMAGE_SCN_CNT_CODE = 0x20
_EXTERNAL, _STATIC, _LABEL = 2, 3, 6


class Coff:
    def __init__(self, path: Path | str):
        self.data = d = Path(path).read_bytes()
        machine, nsec, _t, symptr, nsym, optsz, _ch = struct.unpack_from("<HHIIIHH", d, 0)
        if machine != 0x14C:
            raise ValueError(f"{path}: not an i386 COFF (machine=0x{machine:x})")
        if not symptr or not nsym:
            raise ValueError(f"{path}: no symbol table")
        self._strtab = symptr + nsym * 18
        self.section_chars: list[int] = []
        for i in range(nsec):
            base = 20 + optsz + i * 40
            self.section_chars.append(struct.unpack_from("<I", d, base + 36)[0])
        self.symbols: list[tuple[str, int, int, int]] = []   # (name, value, section, storage)
        i = 0
        while i < nsym:
            o = symptr + i * 18
            raw = d[o:o + 8]
            if raw[:4] == b"\0\0\0\0":
                off = self._strtab + struct.unpack_from("<I", raw, 4)[0]
                name = d[off:d.find(b"\0", off)].decode("latin-1")
            else:
                name = raw.rstrip(b"\0").decode("latin-1")
            value, section, _type, storage, naux = struct.unpack_from("<IhHBB", d, o + 8)
            self.symbols.append((name, value, section, storage))
            i += 1 + naux

    def all_names(self) -> set[str]:
        """Every symbol name - defined, static, label, COMMON, or undefined
        external (a matched global is only REFERENCED by its TU, so it appears
        as `U`). Section symbols (`.text`, `.bss$...`) are filtered."""
        return {name for name, _v, _s, st in self.symbols
                if st in (_EXTERNAL, _STATIC, _LABEL)
                and not name.startswith(".")}

    def code_names(self) -> set[str]:
        """Names defined in an executable section - the function authority."""
        out = set()
        for name, _v, sec, st in self.symbols:
            if st in (_EXTERNAL, _STATIC) and 1 <= sec <= len(self.section_chars) \
                    and self.section_chars[sec - 1] & IMAGE_SCN_CNT_CODE:
                out.add(name)
        return out

    def commons(self) -> dict[str, int]:
        """{name: size} for COFF COMMONs: section 0 with a NON-ZERO value (the
        size). Section 0 with value 0 is an ordinary undefined external."""
        return {name: value for name, value, sec, st in self.symbols
                if sec == 0 and st == _EXTERNAL and value}
