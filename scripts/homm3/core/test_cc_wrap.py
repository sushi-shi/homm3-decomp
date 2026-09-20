from __future__ import annotations

import os
from pathlib import Path
import tempfile
import unittest


class WinePrefixAnchorTests(unittest.TestCase):
    """cc_wrap must pin WINEPREFIX to this tree's prefix.

    The guard used to be `if not Path(os.environ.get("WINEPREFIX", "")).is_dir()`.
    Path("") is PosixPath("."), whose is_dir() is True, so an ABSENT WINEPREFIX
    satisfied the guard and the compile silently ran against the shared ~/.wine
    prefix instead of build/wineprefix. Every unit then failed with
    `fatal error C1083: Cannot open include file: 'va.h'` even though INCLUDE
    named the right directories, and it only looked healthy while a correct
    wineserver happened to be running.
    """

    @staticmethod
    def anchor(environ_value, root):
        prefix = environ_value if environ_value is not None else ""
        if not (prefix and Path(prefix).is_dir()):
            return str(root / "build/wineprefix")
        return prefix

    def test_absent_prefix_is_pinned_to_the_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertEqual(self.anchor(None, root),
                             str(root / "build/wineprefix"))
            self.assertEqual(self.anchor("", root),
                             str(root / "build/wineprefix"))

    def test_stale_prefix_is_pinned_to_the_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertEqual(self.anchor(str(root / "gone"), root),
                             str(root / "build/wineprefix"))

    def test_existing_prefix_is_respected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            other = root / "other"
            other.mkdir()
            self.assertEqual(self.anchor(str(other), root), str(other))

    def test_the_real_guard_matches_this_contract(self):
        source = (Path(__file__).parent / "cc_wrap.py").read_text()
        self.assertIn('_prefix = os.environ.get("WINEPREFIX", "")', source)
        self.assertIn('if not (_prefix and Path(_prefix).is_dir()):', source)
        self.assertNotIn('if not Path(os.environ.get("WINEPREFIX", "")).is_dir()',
                         source)


if __name__ == "__main__":
    unittest.main()
