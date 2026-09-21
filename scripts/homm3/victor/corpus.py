"""Inventory installed resources without conflating archive bitmaps with PCX.

LOD layout follows tools/homm3-lod; bitmap classification follows
homm3-resource. Keep every archive occurrence, including overridden resources.
"""
from __future__ import annotations

import hashlib
from pathlib import Path
import struct
import zlib


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def lod_members(data: bytes):
    if len(data) < 92 or data[:3] != b'LOD':
        raise ValueError('invalid LOD header')
    count, = struct.unpack_from('<I', data, 8)
    end = 92 + count * 32
    if end > len(data):
        raise ValueError('truncated LOD directory')
    for index in range(count):
        record = 92 + index * 32
        name = data[record:record + 16].split(b'\0', 1)[0].decode('ascii')
        offset, size, attributes, compressed = struct.unpack_from('<4I', data, record + 16)
        stored = compressed or size
        if offset < end or offset + stored > len(data):
            raise ValueError(f'out-of-bounds LOD member {name!r}')
        raw = data[offset:offset + stored]
        if compressed:
            inflater = zlib.decompressobj()
            raw = inflater.decompress(raw, size + 1)
            if not inflater.eof or inflater.unused_data or inflater.unconsumed_tail:
                raise ValueError(f'invalid compressed LOD member {name!r}')
        if len(raw) != size:
            raise ValueError(f'wrong LOD member size {name!r}')
        yield index, name, raw


def classify(data: bytes) -> dict:
    # Check the full archive wrapper before interpreting a coincidental 0x0a.
    if len(data) >= 12:
        size, width, height = struct.unpack_from('<3I', data)
        if width and height:
            if size == width * height and len(data) == 12 + size + 768:
                return dict(format='h3-indexed8', width=width, height=height)
            if size == width * height * 3 and len(data) == 12 + size:
                return dict(format='h3-packed24', width=width, height=height)
    if len(data) >= 128 and data[0] == 10:
        x0, y0, x1, y1 = struct.unpack_from('<4H', data, 4)
        return dict(format='pcx', width=x1 - x0 + 1, height=y1 - y0 + 1,
                    encoding=data[2], depth=data[3], planes=data[65])
    return dict(format='unknown')


def inventory(game: Path, out: Path) -> dict:
    """Extract PCX-named or PCX-signature resources and retain content hashes."""
    assets = out / 'assets'
    assets.mkdir(parents=True, exist_ok=False)
    report = dict(gameDirectory=str(game.resolve()), files=[], images=[], counts={})
    for path in sorted(game.rglob('*')):
        if not path.is_file():
            continue
        relative = path.relative_to(game).as_posix()
        # Record every file so the coverage denominator is explicit. Only LOD
        # containers and image candidates need full reads during this stage.
        with path.open('rb') as stream:
            prefix = stream.read(128)
        if path.suffix.lower() == '.lod':
            data = path.read_bytes()
            report['files'].append(dict(path=relative, size=len(data), sha256=sha256(data), kind='lod'))
            members = lod_members(data)
        elif path.suffix.lower() == '.pcx' or (len(prefix) == 128 and prefix[:1] == b'\x0a'):
            data = path.read_bytes()
            report['files'].append(dict(path=relative, size=len(data), sha256=sha256(data), kind='loose-image'))
            members = [(None, path.name, data)]
        else:
            report['files'].append(dict(path=relative, size=path.stat().st_size, kind='other'))
            continue
        for index, name, raw in members:
            if not name.lower().endswith('.pcx') and not (len(raw) >= 128 and raw[0] == 10):
                continue
            info = classify(raw)
            digest = sha256(raw)
            target = assets / (digest + '.bin')
            if not target.exists():
                target.write_bytes(raw)
            report['images'].append(dict(container=relative, index=index, name=name,
                                         sha256=digest, size=len(raw), asset=str(target.relative_to(out)), **info))
            kind = info['format']
            report['counts'][kind] = report['counts'].get(kind, 0) + 1
    return report


def encode_rle(row: bytes) -> bytes:
    encoded = bytearray()
    start = 0
    while start < len(row):
        end = start + 1
        while end < len(row) and end - start < 63 and row[end] == row[start]:
            end += 1
        count = end - start
        value = row[start]
        if count != 1 or value >= 192:
            encoded.extend((192 | count, value))
        else:
            encoded.append(value)
        start = end
    return bytes(encoded)


def bitmap_to_pcx(data: bytes) -> bytes:
    """Losslessly wrap archive pixels as version-5, RLE-encoded PCX.

    Packed archive rows are RGB (Bitmap24Bit::draw reads red first); PCX
    stores each row as separate R, G, B planes. Indexed palettes are RGB.
    Padding is explicit zero input and each plane has an even byte stride.
    """
    info = classify(data)
    if info['format'] not in ('h3-indexed8', 'h3-packed24'):
        raise ValueError('expected an archive bitmap')
    width, height = info['width'], info['height']
    stride = (width + 1) & ~1
    if stride > 65535 or height > 65536:
        raise ValueError('bitmap exceeds PCX 16-bit dimensions')
    planes = 1 if info['format'] == 'h3-indexed8' else 3
    header = bytearray(128)
    header[:4] = bytes((10, 5, 1, 8))
    struct.pack_into('<4H', header, 4, 0, 0, width - 1, height - 1)
    struct.pack_into('<2H', header, 12, 72, 72)
    header[65] = planes
    struct.pack_into('<2H', header, 66, stride, 1)
    encoded = bytearray(header)
    padding = bytes(stride - width)
    for y in range(height):
        row = data[12 + y * width * planes:12 + (y + 1) * width * planes]
        for channel in range(planes):
            encoded.extend(encode_rle(row[channel::planes] + padding))
    if planes == 1:
        encoded.append(12)
        encoded.extend(data[-768:])
    return bytes(encoded)
