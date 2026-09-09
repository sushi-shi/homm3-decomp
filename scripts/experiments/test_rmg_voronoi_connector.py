"""Connector topology against ordered ring insertion, including aliased edges."""
from pathlib import Path
import itertools
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator
from homm3.vc6 import source_families


class VoronoiConnectorTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native C++ oracle needs g++")
    def test_full_insertion_planarity_and_euler_count(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-voronoi-connector-family.py")
        helper = generator("generate-rmg-position-family.py")
        source = (root / module.SOURCE).read_text()
        header = (root / module.HEADER).read_text()
        def block(name, keyword="struct"):
            start = header.index(keyword + " " + name + " {")
            return header[start:header.index("\n};", start)+3]
        types = "struct TRmgZone {};\n" + "\n".join(block(name) for name in (
            "TRmgVector", "TPoint", "TRmgBoundaryVertex")) + "\n" + block("TRmgVoronoi", "class")
        types = types.replace("    void removeEdge", module.DECLARATION + "    void removeEdge")
        methods = "\n".join(helper.definition(source, name, **args) for name, args in (
            ("TRmgBoundaryVertex::TRmgBoundaryVertex", dict(parameters="TPoint sitePosition, TRmgZone* zone, TRmgBoundaryVertex* twin")),
            ("TRmgBoundaryVertex::TRmgBoundaryVertex", dict(parameters="TPoint sitePosition, TRmgZone* zone, TPoint twinSitePosition, TRmgZone* twinZone")),
            ("TRmgBoundaryVertex::splice", {}), ("TRmgBoundaryVertex::detach", {}),
            ("getRmgPointOrientation", {}), ("getRmgSquaredDistance", {}),
            ("TRmgVoronoi::createEdge", {}), ("TRmgVoronoi::removeEdge", {}),
            ("TRmgVoronoi::locate", {}), ("TRmgVoronoi::~TRmgVoronoi", {}),
            ("flipRmgEdge", {}), ("isRmgPointOnSegment", {}), ("isRmgPointInsideCircle", {})))
        # The native spelling is only a portability substitution for VC6's
        # signed 64-bit extension. The matching snapshot is never rewritten.
        methods = methods.replace("__int64", "long long")
        locate = helper.definition(source, "TRmgVoronoi::locate")
        edge_side = ""
        if "isRmgPointRightOfEdge(" in source:
            edge_side = helper.definition(source, "isRmgPointRightOfEdge") + "\n"
            methods = methods.replace(locate, edge_side + locate)
        old_segment = helper.definition(source, "isRmgPointOnSegment")
        forms = [helper.definition(source, "TRmgVoronoi::TRmgVoronoi") + "\n" +
                 helper.definition(source, "TRmgVoronoi::addSite")]
        for option in module.helper_options(source):
            if "+placement_0+" in option["name"]:
                site = next(edit["replace"] for edit in option["extra_edits"]
                            if edit.get("find", "").startswith("void TRmgVoronoi::addSite"))
                forms.append(option["replace"] + "\n" + site)
        self.assertEqual(len(forms), 17)
        form_methods = [methods] * len(forms)
        if os.environ.get("HOMM3_VORONOI_MANIFEST"):
            path = Path(os.environ["HOMM3_VORONOI_MANIFEST"])
            payload, originals, axes = source_families.load_manifest(path, root)
            forms, form_methods = [], []
            for choice in range(len(payload["axes"][0]["options"])):
                changed = source_families.render(originals, axes, (choice,))[module.SOURCE]
                ctor = helper.definition(changed, "TRmgVoronoi::TRmgVoronoi")
                site = helper.definition(changed, "TRmgVoronoi::addSite")
                added = ""
                if "TRmgVoronoi::connectEdges(" in changed:
                    added = helper.definition(changed, "TRmgVoronoi::connectEdges")
                forms.append(added + "\n" + ctor + "\n" + site)
                old_factory = helper.definition(source, "TRmgVoronoi::createEdge")
                new_factory = helper.definition(changed, "TRmgVoronoi::createEdge")
                changed_methods = methods.replace(old_factory, new_factory)
                old_locate = edge_side + helper.definition(source, "TRmgVoronoi::locate")
                new_locate = helper.definition(changed, "TRmgVoronoi::locate")
                if "isRmgPointRightOfEdge(" in changed:
                    new_locate = helper.definition(changed, "isRmgPointRightOfEdge") + "\n" + new_locate
                changed_methods = changed_methods.replace(old_locate, new_locate)
                new_segment = helper.definition(changed, "isRmgPointOnSegment")
                if "isRmgPointOnLine(" in changed:
                    new_segment = helper.definition(changed, "isRmgPointOnLine") + "\n" + new_segment
                line_object = generator("generate-rmg-voronoi-segment-family.py").line_object_code(changed)
                if line_object:
                    new_segment = line_object + "\n" + new_segment
                changed_methods = changed_methods.replace(old_segment, new_segment)
                old_circle = helper.definition(source, "isRmgPointInsideCircle").replace("__int64", "long long")
                new_circle = helper.definition(changed, "isRmgPointInsideCircle").replace("__int64", "long long")
                form_methods.append(changed_methods.replace(old_circle, new_circle))
        text = "#include <vector>\n#include <algorithm>\n#include <cstdio>\n"
        for index, body in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + form_methods[index] + "\n" + body + r"""
long long area(TPoint a,TPoint b,TPoint c) {
    return (long long)(b.m_x-a.m_x)*(c.m_y-a.m_y)-(long long)(b.m_y-a.m_y)*(c.m_x-a.m_x);
}
bool valid(TRmgVoronoi& graph,int inserted) {
    // Four fixed hull vertices and N interior sites imply E=5+3N.
    if(graph.m_edges.size()!=unsigned(10+6*inserted)) return false;
    for(unsigned int i=0;i<graph.m_edges.size();++i) {
        TRmgBoundaryVertex* edge=graph.m_edges[i];
        if(edge->m_twin->m_twin!=edge || edge->m_next->m_previous!=edge
            || edge->m_previous->m_next!=edge || edge->m_next->m_sitePosition!=edge->m_sitePosition) return false;
        if(std::find(graph.m_edges.begin(),graph.m_edges.end(),edge->m_twin)==graph.m_edges.end()) return false;
        for(unsigned int j=i+1;j<graph.m_edges.size();++j) {
            TRmgBoundaryVertex* other=graph.m_edges[j];
            TPoint a=edge->m_sitePosition,b=edge->m_twin->m_sitePosition;
            TPoint c=other->m_sitePosition,d=other->m_twin->m_sitePosition;
            if(a==c || a==d || b==c || b==d) continue;
            long long p=area(a,b,c),q=area(a,b,d),r=area(c,d,a),s=area(c,d,b);
            if(((p<0 && q>0)||(p>0 && q<0)) && ((r<0 && s>0)||(r>0 && s<0))) return false;
        }
    }
    return true;
}
bool check() {
    const TPoint sites[]={TPoint(0,0),TPoint(100,0),TPoint(0,100),TPoint(100,100),
        TPoint(-100,100),TPoint(100,-100),TPoint(-50,-20),TPoint(50,70)};
    for(int rotation=0;rotation<8;++rotation) {
        TRmgVoronoi graph; TRmgZone zones[8];
        if(!valid(graph,0)) return false;
        for(int n=0;n<8;++n) {
            int which=(n+rotation)%8;
            graph.addSite(sites[which],&zones[which]);
            if(!valid(graph,n+1)) return false;
            graph.addSite(sites[which],&zones[which]);
            if(!valid(graph,n+1)) return false;
            bool found=false;
            for(unsigned int i=0;i<graph.m_edges.size();++i)
                if(graph.m_edges[i]->m_sitePosition==sites[which]) {
                    if(graph.m_edges[i]->m_zone!=&zones[which]) return false;
                    found=true;
                }
            if(!found) return false;
        }
    }
    return true;
}
}
"""
        text += "int main(){\n"
        for index in range(len(forms)):
            text += f'if(!Case{index}::check()) {{ std::printf("failed insertion form {index}\\n"); return 1; }}\n'
        text += "return 0;}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-voronoi-insertion-oracle-") as folder:
            source, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            source.write_text(text)
            subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, timeout=15)

    @unittest.skipUnless(shutil.which("g++"), "native C++ oracle needs g++")
    def test_helpers_and_wrong_endpoint_controls(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-voronoi-connector-family.py")
        helper = generator("generate-rmg-position-family.py")
        source = (root / module.SOURCE).read_text()
        header = (root / module.HEADER).read_text()
        def block(name, keyword="struct"):
            start = header.index(keyword + " " + name + " {")
            return header[start:header.index("\n};", start)+3]
        types = "struct TRmgZone {};\n" + "\n".join(block(name) for name in (
            "TRmgVector", "TPoint", "TRmgBoundaryVertex")) + "\n" + block("TRmgVoronoi", "class")
        types = types.replace("    void removeEdge", module.DECLARATION + "    void removeEdge")
        methods = "\n".join(helper.definition(source, name, **args) for name, args in (
            ("TRmgBoundaryVertex::TRmgBoundaryVertex", dict(parameters="TPoint sitePosition, TRmgZone* zone, TRmgBoundaryVertex* twin")),
            ("TRmgBoundaryVertex::TRmgBoundaryVertex", dict(parameters="TPoint sitePosition, TRmgZone* zone, TPoint twinSitePosition, TRmgZone* twinZone")),
            ("TRmgBoundaryVertex::splice", {}), ("TRmgVoronoi::createEdge", {}),
            ("TRmgVoronoi::~TRmgVoronoi", {}), ("TRmgVoronoi::TRmgVoronoi", {})))
        forms = [module.connector(*choice) for choice in itertools.product(range(4), range(3), range(2), range(2))]
        positive_count = len(forms)
        forms += [forms[0].replace("first->m_twin->m_sitePosition", "first->m_sitePosition"),
                  forms[0].replace("result->m_twin->splice(second);", ""),
                  forms[0].replace("first->m_twin->m_previous", "first->m_twin->m_next"),
                  forms[0].replace("second->m_zone", "first->m_zone")]
        text = "#include <vector>\n#include <algorithm>\n#include <cstdio>\n"
        for index, body in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + methods + "\n" + body + r"""
bool check() {
    for(int a=0;a<10;++a) for(int b=0;b<10;++b) {
        TRmgVoronoi graph; TRmgZone zones[10];
        int next[12]={8,2,1,4,9,6,5,0,7,3,10,11};
        int previous[12]={7,2,1,9,3,6,5,8,0,4,10,11};
        for(int i=0;i<10;++i) graph.m_edges[i]->m_zone=&zones[i];
        TPoint firstSite=graph.m_edges[a^1]->m_sitePosition;
        TPoint secondSite=graph.m_edges[b]->m_sitePosition;
        // Insert the first new half before a's opposite in its origin ring.
        int before=previous[a^1];
        next[before]=10; previous[10]=before; next[10]=a^1; previous[a^1]=10;
        // Insert its opposite after b. Use the updated ring for alias cases.
        int after=next[b];
        next[b]=11; previous[11]=b; next[11]=after; previous[after]=11;
        TRmgBoundaryVertex* result=graph.connectEdges(graph.m_edges[a],graph.m_edges[b]);
        if(graph.m_edges.size()!=12 || result!=graph.m_edges[10] || graph.m_root!=graph.m_edges[0]) return false;
        for(int i=0;i<12;++i) {
            TRmgBoundaryVertex* edge=graph.m_edges[i];
            if(edge->m_next!=graph.m_edges[next[i]] || edge->m_previous!=graph.m_edges[previous[i]]
                || edge->m_twin!=graph.m_edges[i^1]) return false;
        }
        if(result->m_sitePosition.m_x!=firstSite.m_x || result->m_sitePosition.m_y!=firstSite.m_y
            || result->m_zone!=&zones[a^1] || result->m_twin->m_sitePosition.m_x!=secondSite.m_x
            || result->m_twin->m_sitePosition.m_y!=secondSite.m_y || result->m_twin->m_zone!=&zones[b]) return false;
    }
    return true;
}
}
"""
        text += "int main(){\n"
        for index in range(len(forms)):
            condition = f"!Case{index}::check()" if index < positive_count else f"Case{index}::check()"
            text += f'if({condition}) {{ std::printf("failed form {index}\\n"); return 1; }}\n'
        text += "return 0;}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-voronoi-connector-oracle-") as folder:
            source, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            source.write_text(text)
            subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
