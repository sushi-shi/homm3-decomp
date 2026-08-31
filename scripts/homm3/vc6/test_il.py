"""Focused negative controls for the VC6 IL record overlays."""

import unittest

from homm3.vc6 import _il


class NameScanTest(unittest.TestCase):
    def test_mangled_function_name_is_not_consumed_as_embedded_handle(self):
        data = bytes.fromhex("0e0b2800") + \
            b"?ProcessCombatMsg@combatManager@@QAEHAAVmessage@@@Z\0"
        self.assertEqual(
            _il.scan_names(data, highwater=0x12270, min_len=3),
            [{"off": 1, "handle": 0x280B,
              "name": "?ProcessCombatMsg@combatManager@@QAEHAAVmessage@@@Z"}],
        )

    def test_rich_sy_local_keeps_full_name_and_real_handle(self):
        data = bytes.fromhex("01028c97020000") + b"msgTemp\0"
        self.assertEqual(
            _il.scan_names(data, highwater=0x12270, min_len=1),
            [{"off": 2, "handle": 0x978C, "name": "msgTemp"}],
        )

    def test_embedded_printable_handle_still_wins_without_strong_frame(self):
        data = bytes.fromhex("0002004927") + b"$kTown0Buildings\0"
        self.assertEqual(
            _il.scan_names(data, highwater=0x12270, min_len=3),
            [{"off": 3, "handle": 0x2749, "name": "$kTown0Buildings"}],
        )


if __name__ == "__main__":
    unittest.main()
