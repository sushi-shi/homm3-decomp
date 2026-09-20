"""Oracle input/provenance contracts, not mock tests of game functions."""
import struct
import unittest
import zlib

from .corpus import bitmap_to_pcx, classify, encode_rle, lod_members


def decode_rle(encoded, size):
    decoded = bytearray()
    cursor = 0
    while len(decoded) < size:
        value = encoded[cursor]
        cursor += 1
        if value & 192 == 192:
            count = value & 63
            value = encoded[cursor]
            cursor += 1
        else:
            count = 1
        decoded.extend([value] * count)
    if len(decoded) != size:
        raise ValueError('overlong row')
    return bytes(decoded), cursor


class CorpusTests(unittest.TestCase):
    def test_rle_literals_runs_and_escape_boundary(self):
        raw = bytes(range(256)) + b'\xc0' * 129 + b'\x01' * 64
        encoded = encode_rle(raw)
        decoded, consumed = decode_rle(encoded, len(raw))
        self.assertEqual(decoded, raw)
        self.assertEqual(consumed, len(encoded))

    def test_indexed_odd_stride_and_palette(self):
        pixels = bytes((192, 193, 4, 6, 7, 8))
        palette = bytes(range(256)) * 3
        raw = struct.pack('<III', 6, 3, 2) + pixels + palette
        pcx = bitmap_to_pcx(raw)
        self.assertEqual(classify(pcx)['planes'], 1)
        self.assertEqual(struct.unpack_from('<H', pcx, 66)[0], 4)
        decoded, used = decode_rle(pcx[128:], 8)
        self.assertEqual(decoded, pixels[:3] + b'\0' + pixels[3:] + b'\0')
        self.assertEqual(pcx[128 + used:], b'\x0c' + palette)

    def test_packed_rgb_to_planar_rgb(self):
        raw = struct.pack('<III', 9, 3, 1) + bytes((1, 2, 3, 4, 5, 6, 7, 8, 9))
        pcx = bitmap_to_pcx(raw)
        decoded, used = decode_rle(pcx[128:], 12)
        self.assertEqual(decoded, bytes((1, 4, 7, 0, 2, 5, 8, 0, 3, 6, 9, 0)))
        self.assertEqual(128 + used, len(pcx))

    def test_lod_occurrences_and_bounds(self):
        payload = b'example' * 3
        compressed = zlib.compress(payload)
        header = bytearray(92)
        header[:4] = b'LOD\0'
        struct.pack_into('<I', header, 8, 1)
        record = b'test.pcx'.ljust(16, b'\0') + struct.pack('<4I', 124, len(payload), 0, len(compressed))
        archive = bytes(header) + record + compressed
        self.assertEqual(list(lod_members(archive)), [(0, 'test.pcx', payload)])
        with self.assertRaises(ValueError):
            list(lod_members(archive[:-1]))
