"""homm3.vc6._toolchain - hash-gated access to the pinned compiler binaries.

Same discipline as core.common's retail-exe gate, applied to the RE
SUBJECTS: refuse to analyse anything but the exact pressing of CL/C1/C1XX/C2
we model. A wrong-hash binary is a hard abort, not a silent wrong answer -
the whole model is only valid against these bytes.

The binaries live under the pinned toolchain (build/homm3-toolchain-vc6-sp3,
or the MSVC_DIR/HOMM3_TOOLCHAIN override), unpacked by `homm3 init`; they are
gitignored. This module never copies them - it reads in place after gating.

`Binary` is a minimal PE reader: section table, rva<->file-offset mapping,
and the little-endian / C-string accessors the spec-table and atlas decoders
need. No capstone here (that is sema's job on demand); this is structural.
"""
from __future__ import annotations

import hashlib
import struct
from dataclasses import dataclass
from pathlib import Path

from homm3.core import cc_wrap
from homm3.vc6 import _common

# Pinned identities (sha256, size). Measured 2026-08-09 against the
# toolchain-vc6-sp3 release tarball. Versions: C1/C1XX 12.00.8472,
# C2/LINK 12.00.8447, CL driver stub 12.00.8168.
PINNED: dict[str, tuple[str, int]] = {
    "CL.EXE":   ("d9220fdfaf355c81f26e7b03c5dc628ec72ff81c99dc38ed98b1ad4bdbbb309c", 65536),
    "C1.DLL":   ("d1b25d84da1d3d0d460996ea298488cb183b857e1b90628147983114ac4ab55f", 675889),
    "C1XX.DLL": ("28c355499c2f8090910624734faff31fe719a69a8aac7dd44b2ce64addf96ee2", 1196083),
    "C2.DLL":   ("a0cc45f83fd0ed009aa4f43df5598105a1049ad9675de0b1089be7cce7d3c153", 737329),
    "LINK.EXE": ("9672e578fdfaa43bdb8e9c16071682988665cb90bba2904bf02f6a5576d8ffbc", 462901),
}


def bin_dir() -> Path:
    """The pinned toolchain's bin/ (reuses cc_wrap's MSVC_DIR resolution)."""
    return cc_wrap.msvc_dir() / "bin"


def resolve(name: str) -> Path:
    """Gated path to a pinned compiler binary (case-insensitive on disk)."""
    if name.upper() not in PINNED:
        _common.die(f"{name}: not a pinned compiler binary "
                    f"({', '.join(PINNED)})")
    path = cc_wrap.find_ci(bin_dir(), name)
    if path is None or not path.is_file():
        _common.die(f"{name} not found under {bin_dir()} - "
                    "run `homm3 init` inside `nix develop .#build`.")
    want_sha, want_size = PINNED[name.upper()]
    size = path.stat().st_size
    if size != want_size:
        _common.die(f"{path}: size {size} != pinned {want_size}")
    sha = hashlib.sha256(path.read_bytes()).hexdigest()
    if sha != want_sha:
        _common.die(f"{path}: sha256 {sha} != pinned {want_sha} - "
                    "the toolchain is not the pinned pressing")
    return path


@dataclass
class Section:
    name: str
    vaddr: int   # RVA (relative to image base)
    vsize: int
    raw: int     # file offset of the section's raw data
    rsize: int


class Binary:
    """A gated, in-memory PE image with rva<->offset mapping."""

    def __init__(self, name: str):
        self.name = name.upper()
        self.path = resolve(name)
        self.data = self.path.read_bytes()
        pe = struct.unpack_from("<I", self.data, 0x3c)[0]
        if self.data[pe:pe + 4] != b"PE\0\0":
            _common.die(f"{self.path}: no PE header")
        nsec = struct.unpack_from("<H", self.data, pe + 6)[0]
        opt = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.image_base = struct.unpack_from("<I", self.data, pe + 24 + 28)[0]
        self.sections: list[Section] = []
        off = pe + 24 + opt
        for _ in range(nsec):
            nm = self.data[off:off + 8].rstrip(b"\0").decode("latin1")
            vsize, va, rsize, raw = struct.unpack_from("<IIII", self.data, off + 8)
            self.sections.append(Section(nm, va, vsize, raw, rsize))
            off += 40

    # --- address arithmetic -------------------------------------------------
    def sec_of_rva(self, rva: int) -> Section | None:
        for s in self.sections:
            if s.vaddr <= rva < s.vaddr + max(s.vsize, s.rsize):
                return s
        return None

    def rva_to_off(self, rva: int) -> int | None:
        s = self.sec_of_rva(rva)
        if s is None or rva - s.vaddr >= s.rsize:
            return None
        return s.raw + (rva - s.vaddr)

    def off_to_rva(self, off: int) -> int | None:
        for s in self.sections:
            if s.raw <= off < s.raw + s.rsize:
                return s.vaddr + (off - s.raw)
        return None

    def va_to_off(self, va: int) -> int | None:
        return self.rva_to_off(va - self.image_base)

    # --- accessors (file offsets) ------------------------------------------
    def u32(self, off: int) -> int:
        return struct.unpack_from("<I", self.data, off)[0]

    def u16(self, off: int) -> int:
        return struct.unpack_from("<H", self.data, off)[0]

    def cstr_at_off(self, off: int, limit: int = 4096) -> str:
        end = self.data.index(b"\0", off, off + limit)
        return self.data[off:end].decode("latin1")

    def cstr_at_va(self, va: int) -> str | None:
        off = self.va_to_off(va)
        return None if off is None else self.cstr_at_off(off)

    def is_va(self, v: int) -> bool:
        """Does this dword look like a VA pointing into our own image?"""
        return self.va_to_off(v) is not None
