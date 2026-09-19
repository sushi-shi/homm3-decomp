"""PE32 address/file layout, independent of the project's executable identity."""
from dataclasses import dataclass
import struct


@dataclass(frozen=True)
class Section:
    rva: int
    raw_size: int
    raw_offset: int


@dataclass(frozen=True)
class Layout:
    image_base: int
    sections: tuple[Section, ...]

    def base(self, segment: int) -> int:
        return self.image_base + self.sections[segment - 1].rva

    @classmethod
    def parse(cls, data: bytes):
        try:
            if data[:2] != b'MZ':
                raise ValueError('not a PE image')
            pe, = struct.unpack_from('<I', data, 0x3c)
            if data[pe:pe + 4] != b'PE\0\0':
                raise ValueError('missing PE signature')
            count, = struct.unpack_from('<H', data, pe + 6)
            optional_size, = struct.unpack_from('<H', data, pe + 20)
            optional = pe + 24
            magic, = struct.unpack_from('<H', data, optional)
            if magic != 0x10b:
                raise ValueError('expected PE32')
            base, = struct.unpack_from('<I', data, optional + 28)
            sections = []
            for i in range(count):
                entry = optional + optional_size + i * 40
                rva, size, raw = struct.unpack_from('<3I', data, entry + 12)
                if raw + size > len(data):
                    raise ValueError('section exceeds file extent')
                sections.append(Section(rva, size, raw))
            return cls(base, tuple(sections))
        except struct.error as exc:
            raise ValueError('truncated PE layout') from exc
