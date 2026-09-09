"""Independent integer-distance predicate oracle for size-binding forms."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class ConnectionOwnerTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native C++ oracle needs g++")
    def test_all_forms_and_negative_controls(self):
        module = generator("generate-rmg-connect-owners-family.py")
        root = Path(__file__).resolve().parents[2]
        original = module.definition((root / "src/rmg.cpp").read_text())
        forms = list(module.variants(original))
        self.assertEqual(len(forms), 60)
        self.assertEqual(len({body for _, body in forms}), 60)
        self.assertEqual(forms[0][1], original)
        forms += list(module.variants(original, snapshots=True))
        positive_count = len(forms)
        self.assertEqual(positive_count, 110)
        forms += [("wrong_level", original.replace("m_z != m_levelPosition.m_z", "m_z == m_levelPosition.m_z")),
                  ("wrong_strictness", original.replace("combinedSize > minimumSize", "combinedSize >= minimumSize")),
                  ("wrong_minimum", original.replace("otherSize < minimumSize", "otherSize > minimumSize")),
                  ("wrong_distance", original.replace("dx * dx + dy * dy", "dx * dx"))]
        text = "#include <cmath>\n#include <cstdio>\nusing std::sqrt;\n"
        text += "struct TRmgTownSlot { int m_size; }; struct Position { int m_x,m_y,m_z; };\n"
        for index, (_, body) in enumerate(forms):
            text += f"struct Zone{index} {{ Position m_levelPosition; TRmgTownSlot* m_slot;\n"
            text += body.replace("TRmgZone::", "").replace("const TRmgZone*", f"const Zone{index}*") + "\n};\n"
        text += r"""
template<class T> bool check() {
    int sizes[] = {-9,-3,-1,0,1,2,3,4,5,9,16,31,64};
    for(int a=0;a<13;++a) for(int b=0;b<13;++b)
    for(int dx=-9;dx<=9;++dx) for(int dy=-9;dy<=9;++dy)
    for(int z=0;z<2;++z) {
        TRmgTownSlot sa={sizes[a]}, sb={sizes[b]}; T first, second;
        first.m_slot=&sa; second.m_slot=&sb;
        first.m_levelPosition.m_x=3; first.m_levelPosition.m_y=-2; first.m_levelPosition.m_z=0;
        second.m_levelPosition.m_x=3+dx; second.m_levelPosition.m_y=-2+dy; second.m_levelPosition.m_z=z;
        int d=0; while((d+1)*(d+1)<=dx*dx+dy*dy) ++d;
        int total=sa.m_size+sb.m_size, small=sa.m_size < sb.m_size ? sa.m_size : sb.m_size;
        bool expected=z ? (total>=d && total-d>small/2) : (11*total>=10*d);
        if(bool(first.canConnect(&second))!=expected) return false;
        // Also test the identity/alias case, which reference bindings must preserve.
        if(bool(first.canConnect(&first)) != (sa.m_size>=0)) return false;
    }
    return true;
}
int main() {
"""
        for index in range(len(forms)):
            condition = f"!check<Zone{index}>()" if index < positive_count else f"check<Zone{index}>()"
            text += f'if({condition}) {{ std::printf("failed form {index}\\n"); return 1; }}\n'
        text += "return 0;}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-connect-owner-oracle-") as folder:
            source, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            source.write_text(text)
            subprocess.run(["g++", "-std=c++98", "-O2", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
