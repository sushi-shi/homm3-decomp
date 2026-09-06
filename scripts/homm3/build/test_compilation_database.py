"""Editor commands preserve TU ABI and keep VC6 build flags separate."""
from pathlib import Path
import unittest

from homm3.build.compilation_database import commands


class CompilationDatabaseTest(unittest.TestCase):
    def test_tu_language_defines_runtime_and_paths(self):
        data = {"flags": {"game": ["/O2", "/GX", "/Gr", "/MT", "/D_WINDOWS"],
                          "zlib": ["/O2", "/TC", "/DNO_ERRNO_H"]},
                "unit": [{"unit": "game", "source": "src/my game.cpp", "flags": "game"},
                         {"unit": "zlib", "source": "vendor/a.c", "flags": "zlib"}]}
        rows = commands(data, Path("/repo with spaces"), "/bin/clang", [Path("/sdk mirror")])
        self.assertEqual(rows[0]["arguments"][-1], "/repo with spaces/src/my game.cpp")
        for flag in ("/EHsc", "/Gr", "/MT", "/D_WINDOWS"):
            self.assertIn(flag, rows[0]["arguments"])
        self.assertNotIn("/GX", rows[0]["arguments"])
        self.assertNotIn("/O2", rows[0]["arguments"])
        self.assertIn("/TC", rows[1]["arguments"])
        self.assertIn("/DNO_ERRNO_H", rows[1]["arguments"])


if __name__ == "__main__":
    unittest.main()
