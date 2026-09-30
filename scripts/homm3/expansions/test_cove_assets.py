"""Native resource contracts; no installed game or upstream checkout required."""
import struct
import unittest

from PIL import Image

from .cove_assets import extend_def, pcx


def frame_directory(data):
    count = struct.unpack_from('<I', data, 788)[0]
    names = [data[800 + i * 13:813 + i * 13].rstrip(b'\0') for i in range(count)]
    offsets = struct.unpack_from(f'<{count}I', data, 800 + count * 13)
    return names, offsets


class NativeAssetsTest(unittest.TestCase):
    def setUp(self):
        self.palette = bytes(range(256)) * 3
        self.picture = Image.new('P', (2, 3), 17)
        self.picture.putpalette(self.palette)
        # The old frame's encoded payload must survive extension byte for byte.
        self.payload = struct.pack('<8I', 6, 0, 2, 3, 2, 3, 0, 0) + bytes([91]) * 6
        self.base = (struct.pack('<4I', 71, 2, 3, 1) + self.palette
                     + struct.pack('<4I', 0, 1, 0, 0) + b'oldframe\0\0\0\0\0'
                     + struct.pack('<I', 817) + self.payload)

    def test_extension_preserves_existing_encoded_frames(self):
        result = extend_def(self.base, {1: self.picture}, 'twcrport')
        names, offsets = frame_directory(result)
        self.assertEqual(names, [b'oldframe', b'twcrport0001'])
        self.assertEqual(offsets[0], 834)
        self.assertEqual(result[offsets[0]:offsets[1]], self.payload)
        self.assertEqual(struct.unpack_from('<8I', result, offsets[1]),
                         (6, 0, 2, 3, 2, 3, 0, 0))
        self.assertEqual(result[offsets[1] + 32:], bytes([17]) * 6)

    def test_frame_names_do_not_alias_between_global_cache_entries(self):
        small = extend_def(self.base, {1: self.picture}, 'cprsmall')
        large = extend_def(self.base, {1: self.picture}, 'twcrport')
        self.assertNotEqual(frame_directory(small)[0][1], frame_directory(large)[0][1])

    def test_invalid_append_or_namespace_is_rejected(self):
        for images, prefix in (({0: self.picture}, 'cove'), ({2: self.picture}, 'cove'),
                               ({1: self.picture}, 'toolongprefix')):
            with self.subTest(images=list(images), prefix=prefix), self.assertRaises(ValueError):
                extend_def(self.base, images, prefix)

    def test_indexed_bitmap_uses_native_archive_format(self):
        result = pcx(self.picture)
        self.assertEqual(struct.unpack_from('<III', result), (6, 2, 3))
        self.assertEqual(result[12:18], bytes([17]) * 6)
        self.assertEqual(result[18:], self.palette)

    def test_truecolor_bitmap_uses_bgr_pixels(self):
        result = pcx(Image.new('RGB', (2, 1), (10, 20, 30)))
        self.assertEqual(struct.unpack_from('<III', result), (6, 2, 1))
        self.assertEqual(result[12:], bytes([30, 20, 10]) * 2)


if __name__ == '__main__':
    unittest.main()
