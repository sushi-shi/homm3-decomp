"""Check the painter dimension matrix without reading uninitialized packed bits."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class PainterSizePolishTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-painter-size-polish-hypotheses.py")
        self.source = (self.root / "src/rmg_terrain.cpp").read_text()

    def test_sixty_dimension_variants_keep_initializers_query_and_resize(self):
        axis = self.module.make_manifest(self.source)["axes"][0]
        self.assertEqual(axis["find"], axis["options"][0]["replace"])
        self.assertEqual(len({row["replace"] for row in axis["options"]}), 60)
        for row in axis["options"]:
            body = row["replace"]
            self.assertEqual(body.count("m_adapter->getSize()"), 1)
            self.assertEqual(body.count("m_packedCells.resize("), 1)
            self.assertNotIn("#pragma", body)
            changed = self.source.replace(axis["find"], body)
            self.assertEqual(self.module.make_manifest(changed)["axes"][0]["options"][0]["replace"], body)
        with self.assertRaisesRegex(ValueError, "review the painter dimension setup"):
            self.module.make_manifest(self.source.replace(axis["find"], axis["find"].replace(
                "m_width * m_height", "m_width + m_height")))

    @unittest.skipUnless(shutil.which("g++"), "painter dimension oracle needs g++")
    def test_all_variants_keep_dimensions_count_and_invalid_initial_cells(self):
        # The native packed-cell mock exposes only the initialized bit. Its
        # other retail bits are indeterminate until a tile is filled, so no
        # oracle may demand values for them or copy them into expected output.
        program = ["#include <vector>\n"]
        for index, (_, body) in enumerate(self.module.bodies()):
            program += [f"namespace Case{index} {{\n", r"""
struct TRmgGridPoint { unsigned int m_x, m_y; };
struct TRmgPackedTerrainCell { bool m_initialized; TRmgPackedTerrainCell() : m_initialized(false) {} };
struct TRmgMapInterface {
    int m_queries;
    TRmgGridPoint m_size;
    virtual TRmgGridPoint getSize() { ++m_queries; return m_size; }
};
struct rmgTerrainPainter {
    TRmgMapInterface* m_adapter;
    int m_paintTerrain, m_transitionStrength;
    unsigned int m_width, m_height;
    std::vector<TRmgPackedTerrainCell> m_packedCells;
    rmgTerrainPainter(TRmgMapInterface*, int, int);
};
""", body, "\n", r"""
int check() {
    const unsigned int dimensions[] = {0, 1, 2, 3, 7, 36, 72};
    for (int width = 0; width != 7; ++width)
    for (int height = 0; height != 7; ++height) {
        TRmgMapInterface map; map.m_queries = 0;
        map.m_size.m_x = dimensions[width]; map.m_size.m_y = dimensions[height];
        rmgTerrainPainter painter(&map, width - 1, height - 1);
        if (map.m_queries != 1 || painter.m_adapter != &map
            || painter.m_width != map.m_size.m_x || painter.m_height != map.m_size.m_y
            || painter.m_paintTerrain != width - 1 || painter.m_transitionStrength != height - 1
            || painter.m_packedCells.size() != dimensions[width] * dimensions[height]) return 1;
        for (unsigned int cell = 0; cell != painter.m_packedCells.size(); ++cell)
            if (painter.m_packedCells[cell].m_initialized) return 2;
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return {index + 1};\n" for index in range(60)]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-painter-size-polish-test-") as raw:
            path = Path(raw)
            cpp, executable = path / "painter.cpp", path / "painter"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
