"""Edge-factory allocation, prefix preservation and paired ownership."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class EdgeFactoryTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native C++ oracle needs g++")
    def test_all_forms_and_wrong_ownership_controls(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-edge-factory-family.py")
        helper = generator("generate-rmg-position-family.py")
        source = (root / "src/rmg_support.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        def block(name, keyword="struct"):
            start = header.index(keyword + " " + name + " {")
            return header[start:header.index("\n};", start)+3]
        types = "struct TRmgZone {};\n" + "\n".join(block(name) for name in (
            "TRmgVector", "TPoint", "TRmgBoundaryVertex")) + "\n" + block("TRmgVoronoi", "class")
        methods = "\n".join(helper.definition(source, name, **args) for name, args in (
            ("TRmgBoundaryVertex::TRmgBoundaryVertex", dict(parameters="TPoint sitePosition, TRmgZone* zone, TRmgBoundaryVertex* twin")),
            ("TRmgBoundaryVertex::TRmgBoundaryVertex", dict(parameters="TPoint sitePosition, TRmgZone* zone, TPoint twinSitePosition, TRmgZone* twinZone")),
            ("TRmgBoundaryVertex::splice", {}), ("TRmgVoronoi::TRmgVoronoi", {}),
            ("TRmgVoronoi::~TRmgVoronoi", {})))
        original = helper.definition(source, "TRmgVoronoi::createEdge")
        forms = list(module.variants(original))
        self.assertEqual(len(forms), 72)
        self.assertEqual(len({body for _, body in forms}), 72)
        self.assertEqual(forms[0][1], original)
        forms += [("wrong_return", original.replace("return edge;", "return twin;")),
                  ("wrong_point", original.replace("first, firstZone, second, secondZone", "first, firstZone, first, secondZone")),
                  ("wrong_zone", original.replace("first, firstZone, second, secondZone", "first, firstZone, second, firstZone"))]
        text = "#include <vector>\n#include <algorithm>\n#include <cstdio>\n"
        for index, (_, body) in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + methods + "\n" + body + r"""
bool check() {
    TRmgVoronoi graph; TRmgZone zones[2];
    if(graph.m_edges.size()!=10) return false;
    for(int n=0;n<40;++n) {
        std::vector<TRmgBoundaryVertex*> old=graph.m_edges;
        TPoint first(n-20,30-n),second(40-n,n+1);
        TRmgBoundaryVertex* edge=graph.createEdge(first,&zones[0],second,&zones[1]);
        if(graph.m_edges.size()!=old.size()+2 || graph.m_edges[old.size()]!=edge
            || graph.m_edges[old.size()+1]!=edge->m_twin) return false;
        for(unsigned int i=0;i<old.size();++i) if(graph.m_edges[i]!=old[i]) return false;
        if(edge->m_sitePosition!=first || edge->m_twin->m_sitePosition!=second
            || edge->m_zone!=&zones[0] || edge->m_twin->m_zone!=&zones[1]
            || edge->m_twin->m_twin!=edge) return false;
        for(int side=0;side<2;++side) {
            TRmgBoundaryVertex* current=side ? edge->m_twin : edge;
            if(current->m_next!=current || current->m_previous!=current || current->m_positionComputed
                || current->m_position.m_x!=-1 || current->m_position.m_y!=-1) return false;
        }
    }
    return true;
}
}
"""
        text += "int main(){\n"
        for index in range(len(forms)):
            condition = f"!Case{index}::check()" if index < 72 else f"Case{index}::check()"
            text += f'if({condition}) {{ std::printf("failed form {index}\\n"); return 1; }}\n'
        text += "return 0;}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-edge-factory-oracle-") as folder:
            source, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            source.write_text(text)
            subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
