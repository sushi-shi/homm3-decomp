"""Actual Voronoi types/helpers checked against an explicit perimeter graph."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class VoronoiConstructionTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native C++ oracle needs g++")
    def test_all_forms_and_negative_controls(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-voronoi-construction-family.py")
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
            ("TRmgBoundaryVertex::splice", {}), ("TRmgVoronoi::createEdge", {}),
            ("TRmgVoronoi::~TRmgVoronoi", {})))
        forms = list(module.variants())
        self.assertEqual(len(forms), 60)
        self.assertEqual(len({body for _, body in forms}), 60)
        original = helper.definition(source, module.FUNCTION)
        self.assertEqual(original, forms[0][1])
        forms += [("wrong_corner", original.replace("TPoint fourth(-200, 400)", "TPoint fourth(-200, -200)")),
                  ("wrong_root", original.replace("m_root = firstEdge", "m_root = secondEdge")),
                  ("wrong_splice", original.replace("diagonal->m_twin->splice(thirdEdge)", "diagonal->m_twin->splice(secondEdge)")),
                  ("missing_splice", original.replace("firstEdge->m_twin->splice(secondEdge);", ""))]
        text = "#include <vector>\n#include <algorithm>\n#include <cstdio>\n"
        for index, (_, body) in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + methods + "\n" + body + r"""
bool check() {
    TRmgVoronoi graph;
    const int points[10][2]={{-200,-200},{400,-200},{400,-200},{400,400},{400,400},
        {-200,400},{-200,400},{-200,-200},{-200,-200},{400,400}};
    const int next[10]={8,2,1,4,9,6,5,0,7,3};
    const int previous[10]={7,2,1,9,3,6,5,8,0,4};
    if(graph.m_edges.size()!=10 || graph.m_root!=graph.m_edges[0]) return false;
    for(int i=0;i<10;++i) {
        TRmgBoundaryVertex* edge=graph.m_edges[i];
        if(edge->m_sitePosition.m_x!=points[i][0] || edge->m_sitePosition.m_y!=points[i][1]
            || edge->m_zone || edge->m_twin!=graph.m_edges[i^1]
            || edge->m_next!=graph.m_edges[next[i]] || edge->m_previous!=graph.m_edges[previous[i]]
            || edge->m_positionComputed || edge->m_position.m_x!=-1 || edge->m_position.m_y!=-1) return false;
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
        with tempfile.TemporaryDirectory(prefix="rmg-voronoi-construction-oracle-") as folder:
            source, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            source.write_text(text)
            subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
