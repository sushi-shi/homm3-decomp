"""Independent caller oracle for junction resets and entrance connections.

Packed/value types and accessors are actual source. Flooding and carving are
opaque scripted boundaries; reduced owners make no host/x86 layout claim.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class JunctionPrepareTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_coordinate_families(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        support = (root / "src/rmg_support.cpp").read_text()
        definition = generator("generate-rmg-position-family.py").definition
        module = generator("generate-rmg-junction-prepare-family.py")
        original = module.baseline_definition(source)
        forms = list(module.variants(original))
        forms.append(("authored", module.authored_definition(source)))
        if os.environ.get("HOMM3_JUNCTION_MANIFEST"):
            for manifest_path in os.environ["HOMM3_JUNCTION_MANIFEST"].split(os.pathsep):
                manifest = json.loads(Path(manifest_path).read_text())
                forms += [(option["name"], option["replace"]) for option in manifest["axes"][0]["options"]]
        forms = list(dict((body, name) for name, body in forms).items())
        positives = len(forms)
        negative = [
            ("wrong_zone", "item->m_zoneState.m_zone == zoneIndex", "item->m_zoneState.m_zone != zoneIndex"),
            ("wrong_water", "item->m_tile.m_landType != eTerrainWater", "item->m_tile.m_landType == eTerrainWater"),
            ("wrong_reset", "previous.m_y = -1;", "previous.m_y = 0;"),
            ("wrong_gate", "item->m_tileData.m_subterraneanGate = 0;", "item->m_tileData.m_subterraneanGate = 1;"),
            ("wrong_object_filter", "static_cast<int>(item->m_objects.size()) <= 0", "static_cast<int>(item->m_objects.size()) > 0"),
            ("wrong_connection", "!item->m_connection.m_present", "item->m_connection.m_present"),
            ("wrong_threshold", "cost > 30000", "cost >= 30000"),
            ("wrong_level", "int level = position.m_z;", "int level = 1 - position.m_z;"),
            ("wrong_endpoint", "TPoint(previous.m_x, previous.m_y)", "from"),
            ("missing_flood", "m_map.floodConnectionCosts(TRmgMapPosition(from.m_x, from.m_y, level), 0);", ""),
            ("cached_entrances", "    for (int entrance = 1; entrance < static_cast<int>(zone->m_entrances.size()); ++entrance)",
             "    int count = zone->m_entrances.size();\n    for (int entrance = 1; entrance < count; ++entrance)"),
        ]
        for name, old, new in negative:
            self.assertEqual(original.count(old), 1, name)
            forms.append((original.replace(old, new), name))
        text = "#include <vector>\n#include <cstdio>\n#include <cstring>\n#include <cstdlib>\n"
        for name in ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds",
                     "TRmgMovementCost", "TRmgZoneCellState", "TRmgGroundTile",
                     "TRmgGroundTileData", "TRmgConnectionDecoration"):
            start = header.index("struct " + name + " {")
            text += header[start:header.index("\n};", start) + 3] + "\n"
        text += definition(support, "TRmgMapPosition::TRmgMapPosition") + "\n"
        text += "enum { eTerrainWater=8 };\nstruct TRmgMapItem {\nstd::vector<int*> m_objects; TRmgMapPosition m_previousTile; TRmgMovementCost m_movement; TRmgZoneCellState m_zoneState; TRmgGroundTile m_tile; TRmgGroundTileData m_tileData; TRmgConnectionDecoration m_connection;\n"
        text += definition(header, "resetMovement") + "\n};\n"
        text += r'''
unsigned word(const void* field) { unsigned value; std::memcpy(&value,field,4); return value; }
void appendCell(std::vector<int>& trace,const TRmgMapItem& cell) {
    trace.push_back(cell.m_previousTile.m_x);trace.push_back(cell.m_previousTile.m_y);trace.push_back(cell.m_previousTile.m_z);
    trace.push_back(word(&cell.m_movement));trace.push_back(word(&cell.m_zoneState));trace.push_back(word(&cell.m_tile));
    trace.push_back(word(&cell.m_tileData));trace.push_back(word(&cell.m_connection));trace.push_back(cell.m_objects.size());
}
struct type_random_map {
    int m_mapWidth,m_mapHeight;
    TRmgMapItem* m_mapItems;
    std::vector<int>* m_trace;
    int m_mode;
    TRmgMapPosition m_seed;
'''
        text += definition(header, "getMapItem", parameters="int x, int y, int z") + "\n"
        text += r'''
    void floodConnectionCosts(TRmgMapPosition point,unsigned char flag) {
        m_trace->push_back(1);m_trace->push_back(point.m_x);m_trace->push_back(point.m_y);m_trace->push_back(point.m_z);m_trace->push_back(flag);
        int count=m_mapWidth*m_mapHeight*2;
        for(int i=0;i<count;++i) appendCell(*m_trace,m_mapItems[i]);
        m_seed=point;
        for(int i=0;i<count;++i) {
            int x=i%m_mapWidth,y=(i/m_mapWidth)%m_mapHeight,z=i/(m_mapWidth*m_mapHeight);
            int distance=std::abs(x-point.m_x)+std::abs(y-point.m_y)+std::abs(z-point.m_z);
            m_mapItems[i].m_movement.m_cost=distance ? (m_mode==0 ? distance*2 : m_mode==1 ? 30000 : m_mode==2 ? 30001 : 0) : 0;
            TRmgMapPosition previous(x,y,z);
            if(z!=point.m_z) previous.m_z=point.m_z;
            else if(x!=point.m_x) previous.m_x += x<point.m_x ? 1 : -1;
            else if(y!=point.m_y) previous.m_y += y<point.m_y ? 1 : -1;
            m_mapItems[i].m_previousTile=previous;
        }
    }
};
struct TRmgTownSlot { int m_zoneIndex; };
struct TRmgZone {
    TRmgTownSlot* m_slot; TRmgZoneBounds m_bounds; TRmgMapPosition m_levelPosition;
    std::vector<TPoint> m_entrances;
    TRmgMapPosition getLevelPosition() const;
};
'''
        text += definition(source, "TRmgZone::getLevelPosition") + "\n"
        text += r'''
struct GeneratorFixture {
    type_random_map m_map;
    std::vector<int> m_trace;
    int m_connectCount;
    void connectJunctionEntrance(TPoint from,TPoint to,TRmgZone* zone) {
        m_trace.push_back(2);m_trace.push_back(from.m_x);m_trace.push_back(from.m_y);
        m_trace.push_back(to.m_x);m_trace.push_back(to.m_y);
        // Opaque source mutation checks that the original level remains owned.
        zone->m_levelPosition.m_z=1-zone->m_levelPosition.m_z;
        if(++m_connectCount==1) zone->m_entrances.push_back(TPoint(0,m_map.m_mapHeight-1));
    }
};
void reference(GeneratorFixture& owner,TRmgZone* zone) {
    int level=zone->m_levelPosition.m_z;
    int count=owner.m_map.m_mapWidth*owner.m_map.m_mapHeight*2;
    for(int i=0;i<count;++i) {
        int x=i%owner.m_map.m_mapWidth,y=(i/owner.m_map.m_mapWidth)%owner.m_map.m_mapHeight,z=i/(owner.m_map.m_mapWidth*owner.m_map.m_mapHeight);
        TRmgMapItem& item=owner.m_map.m_mapItems[i];
        if(z!=level || x<zone->m_bounds.m_minimumX || x>=zone->m_bounds.m_maximumX || y<zone->m_bounds.m_minimumY || y>=zone->m_bounds.m_maximumY) continue;
        if(item.m_zoneState.m_zone!=zone->m_slot->m_zoneIndex || item.m_tile.m_landType==eTerrainWater) continue;
        item.m_movement.m_cost=32000;
        item.m_previousTile.m_x=-1;item.m_previousTile.m_y=-1;item.m_previousTile.m_z=-1;
        if(item.m_objects.empty() && item.m_connection.m_present==0) {
            item.m_tileData.m_subterraneanGate=0;item.m_tileData.m_borderObject=1;
        }
    }
    if(zone->m_entrances.empty()) return;
    TPoint first=zone->m_entrances.front();
    TRmgMapItem* seed=owner.m_map.getMapItem(first.m_x,first.m_y,level);
    seed->m_movement.m_cost=0;
    seed->m_previousTile.m_x=-1;seed->m_previousTile.m_y=-1;seed->m_previousTile.m_z=-1;
    owner.m_map.floodConnectionCosts(TRmgMapPosition(first.m_x,first.m_y,level),0);
    for(unsigned i=1;i<zone->m_entrances.size();++i) {
        TPoint from=zone->m_entrances[i];
        // Every positive scripted path terminates at the last flood root.
        // Derive eligibility without inspecting the candidate's predecessor walk.
        bool atRoot=from.m_x==owner.m_map.m_seed.m_x && from.m_y==owner.m_map.m_seed.m_y && level==owner.m_map.m_seed.m_z;
        if(atRoot || owner.m_map.m_mode>=2) continue;
        TPoint target(owner.m_map.m_seed.m_x,owner.m_map.m_seed.m_y);
        owner.connectJunctionEntrance(from,target,zone);
        owner.m_map.floodConnectionCosts(TRmgMapPosition(from.m_x,from.m_y,level),0);
    }
}
template<class Candidate> bool check() {
    int dimensions[]={1,3,4};
    for(int wi=0;wi<3;++wi) for(int hi=0;hi<3;++hi) for(int level=0;level<2;++level)
    for(int mode=0;mode<4;++mode) for(int entries=0;entries<4;++entries) for(int pattern=0;pattern<3;++pattern) {
        int width=dimensions[wi],height=dimensions[hi],count=width*height*2;
        std::vector<TRmgMapItem> a(count+8),b(count+8);
        for(int i=0;i<count+8;++i) {
            TRmgMapItem& cell=a[i];
            cell.m_previousTile=TRmgMapPosition(17,18,19);
            unsigned bits=0xa583619dU+unsigned(i)*1234567U;
            std::memcpy(&cell.m_movement,&bits,4);std::memcpy(&cell.m_zoneState,&bits,4);
            std::memcpy(&cell.m_tile,&bits,4);std::memcpy(&cell.m_tileData,&bits,4);std::memcpy(&cell.m_connection,&bits,4);
            cell.m_zoneState.m_zone=(i+pattern)%3==0 ? -1 : pattern;
            cell.m_tile.m_landType=(i+pattern)%5==0 ? 8 : 3;
            cell.m_tileData.m_subterraneanGate=1;cell.m_tileData.m_borderObject=0;
            cell.m_connection.m_present=(i+pattern)%4==0;
            if((i+pattern)%3==0) cell.m_objects.push_back(0);
        }
        b=a;
        TRmgTownSlot slot;slot.m_zoneIndex=pattern;
        TRmgZone za;za.m_slot=&slot;za.m_levelPosition=TRmgMapPosition(7,9,level);
        za.m_bounds.m_minimumX=pattern==1 ? width : 0;
        za.m_bounds.m_maximumX=width;za.m_bounds.m_minimumY=0;za.m_bounds.m_maximumY=height;
        if(entries) za.m_entrances.push_back(TPoint(0,0));
        if(entries>1) za.m_entrances.push_back(TPoint(width-1,height-1));
        if(entries>2) { za.m_entrances.push_back(TPoint(width-1,0));za.m_entrances.push_back(TPoint(0,0)); }
        TRmgZone zb=za;
        Candidate actual;GeneratorFixture expected;
        actual.m_connectCount=expected.m_connectCount=0;
        actual.m_map.m_mapWidth=expected.m_map.m_mapWidth=width;actual.m_map.m_mapHeight=expected.m_map.m_mapHeight=height;
        actual.m_map.m_mapItems=&a[4];expected.m_map.m_mapItems=&b[4];
        actual.m_map.m_trace=&actual.m_trace;expected.m_map.m_trace=&expected.m_trace;
        actual.m_map.m_mode=expected.m_map.m_mode=mode;
        actual.prepareJunctionZone(&za);reference(expected,&zb);
        if(actual.m_trace!=expected.m_trace || za.m_levelPosition.m_z!=zb.m_levelPosition.m_z) return false;
        std::vector<int> av,bv;
        for(int i=0;i<count+8;++i) { appendCell(av,a[i]);appendCell(bv,b[i]); }
        if(av!=bv) return false;
    }
    return true;
}
'''
        for index, (body, _) in enumerate(forms):
            text += "namespace N%d { struct type_random_map_generator : GeneratorFixture { void prepareJunctionZone(TRmgZone*); };\n" % index
            text += body + "\n}\n"
        text += "int main() {\n"
        for index, (_, name) in enumerate(forms):
            text += 'if(%scheck<N%d::type_random_map_generator>()) {std::puts("failed %s");return 1;}\n' % (
                "!" if index < positives else "", index, name)
        text += 'std::puts("%d junction forms: 864 scenarios each; eleven negative controls rejected");return 0;}\n' % positives
        with tempfile.TemporaryDirectory(prefix="rmg-junction-oracle-") as folder:
            cpp, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            cpp.write_text(text)
            subprocess.run(["g++", "-std=c++11", "-O2", "-fno-elide-constructors", str(cpp), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
