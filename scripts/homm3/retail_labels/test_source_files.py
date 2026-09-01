import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.retail_labels import source


class SourceFilesTest(unittest.TestCase):
    def test_transient_hidden_directory_is_not_a_source(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "game.cpp").touch()
            (root / "crt0.c").touch()
            (root / "notes.cpp.bak").touch()
            (root / ".codex").mkdir()

            with mock.patch.object(source, "SRC_DIR", root):
                self.assertEqual(
                    [path.name for path in source.src_files()],
                    ["crt0.c", "game.cpp"],
                )


if __name__ == "__main__":
    unittest.main()
