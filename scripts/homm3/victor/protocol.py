"""Strict parsing for the versioned Victor output stream."""
import struct
from .corpus import sha256


def decode(raw: bytes) -> dict:
    cursor = 0
    def take(size):
        nonlocal cursor
        if size < 0 or cursor + size > len(raw):
            raise ValueError('truncated Victor output')
        result = raw[cursor:cursor + size]
        cursor += size
        return result
    def words(count=1):
        return list(struct.unpack('<' + 'I' * count, take(count * 4)))
    def descriptor():
        result = words(11)
        if result[8]:
            result += words(2)
        return result
    def snapshot():
        fields = descriptor()
        header = take(40)
        palette = take(words()[0])
        pixels = take(words()[0])
        if len(palette) != fields[6] * 4 or len(pixels) != struct.unpack_from('<I', header, 20)[0]:
            raise ValueError('inconsistent snapshot sizes')
        return dict(descriptor=fields, header=header.hex(), paletteSha256=sha256(palette),
                    pixelsSha256=sha256(pixels), pixels=pixels, palette=palette)
    if words()[0] != 0x31544356:
        raise ValueError('unknown Victor output version')
    status = words()[0]
    result = dict(infoStatus=status, info=words(8))
    if status == 0:
        status = words()[0]
        result.update(allocateStatus=status, allocatedDescriptor=descriptor())
        if status == 0:
            result['allocated'] = snapshot()
            result['loadStatus'] = words()[0]
            result['loaded'] = snapshot()
            result['flipStatus'] = words()[0]
            result['flipped'] = snapshot()
            result['releasedDescriptor'] = descriptor()
            if any(result['releasedDescriptor']):
                raise ValueError('descriptor not cleared after freeimage')
    if cursor != len(raw):
        raise ValueError('trailing Victor output')
    return result


def verify_pixels(result: dict, original: bytes, kind: str) -> bool:
    """Check decoded artwork against the archive, independently of agreement.

    The flipped Victor DIB is top-down here; true-color bytes remain DIB BGR.
    Palette colors are compared as RGB triples, ignoring no color channels.
    Padding is already compared in the full retail/candidate output stream.
    """
    if kind not in ('h3-indexed8', 'h3-packed24'):
        return True  # Native PCX has no separately decoded archive expectation.
    size, width, height = struct.unpack_from('<3I', original)
    snapshot = result.get('flipped')
    if snapshot is None:
        return False
    fields = snapshot['descriptor']
    stride = fields[5]
    channels = 1 if kind == 'h3-indexed8' else 3
    pixels = snapshot['pixels']
    expected = original[12:12 + size]
    for y in range(height):
        actual = pixels[y * stride:y * stride + width * channels]
        row = expected[y * width * channels:(y + 1) * width * channels]
        if channels == 3:
            reordered = bytearray(len(row))
            reordered[0::3], reordered[1::3], reordered[2::3] = row[2::3], row[1::3], row[0::3]
            row = bytes(reordered)
        if actual != row:
            return False
    if channels == 1:
        palette = snapshot['palette']
        rgb = bytearray(768)
        rgb[0::3], rgb[1::3], rgb[2::3] = palette[2::4], palette[1::4], palette[0::4]
        if rgb != original[-768:]:
            return False
    return True
