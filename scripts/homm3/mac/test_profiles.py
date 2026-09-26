from pathlib import Path
import tempfile
import unittest

from homm3.mac import profiles


class TestMacUnitSettings(unittest.TestCase):
    def write(self, text):
        folder = self.enterContext(tempfile.TemporaryDirectory())
        root = Path(folder)
        (root / "config/mac").mkdir(parents=True)
        (root / "config/mac/units.toml").write_text(text)
        return root

    def test_shared_flags_override_and_dispositions(self):
        root = self.write('flags = ["-O3", "-nolink"]\n'
                          '[units.hero]\nflags = ["-O1", "-nolink"]\n'
                          '[units.winfile]\ndisposition = "platform_rewritten"\nevidence = "File Manager"\n')
        self.assertEqual(profiles.flags(root, "hero"), ("-O1", "-nolink"))
        self.assertEqual(profiles.flags(root, "town"), ("-O3", "-nolink"))
        self.assertEqual(profiles.dispositions(root), {"winfile": ("platform_rewritten", "File Manager")})

    def test_invalid_settings_fail(self):
        for text in ('flags = ["-O3"]\n',
                     'flags = ["-nolink"]\n[units.x]\nmode = "paired_bodies"\n',
                     'flags = ["-nolink"]\n[units.x]\ndisposition = "windows_only"\n',
                     'flags = ["-nolink"]\n[units.x]\ndisposition = "gone"\nevidence = "e"\n'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                profiles.dispositions(self.write(text))


if __name__ == "__main__":
    unittest.main()
