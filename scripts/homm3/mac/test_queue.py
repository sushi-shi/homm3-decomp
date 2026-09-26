"""Routing and freshness contracts for future worker task packets."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.core import inputs
from homm3.mac import calls, queue
from homm3.mac.relocations import Address
from homm3.mac.source import Pair
from homm3.match.status import MatchRow


class TestMacQueue(unittest.TestCase):
    def route(self, **changes):
        empty = calls.analyze(bytes.fromhex("4e800020"), Address(0, 0), {})
        args = dict(va="0x00400100", unit="test", paired=True, deferred=False,
                    windows_stale=False, problem=None,
                    comparison=calls.compare(empty, empty), mac_max=90)
        args.update(changes)
        return queue.route(**args)

    def test_routes_setup_and_matching_separately(self):
        self.assertEqual(self.route(paired=False)["state"], "pairing_needed")
        self.assertFalse(self.route(paired=False)["dispatchable"])
        self.assertEqual(self.route(problem="source changed")["state"], "call_report_needed")
        self.assertFalse(self.route(windows_stale=True)["dispatchable"])
        self.assertEqual(self.route(mac_max=None)["state"], "byte_report_needed")
        self.assertEqual(self.route(mac_max=100)["state"], "windows_instruction_difference")
        self.assertEqual(self.route()["state"], "mac_instruction_difference")
        self.assertTrue(self.route()["dispatchable"])
        deferred = self.route(deferred=True)
        self.assertEqual(deferred["state"], "deferred")
        self.assertFalse(deferred["dispatchable"])
        failed = self.route(comparison_error="unknown TOC reference", mac_max=100)
        self.assertEqual(failed["state"], "comparison_setup_needed")
        self.assertFalse(failed["dispatchable"])

    def test_missing_candidate_and_call_mismatch_have_concrete_commands(self):
        empty = calls.analyze(bytes.fromhex("4e800020"), Address(0, 0), {})
        one = calls.analyze(bytes.fromhex("480001014e800020"), Address(0, 0), {})
        failed = self.route(comparison=calls.compare(empty, None, error="compile failed"))
        self.assertEqual(failed["state"], "compilation_needed")
        self.assertFalse(failed["dispatchable"])
        different = self.route(comparison=calls.compare(one, empty))
        self.assertEqual(different["state"], "call_difference")
        self.assertEqual(different["command"], "homm3 mac calls 0x00400100")

    def test_freshness_checks_source_profile_object_listing_and_analysis(self):
        digest = lambda value: hashlib.sha256(value).hexdigest()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            pair = Pair(0x400100, "test", root / "src/test.cpp", "void test",
                        0, 0x100, 4, ".test", "fixture")
            work = root / "build/mac/objects/00400100"
            work.mkdir(parents=True)
            (work / "candidate.cpp").write_bytes(b"source")
            (work / "candidate.o").write_bytes(b"object")
            (work / "candidate.dis.txt").write_bytes(b"listing")
            stamp = dict(fingerprint="profile", object_sha256=digest(b"object"),
                         listing_sha256=digest(b"listing"))
            (work / "build-stamp.json").write_text(json.dumps(stamp))
            row = dict(mac_section=0, mac_offset="0x100", size=4, signature="void test",
                       source_hash=digest(b"source"), build_hash="profile",
                       object_sha256=digest(b"object"))
            report = dict(target_sha256=inputs.MAC.sha256, analysis_sha256="analysis")
            with patch.object(queue, "candidate_source", return_value="source"), \
                 patch.object(queue.build, "_profile_hash", return_value="profile"), \
                 patch.object(queue.call_report, "analysis_hash", return_value="analysis"):
                self.assertIsNone(queue.observation_problem(root, pair, row, report))
                for field, value in (("source_hash", "changed"), ("build_hash", "changed"),
                                     ("mac_offset", "0x104"), ("object_sha256", "changed")):
                    self.assertIsNotNone(queue.observation_problem(root, pair, dict(row, **{field: value}), report))
                self.assertIsNotNone(queue.observation_problem(root, pair, row, dict(report, analysis_sha256="old")))
                (work / "candidate.dis.txt").write_bytes(b"modified")
                self.assertIsNotNone(queue.observation_problem(root, pair, row, report))

    def test_inventory_covers_unpaired_unowned_and_deferred_targets(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config/mac").mkdir(parents=True)
            (root / "config/mac/campaign.toml").write_text(
                'deferred_modules = ["zlib-1.1.3"]\nworkers_after_tooling = 6\n')
            category = {0x100: "target", 0x200: "target", 0x300: "runtime",
                        0x400: "target", 0x500: "zlib"}
            baseline = {("test", "exact"): MatchRow(100, 100, 100, 0x100),
                        ("test", "unfinished"): MatchRow(70, 70, 70, 0x200)}
            with patch.object(queue.universe, "classify", return_value=(category, {rva: 8 for rva in category})), \
                 patch.object(queue.status, "load_baseline", return_value=baseline), \
                 patch.object(queue.status, "source_hashes", return_value={}), \
                 patch.object(queue.manifest, "by_unit", return_value={"test": {"source": "src/test.cpp"}}), \
                 patch.object(queue, "load_pairs", return_value=[]):
                report = queue.generate(root)
                self.assertEqual(report["coverage"]["windows_targets"], 4)
                self.assertEqual(report["coverage"]["windows_banked_exact"], 1)
                self.assertEqual(report["coverage"]["queued"], 3)
                self.assertEqual(sum(report["coverage"]["dispositions"].values()), 4)
                self.assertEqual(report["coverage"]["dispatchable"], 0)
                self.assertEqual(sum(row["state"] == "deferred" for row in report["rows"]), 1)
                self.assertTrue(all(row["retail_calls"] is None for row in report["rows"]))
                queue.write(root, report)
                self.assertEqual(len(json.loads((root / "build/mac/queue.json").read_text())["rows"]), 3)
                all_callers = queue.generate(root, include_banked_exact=True)
                self.assertEqual(all_callers["coverage"]["queued"], 4)
                self.assertIn("0x00400100", {row["retail_va"] for row in all_callers["rows"]})


if __name__ == "__main__":
    unittest.main()
