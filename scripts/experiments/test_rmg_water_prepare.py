"""Water preparation caller contract, with actual value and packed types.

Opaque floods and island creation use deterministic stateful scripts. The
independent reference enumerates flat cells and filters rectangle membership;
it does not copy the candidate's nested induction or clamp expressions.
Reduced owners are semantic fixtures, not retail ABI/layout evidence.
"""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class WaterPrepareTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native g++")
    def test_all_source_forms(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        support = (root / "src/rmg_support.cpp").read_text()
        module = generator("generate-rmg-water-prepare-family.py")
        definition = generator("generate-rmg-position-family.py").definition

        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]

        types = "\n".join(block(name) for name in (
            "TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds",
            "TRmgMovementCost", "TRmgZoneCellState", "TRmgGroundTileData"))
        types += "\n" + definition(support, "TRmgMapPosition::TRmgMapPosition")
        types += """
struct TRmgMapItem {
    TRmgMovementCost m_movement;
    TRmgZoneCellState m_zoneState;
    TRmgGroundTileData m_tileData;
};
struct type_random_map {
    int m_mapWidth, m_mapHeight;
    TRmgMapItem* m_mapItems;
""" + definition(header, "getMapItem", parameters="int x, int y, int z") + """
    TRmgMapItem* getMapItem(TRmgMapPosition point);
};
struct TRmgTownSlot { int m_zoneIndex; };
struct TRmgZone {
    TRmgTownSlot* m_slot;
    int m_terrain;
    TRmgMapPosition m_levelPosition;
    TRmgZoneBounds m_bounds;
    TRmgMapPosition getLevelPosition() const;
};
enum { eTerrainWater = 8 };
template<class T> const T& min(const T& a,const T& b) {return a<b?a:b;}
template<class T> const T& max(const T& a,const T& b) {return a>b?a:b;}
int g_randomMode, g_randomCount;
std::vector<int> g_randomTrace;
int rand() {
    if(g_randomCount > 2000) throw 1;
    int result=(g_randomCount++ * 13 + g_randomMode * 19) % 97;
    g_randomTrace.push_back(result);
    return result;
}
struct type_random_map_generator {
    type_random_map m_map;
    TRmgZone* m_sourceZone;
    std::vector<int> m_trace;
    void prepareWaterZoneConnections(TRmgZone*);
    void floodWaterZoneDistances(TRmgMapPosition point,int zone) {
        m_trace.push_back(1);m_trace.push_back(point.m_x);
        m_trace.push_back(point.m_y);m_trace.push_back(point.m_z);m_trace.push_back(zone);
        // A nontrivial opaque boundary exposes accidental source-data reloads.
        m_sourceZone->m_slot->m_zoneIndex=77;
        m_sourceZone->m_bounds.m_minimumX=0;
        m_sourceZone->m_bounds.m_maximumY=0;
        m_sourceZone->m_levelPosition.m_z=1-point.m_z;
        const int costs[6]={19,20,27,36,54,72};
        for(int i=0;i<m_map.m_mapWidth*m_map.m_mapHeight*2;++i)
            if(m_map.m_mapItems[i].m_movement.m_zonePathCost>=100)
                m_map.m_mapItems[i].m_movement.m_zonePathCost=costs[i%6];
    }
    void createWaterZoneIsland(TRmgZoneBounds area,int level) {
        m_trace.push_back(2);m_trace.push_back(area.m_minimumX);m_trace.push_back(area.m_minimumY);
        m_trace.push_back(area.m_maximumX);m_trace.push_back(area.m_maximumY);m_trace.push_back(level);
        for(int i=0;i<m_map.m_mapWidth*m_map.m_mapHeight*2;++i) {
            int x=i%m_map.m_mapWidth,y=(i/m_map.m_mapWidth)%m_map.m_mapHeight,z=i/(m_map.m_mapWidth*m_map.m_mapHeight);
            if(z==level && x>=area.m_minimumX && x<area.m_maximumX && y>=area.m_minimumY && y<area.m_maximumY)
                m_map.m_mapItems[i].m_movement.m_zonePathCost=0;
        }
    }
};
"""
        types += definition(source, "type_random_map::getMapItem", parameters="TRmgMapPosition point")
        types += definition(source, "TRmgZone::getLevelPosition")
        original = module.baseline_definition(source)
        forms = [body for _, body in module.variants(original)]
        self.assertEqual(len(forms), 60)
        self.assertEqual(len(set(forms)), 60)
        current = definition(source, module.NAME)
        if current not in forms:
            forms.append(current)
        self.assertIn(current, forms)
        selected = os.environ.get("HOMM3_WATER_PREPARE_MANIFEST")
        if selected:
            for option in json.loads(Path(selected).read_text())["axes"][0]["options"]:
                if option["replace"] not in forms:
                    forms.append(option["replace"])
        positive_count = len(forms)
        negatives = [
            ("zone->m_terrain != eTerrainWater", "zone->m_terrain == eTerrainWater"),
            ("m_zonePathCost = 32000", "m_zonePathCost = 0"),
            ("m_connectionDirection = 0", "m_connectionDirection = 1"),
            ("m_connectionEligibility = 0", "m_connectionEligibility = 1"),
            ("m_zone != zoneIndex", "m_zone == zoneIndex"),
            ("m_zonePathCost >= 20", "m_zonePathCost >= 19"),
            ("rand() % candidates.size()", "0 * rand() % candidates.size()"),
            ("int radius = rand() % range + 3", "int radius = rand() % range + 2"),
            ("createWaterZoneIsland(island, position.m_z)", "createWaterZoneIsland(island, 0)"),
            ("int zoneIndex = zone->m_slot->m_zoneIndex", "const int& zoneIndex = zone->m_slot->m_zoneIndex"),
            ("bounds.m_maximumX, m_map.m_mapWidth - 4", "bounds.m_maximumX, m_map.m_mapWidth - 3"),
            ("bounds.m_maximumY, m_map.m_mapHeight - 4", "bounds.m_maximumY, m_map.m_mapHeight - 3"),
        ]
        for old, new in negatives:
            self.assertIn(old, original)
            forms.append(original.replace(old, new))
        oracle = r"""
void reference(type_random_map_generator& owner,TRmgZone zone) {
    if(zone.m_terrain!=8) return;
    int w=owner.m_map.m_mapWidth,h=owner.m_map.m_mapHeight,level=zone.m_levelPosition.m_z;
    int zoneIndex=zone.m_slot->m_zoneIndex;
    int left=zone.m_bounds.m_minimumX,top=zone.m_bounds.m_minimumY;
    int right=zone.m_bounds.m_maximumX,bottom=zone.m_bounds.m_maximumY;
    for(int i=0;i<w*h;++i) {
        int x=i%w,y=i/w;
        if(x<left || x>=right || y<top || y>=bottom) continue;
        TRmgMapItem& cell=owner.m_map.m_mapItems[level*w*h+i];
        cell.m_movement.m_zonePathCost=32000;
        cell.m_tileData.m_connectionDirection=0;
        cell.m_zoneState.m_connectionEligibility=0;
    }
    for(int i=0;i<w*h;++i) {
        int x=i%w,y=i/w;
        if(x<left-1 || x>=right+1 || y<top-1 || y>=bottom+1) continue;
        if(owner.m_map.m_mapItems[level*w*h+i].m_zoneState.m_zone!=zoneIndex)
            owner.floodWaterZoneDistances(TRmgMapPosition(x,y,level),zoneIndex);
    }
    for(;;) {
        std::vector<int> accepted;
        for(int i=0;i<w*h;++i) {
            int x=i%w,y=i/w;
            if(x<left || x>=right || y<top || y>=bottom || x<3 || y<3 || x>=w-4 || y>=h-4) continue;
            if(owner.m_map.m_mapItems[level*w*h+i].m_movement.m_zonePathCost>=20) accepted.push_back(i);
        }
        if(accepted.empty()) break;
        int chosen=accepted[rand()%accepted.size()];
        int x=chosen%w,y=chosen/w,cost=owner.m_map.m_mapItems[level*w*h+chosen].m_movement.m_zonePathCost;
        int radius=3+rand()%(cost/3-5);
        if(radius>6) radius=6;
        TRmgZoneBounds area={x-radius,y-radius,x+radius,y+radius};
        if(area.m_minimumX<0) area.m_minimumX=0;
        if(area.m_minimumY<0) area.m_minimumY=0;
        if(area.m_maximumX>w) area.m_maximumX=w;
        if(area.m_maximumY>h) area.m_maximumY=h;
        owner.createWaterZoneIsland(area,level);
        owner.floodWaterZoneDistances(TRmgMapPosition(x,y,level),zoneIndex);
    }
}
bool check() {
    for(int w=9;w<=15;w+=3) for(int h=9;h<=12;h+=3)
    for(int level=0;level<2;++level) for(int shape=0;shape<6;++shape)
    for(int pattern=0;pattern<4;++pattern) for(int randomMode=0;randomMode<3;++randomMode)
    for(int water=0;water<2;++water) {
        TRmgTownSlot slot={pattern%2 ? -1:2},expectedSlot=slot;
        TRmgZone zone;zone.m_slot=&slot;zone.m_terrain=water?8:3;
        zone.m_levelPosition=TRmgMapPosition(7,8,level);
        zone.m_bounds.m_minimumX=shape%2?3:0;
        zone.m_bounds.m_minimumY=shape%2?3:0;
        zone.m_bounds.m_maximumX=shape<2?w:w-4;
        zone.m_bounds.m_maximumY=shape<2?h:h-4;
        if(shape==2) zone.m_bounds.m_maximumX=zone.m_bounds.m_minimumX;
        if(shape==3) zone.m_bounds.m_maximumY=zone.m_bounds.m_minimumY;
        TRmgZone expectedZone=zone;expectedZone.m_slot=&expectedSlot;
        std::vector<TRmgMapItem> cells(w*h*2+2);
        for(unsigned i=0;i<cells.size();++i) {
            std::memset(&cells[i],0xa5,sizeof(cells[i]));
            cells[i].m_movement.m_zonePathCost=9+i%70;
            cells[i].m_zoneState.m_zone=(pattern&2) && i%3 ? slot.m_zoneIndex+1:slot.m_zoneIndex;
        }
        std::vector<TRmgMapItem> expected=cells;
        type_random_map_generator actual,model;
        actual.m_map.m_mapWidth=model.m_map.m_mapWidth=w;
        actual.m_map.m_mapHeight=model.m_map.m_mapHeight=h;
        actual.m_map.m_mapItems=&cells[1];model.m_map.m_mapItems=&expected[1];
        actual.m_sourceZone=&zone;model.m_sourceZone=&expectedZone;
        g_randomMode=randomMode;g_randomCount=0;g_randomTrace.clear();
        reference(model,expectedZone);
        std::vector<int> expectedRandom=g_randomTrace;
        g_randomCount=0;g_randomTrace.clear();
        try {actual.prepareWaterZoneConnections(&zone);} catch(...) {return false;}
        if(actual.m_trace!=model.m_trace || g_randomTrace!=expectedRandom) return false;
        if(slot.m_zoneIndex!=expectedSlot.m_zoneIndex) return false;
        if(std::memcmp(&zone.m_bounds,&expectedZone.m_bounds,sizeof(zone.m_bounds))) return false;
        if(std::memcmp(&zone.m_levelPosition,&expectedZone.m_levelPosition,sizeof(zone.m_levelPosition))) return false;
        for(unsigned i=0;i<cells.size();++i)
            if(std::memcmp(&cells[i],&expected[i],sizeof(cells[i]))) return false;
    }
    return true;
}
"""
        text = "#include <vector>\n#include <cstring>\n#include <cstdio>\n"
        for index, body in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + body + "\n" + oracle + "\n}\n"
        text += "int main() {\n"
        for index in range(len(forms)):
            text += f'if(Case{index}::check() != {str(index < positive_count).lower()}) {{std::printf("case {index} failed\\n");return 1;}}\n'
        text += f'std::puts("{positive_count} water-prepare forms: 1728 scenarios each; {len(negatives)} negative controls rejected");return 0;}}\n'
        with tempfile.TemporaryDirectory(prefix="homm3-water-prepare-oracle-") as directory:
            cpp = Path(directory) / "oracle.cpp"
            binary = Path(directory) / "oracle"
            cpp.write_text(text)
            compiled = subprocess.run(["g++", "-std=c++98", "-O0", "-fno-elide-constructors",
                str(cpp), "-o", str(binary)], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            print(result.stdout, end="")


if __name__ == "__main__":
    unittest.main()
