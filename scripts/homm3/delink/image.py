"""homm3.delink.image - retail-image facts core/pe does not carry.

The delink oracles need the PE's base-relocation directory (the exact DIR32
address-operand sites), raw-offset reads, C strings, import-directory slots
and the initialized-vs-loader-zero storage classification with its
FileAlignment ambiguity band. Ported from the old tree's core/pe.py +
core/data_audit.py; layered here because only delink consumes them.
"""

from __future__ import annotations

import bisect
import struct
from functools import lru_cache
from pathlib import Path

from homm3.core.pe import Pe, image


class Image:
    """One retail image + the delink-side derived views, cached."""

    def __init__(self, pe: Pe | None = None):
        self.pe = pe or image()
        self.data = self.pe.data
        self.image_base = self.pe.image_base
        d = self.data
        pe_off = struct.unpack_from("<I", d, 0x3C)[0]
        self._opt = pe_off + 24
        self.file_alignment = struct.unpack_from("<I", d, self._opt + 36)[0]
        self._reloc_sites: list[int] | None = None

    # --- addressing ---------------------------------------------------------
    def off(self, rva: int) -> int | None:
        """File offset of `rva`, or None if unmapped OR past the section's raw
        data (a virtual-only tail has no file bytes)."""
        for s in self.pe.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                o = rva - s["va"] + s["rptr"]
                return o if o < s["rptr"] + s["rsize"] else None
        return None

    def sec_name(self, rva: int) -> str | None:
        for s in self.pe.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                return s["name"]
        return None

    def u32(self, rva: int) -> int | None:
        o = self.off(rva)
        return struct.unpack_from("<I", self.data, o)[0] if o is not None else None

    def cstr(self, rva: int, n: int = 512) -> str | None:
        o = self.off(rva)
        if o is None:
            return None
        end = self.data.find(b"\0", o, o + n)
        return self.data[o:end].decode("latin1") if end != -1 else None

    def cstring(self, rva: int, limit: int = 512) -> bytes | None:
        """NUL-terminated raw bytes at `rva`, bounded to the section's RAW
        extent (the string-pool oracle's read - matches the old Exe.cstring)."""
        for s in self.pe.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                off = s["rptr"] + (rva - s["va"])
                cap = min(s["rptr"] + s["rsize"], off + limit)
                end = off
                while end < cap and self.data[end] != 0:
                    end += 1
                return self.data[off:end] if end < cap else None
        return None

    def payload(self, rva: int, n: int) -> bytes:
        """Retail's bytes at [rva, rva+n); a virtual-only tail reads as zero."""
        out = bytearray()
        for i in range(n):
            o = self.off(rva + i)
            out.append(0 if o is None else self.data[o])
        return bytes(out)

    # --- base relocations (the real DIR32 address-operand sites) ------------
    @property
    def reloc_sites(self) -> list[int]:
        """Sorted RVAs of every IMAGE_REL_BASED_HIGHLOW site (data dir 5)."""
        if self._reloc_sites is None:
            d = self.data
            rva = struct.unpack_from("<I", d, self._opt + 96 + 5 * 8)[0]
            sz = struct.unpack_from("<I", d, self._opt + 96 + 5 * 8 + 4)[0]
            sites = []
            if rva:
                base = self.off(rva)
                p, end = base, base + sz
                while p < end:
                    page, blk = struct.unpack_from("<II", d, p)
                    if blk == 0:
                        break
                    for i in range((blk - 8) // 2):
                        ent = struct.unpack_from("<H", d, p + 8 + i * 2)[0]
                        if ent >> 12 == 3:
                            sites.append(page + (ent & 0xFFF))
                    p += blk
            else:
                # The pinned Complete image has RELOCS_STRIPPED. Only the
                # reviewed operand inventory supplies the missing HIGHLOW
                # sites; numeric pointer-looking words are not admitted here.
                from homm3.core.paths import RETAIL
                from homm3.core.tsv import read
                sites = [int(row["site_rva"], 0)
                         for row in read(RETAIL / "relocs.tsv")[2]]
                for site in sites:
                    if self.off(site) is None or self.off(site + 3) is None:
                        raise ValueError(f"reviewed relocation outside raw image: {site:#x}")
            sites.sort()
            self._reloc_sites = sites
        return self._reloc_sites

    def relocs_in(self, lo: int, hi: int) -> list[int]:
        """HIGHLOW sites inside [lo, hi)."""
        sites = self.reloc_sites
        i, j = bisect.bisect_left(sites, lo), bisect.bisect_left(sites, hi)
        return sites[i:j]

    # --- imports -------------------------------------------------------------
    def import_slots(self) -> list[tuple[int, str | None, str, int | None]]:
        """[(iat_slot_rva, name_or_None, dll, ordinal_or_None)] from the PE."""
        d = self.data
        imp_rva = struct.unpack_from("<I", d, self._opt + 96 + 1 * 8)[0]
        if not imp_rva:
            return []

        def raw(rva: int) -> int:
            o = self.off(rva)
            if o is None:
                raise ValueError(f"RVA 0x{rva:x} outside PE raw sections")
            return o

        def cstr(off: int) -> str:
            return d[off:d.index(0, off)].decode("ascii", "replace")

        out = []
        p = raw(imp_rva)
        while True:
            lookup, ts, fw, name_rva, addr_rva = struct.unpack_from("<IIIII", d, p)
            if not any((lookup, ts, fw, name_rva, addr_rva)):
                break
            dll = cstr(raw(name_rva))
            thunk = raw(lookup or addr_rva)
            i = 0
            while True:
                v = struct.unpack_from("<I", d, thunk + i * 4)[0]
                if not v:
                    break
                slot = addr_rva + i * 4
                if v & 0x80000000:
                    out.append((slot, None, dll, v & 0xFFFF))
                else:
                    out.append((slot, cstr(raw(v) + 2), dll, None))
                i += 1
            p += 20
        return out

    # --- storage classification ----------------------------------------------
    def _emitted_content_floor(self, sec: dict) -> int:
        """Largest section offset PROVABLY inside emitted initialized content.

        `SizeOfRawData` is `round_up(E, FileAlignment)` for the true end E of
        the linker's initialized content, so E > raw_size - FileAlignment.
        """
        return max(0, sec["rsize"] - self.file_alignment)

    def _zero_to_raw_edge(self, sec: dict, offset: int) -> bool:
        """Is [offset, raw_size) all zero? Only then can it be padding."""
        start = sec["rptr"] + offset
        end = sec["rptr"] + sec["rsize"]
        return not any(self.data[start:end])

    def classify_storage(self, rva: int) -> str:
        """'rdata' | 'data-initialized' | 'data-unprovable-tail' |
        'data-loader-zero-tail' | 'other-section' | 'outside-image'.

        The unprovable tail is the <FileAlignment all-zero run at .data's raw
        edge, where alignment padding and a zero-valued global are
        byte-identical (fail-closed: callers must not enrol it bare)."""
        rd = self.pe.section(".rdata")
        if rd["va"] <= rva < rd["va"] + rd["vsize"]:
            return "rdata"
        da = self.pe.section(".data")
        if da["va"] <= rva < da["va"] + da["vsize"]:
            offset = rva - da["va"]
            if offset >= da["rsize"]:
                return "data-loader-zero-tail"
            if offset >= self._emitted_content_floor(da) \
                    and self._zero_to_raw_edge(da, offset):
                return "data-unprovable-tail"
            return "data-initialized"
        for s in self.pe.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                return "other-section"
        return "outside-image"


@lru_cache(maxsize=1)
def retail(path: str | None = None) -> Image:
    return Image(Pe(path) if path else image())


def sections_of(path: Path | str | None = None) -> dict[str, tuple[int, int]]:
    """{name: (base, end)} VIRTUAL bounds for .text/.rdata/.data/.idata."""
    pe = Pe(path) if path else image()
    out = {}
    for name in (".text", ".rdata", ".data", ".idata"):
        try:
            s = pe.section(name)
        except KeyError:
            out[name] = (0, 0)
            continue
        out[name] = (s["va"], s["va"] + s["vsize"])
    return out
