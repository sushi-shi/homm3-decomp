"""Regression tests for the second target's byte and source ownership gates."""
from __future__ import annotations

from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.mac import build, pairs
from homm3.mac.object import CodeHunk, ObjectError, select_hunk
from homm3.mac.pef import PEF, PEFError
from homm3.mac.relocations import Address, link_code


class TestMacTarget(unittest.TestCase):
    def test_ledger_keeps_unscored_rows_and_resets_max_on_source_change(self):
        with tempfile.TemporaryDirectory() as directory:
            baseline = Path(directory) / "match_baseline.tsv"
            baseline.write_text("# header\n# columns\n"
                                "0x00400100\tkept\t0\t0x100\t50.0000\t60.0000\t70.0000\ttokens1:a\n"
                                "0x00400200\tunit\t0\t0x200\t10.0000\t90.0000\t95.0000\ttokens1:old\n"
                                "0x00400300\tunit\t0\t0x300\t10.0000\t20.0000\t20.0000\ttokens1:same\n")

            def result(va, score, source_hash):
                return build.Result(retail_va=va, unit="unit", signature="f", mac_section=0,
                                    mac_offset="0x0", size=4, candidate_size=4, matching_bytes=4,
                                    score=score, exact=False, source_hash=source_hash,
                                    target_sha256="t", mac_symbol=".f", resolved_calls=(),
                                    removed_reload_slots=(), first_difference="+0x0")
            with patch.object(build, "BASELINE", baseline):
                build._checkpoint([result("0x00400200", 30.0, "tokens1:new"),
                                   result("0x00400300", 15.0, "tokens1:same"),
                                   result("0x00400400", 5.0, "tokens1:b")])
            rows = {line.split("\t")[0]: line.split("\t")[4:] for line in baseline.read_text().splitlines()
                    if not line.startswith("#")}
        self.assertEqual(rows["0x00400100"], ["50.0000", "60.0000", "70.0000", "tokens1:a"])
        self.assertEqual(rows["0x00400200"], ["30.0000", "30.0000", "95.0000", "tokens1:new"])
        self.assertEqual(rows["0x00400300"], ["15.0000", "20.0000", "20.0000", "tokens1:same"])
        self.assertEqual(rows["0x00400400"], ["5.0000", "5.0000", "5.0000", "tokens1:b"])

    def test_selector_accepts_va_mac_offset_and_name(self):
        pair = pairs.Pair(0x4e51c0, "hero", Path("src/hero.cpp"), "hero::getHighestSchool",
                          0, 0x106188, 0x40, ".getHighestSchool__4heroFv", "va:0x004e51c0")
        inventory = pairs.Inventory((pair,), (), {}, {}, ())
        self.assertIs(pairs.select(inventory, "0x4e51c0"), pair)
        self.assertIs(pairs.select(inventory, "mac:0:0x10618c"), pair)
        self.assertIs(pairs.select(inventory, "getHighestSchool"), pair)
        with self.assertRaises(pairs.PairError):
            pairs.select(inventory, "mac:0x200000")

    def test_pef_section_relative_span(self):
        data = bytearray(0x80)
        data[:12] = b"Joy!peffpwpc"
        data[12:16] = (1).to_bytes(4, "big")
        data[32:34] = (1).to_bytes(2, "big")
        data[34:36] = (1).to_bytes(2, "big")
        data[48:52] = (8).to_bytes(4, "big")
        data[52:56] = (8).to_bytes(4, "big")
        data[56:60] = (8).to_bytes(4, "big")
        data[60:64] = (0x78).to_bytes(4, "big")
        data[0x78:0x80] = bytes.fromhex("548007ff4e800020")
        pef = PEF(bytes(data))
        self.assertEqual(pef.code(0, 0, 8), bytes.fromhex("548007ff4e800020"))
        with self.assertRaises(PEFError):
            pef.code(0, 4, 8)
        with self.assertRaises(PEFError):
            PEF(bytes(data[:-1]))

    def test_named_object_hunk_and_relocations(self):
        listing = '''Names:
Hunk: Kind=HUNK_GLOBAL_CODE Align=4 Class=PR Name=".helper"(4) Size=8
00000000: 48000001  bl  0
00000004: 4E800020  blr
XRef: Kind=HUNK_XREF_24BIT Offset=$00000000 Class=PR Name=".callee"(5)
'''
        hunk = select_hunk(listing, ".helper")
        self.assertEqual(hunk.data, bytes.fromhex("480000014e800020"))
        self.assertEqual(hunk.xrefs[0], (0, "HUNK_XREF_24BIT", ".callee"))
        with self.assertRaises(ObjectError):
            select_hunk(listing.replace("00000004", "00000008"), ".helper")

    def test_call_relocation_and_reload_slot_collapse(self):
        hunk = CodeHunk(".caller", bytes.fromhex(
            "48000018 48000001 60000000 4182000c "
            "48000001 60000000 4bffffe8 4e800020"),
            ((4, "HUNK_XREF_24BIT", ".a"), (16, "HUNK_XREF_24BIT", ".b")))
        symbols = {".a": Address(0, 0x180), ".b": Address(0, 0x80)}
        linked = link_code(hunk, Address(0, 0x100), symbols, collapse_reloads=True)
        self.assertEqual(linked.data, bytes.fromhex(
            "48000010 4800007d 41820008 4bffff75 4bfffff0 4e800020"))
        self.assertEqual(linked.removed_reload_slots, (8, 20))
        self.assertEqual([call.linked_offset for call in linked.calls], [4, 12])
        changed = link_code(hunk, Address(0, 0x100),
                            dict(symbols, **{".a": Address(0, 0x184)}),
                            collapse_reloads=True)
        self.assertNotEqual(linked.data, changed.data)
        with self.assertRaises(ObjectError):
            link_code(hunk, Address(0, 0x100), {".a": symbols[".a"]},
                      collapse_reloads=True)
        with self.assertRaises(ObjectError):
            link_code(hunk, Address(1, 0x100), symbols, collapse_reloads=True)

    def test_reject_branch_into_removed_reload_slot(self):
        hunk = CodeHunk(".caller", bytes.fromhex("48000008 48000001 60000000 4e800020"),
                        ((4, "HUNK_XREF_24BIT", ".a"),))
        with self.assertRaises(ObjectError):
            link_code(hunk, Address(0, 0), {".a": Address(0, 0x100)},
                      collapse_reloads=True)


if __name__ == "__main__":
    unittest.main()
