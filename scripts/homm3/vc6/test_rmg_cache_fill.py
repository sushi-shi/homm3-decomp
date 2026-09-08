"""Validate cache-fill candidates against adapter ordering and packed fields."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class CacheFillTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-cache-fill-hypotheses.py")
        self.source = (self.root / "src/rmg_terrain.cpp").read_text()

    def test_sixty_canonical_bodies_and_rebase_guard(self):
        payload = self.module.make_manifest(self.source)
        axis = payload["axes"][0]
        self.assertEqual(axis["find"], axis["options"][0]["replace"])
        self.assertEqual(len({row["replace"] for row in axis["options"]}), 60)
        with tempfile.TemporaryDirectory(prefix="rmg-cache-fill-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 60)
        for variant in variants:
            changed = variant.source.decode()
            self.assertEqual(len(self.module._source.find_definitions(changed, self.module.HELPER)), 1)
            body = axis["options"][variant.index]["replace"]
            self.assertEqual(body.count("m_adapter->getTile(point)"), 1)
            self.assertNotIn("#pragma", body)
            self.assertNotIn("inline", body)
            if variant.index % 7 == 0:
                rebased = self.module.make_manifest(changed)["axes"][0]
                self.assertEqual(rebased["find"], rebased["options"][0]["replace"])
        with self.assertRaisesRegex(ValueError, "review the cache fill"):
            self.module.make_manifest(self.source.replace(axis["find"], axis["find"].replace(
                "packed.m_initialized = 1", "packed.m_initialized = 0")))

    def test_ten_parents_cross_six_canonical_helper_orders(self):
        parents = [name for name, _ in self.module.bodies()][:10]
        payload = self.module.make_order_manifest(self.source, parents)
        self.assertEqual(len(payload["axes"][0]["options"]), 10)
        self.assertEqual(len(payload["axes"][1]["options"]), 6)
        with tempfile.TemporaryDirectory(prefix="rmg-cache-order-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 60)
        for variant in variants:
            changed = variant.source.decode()
            for name in ("initializePackedCell", "getPackedCell", "getTerrain", "getWidth", "getHeight"):
                self.assertEqual(len(self.module._source.find_definitions(changed, "rmgTerrainPainter::" + name)), 1)
        with self.assertRaisesRegex(ValueError, "review ten unique"):
            self.module.make_order_manifest(self.source, parents[:-1])

    @unittest.skipUnless(shutil.which("g++"), "packed-cell oracle needs g++")
    def test_every_body_preserves_tile_bits_and_acquires_storage_after_adapter(self):
        header = (self.root / "include/rmg_terrain.h").read_text()
        declarations = []
        for name in ("rmgTerrainTile", "TRmgPackedTerrainCell"):
            start = header.index("struct " + name + " {")
            declarations.append(header[start:header.index("\n};", start) + 3])
        program = ["#include <vector>\n#include <climits>\n"]
        bodies = list(self.module.bodies())
        for index, (_, body) in enumerate(bodies):
            program += [f"namespace Case{index} {{\n", "\n".join(declarations), r"""
struct TRmgGridPoint { unsigned int m_x, m_y; };
struct Adapter { virtual rmgTerrainTile getTile(const TRmgGridPoint&) = 0; };
struct rmgTerrainPainter {
    Adapter* m_adapter;
    std::vector<TRmgPackedTerrainCell> m_packedCells;
    void initializePackedCell(const TRmgGridPoint&, unsigned int);
};
bool equal(const TRmgPackedTerrainCell& a, const TRmgPackedTerrainCell& b) {
    return a.m_initialized == b.m_initialized && a.m_terrain == b.m_terrain
        && a.m_frame == b.m_frame && a.m_flipX == b.m_flipX && a.m_flipY == b.m_flipY
        && a.m_unknown14 == b.m_unknown14;
}
TRmgPackedTerrainCell seeded(unsigned int seed) {
    TRmgPackedTerrainCell cell;
    cell.m_initialized = seed; cell.m_terrain = seed + 3; cell.m_frame = seed + 11;
    cell.m_flipX = seed; cell.m_flipY = seed + 1; cell.m_unknown14 = seed;
    return cell;
}
struct Source : Adapter {
    rmgTerrainPainter* painter;
    rmgTerrainTile value;
    TRmgGridPoint seen;
    int calls;
    bool relocate;
    std::vector<TRmgPackedTerrainCell> retired;
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point) {
        ++calls; seen = point;
        if (relocate) {
            std::vector<TRmgPackedTerrainCell> fresh;
            for (unsigned int i = 0; i != 5; ++i) fresh.push_back(seeded(100 + i));
            retired.swap(painter->m_packedCells);
            painter->m_packedCells.swap(fresh);
        }
        return value;
    }
};
""", body, r"""
int check() {
    const int values[] = {0, 1, -1, 15, 16, 127, 128, 255, INT_MIN, INT_MAX};
    for (unsigned int seed = 0; seed != 10; ++seed)
    for (unsigned int slot = 0; slot != 5; ++slot)
    for (unsigned int relocate = 0; relocate != 2; ++relocate) {
        rmgTerrainPainter painter;
        for (unsigned int i = 0; i != 5; ++i) painter.m_packedCells.push_back(seeded(i));
        Source source; source.painter = &painter; source.calls = 0; source.relocate = relocate != 0;
        source.value.m_terrain = values[seed]; source.value.m_frame = values[(seed + 3) % 10];
        source.value.m_flipX = values[(seed + 5) % 10]; source.value.m_flipY = values[(seed + 7) % 10];
        painter.m_adapter = &source;
        TRmgGridPoint point = {seed * 2 + 1, slot * 3 + 2};
        painter.initializePackedCell(point, slot);
        if (source.calls != 1 || source.seen.m_x != point.m_x || source.seen.m_y != point.m_y
            || painter.m_adapter != &source || painter.m_packedCells.size() != 5) return 1;
        for (unsigned int i = 0; i != 5; ++i) {
            TRmgPackedTerrainCell expected = seeded((relocate ? 100 : 0) + i);
            if (i == slot) {
                expected.m_initialized = 1;
                expected.m_terrain = static_cast<unsigned int>(source.value.m_terrain) & 15;
                expected.m_frame = static_cast<unsigned int>(source.value.m_frame) & 127;
                expected.m_flipX = source.value.m_flipX & 1;
                expected.m_flipY = source.value.m_flipY & 1;
            }
            if (!equal(painter.m_packedCells[i], expected)) return 2;
            if (relocate && !equal(source.retired[i], seeded(i))) return 3;
        }
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return {index + 1};\n" for index in range(len(bodies))]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-cache-fill-oracle-") as raw:
            path = Path(raw)
            cpp, executable = path / "cache.cpp", path / "cache"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
