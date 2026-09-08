"""Check the river tile family against independently decoded retail writes."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class RiverTileTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-river-tile-hypotheses.py")
        self.source = (self.root / "src/rmg.cpp").read_text()

    def test_sixty_reviewed_alternatives_rebase(self):
        for refine in (False, True):
            axis = self.module.make_manifest(self.source, refine)["axes"][0]
            reviewed = {body for _, body in self.module.bodies(refine)}
            self.assertEqual(len(reviewed), 60)
            self.assertEqual(axis["find"], axis["options"][0]["replace"])
            self.assertEqual({row["replace"] for row in axis["options"]}, reviewed | {axis["find"]})
            for row in axis["options"]:
                body = row["replace"]
                self.assertNotIn("#pragma", body)
                self.assertIn("if (tile.m_terrain != 0)", body)
                self.assertIn("neighbour.m_tile.m_riverType == 0", body)
                changed = self.source.replace(axis["find"], body)
                for next_refine in (False, True):
                    rebased = self.module.make_manifest(changed, next_refine)["axes"][0]
                    self.assertEqual(rebased["options"][0]["replace"], body)
        with self.assertRaisesRegex(ValueError, "review the river tile writer"):
            self.module.make_manifest(self.source.replace(axis["find"], axis["find"].replace(
                "m_hasRiver", "m_riverTarget")))

    @unittest.skipUnless(shutil.which("g++"), "packed neighbourhood oracle needs g++")
    def test_all_variants_preserve_packed_fields_and_clip_both_neighbourhoods(self):
        header = (self.root / "include/rmg.h").read_text()
        terrain = (self.root / "include/rmg_terrain.h").read_text()

        def declaration(source, name):
            start = source.index("struct " + name + " {")
            return source[start:source.index("\n};", start) + 3]

        program = ["#include <cstring>\n#include <climits>\n#include <cstddef>\n"]
        # Native mocks supply storage only. Expected updates use raw retail
        # masks and distance tests, not the candidate's bitfields or clamps.
        bodies = dict(self.module.bodies()) | dict(self.module.bodies(True))
        for index, body in enumerate(bodies.values()):
            program += [f"namespace Case{index} {{\n",
                "enum TTerrainType { eTerrainDirt = 0 };\n",
                declaration(header, "TRmgGroundTile"), "\n",
                declaration(header, "TRmgGroundTileData"), "\n",
                declaration(terrain, "rmgTerrainTile"), "\n",
                "struct TPoint { int m_x, m_y; };\n",
                declaration(header, "TRmgZoneBounds"), "\n",
                "struct TRmgGridPoint { unsigned int m_x, m_y; };\n",
                "struct TRmgMapItem { unsigned char prefix[0x24]; TRmgGroundTile m_tile; TRmgGroundTileData m_tileData; unsigned int tail; };\n",
                "struct type_random_map { TRmgMapItem* m_mapItems; int m_mapWidth, m_mapHeight; };\n",
                "struct TRmgMapAdapter { type_random_map* m_map; void setTile(const TRmgGridPoint&, const rmgTerrainTile&); };\n",
                "int min(int a, int b) { return a < b ? a : b; }\n",
                "int max(int a, int b) { return a < b ? b : a; }\n",
                body, "\n", r"""
int check() {
    if (sizeof(TRmgMapItem) != 0x30 || sizeof(rmgTerrainTile) != 12
        || offsetof(TRmgMapItem, m_tile) != 0x24
        || offsetof(TRmgMapItem, m_tileData) != 0x28) return 1;
    const int sizes[][2] = {{1,1}, {1,5}, {5,1}, {2,3}, {5,5}, {7,6}};
    const int kinds[] = {0, 1, 7, 8, 15, 16, 17, 255, 256, -1, -16, INT_MIN, INT_MAX};
    const int frames[] = {0, 255, 256, -1, INT_MIN, INT_MAX};
    const unsigned char flips[][2] = {{0,0}, {0,1}, {1,0}, {1,1}, {2,255}, {255,2}};
    const unsigned int seeds[] = {0, 0xffffffffu, 0xaaaaaaaau, 0x55555555u, 0x40000000u};
    for (int size = 0; size != 6; ++size)
    for (int seed = 0; seed != 5; ++seed)
    for (int kind = 0; kind != 13; ++kind)
    for (int shape = 0; shape != 6; ++shape)
    for (int cell = 0; cell != sizes[size][0] * sizes[size][1]; ++cell) {
        TRmgMapItem items[42], expected[42];
        unsigned int initial[12];
        for (int item = 0; item != 42; ++item) {
            for (int word = 0; word != 12; ++word) initial[word] = seeds[(seed + item + word) % 5];
            std::memcpy(&items[item], initial, sizeof(initial));
        }
        std::memcpy(expected, items, sizeof(items));
        type_random_map map;
        map.m_mapItems = items; map.m_mapWidth = sizes[size][0]; map.m_mapHeight = sizes[size][1];
        TRmgGridPoint point; point.m_x = cell % map.m_mapWidth; point.m_y = cell / map.m_mapWidth;
        rmgTerrainTile tile; tile.m_terrain = kinds[kind]; tile.m_frame = frames[shape];
        tile.m_flipX = flips[shape][0]; tile.m_flipY = flips[shape][1];
        for (int item = 0; item != map.m_mapWidth * map.m_mapHeight; ++item) {
            unsigned int ground, flags;
            std::memcpy(&ground, &expected[item].m_tile, 4);
            std::memcpy(&flags, &expected[item].m_tileData, 4);
            if (item == cell) {
                ground = (ground & 0xfc003fffu)
                    | ((static_cast<unsigned int>(tile.m_terrain) & 15u) << 14)
                    | ((static_cast<unsigned int>(tile.m_frame) & 255u) << 18);
                flags = (flags & 0xdff9ffffu)
                    | ((static_cast<unsigned int>(tile.m_flipX) & 1u) << 17)
                    | ((static_cast<unsigned int>(tile.m_flipY) & 1u) << 18)
                    | (static_cast<unsigned int>(tile.m_terrain != 0) << 29);
            }
            int dx = item % map.m_mapWidth - static_cast<int>(point.m_x);
            int dy = item / map.m_mapWidth - static_cast<int>(point.m_y);
            if (tile.m_terrain != 0) {
                if (-1 <= dx && dx <= 1 && -1 <= dy && dy <= 1) flags |= 0x80000000u;
                if (-2 <= dx && dx <= 2 && -2 <= dy && dy <= 2 && !(ground & 0x3c000u))
                    flags &= 0xbfffffffu;
            }
            std::memcpy(&expected[item].m_tile, &ground, 4);
            std::memcpy(&expected[item].m_tileData, &flags, 4);
        }
        TRmgMapAdapter adapter; adapter.m_map = &map;
        adapter.setTile(point, tile);
        if (std::memcmp(items, expected, sizeof(items))) return 2;
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return {index + 1};\n" for index in range(len(bodies))]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-river-tile-test-") as raw:
            path = Path(raw)
            cpp, executable = path / "tile.cpp", path / "tile"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
