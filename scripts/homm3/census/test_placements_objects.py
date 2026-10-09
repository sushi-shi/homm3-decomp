"""`placements` refuses to derive a table from an image compile lacking unit objects."""
import io
import tempfile
import unittest
from contextlib import redirect_stderr
from pathlib import Path
from unittest import mock

from homm3 import manifest
from homm3.census import placements
from homm3.core import paths

UNITS = [{"unit": "ToolkitWnd", "source": "src/h3maped/ToolkitWnd.cpp"},
         {"unit": "BlackBox", "source": "src/h3maped/BlackBox.cpp"}]


class MissingObjectsTest(unittest.TestCase):
    def setUp(self):
        scratch = tempfile.TemporaryDirectory()
        self.addCleanup(scratch.cleanup)
        self.root = Path(scratch.name)
        (self.root / "build/objdiff/base").mkdir(parents=True)
        (self.root / "retail").mkdir()
        self.table = self.root / "retail/placements.tsv"
        self.table.write_text("# committed\n0x00001000\t0x10\tfunc\t?f@@YAXXZ\tBlackBox\t-\n")
        for patcher in (mock.patch.object(paths, "BUILD", self.root / "build"),
                        mock.patch.object(paths, "is_game", return_value=False),
                        mock.patch.object(paths, "image_key", return_value="h3maped"),
                        mock.patch.object(paths, "retail_dir", return_value=self.root / "retail"),
                        mock.patch.object(manifest, "units", return_value=UNITS)):
            patcher.start()
            self.addCleanup(patcher.stop)

    def compile(self, unit):
        (self.root / "build/objdiff/base" / f"{unit}.obj").write_bytes(b"")

    def test_a_partial_compile_names_the_missing_units(self):
        self.compile("ToolkitWnd")
        with self.assertRaises(placements.MissingObjects) as caught:
            placements.require_objects(UNITS)
        self.assertEqual(caught.exception.units, ["BlackBox"])
        self.assertIn("homm3 --image h3maped build", str(caught.exception))

    def test_a_complete_compile_passes(self):
        for unit in UNITS:
            self.compile(unit["unit"])
        placements.require_objects(UNITS)

    def test_write_with_no_compiled_bodies_fails_and_keeps_the_table(self):
        before = self.table.read_text()
        stderr = io.StringIO()
        with redirect_stderr(stderr):
            self.assertEqual(placements.main(["--write"]), 1)
        self.assertEqual(self.table.read_text(), before)
        self.assertIn("2 unit object(s) missing", stderr.getvalue())

    def test_check_with_no_compiled_bodies_fails(self):
        with redirect_stderr(io.StringIO()):
            self.assertEqual(placements.main(["--check"]), 1)


if __name__ == "__main__":
    unittest.main()
