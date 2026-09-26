from pathlib import Path
import tempfile
import unittest

from homm3.mac import profiles


class TestMacProfiles(unittest.TestCase):
    def fixture(self, root):
        (root / "config/mac").mkdir(parents=True)
        (root / "config/units.toml").write_text(
            '[build]\nincludes=["include"]\n[flags]\nfixture=[]\n'
            '[[unit]]\nunit="test"\nsource="src/test.cpp"\nflags="fixture"\n')
        (root / "config/mac/units.toml").write_text(
            '[units.test]\nmode="paired_bodies"\nflags=["-O1", "-nolink"]\n')

    def test_unit_flags_load(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            self.fixture(root)
            self.assertEqual(profiles.load(root, "test").flags, ("-O1", "-nolink"))
            self.assertIsNone(profiles.load(root, "other"))

    def test_retired_header_settings_are_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            self.fixture(root)
            path = root / "config/mac/units.toml"
            path.write_text(path.read_text() + 'preamble="alternate.h"\n')
            with self.assertRaisesRegex(ValueError, "retired header settings"):
                profiles.load(root, "test")


if __name__ == "__main__":
    unittest.main()
