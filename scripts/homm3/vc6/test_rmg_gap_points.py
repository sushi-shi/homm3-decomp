"""Check gap predicate lifetimes without changing their short-circuit reads."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class RmgGapPointsTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-gap-points-hypotheses.py")
        self.source = (self.root / "src/rmg_terrain.cpp").read_text()

    def test_sixty_atomic_pairs_and_rebase_guard(self):
        payload = self.module.make_manifest(self.source)
        self.assertEqual(len(payload["axes"][0]["options"]), 60)
        with tempfile.TemporaryDirectory(prefix="rmg-gap-points-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 60)
        for variant in variants:
            changed = variant.source.decode()
            self.assertEqual(len(self.module.make_manifest(changed)["axes"][0]["options"]), 60)
            for kind in self.module.KINDS:
                self.assertEqual(len(self.module._source.find_definitions(
                    changed, "rmgTerrainPainter::is" + kind + "Gap")), 2)
            self.assertNotIn("#pragma", variant.source.decode()[variant.source.decode().index(
                self.module.signature("Horizontal")):].split("// Cardinal neighbours", 1)[0])
        original = self.module.canonical("Vertical")
        with self.assertRaisesRegex(ValueError, "review the Vertical gap predicate before"):
            self.module.make_manifest(self.source.replace(original, original.replace("point.m_y > 0", "point.m_y > 1")))

    @unittest.skipUnless(shutil.which("g++"), "gap predicate oracle needs g++")
    def test_unsigned_boundaries_ordered_queries_and_boolean_result(self):
        header = (self.root / "include/rmg.h").read_text()
        start = header.index("struct TRmgGridPoint {")
        point = header[start:header.index("\n};", start) + 3]
        program = ["#include <vector>\n"]
        pairs = list(self.module.pairs())
        for index, (_, pair) in enumerate(pairs):
            program += [f"namespace Case{index} {{\nstruct TPoint {{ int m_x, m_y; }};\n", point, r"""
struct Event {
    unsigned int tag, x, y;
    Event(unsigned int t, unsigned int a = 0, unsigned int b = 0) : tag(t), x(a), y(b) {}
    bool operator==(const Event& other) const { return tag == other.tag && x == other.x && y == other.y; }
};
struct rmgTerrainPainter {
    unsigned int width, height, reads;
    int firstValue, secondValue;
    std::vector<Event> events;
    unsigned int getWidth() { events.push_back(Event(1)); return width; }
    unsigned int getHeight() { events.push_back(Event(2)); return height; }
    int getTerrain(const TRmgGridPoint& point) {
        events.push_back(Event(3, point.m_x, point.m_y));
        return reads++ == 0 ? firstValue : secondValue;
    }
    unsigned char isHorizontalGap(const TRmgGridPoint&, int);
    unsigned char isVerticalGap(const TRmgGridPoint&, int);
};
""", pair[0], "\n", pair[1], r"""
int check() {
    const unsigned int values[] = {0, 1, 2, 7, 0xffffffffU};
    const int terrains[] = {-1, 0, 15};
    for (unsigned int w = 0; w != 5; ++w)
    for (unsigned int h = 0; h != 5; ++h)
    for (unsigned int x = 0; x != 5; ++x)
    for (unsigned int y = 0; y != 5; ++y)
    for (unsigned int a = 0; a != 3; ++a)
    for (unsigned int b = 0; b != 3; ++b)
    for (unsigned int t = 0; t != 3; ++t)
    for (unsigned int vertical = 0; vertical != 2; ++vertical) {
        rmgTerrainPainter painter;
        painter.width = values[w]; painter.height = values[h]; painter.reads = 0;
        painter.firstValue = terrains[a]; painter.secondValue = terrains[b];
        TRmgGridPoint point(values[x], values[y]);
        unsigned int coordinate = vertical ? values[y] : values[x];
        unsigned int limit = vertical ? values[h] : values[w];
        std::vector<Event> expected;
        unsigned int expectedReads = 0;
        unsigned char result = 0;
        if (coordinate != 0) {
            expected.push_back(Event(vertical ? 2 : 1));
            if (coordinate < limit - 1) {
                expected.push_back(Event(3, values[x] - (vertical ? 0 : 1), values[y] - (vertical ? 1 : 0)));
                ++expectedReads;
                if (a != t) {
                    expected.push_back(Event(3, values[x] + (vertical ? 0 : 1), values[y] + (vertical ? 1 : 0)));
                    ++expectedReads;
                    result = b != t;
                }
            }
        }
        unsigned char actual = vertical ? painter.isVerticalGap(point, terrains[t])
                                        : painter.isHorizontalGap(point, terrains[t]);
        if (actual != result || painter.reads != expectedReads || painter.events != expected) return 1;
        if (point.m_x != values[x] || point.m_y != values[y]) return 2;
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return {index + 1};\n" for index in range(len(pairs))]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-gap-points-oracle-") as raw:
            path = Path(raw)
            cpp, executable = path / "gaps.cpp", path / "gaps"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_ten_parents_cross_six_orders_and_rebase(self):
        parents = [name for name, _ in self.module.pairs()][:10]
        payload = self.module.make_order_manifest(self.source, parents)
        with tempfile.TemporaryDirectory(prefix="rmg-gap-points-order-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 60)
        for index, variant in enumerate(variants):
            changed = variant.source.decode()
            for kind in self.module.KINDS:
                self.assertEqual(len(self.module._source.find_definitions(
                    changed, "rmgTerrainPainter::is" + kind + "Gap")), 2)
            self.assertEqual(len(self.module._source.find_definitions(changed, "rmgTerrainPainter::paintTransitions")), 1)
            if index % 7 == 0:
                self.assertEqual(len(self.module.make_order_manifest(changed, parents)["axes"][0]["options"]), 60)
        with self.assertRaisesRegex(ValueError, "review ten unique"):
            self.module.make_order_manifest(self.source, parents[:-1])


if __name__ == "__main__":
    unittest.main()
