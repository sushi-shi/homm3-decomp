#!/usr/bin/env python3
"""Actual seed-path body and packed setters against an independent map/event model.

Valid bounded grids always contain an eligible fallback seed in each zone.
Flooding and path opening are deterministic opaque boundaries; this checks their
ordered inputs and the caller's writes, not the flood algorithm or VC6 ABI.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def record(text,name):
    start=text.index('struct '+name+' {')
    return text[start:text.index('\n};',start)+3]


FIXTURE=r'''
struct TRmgGridPoint {unsigned m_x,m_y;TRmgGridPoint(unsigned x,unsigned y):m_x(x),m_y(y){}};
struct TRmgMapItem {
    TRmgGroundTile m_tile;
    TRmgGroundTileData m_tileData;
    TRmgMovementCost m_movement;
    TRmgZoneCellState m_zoneState;
    TRmgConnectionDecoration m_connection;
    TRmgMapPosition m_previousTile;
    std::vector<int> m_objects;
    void setTerrain(int,int,unsigned char,unsigned char);
    GATE
    RESET
};
struct TRmgZone {
    TRmgZoneBounds m_bounds;TRmgMapPosition m_levelPosition;int m_terrain;
    TRmgMapPosition getLevelPosition() const;
    BOUNDS
};
struct type_random_map {
    int m_numberLevels,m_mapHeight,m_mapWidth;
    TRmgMapItem cells[32];TRmgMapItem* m_mapItems;
    std::vector<int> events;
    TRmgZone* ownerZones;bool mutateZones;
    type_random_map(int levels,int height):m_numberLevels(levels),m_mapHeight(height),m_mapWidth(4),m_mapItems(cells),ownerZones(0),mutateZones(false){}
    int index(int x,int y,int z)const {assert(x>=0&&x<4&&y>=0&&y<m_mapHeight&&z>=0&&z<m_numberLevels);return (z*m_mapHeight+y)*4+x;}
    void event(int kind,const TRmgMapPosition& p,int value=0){events.push_back(kind);events.push_back(p.m_x);events.push_back(p.m_y);events.push_back(p.m_z);events.push_back(value);}
    TRmgMapItem* getMapItem(int x,int y,int z){
        TRmgMapPosition p(x,y,z);
        // An optional initial pure lookup owns the reset iterator. Validate
        // its actual first-cell identity, then trace all later search queries.
        if(cells[0].m_movement.m_zonePathCost==32000)event(1,p);
        else assert(x==0&&y==0&&z==0);
        return cells+index(x,y,z);
    }
    TRmgMapItem* getMapItem(TRmgMapPosition p){return getMapItem(p.m_x,p.m_y,p.m_z);}
    void setTile(const TRmgGridPoint&,const rmgTerrainTile&);
    void floodConnectionCosts(TRmgMapPosition p,unsigned char water){
        event(2,p,water);int seed=index(p.m_x,p.m_y,p.m_z);
        for(int i=0;i<m_numberLevels*m_mapHeight*4;++i)cells[i].m_movement.m_cost=i==seed?0:1+(i+seed)%5;
        // A host-side opaque effect, not a claim about the retail flood body.
        // Mutate only the current zone; later zones retain their fallback seed.
        if(mutateZones){
            int zone=cells[seed].m_zoneState.m_zone;assert(zone>=0&&zone<2);
            TRmgZone& owner=ownerZones[zone];
            owner.m_bounds.m_maximumX=owner.m_bounds.m_minimumX+1;
            owner.m_bounds.m_maximumY=1;
            owner.m_levelPosition=TRmgMapPosition(zone*2,m_mapHeight-1,(owner.m_levelPosition.m_z+1)%m_numberLevels);
        }
    }
};
struct type_random_map_generator {
    type_random_map m_map;TRmgZone zones[2];std::vector<TRmgZone*> m_zones;
    type_random_map_generator(int levels,int height,int land,int zoneLand,int scenario,bool mutate=false):m_map(levels,height){
        m_map.ownerZones=zones;m_map.mutateZones=mutate;
        int count=levels*height*4;
        for(int i=0;i<count;++i){
            TRmgMapItem& c=m_map.cells[i];unsigned bits=unsigned(i+scenario)*2654435761u;
            std::memcpy(&c.m_tile,&bits,4);std::memcpy(&c.m_tileData,&bits,4);
            c.m_zoneState.m_zone=(i+scenario)%3-1;c.m_zoneState.m_connectionEligibility=3;c.m_zoneState.m_score=17;
            c.m_movement.m_cost=7;c.m_movement.m_zonePathCost=11;c.m_previousTile=TRmgMapPosition(1,1,0);
            c.m_connection.m_present=(i+scenario)&1;c.m_connection.m_direction=3;c.m_connection.m_unknown05=0;
            int kind=(i%3==0)?land:((i+scenario)%11-1);
            c.setTerrain(kind,(i+scenario)%32,scenario&1,(scenario>>1)&1);
            assert(int(c.m_tile.m_landType)==kind);
            c.m_tileData.m_roadPassable=(i+scenario)%3!=0;c.m_tileData.m_subterraneanGate=(i+scenario)%4==0;
            c.m_objects.resize((i+scenario)%3);
        }
        // Exercise the second actual setter and its enum conversion too.
        rmgTerrainTile tile;tile.m_terrain=land;tile.m_frame=23;tile.m_flipX=1;tile.m_flipY=0;
        m_map.setTile(TRmgGridPoint(0,0),tile);assert(int(m_map.cells[0].m_tile.m_landType)==land);
        for(int z=0;z<2;++z){
            TRmgZone& zone=zones[z];zone.m_bounds.m_minimumX=z*2;zone.m_bounds.m_maximumX=z*2+2;
            zone.m_bounds.m_minimumY=0;zone.m_bounds.m_maximumY=height;zone.m_levelPosition=TRmgMapPosition(z*2,0,z%levels);
            zone.m_terrain=z?zoneLand:land;m_zones.push_back(&zone);
            for(int y=0;y<height;++y)for(int x=z*2;x<z*2+2;++x){
                TRmgMapItem& c=m_map.cells[m_map.index(x,y,z%levels)];
                if((2*x+y+scenario)%3)c.m_zoneState.m_zone=z;
            }
            TRmgMapItem& probe=m_map.cells[m_map.index(z*2,0,z%levels)];
            probe.m_zoneState.m_zone=z;probe.setTerrain(land,0,0,0);probe.m_objects.clear();
            probe.m_tileData.m_subterraneanGate=scenario&1;probe.m_tileData.m_roadPassable=(scenario>>1)&1;
            TRmgMapItem& fallback=m_map.cells[m_map.index(z*2+1,height-1,z%levels)];
            fallback.m_zoneState.m_zone=z;fallback.setTerrain(eTerrainDirt,0,0,0);fallback.m_objects.clear();fallback.m_tileData.m_subterraneanGate=0;
        }
    }
    void openConnectionPath(TRmgMapPosition p,unsigned char narrow){m_map.event(3,p,narrow);}
    void buildZoneConnectionPaths();
};
void reference(type_random_map_generator& g){
    for(int i=0;i<g.m_map.m_numberLevels*g.m_map.m_mapHeight*4;++i){
        TRmgMapItem& c=g.m_map.cells[i];c.m_movement.m_zonePathCost=32000;c.m_tileData.m_connectionDirection=0;
        c.m_zoneState.m_connectionEligibility=-1;c.m_movement.m_cost=32000;c.m_previousTile=TRmgMapPosition(-1,-1,-1);
    }
    for(unsigned z=0;z<g.m_zones.size();++z){
        TRmgZone& zone=*g.m_zones[z];TRmgMapPosition seed;bool assigned=false,found=false;
        const int minX=zone.m_bounds.m_minimumX,maxX=zone.m_bounds.m_maximumX;
        const int minY=zone.m_bounds.m_minimumY,maxY=zone.m_bounds.m_maximumY;
        const int initialLevel=zone.m_levelPosition.m_z;
        for(int y=minY;y<maxY&&!found;++y){
            for(int x=minX;x<maxX;++x){
                TRmgMapItem& c=*g.m_map.getMapItem(x,y,initialLevel);
                bool own=int(c.m_zoneState.m_zone)==int(z);
                int kind=c.m_tile.m_landType;
                bool waterAllowed=kind!=eTerrainWater||zone.m_terrain==eTerrainWater;
                if(own&&waterAllowed&&c.m_objects.empty()){
                    seed=TRmgMapPosition(x,y,initialLevel);assigned=true;
                    if(c.m_tileData.m_subterraneanGate&&c.m_tileData.m_roadPassable&&kind!=eTerrainRock){found=true;break;}
                }
            }
        }
        assert(assigned);
        if(!found){TRmgMapItem& c=*g.m_map.getMapItem(seed);if(!c.m_connection.m_present){c.m_tileData.m_borderObject=0;c.m_tileData.m_subterraneanGate=1;}}
        g.m_map.floodConnectionCosts(seed,zone.m_terrain==eTerrainWater);
        const int pathLevel=zone.m_levelPosition.m_z;
        for(int y=minY;y<maxY;++y)for(int x=minX;x<maxX;++x){
            TRmgMapPosition p(x,y,pathLevel);TRmgMapItem& c=*g.m_map.getMapItem(p);
            int kind=c.m_tile.m_landType;
            if(int(c.m_zoneState.m_zone)==int(z)&&c.m_tileData.m_subterraneanGate&&c.m_tileData.m_roadPassable&&kind!=eTerrainRock&&c.m_movement.m_cost&&kind!=eTerrainWater){
                g.openConnectionPath(p,0);g.m_map.floodConnectionCosts(p,zone.m_terrain==eTerrainWater);
            }
        }
    }
}
bool check(){
    for(int levels=1;levels<=2;++levels)for(int height=2;height<=4;++height)
    for(int land=-1;land<=9;++land)for(int zone=-1;zone<=9;++zone)for(int scenario=0;scenario<8;++scenario)
    for(int mutation=0;mutation<2;++mutation){
        type_random_map_generator a(levels,height,land,zone,scenario,mutation!=0),b(levels,height,land,zone,scenario,mutation!=0);
        a.buildZoneConnectionPaths();reference(b);
        if(a.m_map.events!=b.m_map.events)return false;
        for(int i=0;i<levels*height*4;++i){
            const TRmgMapItem& x=a.m_map.cells[i];const TRmgMapItem& y=b.m_map.cells[i];
            if(std::memcmp(&x.m_tile,&y.m_tile,4)||std::memcmp(&x.m_tileData,&y.m_tileData,4)||std::memcmp(&x.m_movement,&y.m_movement,4)||std::memcmp(&x.m_zoneState,&y.m_zoneState,4))return false;
            if(x.m_previousTile.m_x!=-1||x.m_previousTile.m_y!=-1||x.m_previousTile.m_z!=-1)return false;
        }
    }
    return true;
}
'''


def program(source,header):
    extract=generator('generate-rmg-position-family.py').definition
    text='#include <vector>\n#include <cassert>\n#include <cstring>\nstruct TPoint {int m_x,m_y;};\n'+(HOMM3_DIR/'include/terrain_type.h').read_text()+'\n'
    for name in ('TRmgMapPosition','TRmgZoneBounds','TRmgGroundTile','TRmgGroundTileData','TRmgMovementCost','TRmgZoneCellState','TRmgConnectionDecoration'):text+=record(header,name)+'\n'
    text+=record((HOMM3_DIR/'include/rmg_terrain.h').read_text(),'rmgTerrainTile')+'\n'
    text+=extract(source,'TRmgMapPosition::TRmgMapPosition')+'\n'
    text+=FIXTURE.replace('    GATE','    '+extract(header,'hasSubterraneanGate')).replace('    RESET','    '+extract(header,'resetMovement')).replace('    BOUNDS','    '+extract(header,'getBounds'))
    for name in ('TRmgZone::getLevelPosition','type_random_map::setTile','TRmgMapItem::setTerrain','type_random_map_generator::buildZoneConnectionPaths'):text+='\n'+extract(source,name)
    return text+'\nint main(){return check()?0:1;}\n'


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('manifest',type=Path);args=p.parse_args()
    _,sources,axes=source_families.load_manifest(args.manifest,HOMM3_DIR)
    header=(HOMM3_DIR/'include/rmg.h').read_text();models=[source_families.render(sources,axes,(i,)) for i in range(len(axes[0].options))]
    tests=[(str(i),m['src/rmg.cpp'],m.get('include/rmg.h',header),True) for i,m in enumerate(models)]
    base=models[0]['src/rmg.cpp'];extract=generator('generate-rmg-position-family.py').definition;body=extract(base,'type_random_map_generator::buildZoneConnectionPaths')
    for name,old,new in [('water','zone->m_terrain == terrain','zone->m_terrain != terrain'),('objects','static_cast<int>(current->m_objects.size()) <= 0','static_cast<int>(current->m_objects.size()) >= 0'),('step','++x;','x += 2;'),('seedlevel','seed.m_z = position.m_z;','seed.m_z = 0;'),('rock','terrain != eTerrainRock','terrain == eTerrainRock')]:
        assert old in body;tests.append(('wrong-'+name,base.replace(body,body.replace(old,new)),header,False))
    for name,old,new in [('reset-predecessor','previous.m_z = -1;','previous.m_z = 0;'),('reset-count','while (count--)','while (--count)')]:
        assert old in body;tests.append(('wrong-'+name,base.replace(body,body.replace(old,new)),header,False))
    lookup='TRmgMapItem* item = m_map.getMapItem(0, 0, 0);'
    lookup_model=next((m for m in models if lookup in m['src/rmg.cpp']),None)
    if lookup_model:
        bad=lookup_model['src/rmg.cpp'].replace(lookup,'TRmgMapItem* item = m_map.getMapItem(1, 0, 0);')
        tests.append(('wrong-first-cell',bad,lookup_model.get('include/rmg.h',header),False))
    for name,old,new in [
        ('live-bounds','TRmgZoneBounds bounds = zone->m_bounds;','const TRmgZoneBounds& bounds = zone->m_bounds;'),
        ('stale-level','pathPosition = zone->m_levelPosition;','pathPosition = position;'),
        ('live-path-level','TRmgMapItem* current = m_map.getMapItem(pathPosition);','TRmgMapItem* current = m_map.getMapItem(pathPosition.m_x, pathPosition.m_y, zone->m_levelPosition.m_z);'),
    ]:
        assert body.count(old)==1;tests.append(('wrong-'+name,base.replace(body,body.replace(old,new)),header,False))
    with tempfile.TemporaryDirectory(prefix='rmg-water-compatibility-') as folder:
        for name,source,decl,positive in tests:
            path=Path(folder)/name;path.with_suffix('.cpp').write_text(program(source,decl))
            subprocess.run(['g++','-std=c++98','-O1','-fsanitize=undefined','-fno-sanitize-recover=all',str(path.with_suffix('.cpp')),'-o',str(path)],check=True)
            r=subprocess.run([str(path)],capture_output=True,text=True)
            assert (r.returncode==0)==positive,(name,r.returncode,r.stderr)
            if positive:assert not r.stderr,(name,r.stderr)
    for field in range(-32,32):
        for zone in range(-32,64):
            values=(field%(2**32),field&63)
            predicates=[(v!=8 or zone==v,v!=8 or zone==8,not(v==8 and zone!=8),v!=9) for v in values]
            assert predicates[0]==predicates[1] and len(set(predicates[0][:3]))==1
    print('%d actual full-body models x11616 maps including opaque zone mutation; actual getters and setters; %d wrong controls; all64 signed predicate encodings; UBSan clean'%(len(models),len(tests)-len(models)))


if __name__=='__main__':main()
