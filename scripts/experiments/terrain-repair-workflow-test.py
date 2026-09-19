#!/usr/bin/env python3
"""Verify ordered repair decisions independently of their source grouping."""
import importlib.util
from pathlib import Path
import resource
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR

spec = importlib.util.spec_from_file_location("family", Path(__file__).with_name("terrain-repair-workflow.py"))
family = importlib.util.module_from_spec(spec)
spec.loader.exec_module(family)
source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
forms = list(family.workflows(family.prefix(source)))
fixture = r'''
#include <cassert>
#include <vector>
struct TRmgGridPoint {
 unsigned int m_x,m_y;
 TRmgGridPoint(const unsigned int& x,const unsigned int& y):m_x(x),m_y(y){}
};
struct rmgTerrainPainter {
 unsigned int mask;
 int terrain;
 std::vector<int> events;
 void event(int id,const TRmgGridPoint& p,int t){events.push_back(id);events.push_back(p.m_x);events.push_back(p.m_y);events.push_back(t);}
 bool query(int bit,const TRmgGridPoint& p,int t=0){event(bit,p,t);return (mask>>bit)&1;}
 unsigned char isVerticalGap(const TRmgGridPoint& p){return query(0,p);}
 unsigned char isHorizontalGap(const TRmgGridPoint& p){return query(5,p);}
 unsigned char isHorizontalGap(const TRmgGridPoint& p,int t){return query(p.m_y<4?3:4,p,t);}
 unsigned char isVerticalGap(const TRmgGridPoint& p,int t){return query(p.m_x<3?8:9,p,t);}
 unsigned char needsTerrainRepair(const TRmgGridPoint& p){return query(p.m_y<4?1:p.m_y>4?2:p.m_x<3?6:7,p);}
 int getPaintTerrain()const{return terrain;}
 void paintPoint(const TRmgGridPoint& p){event(10,p,terrain);++terrain;}
 DECLARATIONS
};
DEFINITIONS
void append(std::vector<int>& out,int id,int x,int y,int t=0){out.push_back(id);out.push_back(x);out.push_back(y);out.push_back(t);}
std::vector<int> expected(unsigned int mask){
 std::vector<int> out;int terrain=7;
 for(int phase=0;phase<2;++phase){
  int base=phase?5:0;append(out,base,3,4);
  if(!(mask&(1u<<base)))continue;
  int lx=phase?2:3,ly=phase?4:3,hx=phase?4:3,hy=phase?4:5;
  append(out,base+1,lx,ly);
  bool forward=false;
  if(!(mask&(1u<<(base+1)))){
   append(out,base+2,hx,hy);
   if(mask&(1u<<(base+2)))forward=true;
   else{
    append(out,base+3,lx,ly,terrain);
    if(mask&(1u<<(base+3))){append(out,base+4,hx,hy,terrain);forward=!(mask&(1u<<(base+4)));}
   }
  }
  append(out,10,forward?hx:lx,forward?hy:ly,terrain++);
 }
 return out;
}
int main(){
 typedef void(rmgTerrainPainter::*Repair)(const TRmgGridPoint&);
 Repair forms[]={FORMS};
 for(unsigned int mask=0;mask<1024;++mask)for(unsigned int i=0;i<6;++i){
  rmgTerrainPainter painter;painter.mask=mask;painter.terrain=7;
  const TRmgGridPoint point(3,4);(painter.*forms[i])(point);
  assert(painter.events==expected(mask));assert(point.m_x==3&&point.m_y==4);
 }
}
'''
fixture = fixture.replace("DECLARATIONS", "\n".join(f"void repair{i}(const TRmgGridPoint&);" for i in range(6)))
fixture = fixture.replace("DEFINITIONS", "\n".join(body.replace("repairTerrainPoint", f"repair{i}") + "\n}" for i, (_, body) in enumerate(forms)))
fixture = fixture.replace("FORMS", ",".join(f"&rmgTerrainPainter::repair{i}" for i in range(6)))
with tempfile.TemporaryDirectory(prefix="terrain-workflow-oracle-") as temp:
    path = Path(temp)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    controls = [
        ("authored", fixture),
        ("direction", fixture.replace("paintPoint(TRmgGridPoint(point.m_x, point.m_y + 1))", "paintPoint(TRmgGridPoint(point.m_x, point.m_y - 1))", 1)),
        ("guard", fixture.replace("!needsTerrainRepair(TRmgGridPoint", "needsTerrainRepair(TRmgGridPoint", 1)),
        ("terrain", fixture.replace("return terrain;", "return 0;", 1)),
    ]
    for name, text in controls:
        cpp = path / (name + ".cpp")
        binary = path / name
        cpp.write_text(text)
        subprocess.run(["c++", "-std=c++11", "-O2", str(cpp), "-o", str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True)
        assert (result.returncode == 0) == (name == "authored"), (name, result.stderr)
print("All six workflows preserve 1024 ordered decision cases; three wrong controls fail.")
