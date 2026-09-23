"""Complete PE32 file/image partitions for accounting, including unmapped bytes."""
from __future__ import annotations

from dataclasses import dataclass
import struct


@dataclass(frozen=True)
class Region:
    domain: str
    start: int
    end: int
    section: str
    storage: str
    file_offset: int | None
    rva: int | None

    @property
    def size(self):
        return self.end - self.start


@dataclass(frozen=True)
class Section:
    name: str
    rva: int
    virtual_size: int
    raw_offset: int
    raw_size: int
    flags: int

    @property
    def mapped_size(self):
        return max(self.virtual_size, self.raw_size)


def fill_regions(domain, size, regions):
    """Exactly partition [0,size), rejecting any conflicting backing mapping."""
    cursor, result = 0, []
    for region in sorted(regions, key=lambda r: r.start):
        if region.start < cursor or region.end > size or region.size <= 0:
            raise ValueError(f'{domain}: overlapping/out-of-bounds PE region {region}')
        if cursor < region.start:
            result.append(Region(domain, cursor, region.start, '', 'gap',
                                 cursor if domain == 'file' else None,
                                 cursor if domain == 'image' else None))
        result.append(region)
        cursor = region.end
    if cursor < size:
        result.append(Region(domain, cursor, size, '', 'overlay' if domain == 'file' else 'gap',
                             cursor if domain == 'file' else None,
                             cursor if domain == 'image' else None))
    return result


class Layout:
    def __init__(self, data):
        self.data = data
        try:
            self._parse()
        except struct.error as exc:
            raise ValueError('truncated PE layout') from exc

    def _parse(self):
        data = self.data
        if data[:2] != b'MZ':
            raise ValueError('expected DOS signature')
        self.pe = struct.unpack_from('<I', data, 0x3c)[0]
        if self.pe < 64 or data[self.pe:self.pe + 4] != b'PE\0\0':
            raise ValueError('invalid PE signature or DOS-header overlap')
        count = struct.unpack_from('<H', data, self.pe + 6)[0]
        optional_size = struct.unpack_from('<H', data, self.pe + 20)[0]
        self.optional = self.pe + 24
        if optional_size < 96 or struct.unpack_from('<H', data, self.optional)[0] != 0x10b:
            raise ValueError('expected complete PE32 optional header')
        self.base = struct.unpack_from('<I', data, self.optional + 28)[0]
        self.image_size, self.header_size = struct.unpack_from('<II', data, self.optional + 56)
        if not 0 < self.header_size <= min(len(data), self.image_size):
            raise ValueError('invalid PE header extent')
        self.section_table = self.optional + optional_size
        self.section_table_end = self.section_table + 40 * count
        if self.section_table_end > self.header_size:
            raise ValueError('section table extends beyond SizeOfHeaders')
        directory_count = struct.unpack_from('<I', data, self.optional + 92)[0]
        if directory_count > (optional_size - 96) // 8:
            raise ValueError('data directories extend beyond optional header')
        self.directories = [struct.unpack_from('<II', data, self.optional + 96 + 8 * i)
                            for i in range(directory_count)]
        self.sections = []
        file_regions = [Region('file', 0, self.header_size, 'headers', 'headers', 0, 0)]
        image_regions = [Region('image', 0, self.header_size, 'headers', 'headers', 0, 0)]
        for i in range(count):
            offset = self.section_table + 40 * i
            name = data[offset:offset + 8].rstrip(b'\0').decode('latin1')
            virtual, rva, raw_size, disk = struct.unpack_from('<4I', data, offset + 8)
            flags = struct.unpack_from('<I', data, offset + 36)[0]
            section = Section(name, rva, virtual, disk, raw_size, flags)
            self.sections.append(section)
            if raw_size:
                if disk + raw_size > len(data):
                    raise ValueError(f'{name}: raw section outside file')
                # Preserve the boundary between virtual content and excess raw bytes.
                cuts = sorted({0, raw_size, min(virtual, raw_size) if virtual else raw_size})
                for lo, hi in zip(cuts, cuts[1:]):
                    storage = 'raw-tail' if virtual and lo >= virtual else 'initialized'
                    file_regions.append(Region('file', disk + lo, disk + hi, name,
                                               storage, disk + lo, rva + lo))
                    image_regions.append(Region('image', rva + lo, rva + hi, name,
                                                storage, disk + lo, rva + lo))
            if virtual > raw_size:
                image_regions.append(Region('image', rva + raw_size, rva + virtual,
                                            name, 'zero-fill', None, rva + raw_size))
        self.file_regions = fill_regions('file', len(data), file_regions)
        self.image_regions = fill_regions('image', self.image_size, image_regions)

    def directory(self, index):
        return self.directories[index] if index < len(self.directories) else (0, 0)

    def raw(self, rva, size):
        if size < 0:
            raise ValueError('negative PE read extent')
        if 0 <= rva and rva + size <= self.header_size:
            return rva
        for s in self.sections:
            if s.rva <= rva and rva + size <= s.rva + s.raw_size:
                return s.raw_offset + rva - s.rva
        raise ValueError(f'RVA 0x{rva:x}+0x{size:x} is not wholly file-backed')

    def read(self, rva, size):
        start = self.raw(rva, size)
        return self.data[start:start + size]

    def unpack(self, fmt, rva):
        return struct.unpack(fmt, self.read(rva, struct.calcsize(fmt)))

    def cstring_size(self, rva):
        start = self.raw(rva, 1)
        region = next(r for r in self.file_regions if r.start <= start < r.end)
        end = self.data.find(b'\0', start, region.end)
        if end < 0:
            raise ValueError(f'unterminated string at RVA 0x{rva:x}')
        return end - start + 1
