"""Native water-flood oracle; actual packed fields and coordinate helpers.

The owner/item layouts are reduced fixtures, not x86 ABI evidence. Linear
minimum selection supplies FIFO ties independently of the binary worklist.
"""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class WaterQueueTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native g++")
    def test_all_source_forms(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        support = (root / "src/rmg_support.cpp").read_text()
        module = generator("generate-rmg-water-queue-family.py")
        definition = generator("generate-rmg-position-family.py").definition

        def block(prefix):
            start = header.index(prefix + " {")
            return header[start:header.index("\n};", start) + 3]

        types = "\n".join(block("struct " + name) for name in (
            "TRmgVector", "TPoint", "TRmgMapPosition", "TRmgMovementCost",
            "TRmgZoneCellState", "TRmgGroundTileData"))
        types += "\n" + definition(support, "TRmgMapPosition::TRmgMapPosition")
        types += "\n" + definition(source, "TRmgMapPosition::operator+=")
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
struct type_random_map_generator {
    type_random_map m_map;
    void floodWaterZoneDistances(TRmgMapPosition, int);
};
"""
        types += definition(source, "type_random_map::getMapItem", parameters="TRmgMapPosition point")
        start = source.index("TPoint g_rmgDirections[RMG_DIRECTION_COUNT] = {")
        types += "\nenum { RMG_DIRECTION_COUNT = 8 };\n" + source[start:source.index("\n};", start) + 3]
        types += "\n" + definition(source, "insertRmgWorkItem", parameters=
            "std::vector<TRmgMapPosition>& positions, std::vector<int>& costs, TRmgMapPosition position, int cost")
        original = module.baseline_definition(source)
        forms = [body for _, body in module.variants(original)]
        self.assertEqual(len(forms), 48)
        self.assertEqual(len(set(forms)), 48)
        current = definition(source, module.NAME)
        if current not in forms:
            forms.append(current)
        selected = os.environ.get("HOMM3_WATER_QUEUE_MANIFEST")
        if selected:
            for option in json.loads(Path(selected).read_text())["axes"][0]["options"]:
                if option["replace"] not in forms:
                    forms.append(option["replace"])
        positive_count = len(forms)
        negative = [
            ("? 3 : 2", "? 4 : 2"),
            ("? 3 : 2", "? 2 : 3"),
            ("next.m_z = position.m_z", "next.m_z = 0"),
            ("item->m_zoneState.m_zone != zoneIndex", "item->m_zoneState.m_zone == zoneIndex"),
            ("m_connectionDirection = direction", "m_connectionDirection = direction + 1"),
            ("seed->m_movement.m_zonePathCost = 0", "seed->m_movement.m_zonePathCost = 1"),
        ]
        for old, new in negative:
            self.assertIn(old, original)
            forms.append(original.replace(old, new))
        text = "#include <vector>\n#include <cstring>\n#include <cstdio>\n"
        oracle = r"""
struct Pending { int m_cell, m_cost, m_serial; };
void reference(std::vector<TRmgMapItem>& cells,int w,int h,int seed,int zone) {
    const int dx[8]={1,1,0,-1,-1,-1,0,1},dy[8]={0,1,1,1,0,-1,-1,-1};
    std::vector<Pending> pending;
    int serial=0;
    Pending first={seed,0,serial++};pending.push_back(first);
    cells[seed].m_movement.m_zonePathCost=0;
    cells[seed].m_tileData.m_connectionDirection=0;
    cells[seed].m_zoneState.m_connectionEligibility=0;
    while(!pending.empty()) {
        unsigned best=0;
        for(unsigned i=1;i<pending.size();++i)
            if(pending[i].m_cost<pending[best].m_cost ||
                (pending[i].m_cost==pending[best].m_cost && pending[i].m_serial<pending[best].m_serial)) best=i;
        int cell=pending[best].m_cell;pending.erase(pending.begin()+best);
        int x=cell%w,y=(cell/w)%h,z=cell/(w*h);
        int cost=cells[cell].m_movement.m_zonePathCost;
        for(int dir=0;dir<8;++dir) {
            int nx=x+dx[dir],ny=y+dy[dir];
            if(nx<0 || nx>=w || ny<0 || ny>=h) continue;
            int index=(z*h+ny)*w+nx;
            if(cells[index].m_zoneState.m_zone!=zone) continue;
            int proposed=cost+2+(dx[dir]!=0 && dy[dir]!=0);
            if(proposed>=static_cast<int>(cells[index].m_movement.m_zonePathCost)) continue;
            cells[index].m_movement.m_zonePathCost=proposed;
            cells[index].m_tileData.m_connectionDirection=dir;
            cells[index].m_zoneState.m_connectionEligibility=0;
            Pending next={index,proposed,serial++};pending.push_back(next);
        }
    }
}
bool check() {
    for(int w=1;w<=5;++w) for(int h=1;h<=4;++h)
    for(int level=0;level<2;++level) for(int pattern=0;pattern<32;++pattern)
    for(int end=0;end<2;++end) {
        std::vector<TRmgMapItem> cells(w*h*2);
        int zone=pattern%3-1;
        for(unsigned i=0;i<cells.size();++i) {
            std::memset(&cells[i],0,sizeof(cells[i]));
            cells[i].m_movement.m_cost=(i*37+pattern)%32000;
            cells[i].m_movement.m_zonePathCost=(pattern&16) && i%5==2 ? 0:32000;
            cells[i].m_zoneState.m_zone=(pattern&(1<<(i%4))) ? zone+1:zone;
            cells[i].m_zoneState.m_connectionEligibility=7;
            std::memset(&cells[i].m_tileData,0xa5,sizeof(cells[i].m_tileData));
        }
        int seed=level*w*h+(end ? w*h-1:0);
        std::vector<TRmgMapItem> expected=cells;
        reference(expected,w,h,seed,zone);
        type_random_map_generator owner;
        owner.m_map.m_mapWidth=w;owner.m_map.m_mapHeight=h;owner.m_map.m_mapItems=&cells[0];
        TRmgMapPosition position(seed%w,(seed/w)%h,level);
        owner.floodWaterZoneDistances(position,zone);
        for(unsigned i=0;i<cells.size();++i)
            if(std::memcmp(&cells[i],&expected[i],sizeof(cells[i]))) return false;
        if(position.m_x!=seed%w || position.m_y!=(seed/w)%h || position.m_z!=level) return false;
    }
    return true;
}
"""
        for index, body in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + body + "\n" + oracle + "\n}\n"
        text += "int main() {\n"
        for index in range(len(forms)):
            text += f'if(Case{index}::check() != {str(index < positive_count).lower()}) {{ std::printf("case {index} failed\\n"); return 1; }}\n'
        text += f'std::puts("{positive_count} water-flood forms: 2560 maps each; six negative controls rejected"); return 0; }}\n'
        with tempfile.TemporaryDirectory(prefix="homm3-water-queue-oracle-") as directory:
            cpp = Path(directory) / "oracle.cpp"
            binary = Path(directory) / "oracle"
            cpp.write_text(text)
            subprocess.run(["g++", "-std=c++98", "-O0", "-fno-elide-constructors", str(cpp), "-o", str(binary)], check=True, capture_output=True, text=True)
            result = subprocess.run([str(binary)], check=True, capture_output=True, text=True, timeout=120)
            print(result.stdout, end="")


if __name__ == "__main__":
    unittest.main()
