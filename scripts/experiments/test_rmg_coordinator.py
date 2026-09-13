#!/usr/bin/env python3
"""Native contract check for retail-only generate (0x549930).

Extracts authored bodies; models opaque helpers as ordered callbacks. This is
not an ABI/codegen verdict. Valid templates only: enough human/computer slots,
at most eight players, valid alignments, successful allocations. Expected map
uses independent stable partition/slot filtering, not the candidate loops.
"""
import argparse
import itertools
import json
import subprocess
import tempfile
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.test_rmg_families import generator


HARNESS = r'''
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
using std::memset;
@ENUMS@
std::vector<int> events;
int randomValue, randomCalls;
int scriptedRand() { ++randomCalls; return randomValue; }
struct TRmgTownSlot { int m_kind, m_playerIndex; };
struct TRmgTemplate { std::string m_name; std::vector<TRmgTownSlot*> m_zones; };
struct TRmgZone {
    TRmgTownSlot* m_slot;
    int m_terrain, m_alignment, id;
    unsigned char m_active;
};
struct Progress { void advance(int n) { events.push_back(10000+n); } };
struct Map {
    int m_numberLevels;
    void markCoastalTiles() { events.push_back(900); }
};
struct type_random_map_generator {
    std::vector<TRmgTemplate*> m_templates;
    std::vector<TRmgZone*> m_zones;
    std::string m_templateName;
    unsigned char m_fixedHumanPlayers[8];
    int m_playerIndexMap[9], m_activeZoneCountsByAlignment[9];
    int m_humanPlayerCount, m_computerPlayerCount, m_activeZoneCount;
    Map m_map;
    Progress* m_progress;
    TRmgZone zones[5];
    TRmgTemplate* replacement;
    int selected, initialZones;
    bool mutate;
    std::vector<int> capturedMap, capturedCounts;
    @DECLARATIONS@
    void initializeZones(TRmgTemplate* t) {
        events.push_back(100 + (t == replacement ? 9 : selected));
        capturedMap.assign(m_playerIndexMap, m_playerIndexMap+9);
        for (int i=0; i<initialZones; ++i) m_zones.push_back(zones+i);
        if (mutate) m_templates[selected]=replacement;
    }
    void buildZoneBoundaries(TRmgTemplate* t, int level) {
        events.push_back(200 + (t==replacement ? 10 : 0) + level);
        if (mutate && level==0 && m_map.m_numberLevels==1) m_map.m_numberLevels=2;
    }
    void paintZoneTerrain() { events.push_back(300); }
    void placePrimaryTown(TRmgZone* z) {
        events.push_back(400+z->id);
        if (mutate && z->id==0) m_zones.push_back(zones+3);
    }
    void placeAdditionalTowns(TRmgZone* z) { events.push_back(500+z->id); }
    void prepareZoneConnections() { events.push_back(600); }
    void prepareJunctionZone(TRmgZone* z) { events.push_back(610+z->id); }
    void placeMines() {
        events.push_back(700);
        for (unsigned i=0; i<m_zones.size(); ++i) {
            m_zones[i]->m_active = (i%2==0 ? 255 : 0);
            m_zones[i]->m_alignment = (i*3)%9;
        }
    }
    void buildZoneConnectionPaths() {
        events.push_back(750);
        capturedCounts.assign(m_activeZoneCountsByAlignment,m_activeZoneCountsByAlignment+9);
        capturedCounts.push_back(m_activeZoneCount);
    }
    void placeZoneTreasures(TRmgZone* z) {
        events.push_back(800+z->id);
        if (mutate && z->id==0) m_zones.push_back(zones+4);
    }
    void decorateUnderground() { events.push_back(850); }
    void decorateMap() { events.push_back(901); }
    void createRoads() { events.push_back(902); }
    void createRivers() { events.push_back(903); }
};
#define rand scriptedRand
@BODIES@
#undef rand
typedef unsigned char (type_random_map_generator::*Fn)();
bool check(Fn fn, int mask, int hMode, int cMode, bool mutate, bool progress, int levels) {
    type_random_map_generator g;
    TRmgTemplate templates[4];
    TRmgTownSlot slots[3][10], zoneSlots[5];
    Progress sink;
    randomValue=(mask*127+17)%32768;
    g.selected=randomValue%3;
    for (int t=0;t<3;++t) {
        templates[t].m_name=std::string(1,char('A'+t));
        for (int i=0;i<10;++i) {
            slots[t][i].m_kind=i<8 ? ((mask&(1<<i)) ? 0:1) : i-6;
            slots[t][i].m_playerIndex=i<8 ? i:99;
            templates[t].m_zones.push_back(&slots[t][i]);
        }
        g.m_templates.push_back(templates+t);
    }
    templates[3].m_name="replacement";
    g.replacement=templates+3;
    g.mutate=mutate;
    g.initialZones=mask%4;
    for (int i=0;i<5;++i) {
        zoneSlots[i].m_kind=i==2 ? 0:3;
        g.zones[i].m_slot=zoneSlots+i;
        g.zones[i].id=i;
        g.zones[i].m_terrain=i==1 ? eTerrainWater:0;
        g.zones[i].m_active=1;
        g.zones[i].m_alignment=8;
    }
    std::vector<int> human, ordered;
    for (int i=0;i<8;++i) {
        if (mask&(1<<i)) human.push_back(i);
        g.m_fixedHumanPlayers[i]=((mask^0xa5)&(1<<i)) ? (i%2 ? 255:1):0;
    }
    for (int fixed=1;fixed>=0;--fixed)
        for (int i=0;i<8;++i)
            if ((g.m_fixedHumanPlayers[i]!=0)==(fixed!=0)) ordered.push_back(i);
    g.m_humanPlayerCount=hMode==0 ? 0 : hMode==1 ? human.size()/2 : human.size();
    g.m_computerPlayerCount=cMode ? 8-g.m_humanPlayerCount:0;
    std::vector<int> expectedMap(9,-1), used;
    for (int i=0;i<g.m_humanPlayerCount;++i) {
        used.push_back(human[i]); expectedMap[human[i]+1]=ordered[i];
    }
    int ordinal=g.m_humanPlayerCount;
    for (int i=0;i<8 && ordinal<g.m_humanPlayerCount+g.m_computerPlayerCount;++i)
        if (std::find(used.begin(),used.end(),i)==used.end()) expectedMap[i+1]=ordered[ordinal++];
    std::fill(g.m_playerIndexMap,g.m_playerIndexMap+9,42);
    std::fill(g.m_activeZoneCountsByAlignment,g.m_activeZoneCountsByAlignment+9,42);
    g.m_activeZoneCount=42;
    g.m_templateName="before";
    g.m_map.m_numberLevels=levels;
    g.m_progress=progress ? &sink:0;
    std::vector<int> expected;
    expected.push_back(100+g.selected);
    int finalLevels=mutate && levels==1 ? 2:levels;
    for (int i=0;i<finalLevels;++i) expected.push_back(200+(mutate ? 10:0)+i);
    expected.push_back(300);
    std::vector<int> ids;
    for (int i=0;i<g.initialZones;++i) ids.push_back(i);
    if (mutate && g.initialZones) ids.push_back(3);
    for (unsigned i=0;i<ids.size();++i) expected.push_back(400+ids[i]);
    for (unsigned i=0;i<ids.size();++i) expected.push_back(500+ids[i]);
    expected.push_back(600);
    for (unsigned i=0;i<ids.size();++i)
        if (ids[i]!=1 && ids[i]!=2) expected.push_back(610+ids[i]);
    expected.push_back(700);
    std::vector<int> counts(10,0);
    for (unsigned i=0;i<ids.size();i+=2) { ++counts[(i*3)%9]; ++counts[9]; }
    expected.push_back(750);
    if (mutate && g.initialZones) ids.push_back(4);
    for (unsigned i=0;i<ids.size();++i) {
        expected.push_back(800+ids[i]);
        if (progress) expected.push_back(10000+6900/ids.size());
    }
    if (finalLevels>1) expected.push_back(850);
    for (int i=900;i<=903;++i) expected.push_back(i);
    events.clear(); randomCalls=0;
    if ((g.*fn)()!=1 || randomCalls!=1 || events!=expected || g.capturedMap!=expectedMap
        || g.capturedCounts!=counts || g.m_templateName!=templates[g.selected].m_name
        || !std::equal(expectedMap.begin(),expectedMap.end(),g.m_playerIndexMap)) return false;
    g.m_templates.clear(); events.clear(); randomCalls=0;
    return (g.*fn)()==0 && events.empty() && randomCalls==0
        && g.m_templateName==templates[g.selected].m_name
        && std::equal(expectedMap.begin(),expectedMap.end(),g.m_playerIndexMap)
        && std::equal(counts.begin(),counts.begin()+9,g.m_activeZoneCountsByAlignment)
        && g.m_activeZoneCount==counts[9] && g.m_zones.size()==ids.size();
}
int main() {
    Fn functions[]={@FUNCTIONS@};
    int positives=@POSITIVE_COUNT@;
    for (unsigned f=0;f<sizeof(functions)/sizeof(*functions);++f) {
        int failures=0, cases=0;
        for (int mask=0;mask<256;++mask) for (int h=0;h<3;++h)
        for (int c=0;c<2;++c) for (int mut=0;mut<2;++mut)
        for (int p=0;p<2;++p) for (int l=0;l<3;++l) {
            ++cases; if (!check(functions[f],mask,h,c,mut,p,l)) ++failures;
        }
        if ((f<unsigned(positives) && failures) || (f>=unsigned(positives) && !failures)) {
            std::printf("FAIL body %u failures=%d/%d\n",f,failures,cases); return 1;
        }
    }
    std::printf("PASS %d source forms, %u negative controls; 18432 cases/body plus empty-template exits\n",
        positives,unsigned(sizeof(functions)/sizeof(*functions))-positives);
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, action="append", default=[])
    args = parser.parse_args()
    family = generator("generate-rmg-coordinator-family.py")
    authored = family.definition((HOMM3_DIR / "src/rmg.cpp").read_text())
    original = family.variant(authored, 0, 0, 0)
    bodies = [family.variant(original, *v) for v in itertools.product(range(5), range(4), range(3))]
    for path in args.manifest:
        payload = json.loads(path.read_text())
        for axis in payload["axes"]:
            for option in axis["options"]:
                body = option["replace"]
                if body not in bodies:
                    bodies.append(body)
    positive_count = len(bodies)
    for old, new in (
        ("advance(6900 /", "advance(7000 /"),
        ("sizeof(m_playerIndexMap)", "sizeof(m_playerIndexMap) - sizeof(int)"),
        ("allSlots[slot] = 0;", "allSlots[slot] = 1;"),
        ("buildZoneBoundaries(m_templates[selected], level)", "buildZoneBoundaries(mapTemplate, level)"),
        ("placeAdditionalTowns(m_zones[zone]);", ";"),
        ("&& m_zones[zone]->m_terrain != eTerrainWater", ""),
        ("createRoads();", ";"),
    ):
        bodies.append(family.replace(original, old, new))
    projected = []
    for i, body in enumerate(bodies):
        # Only host adaptation: restore VC6's function-wide for declarations.
        body = body.replace("::generate()", "::generate%d()" % i)
        body = body.replace("{\n", "{\n    unsigned int zone; int player;\n", 1)
        body = body.replace("for (unsigned int zone =", "for (zone =")
        body = body.replace("for (int player =", "for (player =")
        projected.append(body)
    source = HARNESS.replace("@DECLARATIONS@", "\n".join("unsigned char generate%d();" % i for i in range(len(bodies))))
    enums = []
    for file, name in (("rmg.h", "ERmgTemplateZoneKind"), ("terrain_type.h", "TTerrainType")):
        header = (HOMM3_DIR / "include" / file).read_text()
        start = header.index("enum " + name + " {")
        enums.append(header[start:header.index("};", start) + 2])
    source = source.replace("@ENUMS@", "\n".join(enums))
    source = source.replace("@BODIES@", "\n".join(projected))
    source = source.replace("@FUNCTIONS@", ",".join("&type_random_map_generator::generate%d" % i for i in range(len(bodies))))
    source = source.replace("@POSITIVE_COUNT@", str(positive_count))
    with tempfile.TemporaryDirectory(prefix="rmg-coordinator-") as directory:
        executable = Path(directory) / "test"
        subprocess.run(["g++", "-std=c++98", "-O2", "-x", "c++", "-", "-o", str(executable)], input=source, text=True, check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
