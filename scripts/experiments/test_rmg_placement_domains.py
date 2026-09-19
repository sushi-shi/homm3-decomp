#!/usr/bin/env python3
"""Native oracle for actual placement trio bodies and coordinate operations.

Independent footprint enumeration checks each mask domain against bounded
signed world coordinates; no signed overflow is admitted. Mock outline calls
mutate the prototype and replace its owner pointer to test snapshot ownership
and post-call reads. Host fixture models semantics, not VC6 layout or EH.
"""
import argparse
import subprocess
import tempfile
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator

FIXTURE = r'''
enum { eTerrainWater=8, eTerrainRock=9 };
struct TObjectType {
    enum { SLOT_CATEGORY_0=0 };
    struct TPoint { int m_x,m_y; };
    std::bitset<48> m_triggerMask,m_passableMask;
    std::bitset<10> m_recommendedTerrainMask;
    int m_objectType,m_slotCategory,width,height;
    unsigned char m_hasTrigger;
    TPoint m_triggerCell;
    int getWidth() const {return width;} int getHeight() const {return height;}
};
static std::vector<int>* events;
static TObjectType* observed;
static TObjectType* replacement;
static int mode;
static unsigned char g_adventureObjectLandBlocked[16][16];
struct TRmgObjectPropertiesRef {
    TObjectType* m_prototype;
    std::vector<TPoint> m_outline;
    void buildOutline() {
        events->push_back(10000);
        if(mode&1) { observed->m_objectType=7; observed->m_hasTrigger=1;
            observed->m_triggerCell.m_x=observed->width ? observed->width-1 : 0; }
        if(mode&2) m_prototype=replacement;
    }
};
struct type_object { TRmgObjectPropertiesRef* m_properties; TRmgMapPosition m_position; };
struct TRmgMapItem {
    struct Flags { unsigned m_roadPassable,m_roadEntrance,m_borderObject,m_subterraneanGate; } m_tileData;
    struct Tile { int m_landType; } m_tile;
    struct Zone { int m_zone; } m_zoneState;
    struct Connection { unsigned m_present; } m_connection;
    std::vector<type_object*> m_objects;
    unsigned char isRoadEntrance() const {return m_tileData.m_roadEntrance;}
    unsigned char hasBorderObject() const {return m_tileData.m_borderObject;}
};
struct Slot {int m_zoneIndex;};
struct TRmgZone {Slot* m_slot; int m_terrain;};
struct MapFixture {
    int m_mapWidth,m_mapHeight;
    std::vector<TRmgMapItem> cells;
    std::vector<int> trace;
    unsigned char connected;
    MapFixture():m_mapWidth(6),m_mapHeight(6),cells(72),connected(1){}
    TRmgMapItem* getMapItem(TRmgMapPosition p) {
        assert(p.m_x>=0 && p.m_x<6 && p.m_y>=0 && p.m_y<6 && p.m_z>=0 && p.m_z<2);
        trace.push_back((p.m_z*6+p.m_y)*6+p.m_x);
        return &cells[(p.m_z*6+p.m_y)*6+p.m_x];
    }
    unsigned char hasConnectedOutline(const std::vector<TPoint>&,TRmgMapPosition p,
        unsigned char allow,TRmgZone*,unsigned char gate) {
        trace.push_back(20000+allow*100+gate*10+p.m_z);
        if(mode&1) observed->m_triggerCell.m_y=observed->height ? observed->height-1 : 0;
        return connected;
    }
};
unsigned char expectedBlocked(MapFixture& map,TRmgObjectPropertiesRef* properties,
    TRmgMapPosition p,int zone,unsigned char border) {
    const TObjectType& t=*properties->m_prototype;
    if(p.m_x<t.width-1 || p.m_x>=6 || p.m_y<t.height-1 || p.m_y>=6) return 1;
    for(unsigned row=0;row<(unsigned)t.height;++row) for(unsigned col=0;col<(unsigned)t.width;++col) {
        TRmgMapItem& cell=*map.getMapItem(TRmgMapPosition(p.m_x-(int)col,p.m_y-(int)row,p.m_z));
        bool trigger=t.m_triggerMask.test(47-row*8-col), blocked=!t.m_passableMask.test(47-row*8-col);
        bool bad=!cell.m_tileData.m_roadPassable || cell.m_tile.m_landType==9
            || cell.m_tileData.m_roadEntrance || cell.m_zoneState.m_zone!=zone;
        if(trigger && (bad || (border && cell.m_tileData.m_borderObject))) return 1;
        if(blocked) {
            if(bad) return 1;
            bool water=cell.m_tile.m_landType==8;
            bool waterOnly=t.m_slotCategory==0 && t.m_recommendedTerrainMask.test(8);
            if(water!=waterOnly) return 1;
        }
    }
    return 0;
}
unsigned char expectedPlace(MapFixture& map,TRmgObjectPropertiesRef* properties,
    TRmgMapPosition p,TRmgZone* zone) {
    TObjectType* saved=properties->m_prototype;
    int index=zone->m_slot->m_zoneIndex;
    if(expectedBlocked(map,properties,p,index,0)) return 0;
    int type=saved->m_objectType;
    properties->buildOutline();
    unsigned char allow=g_adventureObjectLandBlocked[type][2] && g_adventureObjectLandBlocked[type][1];
    if(!map.hasConnectedOutline(properties->m_outline,p,allow,zone,0)) return 0;
    if(!saved->m_hasTrigger) return 1;
    int x=p.m_x-saved->m_triggerCell.m_x, y=p.m_y-saved->m_triggerCell.m_y+1;
    if(y>=6) return 0;
    TRmgMapItem& cell=*map.getMapItem(TRmgMapPosition(x,y,p.m_z));
    if(!cell.m_tileData.m_roadPassable || cell.m_tile.m_landType==9
        || cell.m_zoneState.m_zone<0 || cell.m_zoneState.m_zone!=index) return 0;
    if(cell.m_tileData.m_roadEntrance && !g_adventureObjectLandBlocked[
        cell.m_objects[0]->m_properties->m_prototype->m_objectType][2]) return 0;
    return (zone->m_terrain==8)==(cell.m_tile.m_landType==8);
}
void expectedAdd(MapFixture& map,type_object* object,TRmgMapPosition p) {
    const TObjectType& t=*object->m_properties->m_prototype;
    object->m_position=p;
    for(unsigned row=0;row<(unsigned)t.height;++row) {
        int y=p.m_y-(int)row; if(y<0 || y>=6) continue;
        for(unsigned col=0;col<(unsigned)t.width;++col) {
            int x=p.m_x-(int)col; if(x<0 || x>=6) continue;
            TRmgMapItem& cell=*map.getMapItem(TRmgMapPosition(x,y,p.m_z));
            if(t.m_triggerMask.test(47-row*8-col)) {
                cell.m_tileData.m_roadEntrance=1;
                if(!cell.m_connection.m_present) {
                    cell.m_tileData.m_borderObject=0; cell.m_tileData.m_subterraneanGate=1;
                }
                cell.m_objects.push_back(object);
            } else if(!t.m_passableMask.test(47-row*8-col)) {
                cell.m_tileData.m_roadPassable=0; cell.m_objects.push_back(object);
            }
        }
    }
}
'''

CHECKS = r'''
template<class Map> void check() {
    const int origins[][2]={{-1,-1},{0,0},{1,1},{3,3},{5,5},{6,6}};
    for(int seed=0;seed<128;++seed) for(int origin=0;origin<6;++origin) for(int level=0;level<2;++level) {
        TObjectType initial;
        initial.width=1+seed%3; initial.height=1+(seed/3)%3;
        initial.m_objectType=2; initial.m_slotCategory=seed%2;
        initial.m_hasTrigger=(seed/2)%2;
        initial.m_triggerCell.m_x=seed%initial.width; initial.m_triggerCell.m_y=seed%initial.height;
        initial.m_recommendedTerrainMask.reset(); if(seed&8) initial.m_recommendedTerrainMask.set(8);
        initial.m_passableMask.set(); initial.m_triggerMask.reset();
        for(int bit=0;bit<48;++bit) {
            if((bit+seed)%7==0) initial.m_triggerMask.set(bit);
            if((bit+seed)%5==0) initial.m_passableMask.reset(bit);
        }
        if(seed%16==0) {initial.width=0; initial.m_hasTrigger=0;}
        if(seed%17==0) {initial.height=0; initial.m_hasTrigger=0;}
        mode=seed%4; if(!initial.width || !initial.height) mode=0;
        for(int i=0;i<16;++i) for(int j=0;j<16;++j) g_adventureObjectLandBlocked[i][j]=(i+j+seed)%3!=0;
        Map actual; MapFixture wanted;
        actual.connected=wanted.connected=seed%7!=0;
        TObjectType oldA=initial,oldB=initial,newA=initial,newB=initial;
        newA.m_hasTrigger=newB.m_hasTrigger=0;
        TRmgObjectPropertiesRef pa,pb; pa.m_prototype=&oldA;pb.m_prototype=&oldB;
        type_object oa,ob; oa.m_properties=&pa;ob.m_properties=&pb;
        type_object sentinel; sentinel.m_properties=&pa;
        for(int i=0;i<72;++i) {
            TRmgMapItem cell;
            cell.m_tileData.m_roadPassable=1;
            cell.m_tileData.m_roadEntrance=0;
            cell.m_tileData.m_borderObject=(i+seed)%4==0;
            cell.m_tileData.m_subterraneanGate=0;
            cell.m_tile.m_landType=(seed&8)?8:3;
            cell.m_zoneState.m_zone=2;
            cell.m_connection.m_present=(i+seed)%3==0;
            if(seed%3==0 && i==seed%72) {cell.m_tileData.m_roadEntrance=1;cell.m_objects.push_back(&sentinel);}
            if(seed%5==0 && i==seed%72) cell.m_tileData.m_roadPassable=0;
            if(seed%7==0 && i==seed%72) cell.m_tile.m_landType=9;
            if(seed%11==0 && i==seed%72) cell.m_zoneState.m_zone=-1;
            actual.cells[i]=wanted.cells[i]=cell;
        }
        TRmgMapPosition p(origins[origin][0],origins[origin][1],level);
        unsigned char a=actual.isPlacementBlocked(&pa,p,2,seed%2);
        unsigned char b=expectedBlocked(wanted,&pb,p,2,seed%2);
        assert(a==b && actual.trace==wanted.trace);
        actual.trace.clear(); wanted.trace.clear();
        Slot slot;slot.m_zoneIndex=2; TRmgZone zone;zone.m_slot=&slot;zone.m_terrain=(seed&4)?8:3;
        events=&actual.trace;observed=&oldA;replacement=&newA;
        a=actual.canPlaceObject(&pa,p,&zone);
        events=&wanted.trace;observed=&oldB;replacement=&newB;
        b=expectedPlace(wanted,&pb,p,&zone);
        assert(a==b && actual.trace==wanted.trace);
        actual.trace.clear();wanted.trace.clear();
        actual.addObject(&oa,p);expectedAdd(wanted,&ob,p);
        assert(actual.trace==wanted.trace);
        assert(oa.m_position.m_x==ob.m_position.m_x && oa.m_position.m_y==ob.m_position.m_y && oa.m_position.m_z==ob.m_position.m_z);
        for(int i=0;i<72;++i) {
            const TRmgMapItem& a=actual.cells[i];const TRmgMapItem& b=wanted.cells[i];
            assert(a.m_tileData.m_roadPassable==b.m_tileData.m_roadPassable);
            assert(a.m_tileData.m_roadEntrance==b.m_tileData.m_roadEntrance);
            assert(a.m_tileData.m_borderObject==b.m_tileData.m_borderObject);
            assert(a.m_tileData.m_subterraneanGate==b.m_tileData.m_subterraneanGate);
            assert(a.m_objects.size()==b.m_objects.size());
            for(unsigned j=0;j<a.m_objects.size();++j)
                assert((a.m_objects[j]==&oa)==(b.m_objects[j]==&ob));
        }
    }
}
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest',type=Path,nargs='?')
    parser.add_argument('--authored',action='store_true',help='check the current authored placement bodies')
    args=parser.parse_args()
    if args.authored == bool(args.manifest):
        parser.error('choose a manifest or --authored')
    if args.manifest:
        _,originals,axes=load_manifest(args.manifest,HOMM3_DIR)
    extract=generator('generate-rmg-position-family.py').definition
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    text='#include <cassert>\n#include <bitset>\n#include <vector>\n#define VA(a,b)\n'
    for name in ('TRmgVector','TPoint','TRmgMapPosition'):
        start=header.index('struct '+name+' {');end=header.index('\n};',start)+3
        text+=header[start:end]+'\n'
    start=header.index('template<class Coordinate> struct TRmgCoordinatePoint {')
    end=header.index('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')+len('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')
    text+=header[start:end]+'\n'
    for name in ('TRmgMapPosition::TRmgMapPosition','TRmgMapPosition::operator-='):
        text+=extract(source,name)+'\n'
    mapcell=(HOMM3_DIR/'include/mapcell.h').read_text()
    start=mapcell.index('    static unsigned getBitPos(');end=mapcell.index('\n    }',start)+6
    text+='class CObjectType {public:\n'+mapcell[start:end]+'\n};\n'+FIXTURE+CHECKS
    names=('isPlacementBlocked','canPlaceObject','addObject')
    cases=[]
    declarations='''struct type_random_map:MapFixture {
    unsigned char isPlacementBlocked(TRmgObjectPropertiesRef*,TRmgMapPosition,int,unsigned char);
    unsigned char canPlaceObject(TRmgObjectPropertiesRef*,TRmgMapPosition,TRmgZone*);
    void addObject(type_object*,TRmgMapPosition);
};
'''
    model_sources = [source] if args.authored else [render(originals,axes,(i,))['src/rmg.cpp']
                                                   for i in range(len(axes[0].options))]
    for i, src in enumerate(model_sources):
        body='\n'.join(extract(src,'type_random_map::'+name) for name in names)
        case_text = text + declarations + body + '\nint main(){check<type_random_map>();}\n'
        if 'type_random_map::addObject(type_object& object,' in body:
            case_text = case_text.replace('void addObject(type_object*,TRmgMapPosition);',
                                          'void addObject(type_object&,TRmgMapPosition);')
            case_text = case_text.replace('actual.addObject(&oa,p);', 'actual.addObject(oa,p);')
        cases.append((str(i),case_text,True))
    positive=cases[-1][1]
    mask_args=('maskPoint.getX(), maskPoint.getY()' if 'CObjectType::getBitPos(maskPoint.getX()' in positive
               else ('maskPoint.m_x, maskPoint.m_y' if 'CObjectType::getBitPos(maskPoint.m_x' in positive else 'x, y'))
    flipped=', '.join(reversed(mask_args.split(', ')))
    world_step = '--nearby.m_y' if '--nearby.m_y' in positive else ('--row' if '--row' in positive else '--position.m_y')
    controls=[
        ('wrong_mask_axis','CObjectType::getBitPos('+mask_args+')','CObjectType::getBitPos('+flipped+')'),
        ('wrong_world_step', world_step, world_step.replace('--', '++')),
        ('trigger_row','++entrance.m_y;' if '++entrance.m_y;' in positive else '++y;',
            '--entrance.m_y;' if '++entrance.m_y;' in positive else '--y;'),
        ('missing_registration', 'item->m_objects.push_back(&object);' if 'item->m_objects.push_back(&object);' in positive else 'item->m_objects.push_back(object);', ';'),
        ('reload_owner','if (!prototype.m_hasTrigger)','if (!properties->m_prototype->m_hasTrigger)'),
    ]
    if 'entrance -= TPoint(' in positive:
        controls.append(('trigger_direction','m_x -= offset.m_x;','m_x += offset.m_x;'))
    else:
        if 'maskPoint.setY(maskPoint.getY() + 1)' in positive:
            controls.append(('wrong_mask_step','maskPoint.setY(maskPoint.getY() + 1)',
                             'maskPoint.setY(maskPoint.getY() - 1)'))
        elif '++maskPoint.m_x' in positive:
            controls.append(('wrong_mask_step','++maskPoint.m_x','--maskPoint.m_x'))
        elif 'TRmgGridPoint maskPoint(x, y);' in positive:
            controls.append(('wrong_mask_origin', 'TRmgGridPoint maskPoint(x, y);',
                             'TRmgGridPoint maskPoint(x + 1, y);'))
        else:
            controls.append(('wrong_mask_origin', 'CObjectType::getBitPos(x, y)',
                             'CObjectType::getBitPos(x + 1, y)'))
    for name,old,new in controls:
        assert old in positive
        cases.append((name,positive.replace(old,new),False))
    with tempfile.TemporaryDirectory(prefix='rmg-placement-domains-oracle-') as tmp:
        for name,src,expected in cases:
            path=Path(tmp)/name;path.with_suffix('.cpp').write_text(src)
            subprocess.run(['g++','-std=c++98','-O1','-fsanitize=undefined',str(path.with_suffix('.cpp')),'-o',str(path)],check=True)
            result=subprocess.run([str(path)],capture_output=True)
            assert (result.returncode==0 and not result.stderr)==expected,(name,result.stderr.decode())
    print('%d actual models × 1536 scenarios; opaque-call mutations checked; %d wrong controls rejected; bounded signed arithmetic passes UBSan' % (len(model_sources),len(controls)))


if __name__=='__main__':
    main()
