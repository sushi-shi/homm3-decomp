"""Build-mode controls: full checkpoints refresh targets; iteration preserves them."""
from __future__ import annotations

import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.build import build, configure, delink, normalize_objs
from homm3.cleanliness import board
from homm3.core import inputs
from homm3.core.nb11 import NB11Error
from homm3.match import banked_rows, single_view, source_ownership, source_inventory, status, verify_va_claims


class BuildModeTest(unittest.TestCase):
    def setUp(self):
        directory = self.enterContext(tempfile.TemporaryDirectory())
        self.root = Path(directory)
        self.target = self.root / "build/objdiff/target/cursor.c.obj"
        self.target.parent.mkdir(parents=True)
        self.target.write_bytes(b"existing retail target")
        self.events = []
        self.mocks = {}
        self.preflight = self.enterContext(patch.object(inputs, "stage_executable"))
        self.enterContext(patch.object(build, "ROOT", self.root))
        self.enterContext(patch.object(status, "REPORT", self.root / "build/objdiff/report.json"))
        self.enterContext(patch.object(status, "overall_line", return_value="report"))
        self.enterContext(contextlib.redirect_stdout(io.StringIO()))
        self.stderr = self.enterContext(contextlib.redirect_stderr(io.StringIO()))

        for name, module, function, result in [
            ("configure", configure, "configure", None),
            ("compile", build, "_run", 0),
            ("delink", delink, "run", 0),
            ("normalize", normalize_objs, "normalize_all", 0),
            ("report", status, "refresh_report", {}),
            ("fingerprints", status, "source_hash_pair", ({}, {})),
            ("history", status, "baseline_history", ''),
            ("check", status, "cmd_check", None),
            ("checkpoint", status, "cmd_update", None),
            ("origins", source_ownership, "read_dc", []),
            ("banked", banked_rows, "run_gate", []),
            ("claims", verify_va_claims, "run_gate", []),
            ("single_view", single_view, "run_gate", []),
            ("ownership", source_ownership, "run_gate", []),
            ("inventory", source_inventory, "run_gate", []),
            ("cleanliness", board, "check_and_roll", []),
            ("readme", status, "write_readme", None),
        ]:
            def called(*args, _name=name, _result=result, **kwargs):
                self.events.append(_name)
                return _result
            self.mocks[name] = self.enterContext(patch.object(module, function, side_effect=called))

    def test_full_build_refreshes_existing_targets_before_checkpoint(self):
        self.assertEqual(build.main([]), 0)
        self.assertEqual(self.events, ["configure", "compile", "delink", "report",
                                      "fingerprints", "history", "check", "checkpoint", "banked", "claims",
                                      "single_view", "origins", "ownership", "inventory", "cleanliness", "readme"])
        self.mocks["compile"].assert_called_once_with("ninja")
        self.mocks["normalize"].assert_not_called()  # delink already normalizes
        self.assertEqual(self.preflight.call_count, 2)
        self.mocks["origins"].assert_called_once_with(include_declarations=True)
        self.mocks["ownership"].assert_called_once_with(origins=[])
        self.mocks["cleanliness"].assert_called_once_with(write=True, dc_origins=[])

    def test_missing_dc_evidence_preserves_independent_diagnostics_and_fails(self):
        for error in (inputs.InputError, NB11Error, FileNotFoundError):
            with self.subTest(error=error):
                self.mocks['origins'].side_effect = error('missing DC evidence')
                before = len(self.events)
                self.assertEqual(build.main([]), 1)
                events = self.events[before:]
                for gate in ('banked', 'claims', 'single_view', 'readme'):
                    self.assertIn(gate, events)
                self.mocks['ownership'].assert_not_called()
                self.mocks['cleanliness'].assert_not_called()
                self.assertIn('missing DC evidence', self.stderr.getvalue())
                self.assertIn('floors unchanged', self.stderr.getvalue())

    def test_unreadable_independent_gate_does_not_skip_later_gates(self):
        self.mocks['banked'].side_effect = OSError('unreadable banked evidence')
        self.assertEqual(build.main([]), 1)
        for gate in ('claims', 'single_view', 'ownership', 'readme'):
            self.mocks[gate].assert_called_once()
        self.mocks['cleanliness'].assert_called_once_with(write=False, dc_origins=[])

    def test_missing_pinned_input_fails_before_compile_or_ledger_changes(self):
        self.preflight.side_effect = inputs.InputError("Dreamcast executable missing")
        self.assertEqual(build.main([]), 1)
        self.assertEqual(self.events, [])

    def test_full_build_also_initializes_missing_targets(self):
        self.target.unlink()
        self.assertEqual(build.main([]), 0)
        self.mocks["delink"].assert_called_once_with()

    def test_failed_source_gate_still_refreshes_readme_and_remains_fatal(self):
        self.mocks["claims"].side_effect = lambda: ["invalid source claim"]
        self.assertEqual(build.main([]), 1)
        self.mocks["cleanliness"].assert_called_once_with(write=False, dc_origins=[])
        self.mocks["checkpoint"].assert_called_once()
        self.mocks["readme"].assert_called_once()

    def test_unexplained_dc_body_fails_the_full_checkpoint(self):
        self.mocks['inventory'].side_effect = lambda **kwargs: ['SOURCE-INVENTORY missing DC body']
        self.assertEqual(build.main([]), 1)
        self.mocks['inventory'].assert_called_once_with(origins=[])
        self.mocks['cleanliness'].assert_called_once_with(write=False, dc_origins=[])
        self.assertIn('SOURCE-INVENTORY', self.stderr.getvalue())

    def test_fast_build_preserves_targets_and_skips_checkpoint(self):
        self.assertEqual(build.main(["--fast", "cursor"]), 0)
        self.assertEqual(self.events, ["configure", "compile", "normalize", "configure", "report", "fingerprints"])
        self.mocks["compile"].assert_called_once_with("ninja", "cursor")
        self.assertEqual(self.target.read_bytes(), b"existing retail target")
        self.mocks["delink"].assert_not_called()
        self.mocks["checkpoint"].assert_not_called()
        self.preflight.assert_not_called()

    def test_fast_build_cannot_silently_bootstrap_a_delink(self):
        self.target.unlink()
        self.assertEqual(build.main(["--fast", "cursor"]), 1)
        self.assertEqual(self.events, [])

    def test_failed_compilation_cannot_delink_from_stale_objects(self):
        self.mocks["compile"].side_effect = lambda *args: 1
        self.assertEqual(build.main([]), 1)
        self.mocks["delink"].assert_not_called()
        self.mocks["report"].assert_not_called()

    def test_failed_delink_cannot_update_checkpoint(self):
        self.mocks["delink"].side_effect = RuntimeError("delink failed")
        with self.assertRaisesRegex(RuntimeError, "delink failed"):
            build.main([])
        self.mocks["report"].assert_not_called()
        self.mocks["checkpoint"].assert_not_called()

    def test_failed_normalization_cannot_report_a_fast_build(self):
        self.mocks["normalize"].side_effect = RuntimeError("normalization failed")
        with self.assertRaisesRegex(RuntimeError, "normalization failed"):
            build.main(["--fast", "cursor"])
        self.mocks["report"].assert_not_called()

    def test_removed_units_prune_cache_even_when_old_raw_objects_remain(self):
        raw = self.root / 'build/objdiff/base'
        raw.mkdir(parents=True)
        normalized = self.root / 'build/objdiff/normalized/base'
        normalized.mkdir(parents=True)
        for name in ('kept', 'removed'):
            (raw / (name + '.obj')).write_bytes(b'raw')
            for suffix in ('.obj', '.obj.stamp.json', '.symbols.tsv'):
                (normalized / (name + suffix)).write_bytes(b'cache')
        with patch.object(delink.common, 'HOMM3_DIR', self.root):
            delink._prune_normalized([{'unit': 'kept'}])
        self.assertEqual({p.name for p in normalized.iterdir()},
                         {'kept.obj', 'kept.obj.stamp.json', 'kept.symbols.tsv'})
        self.assertTrue((raw / 'removed.obj').exists())

    def test_prune_handles_nested_units_and_interrupted_stamp_writes(self):
        for side, obj_suffix in (('base', '.obj'), ('target', '.c.obj')):
            raw = self.root / 'build/objdiff' / side
            normalized = self.root / 'build/objdiff/normalized' / side
            expected = set()
            for unit in ('kept', 'nested/kept', 'removed/kept'):
                obj = raw / (unit + obj_suffix)
                obj.parent.mkdir(parents=True, exist_ok=True)
                obj.write_bytes(b'raw')
                cached = normalized / (unit + obj_suffix)
                cached.parent.mkdir(parents=True, exist_ok=True)
                for file in (cached, cached.with_suffix('.symbols.tsv'),
                             cached.with_name(cached.name + '.stamp.json')):
                    file.write_bytes(b'cache')
                    if not unit.startswith('removed/'):
                        expected.add(file)
            # Both legacy unnamed temps and new identifiable temps are debris.
            for name in ('tmp123abc', 'nested/.stamp-abcd.tmp'):
                (normalized / name).write_bytes(b'incomplete stamp')
            with patch.object(delink.common, 'HOMM3_DIR', self.root):
                delink._prune_normalized([{'unit': 'kept'}, {'unit': 'nested/kept'}])
            self.assertEqual({p for p in normalized.rglob('*') if p.is_file()}, expected)
            self.assertTrue((raw / ('removed/kept' + obj_suffix)).exists())


if __name__ == "__main__":
    unittest.main()
