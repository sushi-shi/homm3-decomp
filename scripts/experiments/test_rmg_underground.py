"""Underground coordinator oracle with opaque brush/progress boundaries.

Actual coordinate/packed types, queries, getter and borrowed constructor are
imported. Flat-cell enumeration independently derives paints and preserved
data. Host owners test semantics and RAII order, not the x86 layout.
"""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class UndergroundTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native g++")
    def test_all_source_forms(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        support = (root / "src/rmg_support.cpp").read_text()
        module = generator("generate-rmg-underground-family.py")
        definition = generator("generate-rmg-position-family.py").definition

        def block(text, prefix):
            start = text.index(prefix + " {")
            return text[start:text.index("\n};", start) + 3]

        types = "\n".join(block(header, "struct " + name) for name in (
            "TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds",
            "TRmgZoneCellState", "TRmgGroundTile", "TRmgGroundTileData"))
        types += "\n" + block((root / "include/terrain_type.h").read_text(), "enum TTerrainType")
        types += "\n" + definition(support, "TRmgMapPosition::TRmgMapPosition")
        types += """
struct TRmgMapItem {
    TRmgZoneCellState m_zoneState;
    TRmgGroundTile m_tile;
    TRmgGroundTileData m_tileData;
    std::vector<void*> m_objects;
""" + definition(header, "isRoadEntrance") + definition(header, "hasSubterraneanGate") + r"""
};
struct Event { int m_kind,m_a,m_b,m_c; };
std::vector<Event> g_events;
int g_width,g_height,g_progressMode;
TRmgMapItem* g_cells;
bool g_valid;
void append(std::vector<Event>& events,int kind,int a=0,int b=0,int c=0) {
    Event e={kind,a,b,c};events.push_back(e);
}
struct type_random_map {
    unsigned char m_ownsMapItems;
    TRmgMapItem* m_mapItems;
    int m_mapWidth,m_mapHeight,m_numberLevels;
    type_random_map() : m_ownsMapItems(1) {}
    ~type_random_map() {if(!m_ownsMapItems) append(g_events,6);}
""" + definition(header, "type_random_map", parameters="TRmgMapItem* items, int width, int height")
        types += definition(header, "getMapItem", parameters="int x, int y, int z")
        types += r"""
    TRmgMapItem* getMapItem(TRmgMapPosition point);
};
struct TRmgZone {
    TRmgMapPosition m_levelPosition;
    TRmgZoneBounds m_bounds;
    int m_terrain;
    TRmgMapPosition getLevelPosition() const;
};
struct TRmgTerrainBrush {
    type_random_map* m_map;
    int m_terrain;
    TRmgTerrainBrush(type_random_map* map,int terrain,int strength) : m_map(map),m_terrain(terrain) {
        append(g_events,1,terrain,strength);
        if(map->m_mapWidth!=g_width || map->m_mapHeight!=g_height || map->m_numberLevels!=1 ||
           map->m_ownsMapItems || map->m_mapItems!=g_cells+g_width*g_height) g_valid=false;
    }
    ~TRmgTerrainBrush() {append(g_events,5);}
    void changeTerrain(int terrain,int strength) {
        append(g_events,3,terrain,strength);m_terrain=terrain;
    }
    void paintRectangle(unsigned x,unsigned y,unsigned w,unsigned h) {
        append(g_events,2,x,y,w*10+h);
        if(w!=1 || h!=1 || x>=static_cast<unsigned>(g_width) || y>=static_cast<unsigned>(g_height)) {
            g_valid=false;return;
        }
        if(g_valid) m_map->m_mapItems[y*g_width+x].m_tile.m_landType=m_terrain;
    }
};
struct Progress {int m_id;void advance(int amount);};
struct type_random_map_generator {
    type_random_map m_map;
    std::vector<TRmgZone*> m_zones;
    Progress* m_progress;
    void decorateUnderground();
};
type_random_map_generator* g_owner;
Progress* g_replacement;
void Progress::advance(int amount) {
    append(g_events,4,amount,m_id);
    if(m_id==0 && g_progressMode==2) g_owner->m_progress=g_replacement;
    if(m_id==0 && g_progressMode==3) g_owner->m_progress=0;
}
bool sameEvent(const Event& a,const Event& b) {
    return a.m_kind==b.m_kind && a.m_a==b.m_a && a.m_b==b.m_b && a.m_c==b.m_c;
}
"""
        types += definition(source, "type_random_map::getMapItem", parameters="TRmgMapPosition point")
        types += definition(source, "TRmgZone::getLevelPosition")
        original = module.baseline_definition(source)
        forms = [body for _, body in module.variants(original)]
        self.assertEqual(len(set(forms)), 60)
        self.assertIn(original, forms)
        self.assertIn(definition(source, module.NAME), forms)
        selected = os.environ.get("HOMM3_UNDERGROUND_MANIFEST")
        if selected:
            for option in json.loads(Path(selected).read_text())["axes"][0]["options"]:
                if option["replace"] not in forms:
                    forms.append(option["replace"])
        constructor = definition(header, "type_random_map", parameters="TRmgMapItem* items, int width, int height")
        body_constructors = [(body, constructor) for body in forms]
        if selected:
            for option in json.loads(Path(selected).read_text())["axes"][0]["options"]:
                changed = constructor
                for edit in option.get("extra_edits", []):
                    self.assertEqual(edit["source"], "include/rmg.h")
                    self.assertEqual(edit["find"], constructor)
                    changed = edit["replace"]
                pair = (option["replace"], changed)
                if pair not in body_constructors:
                    body_constructors.append(pair)
        positive_count = len(body_constructors)
        negatives = [
            ("m_map.getMapItem(0, 0, 1)", "m_map.getMapItem(0, 0, 0)"),
            ("TRmgTerrainBrush brush(&map, eTerrainRock, 4)", "TRmgTerrainBrush brush(&map, eTerrainRock, 3)"),
            ("!item->hasSubterraneanGate()", "item->hasSubterraneanGate()"),
            ("&& item->m_tileData.m_roadPassable", "&& !item->m_tileData.m_roadPassable"),
            ("&& !item->isRoadEntrance()", "&& item->isRoadEntrance()"),
            ("position.m_z != 1", "position.m_z == 1"),
            ("item->m_zoneState.m_zone == zone", "item->m_zoneState.m_zone != zone"),
            ("item->hasSubterraneanGate() || item->m_objects.size()", "item->hasSubterraneanGate() && item->m_objects.size()"),
            ("int currentTerrain = eTerrainRock", "int currentTerrain = eTerrainWater"),
            ("brush.changeTerrain(terrain, 4)", "brush.changeTerrain(eTerrainRock, 4)"),
            ("m_progress->advance(1200)", "m_progress->advance(1199)"),
        ]
        for old, new in negatives:
            self.assertIn(old, original)
            body_constructors.append((original.replace(old, new), constructor))
        oracle = r"""
bool check() {
    const int dimensions[][2]={{0,3},{3,0},{1,1},{1,4},{4,1},{3,4},{4,3}};
    for(int shape=0;shape<7;++shape) for(int pattern=0;pattern<32;++pattern)
    for(int zoneCount=0;zoneCount<=4;++zoneCount) for(int progress=0;progress<4;++progress) {
        int w=dimensions[shape][0],h=dimensions[shape][1],plane=w*h;
        TRmgZone zones[4];
        const int terrains[]={9,2,0,9};
        for(int a=0;a<4;++a) {
            zones[a].m_levelPosition=TRmgMapPosition(a,-a,(a+pattern)%3);
            zones[a].m_terrain=terrains[(a+pattern)%4];
            zones[a].m_bounds.m_minimumX=w>1 && a%2?1:0;
            zones[a].m_bounds.m_minimumY=h>1 && a%2?1:0;
            zones[a].m_bounds.m_maximumX=pattern%4==0?zones[a].m_bounds.m_minimumX:w;
            zones[a].m_bounds.m_maximumY=pattern%4==1?zones[a].m_bounds.m_minimumY:h;
        }
        std::vector<TRmgMapItem> cells(plane*2+2);
        for(unsigned i=0;i<cells.size();++i) {
            std::memset(&cells[i].m_zoneState,0xa5,sizeof(cells[i].m_zoneState));
            std::memset(&cells[i].m_tile,0xa5,sizeof(cells[i].m_tile));
            std::memset(&cells[i].m_tileData,0xa5,sizeof(cells[i].m_tileData));
            cells[i].m_zoneState.m_zone=(int)((i+pattern)%6)-2;
            cells[i].m_tile.m_landType=(i+pattern)%10;
            cells[i].m_tileData.m_subterraneanGate=((i^pattern)&1)!=0;
            cells[i].m_tileData.m_roadPassable=((i*3+pattern)&2)!=0;
            cells[i].m_tileData.m_roadEntrance=((i*5+pattern)&4)!=0;
            if((i*7+pattern)%3==0) cells[i].m_objects.push_back(&zones[0]);
        }
        std::vector<TRmgMapItem> expected=cells;
        std::vector<Event> events;append(events,1,9,4);
        for(int i=0;i<plane;++i) {
            TRmgMapItem& cell=expected[1+plane+i];
            if(cell.m_tileData.m_subterraneanGate || !cell.m_tileData.m_roadPassable ||
               cell.m_tileData.m_roadEntrance || cell.m_tile.m_landType==9) continue;
            append(events,2,i%w,i/w,11);cell.m_tile.m_landType=9;
        }
        if(progress) append(events,4,1200,0);
        int brushTerrain=9;
        for(int a=0;a<zoneCount;++a) {
            if(zones[a].m_levelPosition.m_z!=1) continue;
            int terrain=zones[a].m_terrain;
            if(brushTerrain==9) {append(events,3,terrain,4);brushTerrain=terrain;}
            for(int i=0;i<plane;++i) {
                int x=i%w,y=i/w;
                TRmgZoneBounds& bounds=zones[a].m_bounds;
                if(x<bounds.m_minimumX || x>=bounds.m_maximumX || y<bounds.m_minimumY || y>=bounds.m_maximumY) continue;
                TRmgMapItem& cell=expected[1+plane+i];
                if(cell.m_tile.m_landType!=9 || cell.m_zoneState.m_zone!=a ||
                   (!cell.m_tileData.m_subterraneanGate && cell.m_objects.empty())) continue;
                if(terrain!=brushTerrain) {append(events,3,terrain,4);brushTerrain=terrain;}
                append(events,2,x,y,11);cell.m_tile.m_landType=terrain;
            }
        }
        if(progress && progress!=3) append(events,4,1200,progress==2?1:0);
        append(events,5);append(events,6);
        Progress first={0},second={1};type_random_map_generator owner;
        owner.m_map.m_mapWidth=w;owner.m_map.m_mapHeight=h;owner.m_map.m_numberLevels=2;
        owner.m_map.m_mapItems=&cells[1];owner.m_progress=progress?&first:0;
        for(int a=0;a<zoneCount;++a) owner.m_zones.push_back(&zones[a]);
        g_width=w;g_height=h;g_cells=&cells[1];g_progressMode=progress;
        g_owner=&owner;g_replacement=&second;g_valid=true;g_events.clear();
        owner.decorateUnderground();
        if(!g_valid || g_events.size()!=events.size()) return false;
        for(unsigned i=0;i<events.size();++i) if(!sameEvent(events[i],g_events[i])) return false;
        for(unsigned i=0;i<cells.size();++i) {
            if(std::memcmp(&cells[i].m_zoneState,&expected[i].m_zoneState,sizeof(cells[i].m_zoneState))) return false;
            if(std::memcmp(&cells[i].m_tile,&expected[i].m_tile,sizeof(cells[i].m_tile))) return false;
            if(std::memcmp(&cells[i].m_tileData,&expected[i].m_tileData,sizeof(cells[i].m_tileData))) return false;
            if(cells[i].m_objects!=expected[i].m_objects) return false;
        }
    }
    return true;
}
"""
        text = "#include <vector>\n#include <cstring>\n#include <cstdio>\n"
        self.assertEqual(types.count(constructor), 1)
        for index, (body, body_constructor) in enumerate(body_constructors):
            variant_types = types.replace(constructor, body_constructor)
            text += f"namespace Case{index} {{\n" + variant_types + "\n" + body + "\n" + oracle + "\n}\n"
        text += "int main() {\n"
        for index in range(len(body_constructors)):
            text += f'if(Case{index}::check() != {str(index < positive_count).lower()}) {{std::printf("case {index} failed\\n");return 1;}}\n'
        text += f'std::puts("{positive_count} underground forms: 4480 scenarios each; {len(negatives)} negative controls rejected");return 0;}}\n'
        with tempfile.TemporaryDirectory(prefix="homm3-underground-oracle-") as directory:
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
