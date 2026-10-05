"""Hermetic controls: one unit normalized alone equals the same unit
normalized by the tree-wide pass."""
from __future__ import annotations

import contextlib
import io
import os
import shutil
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path

from homm3.build import normalize_objs
from homm3.build.test_equivalent_relocation_normalization import _base, _target
from homm3.build.normalized_freshness import stamp_path, write_stamp
from homm3.core import inputs

_needs_retail = unittest.skipUnless(inputs.is_staged(inputs.RETAIL),
                                    inputs.requires_staged(inputs.RETAIL))


class NormalizeUnitTest(unittest.TestCase):
    def setUp(self):
        self.enterContext(patch.object(normalize_objs, "retail_image_base", return_value=0x400000))
        # Process-wide indexes must not leak between fixtures.
        self.enterContext(patch.object(normalize_objs, "_ICF_INDEX", None))
        self.enterContext(patch.object(normalize_objs, "_RETAIL_TWINS", None))
        self.dir = tempfile.TemporaryDirectory()
        root = Path(self.dir.name)
        self.enterContext(patch.object(normalize_objs, 'DATA_MANIFEST', root / 'data.tsv'))
        self.enterContext(patch.object(normalize_objs, "FUNCLETS", root / "funclets.tsv"))
        self.enterContext(patch.object(normalize_objs, "FUNCTIONS", root / "functions.tsv"))
        (root / "functions.tsv").write_text("rva\tsize\n")
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

    @_needs_retail
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

    @_needs_retail
    def test_scoped_tree_pass_matches_the_complete_pass(self):
        with contextlib.redirect_stdout(io.StringIO()):
            counts = normalize_objs.normalize_all({"probe"})
        self.assertEqual(counts["wrote"], 2)
        scoped = self._normalized()
        self.assertFalse((self.objdiff / "normalized/base/lonely.obj").exists())
        shutil.rmtree(self.objdiff / "normalized")
        normalize_objs._ICF_INDEX = None
        with contextlib.redirect_stdout(io.StringIO()):
            normalize_objs.normalize_all()
        tree = self._normalized()
        self.assertIn("normalized/base/lonely.obj", tree)
        for key, data in scoped.items():
            self.assertEqual(tree[key], data, key)

    @_needs_retail
    def test_rewritten_object_invalidates_the_process_icf_index(self):
        # probe and lonely define the same external body; a header-only
        # change recompiles lonely with a different one.
        with contextlib.redirect_stdout(io.StringIO()):
            normalize_objs.normalize_all({"probe"})
        self.assertIn("probe", normalize_objs._icf_index()[0])
        (self.objdiff / "base" / "lonely.obj").write_bytes(_base(literal=0x00401024))
        with contextlib.redirect_stdout(io.StringIO()):
            normalize_objs.normalize_all()
        # The complete pass rewrote lonely, so the memo from the scoped pass
        # is gone and the rebuilt index sees the now-ambiguous body.
        self.assertNotIn("probe", normalize_objs._icf_index()[0])

    def test_icf_index_cache_reuses_only_unchanged_objects(self):
        normalize_objs.normalize_unit("probe")
        cold = normalize_objs._icf_index()
        self.assertTrue((self.objdiff.parent / "gen/cache/icf-bodies.pickle").is_file())
        normalize_objs._ICF_INDEX = None
        with patch.object(normalize_objs.canon, "CoffObject",
                          side_effect=AssertionError("unchanged object reparsed")):
            self.assertEqual(normalize_objs._icf_index(), cold)
        normalize_objs._ICF_INDEX = None
        changed = self.objdiff / "normalized/base/probe.obj"
        changed.write_bytes(changed.read_bytes() + b"\0")
        parse = normalize_objs.canon.CoffObject
        with patch.object(normalize_objs.canon, "CoffObject", wraps=parse) as parsed:
            normalize_objs._icf_index()
            parsed.assert_called_once()

    def test_retail_twin_index_cache_is_keyed_by_the_census(self):
        from types import SimpleNamespace
        image = SimpleNamespace(pe=SimpleNamespace(read=lambda rva, size: b"\xc3" * size))
        normalize_objs.FUNCTIONS.write_text("rva\tsize\n0x1000\t1\n0x1010\t1\n")
        with patch("homm3.delink.image.retail", return_value=image):
            first = normalize_objs._retail_twins()
            self.assertEqual(first(0x1000), [0x1010])
            normalize_objs._RETAIL_TWINS = None
            cached = normalize_objs._retail_twins()
            self.assertEqual(cached._index, first.index())
            normalize_objs._RETAIL_TWINS = None
            normalize_objs.FUNCTIONS.write_text("rva\tsize\n0x1000\t1\n")
            rebuilt = normalize_objs._retail_twins()
            self.assertEqual(rebuilt(0x1000), [])

    @_needs_retail
    def test_changed_funclet_inventory_invalidates_both_paired_stamps(self):
        normalize_objs.FUNCLETS.write_text("rva\tparent_rva\tstate\n")
        normalize_objs.FUNCTIONS.write_text("rva\tsize\n")
        normalize_objs.normalize_unit("probe")
        with patch.object(normalize_objs, "_retail_funclet_owners", return_value={}) as owners:
            normalize_objs.normalize_unit("probe")
            owners.assert_not_called()
            for inventory in (normalize_objs.FUNCLETS, normalize_objs.FUNCTIONS):
                with inventory.open("a") as stream:
                    stream.write("# changed admission\n")
                normalize_objs.normalize_unit("probe")
                owners.assert_called_once()
                owners.reset_mock()

    def test_unit_without_a_target_gets_only_its_base_copy(self):
        counts = normalize_objs.normalize_unit("lonely")
        self.assertEqual(counts["wrote"], 1)
        self.assertTrue((self.objdiff / "normalized/base/lonely.obj").is_file())
        self.assertFalse((self.objdiff / "normalized/target/lonely.c.obj").exists())

    def test_unknown_unit_is_a_no_op(self):
        self.assertEqual(normalize_objs.normalize_unit("nothing")["wrote"], 0)

    @_needs_retail
    def test_unchanged_pair_skips_coff_parsing_and_manifest_loading(self):
        normalize_objs.normalize_unit("probe")
        paths = list((self.objdiff / "normalized").rglob("*"))
        mtimes = {p: p.stat().st_mtime_ns for p in paths if p.is_file()}
        with patch.object(normalize_objs.canon, "CoffObject", side_effect=AssertionError("COFF parsed")), \
                patch.object(normalize_objs.canon, "load_compgen_claims", side_effect=AssertionError("claims parsed")):
            self.assertEqual(dict(normalize_objs.normalize_unit("probe")), {})
        self.assertEqual(mtimes, {p: p.stat().st_mtime_ns for p in mtimes})

    @_needs_retail
    def test_same_timestamp_rebuild_and_corruption_match_forced_normalization(self):
        normalize_objs.normalize_unit('probe')
        raw = self.objdiff / 'base/probe.obj'
        stat = raw.stat()
        raw.write_bytes(_base(literal=0x00401024))
        os.utime(raw, ns=(stat.st_atime_ns, stat.st_mtime_ns))
        normalize_objs.normalize_unit('probe')
        refreshed = self._normalized()
        shutil.rmtree(self.objdiff / 'normalized')
        normalize_objs.normalize_unit('probe')
        self.assertEqual(refreshed, self._normalized())
        normalized = self.objdiff / 'normalized/base/probe.obj'
        normalized.write_bytes(b'corrupt')
        normalize_objs.normalize_unit('probe')
        self.assertEqual(refreshed, self._normalized())

    @_needs_retail
    def test_project_input_change_invalidates_comparison_copies(self):
        root = self.objdiff.parent
        (root / 'config').mkdir()
        specification = root / 'config/project.toml'
        specification.write_text('[inputs.retail]\nimage_base=4194304\n')
        with patch.object(normalize_objs.common, 'HOMM3_DIR', root):
            normalize_objs.normalize_unit('probe')
            specification.write_text('[inputs.retail]\nimage_base=7340032\n')
            transform = normalize_objs._retain_matching_target_padding
            with patch.object(normalize_objs, '_retain_matching_target_padding', wraps=transform) as paired:
                normalize_objs.normalize_unit('probe')
                paired.assert_called_once()

    @_needs_retail
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

    @_needs_retail
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

    @_needs_retail
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
