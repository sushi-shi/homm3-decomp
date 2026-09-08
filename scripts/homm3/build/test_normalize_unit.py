"""Hermetic controls: one unit normalized alone equals the same unit
normalized by the tree-wide pass."""
from __future__ import annotations

import contextlib
import io
import shutil
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path

from homm3.build import normalize_objs
from homm3.build.test_equivalent_relocation_normalization import _base, _target
from homm3.build.normalized_freshness import stamp_path, write_stamp


class NormalizeUnitTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        root = Path(self.dir.name)
        self.objdiff = root / "objdiff"
        (self.objdiff / "base").mkdir(parents=True)
        (self.objdiff / "target").mkdir(parents=True)
        (self.objdiff / "base" / "probe.obj").write_bytes(_base())
        (self.objdiff / "target" / "probe.c.obj").write_bytes(_target())
        (self.objdiff / "base" / "lonely.obj").write_bytes(_base())
        names = root / "symbol_names.csv"
        names.write_text("rva,name,unit,size,kind,provenance\n"
                         "0x1020,owner,probe,0x10,data,dc\n")
        self._saved = (normalize_objs.OBJDIFF, normalize_objs.SYMBOL_NAMES,
                       normalize_objs.COMPGEN_MANIFEST)
        normalize_objs.OBJDIFF = self.objdiff
        normalize_objs.SYMBOL_NAMES = names
        normalize_objs.COMPGEN_MANIFEST = root / "absent.tsv"

    def tearDown(self):
        (normalize_objs.OBJDIFF, normalize_objs.SYMBOL_NAMES,
         normalize_objs.COMPGEN_MANIFEST) = self._saved
        self.dir.cleanup()

    def _normalized(self):
        out = {}
        for path in sorted((self.objdiff / "normalized").rglob("*")):
            if path.is_file() and not path.name.endswith(".stamp.json"):
                out[str(path.relative_to(self.objdiff))] = path.read_bytes()
        return out

    def test_unit_alone_matches_the_tree_wide_pass(self):
        counts = normalize_objs.normalize_unit("probe")
        self.assertEqual(counts["wrote"], 2)
        alone = self._normalized()
        self.assertIn("normalized/base/probe.obj", alone)
        self.assertIn("normalized/target/probe.c.obj", alone)
        # a second call finds both copies fresh and rewrites nothing raw
        self.assertEqual(normalize_objs.normalize_unit("probe")["wrote"], 0)
        shutil.rmtree(self.objdiff / "normalized")
        with contextlib.redirect_stdout(io.StringIO()):
            normalize_objs.main([])
        tree = self._normalized()
        for key, data in alone.items():
            self.assertEqual(tree[key], data, key)

    def test_unit_without_a_target_gets_only_its_base_copy(self):
        counts = normalize_objs.normalize_unit("lonely")
        self.assertEqual(counts["wrote"], 1)
        self.assertTrue((self.objdiff / "normalized/base/lonely.obj").is_file())
        self.assertFalse((self.objdiff / "normalized/target/lonely.c.obj").exists())

    def test_unknown_unit_is_a_no_op(self):
        self.assertEqual(normalize_objs.normalize_unit("nothing")["wrote"], 0)

    def test_unchanged_pair_skips_coff_parsing_and_manifest_loading(self):
        normalize_objs.normalize_unit("probe")
        paths = list((self.objdiff / "normalized").rglob("*"))
        mtimes = {p: p.stat().st_mtime_ns for p in paths if p.is_file()}
        with patch.object(normalize_objs.canon, "CoffObject", side_effect=AssertionError("COFF parsed")), \
                patch.object(normalize_objs.canon, "load_compgen_claims", side_effect=AssertionError("claims parsed")):
            self.assertEqual(dict(normalize_objs.normalize_unit("probe")), {})
        self.assertEqual(mtimes, {p: p.stat().st_mtime_ns for p in mtimes})

    def test_each_paired_input_change_still_runs_transforms(self):
        normalize_objs.normalize_unit("probe")
        inputs = (self.objdiff / "base/probe.obj", self.objdiff / "target/probe.c.obj",
                  normalize_objs.SYMBOL_NAMES)
        transform = normalize_objs._retain_matching_target_padding
        for path in inputs:
            with self.subTest(path=path):
                payload = bytearray(path.read_bytes())
                if path.suffix == ".obj":
                    payload[4] ^= 1  # valid COFF, different timestamp field
                else:
                    payload.extend(b"\n")
                path.write_bytes(payload)
                with patch.object(normalize_objs, "_retain_matching_target_padding", wraps=transform) as paired:
                    normalize_objs.normalize_unit("probe")
                    paired.assert_called_once()
        # Introducing a previously absent claim manifest must also invalidate
        # a pair even though the old stamps could not record that input.
        normalize_objs.COMPGEN_MANIFEST.write_text("unit\tname\tkind\towner\tsize\n")
        with patch.object(normalize_objs, "_retain_matching_target_padding", wraps=transform) as paired:
            normalize_objs.normalize_unit("probe")
            paired.assert_called_once()

    def test_missing_or_raw_only_stamp_cannot_skip_pairing(self):
        transform = normalize_objs._retain_matching_target_padding
        for side, name in (("base", "probe.obj"), ("target", "probe.c.obj")):
            for raw_only in (False, True):
                with self.subTest(side=side, raw_only=raw_only):
                    normalize_objs.normalize_unit("probe")
                    out = self.objdiff / "normalized" / side / name
                    if raw_only:
                        write_stamp(out, {"raw": self.objdiff / side / name})
                    else:
                        stamp_path(out).unlink()
                    with patch.object(normalize_objs, "_retain_matching_target_padding", wraps=transform) as paired:
                        normalize_objs.normalize_unit("probe")
                        paired.assert_called_once()

    def test_changed_input_path_cannot_reuse_same_content_stamp(self):
        normalize_objs.normalize_unit("probe")
        replacement = normalize_objs.SYMBOL_NAMES.with_name("replacement.csv")
        replacement.write_bytes(normalize_objs.SYMBOL_NAMES.read_bytes())
        with patch.object(normalize_objs, "SYMBOL_NAMES", replacement), \
                patch.object(normalize_objs, "_retain_matching_target_padding",
                             wraps=normalize_objs._retain_matching_target_padding) as paired:
            normalize_objs.normalize_unit("probe")
            paired.assert_called_once()


if __name__ == "__main__":
    unittest.main()
