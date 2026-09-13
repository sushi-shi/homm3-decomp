"""Segment membership against independent collinearity and projection bounds."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class SegmentTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_segment_and_negative_controls(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-voronoi-segment-family.py")
        helper = generator("generate-rmg-position-family.py")
        source = (root / module.SOURCE).read_text()
        header = (root / "include/rmg.h").read_text()
        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]
        types = "struct TRmgZone {};\n" + "\n".join(block(name) for name in (
            "TRmgVector", "TPoint", "TRmgBoundaryVertex"))
        methods = "\n".join(helper.definition(source, name, **args) for name, args in (
            ("TRmgBoundaryVertex::initialize", {}),
            ("TRmgBoundaryVertex::TRmgBoundaryVertex", dict(parameters="TPoint sitePosition, TRmgZone* zone, TRmgBoundaryVertex* twin")),
            ("TRmgBoundaryVertex::TRmgBoundaryVertex", dict(parameters="TPoint sitePosition, TRmgZone* zone, TPoint twinSitePosition, TRmgZone* twinZone")),
            ("getRmgPointOrientation", {}), ("getRmgSquaredDistance", {})))
        use_object = bool(os.environ.get("HOMM3_SEGMENT_LINE_OBJECT"))
        forms = [option["replace"] for option in module.variants(source, use_object)]
        count = len(forms)
        seed = forms[1]
        expression = ("m_constant - point.m_y * m_dx + point.m_x * m_dy == 0" if use_object else
                      "constant - point.m_y * dx + point.m_x * dy == 0")
        forms += [seed.replace("firstDistance <= edgeDistance", "firstDistance < edgeDistance"),
                  seed.replace("firstDistance <= edgeDistance", "true"),
                  seed.replace("secondDistance <= edgeDistance", "true"),
                  seed.replace(expression, expression.replace("==", "!=")),
                  seed.replace(expression, expression.replace("+ point.m_x", "- point.m_x"))]
        self.assertTrue(all(form != seed for form in forms[count:]))
        text = "#include <cstdio>\n"
        for index, form in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + methods + "\n" + form + r"""
bool check() {
    const int coordinates[]={-400,-200,-1,0,1,200,400};
    TRmgZone firstZone,secondZone;
    TRmgBoundaryVertex edge(TPoint(0,0),&firstZone,TPoint(0,0),&secondZone);
    bool okay=true;
    for(int ax=0;ax<7;++ax) for(int ay=0;ay<7;++ay)
    for(int bx=0;bx<7;++bx) for(int by=0;by<7;++by)
    for(int px=0;px<7;++px) for(int py=0;py<7;++py) {
        TPoint a(coordinates[ax],coordinates[ay]);
        TPoint b(coordinates[bx],coordinates[by]);
        TPoint p(coordinates[px],coordinates[py]);
        edge.m_sitePosition=a; edge.m_twin->m_sitePosition=b;
        long long dx=(long long)b.m_x-a.m_x,dy=(long long)b.m_y-a.m_y;
        long long x=(long long)p.m_x-a.m_x,y=(long long)p.m_y-a.m_y;
        long long projection=x*dx+y*dy,length=dx*dx+dy*dy;
        bool expected=length ? x*dy==y*dx && projection>=0 && projection<=length : x==0 && y==0;
        if(bool(isRmgPointOnSegment(p,&edge))!=expected
            || !isRmgPointOnSegment(edge.m_sitePosition,&edge)
            || !isRmgPointOnSegment(edge.m_twin->m_sitePosition,&edge)
            || edge.m_sitePosition!=a || edge.m_twin->m_sitePosition!=b
            || edge.m_zone!=&firstZone || edge.m_twin->m_zone!=&secondZone) okay=false;
    }
    delete edge.m_twin;
    return okay;
}
}
"""
        text += "int main(){\n"
        for index in range(len(forms)):
            condition = f"!Case{index}::check()" if index < count else f"Case{index}::check()"
            text += f'if({condition}) {{ std::printf("failed segment form {index}\\n"); return 1; }}\n'
        text += "return 0;}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-segment-oracle-") as folder:
            cpp,binary=Path(folder)/"oracle.cpp",Path(folder)/"oracle"
            cpp.write_text(text)
            subprocess.run(["g++","-std=c++98","-O1","-fno-elide-constructors",str(cpp),"-o",str(binary)],check=True)
            subprocess.run([str(binary)],check=True,timeout=20)


if __name__ == "__main__":
    unittest.main()
