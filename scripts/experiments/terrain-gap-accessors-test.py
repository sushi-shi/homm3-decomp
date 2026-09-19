#!/usr/bin/env python3
"""Check the authored gap predicates and every accessor form against an oracle."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import resource
import argparse
import json

from homm3.core.common import HOMM3_DIR

spec = importlib.util.spec_from_file_location("family", Path(__file__).with_name("terrain-gap-accessors.py"))
family = importlib.util.module_from_spec(spec)
spec.loader.exec_module(family)
source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
definitions = []
names = []
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--manifest", type=Path)
args = parser.parse_args()
manifest = json.loads(args.manifest.read_text()) if args.manifest else None
for kind in ("Horizontal", "Vertical"):
    if manifest:
        options = manifest["axes"][0]["options"]
        bodies = [(item["name"], item["replace"] if kind == "Horizontal" else item["extra_edits"][0]["replace"]) for item in options]
    else:
        bodies = family.variants(family.definition(source, kind), kind)
    for index, (_, body) in enumerate(bodies):
        name = f"is{kind}Gap{index}"
        names.append((name, kind == "Horizontal"))
        definitions.append(body.replace(f"is{kind}Gap", name))

fixture = r'''
#include <cassert>
#include <vector>
struct TRmgGridPoint {
    unsigned int m_x, m_y;
    TRmgGridPoint(const unsigned int& x, const unsigned int& y):m_x(x),m_y(y){}
    unsigned int getX()const{return m_x;}
    unsigned int getY()const{return m_y;}
};
struct rmgTerrainPainter {
    unsigned int width,height;
    int values[2];
    std::vector<TRmgGridPoint> reads;
    unsigned int getWidth()const{return width;}
    unsigned int getHeight()const{return height;}
    int getTerrain(const TRmgGridPoint& p){reads.push_back(p);return values[reads.size()-1];}
DECLARATIONS
};
DEFINITIONS
int main(){
    rmgTerrainPainter painter;
    typedef unsigned char(rmgTerrainPainter::*Predicate)(const TRmgGridPoint&,int);
    Predicate predicates[]={PREDICATES};
    bool horizontal[]={HORIZONTAL};
    for(unsigned int width=1;width<=8;++width)for(unsigned int height=1;height<=8;++height)
    for(unsigned int x=0;x<=width;++x)for(unsigned int y=0;y<=height;++y)
    for(int mask=0;mask<4;++mask)for(unsigned int which=0;which<PREDICATE_COUNT;++which){
        painter.width=width;painter.height=height;painter.reads.clear();
        painter.values[0]=mask&1;painter.values[1]=(mask>>1)&1;
        TRmgGridPoint p(x,y);
        unsigned int axis=horizontal[which]?x:y;
        unsigned int bound=horizontal[which]?width:height;
        bool within=axis>=1 && axis+1<bound;
        unsigned int count=within?(painter.values[0]?2:1):0;
        bool expected=within && painter.values[0] && painter.values[1];
        assert((painter.*predicates[which])(p,0)==expected);
        assert(painter.reads.size()==count);
        for(unsigned int n=0;n<count;++n){
            unsigned int a=n?axis+1:axis-1;
            assert(painter.reads[n].m_x==(horizontal[which]?a:x));
            assert(painter.reads[n].m_y==(horizontal[which]?y:a));
        }
        assert(p.m_x==x && p.m_y==y);
    }
}
'''
fixture = fixture.replace("DECLARATIONS", "\n".join(
    f"unsigned char {name}(const TRmgGridPoint&,int);" for name, _ in names))
fixture = fixture.replace("DEFINITIONS", "\n".join(definitions))
fixture = fixture.replace("PREDICATES", ",".join("&rmgTerrainPainter::" + name for name, _ in names))
fixture = fixture.replace("HORIZONTAL", ",".join("true" if h else "false" for _, h in names))
fixture = fixture.replace("PREDICATE_COUNT", str(len(names)))
with tempfile.TemporaryDirectory(prefix="terrain-gap-oracle-") as temp:
    path = Path(temp)
    (path / "oracle.cpp").write_text(fixture)
    subprocess.run(["c++", "-std=c++11", "-O2", str(path / "oracle.cpp"), "-o", str(path / "oracle")], check=True)
    subprocess.run([str(path / "oracle")], check=True)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    controls = [
        ("boundary", "point.m_x > 0", "point.m_x >= 0"),
        ("neighbour", "point.m_x - 1", "point.m_x + 1"),
        ("comparison", ") != terrain", ") == terrain"),
    ]
    for name, original, wrong in controls:
        control = path / (name + ".cpp")
        control.write_text(fixture.replace(original, wrong, 1))
        binary = path / name
        subprocess.run(["c++", "-std=c++11", "-O2", str(control), "-o", str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True)
        assert result.returncode != 0, name + " negative control unexpectedly passed"
print(f"All {len(names)} predicate forms preserve ordered reads, boundaries and results; three wrong controls fail.")
