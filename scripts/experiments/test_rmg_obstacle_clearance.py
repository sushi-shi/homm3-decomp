"""Clearance semantics using real packed/value types and connection lookup.

The reference filters flat cells by Chebyshev distance, independently of the
three candidate clamp/scan phases. Reduced owner layouts are host fixtures,
not evidence for the retail ABI. No opaque callbacks occur before progress.
"""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class ClearanceTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native g++")
    def test_all_source_forms(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        support = (root / "src/rmg_support.cpp").read_text()
        module = generator("generate-rmg-obstacle-clearance-family.py")
        definition = generator("generate-rmg-position-family.py").definition

        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]

        types = "struct TRmgTownSlot;\n" + "\n".join(block(name) for name in (
            "TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds",
            "TRmgZoneConnection", "TRmgZoneCellState", "TRmgGroundTile",
            "TRmgGroundTileData", "TRmgConnectionDecoration"))
        types += "\n" + definition(support, "TRmgMapPosition::TRmgMapPosition")
        types += """
struct TRmgMapItem {
    TRmgZoneCellState m_zoneState;
    TRmgGroundTile m_tile;
    TRmgGroundTileData m_tileData;
    TRmgConnectionDecoration m_connection;
    std::vector<void*> m_objects;
};
struct type_random_map {
    int m_mapWidth, m_mapHeight, m_numberLevels;
    TRmgMapItem* m_mapItems;
""" + definition(header, "getMapItem", parameters="int x, int y, int z") + """
    TRmgMapItem* getMapItem(TRmgMapPosition point);
};
struct TRmgTownSlot {
    int m_zoneIndex;
    std::vector<TRmgZoneConnection> m_connections;
    TRmgZoneConnection* findConnection(int destinationZone);
};
struct TRmgZone { TRmgTownSlot* m_slot; };
struct Progress {
    std::vector<int> m_trace;
    void advance(int amount) {m_trace.push_back(amount);}
};
enum { eTerrainWater = 8 };
template<class T> const T& min(const T& a,const T& b) {return a<b?a:b;}
template<class T> const T& max(const T& a,const T& b) {return a>b?a:b;}
struct type_random_map_generator {
    type_random_map m_map;
    std::vector<TRmgZone*> m_zones;
    Progress* m_progress;
    void expandObstacleClearance();
};
"""
        types += definition(source, "type_random_map::getMapItem", parameters="TRmgMapPosition point")
        types += definition(source, "TRmgTownSlot::findConnection")
        original = module.baseline_definition(source)
        forms = [body for _, body in module.variants(original)]
        self.assertEqual(len(set(forms)), 64)
        forms += [body for _, body in module.upper_variants(original) if body not in forms]
        self.assertEqual(len(set(forms)), 127)
        self.assertIn(original, forms)
        self.assertIn(definition(source, module.NAME), forms)
        selected = os.environ.get("HOMM3_CLEARANCE_MANIFEST")
        if selected:
            for option in json.loads(Path(selected).read_text())["axes"][0]["options"]:
                if option["replace"] not in forms:
                    forms.append(option["replace"])
        positive_count = len(forms)
        negatives = [
            ("if (otherZone < 0)", "if (otherZone == -1)"),
            ("current->m_tile.m_landType == eTerrainWater", "current->m_tile.m_landType != eTerrainWater"),
            ("otherZone != zoneIndex", "otherZone == zoneIndex"),
            ("position.m_z == 1", "position.m_z >= 1"),
            ("connection && !connection->m_unguarded", "connection && connection->m_unguarded"),
            ("if (!connection || position.m_z == 1)", "if (position.m_z == 1)"),
            ("if (!current->m_connection.m_present)", "if (current->m_connection.m_present)"),
            ("static_cast<int>(item->m_objects.size()) <= 0", "static_cast<int>(item->m_objects.size()) > 0"),
            ("!item->m_connection.m_present", "item->m_connection.m_present"),
            ("position.m_x + 2", "position.m_x + 1"),
            ("position.m_y - 1", "position.m_y"),
            ("m_progress->advance(1600)", "m_progress->advance(1601)"),
        ]
        # The changed -2 arm calls a valid linear lookup with a negative ID;
        # no vector is indexed by that neighbor ID.
        for old, new in negatives:
            self.assertIn(old, original)
            forms.append(original.replace(old, new))
        oracle = r"""
void reference(type_random_map_generator& owner,int policy[3][3]) {
    int w=owner.m_map.m_mapWidth,h=owner.m_map.m_mapHeight;
    int count=w*h*owner.m_map.m_numberLevels;
    for(int i=0;i<count;++i) {
        TRmgMapItem& current=owner.m_map.m_mapItems[i];
        int zone=current.m_zoneState.m_zone;
        if(zone<0 || current.m_tile.m_landType==8) continue;
        int x=i%w,y=(i/w)%h,z=i/(w*h);
        bool admitted=false;
        for(int j=0;j<count;++j) {
            int dx=j%w-x,dy=(j/w)%h-y;
            if(j/(w*h)!=z || dx < -1 || dx > 1 || dy < -1 || dy > 1) continue;
            TRmgMapItem& other=owner.m_map.m_mapItems[j];
            int destination=other.m_zoneState.m_zone;
            if(destination<0) admitted|=other.m_tile.m_landType==8;
            else if(destination!=zone)
                admitted|=z==1 || policy[zone][destination]!=2;
        }
        if(!admitted) continue;
        if(!current.m_connection.m_present) {
            current.m_tileData.m_subterraneanGate=0;
            current.m_tileData.m_borderObject=1;
        }
        for(int j=0;j<count;++j) {
            int dx=j%w-x,dy=(j/w)%h-y;
            if(j/(w*h)!=z || dx < -1 || dx > 1 || dy < -1 || dy > 1) continue;
            TRmgMapItem& other=owner.m_map.m_mapItems[j];
            if(other.m_objects.empty() && !other.m_connection.m_present)
                other.m_tileData.m_subterraneanGate=0;
        }
    }
    if(owner.m_progress) owner.m_progress->advance(1600);
}
bool check() {
    const int dimensions[][2]={{0,3},{3,0},{1,1},{1,4},{4,1},{3,4},{4,3}};
    for(int shape=0;shape<7;++shape) for(int levels=0;levels<=3;++levels)
    for(int pattern=0;pattern<16;++pattern) for(int mode=0;mode<6;++mode)
    for(int progress=0;progress<2;++progress) {
        int w=dimensions[shape][0],h=dimensions[shape][1],count=w*h*levels;
        TRmgTownSlot slots[3];TRmgZone zones[3];int policy[3][3];
        for(int a=0;a<3;++a) {slots[a].m_zoneIndex=a;zones[a].m_slot=&slots[a];}
        for(int a=0;a<3;++a) for(int b=0;b<3;++b) {
            policy[a][b]=mode<3?mode:(a*2+b+mode)%3;
            if(policy[a][b]) {
                TRmgZoneConnection link={};link.m_destination=&slots[b];
                link.m_unguarded=policy[a][b]==2;
                slots[a].m_connections.push_back(link);
            }
        }
        std::vector<TRmgMapItem> cells(count+2);
        for(unsigned i=0;i<cells.size();++i) {
            std::memset(&cells[i].m_zoneState,0xa5,sizeof(cells[i].m_zoneState));
            std::memset(&cells[i].m_tile,0xa5,sizeof(cells[i].m_tile));
            std::memset(&cells[i].m_tileData,0xa5,sizeof(cells[i].m_tileData));
            std::memset(&cells[i].m_connection,0xa5,sizeof(cells[i].m_connection));
            cells[i].m_zoneState.m_zone=(int)((i*7+pattern)%5)-2;
            cells[i].m_tile.m_landType=(i+pattern)%3==0?8:3;
            cells[i].m_tileData.m_subterraneanGate=1;
            cells[i].m_tileData.m_borderObject=0;
            cells[i].m_connection.m_present=(i+pattern)%4==0;
            if((i*3+pattern)%4==0) cells[i].m_objects.push_back(&slots[0]);
            if(pattern==0) cells[i].m_tile.m_landType=8;
            if(pattern==1) cells[i].m_zoneState.m_zone=-2;
            if(pattern==2) cells[i].m_zoneState.m_zone=0;
        }
        std::vector<TRmgMapItem> expected=cells;
        Progress trace,expectedTrace;type_random_map_generator actual,model;
        actual.m_map.m_mapWidth=model.m_map.m_mapWidth=w;
        actual.m_map.m_mapHeight=model.m_map.m_mapHeight=h;
        actual.m_map.m_numberLevels=model.m_map.m_numberLevels=levels;
        actual.m_map.m_mapItems=&cells[1];model.m_map.m_mapItems=&expected[1];
        actual.m_progress=progress?&trace:0;model.m_progress=progress?&expectedTrace:0;
        for(int a=0;a<3;++a) actual.m_zones.push_back(&zones[a]);
        reference(model,policy);actual.expandObstacleClearance();
        if(trace.m_trace!=expectedTrace.m_trace) return false;
        for(unsigned i=0;i<cells.size();++i) {
            if(std::memcmp(&cells[i].m_zoneState,&expected[i].m_zoneState,sizeof(cells[i].m_zoneState))) return false;
            if(std::memcmp(&cells[i].m_tile,&expected[i].m_tile,sizeof(cells[i].m_tile))) return false;
            if(std::memcmp(&cells[i].m_tileData,&expected[i].m_tileData,sizeof(cells[i].m_tileData))) return false;
            if(std::memcmp(&cells[i].m_connection,&expected[i].m_connection,sizeof(cells[i].m_connection))) return false;
            if(cells[i].m_objects!=expected[i].m_objects) return false;
        }
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
        text += f'std::puts("{positive_count} clearance forms: 5376 scenarios each; {len(negatives)} negative controls rejected");return 0;}}\n'
        with tempfile.TemporaryDirectory(prefix="homm3-clearance-oracle-") as directory:
            cpp = Path(directory) / "oracle.cpp"
            binary = Path(directory) / "oracle"
            cpp.write_text(text)
            compiled = subprocess.run(["g++", "-std=c++98", "-O0", str(cpp), "-o", str(binary)], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            print(result.stdout, end="")


if __name__ == "__main__":
    unittest.main()
