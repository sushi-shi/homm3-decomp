"""Output framing and retail-fallback rejection contracts."""
import struct
import unittest
from unittest.mock import patch

from .protocol import decode, verify_pixels
from .runtime import resolve


class ProtocolTests(unittest.TestCase):
    def test_error_result_requires_complete_exact_framing(self):
        raw = struct.pack('<10I', 0x31544356, 0xfffffffc, *([0xa5a5a5a5] * 8))
        self.assertEqual(decode(raw)['infoStatus'], 0xfffffffc)
        for invalid in (raw[:-1], raw + b'\0', b'XXXX' + raw[4:]):
            with self.assertRaises(ValueError):
                decode(invalid)

    def test_missing_victor_body_is_not_bound_to_retail(self):
        with patch('homm3.victor.runtime.undefined_symbols', return_value={'?loadpcx@@YGHPBDPAUimgdes@@@Z'}):
            with self.assertRaisesRegex(ValueError, 'no retail Victor fallback'):
                resolve([])

    def test_pixel_mutation_is_detected_independently(self):
        original = struct.pack('<3I', 6, 2, 1) + bytes((1, 2, 3, 4, 5, 6))
        fields = [0] * 13
        fields[5] = 8
        result = {'flipped': {'descriptor': fields, 'pixels': bytes((3, 2, 1, 6, 5, 4, 165, 165))}}
        self.assertTrue(verify_pixels(result, original, 'h3-packed24'))
        result['flipped']['pixels'] = bytes(8)
        self.assertFalse(verify_pixels(result, original, 'h3-packed24'))
