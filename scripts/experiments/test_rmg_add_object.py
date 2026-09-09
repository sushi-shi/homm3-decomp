"""Object registration and score propagation, with an independent fixed point.

Actual coordinate/packed types, lookup, base registration and sorted helper
are imported. Map registration is an opaque scripted boundary; reduced owners
are not retail layout evidence. Allocation succeeds and scores stay bounded.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class AddObjectTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_registration_and_distance_flood(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        support = (root / "src/rmg_support.cpp").read_text()
        objects = (root / "include/advmgr_objects.h").read_text()
        mapcell = (root / "include/mapcell.h").read_text()
        module = generator("generate-rmg-add-object-family.py")
        definition = generator("generate-rmg-position-family.py").definition
        original = module.definition(source)
        forms = list(module.variants(original))
        selected = os.environ.get("HOMM3_ADD_OBJECT_MANIFEST", "")
        for path in filter(None, selected.split(os.pathsep)):
            payload = json.loads(Path(path).read_text())
            forms += [(o["name"], o["replace"]) for o in payload["axes"][0]["options"]]
        forms = list(dict((body, name) for name, body in forms).items())
        positive_count = len(forms)
        negative = [
            ("global_count", "++m_objectCountByType[objectType];", "m_objectCountByType[objectType] += 2;"),
            ("zone_count", "if (zoneIndex >= 0)", "if (zoneIndex > 0)"),
            ("wrong_seed", "seed->m_zoneState.m_score = 0;", "seed->m_zoneState.m_score = 1;"),
            ("wrong_step", "m_zoneState.m_score + 2;", "m_zoneState.m_score + 3;"),
            ("wrong_diagonal", "if (direction & 1)", "if (!(direction & 1))"),
            ("wrong_level", "nextPosition.m_z = currentPosition.m_z;", "nextPosition.m_z = 0;"),
            ("wrong_trigger", "position.m_x - trigger.m_x", "position.m_x + trigger.m_x"),
            ("missing_seed", "positions.push_back(currentPosition);", ""),
            ("missing_registration", "    TRmgGeneratorBase::addObject(object, position);\n", ""),
            ("early_prototype", "    TRmgGeneratorBase::addObject(object, position);\n    TObjectType* prototype = object->m_properties->m_prototype;",
             "    TObjectType* prototype = object->m_properties->m_prototype;\n    TRmgGeneratorBase::addObject(object, position);"),
        ]
        for name, old, new in negative:
            self.assertEqual(original.count(old), 1, name)
            forms.append((original.replace(old, new), name))

        def block(text, prefix):
            start = text.index(prefix + " {")
            return text[start:text.index("\n};", start) + 3]

        def field(text, name):
            return next(line.split("//")[0].strip() for line in text.splitlines() if name + ";" in line)

        text = "#include <vector>\n#include <cstring>\n#include <cstdio>\n"
        for name in ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgMovementCost",
                     "TRmgZoneCellState", "TRmgGroundTile", "TRmgGroundTileData", "TRmgConnectionDecoration"):
            text += block(header, "struct " + name) + "\n"
        text += definition(support, "TRmgMapPosition::TRmgMapPosition") + "\n"
        text += definition(source, "TRmgMapPosition::operator+=") + "\n"
        text += block(mapcell, "enum TAdventureObjectType") + "\n"
        start = objects.index("    struct TPoint {")
        nested = objects[start:objects.index("\n    };", start) + 7]
        text += "struct TObjectType {\n" + nested + "\n"
        text += "\n".join(field(objects, name) for name in ("m_objectType", "m_hasTrigger", "m_triggerCell")) + "\n};\n"
        text += "struct TRmgObjectPropertiesRef { " + field(header, "m_prototype") + " };\n"
        text += "struct type_object { " + field(header, "m_properties") + " };\n"
        text += r'''
struct TRmgMapItem {
    TRmgMapPosition m_previousTile; TRmgMovementCost m_movement; TRmgZoneCellState m_zoneState;
    TRmgGroundTile m_tile; TRmgGroundTileData m_tileData; TRmgConnectionDecoration m_connection;
};
struct type_random_map {
    int m_mapWidth,m_mapHeight; TRmgMapItem* m_mapItems;
    TRmgObjectPropertiesRef* m_replacement;
    std::vector<int> m_trace;
    void addObject(type_object* object,TRmgMapPosition position) {
        m_trace.push_back(position.m_x);m_trace.push_back(position.m_y);m_trace.push_back(position.m_z);
        m_trace.push_back(object->m_properties->m_prototype->m_objectType);
        if(m_replacement) object->m_properties=m_replacement;
    }
    TRmgMapItem* getMapItem(TRmgMapPosition point);
'''
        text += definition(header, "getMapItem", parameters="int x, int y, int z") + "\n};\n"
        text += definition(source, "type_random_map::getMapItem", parameters="TRmgMapPosition point") + "\n"
        text += r'''
struct TRmgGeneratorBase {
    type_random_map m_map; std::vector<type_object*> m_positions;
    virtual void addObject(type_object*,TRmgMapPosition);
};
'''
        text += definition(source, "TRmgGeneratorBase::addObject") + "\n"
        counts = next(line.split("//")[0].strip() for line in header.splitlines() if "m_objectCountByType[" in line)
        text += "struct TRmgZone { " + counts + " };\n"
        text += "struct GeneratorFixture : TRmgGeneratorBase { " + counts + " std::vector<TRmgZone*> m_zones; };\n"
        start = source.index("TPoint g_rmgDirections[RMG_DIRECTION_COUNT] = {")
        text += "enum { RMG_DIRECTION_COUNT=8 };\n" + source[start:source.index("\n};", start) + 3] + "\n"
        text += definition(source, "insertRmgWorkItem", parameters=
            "std::vector<TRmgMapPosition>& positions, std::vector<int>& costs, TRmgMapPosition position, int cost") + "\n"
        text += r'''
void reference(GeneratorFixture& owner,type_object* object,TRmgMapPosition position) {
    owner.TRmgGeneratorBase::addObject(object,position);
    const TObjectType& prototype=*object->m_properties->m_prototype;
    ++owner.m_objectCountByType[prototype.m_objectType];
    if(!prototype.m_hasTrigger) return;
    int width=owner.m_map.m_mapWidth,height=owner.m_map.m_mapHeight;
    int sx=position.m_x-prototype.m_triggerCell.m_x,sy=position.m_y-prototype.m_triggerCell.m_y;
    int seed=(position.m_z*height+sy)*width+sx;
    TRmgMapItem* cells=owner.m_map.m_mapItems;
    int zone=cells[seed].m_zoneState.m_zone;
    if(zone>=0) ++owner.m_zones[zone]->m_objectCountByType[prototype.m_objectType];
    cells[seed].m_zoneState.m_score=0;
    // Independent monotone fixed point: no priority queue or binary insertion.
    std::vector<unsigned char> reached(width*height*2,0);reached[seed]=1;
    bool changed=true;
    while(changed) {
        changed=false;
        for(int i=0;i<width*height*2;++i) if(reached[i]) {
            int x=i%width,y=(i/width)%height,z=i/(width*height);
            for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
                if((dx==0 && dy==0) || x+dx<0 || x+dx>=width || y+dy<0 || y+dy>=height) continue;
                int next=(z*height+y+dy)*width+x+dx;
                unsigned proposed=cells[i].m_zoneState.m_score+2+(dx!=0 && dy!=0);
                if(proposed<cells[next].m_zoneState.m_score) {
                    cells[next].m_zoneState.m_score=proposed;reached[next]=1;changed=true;
                }
            }
        }
    }
}
template<class Candidate> bool check() {
    int widths[]={1,3,5},heights[]={1,4};
    for(int wi=0;wi<3;++wi) for(int hi=0;hi<2;++hi) for(int level=0;level<2;++level)
    for(int end=0;end<2;++end) for(int offset=0;offset<3;++offset) for(int active=0;active<3;++active)
    for(int zone=-1;zone<2;++zone) for(int pattern=0;pattern<3;++pattern) for(int mutate=0;mutate<2;++mutate) {
        int w=widths[wi],h=heights[hi],count=w*h*2;
        std::vector<TRmgMapItem> a(count+128),b(count+128);
        for(int i=0;i<count+128;++i) {
            std::memset(&a[i],0,sizeof(a[i]));
            a[i].m_zoneState.m_score=(pattern==1 && i%4==0) ? 0 : (pattern==2 ? 2+i%7 : 32000);
            a[i].m_zoneState.m_zone=zone;
            a[i].m_zoneState.m_connectionEligibility=i%5;
            a[i].m_movement.m_cost=i*7;
            a[i].m_movement.m_zonePathCost=i*11;
            a[i].m_previousTile=TRmgMapPosition(7,11,13);
            unsigned bits=0x91326478U+unsigned(i)*317U;
            std::memcpy(&a[i].m_tile,&bits,4);std::memcpy(&a[i].m_tileData,&bits,4);std::memcpy(&a[i].m_connection,&bits,4);
            if(i<64 || i>=count+64) a[i].m_zoneState.m_zone=-1;
        }
        b=a;
        TObjectType first,last;
        first.m_objectType=static_cast<TAdventureObjectType>(1);last.m_objectType=static_cast<TAdventureObjectType>(4);
        last.m_hasTrigger=active==2 ? 255 : active;
        first.m_hasTrigger=mutate ? (active ? 0 : 1) : last.m_hasTrigger;
        first.m_triggerCell.m_x=last.m_triggerCell.m_x=offset-1;
        first.m_triggerCell.m_y=last.m_triggerCell.m_y=1-offset;
        TRmgObjectPropertiesRef oldProperties={&first},newProperties={&last};
        type_object oa={&oldProperties},ob={&oldProperties};
        Candidate actual;GeneratorFixture expected;
        TRmgZone za[2],zb[2];
        for(int type=0;type<232;++type) {
            actual.m_objectCountByType[type]=expected.m_objectCountByType[type]=type%7;
            for(int z=0;z<2;++z) za[z].m_objectCountByType[type]=zb[z].m_objectCountByType[type]=type%3;
        }
        for(int z=0;z<2;++z) {actual.m_zones.push_back(&za[z]);expected.m_zones.push_back(&zb[z]);}
        actual.m_positions.push_back(0);expected.m_positions.push_back(0);
        actual.m_map.m_mapWidth=expected.m_map.m_mapWidth=w;actual.m_map.m_mapHeight=expected.m_map.m_mapHeight=h;
        actual.m_map.m_mapItems=&a[64];expected.m_map.m_mapItems=&b[64];
        actual.m_map.m_replacement=expected.m_map.m_replacement=mutate ? &newProperties : 0;
        TRmgMapPosition input((end?w-1:0)+first.m_triggerCell.m_x,(end?h-1:0)+first.m_triggerCell.m_y,level);
        TRmgMapPosition saved=input;
        actual.addObject(&oa,input);reference(expected,&ob,input);
        if(std::memcmp(&a[0],&b[0],a.size()*sizeof(TRmgMapItem))) return false;
        if(std::memcmp(actual.m_objectCountByType,expected.m_objectCountByType,sizeof(actual.m_objectCountByType))) return false;
        if(std::memcmp(za,zb,sizeof(za))) return false;
        if(actual.m_map.m_trace!=expected.m_map.m_trace || oa.m_properties!=ob.m_properties) return false;
        if(actual.m_positions.size()!=2 || actual.m_positions[0]!=0 || actual.m_positions[1]!=&oa) return false;
        if(input.m_x!=saved.m_x || input.m_y!=saved.m_y || input.m_z!=saved.m_z) return false;
    }
    return true;
}
'''
        for index, (body, _) in enumerate(forms):
            text += "namespace N%d { struct type_random_map_generator : GeneratorFixture { void addObject(type_object*,TRmgMapPosition); };\n" % index
            text += body + "\n}\n"
        text += "int main() {\n"
        for index, (_, name) in enumerate(forms):
            text += 'if(%scheck<N%d::type_random_map_generator>()) {std::puts("failed %s");return 1;}\n' % (
                "!" if index < positive_count else "", index, name)
        text += 'std::puts("%d object-distance forms: 3888 scenarios each; ten negative controls rejected");return 0;}\n' % positive_count
        with tempfile.TemporaryDirectory(prefix="rmg-add-object-oracle-") as folder:
            cpp,binary=Path(folder)/"oracle.cpp",Path(folder)/"oracle"
            cpp.write_text(text)
            subprocess.run(["g++","-std=c++11","-O2","-fno-elide-constructors",str(cpp),"-o",str(binary)],check=True)
            subprocess.run([str(binary)],check=True,timeout=120)


if __name__ == "__main__":
    unittest.main()
