#!/usr/bin/env python3
"""Actual count helper versus independent traversal with opaque-call mutations."""
import argparse
import json
import subprocess
import tempfile
from pathlib import Path
from homm3.core.common import HOMM3_DIR

PRE=r'''
#include <cassert>
#include <vector>
struct TRmgZone;
struct TRmgTownSlot;
struct TRmgZoneConnection {TRmgTownSlot* m_destination;};
struct TRmgTownSlot {int m_zoneIndex;std::vector<TRmgZoneConnection> m_connections;};
struct Fixture;
static Fixture* active;
struct TRmgZone {TRmgTownSlot* m_slot;int id;unsigned char canConnect(TRmgZone*) const;};
struct Fixture {
    std::vector<TRmgZone*> m_zones;
    TRmgTownSlot slots[16];TRmgZone nodes[12];TRmgZone placed;
    std::vector<int> trace;int mode,mask;bool changed;
    void setup(int zoneCount,int connections,int flags,int mutation) {
        mode=mutation;mask=flags;changed=false;
        for(int i=0;i<16;++i)slots[i].m_zoneIndex=i-2;
        for(int i=0;i<12;++i){nodes[i].id=i;nodes[i].m_slot=&slots[i];}
        for(int i=0;i<zoneCount;++i)m_zones.push_back(&nodes[i]);
        placed.id=99;placed.m_slot=&slots[15];
        for(int i=0;i<connections;++i){TRmgZoneConnection e;e.m_destination=&slots[(i*3+flags)%14];slots[15].m_connections.push_back(e);}
    }
};
unsigned char TRmgZone::canConnect(TRmgZone* other) const {
    assert(other==&active->placed);active->trace.push_back(id);
    if(!active->changed) {
        active->changed=true;
        if(active->mode&1)active->m_zones.push_back(&active->nodes[10]);
        if(active->mode&2){TRmgZoneConnection e;e.m_destination=&active->slots[2];active->slots[15].m_connections.push_back(e);}
        if(active->mode&4)other->m_slot=&active->slots[14];
    }
    return ((id+active->mask)%3)!=0;
}
int expected(Fixture& f) {
    TRmgTownSlot* original=f.placed.m_slot;int total=0;unsigned i=0;
    while(i<original->m_connections.size()) {
        const int index=original->m_connections[i].m_destination->m_zoneIndex;
        if(index>=0 && unsigned(index)<f.m_zones.size())total+=f.m_zones[index]->canConnect(&f.placed)!=0;
        ++i;
    }
    return total;
}
'''

def main():
    parser=argparse.ArgumentParser();parser.add_argument('manifest',type=Path);args=parser.parse_args()
    manifest=json.loads(args.manifest.read_text());options=manifest['axes'][0]['options'];chunks=[PRE]
    bodies=[]
    for option in options:
        body=option['replace'].replace('int type_random_map_generator::countPlacedZoneConnections','int count')
        bodies.append(body)
    wrong=[bodies[0].replace('++result;','result += 2;'),
           bodies[0].replace('slot->m_connections.size()','initialCount').replace('    for (int connection','    unsigned initialCount = slot->m_connections.size();\n    for (int connection'),
           bodies[0].replace('slot->m_connections[connection]','zone->m_slot->m_connections[connection]')]
    # The last control may address an empty replacement vector; use guarded wrong
    # semantics instead so rejection does not depend on an invalid memory access.
    wrong[-1]=bodies[0].replace('++connection) {','++connection) {\n        if (zone->m_slot != slot) break;')
    for i,body in enumerate(bodies+wrong):chunks.append(f'struct Model{i}: Fixture {{\n{body}\n}};')
    chunks.append(r'''
template<class T> bool check() {
 for(int z=0;z<=8;++z)for(int n=0;n<=7;++n)for(int mask=0;mask<16;++mask)for(int mode=0;mode<8;++mode) {
    T a;Fixture b;a.setup(z,n,mask,mode);b.setup(z,n,mask,mode);
    active=&a;int actual=a.count(&a.placed);active=&b;int wanted=expected(b);
    if(actual!=wanted || a.trace!=b.trace)return false;
 }
 return true;
}
int main(){
''')
    for i in range(len(bodies)):chunks.append(f'assert(check<Model{i}>());')
    for i in range(len(bodies),len(bodies)+len(wrong)):chunks.append(f'assert(!check<Model{i}>());')
    chunks.append('}')
    with tempfile.TemporaryDirectory(prefix='rmg-count-owner-') as tmp:
        p=Path(tmp)/'test';p.with_suffix('.cpp').write_text('\n'.join(chunks))
        subprocess.run(['g++','-std=c++98','-O1','-fsanitize=undefined',str(p.with_suffix('.cpp')),'-o',str(p)],check=True)
        r=subprocess.run([str(p)],capture_output=True);assert r.returncode==0 and not r.stderr,r.stderr.decode()
    print(f'{len(bodies)} actual helpers ×9216 cases; live vector/connection growth and slot replacement checked; {len(wrong)} wrong controls rejected; UBSan clean')


if __name__=='__main__':main()
