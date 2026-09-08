"""Check reviewed overlay alternatives against retail's packed-bit operations."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class RiverOverlayTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-river-overlay-hypotheses.py")
        self.source = (self.root / "src/rmg.cpp").read_text()

    def test_sixty_unique_reviewed_alternatives_rebase(self):
        payload = self.module.make_manifest(self.source)
        axis = payload["axes"][0]
        self.assertEqual(axis["find"], axis["options"][0]["replace"])
        self.assertEqual(len({option["replace"] for option in axis["options"]}), 60)
        for option in axis["options"]:
            body = option["replace"]
            self.assertIn("m_hasRiver", body)
            self.assertNotIn("m_riverTarget", body)
            self.assertNotIn("#pragma", body)
            changed = self.source.replace(axis["find"], body)
            rebased = self.module.make_manifest(changed)["axes"][0]
            self.assertEqual(rebased["find"], body)
            self.assertEqual(rebased["options"][0]["replace"], body)
        with self.assertRaisesRegex(ValueError, "review the river-presence writer"):
            self.module.make_manifest(self.source.replace(axis["find"], axis["find"].replace(
                "m_hasRiver", "m_riverTarget")))

    @unittest.skipUnless(shutil.which("g++"), "packed-bit oracle needs g++")
    def test_all_variants_preserve_other_bits_and_test_full_input(self):
        header = (self.root / "include/rmg.h").read_text()

        def declaration(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]

        # Use the actual bitfield declarations; expected writes below are
        # independently transcribed from retail 0x532753..0x532780. The mock
        # map only supplies indexed cells and never enters a VC6 source tree.
        program = ["#include <cstring>\n#include <climits>\n#include <cstddef>\n"]
        for index, (_, body) in enumerate(self.module.bodies()):
            program.extend([f"namespace Case{index} {{\n",
                "enum TTerrainType { eTerrainDirt = 0 };\n",
                declaration("TRmgGroundTile"), "\n",
                declaration("TRmgGroundTileData"), "\n",
                "struct TRmgGridPoint { unsigned int m_x, m_y; };\n",
                "struct TRmgMapItem { unsigned char prefix[0x24]; TRmgGroundTile m_tile; TRmgGroundTileData m_tileData; unsigned int tail; };\n",
                "struct type_random_map { TRmgMapItem* m_mapItems; int m_mapWidth; };\n",
                "struct TRmgMapAdapter { type_random_map* m_map; void setOverlay(const TRmgGridPoint&, int); };\n",
                body, "\n", r"""
int check() {
    if (sizeof(unsigned int) != 4 || sizeof(TRmgMapItem) != 0x30
        || offsetof(TRmgMapItem, m_tile) != 0x24
        || offsetof(TRmgMapItem, m_tileData) != 0x28) return 1;
    const int values[] = {0, 1, 2, 7, 8, 15, 16, 17, 255, 256, -1, -16, INT_MIN, INT_MAX};
    const unsigned int seeds[] = {0, 0xffffffffu, 0xaaaaaaaau, 0x55555555u, 0x40000000u};
    for (unsigned int seed = 0; seed != sizeof(seeds) / sizeof(seeds[0]); ++seed)
    for (unsigned int value = 0; value != sizeof(values) / sizeof(values[0]); ++value)
    for (unsigned int cell = 0; cell != 6; ++cell) {
        TRmgMapItem items[6], expected[6];
        unsigned int before[12];
        for (int word = 0; word != 12; ++word) before[word] = seeds[seed];
        for (int item = 0; item != 6; ++item) std::memcpy(&items[item], before, sizeof(before));
        std::memcpy(expected, items, sizeof(items));
        unsigned int kind = (seeds[seed] & 0xfffc3fffu)
            | ((static_cast<unsigned int>(values[value]) & 15u) << 14);
        unsigned int flags = (seeds[seed] & 0xdfffffffu)
            | (static_cast<unsigned int>(values[value] != 0) << 29);
        std::memcpy(&expected[cell].m_tile, &kind, sizeof(kind));
        std::memcpy(&expected[cell].m_tileData, &flags, sizeof(flags));
        type_random_map map; map.m_mapItems = items; map.m_mapWidth = 3;
        TRmgMapAdapter adapter; adapter.m_map = &map;
        TRmgGridPoint point; point.m_x = cell % 3; point.m_y = cell / 3;
        adapter.setOverlay(point, values[value]);
        if (std::memcmp(items, expected, sizeof(items))) return 2;
    }
    return 0;
}
}
"""])
        program.append("int main() {\n")
        program.extend(f"if (Case{index}::check()) return {index + 1};\n" for index in range(60))
        program.append("return 0;\n}\n")
        with tempfile.TemporaryDirectory(prefix="rmg-river-overlay-test-") as raw:
            path = Path(raw)
            cpp, executable = path / "overlay.cpp", path / "overlay"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
