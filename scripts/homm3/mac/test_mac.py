"""Regression tests for the second target's byte and source ownership gates."""
from __future__ import annotations

from pathlib import Path
from contextlib import ExitStack, redirect_stdout
import io
import json
import tempfile
import unittest
from unittest.mock import patch

from homm3.mac import build, calls
from homm3.mac.object import CodeHunk, ObjectError, select_hunk
from homm3.mac.pef import PEF, PEFError
from homm3.mac.relocations import Address, link_code
from homm3.mac.source import Pair, candidate_source, load_data, load_pairs
from homm3.mac.__main__ import _select


ROOT = Path(__file__).resolve().parents[3]


class TestMacTarget(unittest.TestCase):
    def test_failed_pair_keeps_later_call_reports_and_cannot_checkpoint_partial_scores(self):
        pairs = load_pairs(ROOT)[:2]
        empty = calls.analyze(bytes.fromhex("4e800020"), Address(0, 0), {})
        comparison = calls.compare(empty, empty)
        successful = build.Result(
            retail_va=f"0x{pairs[1].retail_va:08x}", unit=pairs[1].unit, signature=pairs[1].signature,
            mac_section=0, mac_offset="0x100", size=4, candidate_size=4, matching_bytes=4,
            score=100, exact=True, source_hash="source", target_sha256="target",
            object_sha256="object", build_hash="profile", resolved_calls=(),
            removed_reload_slots=(), first_difference=None, calls=comparison)
        with tempfile.TemporaryDirectory() as directory, ExitStack() as stack:
            root = Path(directory)
            report_path = root / "build/mac/report.json"
            stack.enter_context(patch.object(build, "ROOT", root))
            stack.enter_context(patch.object(build, "REPORT", report_path))
            stack.enter_context(patch.object(build, "load_pairs", return_value=pairs))
            stack.enter_context(patch.object(build.inputs, "stage_executable", return_value=root / "game.pef"))
            stack.enter_context(patch.object(build.inputs, "read_verified", return_value=b"fixture"))
            stack.enter_context(patch.object(build.toolchain, "stage", return_value=root / "tools"))
            stack.enter_context(patch.object(build, "PEF", return_value=object()))
            stack.enter_context(patch.object(build.call_report, "analysis_hash", return_value="analysis"))
            stack.enter_context(patch.object(build.call_report, "inspect", side_effect=lambda _, pair, *_args: {
                "retail_va": f"0x{pair.retail_va:08x}", "calls": comparison}))
            compare = stack.enter_context(patch.object(build, "compare_pair", side_effect=[
                ValueError("unknown TOC reference"), successful]))
            write = stack.enter_context(patch.object(build.call_report, "write"))
            checkpoint = stack.enter_context(patch.object(build, "_checkpoint"))
            stack.enter_context(redirect_stdout(io.StringIO()))
            with self.assertRaises(build.MacBuildError):
                build.run(checkpoint=True)
            self.assertEqual(compare.call_count, 2)
            self.assertEqual(len(write.call_args.args[1]), 2)
            self.assertIn("comparison_error", write.call_args.args[1][0])
            checkpoint.assert_not_called()
            report = json.loads(report_path.read_text())
            self.assertEqual(len(report["errors"]), 1)
            self.assertEqual(len(report["pairs"]), 1)

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

    def test_project_body_is_extracted_once(self):
        pair = next(pair for pair in load_pairs(ROOT) if pair.retail_va == 0x004e51c0)
        generated = candidate_source(pair)
        self.assertEqual(generated.count("hero::getHighestSchool"), 1)
        self.assertNotIn("bestSchool = schoolMask", generated)
        self.assertLess(generated.index("int bestLevel"), generated.index("TSpellSchool bestSchool;", generated.index("hero::getHighestSchool")))

    def test_mac_section_offset_inherits_windows_label(self):
        pair = _select("mac:0:0x106188")
        self.assertEqual(pair.retail_va, 0x004e51c0)
        self.assertEqual(_select("mac:0x10618c"), pair)

    def test_data_definition_and_name_are_owned_by_source(self):
        pair = next(pair for pair in load_pairs(ROOT) if pair.retail_va == 0x004da3a0)
        claim = next(claim for claim in load_data(ROOT) if claim.retail_va == 0x00679c88)
        self.assertEqual(claim.name, "g_experienceForLevel")
        generated = candidate_source(pair)
        self.assertEqual(generated.count("static short g_experienceForLevel[12]"), 1)
        self.assertIn(claim.definition, generated)
        self.assertNotIn("DATA(", generated)

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

    def test_compile_cache_tracks_source_profile_and_artifact_bytes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            pair = Pair(0x400100, "control", root / "control.cpp", "void control",
                        root / "shim.h", 0, 0x100, 4, ".control", "test")
            profile = {"flags": ["-O1"], "files": {}, "collapse_reloads": True}
            compiles = []

            def run(command, cwd, env):
                if command[1].endswith("MWCPPC.exe"):
                    compiles.append(command)
                    (cwd / "candidate.o").write_bytes(b"MWOBPPC " + bytes([len(compiles)]))
                    return ""
                return ('Hunk: Kind=HUNK_GLOBAL_CODE Align=4 Class=PR '
                        'Name=".control"(1) Size=4\n00000000: 4E800020 blr\n')

            with patch.object(build, "ROOT", root), \
                    patch.object(build, "candidate_source", return_value="body A") as source, \
                    patch.object(build, "_wine_version", return_value="test"), \
                    patch.object(build.toolchain, "specification", return_value=profile), \
                    patch.object(build, "_run", side_effect=run):
                first = build.compile_pair(pair, root / "tools")
                self.assertEqual(build.compile_pair(pair, root / "tools"), first)
                self.assertEqual(len(compiles), 1)
                work = root / "build/mac/objects/00400100"
                (work / "candidate.o").write_bytes(b"MWOBPPC corrupt")
                build.compile_pair(pair, root / "tools")
                (work / "candidate.dis.txt").write_text("stale listing")
                build.compile_pair(pair, root / "tools")
                source.return_value = "body B including changed shim"
                second = build.compile_pair(pair, root / "tools")
                self.assertNotEqual(first.source_hash, second.source_hash)
                profile["flags"] = ["-O1", "-proc", "750"]
                third = build.compile_pair(pair, root / "tools")
                self.assertEqual(second.source_hash, third.source_hash)
                self.assertNotEqual(second.build_hash, third.build_hash)
                self.assertEqual(len(compiles), 5)


if __name__ == "__main__":
    unittest.main()
