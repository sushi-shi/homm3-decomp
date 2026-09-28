"""homm3.delink.coffx - full COFF topology for the delink oracles.

core/coff.py answers the name-authority question and nothing else; the delink
side additionally needs section payloads, per-section members, relocations and
COMDAT metadata (the string/vtable/RTTI oracles and the candidate section
manifest). Ported from the old tree's coff_oracle._Coff.
"""

from __future__ import annotations

import struct
from pathlib import Path


def section_alignment(characteristics: int) -> int:
    """The IMAGE_SCN_ALIGN_* nibble decoded to a byte count (1 when unset)."""
    a = (characteristics & 0x00F00000) >> 20
    return 1 << (a - 1) if a else 1


class Obj:
    """Read-only i386 COFF object: symbols, section bytes, relocations.

    `section_table` carries the per-section topology (name, characteristics,
    COMDAT selection/associate, the aux `scnlen` that is authoritative for
    .bss) that the candidate section manifest is derived from.
    """

    def __init__(self, path: Path | str):
        self.buf = b = Path(path).read_bytes()
        if struct.unpack_from("<H", b, 0)[0] != 0x14C:
            raise ValueError(f"{path}: not an i386 COFF")
        self.nsec = struct.unpack_from("<H", b, 2)[0]
        self.symptr = struct.unpack_from("<I", b, 8)[0]
        self.nsym = struct.unpack_from("<I", b, 12)[0]
        opt = struct.unpack_from("<H", b, 16)[0]
        self.strtab_off = self.symptr + self.nsym * 18
        self.sections: list[tuple[int, int]] = []       # (rawptr, rawsize)
        self.section_table: list[dict] = []
        for i in range(self.nsec):
            o = 20 + opt + i * 40
            name = b[o:o + 8].split(b"\0")[0].decode("latin1")
            if name.startswith("/"):        # long name -> string table offset
                off = int(name[1:])
                end = b.index(b"\0", self.strtab_off + off)
                name = b[self.strtab_off + off:end].decode("latin1")
            vsize, _vaddr, rawsize, rawptr = struct.unpack_from("<IIII", b, o + 8)
            relptr, nrel = struct.unpack_from("<IxxxxH", b, o + 24)
            chars = struct.unpack_from("<I", b, o + 36)[0]
            self.sections.append((rawptr, rawsize))
            self.section_table.append({
                "index": i + 1, "name": name, "characteristics": chars,
                "alignment": section_alignment(chars),
                "size": rawsize or vsize, "comdat": 0, "assoc": 0,
                "reloc_offset": relptr, "reloc_count": nrel,
            })
        # The section-definition aux record carries the authoritative length
        # (.bss has rawsize 0) plus the COMDAT selection/associate.
        for idx, _value, secnum in self.iter_symbols():
            base = self.symptr + idx * 18
            scl, naux = struct.unpack_from("<BB", b, base + 16)
            if scl != 3 or not naux or not (1 <= secnum <= self.nsec):
                continue
            a = self.symptr + (idx + 1) * 18
            scnlen, _nr, _nl, _ck, assoc, sel = struct.unpack_from("<IHHIHB", b, a)
            sec = self.section_table[secnum - 1]
            sec["size"] = scnlen
            sec["comdat"] = sel
            # The aux `Number` field is meaningful as an associated-section
            # ordinal only for IMAGE_COMDAT_SELECT_ASSOCIATIVE.  cl 5 writes
            # the section's own ordinal there for ordinary `Any` COMDATs; do
            # not serialize that bookkeeping value as an association.
            sec["assoc"] = assoc if sel == 5 else 0

    def sym_name(self, idx: int) -> str:
        base = self.symptr + idx * 18
        if struct.unpack_from("<I", self.buf, base)[0] == 0:
            off = struct.unpack_from("<I", self.buf, base + 4)[0]
            end = self.buf.index(b"\0", self.strtab_off + off)
            return self.buf[self.strtab_off + off:end].decode("latin1")
        return self.buf[base:base + 8].split(b"\0")[0].decode("latin1")

    def iter_symbols(self):
        i = 0
        while i < self.nsym:
            base = self.symptr + i * 18
            value, secnum, _typ, _scl, naux = struct.unpack_from(
                "<IhHBB", self.buf, base + 8)
            yield i, value, secnum
            i += 1 + naux

    def _section_index(self) -> dict[int, list[tuple[int, str, int]]]:
        """{secnum: sorted [(offset, name, storage_class)]} for class 2/3
        symbols, built in one pass: per-section queries run once per section
        of every object, and rescanning the symbol table each time was
        quadratic."""
        index = getattr(self, "_by_section", None)
        if index is None:
            index = {}
            for idx, value, sn in self.iter_symbols():
                scl = self.buf[self.symptr + idx * 18 + 16]
                if scl in (2, 3):
                    index.setdefault(sn, []).append((value, self.sym_name(idx), scl))
            for rows in index.values():
                rows.sort()
            self._by_section = index
        return index

    def defined_symbols(self, secnum: int):
        """[(offset, name)] of the class-EXTERNAL symbols defined in one section."""
        return [(value, name) for value, name, scl in
                self._section_index().get(secnum, ()) if scl == 2]

    def section_members(self, secnum: int):
        """[(offset, name, storage_class)] of EVERY datum defined in one section.

        Keeps the class-STATIC (`scl == 3`) members too - cl gives a
        function-local `static` a file-scope `$S<id>` symbol with that class.
        The section-definition symbol itself is not a datum and is dropped.
        """
        own = self.section_table[secnum - 1]["name"]
        return [row for row in self._section_index().get(secnum, ())
                if row[1] != own]

    def section_payload(self, secnum: int) -> bytes:
        """The raw bytes of one section (b"" when it has none, e.g. .bss)."""
        if not (1 <= secnum <= self.nsec):
            return b""
        rawptr, rawsize = self.sections[secnum - 1]
        return self.buf[rawptr:rawptr + rawsize] if rawptr else b""

    def relocations(self, secnum: int) -> dict[int, str]:
        """{site -> referent name} for one section's COFF relocations.

        `IMAGE_SCN_LNK_NRELOC_OVFL` moves the real count into a leading
        pseudo-record, which is honoured.
        """
        if not (1 <= secnum <= self.nsec):
            return {}
        sec = self.section_table[secnum - 1]
        ptr, count, first = sec["reloc_offset"], sec["reloc_count"], 0
        if not ptr:
            return {}
        if sec["characteristics"] & 0x01000000 and count == 0xFFFF:
            count = struct.unpack_from("<I", self.buf, ptr)[0]
            first = 1
        out = {}
        for i in range(first, count):
            site, idx, _typ = struct.unpack_from("<IIH", self.buf, ptr + i * 10)
            out[site] = self.sym_name(idx)
        return out

    def typed_relocations(self, secnum: int) -> dict[int, tuple[str, int]]:
        """{site -> (referent name, type)} - the DIR32-filtering callers' shape."""
        if not (1 <= secnum <= self.nsec):
            return {}
        sec = self.section_table[secnum - 1]
        ptr, count, first = sec["reloc_offset"], sec["reloc_count"], 0
        if not ptr:
            return {}
        if sec["characteristics"] & 0x01000000 and count == 0xFFFF:
            count = struct.unpack_from("<I", self.buf, ptr)[0]
            first = 1
        out = {}
        for i in range(first, count):
            site, idx, typ = struct.unpack_from("<IIH", self.buf, ptr + i * 10)
            out[site] = (self.sym_name(idx), typ)
        return out

    def cstring(self, secnum: int, value: int, limit: int = 512) -> bytes | None:
        if secnum < 1 or secnum > self.nsec:
            return None
        rawptr, rawsize = self.sections[secnum - 1]
        if not rawptr:
            return None
        start = rawptr + value
        cap = min(rawptr + rawsize, start + limit)
        if not (rawptr <= start < rawptr + rawsize):
            return None
        end = start
        while end < cap and self.buf[end] != 0:
            end += 1
        return bytes(self.buf[start:end]) if end < cap else None


def objects(base_dir: Path) -> list[tuple[str, Obj]]:
    """[(unit stem, Obj)] for every parseable base object, sorted by stem."""
    out = []
    for path in sorted(Path(base_dir).glob("*.obj")):
        try:
            out.append((path.stem, Obj(path)))
        except (ValueError, OSError, struct.error):
            continue
    return out


def build_string_map(base_dir: Path) -> dict[bytes, str]:
    """{string bytes (sans NUL) -> ??_C@... name} from every base object."""
    out: dict[bytes, str] = {}
    for _stem, c in objects(base_dir):
        for idx, value, secnum in c.iter_symbols():
            name = c.sym_name(idx)
            if name.startswith("??_C@") and secnum >= 1:
                cs = c.cstring(secnum, value)
                if cs is not None:
                    out.setdefault(cs, name)
    return out
