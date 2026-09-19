#!/usr/bin/env python3
"""Actual filter bodies versus independent stable candidate ranking.

Modern native C++ scopes for-indices differently from VC6. Hoist those integer
indices in the native harness only; no index escapes the authored function.
Pure mock getters/canConnect test selection and final zone state. The separate
count-owner oracle tests opaque-call mutation; this one tests phase composition.
"""
import argparse
import json
import subprocess
import tempfile
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.test_rmg_families import generator


PRE = r'''
#include <algorithm>
#include <cassert>
#include <vector>
using std::min; using std::max;
struct TRmgMapPosition {
 int m_x,m_y,m_z;
 TRmgMapPosition():m_x(0),m_y(0),m_z(0){}
 TRmgMapPosition(int x,int y,int z):m_x(x),m_y(y),m_z(z){}
 bool operator==(const TRmgMapPosition& b)const{return m_x==b.m_x&&m_y==b.m_y&&m_z==b.m_z;}
};
struct TRmgTownSlot;
struct TRmgZoneConnection{TRmgTownSlot* m_destination;};
struct TRmgTownSlot{int m_zoneIndex,m_size;std::vector<TRmgZoneConnection> m_connections;};
struct TRmgZone {
 TRmgTownSlot* m_slot;TRmgMapPosition pos;int id,mask;
 TRmgMapPosition getLevelPosition()const{return pos;}
 void setLevelPosition(TRmgMapPosition p){pos=p;}
 int getSize()const{return m_slot->m_size;}
 bool canConnect(TRmgZone* other)const{
   return pos.m_z==other->pos.m_z && ((other->pos.m_x+2*other->pos.m_y+id+mask)%3)!=0;
 }
};
struct Fixture {
 struct Map{int m_numberLevels;}m_map;
 std::vector<TRmgZone*>m_zones;
 TRmgTownSlot slots[10];TRmgZone nodes[5],placed;
 std::vector<TRmgMapPosition> candidates;
 void setup(int n,int count,int shape,int levels,int flags){
  m_map.m_numberLevels=levels;
  for(int i=0;i<10;++i){slots[i].m_zoneIndex=i-2;slots[i].m_size=1+(i+flags)%4;}
  for(int i=0;i<5;++i){nodes[i].id=i;nodes[i].mask=flags;nodes[i].m_slot=&slots[i+2];
   nodes[i].pos=TRmgMapPosition(i*5-8,(i*3+shape)%13-6,(i+shape)%levels);}
  placed.id=9;placed.mask=flags;placed.m_slot=&slots[9];placed.pos=TRmgMapPosition(17,-23,0);
  for(int i=0;i<n;++i)m_zones.push_back(&nodes[i]);
  if(flags&1)m_zones.push_back(&placed);
  for(int i=0;i<8;++i){TRmgZoneConnection e;e.m_destination=&slots[(i*3+flags)%9];slots[9].m_connections.push_back(e);}
  for(int i=0;i<count;++i)candidates.push_back(TRmgMapPosition((i*7+shape*3)%19-9,(i*5+flags*2)%17-8,(i+flags)%levels));
 }
};
int countAt(const Fixture& f,const TRmgMapPosition& p){
 int total=0;
 for(unsigned i=0;i<f.slots[9].m_connections.size();++i){
  int d=f.slots[9].m_connections[i].m_destination->m_zoneIndex;
  if(d<0||unsigned(d)>=f.m_zones.size())continue;
  const TRmgZone* other=f.m_zones[d];
  int level=other==&f.placed?p.m_z:other->pos.m_z;
  total+=(level==p.m_z && ((p.m_x+2*p.m_y+other->id+other->mask)%3)!=0);
 }
 return total;
}
void reference(Fixture& f,int mapSize){
 std::vector<TRmgMapPosition> active=f.candidates;
 if(f.m_map.m_numberLevels>1){
  bool used[2]={false,false};
  for(unsigned i=0;i<f.m_zones.size();++i)if(f.m_zones[i]!=&f.placed)used[f.m_zones[i]->pos.m_z]=true;
  int last=-1;
  for(unsigned i=0;i<active.size();++i)if(!used[active[i].m_z])last=int(i);
  if(last>0){std::vector<TRmgMapPosition> keep;for(unsigned i=0;i<active.size();++i)if(!used[active[i].m_z])keep.push_back(active[i]);active=keep;}
 }
 int bestCount=0;
 std::vector<int> scores;
 for(unsigned i=0;i<active.size();++i){int score=countAt(f,active[i]);scores.push_back(score);if(score>bestCount)bestCount=score;}
 if(!active.empty())f.placed.pos=active.front();
 std::vector<TRmgMapPosition> connected;
 for(unsigned i=0;i<active.size();++i)if(scores[i]==bestCount)connected.push_back(active[i]);
 int lowX=0,lowY=0,highX=0,highY=0;
 for(unsigned i=0;i<f.m_zones.size();++i){
  const TRmgZone* other=f.m_zones[i];if(other==&f.placed)continue;
  int r=other->m_slot->m_size;
  lowX=std::min(lowX,other->pos.m_x-r);lowY=std::min(lowY,other->pos.m_y-r);
  highX=std::max(highX,other->pos.m_x+r+1);highY=std::max(highY,other->pos.m_y+r+1);
 }
 int best=32000,r=f.placed.m_slot->m_size;scores.clear();
 for(unsigned i=0;i<connected.size();++i){
  TRmgMapPosition p=connected[i];
  int x=std::max(highX,p.m_x+r+1)-std::min(lowX,p.m_x-r);
  int y=std::max(highY,p.m_y+r+1)-std::min(lowY,p.m_y-r);
  int score=std::max(mapSize,std::max(x,y));scores.push_back(score);best=std::min(best,score);
 }
 f.candidates.clear();for(unsigned i=0;i<connected.size();++i)if(scores[i]==best)f.candidates.push_back(connected[i]);
}
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest',type=Path)
    args=parser.parse_args()
    options=json.loads(args.manifest.read_text())['axes'][0]['options']
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    count=generator('generate-rmg-position-family.py').definition(source,'type_random_map_generator::countPlacedZoneConnections').replace('type_random_map_generator::','')
    bodies=[o['replace'].replace('type_random_map_generator::','') for o in options]
    wrong=[bodies[0].replace('if (candidate > 0)', 'if (candidate >= 0)'),
           bodies[0].replace('if (connections > bestConnections)', 'if (connections < bestConnections)'),
           bodies[0].replace('if (bestSize < candidateSize)', 'if (bestSize > candidateSize)'),
           bodies[0].replace('m_zones[other] != zone', 'true')]
    chunks=[PRE]
    for i,body in enumerate(bodies+wrong):
        body=body.replace('for (int candidate =','for (candidate =').replace('for (int other =','for (other =')
        body=body.replace('{\n', '{\n    int candidate = 0, other = 0;\n',1)
        chunks.append(f'struct Model{i}:Fixture{{\n{count}\n{body}\n}};')
    chunks.append(r'''
template<class T>bool check(){
 for(int n=0;n<5;++n)for(int count=0;count<6;++count)for(int shape=0;shape<4;++shape)
 for(int levels=1;levels<=2;++levels)for(int flags=0;flags<8;++flags)for(int size=0;size<3;++size){
  T actual;Fixture wanted;actual.setup(n,count,shape,levels,flags);wanted.setup(n,count,shape,levels,flags);
  actual.filterZonePositions(&actual.placed,actual.candidates,size*10);
  reference(wanted,size*10);
  if(actual.candidates!=wanted.candidates || !(actual.placed.pos==wanted.placed.pos))return false;
 }
 return true;
}
int main(){
''')
    chunks.extend(f'assert(check<Model{i}>());' for i in range(len(bodies)))
    chunks.extend(f'assert(!check<Model{i}>());' for i in range(len(bodies),len(bodies)+len(wrong)))
    chunks.append('}')
    with tempfile.TemporaryDirectory(prefix='rmg-filter-phase-') as tmp:
        path=Path(tmp)/'test';path.with_suffix('.cpp').write_text('\n'.join(chunks))
        subprocess.run(['g++','-std=c++98','-O1','-fsanitize=undefined',str(path.with_suffix('.cpp')),'-o',str(path)],check=True)
        run=subprocess.run([str(path)],capture_output=True)
        assert run.returncode==0 and not run.stderr,run.stderr.decode()
    print(f'{len(bodies)} actual filters ×5760 fixtures; stable candidates/final zone state; {len(wrong)} wrong controls rejected; UBSan clean')


if __name__=='__main__':main()
