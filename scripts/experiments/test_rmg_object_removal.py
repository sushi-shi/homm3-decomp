"""Object removal oracle: independent cell selection and first-occurrence filter.

Imports actual coordinate, mask, packed-field and accessor definitions. Host
iterators expose their raw pointer only for the retail's observed null tests.
Nonempty lists contain the object; erase(end) behavior is outside the contract.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class ObjectRemovalTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_counts_footprints_and_borrowed_lists(self):
        root=Path(__file__).resolve().parents[2]
        source=(root/"src/rmg.cpp").read_text()
        header=(root/"include/rmg.h").read_text()
        support=(root/"src/rmg_support.cpp").read_text()
        objects=(root/"include/advmgr_objects.h").read_text()
        mapcell=(root/"include/mapcell.h").read_text()
        module=generator("generate-rmg-object-removal-family.py")
        definition=generator("generate-rmg-position-family.py").definition
        current=module.definition(source)
        original=module.scalar_seed(current)
        forms=[("current",current)]+list(module.variants(original))
        for path in filter(None,os.environ.get("HOMM3_OBJECT_REMOVAL_MANIFEST","").split(os.pathsep)):
            payload=json.loads(Path(path).read_text())
            forms += [(o["name"],o["replace"]) for o in payload["axes"][0]["options"]]
        forms=list(dict((body,name) for name,body in forms).items())
        positives=len(forms)
        negatives=[
            ("global_count","--m_objectCountByType[prototype->m_objectType];","++m_objectCountByType[prototype->m_objectType];"),
            ("zone_count","if (zone >= 0)","if (zone > 0)"),
            ("color_release","m_disabledKeyTents[prototype->m_subtype] = 0;","m_disabledKeyTents[prototype->m_subtype] = 1;"),
            ("next_color","m_nextKeyTentColor = 0;","m_nextKeyTentColor = 1;"),
            ("global_erase","m_positions.erase(found);",""),
            ("cell_erase","item->m_objects.erase(entry);",""),
            ("wrong_level","getMapItem(mapX, mapY, position.m_z)","getMapItem(mapX, mapY, 0)"),
            ("left_edge","if (mapX < 0 ||", "if (mapX <= 0 ||"),
            ("passable","!prototype->m_passableMask.test", "prototype->m_passableMask.test"),
            ("trigger_union","|| prototype->m_triggerMask.test", "&& prototype->m_triggerMask.test"),
            ("score","m_score = 32700;","m_score = 32699;"),
            ("empty_flags","if (item->m_objects.empty())", "if (!item->m_objects.empty())"),
        ]
        for name,old,new in negatives:
            self.assertEqual(original.count(old),1,name)
            forms.append((original.replace(old,new),name))

        def block(text,prefix):
            start=text.index(prefix+" {")
            return text[start:text.index("\n};",start)+3]

        def field(text,name):
            return next(line.split("//")[0].strip() for line in text.splitlines() if name+";" in line)

        text="#include <vector>\n#include <bitset>\n#include <algorithm>\n#include <cstring>\n#include <cstdio>\n"
        for name in ("TRmgVector","TPoint","TRmgMapPosition","TRmgGridPoint","TRmgZoneCellState","TRmgGroundTileData"):
            text+=block(header,"struct "+name)+"\n"
        text+=definition(support,"TRmgMapPosition::TRmgMapPosition")+"\n"
        text+=block(mapcell,"enum TAdventureObjectType")+"\n"
        start=objects.index("    struct TPoint {")
        nested_end=objects.index("\n    };",objects.index("    struct TImageInfo {",start))+7
        text+="struct TObjectType {\n"+objects[start:nested_end]+"\n"
        for name in ("m_objectType","m_subtype","m_triggerCell","m_imageInfo","m_passableMask","m_triggerMask"):
            text+=field(objects,name)+"\n"
        text+=definition(objects,"getWidth",parameters="")+"\n"
        text+=definition(objects,"getHeight",parameters="")+"\n};\n"
        text+="struct CObjectType {\n"+definition(objects,"getBitPos",parameters="unsigned x, unsigned y")+"\n};\n"
        text+="struct TRmgObjectPropertiesRef { "+field(header,"m_prototype")+" };\n"
        text+="struct type_object { "+field(header,"m_properties")+" "+field(header,"m_position")+" };\n"
        text+="struct TRmgMapItem { "+field(header,"m_objects")+" TRmgZoneCellState m_zoneState; TRmgGroundTileData m_tileData; };\n"
        text+="struct type_random_map { int m_mapWidth,m_mapHeight; TRmgMapItem* m_mapItems; TRmgMapItem* getMapItem(TRmgMapPosition point);\n"
        text+=definition(header,"getMapItem",parameters="int x, int y, int z")+"\n};\n"
        text+=definition(source,"type_random_map::getMapItem",parameters="TRmgMapPosition point")+"\n"
        counts=next(line.split("//")[0].strip() for line in header.splitlines() if "m_objectCountByType[" in line)
        text+="struct TRmgZone { "+counts+" };\n"
        text+="struct GeneratorFixture { type_random_map m_map; std::vector<TRmgZone*> m_zones; "+counts+" std::vector<type_object*> m_positions; std::vector<unsigned char> m_disabledKeyTents; int m_nextKeyTentColor; };\n"
        text+=r'''
static void removeFirst(std::vector<type_object*>& values,type_object* object) {
    std::vector<type_object*> kept;bool removed=false;
    for(unsigned i=0;i<values.size();++i) {
        if(!removed && values[i]==object) removed=true;
        else kept.push_back(values[i]);
    }
    values.swap(kept);
}
static void reference(GeneratorFixture& owner,type_object* object) {
    const TObjectType& p=*object->m_properties->m_prototype;
    TRmgMapPosition position=object->m_position;
    bool registered=false;
    for(unsigned i=0;i<owner.m_positions.size();++i) registered|=owner.m_positions[i]==object;
    if(registered) {
        removeFirst(owner.m_positions,object);--owner.m_objectCountByType[p.m_objectType];
        int linear=position.m_z*owner.m_map.m_mapHeight*owner.m_map.m_mapWidth
            +(position.m_y-p.m_triggerCell.m_y)*owner.m_map.m_mapWidth+position.m_x-p.m_triggerCell.m_x;
        int zone=owner.m_map.m_mapItems[linear].m_zoneState.m_zone;
        if(zone>=0) --owner.m_zones[zone]->m_objectCountByType[p.m_objectType];
    }
    if(p.m_objectType==BORDER_GUARD) {
        owner.m_disabledKeyTents[p.m_subtype]=0;
        std::vector<int> available;
        for(unsigned i=0;i<owner.m_disabledKeyTents.size();++i) if(!owner.m_disabledKeyTents[i]) available.push_back(i);
        owner.m_nextKeyTentColor=available.empty()?int(owner.m_disabledKeyTents.size()):available.front();
    }
    // Enumerate destination cells, not the candidate's reverse footprint scan.
    for(int linear=0;linear<owner.m_map.m_mapWidth*owner.m_map.m_mapHeight*2;++linear) {
        int x=linear%owner.m_map.m_mapWidth;
        int y=(linear/owner.m_map.m_mapWidth)%owner.m_map.m_mapHeight;
        int z=linear/(owner.m_map.m_mapWidth*owner.m_map.m_mapHeight);
        int dx=position.m_x-x,dy=position.m_y-y;
        if(z!=position.m_z || dx<0 || dy<0 || dx>=p.m_imageInfo.m_objectSize.m_x || dy>=p.m_imageInfo.m_objectSize.m_y) continue;
        int bit=8*(5-dy)+(7-dx);
        if(p.m_passableMask[bit] && !p.m_triggerMask[bit]) continue;
        TRmgMapItem& cell=owner.m_map.m_mapItems[linear];
        bool contains=false;
        for(unsigned i=0;i<cell.m_objects.size();++i) contains|=cell.m_objects[i]==object;
        if(contains) {
            removeFirst(cell.m_objects,object);
            if(cell.m_objects.size()==0) { cell.m_tileData.m_roadEntrance=0;cell.m_tileData.m_roadPassable=1; }
            cell.m_zoneState.m_score=32700;
        }
    }
}
template<class Candidate> bool check() {
    int widths[]={0,1,8},heights[]={0,1,6};
    for(int wi=0;wi<3;++wi) for(int hi=0;hi<3;++hi) for(int map=0;map<2;++map)
    for(int origin=0;origin<4;++origin) for(int level=0;level<2;++level) for(int mask=0;mask<4;++mask)
    for(int kind=0;kind<2;++kind) for(int registered=0;registered<2;++registered)
    for(int population=0;population<2;++population) for(int zoneMode=0;zoneMode<4;++zoneMode) {
        int w=map?6:3,h=map?3:4,n=w*h*2;
        Candidate actual;GeneratorFixture expected;TRmgZone az[2],ez[2];
        for(int i=0;i<232;++i) {
            actual.m_objectCountByType[i]=expected.m_objectCountByType[i]=100+i;
            for(int j=0;j<2;++j) az[j].m_objectCountByType[i]=ez[j].m_objectCountByType[i]=200+j*300+i;
        }
        for(int i=0;i<128;++i) { actual.m_zones.push_back(&az[i%2]);expected.m_zones.push_back(&ez[i%2]); }
        TObjectType prototype;prototype.m_objectType=kind?BORDER_GUARD:BORDER_TENT;prototype.m_subtype=(origin+mask)%4;
        prototype.m_imageInfo.m_objectSize.m_x=widths[wi];prototype.m_imageInfo.m_objectSize.m_y=heights[hi];
        prototype.m_triggerCell.m_x=mask-1;prototype.m_triggerCell.m_y=1-mask;
        for(int bit=0;bit<48;++bit) {
            prototype.m_passableMask[bit]=mask<2 || bit%3==0;
            prototype.m_triggerMask[bit]=(mask&1) && bit%5<2;
        }
        TRmgObjectPropertiesRef properties;properties.m_prototype=&prototype;
        type_object object,other;object.m_properties=&properties;other.m_properties=&properties;
        object.m_position.m_x=origin==0?-1:origin==1?0:origin==2?w-1:w+1;
        object.m_position.m_y=origin<2?1:origin==2?h-1:h+1;object.m_position.m_z=level;
        TRmgMapPosition savedPosition=object.m_position;
        std::vector<TRmgMapItem> a(n+128),b(n+128);
        for(int i=0;i<n+128;++i) {
            unsigned zoneBits=0x89123456U+unsigned(i)*7919U,tileBits=0xa3175239U+unsigned(i)*1337U;
            std::memcpy(&a[i].m_zoneState,&zoneBits,4);std::memcpy(&a[i].m_tileData,&tileBits,4);
            a[i].m_tileData.m_roadEntrance=1;a[i].m_tileData.m_roadPassable=0;
            a[i].m_zoneState.m_zone=(i<64 || i>=64+n)?-1:zoneMode==0?-128:zoneMode==1?-1:zoneMode==2?i%2:126+i%2;
            if(i>=64 && i<64+n) {
                if(population) a[i].m_objects.push_back(&other);
                a[i].m_objects.push_back(&object);
                if(population) a[i].m_objects.push_back(&object);
            }
            b[i]=a[i];
        }
        actual.m_map.m_mapWidth=expected.m_map.m_mapWidth=w;
        actual.m_map.m_mapHeight=expected.m_map.m_mapHeight=h;
        actual.m_map.m_mapItems=&a[64];expected.m_map.m_mapItems=&b[64];
        if(registered) {
            if(population) actual.m_positions.push_back(&other);
            actual.m_positions.push_back(&object);
            if(population) actual.m_positions.push_back(&object);
        }
        expected.m_positions=actual.m_positions;
        for(int i=0;i<4;++i) actual.m_disabledKeyTents.push_back(((mask+5)>>i)&1);
        expected.m_disabledKeyTents=actual.m_disabledKeyTents;
        actual.m_nextKeyTentColor=expected.m_nextKeyTentColor=3;
        reference(expected,&object);actual.removeObject(&object);
        if(actual.m_positions!=expected.m_positions || actual.m_disabledKeyTents!=expected.m_disabledKeyTents
            || actual.m_nextKeyTentColor!=expected.m_nextKeyTentColor
            || std::memcmp(actual.m_objectCountByType,expected.m_objectCountByType,sizeof(actual.m_objectCountByType))
            || std::memcmp(az,ez,sizeof(az))) return false;
        if(object.m_properties!=&properties || object.m_position.m_x!=savedPosition.m_x
            || object.m_position.m_y!=savedPosition.m_y || object.m_position.m_z!=savedPosition.m_z) return false;
        for(int i=0;i<n+128;++i) if(a[i].m_objects!=b[i].m_objects
            || std::memcmp(&a[i].m_zoneState,&b[i].m_zoneState,4)
            || std::memcmp(&a[i].m_tileData,&b[i].m_tileData,4)) return false;
    }
    return true;
}
'''
        for index,(body,name) in enumerate(forms):
            # Dinkumware iterators are pointers; preserve the exact null predicate.
            body=body.replace("if (found)","if (found.base())").replace("if (entry)","if (entry.base())")
            text+="namespace N%d { struct type_random_map_generator : GeneratorFixture { void removeObject(type_object*); };\n"%index+body+"\n}\n"
        text+="int main() {\n"
        for index,(body,name) in enumerate(forms):
            text+='if(check<N%d::type_random_map_generator>() != %s) { std::fprintf(stderr,"failed %s\\n"); return 1; }\n'%(index,"true" if index<positives else "false",name)
        text+='std::printf("%d removal forms; 18432 scenarios each; twelve negative controls rejected\\n");}\n'%positives
        with tempfile.TemporaryDirectory(prefix="homm3-object-removal-") as directory:
            source_path=Path(directory)/"oracle.cpp";source_path.write_text(text)
            executable=Path(directory)/"oracle"
            subprocess.run(["g++","-std=c++11","-O2","-fno-elide-constructors",str(source_path),"-o",str(executable)],check=True,timeout=120)
            subprocess.run([str(executable)],check=True,timeout=180)


if __name__=="__main__":
    unittest.main()
