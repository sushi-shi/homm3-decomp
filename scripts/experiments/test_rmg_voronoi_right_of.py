"""Independent determinant-sign controls for the shared edge-side hypothesis."""
import itertools
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class RightOfTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_predicates_and_negative_controls(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-voronoi-right-of-family.py")
        helper = generator("generate-rmg-position-family.py")
        source = (root / module.SOURCE).read_text()
        header = (root / "include/rmg.h").read_text()
        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]
        types = "struct TRmgZone {};\n" + "\n".join(block(name) for name in (
            "TRmgVector", "TPoint", "TRmgBoundaryVertex"))
        methods = "\n".join(helper.definition(source, name, **args) for name, args in (
            ("TRmgBoundaryVertex::TRmgBoundaryVertex", dict(parameters="TPoint sitePosition, TRmgZone* zone, TRmgBoundaryVertex* twin")),
            ("TRmgBoundaryVertex::TRmgBoundaryVertex", dict(parameters="TPoint sitePosition, TRmgZone* zone, TPoint twinSitePosition, TRmgZone* twinZone")),
            ("getRmgPointOrientation", {})))
        forms = [module.predicate(*choice) for choice in itertools.product(range(3), range(5), range(2), range(2))]
        count = len(forms)
        forms += [forms[0].replace(") > 0;", ") < 0;"),
                  forms[0].replace(") > 0;", ") >= 0;"),
                  forms[0].replace("edge->m_twin->m_sitePosition", "edge->m_sitePosition")]
        text = "#include <cstdio>\n"
        for index, form in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + methods + "\n" + form + r"""
bool check() {
    const int coordinates[]={-400,-200,-1,0,1,200,400};
    TRmgZone firstZone, secondZone;
    TRmgBoundaryVertex edge(TPoint(0,0), &firstZone, TPoint(0,0), &secondZone);
    bool okay=true;
    for(int ax=0;ax<7;++ax) for(int ay=0;ay<7;++ay)
    for(int bx=0;bx<7;++bx) for(int by=0;by<7;++by)
    for(int px=0;px<7;++px) for(int py=0;py<7;++py) {
        TPoint a(coordinates[ax],coordinates[ay]);
        TPoint b(coordinates[bx],coordinates[by]);
        TPoint p(coordinates[px],coordinates[py]);
        edge.m_sitePosition=a;
        edge.m_twin->m_sitePosition=b;
        // Shoelace determinant around a,p,b, independent of the candidate's
        // translated-vector cross product and its cyclic argument order.
        long long positive=(long long)a.m_x*p.m_y+(long long)p.m_x*b.m_y+(long long)b.m_x*a.m_y;
        long long negative=(long long)a.m_y*p.m_x+(long long)p.m_y*b.m_x+(long long)b.m_y*a.m_x;
        bool expected=positive>negative;
        if(bool(isRmgPointRightOfEdge(p,&edge))!=expected
            || isRmgPointRightOfEdge(edge.m_sitePosition,&edge)
            || isRmgPointRightOfEdge(edge.m_twin->m_sitePosition,&edge)
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
            text += f'if({condition}) {{ std::printf("failed edge-side form {index}\\n"); return 1; }}\n'
        text += "return 0;}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-right-of-oracle-") as folder:
            cpp, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            cpp.write_text(text)
            subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(cpp), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, timeout=20)


if __name__ == "__main__":
    unittest.main()
