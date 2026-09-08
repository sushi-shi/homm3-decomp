"""Check transition-strength point/query families with stateful cache replies."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class StrengthPolishTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-strength-polish-hypotheses.py")
        self.source = (self.root / "src/rmg_terrain.cpp").read_text()

    def test_sixty_reviewed_point_query_forms_rebase(self):
        axis = self.module.make_manifest(self.source)["axes"][0]
        self.assertEqual(axis["find"], axis["options"][0]["replace"])
        self.assertEqual(len({row["replace"] for row in axis["options"]}), 60)
        for row in axis["options"]:
            body = row["replace"]
            self.assertEqual(body.count("getTerrain(nearby)"), 4)
            self.assertEqual(body.count("getPackedCell(nearby)->getFrame()"), 4)
            self.assertEqual(body.count("strength >>= 1"), 4)
            self.assertNotIn("#pragma", body)
            changed = self.source.replace(axis["find"], body)
            self.assertEqual(self.module.make_manifest(changed)["axes"][0]["options"][0]["replace"], body)
        with self.assertRaisesRegex(ValueError, "review transition strength"):
            self.module.make_manifest(self.source.replace(axis["find"], axis["find"].replace("strength >>= 1", "strength /= 2")))

    def test_helper_order_preserves_one_canonical_definition_and_annotation(self):
        labels = [name for name, _ in self.module.bodies()][:10]
        payload = self.module.make_order_manifest(self.source, labels)
        self.assertEqual([len(axis["options"]) for axis in payload["axes"]], [10, 6])
        axis = payload["axes"][1]
        self.assertEqual(axis["options"][0]["replace"], axis["find"])
        names = ("getPackedCell", "getTerrain", "getWidth", "getHeight")
        originals = {}
        for name in names:
            item = self.module._source.find_definitions(self.source, "rmgTerrainPainter::" + name)[0]
            originals[name] = self.source[item.head:item.body_close + 1]
        for option in axis["options"]:
            changed = self.source.replace(axis["find"], option["replace"])
            for name in names:
                found = self.module._source.find_definitions(changed, "rmgTerrainPainter::" + name)
                self.assertEqual(len(found), 1)
                self.assertEqual(changed[found[0].head:found[0].body_close + 1], originals[name])
            self.assertEqual(changed.count("VA(0x005B48D0,"), 1)
            rebased = self.module.helper_order_axis(changed)
            self.assertEqual(rebased["find"], option["replace"])
            self.assertEqual(rebased["options"][0]["replace"], option["replace"])
        with self.assertRaisesRegex(ValueError, "review ten unique"):
            self.module.make_order_manifest(self.source, labels[:-1])

    @unittest.skipUnless(shutil.which("g++"), "strength query oracle needs g++")
    def test_all_variants_preserve_order_separate_queries_and_logical_halving(self):
        header = (self.root / "include/rmg.h").read_text()
        start = header.index("struct TRmgGridPoint {")
        grid = header[start:header.index("\n};", start) + 3]
        program = ["#include <vector>\n#include <climits>\n"]
        for index, (_, body) in enumerate(self.module.bodies()):
            program += [f"namespace Case{index} {{\n", "struct TPoint { int m_x, m_y; };\n", grid, "\n", r"""
std::vector<int> g_events;
int g_queries, g_mode, g_terrain;
struct TRmgPackedTerrainCell {
    int m_terrain, m_frame;
    int getTerrain() { return m_terrain; }
    int getFrame() { return m_frame; }
};
struct TRmgTerrainRule {
    virtual unsigned char isSpecialFrame(int frame) { g_events.push_back(10000 + frame); return frame % 2; }
};
TRmgTerrainRule g_rule;
TRmgTerrainRule* const g_rmgTerrainRules[] = {&g_rule, &g_rule, &g_rule};
struct rmgTerrainPainter {
    int m_transitionStrength;
    unsigned int m_width, m_height;
    TRmgPackedTerrainCell m_scratch;
    unsigned int getWidth() const { return m_width; }
    unsigned int getHeight() const { return m_height; }
    TRmgPackedTerrainCell* getPackedCell(const TRmgGridPoint& point) {
        g_events.push_back(point.m_y * 16 + point.m_x);
        ++g_queries;
        m_scratch.m_terrain = g_mode == 1 ? g_terrain : g_mode == 2 ? (g_terrain + 1) % 3
            : (point.m_x + 2 * point.m_y + g_queries) % 3;
        m_scratch.m_frame = (point.m_x * 5 + point.m_y * 3 + g_queries) % 7;
        return &m_scratch;
    }
    int getTerrain(const TRmgGridPoint& point) { return getPackedCell(point)->getTerrain(); }
    int getTransitionStrength(const TRmgGridPoint&, int);
};
""", body, "\n", r"""
int check() {
    const int strengths[] = {0, 1, 15, 128, -1, INT_MIN, INT_MAX};
    const int offsets[][2] = {{-1,0}, {0,-1}, {1,0}, {0,1}};
    rmgTerrainPainter painter;
    for (unsigned int width = 1; width != 6; ++width)
    for (unsigned int height = 1; height != 6; ++height)
    for (unsigned int x = 0; x != width; ++x)
    for (unsigned int y = 0; y != height; ++y)
    for (int mode = 0; mode != 3; ++mode)
    for (int terrain = 0; terrain != 3; ++terrain)
    for (int strength = 0; strength != 7; ++strength) {
        painter.m_width = width; painter.m_height = height;
        painter.m_transitionStrength = strengths[strength];
        std::vector<int> expectedEvents;
        unsigned int expected = strengths[strength];
        int queries = 0;
        for (int direction = 0; direction != 4; ++direction) {
            int nx = static_cast<int>(x) + offsets[direction][0];
            int ny = static_cast<int>(y) + offsets[direction][1];
            if (nx < 0 || ny < 0 || nx >= static_cast<int>(width) || ny >= static_cast<int>(height)) continue;
            expectedEvents.push_back(ny * 16 + nx); ++queries;
            int kind = mode == 1 ? terrain : mode == 2 ? (terrain + 1) % 3 : (nx + 2 * ny + queries) % 3;
            if (kind != terrain) continue;
            expectedEvents.push_back(ny * 16 + nx); ++queries;
            int frame = (nx * 5 + ny * 3 + queries) % 7;
            expectedEvents.push_back(10000 + frame);
            if (frame % 2) expected >>= 1;
        }
        g_events.clear(); g_queries = 0; g_mode = mode; g_terrain = terrain;
        TRmgGridPoint point(x, y);
        int actual = painter.getTransitionStrength(point, terrain);
        if (static_cast<unsigned int>(actual) != expected || g_events != expectedEvents || g_queries != queries) return 1;
        if (point.m_x != x || point.m_y != y) return 2;
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return {index + 1};\n" for index in range(60)]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-strength-polish-test-") as raw:
            path = Path(raw)
            cpp, executable = path / "strength.cpp", path / "strength"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
