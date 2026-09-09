"""Bounded integer-distance and player-zone admission oracle."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class ZoneFitTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_distance_floor_filtering_and_input_ownership(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-zone-fit-family.py")
        helper = generator("generate-rmg-position-family.py")
        source = (root / module.SOURCE).read_text()
        header = (root / "include/rmg.h").read_text()
        towns = (root / "include/town.h").read_text()
        def block(text, prefix):
            start = text.index(prefix + " {")
            return text[start:text.index("\n};", start) + 3]
        slot = block(header, "struct TRmgTownSlot")
        zone = block(header, "struct TRmgZone")
        def field(text, name):
            return next(line.split("//")[0].strip() for line in text.splitlines() if name + ";" in line)
        types = block(header, "struct TRmgVector") + "\n" + block(header, "struct TPoint")
        types += "\n" + block(header, "struct TRmgMapPosition")
        types += "\n" + block(header, "enum ERmgTemplateZoneKind") + "\n" + block(towns, "enum TTownType")
        types += "\nstruct TRmgTownSlot { " + " ".join(field(slot, name) for name in ("m_zoneIndex", "m_kind", "m_size")) + " };\n"
        types += "struct TRmgZone { " + " ".join(field(zone, name) for name in ("m_slot", "m_alignment", "m_levelPosition"))
        types += " TRmgMapPosition getLevelPosition() const; };\n"
        types += helper.definition(source, "TRmgZone::getLevelPosition")
        forms = [option["replace"] for option in module.variants(source)]
        self.assertEqual(len(forms), 60)
        self.assertEqual(len(set(forms)), 60)
        displacements = [option["replace"] for option in module.displacement_frontier(source)]
        self.assertEqual(len(displacements), 45)
        forms = list(dict.fromkeys(forms + displacements))
        self.assertEqual(len(forms), 104)
        count = len(forms)
        seed = forms[0]
        forms += [seed.replace("position.m_z == 1", "position.m_z != 0"),
                  seed.replace("zone->m_alignment != TOWN_INFERNO", "zone->m_alignment != TOWN_CASTLE"),
                  seed.replace("getLevelPosition().m_z != position.m_z", "getLevelPosition().m_z == position.m_z"),
                  seed.replace("m_zoneIndex == zoneIndex", "m_zoneIndex != zoneIndex"),
                  seed.replace("dx * dx + dy * dy", "dx * dx + dx * dx"),
                  seed.replace("10 * distance < 8 *", "10 * distance <= 8 *"),
                  seed.replace("10 * distance < 8 *", "10 * distance < 7 *")]
        self.assertTrue(all(body != seed for body in forms[count:]))
        text = "#include <vector>\n#include <cmath>\n#include <cstdio>\n"
        for index, body in enumerate(forms):
            text += f"namespace Case{index} {{\n" + types + r"""
static std::vector<int> g_queries;
static bool g_badQuery;
double sqrt(double value) {
    int integer=static_cast<int>(value);
    if(value!=integer || integer<0) g_badQuery=true;
    g_queries.push_back(integer);
    return std::sqrt(value);
}
struct type_random_map_generator {
    std::vector<TRmgZone*> m_zones;
    unsigned char canPlaceZone(TRmgZone*);
};
""" + body + r"""
int floorRoot(int squared) {
    int lo=0,hi=128;
    while(lo+1<hi) {
        int mid=(lo+hi)/2;
        if(mid*mid<=squared) lo=mid; else hi=mid;
    }
    return lo;
}
bool same(const TRmgZone& a,const TRmgZone& b) {
    return a.m_slot==b.m_slot && a.m_alignment==b.m_alignment
        && a.m_levelPosition.m_x==b.m_levelPosition.m_x
        && a.m_levelPosition.m_y==b.m_levelPosition.m_y
        && a.m_levelPosition.m_z==b.m_levelPosition.m_z;
}
bool check() {
    for(int pattern=0;pattern<32;++pattern) for(int kind=-1;kind<5;++kind)
    for(int level=-1;level<3;++level) for(int alignment=-1;alignment<9;++alignment)
    for(int radius=0;radius<3;++radius) {
        type_random_map_generator owner;
        TRmgTownSlot slots[3];TRmgZone zones[3];
        for(int i=0;i<3;++i) {
            slots[i].m_zoneIndex=pattern%7-3+i;
            slots[i].m_kind=kind;slots[i].m_size=radius==1?0:radius==2?4+i:2+i;
            zones[i].m_slot=&slots[i];zones[i].m_alignment=alignment;
            zones[i].m_levelPosition.m_x=pattern%11-7;
            zones[i].m_levelPosition.m_y=pattern%9-4;
            zones[i].m_levelPosition.m_z=level;
        }
        zones[1].m_levelPosition.m_x+=pattern-(radius==2?13:12);
        zones[1].m_levelPosition.m_y+=radius==2?0:pattern%7-3;
        zones[1].m_levelPosition.m_z+=(pattern&1)?1:0;
        if(pattern&2) slots[1].m_zoneIndex=slots[0].m_zoneIndex;
        zones[2].m_levelPosition.m_x+=40;
        zones[2].m_levelPosition.m_y-=20;
        if(pattern) owner.m_zones.push_back(&zones[1]);
        if(pattern&4) owner.m_zones.push_back(&zones[0]);
        if(pattern&8) owner.m_zones.push_back(&zones[2]);
        TRmgZone saved[3]={zones[0],zones[1],zones[2]};
        TRmgTownSlot savedSlots[3]={slots[0],slots[1],slots[2]};
        std::vector<TRmgZone*> savedList=owner.m_zones;
        std::vector<int> expectedQueries;
        bool permitted=!(kind>=0 && kind<=1 && level==1 && !(alignment>=3 && alignment<=5));
        if(permitted) for(unsigned i=0;i<owner.m_zones.size();++i) {
            const TRmgZone& other=*owner.m_zones[i];
            bool eligible=other.m_levelPosition.m_z==level && other.m_slot->m_zoneIndex!=slots[0].m_zoneIndex;
            if(!eligible) continue;
            int x=other.m_levelPosition.m_x-zones[0].m_levelPosition.m_x;
            int y=other.m_levelPosition.m_y-zones[0].m_levelPosition.m_y;
            int squared=x*x+y*y;
            expectedQueries.push_back(squared);
            int minimum=8*(other.m_slot->m_size+slots[0].m_size);
            if(10*floorRoot(squared)<minimum) { permitted=false;break; }
        }
        g_queries.clear();g_badQuery=false;
        unsigned char result=owner.canPlaceZone(&zones[0]);
        if(result!=static_cast<unsigned char>(permitted) || g_badQuery || g_queries!=expectedQueries
            || owner.m_zones!=savedList) return false;
        for(int i=0;i<3;++i) if(!same(zones[i],saved[i]) || slots[i].m_kind!=savedSlots[i].m_kind
            || slots[i].m_size!=savedSlots[i].m_size || slots[i].m_zoneIndex!=savedSlots[i].m_zoneIndex) return false;
    }
    return true;
}
}
"""
        text += "int main() {\n"
        text += "".join(f"if (!Case{i}::check()) return 1;\n" for i in range(count))
        text += "".join(f"if (Case{i}::check()) return {i-count+2};\n" for i in range(count, len(forms)))
        text += "return 0;\n}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-zone-fit-") as raw:
            directory = Path(raw)
            cpp, binary = directory / "predicate.cpp", directory / "predicate"
            cpp.write_text(text)
            result = subprocess.run([shutil.which("g++"), "-std=c++98", "-fno-elide-constructors", str(cpp), "-o", str(binary)],
                                    capture_output=True, text=True, timeout=90)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=90)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
