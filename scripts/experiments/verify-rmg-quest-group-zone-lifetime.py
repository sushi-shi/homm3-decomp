#!/usr/bin/env python3
"""Check quest-group source bodies against independent stable-priority ordering.

Compile the actual worker with native std::vector and controlled distance,
random and placement calls. Cover score boundaries, exclusions, ties, early
success, and live growth of the zone list during randomization. The distance
algorithm and actual map placement are dependencies, not tested here.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator

FIXTURE = r'''
#include <algorithm>
#include <cstdio>
#include <vector>
@ZONE_KIND@
enum { eTerrainWater = 8 };
struct TRmgTownSlot { int m_kind; };
struct TRmgZone { TRmgTownSlot* m_slot; int m_questPlacementScore, m_terrain, id; };
struct TRmgTreasureGroup { int id; };
struct Root;
static Root* active;
static int randomValue(int seed, int draw) { return seed == 0 ? 0 : (seed * 37 + draw * 17); }
struct Root {
    std::vector<TRmgZone*> m_zones;
    TRmgTownSlot slots[5];
    TRmgZone zones[5];
    int distances[5], seed, draws, success, grow, bad, calculated;
    TRmgTreasureGroup* expectedGroup;
    std::vector<int> placements;
    void calculateQuestZoneDistances(TRmgZone* origin) {
        if (origin != &zones[0]) ++bad;
        ++calculated;
        for (unsigned i = 0; i < m_zones.size(); ++i)
            m_zones[i]->m_questPlacementScore = distances[m_zones[i]->id];
    }
    unsigned char placeTreasureGroup(TRmgTreasureGroup* group, TRmgZone* zone, int spacing) {
        if (group != expectedGroup || spacing != 1) ++bad;
        placements.push_back(zone->id);
        return zone->id == success;
    }
};
static int controlledRandom() {
    int value = randomValue(active->seed, active->draws);
    if (!active->draws && active->grow) active->m_zones.push_back(&active->zones[4]);
    ++active->draws;
    return value;
}
#define rand controlledRandom
@BODIES@
#undef rand
struct Ranked {
    int id, priority;
    Ranked(int i, int p) : id(i), priority(p) {}
};
static bool earlier(const Ranked& a, const Ranked& b) { return a.priority < b.priority; }
template<class Candidate> static bool check() {
    const int distances[6] = {0,1,2,199,200,32000};
    for (int code = 0; code < 1296; ++code) for (int seed = 0; seed < 3; ++seed)
    for (int policy = 0; policy < 4; ++policy) for (int success = 0; success < 5; ++success)
    for (int grow = 0; grow < 2; ++grow) {
        Candidate c;
        active = &c;
        c.seed = seed; c.draws = c.bad = c.calculated = 0;
        c.success = success; c.grow = grow;
        int digits = code;
        for (int i = 0; i < 5; ++i) {
            c.distances[i] = i == 4 ? 2 : distances[digits % 6];
            digits /= 6;
            c.slots[i].m_kind = policy == 1 && i == 1 ? RMG_TEMPLATE_JUNCTION : 0;
            c.zones[i].m_terrain = policy == 2 && i == 2 ? eTerrainWater : 0;
            if (policy == 3) {
                c.slots[i].m_kind = i == 2 ? RMG_TEMPLATE_JUNCTION : 0;
                c.zones[i].m_terrain = i == 3 ? eTerrainWater : 0;
            }
            c.zones[i].m_slot = &c.slots[i]; c.zones[i].id = i;
            c.zones[i].m_questPlacementScore = c.distances[i];
            if (i < 4) c.m_zones.push_back(&c.zones[i]);
        }
        TRmgTreasureGroup group;
        c.expectedGroup = &group;
        unsigned char got = c.placeQuestGroup(&group, &c.zones[0]);
        int count = 4 + grow;
        if (c.bad || c.calculated != 1 || c.draws != count || int(c.m_zones.size()) != count) return false;
        std::vector<Ranked> ranked;
        for (int i = 0; i < count; ++i) {
            int score = (c.distances[i] == 1 ? 1000 : c.distances[i] * 10) + randomValue(seed, i) % 10;
            if (c.zones[i].m_questPlacementScore != score || c.m_zones[i] != &c.zones[i]) return false;
            if (i != 0 && c.slots[i].m_kind != RMG_TEMPLATE_JUNCTION && score <= 2000
                && c.zones[i].m_terrain != eTerrainWater) ranked.push_back(Ranked(i, score));
        }
        std::stable_sort(ranked.begin(), ranked.end(), earlier);
        std::vector<int> wanted;
        bool placed = false;
        for (unsigned i = 0; i < ranked.size(); ++i) {
            wanted.push_back(ranked[i].id);
            if (ranked[i].id == success) { placed = true; break; }
        }
        if (c.placements != wanted || got != int(placed)) return false;
    }
    return true;
}
int main() {
@CHECKS@
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--snapshot', type=Path,
                        help='frozen source-family snapshot for a pre-adoption manifest')
    args = parser.parse_args()
    _, originals, axes = load_manifest(args.manifest, args.snapshot or HOMM3_DIR)
    if len(axes) != 1:
        raise ValueError('expected one full-function axis')
    helper = generator('generate-rmg-position-family.py')
    function = 'type_random_map_generator::placeQuestGroup'
    baseline = helper.definition(originals['src/rmg.cpp'], function)
    methods = list(dict.fromkeys(helper.definition(render(originals, axes, (i,))['src/rmg.cpp'], function)
                                for i in range(len(axes[0].options))))
    current = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), function)
    methods = list(dict.fromkeys([*methods, current]))
    bodies, checks = [], []

    def add(name, body, valid):
        body = body.replace('type_random_map_generator::', name + '::')
        # VC6's for-scope variable is used by the two subsequent loops.
        body = body.replace('    for (unsigned int index = 0;',
                            '    unsigned int index;\n    for (index = 0;', 1)
        bodies.append('struct ' + name + ' : Root { unsigned char placeQuestGroup(TRmgTreasureGroup*, TRmgZone*); };\n' + body)
        condition = '!check<' if valid else 'check<'
        checks.append('    if (' + condition + name + '>()) { std::fprintf(stderr, "failed ' + name + '\\n"); return 1; }')

    for i, body in enumerate(methods):
        add('Candidate' + str(i), body, True)
    zone_name = 'sharedZone' if 'TRmgZone* sharedZone;' in baseline else 'zone'
    controls = (
        ('WrongTies', '>= candidates[insertion]->m_questPlacementScore', '> candidates[insertion]->m_questPlacementScore'),
        ('WrongThreshold', 'zone->m_questPlacementScore > 2000', 'zone->m_questPlacementScore >= 2000'),
        ('WrongOrigin', 'zone == origin || ', ''),
        ('WrongSpacing', 'placeTreasureGroup(group, zone, 1)', 'placeTreasureGroup(group, zone, 2)'),
        ('WrongLiveBound', '    for (unsigned int index = 0; index < m_zones.size(); ++index)',
         '    unsigned int count = m_zones.size();\n    for (unsigned int index = 0; index < count; ++index)'),
    )
    for name, before, after in controls:
        before = before.replace('zone->', zone_name + '->').replace('zone == origin', zone_name + ' == origin').replace('group, zone,', 'group, ' + zone_name + ',')
        after = after.replace('zone->', zone_name + '->').replace('zone == origin', zone_name + ' == origin').replace('group, zone,', 'group, ' + zone_name + ',')
        if baseline.count(before) != 1:
            raise ValueError('review negative control ' + name)
        add(name, baseline.replace(before, after), False)
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    at = header.index('enum ERmgTemplateZoneKind {')
    kind = header[at:header.index('\n};', at) + 3]
    program = FIXTURE.replace('@ZONE_KIND@', kind)
    program = program.replace('@BODIES@', '\n'.join(bodies)).replace('@CHECKS@', '\n'.join(checks))
    with tempfile.TemporaryDirectory(prefix='rmg-quest-group-lifetime-') as directory:
        cpp, exe = Path(directory) / 'check.cpp', Path(directory) / 'check'
        cpp.write_text(program)
        subprocess.run(['g++', '-std=c++98', '-O2', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print(len(methods), 'bodies x 155520 scenarios; five negative controls rejected')


if __name__ == '__main__':
    main()
