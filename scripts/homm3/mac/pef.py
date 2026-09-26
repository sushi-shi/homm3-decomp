"""Bounded reader for the pinned PowerPC PEF's section-relative bytes."""
from __future__ import annotations

from dataclasses import dataclass
import struct


class PEFError(ValueError):
    pass


@dataclass(frozen=True)
class Section:
    index: int
    kind: int
    file_offset: int
    packed_size: int
    unpacked_size: int
    total_size: int
    default_address: int = 0


def unpack_data(packed: bytes, size: int) -> bytes:
    """Decode PEF's five pattern-data operations, checking every input/output span."""
    pos = 0
    result = bytearray()

    def take(count):
        nonlocal pos
        if count < 0 or pos + count > len(packed):
            raise PEFError("truncated PEF packed data")
        data = packed[pos:pos + count]
        pos += count
        return data

    def number():
        value = 0
        for _ in range(5):
            byte = take(1)[0]
            value = (value << 7) | (byte & 0x7f)
            if value > 0xffffffff:
                break
            if not byte & 0x80:
                return value
        raise PEFError("invalid PEF packed-data count")

    def room(count):
        if count < 0 or len(result) + count > size:
            raise PEFError("PEF packed data exceeds unpacked size")

    while pos < len(packed):
        byte = take(1)[0]
        opcode, count = byte >> 5, byte & 31
        if not count:
            count = number()
        if opcode in (0, 1):
            room(count)
            result.extend(bytes(count) if opcode == 0 else take(count))
        elif opcode == 2:
            repeats = number() + 1
            room(count * repeats)
            result.extend(take(count) * repeats)
        elif opcode in (3, 4):
            custom, repeats = number(), number()
            room(count * (repeats + 1) + custom * repeats)
            common = take(count) if opcode == 3 else bytes(count)
            # Read all unique bytes before iterating, including zero-length runs.
            unique = take(custom * repeats)
            if not count and not custom:
                continue
            for index in range(repeats):
                result.extend(common)
                result.extend(unique[index * custom:(index + 1) * custom])
            result.extend(common)
        else:
            raise PEFError(f"unknown PEF packed-data opcode {opcode}")
    if len(result) != size:
        raise PEFError(f"PEF unpacked size mismatch: {len(result)} != {size}")
    return bytes(result)


class PEF:
    def __init__(self, data: bytes):
        if len(data) < 40 or data[:12] != b"Joy!peffpwpc":
            raise PEFError("expected a PowerPC PEF container")
        version, timestamp = struct.unpack_from(">II", data, 12)
        if version != 1:
            raise PEFError(f"unsupported PEF version {version}")
        count, instantiated = struct.unpack_from(">HH", data, 32)
        if count == 0 or instantiated > count or 40 + 28 * count > len(data):
            raise PEFError("invalid PEF section table")
        self.data = data
        self.timestamp = timestamp
        self.instantiated = instantiated
        self.sections = []
        self._contents: dict[int, bytes] = {}
        for index in range(count):
            start = 40 + index * 28
            _, default, total, unpacked, packed, file_offset = struct.unpack_from(">iIIIII", data, start)
            kind = data[start + 24]
            # Loader sections store metadata, not instantiated memory; their
            # total/unpacked size fields can both be zero with packed bytes.
            if ((kind != 4 and (unpacked > total or (kind != 2 and packed != unpacked)))
                    or file_offset + packed > len(data)):
                raise PEFError(f"section {index} exceeds the PEF container")
            self.sections.append(Section(index, kind, file_offset, packed, unpacked, total, default))

    def section(self, index: int) -> Section:
        if not 0 <= index < len(self.sections):
            raise PEFError(f"missing section {index}")
        return self.sections[index]

    def contents(self, index: int) -> bytes:
        """Initialized section bytes; BSS is supplied only by read()."""
        section = self.section(index)
        if index not in self._contents:
            data = self.data[section.file_offset:section.file_offset + section.packed_size]
            self._contents[index] = (unpack_data(data, section.unpacked_size)
                                     if section.kind == 2 else data)
        return self._contents[index]

    def read(self, index: int, offset: int, size: int) -> bytes:
        section = self.section(index)
        if index >= self.instantiated or offset < 0 or size < 0 or offset + size > section.total_size:
            raise PEFError(f"invalid memory span in section {index}: {offset:#x}+{size:#x}")
        data = self.contents(index)[offset:offset + size]
        return data + bytes(size - len(data))

    def code(self, section_index: int, offset: int, size: int) -> bytes:
        if section_index < 0 or section_index >= len(self.sections):
            raise PEFError(f"missing section {section_index}")
        section = self.sections[section_index]
        if section.kind != 0:
            raise PEFError(f"section {section_index} is not executable code")
        if offset < 0 or offset % 4 or size <= 0 or size % 4 or offset + size > section.packed_size:
            raise PEFError(f"invalid code span in section {section_index}")
        start = section.file_offset + offset
        return self.data[start:start + size]
