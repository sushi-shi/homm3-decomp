"""homm3.core.pe - the retail image, parsed once.

The PE section table is the authority for every address-space edge; nothing
in the tree hardcodes an image constant. Parsed lazily and cached per
process (the image never changes).
"""

from __future__ import annotations

import struct
from functools import lru_cache
from pathlib import Path

from homm3.core.paths import retail_exe


class Pe:
    def __init__(self, path: Path | str | None = None):
        self.path = Path(path or retail_exe())
        self.data = d = self.path.read_bytes()
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        optsz = struct.unpack_from("<H", d, pe + 20)[0]
        magic = struct.unpack_from("<H", d, pe + 24)[0]
        if magic != 0x10B:
            raise ValueError(f"{self.path}: not a PE32 image (magic 0x{magic:x})")
        self.image_base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        self.iat = struct.unpack_from("<II", d, pe + 24 + 96 + 12 * 8)
        self.sections: list[dict] = []
        for i in range(nsec):
            base = pe + 24 + optsz + i * 40
            name = d[base:base + 8].rstrip(b"\0").decode("latin-1")
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", d, base + 8)
            self.sections.append({"name": name, "va": va, "vsize": vsize,
                                  "rsize": rsize, "rptr": rptr})

    def section(self, name: str) -> dict:
        s = next((s for s in self.sections if s["name"] == name), None)
        if s is None:
            raise KeyError(f"{self.path}: no section {name}")
        return s

    def text_span(self) -> tuple[int, int]:
        """[lo, hi) of .text's VIRTUAL extent (what function extents cap at)."""
        t = self.section(".text")
        return t["va"], t["va"] + t["vsize"]

    def data_regions(self) -> dict[str, tuple[int, int]]:
        """The four data regions: .rdata's raw bytes, .data's raw
        (initialized) bytes, .data's loader-zero virtual tail (this image has
        no separate .bss header), and .idata - whose virtual tail the retail
        linker reused for late zero-fill globals."""
        rd, da = self.section(".rdata"), self.section(".data")
        regions = {"rdata": (rd["va"], rd["va"] + rd["rsize"]),
                "data": (da["va"], da["va"] + da["rsize"]),
                "bss": (da["va"] + da["rsize"], da["va"] + da["vsize"])}
        # HoMM3's imports are in .rdata; Gruntz has a separate .idata.
        if any(s["name"] == ".idata" for s in self.sections):
            it = self.section(".idata")
            regions["idata"] = (it["va"], it["va"] + max(it["vsize"], it["rsize"]))
        return regions

    def read(self, rva: int, size: int) -> bytes | None:
        """Bytes at rva; loader zero-fill (past a section's raw size) reads as
        ZEROS, never as the next section's file bytes; short reads are None."""
        for s in self.sections:
            if s["va"] <= rva and rva + size <= s["va"] + max(s["vsize"], s["rsize"]):
                raw_end = s["va"] + s["rsize"]
                if rva >= raw_end:
                    return bytes(size)
                stored = min(size, raw_end - rva)
                off = s["rptr"] + rva - s["va"]
                chunk = self.data[off:off + stored]
                if len(chunk) != stored:
                    return None
                return chunk + bytes(size - stored)
        return None


@lru_cache(maxsize=1)
def image() -> Pe:
    return Pe()
