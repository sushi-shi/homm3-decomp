"""Dimension-accessor and initialized-cell semantics, independent of VC6."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class PainterAreaTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native C++ oracle needs g++")
    def test_all_forms_and_negative_controls(self):
        module = generator("generate-rmg-painter-area-family.py")
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/rmg_terrain.cpp").read_text()
        forms = list(module.variants())
        self.assertEqual(len(forms), 60)
        self.assertEqual(len({body for _, body in forms}), 60)
        original = forms[0][1]
        forms += [("wrong_height", original.replace("m_height = size.m_y", "m_height = size.m_x")),
                  ("wrong_area", original.replace("m_width * m_height", "m_width + m_height")),
                  ("wrong_strength", original.replace("m_transitionStrength(strength)", "m_transitionStrength(terrain)")),
                  ("wrong_query_count", original.replace("size = m_adapter->getSize();", "size = m_adapter->getSize();\n    size = m_adapter->getSize();"))]
        helper = generator("generate-rmg-position-family.py")
        accessors = "\n".join(helper.definition(source, "rmgTerrainPainter::" + name)
                              for name in ("getWidth", "getHeight"))
        text = "#include <vector>\n#include <cstdio>\n"
        for index, (_, body) in enumerate(forms):
            text += f"namespace Case{index} {{\n" + r"""
struct TRmgGridPoint { unsigned int m_x, m_y; };
// Only this bit is defined by the actual constructor. Do not inspect or
// require values for the packed cell's other, initially indeterminate bits.
struct TRmgPackedTerrainCell { bool m_initialized; TRmgPackedTerrainCell():m_initialized(false){} };
struct TRmgMapInterface {
    int m_queries; TRmgGridPoint m_size;
    virtual TRmgGridPoint getSize() { ++m_queries; return m_size; }
};
struct rmgTerrainPainter {
    TRmgMapInterface* m_adapter; int m_paintTerrain,m_transitionStrength;
    unsigned int m_width,m_height; std::vector<TRmgPackedTerrainCell> m_packedCells;
    rmgTerrainPainter(TRmgMapInterface*,int,int);
    unsigned int getWidth() const; unsigned int getHeight() const;
};
""" + body + "\n" + accessors + r"""
bool check() {
    const unsigned int dims[]={0,1,2,3,7,36,72};
    for(int x=0;x<7;++x) for(int y=0;y<7;++y) {
        TRmgMapInterface map; map.m_queries=0; map.m_size.m_x=dims[x]; map.m_size.m_y=dims[y];
        rmgTerrainPainter painter(&map,x-1,y-1);
        unsigned int count=0; for(unsigned int row=0;row<dims[y];++row) count+=dims[x];
        if(map.m_queries!=1 || painter.m_adapter!=&map || painter.getWidth()!=dims[x]
            || painter.getHeight()!=dims[y] || painter.m_paintTerrain!=x-1
            || painter.m_transitionStrength!=y-1 || painter.m_packedCells.size()!=count) return false;
        for(unsigned int cell=0;cell<count;++cell) if(painter.m_packedCells[cell].m_initialized) return false;
    }
    return true;
}
}
"""
        text += "int main(){\n"
        for index in range(len(forms)):
            condition = f"!Case{index}::check()" if index < 60 else f"Case{index}::check()"
            text += f'if({condition}) {{ std::printf("failed form {index}\\n"); return 1; }}\n'
        text += "return 0;}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-painter-area-oracle-") as folder:
            source, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            source.write_text(text)
            subprocess.run(["g++", "-std=c++98", "-O1", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
