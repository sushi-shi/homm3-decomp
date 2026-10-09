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
    def test_generated_copy_assignment_requires_const_reference_to_same_owner(self):
        symbol = ".__as__14CMapHeaderDataFRC14CMapHeaderData"
        self.assertEqual(pairs.generated_copy_owner(symbol),
                         ("IMPLICIT_COPY_ASSIGN", "CMapHeaderData"))
        for invalid in (
            ".__as__14CMapHeaderDataF14CMapHeaderData",
            ".__as__14CMapHeaderDataCFRC14CMapHeaderData",
            ".__as__14CMapHeaderDataFRC15SavedGameHeader",
            ".__as__14CMapHeaderDataFRC14CMapHeaderDatai",
            ".__dt__14CMapHeaderDataFRC14CMapHeaderData",
        ):
            with self.subTest(symbol=invalid):
                self.assertIsNone(pairs.generated_copy_owner(invalid))

    def test_generated_copy_constructor_requires_const_reference_to_same_owner(self):
        for symbol, owner in (
            (".__ct__16type_dialog_iconFRC16type_dialog_icon", "type_dialog_icon"),
            (".__ct__Q23Foo3BarFRCQ23Foo3Bar", "Foo::Bar"),
        ):
            self.assertEqual(pairs.generated_copy_owner(symbol), ("IMPLICIT_COPY_CTOR", owner))
        for invalid in (
            ".__ct__16type_dialog_iconFv",
            ".__ct__16type_dialog_iconFR16type_dialog_icon",
            ".__ct__16type_dialog_iconFRC14CMapHeaderData",
            ".__ct__16type_dialog_iconFRC16type_dialog_iconi",
            ".__ct__16type_dialog_iconCFRC16type_dialog_icon",
            ".__ct__Q23Foo3BarFRCQ23Baz3Bar",
        ):
            with self.subTest(symbol=invalid):
                self.assertIsNone(pairs.generated_copy_owner(invalid))

    def test_generated_copy_constructor_binds_only_its_source_claim(self):
        from homm3.mac.addresses import Claim
        ctor = ".__ct__16type_dialog_iconFRC16type_dialog_icon"
        assignment = ".__as__16type_dialog_iconFRC16type_dialog_icon"
        claim = Claim("src/kb.cpp", 4627, 0x118134, 0xac,
                      0x4f6810, ("IMPLICIT_COPY_CTOR", "type_dialog_icon"))
        with patch.object(pairs.emitted, "object_hunks", return_value={}), \
             patch.object(pairs.emitted, "claim_bodies", return_value=[]), \
             patch.object(pairs.emitted, "bind", return_value=[]), \
             patch.object(pairs, "_universe", return_value={ctor, assignment}):
            result = pairs.load(Path("."), definitions=[], claims=[claim])
            self.assertEqual(result.symbols, {ctor: 0x118134})
            self.assertEqual(result.labels[ctor], "type_dialog_icon::type_dialog_icon")
            self.assertFalse(result.pairs)
            self.assertFalse(pairs.load(Path("."), definitions=[], claims=[]).symbols)
            conflicting = Claim("src/other.cpp", 1, 0x1234, 0xac,
                                0x123456, claim.compgen)
            result = pairs.load(Path("."), definitions=[], claims=[claim, conflicting])
            self.assertNotIn(ctor, result.symbols)
            self.assertEqual(result.conflicts, (ctor,))

    def test_generated_copy_assignment_uses_source_claim_and_preserves_conflicts(self):
        from homm3.mac.addresses import Claim
        symbol = ".__as__14CMapHeaderDataFRC14CMapHeaderData"
        claim = Claim("src/campaignbrief.cpp", 178, 0x64044, 0x228,
                      0x457cb0, ("IMPLICIT_COPY_ASSIGN", "CMapHeaderData"))
        with patch.object(pairs.emitted, "object_hunks", return_value={}), \
             patch.object(pairs.emitted, "claim_bodies", return_value=[]), \
             patch.object(pairs.emitted, "bind", return_value=[]), \
             patch.object(pairs, "_universe", return_value={symbol}):
            result = pairs.load(Path("."), definitions=[], claims=[claim])
            self.assertEqual(result.symbols[symbol], 0x64044)
            self.assertFalse(result.pairs)  # a call binding is not an exact-body verdict
            conflicting = Claim("src/other.cpp", 1, 0x1234, 0x228,
                                0x123456, claim.compgen)
            result = pairs.load(Path("."), definitions=[], claims=[claim, conflicting])
            self.assertNotIn(symbol, result.symbols)
            self.assertEqual(result.conflicts, (symbol,))

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
            with patch.object(build, "BASELINE", baseline), \
                 patch.object(build, "PROFILES", Path(directory) / "match_profiles.tsv"), \
                 patch.object(build.profiles, "flags", return_value=("-O3", "-nolink")):
                build._checkpoint([result("0x00400200", 30.0, "tokens1:new"),
                                   result("0x00400300", 15.0, "tokens1:same"),
                                   result("0x00400400", 5.0, "tokens1:b")])
            rows = {line.split("\t")[0]: line.split("\t")[4:] for line in baseline.read_text().splitlines()
                    if not line.startswith("#")}
        self.assertEqual(rows["0x00400100"], ["50.0000", "60.0000", "70.0000", "tokens1:a", "0"])
        self.assertEqual(rows["0x00400200"], ["30.0000", "30.0000", "95.0000", "tokens1:new", "1"])
        self.assertEqual(rows["0x00400300"], ["15.0000", "20.0000", "20.0000", "tokens1:same", "1"])
        self.assertEqual(rows["0x00400400"], ["5.0000", "5.0000", "5.0000", "tokens1:b", "1"])

    def test_preservation_gate_requires_reviewed_one_checkpoint_exception(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "owner.cpp"
            source.write_text("VA(0x00400100, 4) MAC_ADDRESS(0x100, 4)\n")
            pair = pairs.Pair(0x400100, "unit", source, "owner::helper", 0,
                              0x100, 4, ".helper", "va:0x00400100", line=1)
            inventory = pairs.Inventory((pair,), (), {}, {}, ())
            old = {"0x00400100": ["0x00400100", "unit", "0", "0x100", "60.0000",
                                   "60.0000", "60.0000", "tokens1:abc", "1"]}
            result = build.Result(retail_va="0x00400100", unit="unit", signature="owner::helper",
                                  mac_section=0, mac_offset="0x100", size=4,
                                  candidate_size=4, matching_bytes=2, score=50.0,
                                  exact=False, source_hash="tokens1:def", target_sha256="t",
                                  mac_symbol=".helper", resolved_calls=(), removed_reload_slots=(),
                                  first_difference="+0x2")
            problems, exceptions = build._preservation_problems([result], inventory, [], old)
            self.assertEqual(len(problems), 1)
            self.assertFalse(exceptions)

            source.write_text("VA(0x00400100, 4) MAC_ADDRESS(0x100, 4) "
                              "// MAC_ABSTRACTION_FROM(tokens1:abc,60.0000): "
                              "restore the canonical owner helper\n")
            problems, exceptions = build._preservation_problems([result], inventory, [], old)
            self.assertFalse(problems)
            self.assertEqual(len(exceptions), 1)

            later = {"0x00400100": [*old["0x00400100"][:4], "50.0000", "60.0000",
                                   "60.0000", "tokens1:def", "1"]}
            lower = build.Result(**{**result.__dict__, "score": 40.0})
            problems, exceptions = build._preservation_problems([lower], inventory, [], later)
            self.assertEqual(len(problems), 1)  # old marker cannot waive a later drop
            self.assertFalse(exceptions)

            problems, _ = build._preservation_problems([], inventory, [], later)
            self.assertIn("unavailable", problems[0])
            legacy = {"0x00400100": later["0x00400100"][:8]}
            problems, _ = build._preservation_problems([], inventory, [], legacy)
            self.assertFalse(problems)  # pre-gate historical gap

    def test_preservation_gate_separates_profile_changes_from_source_changes(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "owner.cpp"
            source.write_text("VA(0x00400100, 4) MAC_ADDRESS(0x100, 4)\n")
            pair = pairs.Pair(0x400100, "unit", source, "owner::helper", 0,
                              0x100, 4, ".helper", "va:0x00400100", line=1)
            inventory = pairs.Inventory((pair,), (), {}, {}, ())
            old = {"0x00400100": ["0x00400100", "unit", "0", "0x100", "60.0000",
                                   "60.0000", "60.0000", "tokens1:abc", "1"]}
            same = build.Result(retail_va="0x00400100", unit="unit", signature="owner::helper",
                                mac_section=0, mac_offset="0x100", size=4,
                                candidate_size=4, matching_bytes=2, score=50.0,
                                exact=False, source_hash="tokens1:abc", target_sha256="t",
                                mac_symbol=".helper", resolved_calls=(), removed_reload_slots=(),
                                first_difference="+0x2")
            o3, o1 = {"unit": "-O3 -nolink"}, {"unit": "-O1 -nolink"}
            problems, exceptions = build._preservation_problems([same], inventory, [], old, o3, o1)
            self.assertFalse(problems)
            self.assertIn("profile '-O3 -nolink' -> '-O1 -nolink'", exceptions[0])
            for before, after in ((o3, o3), ({}, o1)):
                problems, exceptions = build._preservation_problems(
                    [same], inventory, [], old, before, after)
                self.assertEqual(len(problems), 1)  # unchanged or unrecorded profile
            edited = build.Result(**{**same.__dict__, "source_hash": "tokens1:def"})
            problems, _ = build._preservation_problems([edited], inventory, [], old, o3, o1)
            self.assertEqual(len(problems), 1)  # a source edit needs its own review

            problems, _ = build._preservation_problems([], inventory, [], old)
            self.assertIn("unavailable", problems[0])
            source.write_text("VA(0x00400100, 4) MAC_ADDRESS(0x100, 4) "
                              "// MAC_ABSTRACTION_FROM(tokens1:abc,60.0000): "
                              "retail's object split makes this a header inline body\n")
            problems, exceptions = build._preservation_problems([], inventory, [], old)
            self.assertFalse(problems)
            self.assertIn("unavailable", exceptions[0])
            unemitted = pairs.Inventory((), ({"retail_va": "0x00400100", "unit": "unit",
                                              "file": "owner.cpp", "line": 1,
                                              "reason": "not_emitted"},), {}, {}, ())
            with patch.object(build, "ROOT", Path(directory)):
                problems, exceptions = build._preservation_problems([], unemitted, [], old)
            self.assertFalse(problems)
            self.assertIn("not_emitted", exceptions[0])

    def test_checkpoint_records_unit_profiles(self):
        with tempfile.TemporaryDirectory() as directory:
            baseline = Path(directory) / "match_baseline.tsv"
            recorded = Path(directory) / "match_profiles.tsv"
            recorded.write_text("# header\n# columns\nkept\t-O3 -nolink\nunit\t-O3 -nolink\n")
            row = build.Result(retail_va="0x00400100", unit="unit", signature="f", mac_section=0,
                               mac_offset="0x0", size=4, candidate_size=4, matching_bytes=4,
                               score=100.0, exact=True, source_hash="tokens1:a",
                               target_sha256="t", mac_symbol=".f", resolved_calls=(),
                               removed_reload_slots=(), first_difference=None)
            with patch.object(build, "BASELINE", baseline), \
                 patch.object(build, "PROFILES", recorded), \
                 patch.object(build.profiles, "flags", return_value=("-O1", "-nolink")):
                build._checkpoint([row])
                self.assertEqual(build._previous_profiles(),
                                 {"kept": "-O3 -nolink", "unit": "-O1 -nolink"})

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
