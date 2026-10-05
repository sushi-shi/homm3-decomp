"""`homm3 evidence`: section selection, capture, Mac gating and outputs."""
from __future__ import annotations

import contextlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from homm3 import evidence

ORIGINAL_STDOUT = sys.stdout


def _bound_renderer(text, out=ORIGINAL_STDOUT):
    # Like dreamcast's renderers: a default bound to the stream at import time.
    print(text, file=out)


class FakeCommands:
    def __init__(self, rc=None):
        self.calls = []
        self.rc = rc or {}

    def entry(self, group):
        def run(argv):
            self.calls.append((group, argv))
            if "--json" in argv:
                print(json.dumps({"group": group, "argv": argv}))
            else:
                print(f"{group} {' '.join(argv)}")
                print("note", file=sys.stderr)
            return self.rc.get((group, argv[0]), 0)
        return run


class EvidenceTests(unittest.TestCase):
    def test_sections_follow_agents_order_and_accept_groups(self):
        names = [s.name for s in evidence.select_sections(None, None)]
        self.assertEqual(names, ["show", "lines", "asm", "inline-clues", "audit", "summary",
                                 "structure", "source", "mac-show", "mac-disasm", "mac-calls"])
        self.assertEqual([s.name for s in evidence.select_sections(["sema", "show"], None)],
                         ["show", "summary", "structure", "source"])
        self.assertEqual([s.name for s in evidence.select_sections(None, ["mac,audit", "dc"])],
                         ["summary", "structure", "source"])
        with self.assertRaises(SystemExit):
            evidence.select_sections(["nonsense"], None)

    def test_capture_includes_child_processes_and_bound_streams(self):
        def entry(argv):
            print("python stdout")
            _bound_renderer("bound renderer")
            sys.stdout.flush()
            subprocess.run([sys.executable, "-c", "print('child stdout')"], check=True)
            print("warning", file=sys.stderr)
            return 1
        rc, out, err = evidence.run_captured(entry, [], merge=False)
        self.assertEqual(rc, 1)
        self.assertEqual(out.splitlines(), ["python stdout", "bound renderer", "child stdout"])
        self.assertEqual(err, "warning\n")
        rc, out, _ = evidence.run_captured(lambda argv: sys.exit("fatal"), [], merge=True)
        self.assertEqual((rc, out), (2, "fatal\n"))
        rc, out, _ = evidence.run_captured(lambda argv: 1 / 0, [], merge=True)
        self.assertEqual(rc, 2)
        self.assertIn("ZeroDivisionError", out)

    def test_unclaimed_va_skips_mac_and_flags_pass_through(self):
        fake = FakeCommands()
        sections = evidence.select_sections(["summary", "mac", "asm"], None)
        results = evidence.gather("0x00524dd0", sections, as_json=False, no_build=True,
                                  mac_check=lambda va: False, entry_for=fake.entry)
        self.assertEqual(fake.calls, [
            ("dreamcast", ["asm", "0x00524dd0", "--blocks"]),
            ("sema", ["diff", "0x00524dd0", "--summary", "--no-build"])])
        self.assertEqual([r.get("skipped") for r in results[2:]],
                         ["no MAC_ADDRESS claim pairs 0x00524dd0"] * 3)
        self.assertIn("note", results[0]["text"])  # stderr merged into the section text

    def test_failed_mac_show_skips_the_other_mac_sections(self):
        fake = FakeCommands({("mac", "show"): 2})
        results = evidence.gather("someName", evidence.select_sections(["mac"], None),
                                  as_json=False, no_build=False,
                                  mac_check=lambda va: True, entry_for=fake.entry)
        self.assertEqual(len(fake.calls), 1)
        self.assertEqual([r.get("skipped") for r in results],
                         [None, "mac show found no claimed pair", "mac show found no claimed pair"])

    def test_json_and_out_directory(self):
        fake = FakeCommands({("sema", "diff"): 1})
        sections = evidence.select_sections(["show", "structure", "summary"], None)
        reports = [{"selector": "0x00524dd0",
                    "sections": evidence.gather("0x00524dd0", sections, as_json=True,
                                                no_build=False, mac_check=lambda va: True,
                                                entry_for=fake.entry)}]
        show, summary, structure = reports[0]["sections"]
        self.assertEqual(show["data"]["argv"], ["show", "0x00524dd0", "--json"])
        self.assertEqual(summary["data"]["argv"], ["diff", "0x00524dd0", "--summary", "--json"])
        self.assertEqual(structure["text"], "sema diff 0x00524dd0 --structure\n")
        self.assertEqual(structure["stderr"], "note\n")
        self.assertEqual(evidence.overall_rc(reports), 1)
        # Audit coverage gaps (3) and findings plus gaps (4) are answers, as
        # is a section's 1; an audit 2 and any other section's 3 are failures.
        for name, code, overall in (("audit", 3, 1), ("audit", 4, 1), ("audit", 2, 2),
                                    ("summary", 3, 2), ("audit", 0, 0)):
            with self.subTest(name=name, code=code):
                self.assertEqual(evidence.overall_rc(
                    [{"sections": [{"name": name, "rc": code}]}]), overall)
        with tempfile.TemporaryDirectory() as raw:
            evidence.write_outputs(reports, Path(raw))
            self.assertEqual(sorted(p.name for p in Path(raw).iterdir()), [
                "0x00524dd0.show.json", "0x00524dd0.structure.stderr.txt",
                "0x00524dd0.structure.txt", "0x00524dd0.summary.json"])
            self.assertTrue(all("path" in section for section in reports[0]["sections"]))
            index = io.StringIO()
            evidence.render_index(reports, index)
            self.assertIn("0x00524dd0.summary.json", index.getvalue())

    def test_main_renders_sections_with_headers(self):
        fake = FakeCommands()
        out = io.StringIO()
        from homm3.analysis import dreamcast
        with patch.object(evidence, "_entry", fake.entry), \
                patch.object(evidence, "mac_claimed", return_value=False), \
                patch.object(dreamcast, "shared_corpus", contextlib.nullcontext), \
                contextlib.redirect_stdout(out):
            rc = evidence.main(["0x00524dd0", "dc:0x10", "--only", "lines,mac"])
        self.assertEqual(rc, 0)
        text = out.getvalue()
        self.assertIn("===== [lines] homm3 dreamcast lines 0x00524dd0 =====", text)
        self.assertIn("===== [lines] homm3 dreamcast lines dc:0x10 =====", text)
        self.assertIn("skipped: no MAC_ADDRESS claim pairs 0x00524dd0", text)
        self.assertIn("[evidence] summary", text)
        # A non-VA selector has no cheap pair check, so `mac show` decides.
        self.assertIn(("mac", ["show", "dc:0x10"]), fake.calls)


if __name__ == "__main__":
    unittest.main()
