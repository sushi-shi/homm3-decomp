"""GCC 2.95 .eh_frame: one CIE per object, one FDE per compiled function.

The CIE sequence therefore partitions the linked program into its compiled
objects, and every FDE gives an exact function start and length, including
file-static functions that .dynsym does not export.
"""
from __future__ import annotations

from dataclasses import dataclass
import struct

from homm3.loki.elf import Elf


@dataclass(frozen=True)
class Fde:
    offset: int      # offset of the FDE in .eh_frame
    cie: int         # offset of its CIE (= the compiled object's frame block)
    start: int
    size: int


def frames(elf: Elf) -> tuple[list[int], list[Fde]]:
    """CIE offsets in link order and every FDE (absolute pointers, no augmentation)."""
    section = elf.section(".eh_frame")
    data = elf.bytes(section)
    cies, fdes = [], []
    offset = 0
    while offset + 4 <= len(data):
        (length,) = struct.unpack_from("<I", data, offset)
        if length == 0:
            break
        (pointer,) = struct.unpack_from("<I", data, offset + 4)
        if pointer == 0:
            augmentation = data[offset + 9:data.index(b"\0", offset + 9)]
            if augmentation not in (b"", b"eh"):
                raise ValueError(f"unsupported CIE augmentation {augmentation!r} at 0x{offset:x}")
            cies.append(offset)
        else:
            start, size = struct.unpack_from("<II", data, offset + 8)
            fdes.append(Fde(offset, offset + 4 - pointer, start, size))
        offset += 4 + length
    return cies, fdes
