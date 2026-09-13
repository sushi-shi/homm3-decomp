"""Independent stable-priority flood and shared insertion-helper contracts.

Imports actual coordinate/packed-field declarations and helper bodies. Owner
and object records are reduced host fixtures, not claims about the x86 ABI.
Successful allocations, positive dimensions and bounded costs are the domain.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class ConnectionQueueTests(unittest.TestCase):
    def test_adopted_child_is_recognized_but_new_semantics_are_not(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-connection-queue-family.py")
        source = (root / module.SOURCE).read_text()
        original = module.origin_source(source)
        bodies = {choice["replace"] for choice in module.local_options(original)}
        self.assertIn(module.definition(source, module.NAME), bodies)
        self.assertEqual(module.helper_pair(source)[0], module.helper_pair(original)[0])
        changed = source.replace("currentCost + 10", "currentCost + 9")
        with self.assertRaisesRegex(ValueError, "review changed"):
            module.origin_source(changed)

    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_stable_queue_and_connection_flood(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-connection-queue-family.py")
        source = (root / module.SOURCE).read_text()
        header = (root / "include/rmg.h").read_text()
        def block(text, prefix):
            start = text.index(prefix + " {")
            return text[start:text.index("\n};", start) + 3]
        types = "\n".join(block(header, "struct " + name) for name in (
            "TRmgVector", "TPoint", "TRmgMapPosition", "TRmgMovementCost",
            "TRmgZoneCellState", "TRmgGroundTile", "TRmgGroundTileData", "TRmgConnectionDecoration"))
        types += "\n" + block((root / "include/terrain_type.h").read_text(), "enum TTerrainType")
        support = (root / "src/rmg_support.cpp").read_text()
        types += "\n" + module.definition(support, "TRmgMapPosition::TRmgMapPosition")
        types += "\n" + module.definition(source, "TRmgMapPosition::operator+=")
        item = block(header, "struct TRmgMapItem")
        fields = "\n".join(line.split("//")[0] for line in item.splitlines()
                           if any(name + ";" in line for name in (
                               "m_objects", "m_previousTile", "m_movement", "m_zoneState",
                               "m_tile", "m_tileData", "m_connection")))
        types += """
struct TObjectType { int m_objectType; };
struct TRmgObjectPropertiesRef { TObjectType* m_prototype; };
struct type_object { TRmgObjectPropertiesRef* m_properties; };
struct TRmgMapItem {
""" + fields
        for name in ("isRoadEntrance", "hasSubterraneanGate", "setMovementCost"):
            types += "\n" + module.definition(header, name)
        types += "\n};\nstruct type_random_map {\n"
        types += "int m_mapWidth, m_mapHeight; TRmgMapItem* m_mapItems;\n"
        types += module.definition(header, "getMapItem", parameters="int x, int y, int z")
        types += "\nTRmgMapItem* getMapItem(TRmgMapPosition point);\nvoid floodConnectionCosts(TRmgMapPosition, unsigned char);\n};\n"
        types += module.definition(source, "type_random_map::getMapItem", parameters="TRmgMapPosition point")
        start = source.index("TPoint g_rmgDirections[RMG_DIRECTION_COUNT] = {")
        types += "\nenum { RMG_DIRECTION_COUNT = 8 };\n" + source[start:source.index("\n};", start) + 3]
        types += "\nunsigned char g_adventureObjectLandBlocked[8][16];\n"
        choices = list(module.options(module.origin_source(source)))
        self.assertEqual(len(choices), 49)
        self.assertEqual(len({option["replace"] for option in choices}), 49)
        old_helper, new_helper = module.helper_pair(source)
        forms = [(choice["replace"], old_helper if i == 0 else new_helper)
                 for i, choice in enumerate(choices)]
        current = (module.definition(source, module.NAME), old_helper)
        if current not in forms:
            forms.append(current)
        selected = os.environ.get("HOMM3_CONNECTION_QUEUE_MANIFEST")
        selected_paths = os.environ.get("HOMM3_CONNECTION_QUEUE_MANIFESTS", "").split(os.pathsep)
        if selected:
            selected_paths.append(selected)
        for path in filter(None, selected_paths):
            manifest = json.loads(Path(path).read_text())
            for choice in manifest["axes"][0]["options"]:
                helpers = old_helper if not choice.get("extra_edits") else choice["extra_edits"][0]["replace"]
                form = (choice["replace"], helpers)
                if form not in forms:
                    forms.append(form)
        positive_count = len(forms)
        seed = choices[1]["replace"]
        # Each invalid control must be caught by the same independent oracle.
        for old, new in (
            ("currentCost + 10", "currentCost + 9"),
            ("next->m_movement.m_zonePathCost <= nextCost", "next->m_movement.m_zonePathCost < nextCost"),
            ("traits[0] && !traits[2]", "traits[0] && traits[2]"),
            ("direction > 0 && direction < 4", "direction >= 0 && direction < 4"),
            ("nextPosition.m_z = currentPosition.m_z", "nextPosition.m_z = 0"),
            ("next->m_tile.m_landType != eTerrainWater || waterZone", "next->m_tile.m_landType != eTerrainWater && waterZone"),
            ("next->setMovementCost(nextCost, currentPosition)", "next->setMovementCost(nextCost, nextPosition)"),
            ("next->m_zoneState.m_zone < 0", "next->m_zoneState.m_zone > 0"),
        ):
            self.assertIn(old, seed)
            forms.append((seed.replace(old, new), new_helper))
        forms.append((seed, new_helper.replace("cost < costs[middle]", "cost <= costs[middle]")))
        text = "#include <vector>\n#include <algorithm>\n#include <cstring>\n#include <cstdio>\n"
        for index, (body, helpers) in enumerate(forms):
            # Dinkumware's vector iterator is the actual element pointer.
            # libstdc++ wraps that same pointer; unwrap it only at the native
            # fixture boundary, including an empty vector's valid begin().
            # No matching source or search-loop expression is changed.
            if "int* costs" in helpers:
                body = body.replace("findRmgWorkItemInsertion(costs.begin(),", "findRmgWorkItemInsertion(costs.begin().base(),")
                helpers = helpers.replace("findRmgWorkItemInsertion(costs.begin(),", "findRmgWorkItemInsertion(costs.begin().base(),")
            range_check = "bool rangeCheck() { return true; }\n"
            if "int first, int last" in helpers:
                argument = "costs.begin().base()" if "int* costs" in helpers else "costs"
                range_check = r"""
bool rangeCheck() {
    for(int count=0;count<25;++count) {
        std::vector<int> costs;
        for(int i=0;i<count;++i) costs.push_back(9-i/2);
        for(int first=0;first<=count;++first) for(int last=first;last<=count;++last)
        for(int cost=-3;cost<13;++cost) {
            int expected=first;
            while(expected<last && costs[expected]>cost) ++expected;
            if(findRmgWorkItemInsertion(COST_ARRAY,first,last,cost)!=expected) return false;
        }
    }
    return true;
}
""".replace("COST_ARRAY", argument)
            text += f"namespace Case{index} {{\n" + types + "\n" + helpers + "\n" + body + "\n" + range_check + r"""
bool equalPosition(const TRmgMapPosition& a,const TRmgMapPosition& b) {
    return a.m_x==b.m_x && a.m_y==b.m_y && a.m_z==b.m_z;
}
bool same(const TRmgMapItem& a,const TRmgMapItem& b) {
    return a.m_objects==b.m_objects && equalPosition(a.m_previousTile,b.m_previousTile)
        && !std::memcmp(&a.m_movement,&b.m_movement,sizeof(a.m_movement))
        && !std::memcmp(&a.m_zoneState,&b.m_zoneState,sizeof(a.m_zoneState))
        && !std::memcmp(&a.m_tile,&b.m_tile,sizeof(a.m_tile))
        && !std::memcmp(&a.m_tileData,&b.m_tileData,sizeof(a.m_tileData))
        && !std::memcmp(&a.m_connection,&b.m_connection,sizeof(a.m_connection));
}
// Linear minimum selection gives FIFO ties independently of binary insertion.
struct Pending { int m_cell, m_cost, m_arrival; };
void reference(std::vector<TRmgMapItem>& cells,int width,int height,int seed,unsigned char waterZone) {
    const int dx[8]={1,1,0,-1,-1,-1,0,1},dy[8]={0,1,1,1,0,-1,-1,-1};
    int zone=cells[seed].m_zoneState.m_zone,serial=0;
    std::vector<Pending> queue;
    Pending first={seed,0,serial++}; queue.push_back(first);
    cells[seed].m_movement.m_cost=0;
    cells[seed].m_previousTile.m_x=cells[seed].m_previousTile.m_y=cells[seed].m_previousTile.m_z=-1;
    while(!queue.empty()) {
        unsigned selected=0;
        for(unsigned i=1;i<queue.size();++i)
            if(queue[i].m_cost<queue[selected].m_cost ||
               (queue[i].m_cost==queue[selected].m_cost && queue[i].m_arrival<queue[selected].m_arrival)) selected=i;
        int cell=queue[selected].m_cell;queue.erase(queue.begin()+selected);
        int level=cell/(width*height),x=cell%width,y=(cell/width)%height;
        TRmgMapItem& current=cells[cell];
        int currentZone=current.m_zoneState.m_zone;
        int cost=currentZone==zone ? current.m_movement.m_cost : current.m_movement.m_zonePathCost;
        int maximum=7;
        if(current.m_tileData.m_roadEntrance && !g_adventureObjectLandBlocked[current.m_objects[0]->m_properties->m_prototype->m_objectType][1]) maximum=4;
        for(int dir=maximum;dir>=0;--dir) {
            int nx=x+dx[dir],ny=y+dy[dir];
            if(nx<0 || nx>=width || ny<0 || ny>=height) continue;
            int neighbor=(level*height+ny)*width+nx;
            TRmgMapItem& next=cells[neighbor];
            int nextZone=next.m_zoneState.m_zone;
            if(nextZone<0 || next.m_tileData.m_roadPassable==0 || next.m_tile.m_landType==eTerrainRock) continue;
            if(next.m_tileData.m_roadEntrance) {
                const unsigned char* traits=g_adventureObjectLandBlocked[next.m_objects[0]->m_properties->m_prototype->m_objectType];
                bool veto=(traits[0]!=0 && traits[2]==0);
                bool diagonal=(dir==1 || dir==2 || dir==3);
                if(veto || (traits[1]==0 && diagonal)) continue;
            }
            int proposed=cost+(nextZone==zone && next.m_tile.m_landType!=eTerrainWater ? 1 : 10);
            if(nextZone==zone) {
                if(currentZone!=zone || proposed>=static_cast<int>(next.m_movement.m_cost)) continue;
                if(cost==0 && next.m_tileData.m_subterraneanGate &&
                   !(next.m_tile.m_landType==eTerrainWater && waterZone==0)) proposed=0;
                next.m_movement.m_cost=proposed;
                next.m_previousTile.m_x=x;next.m_previousTile.m_y=y;next.m_previousTile.m_z=level;
            } else {
                if((currentZone!=zone && currentZone!=nextZone) || proposed>=static_cast<int>(next.m_movement.m_zonePathCost)) continue;
                next.m_movement.m_zonePathCost=proposed;
                next.m_tileData.m_connectionDirection=(dir+4)%8;
                next.m_zoneState.m_connectionEligibility=zone;
            }
            Pending pending={neighbor,proposed,serial++};queue.push_back(pending);
        }
    }
}
bool queueCheck() {
    for(int pattern=0;pattern<32;++pattern) {
        std::vector<TRmgMapPosition> positions;std::vector<int> costs;
        std::vector<Pending> expected;
        for(int i=0;i<40;++i) {
            TRmgMapPosition p(i,i*2,pattern);
            int cost=(i*17+pattern*13)%9;
            insertRmgWorkItem(positions,costs,p,cost);
            Pending item={i,cost,i};expected.push_back(item);
        }
        while(!expected.empty()) {
            unsigned best=0;
            for(unsigned i=1;i<expected.size();++i)
                if(expected[i].m_cost<expected[best].m_cost ||
                   (expected[i].m_cost==expected[best].m_cost && expected[i].m_arrival<expected[best].m_arrival)) best=i;
            int identity=expected[best].m_cell;
            if(costs.back()!=expected[best].m_cost || positions.back().m_x!=identity ||
               positions.back().m_y!=2*identity || positions.back().m_z!=pattern) return false;
            costs.pop_back();positions.pop_back();expected.erase(expected.begin()+best);
        }
    }
    return true;
}
bool check() {
    if(!queueCheck() || !rangeCheck()) return false;
    TObjectType prototypes[8];TRmgObjectPropertiesRef properties[8];type_object objects[8];
    for(int i=0;i<8;++i) {
        prototypes[i].m_objectType=i;properties[i].m_prototype=&prototypes[i];objects[i].m_properties=&properties[i];
        std::memset(g_adventureObjectLandBlocked[i],0,16);
        for(int j=0;j<3;++j) g_adventureObjectLandBlocked[i][j]=(i & (1<<j)) ? 255 : 0;
    }
    const int widths[3]={1,2,5},heights[3]={1,4,3};
    for(int shape=0;shape<3;++shape) for(int level=0;level<2;++level)
    for(int pattern=0;pattern<192;++pattern) for(int flag=0;flag<3;++flag) {
        int width=widths[shape],height=heights[shape],count=2*width*height;
        std::vector<TRmgMapItem> cells(count),before;
        for(int i=0;i<count;++i) {
            TRmgMapItem& cell=cells[i];unsigned bits=(i*137+pattern*811)%1024;
            cell.m_previousTile=TRmgMapPosition(-7,-8,-9);
            std::memset(&cell.m_movement,0,sizeof(cell.m_movement));
            std::memset(&cell.m_zoneState,0,sizeof(cell.m_zoneState));
            std::memset(&cell.m_tile,0,sizeof(cell.m_tile));
            std::memset(&cell.m_tileData,0,sizeof(cell.m_tileData));
            std::memset(&cell.m_connection,0,sizeof(cell.m_connection));
            cell.m_movement.m_cost=pattern%12==0 ? (i%7) : 65535;
            cell.m_movement.m_zonePathCost=pattern%13==0 ? (i%23) : 65535;
            cell.m_zoneState.m_score=123+i;
            cell.m_zoneState.m_zone=pattern%6==0 ? 4 : bits%7==0 ? -1 : 4+bits%3;
            cell.m_zoneState.m_connectionEligibility=-37;
            cell.m_tile.m_landType=pattern%5==0 ? eTerrainDirt : bits%7==0 ? eTerrainRock : bits%3==0 ? eTerrainWater : eTerrainDirt;
            cell.m_tile.m_terrainFrame=51;
            cell.m_tileData.m_roadPassable=pattern%4==0 || (bits & 8)!=0;
            cell.m_tileData.m_roadEntrance=(bits & 32)!=0;
            cell.m_tileData.m_subterraneanGate=(bits & 64)!=0;
            cell.m_tileData.m_connectionDirection=7;
            cell.m_tileData.m_borderObject=1;
            cell.m_connection.m_direction=11;
            cell.m_objects.push_back(&objects[(i+pattern)%8]);
        }
        int seed=(level*height+pattern%height)*width+pattern%width;
        cells[seed].m_zoneState.m_zone=4;
        before=cells;
        std::vector<TRmgMapItem> expected=cells;
        unsigned char waterZone=flag==0 ? 0 : flag==1 ? 1 : 255;
        reference(expected,width,height,seed,waterZone);
        type_random_map map;map.m_mapWidth=width;map.m_mapHeight=height;map.m_mapItems=&cells[0];
        TRmgMapPosition position(pattern%width,pattern%height,level);
        map.floodConnectionCosts(position,waterZone);
        for(int i=0;i<count;++i) if(!same(cells[i],expected[i])) return false;
        if(map.m_mapWidth!=width || map.m_mapHeight!=height || map.m_mapItems!=&cells[0]) return false;
    }
    return true;
}
}
"""
        text += "int main() {\n"
        for index in range(len(forms)):
            expected = "true" if index < positive_count else "false"
            text += f'if(Case{index}::check()!={expected}) {{ std::printf("failed case {index}\\n");return 1; }}\n'
        text += "return 0; }\n"
        with tempfile.TemporaryDirectory(prefix="homm3-connection-queue-oracle-") as raw:
            cpp, exe = Path(raw) / "oracle.cpp", Path(raw) / "oracle"
            cpp.write_text(text)
            built = subprocess.run(["g++", "-std=c++98", "-O2", "-fno-elide-constructors", str(cpp), "-o", str(exe)], capture_output=True, text=True)
            self.assertEqual(built.returncode, 0, built.stderr)
            run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=60)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        print(f"{positive_count} source/helper states: 3456 maps and 32 stable queues per state; nine negative controls rejected")


if __name__ == "__main__":
    unittest.main()
