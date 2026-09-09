"""Removal order and ring bypass on paired owned edges."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class EdgeRemovalTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native C++ oracle needs g++")
    def test_all_forms_and_detach_controls(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-edge-removal-family.py")
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
            ("TRmgBoundaryVertex::splice", {}), ("TRmgBoundaryVertex::detach", {}),
            ("TRmgVoronoi::TRmgVoronoi", {}), ("TRmgVoronoi::createEdge", {}),
            ("TRmgVoronoi::~TRmgVoronoi", {})))
        forms = list(module.variants())
        self.assertEqual(len(forms), 64)
        self.assertEqual(len({body for _, body in forms}), 64)
        original = forms[0][1]
        self.assertEqual(original, helper.definition(source, "TRmgVoronoi::removeEdge"))
        forms += [("no_detach", original.replace("edge->detach();", "")),
                  ("only_first", original.replace("edge->detach();", "edge->splice(edge->m_previous);")),
                  ("only_twin", original.replace("edge->detach();", "edge->m_twin->splice(edge->m_twin->m_previous);"))]
        text = "#include <vector>\n#include <algorithm>\n#include <cstdio>\n"
        for index, (_, body) in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + methods + "\n" + body + r"""
bool check() {
    const int extras[]={0,1,3,17};
    for(int count=0;count<4;++count) for(int target=0;target<10+2*extras[count];++target) {
        TRmgVoronoi graph;
        for(int extra=0;extra<extras[count];++extra) graph.createEdge(TPoint(extra,0),0,TPoint(extra,1),0);
        std::vector<TRmgBoundaryVertex*> old=graph.m_edges,expected;
        int next[44],previous[44];
        for(unsigned int i=0;i<old.size();++i) {
            next[i]=std::find(old.begin(),old.end(),old[i]->m_next)-old.begin();
            previous[i]=std::find(old.begin(),old.end(),old[i]->m_previous)-old.begin();
            if(int(i)!=target && int(i)!=(target^1)) expected.push_back(old[i]);
        }
        // Sequential deletion from each origin's circular list. Singleton
        // rings need no survivor update; their only edge is about to disappear.
        for(int side=0;side<2;++side) {
            int gone=target^side;
            next[previous[gone]]=next[gone]; previous[next[gone]]=previous[gone];
        }
        graph.removeEdge(old[target]);
        if(graph.m_edges!=expected) return false;
        for(unsigned int i=0;i<old.size();++i) if(int(i)!=target && int(i)!=(target^1)) {
            if(old[i]->m_next!=old[next[i]] || old[i]->m_previous!=old[previous[i]]) return false;
        }
    }
    return true;
}
}
"""
        text += "int main(){\n"
        for index in range(len(forms)):
            condition = f"!Case{index}::check()" if index < 64 else f"Case{index}::check()"
            text += f'if({condition}) {{ std::printf("failed form {index}\\n"); return 1; }}\n'
        text += "return 0;}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-edge-removal-oracle-") as folder:
            source, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            source.write_text(text)
            compiled = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(source), "-o", str(binary)], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr[-5000:])
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
