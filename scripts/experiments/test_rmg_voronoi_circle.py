"""Circle predicates against an independent four-by-four determinant."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class CircleTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_determinant_and_negative_controls(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-voronoi-circle-family.py")
        helper = generator("generate-rmg-position-family.py")
        source = (root / module.SOURCE).read_text()
        header = (root / "include/rmg.h").read_text()
        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]
        types = "\n".join(block(name) for name in ("TRmgVector", "TPoint"))
        orientation = helper.definition(source, "getRmgPointOrientation")
        forms = [option["replace"] for option in module.variants(source)]
        count = len(forms)
        seed = forms[0]
        forms += [seed.replace("determinant > 0", "determinant >= 0"),
                  seed.replace("- static_cast<__int64>(second", "+ static_cast<__int64>(second"),
                  seed.replace("point.m_y * point.m_y", "point.m_x * point.m_y"),
                  seed.replace("int pointArea = getRmgPointOrientation(first, second, third);", "int pointArea = 0;")]
        self.assertTrue(all(form != seed for form in forms[count:]))
        text = "#include <cstdio>\n"
        for index, form in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + orientation + "\n" + form.replace("__int64", "long long") + r"""
long long reference(TPoint* point) {
    long long matrix[4][4];
    for(int row=0;row<4;++row) {
        long long x=point[row].m_x,y=point[row].m_y;
        matrix[row][0]=x;matrix[row][1]=y;matrix[row][2]=x*x+y*y;matrix[row][3]=1;
    }
    long long determinant=0;
    for(int a=0;a<4;++a) for(int b=0;b<4;++b) if(b!=a)
    for(int c=0;c<4;++c) if(c!=a && c!=b)
    for(int d=0;d<4;++d) if(d!=a && d!=b && d!=c) {
        int inversions=(a>b)+(a>c)+(a>d)+(b>c)+(b>d)+(c>d);
        long long term=matrix[0][a]*matrix[1][b]*matrix[2][c]*matrix[3][d];
        determinant+=(inversions&1) ? -term : term;
    }
    return determinant;
}
bool check() {
    const int grid[]={-2,0,3};
    unsigned int state=0x721384abU;
    for(int sample=0;sample<8609;++sample) {
        TPoint point[4];
        int digits=sample;
        for(int p=0;p<4;++p) {
            if(sample<6561) {
                point[p].m_x=grid[digits%3];digits/=3;
                point[p].m_y=grid[digits%3];digits/=3;
            } else {
                state=1664525U*state+1013904223U;point[p].m_x=int(state%801)-400;
                state=1664525U*state+1013904223U;point[p].m_y=int(state%801)-400;
            }
        }
        bool expected=reference(point)>0;
        if(bool(isRmgPointInsideCircle(point[0],point[1],point[2],point[3]))!=expected) return false;
    }
    return true;
}
}
"""
        text += "int main(){\n"
        for index in range(len(forms)):
            condition = f"!Case{index}::check()" if index < count else f"Case{index}::check()"
            text += f'if({condition}) {{ std::printf("failed circle form {index}\\n"); return 1; }}\n'
        text += "return 0;}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-circle-oracle-") as folder:
            cpp,binary=Path(folder)/"oracle.cpp",Path(folder)/"oracle"
            cpp.write_text(text)
            subprocess.run(["g++","-std=c++98","-O1","-fno-elide-constructors",str(cpp),"-o",str(binary)],check=True)
            subprocess.run([str(binary)],check=True,timeout=20)


if __name__ == "__main__":
    unittest.main()
