"""Hermetic controls: one unit normalized alone equals the same unit
normalized by the tree-wide pass."""
from __future__ import annotations

import contextlib
import io
import shutil
import tempfile
import unittest
from pathlib import Path

from homm3.build import normalize_objs
from homm3.build.test_equivalent_relocation_normalization import _base, _target


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


if __name__ == "__main__":
    unittest.main()
