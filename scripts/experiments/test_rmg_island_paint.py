"""Native island painting contract with opaque brush/mask/progress boundaries.

Imports actual value types, packed fields, lookup and borrowed constructor.
The brush fixture tests arguments, ordered paints and destruction-before-tagging;
it does not reconstruct the terrain brush or assert a native/x86 layout match.
"""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class IslandPaintTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native g++")
    def test_coordinate_and_lifetime_forms(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        module = generator("generate-rmg-island-paint-family.py")
        definition = generator("generate-rmg-position-family.py").definition

        def block(text, prefix):
            start = text.index(prefix + " {")
            return text[start:text.index("\n};", start) + 3]

        types = "\n".join(block(header, "struct " + name) for name in (
            "TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds", "TRmgGroundTile",
            "TRmgGroundTileData", "TRmgConnectionDecoration"))
        types += "\n" + block((root / "include/terrain_type.h").read_text(), "enum TTerrainType")
        types += r"""
struct TRmgMapItem {
    TRmgGroundTile m_tile;
    TRmgGroundTileData m_tileData;
    TRmgConnectionDecoration m_connection;
};
struct Event { int m_kind,m_a,m_b,m_c; };
std::vector<Event> g_events;
int g_pattern,g_width,g_height,g_level,g_extentWidth,g_extentHeight;
TRmgMapItem* g_cells;
bool g_valid;
void event(int kind,int a=0,int b=0,int c=0) {
    Event e={kind,a,b,c};g_events.push_back(e);
}
int rand() { event(1); return g_pattern; }
struct type_random_map {
    unsigned char m_ownsMapItems;
    TRmgMapItem* m_mapItems;
    int m_mapWidth,m_mapHeight,m_numberLevels;
    type_random_map() : m_ownsMapItems(1) {}
    ~type_random_map() { if(!m_ownsMapItems) event(5); }
"""
        types += definition(header, "type_random_map", parameters="TRmgMapItem* items, int width, int height")
        types += "\n" + definition(header, "getMapItem", parameters="int x, int y, int z")
        types += "\nTRmgMapItem* getMapItem(TRmgMapPosition point);\n"
        types += r"""
};
struct TRmgTerrainBrush {
    type_random_map* m_map;
    int m_terrain;
    TRmgTerrainBrush(type_random_map* map,int terrain,int strength) : m_map(map),m_terrain(terrain) {
        event(2,terrain,strength);
        if(map->m_mapWidth!=g_width || map->m_mapHeight!=g_height || map->m_numberLevels!=1 ||
           map->m_ownsMapItems || map->m_mapItems!=g_cells+g_level*g_width*g_height) g_valid=false;
    }
    ~TRmgTerrainBrush() {
        event(4);
        // An opaque completion update makes premature tile tagging observable.
        if(g_valid) m_map->m_mapItems[0].m_tile.m_landType=eTerrainWater;
    }
    void paintRectangle(unsigned x,unsigned y,unsigned w,unsigned h) {
        event(3,x,y,w*10+h);
        if(w!=1 || h!=1 || x>=static_cast<unsigned>(g_width) || y>=static_cast<unsigned>(g_height)) {
            g_valid=false;return;
        }
        if(g_valid) m_map->m_mapItems[y*g_width+x].m_tile.m_landType=m_terrain;
    }
};
struct TProgressSink { void advance(int amount) { event(6,amount); } };
struct type_random_map_generator {
    type_random_map m_map;
    TProgressSink* m_progress;
    void createWaterZoneIsland(const TRmgZoneBounds&,int);
};
void generateRmgIslandMask(unsigned char* mask,int width,int height) {
    event(7,width,height);
    if(width!=g_extentWidth || height!=g_extentHeight) {g_valid=false;return;}
    for(int y=0;y<height;++y) for(int x=0;x<width;++x)
        mask[y*width+x]=((3*x+5*y+g_pattern)%4)!=0;
}
bool sameEvent(const Event& a,const Event& b) {
    return a.m_kind==b.m_kind && a.m_a==b.m_a && a.m_b==b.m_b && a.m_c==b.m_c;
}
void expectedEvent(std::vector<Event>& events,int kind,int a=0,int b=0,int c=0) {
    Event e={kind,a,b,c};events.push_back(e);
}
"""
        types += "\n" + definition(source, "type_random_map::getMapItem", parameters="TRmgMapPosition point")
        original = module.baseline_definition(source)
        forms = [body for _, body in module.variants(original)]
        self.assertEqual(len(forms), 60)
        self.assertEqual(len(set(forms)), 60)
        current = definition(source, module.NAME)
        if current not in forms:
            forms.append(current)
        selected = os.environ.get("HOMM3_ISLAND_PAINT_MANIFEST")
        if selected:
            for option in json.loads(Path(selected).read_text())["axes"][0]["options"]:
                if option["replace"] not in forms:
                    forms.append(option["replace"])
        positive_count = len(forms)
        for old, new in (
            ("m_map.getMapItem(0, 0, level)", "m_map.getMapItem(0, 0, 0)"),
            ("TRmgTerrainBrush brush(&map, terrain, 4)", "TRmgTerrainBrush brush(&map, terrain, 3)"),
            ("rand() % 6", "rand() % 6 + 1"),
            ("bounds.m_minimumX] > 0", "bounds.m_minimumX] == 0"),
            ("!= eTerrainWater", "== eTerrainWater"),
            ("advance(1000)", "advance(999)"),
        ):
            self.assertIn(old, original)
            forms.append(original.replace(old, new))
        if "point.m_z = level;" in current:
            forms.append(current.replace("point.m_z = level;", "point.m_z = 0;"))
        negative_count = len(forms) - positive_count
        oracle = r"""
bool check() {
    for(int width=2;width<=5;++width) for(int height=2;height<=4;++height)
    for(int level=0;level<2;++level) for(int pattern=0;pattern<12;++pattern)
    for(int shape=0;shape<4;++shape) {
        TRmgZoneBounds bounds;
        bounds.m_minimumX=(shape==1 || shape==3) ? 1:0;
        bounds.m_minimumY=shape==2 ? 1:0;
        bounds.m_maximumX=width;
        bounds.m_maximumY=shape==3 ? height-1:height;
        std::vector<TRmgMapItem> cells(width*height*2);
        for(unsigned i=0;i<cells.size();++i) {
            std::memset(&cells[i],0,sizeof(cells[i]));
            cells[i].m_tile.m_landType=(i+pattern)%10;
            cells[i].m_connection.m_present=(i+pattern)%3==0;
            std::memset(&cells[i].m_tileData,0xa5,sizeof(cells[i].m_tileData));
            cells[i].m_tileData.m_subterraneanGate=1;
            cells[i].m_tileData.m_borderObject=0;
        }
        std::vector<TRmgMapItem> expected=cells;
        std::vector<Event> events;
        expectedEvent(events,1);expectedEvent(events,2,pattern%6,4);
        int ex=bounds.m_maximumX-bounds.m_minimumX,ey=bounds.m_maximumY-bounds.m_minimumY;
        expectedEvent(events,7,ex,ey);
        // Enumerate flat cells and test rectangle membership independently.
        for(int index=0;index<width*height;++index) {
            int x=index%width,y=index/width;
            bool inside=x>=bounds.m_minimumX && x<bounds.m_maximumX &&
                        y>=bounds.m_minimumY && y<bounds.m_maximumY;
            bool marked=(3*(x-bounds.m_minimumX)+5*(y-bounds.m_minimumY)+pattern)%4!=0;
            if(inside && marked) {
                expectedEvent(events,3,x,y,11);
                expected[level*width*height+index].m_tile.m_landType=pattern%6;
            }
        }
        expectedEvent(events,4);
        expected[level*width*height].m_tile.m_landType=eTerrainWater;
        expectedEvent(events,5);
        for(int index=0;index<width*height;++index) {
            int x=index%width,y=index/width;
            TRmgMapItem& item=expected[level*width*height+index];
            if(x>=bounds.m_minimumX && x<bounds.m_maximumX && y>=bounds.m_minimumY && y<bounds.m_maximumY &&
                item.m_tile.m_landType!=eTerrainWater && !item.m_connection.m_present) {
                item.m_tileData.m_subterraneanGate=0;item.m_tileData.m_borderObject=1;
            }
        }
        TProgressSink progress;
        if(pattern&1) expectedEvent(events,6,1000);
        g_width=width;g_height=height;g_level=level;g_pattern=pattern;
        g_extentWidth=ex;g_extentHeight=ey;g_cells=&cells[0];g_valid=true;g_events.clear();
        type_random_map_generator owner;
        owner.m_map.m_mapWidth=width;owner.m_map.m_mapHeight=height;
        owner.m_map.m_numberLevels=2;owner.m_map.m_mapItems=&cells[0];
        owner.m_progress=(pattern&1) ? &progress:0;
        TRmgZoneBounds oldBounds=bounds;
        owner.createWaterZoneIsland(bounds,level);
        if(!g_valid || g_events.size()!=events.size() || std::memcmp(&bounds,&oldBounds,sizeof(bounds))) return false;
        for(unsigned i=0;i<events.size();++i) if(!sameEvent(events[i],g_events[i])) return false;
        for(unsigned i=0;i<cells.size();++i) if(std::memcmp(&cells[i],&expected[i],sizeof(cells[i]))) return false;
    }
    return true;
}
"""
        text = "#include <vector>\n#include <cstring>\n#include <cstdio>\n"
        for index, body in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + "\n" + body + "\n" + oracle + "\n}\n"
        text += "int main() {\n"
        for index in range(len(forms)):
            text += f'if(Case{index}::check() != {str(index < positive_count).lower()}) {{ std::printf("case {index} failed\\n"); return 1; }}\n'
        text += f'std::puts("{positive_count} island-paint forms: 1152 scenarios each; {negative_count} negative controls rejected"); return 0; }}\n'
        with tempfile.TemporaryDirectory(prefix="homm3-island-paint-oracle-") as directory:
            cpp = Path(directory) / "oracle.cpp"
            binary = Path(directory) / "oracle"
            cpp.write_text(text)
            compiled = subprocess.run(["g++", "-std=c++98", "-O0", "-fno-elide-constructors", str(cpp), "-o", str(binary)], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            checked = subprocess.run([str(binary)], capture_output=True, text=True, timeout=120)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)
            print(checked.stdout, end="")


if __name__ == "__main__":
    unittest.main()
