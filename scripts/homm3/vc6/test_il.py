"""Focused negative controls for the VC6 IL record overlays."""

import unittest
import struct
from pathlib import Path
from unittest.mock import patch

from homm3.vc6 import _il, il


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


    def test_rmg_local_tags_do_not_consume_printable_name_prefixes(self):
        for tag, handle, name in ((2, 0x56AC, "nearby"),
                                  (2, 0x56A2, "destinationZone"),
                                  (4, 0x56D8, "offset"),
                                  (5, 0x56E0, "value")):
            with self.subTest(name=name):
                data = bytes((1, tag)) + struct.pack("<H", handle) + b"\0" + name.encode() + b"\0"
                self.assertEqual(_il.scan_names(data, 0x6672),
                                 [{"off": 2, "handle": handle, "name": name}])


def function_record(handle, name, ex, sy):
    return (b"\x0e" + struct.pack("<H", handle) + b"\0" + name.encode() + b"\0"
            + b"\x02\x05\x01\x14\0\0\x80" + struct.pack("<i", ex)
            + (bytes((sy,)) if 0 <= sy < 128 else b"\x80" + struct.pack("<i", sy))
            + b"\x01\x28\x05\x03")


class BodyOffsetTest(unittest.TestCase):
    def test_offset_reader_signed_short_extended_and_truncated(self):
        self.assertEqual(_il.read_signed_offset(b"\x7f", 0), (127, 1))
        self.assertEqual(_il.read_signed_offset(b"\xff", 0), (-1, 1))
        self.assertEqual(_il.read_signed_offset(b"\x80\x38\x9e\0\0", 0), (0x9E38, 5))
        self.assertEqual(_il.read_signed_offset(b"\x80\xff\xff\xff\xff", 0), (-1, 5))
        for data in (b"", b"\x80", b"\x80\0\0\0"):
            with self.subTest(data=data), self.assertRaises(ValueError):
                _il.read_signed_offset(data, 0)

    def test_observed_shipyard_pair_ignores_overlapping_marker(self):
        gl = function_record(0x4377, "?shipyard@@", 0x27546, 0x9E38)
        records = _il.fn_body_offsets(gl, _il.scan_names(gl, 0x6672, 3), 0x50000, 0x20000)
        self.assertEqual([(r["ex"], r["sy"]) for r in records], [(0x27546, 0x9E38)])

    def test_unframed_data_and_invalid_offsets_are_not_function_bodies(self):
        gl = function_record(100, "?data@@", 200, 10).replace(b"\x0e", b"\x02", 1)
        self.assertEqual(_il.fn_body_offsets(gl, _il.scan_names(gl, 1000), 1000, 100), [])
        for ex, sy in ((1000, 10), (200, 100), (200, -1)):
            gl = function_record(100, "?bad@@", ex, sy)
            self.assertEqual(_il.fn_body_offsets(gl, _il.scan_names(gl, 1000), 1000, 100), [])

    def test_independent_ambiguous_pairs_fail(self):
        gl = function_record(100, "?bad@@", 200, 10) + b"\0\0\x80\x2c\x01\0\0\x14"
        with self.assertRaisesRegex(ValueError, "ambiguous"):
            _il.fn_body_offsets(gl, _il.scan_names(gl, 1000), 1000, 100)

    def streams(self):
        # Both methods were declared before either body's locals. Declaration
        # handle proximity would lose lateLocal and include no useful locals.
        gl = (_il.GL_MAGIC + struct.pack("<I", 0x6000)
              + function_record(100, "?first@@", 200, 0)
              + function_record(101, "?second@@", 300, 40))
        sy = (b"\x03\x01\0\x01\x02\0\x50\0lateLocal\0").ljust(40, b"\0")
        sy += b"\x03\x01\0\x01\x02\x01\x50\0otherLocal\0"
        return {"gl": gl, "sy": sy, "ex": bytes(1000)}

    def test_body_offsets_select_late_locals_and_exclude_next_body(self):
        notes = "\n".join(il.local_symbol_notes(self.streams(), "first"))
        self.assertIn("declaration handle 0x64", notes)
        self.assertIn("SY window [0x0, 0x28)", notes)
        self.assertIn("0x5000", notes)
        self.assertIn("lateLocal", notes)
        self.assertNotIn("otherLocal", notes)
        self.assertIn("not optimizer pseudo/register order", notes)

    def test_missing_ambiguous_and_shared_starts_fail(self):
        for name in ("absent", "@@", ""):
            with self.subTest(name=name), self.assertRaises(ValueError):
                il.local_symbol_notes(self.streams(), name)
        streams = self.streams()
        streams["gl"] += function_record(102, "?alias@@", 400, 0)
        with self.assertRaisesRegex(ValueError, "share SY start"):
            il.local_symbol_notes(streams, "first")

    @patch("homm3.vc6.il._gate_subjects")
    @patch("homm3.vc6.il._ensure_wine_env")
    @patch("homm3.vc6.shim.build._traceCapture")
    @patch("homm3.vc6._unit.flags_for_unit", return_value=["/O2", "/MT", "/Gr"])
    @patch("homm3.vc6._unit.unit_for_source", return_value="rmg")
    def test_capture_keeps_manifest_flags_and_canonical_source(self, unit, flags, capture, wine, gate):
        import tempfile
        source = Path("/repo/src/rmg.cpp")
        capture.return_value = self.streams()
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "locals"
            il.capture_local_symbols(source, "first", output)
            capture.assert_called_once_with(source, ["/O2", "/MT", "/Gr"], output)
        unit.assert_called_once_with(source)
        flags.assert_called_once_with("rmg")
        gate.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
