"""Independent flattened-cell oracle for canonical scalar lookup families."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class MapAccessorTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native C++ oracle needs g++")
    def test_all_source_forms_and_wrong_dimension_controls(self):
        module = generator("generate-rmg-map-accessor-family.py")
        root = Path(__file__).resolve().parents[2]
        original = module.definition((root / "include/rmg.h").read_text())
        forms = list(module.variants(original))
        self.assertEqual(len(forms), 60)
        self.assertEqual(len({body for _, body in forms}), 60)
        self.assertEqual(forms[0][1], original)
        for label, body in generator("generate-rmg-map-base-family.py").variants(original):
            if body not in {body for _, body in forms}:
                forms.append((label, body))
        positive_count = len(forms)
        forms += [("wrong_height", original.replace("z * m_mapHeight", "z * m_mapWidth")),
                  ("wrong_width", original.replace(
                      "(z * m_mapHeight + y) * m_mapWidth", "(z * m_mapHeight + y) * m_mapHeight")),
                  ("missing_level", original.replace("z * m_mapHeight + y", "y")),
                  ("wrong_x", original.replace("+ x;", "+ y;")),
                  ("mutated_base", original.replace("return m_mapItems +", "m_mapItems +=").replace("+ x;", "+ x;\n        return m_mapItems;"))]
        text = "#include <cstdio>\nstruct TRmgMapItem { int marker; };\n"
        header = (root / "include/rmg.h").read_text()
        for name in ("TRmgVector", "TPoint"):
            start = header.index("struct " + name + " {")
            text += header[start:header.index("\n};", start) + 3] + "\n"
        # An intentionally oversized allocation also keeps wrong controls'
        # arithmetic within one array. Marker/layout are fixture-only; pointer
        # offsets, not a substituted retail object layout, are the oracle.
        text += "static TRmgMapItem cells[200000];\n"
        for index, (_, body) in enumerate(forms):
            text += f"struct Map{index} {{ TRmgMapItem* m_mapItems; int m_mapWidth, m_mapHeight;\n{body}\n}};\n"
        text += "template<class T> bool check() { T map; map.m_mapItems = cells + 16;\n"
        text += "int sizes[] = {1,2,3,7,36,72,144};\n"
        text += "for (int wi=0; wi<7; ++wi) for(int hi=0; hi<7; ++hi) {\n"
        text += "map.m_mapWidth=sizes[wi]; map.m_mapHeight=sizes[hi];\n"
        text += "int xs[]={0,map.m_mapWidth/2,map.m_mapWidth-1}; int ys[]={0,map.m_mapHeight/2,map.m_mapHeight-1};\n"
        text += "for(int z=0;z<2;++z) for(int xi=0;xi<3;++xi) for(int yi=0;yi<3;++yi) {\n"
        text += "long expected=xs[xi]; for(int level=0;level<z;++level) expected += long(map.m_mapWidth)*map.m_mapHeight;\n"
        text += "for(int row=0;row<ys[yi];++row) expected += map.m_mapWidth;\n"
        text += "TRmgMapItem* before=map.m_mapItems; int width=map.m_mapWidth,height=map.m_mapHeight;\n"
        text += "if(map.getMapItem(xs[xi],ys[yi],z) != before+expected) return false;\n"
        text += "if(map.m_mapItems!=before || map.m_mapWidth!=width || map.m_mapHeight!=height) return false;\n}} return true; }\nint main(){\n"
        for index in range(len(forms)):
            condition = f"!check<Map{index}>()" if index < positive_count else f"check<Map{index}>()"
            text += f'if({condition}) {{ std::printf("failed form {index}\\n"); return 1; }}\n'
        text += f'std::puts("{positive_count} accessor forms: 882 coordinates each; five negative controls rejected");return 0;}}\n'
        with tempfile.TemporaryDirectory(prefix="rmg-map-accessor-oracle-") as folder:
            source, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            source.write_text(text)
            subprocess.run(["g++", "-std=c++11", "-O2", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
