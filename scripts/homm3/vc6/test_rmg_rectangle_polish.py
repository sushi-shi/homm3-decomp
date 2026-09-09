"""Check rectangle hypotheses with ordered, stateful terrain/frame queries."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class RectanglePolishTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-rectangle-polish-hypotheses.py")
        self.source = (self.root / "src/rmg_terrain.cpp").read_text()

    def test_sixty_reviewed_variants_rebase(self):
        axis = self.module.make_manifest(self.source)["axes"][0]
        self.assertEqual(axis["find"], axis["options"][0]["replace"])
        self.assertEqual(len({row["replace"] for row in axis["options"]}), 60)
        for index, option in enumerate(axis["options"]):
            body = option["replace"]
            self.assertEqual(body.count("TRmgGridPoint point;"), 1)
            self.assertEqual(body.count("selectBaseFrame("), 1)
            self.assertEqual(body.count("setTile("), 1)
            self.assertEqual(body.count("paintPoint("), 1)
            self.assertNotIn("initializePackedCell", body)
            self.assertNotIn("#pragma", body)
            if index % 7 == 0:
                changed = self.source.replace(axis["find"], body)
                rebased = self.module.make_manifest(changed)["axes"][0]
                self.assertEqual(rebased["options"][0]["replace"], body)
        with self.assertRaisesRegex(ValueError, "review the rectangle painter"):
            self.module.make_manifest(self.source.replace(axis["find"], axis["find"].replace(
                "++point.m_y", "point.m_y += 2")))

    def test_ten_parents_cross_six_loop_forms(self):
        labels = [name for name, _ in self.module.bodies() if name.endswith("+named")]
        self.assertEqual(len(labels), 10)
        payload = self.module.make_loop_manifest(self.source, labels)
        options = payload["axes"][0]["options"]
        self.assertEqual(len({option["replace"] for option in options}), 60)
        for loop in self.module.LOOPS:
            self.assertEqual(sum(option["name"].endswith("+" + loop) for option in options), 10)
        with self.assertRaisesRegex(ValueError, "review ten unique"):
            self.module.make_loop_manifest(self.source, labels[:-1])

    @unittest.skipUnless(shutil.which("g++"), "rectangle query oracle needs g++")
    def test_all_variants_keep_unsigned_bounds_and_ordered_operations(self):
        header = (self.root / "include/rmg_terrain.h").read_text()
        start = header.index("struct rmgTerrainTile {")
        tile = header[start:header.index("\n};", start) + 3]
        methods = []
        for name in ("getPaintTerrain", "isPaintTerrain"):
            found = self.module._source.find_definitions(self.source, "rmgTerrainPainter::" + name)
            self.assertEqual(len(found), 1)
            item = found[0]
            start = self.source.rfind("\n", 0, item.head) + 1
            methods.append(self.source[start:item.body_close + 1])
        program = ["#include <vector>\n#include <climits>\n"]
        parents = [name for name, _ in self.module.bodies() if name.endswith("+named")]
        follow_up = self.module.make_loop_manifest(self.source, parents)
        bodies = list(dict.fromkeys([body for _, body in self.module.bodies()]
                      + [row["replace"] for row in follow_up["axes"][0]["options"]]))
        for index, body in enumerate(bodies):
            program += [f"namespace Case{index} {{\n", tile, "\n", r"""
struct TRmgGridPoint { unsigned int m_x, m_y; };
struct Event {
    int kind, terrain, frame;
    unsigned int x, y;
    bool operator==(const Event& other) const {
        return kind == other.kind && terrain == other.terrain && frame == other.frame
            && x == other.x && y == other.y;
    }
};
void add(std::vector<Event>& events, int kind, unsigned int x, unsigned int y, int terrain, int frame) {
    Event event = {kind, terrain, frame, x, y}; events.push_back(event);
}
struct rmgTerrainPainter {
    int m_paintTerrain, queries, frames;
    std::vector<Event> events;
    int getPaintTerrain() const;
    unsigned char isPaintTerrain(const TRmgGridPoint&);
    int getTerrain(const TRmgGridPoint& point) {
        ++queries;
        int terrain = (point.m_x + 2 * point.m_y + queries) % 3;
        add(events, 1, point.m_x, point.m_y, terrain, 0); return terrain;
    }
    void paintPoint(const TRmgGridPoint& point) { add(events, 2, point.m_x, point.m_y, m_paintTerrain, 0); }
    int selectBaseFrame(const TRmgGridPoint& point, int terrain, int oldFrame) {
        ++frames; int frame = (point.m_x + 11 * point.m_y + 5 * terrain + 3 * frames) % 128;
        add(events, 3, point.m_x, point.m_y, terrain, oldFrame); return frame;
    }
    void setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile) {
        add(events, tile.m_flipX || tile.m_flipY ? -1 : 4,
            point.m_x, point.m_y, tile.m_terrain, tile.m_frame);
    }
    void paintRectangle(unsigned int, unsigned int, unsigned int, unsigned int);
};
""", "\n".join(methods), "\n", body, "\n", r"""
int check() {
    const unsigned int cases[][4] = {
        {0,0,0,0}, {0,0,0,5}, {0,0,5,0}, {0,0,1,1}, {0,0,4,5}, {2,3,4,5},
        {UINT_MAX,1,1,3}, {1,UINT_MAX,3,1}, {UINT_MAX-1,0,3,2}, {0,UINT_MAX-1,2,3}
    };
    for (unsigned int test = 0; test != 10; ++test)
    for (int terrain = 0; terrain != 3; ++terrain) {
        rmgTerrainPainter painter; painter.m_paintTerrain = terrain; painter.queries = painter.frames = 0;
        std::vector<Event> expected;
        unsigned int endX = cases[test][0] + cases[test][2], endY = cases[test][1] + cases[test][3];
        int queries = 0, frames = 0;
        for (unsigned int y = cases[test][1]; y < endY; ++y)
        for (unsigned int x = cases[test][0]; x < endX; ++x) {
            int kind = (x + 2 * y + ++queries) % 3;
            add(expected, 1, x, y, kind, 0);
            if (kind != terrain) add(expected, 2, x, y, terrain, 0);
            else {
                int frame = (x + 11 * y + 5 * terrain + 3 * ++frames) % 128;
                add(expected, 3, x, y, terrain, -1); add(expected, 4, x, y, terrain, frame);
            }
        }
        painter.paintRectangle(cases[test][0], cases[test][1], cases[test][2], cases[test][3]);
        if (painter.events != expected || painter.queries != queries || painter.frames != frames
            || painter.m_paintTerrain != terrain) return 1;
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return {index + 1};\n" for index in range(len(bodies))]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-rectangle-polish-test-") as raw:
            path = Path(raw)
            cpp, executable = path / "rectangle.cpp", path / "rectangle"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
