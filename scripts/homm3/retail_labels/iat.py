"""homm3.retail_labels.iat - IAT slot claims from the retail import directory.

The one channel derived from retail bytes plus the pinned toolchain (no
gruntz analog - its providers are all committed tables): each IAT slot is a
4-byte data claim. Decoration proof comes from the VC6 import libraries'
archive symbol tables - the linker generation that built retail - so an
`__imp_` spelling is PLACEHOLDER-undecorated until a library proves it, and
a name whose libraries disagree stays unproven. Parse-only; mechanisms moved
unchanged from the pre-port homm3.build.labels.
"""

from __future__ import annotations

import struct
from pathlib import Path

from homm3.core import common
from homm3.retail_labels import Claim


def implib_decorations(libdir: Path) -> dict:
    """Import-lib PROOF for stdcall decoration: import-directory name ->
    full __imp_ symbol, read from the VC6 toolchain import libraries'
    archive symbol tables (the linker generation that built retail).
    A name whose libraries disagree on decoration stays unproven."""
    out, ambiguous = {}, set()
    for lib in sorted(libdir.iterdir()) if libdir.is_dir() else []:
        if lib.suffix.lower() != ".lib" or not lib.is_file():
            continue
        data = lib.read_bytes()
        if not data.startswith(b"!<arch>\n"):
            continue
        try:
            size = int(data[8 + 48:8 + 58].split()[0])
            count = int.from_bytes(data[68:72], "big")
        except (ValueError, IndexError):
            continue
        blob = data[72 + 4 * count:8 + 60 + size]
        for raw_sym in blob.split(b"\0")[:count]:
            sym = raw_sym.decode("latin-1")
            if not sym.startswith("__imp__") or "@" not in sym:
                continue
            key = sym[7:].rsplit("@", 1)[0]
            if out.setdefault(key, sym) != sym:
                ambiguous.add(key)
    for key in ambiguous:
        out.pop(key, None)
    return out


def iat_slots(exe_path: Path, libdir: Path) -> dict[int, tuple[str, str]]:
    """slot rva -> (__imp_ spelling, channel), from the import directory."""
    decorations = implib_decorations(libdir)
    data = exe_path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    osz = struct.unpack_from("<H", data, pe + 20)[0]
    sections = []
    for i in range(nsec):
        off = pe + 24 + osz + i * 40
        vs, va, rs, ro = struct.unpack_from("<4I", data, off + 8)
        sections.append((va, max(vs, rs), ro))

    def raw(rva):
        for va, span, ro in sections:
            if va <= rva < va + span:
                return ro + (rva - va)
        common.die(f"import walk: rva 0x{rva:x} in no section")

    imp_rva = struct.unpack_from("<II", data, pe + 24 + 96 + 1 * 8)[0]
    slots = {}
    off = raw(imp_rva)
    while True:
        ilt, _ts, _fc, name_rva, iat = struct.unpack_from("<IIIII",
                                                          data, off)
        if not (ilt or name_rva):
            break
        dll = data[raw(name_rva):raw(name_rva) + 64].split(b"\0")[0] \
            .decode("latin-1")
        entry = raw(ilt)
        index = 0
        while True:
            thunk = struct.unpack_from("<I", data, entry + index * 4)[0]
            if not thunk:
                break
            slot = iat + index * 4
            if thunk & 0x80000000:
                stem = dll.rsplit(".", 1)[0].lower()
                slots[slot] = (f"__imp__{stem}_ordinal_{thunk & 0xFFFF}",
                               "iat-ordinal")
            else:
                name = data[raw(thunk) + 2:raw(thunk) + 2 + 256] \
                    .split(b"\0")[0].decode("latin-1")
                proven = (None if name.startswith("?")
                          else decorations.get(name))
                if proven:
                    slots[slot] = (proven, "iat-implib")
                else:
                    prefix = "__imp_" if name.startswith("?") else "__imp__"
                    slots[slot] = (prefix + name, "iat-undecorated")
            index += 1
        off += 20
    return slots


def claims(exe_path: Path, libdir: Path) -> list[Claim]:
    """One 4-byte data Claim per IAT slot, sorted by slot rva."""
    exe = exe_path
    return [Claim(slot, name, "data", channel, 4, "", {})
            for slot, (name, channel) in sorted(iat_slots(exe, libdir).items())]
