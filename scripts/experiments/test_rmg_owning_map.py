"""Allocation count, dimensions, ownership and throwing-array cleanup oracle."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class OwningMapConstruction(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_construction_and_cleanup(self):
        module = generator("generate-rmg-owning-map-family.py")
        forms = [row["replace"] for row in module.variants()]
        self.assertEqual(len(forms), 55)
        wrong = [forms[0].replace("width * height * levels", "width * height"),
                 forms[0].replace("m_ownsMapItems = 1", "m_ownsMapItems = 0"),
                 forms[0].replace("m_mapWidth = width", "m_mapWidth = height"),
                 forms[0].replace("m_numberLevels = levels", "m_numberLevels = 1")]
        text = "#include <cstddef>\n"
        for i, body in enumerate(forms + wrong):
            text += f"namespace Case{i} {{\n" + r"""
static int liveCells, attempts, throwAt;
struct TRmgMapItem {
    TRmgMapItem() { if (attempts++ == throwAt) throw 1; ++liveCells; }
    ~TRmgMapItem() { --liveCells; }
};
struct TPoint {
    int m_x, m_y;
    TPoint(int x, int y):m_x(x),m_y(y) {}
    int getX() const { return m_x; }
    int getY() const { return m_y; }
};
struct TRmgGridPoint {
    unsigned int m_x, m_y;
    TRmgGridPoint(const unsigned int& x, const unsigned int& y):m_x(x),m_y(y) {}
    unsigned int getX() const { return m_x; }
    unsigned int getY() const { return m_y; }
};
struct type_random_map {
    unsigned char m_ownsMapItems;
    TRmgMapItem* m_mapItems;
    int m_mapWidth, m_mapHeight, m_numberLevels;
    type_random_map(int width, int height, int levels);
    ~type_random_map() { delete[] m_mapItems; }
};
""" + body + r"""
bool check() {
    for (int w = 0; w <= 5; ++w) for (int h = 0; h <= 5; ++h)
    for (int z = 1; z <= 2; ++z) {
        int expected = 0;
        for (int k = 0; k < z; ++k) for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) ++expected;
        attempts = liveCells = 0; throwAt = -1;
        {
            type_random_map map(w, h, z);
            if (map.m_mapWidth != w || map.m_mapHeight != h
                || map.m_numberLevels != z || map.m_ownsMapItems != 1
                || liveCells != expected || attempts != expected) return false;
        }
        if (liveCells) return false;
        for (int failure = 0; failure < expected; ++failure) {
            attempts = liveCells = 0; throwAt = failure;
            bool threw = false;
            try { type_random_map map(w, h, z); }
            catch (int) { threw = true; }
            if (!threw || liveCells || attempts != failure + 1) return false;
        }
    }
    return true;
}
}
"""
        text += "int main() {\n"
        for i in range(len(forms)):
            text += f"if (!Case{i}::check()) return 1;\n"
        for i in range(len(forms), len(forms) + len(wrong)):
            text += f"if (Case{i}::check()) return 2;\n"
        text += "return 0;\n}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-owning-map-") as raw:
            root = Path(raw)
            source, binary = root / "maps.cpp", root / "maps"
            source.write_text(text)
            subprocess.run([shutil.which("g++"), "-std=c++98", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
