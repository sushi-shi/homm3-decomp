#!/usr/bin/env python3
"""Check the edited subterranean-gate prefix through candidate selection.

Extracts the actual entry checks, bounds snapshots, RNG query and complete
candidate scan. The untouched placement/guard suffix is outside this oracle.
An independent rectangle intersection and ordered ranking model checks the
chosen vector and query trace. Placement callbacks mutate the original zone
bounds; a first RNG callback can change the source level before its value is
captured. Coordinate and packed-zone declarations and the position accessor
come from the authored source.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def block(header, name):
    start = header.index('struct ' + name + ' {')
    return header[start:header.index('\n};', start) + 3]


def prefix(body):
    end = body.index('    if (candidates.size() == 0)')
    return body[:end] + '    selected = candidates;\n    return !candidates.empty();\n}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    args = parser.parse_args()
    _, originals, axes = load_manifest(args.manifest, HOMM3_DIR)
    if len(axes) != 1:
        raise ValueError('expected one subterranean source axis')
    helper = generator('generate-rmg-position-family.py')
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    full_bodies = [helper.definition(render(originals, axes, (i,))['src/rmg.cpp'],
                                    'type_random_map_generator::createSubterraneanGate')
                   for i in range(len(axes[0].options))]
    bodies = [prefix(body) for body in full_bodies]
    good = len(bodies)
    for old, new in (
            ('source->getLevelPosition().m_z == destination->getLevelPosition().m_z',
             'source->getLevelPosition().m_z != destination->getLevelPosition().m_z'),
            ('sourceBounds.m_minimumX, destinationBounds.m_minimumX',
             'sourceBounds.m_minimumX, destinationBounds.m_maximumX'),
            ('score < bestScore', 'score <= bestScore'),
            ('candidates.clear();', '/* bad control: keep earlier lower scores */'),
            ('score += destinationItem->m_zoneState.m_score;',
             'score = static_cast<unsigned short>(score + destinationItem->m_zoneState.m_score);'),
            ('TRmgZoneBounds sourceBounds = source->m_bounds;',
             'TRmgZoneBounds& sourceBounds = source->m_bounds;')):
        if bodies[0].count(old) != 1:
            raise ValueError('stale negative control: ' + old)
        bodies.append(bodies[0].replace(old, new))
    # Separately exercise any edited projection after the first object was
    # placed. The addObject callback is outside the edited region; its result
    # is represented by the current destination level supplied to this method.
    projected = []
    for body in full_bodies:
        start = body.index('\n    TRmgMapPosition otherPosition', body.index('    position = candidates[rand()'))
        end = body.index('    addObject(new type_object(gateProperties), otherPosition);', start)
        projected.append('    TRmgMapPosition projectPlaced(TRmgMapPosition position, TRmgZone* destination) {\n'
                         + body[start:end] + '    return otherPosition;\n    }\n')
        start = body.index('\n    otherPosition', body.index('    position -= TPoint('))
        end = body.index('    source->m_entrances.push_back(', start)
        projected[-1] += ('    TRmgMapPosition projectEntrance(TRmgMapPosition position, TRmgZone* destination) {\n'
                          '    TRmgMapPosition otherPosition(91, 92, 1);\n'
                          + body[start:end] + '    return otherPosition;\n    }\n')
    text = [r'''
#include <vector>
#include <cstdio>
namespace std {
template<class T> const T& _cpp_min(const T& a, const T& b) { return b < a ? b : a; }
template<class T> const T& _cpp_max(const T& a, const T& b) { return a < b ? b : a; }
}
#include "homm3_minmax.h"
''']
    text += [block(header, name) + '\n' for name in
             ('TRmgVector', 'TPoint', 'TRmgMapPosition', 'TRmgZoneBounds', 'TRmgZoneCellState')]
    text += [helper.definition(source, 'TRmgMapPosition::TRmgMapPosition'), r'''
enum { eTerrainWater = 8 };
struct TRmgTownSlot { int m_zoneIndex; };
struct TRmgZoneConnection { TRmgTownSlot* m_destination; };
struct TRmgZone {
    TRmgTownSlot* m_slot;
    int m_terrain;
    TRmgZoneBounds m_bounds;
    TRmgMapPosition m_levelPosition;
    TRmgMapPosition getLevelPosition() const;
};
''', helper.definition(source, 'TRmgZone::getLevelPosition'), r'''
struct TObjectType {};
struct TRmgObjectPropertiesRef { TObjectType* m_prototype; int id; };
struct TRmgMapItem { TRmgZoneCellState m_zoneState; };
int indexOf(const TRmgMapPosition& p) { return (p.m_z * 8 + p.m_y) * 8 + p.m_x; }
struct Environment {
    TRmgTownSlot slots[2];
    TRmgZone zones[2];
    TRmgZoneConnection connection;
    std::vector<TRmgZone*> m_zones;
    TObjectType prototypes[3];
    TRmgObjectPropertiesRef properties[3];
    std::vector<TRmgObjectPropertiesRef*> m_objectPrototypes[104];
    std::vector<TRmgMapPosition> selected;
    std::vector<int> queries, placements;
    int pattern, draws;
    bool invalid;
    struct Map {
        Environment* owner;
        TRmgMapItem cells[128];
        TRmgMapItem* getMapItem(TRmgMapPosition p) {
            if (p.m_x < 0 || p.m_x >= 8 || p.m_y < 0 || p.m_y >= 8 || p.m_z < 0 || p.m_z >= 2) {
                owner->invalid = true;
                return cells;
            }
            int index = indexOf(p);
            owner->queries.push_back(index);
            return cells + index;
        }
        bool canPlaceObject(TRmgObjectPropertiesRef* property, TRmgMapPosition p, TRmgZone* zone) {
            int index = indexOf(p), number = zone->m_slot->m_zoneIndex;
            owner->placements.push_back(index * 8 + number * 3 + property->id);
            // A reference to the source bounds cannot survive this callback
            // as an equivalent replacement for the original copied bounds.
            owner->zones[0].m_bounds.m_maximumX = 0;
            owner->zones[1].m_bounds.m_maximumY = 0;
            return (index + number * 3 + property->id + owner->pattern) % 5 != 0;
        }
    } m_map;
    int rand() {
        ++draws;
        if (pattern & 32) zones[0].m_levelPosition.m_z ^= 1;
        return pattern % 3;
    }
    void setup(int bits, int bounds, int levels, int water) {
        pattern = bits; draws = 0; invalid = false;
        selected.clear(); queries.clear(); placements.clear(); m_zones.clear();
        for (int i = 0; i < 104; ++i) m_objectPrototypes[i].clear();
        for (int i = 0; i < 3; ++i) {
            properties[i].m_prototype = prototypes + i; properties[i].id = i;
            m_objectPrototypes[103].push_back(properties + i);
        }
        const int rectangles[8][8] = {
            {1,1,5,5,2,2,6,6}, {2,2,6,6,1,1,5,5},
            {1,1,3,3,4,4,6,6}, {2,2,4,4,2,2,4,4},
            {0,0,7,7,1,3,6,5}, {1,3,6,5,0,0,7,7},
            {3,3,3,5,1,1,6,6}, {1,1,5,3,2,3,6,5}
        };
        for (int i = 0; i < 2; ++i) {
            slots[i].m_zoneIndex = i; zones[i].m_slot = slots + i;
            zones[i].m_levelPosition = TRmgMapPosition(2 + i, 3 + i, i);
            zones[i].m_terrain = i ? 0 : (water ? 8 : 3);
            zones[i].m_bounds.m_minimumX = rectangles[bounds][4*i];
            zones[i].m_bounds.m_minimumY = rectangles[bounds][4*i+1];
            zones[i].m_bounds.m_maximumX = rectangles[bounds][4*i+2];
            zones[i].m_bounds.m_maximumY = rectangles[bounds][4*i+3];
            m_zones.push_back(zones + i);
        }
        if (levels == 1) zones[1].m_levelPosition.m_z = 0;
        if (levels == 2) { zones[0].m_levelPosition.m_z = 1; zones[1].m_levelPosition.m_z = 0; }
        connection.m_destination = slots + 1;
        m_map.owner = this;
        for (int i = 0; i < 128; ++i) {
            m_map.cells[i].m_zoneState.m_zone = (i * 7 + bits) % 11 ? i / 64 : -1;
            int score = (i * 13 + bits * 3) % 7;
            m_map.cells[i].m_zoneState.m_score = (bits & 16) && i % 3 ? 65535 - score : score;
            m_map.cells[i].m_zoneState.m_connectionEligibility = (i + bits) % 100;
        }
    }
};
''']
    for i, body in enumerate(bodies):
        text += ['struct Generator' + str(i) + ' : Environment {\n',
                 body.replace('type_random_map_generator::', ''), '\n};\n']
    for i, method in enumerate(projected):
        text += ['struct Projection' + str(i) + ' {\n', method, '};\n']
    text += [r'''
bool samePoints(const std::vector<TRmgMapPosition>& a, const std::vector<TRmgMapPosition>& b) {
    if (a.size() != b.size()) return false;
    for (unsigned i = 0; i < a.size(); ++i) if (indexOf(a[i]) != indexOf(b[i])) return false;
    return true;
}
void expected(const Environment& initial, std::vector<TRmgMapPosition>& result,
              std::vector<int>& queries, std::vector<int>& placements, int& draws) {
    const TRmgZone& source = initial.zones[0];
    const TRmgZone& destination = initial.zones[1];
    if (source.m_levelPosition.m_z == destination.m_levelPosition.m_z || source.m_terrain == 8) return;
    // Enumerate the geometric intersection independently instead of using
    // the candidate's min/max selectors or updated bounds record.
    std::vector<TPoint> points;
    for (int y = 0; y != 8; ++y) for (int x = 0; x != 8; ++x) {
        if (x >= source.m_bounds.m_minimumX && x < source.m_bounds.m_maximumX &&
            y >= source.m_bounds.m_minimumY && y < source.m_bounds.m_maximumY &&
            x >= destination.m_bounds.m_minimumX && x < destination.m_bounds.m_maximumX &&
            y >= destination.m_bounds.m_minimumY && y < destination.m_bounds.m_maximumY)
            points.push_back(TPoint(x, y));
    }
    if (points.empty()) return;
    draws = 1;
    int sourceLevel = source.m_levelPosition.m_z ^ ((initial.pattern & 32) != 0);
    int gate = initial.pattern % 3, best = 0;
    for (unsigned i = 0; i < points.size(); ++i) {
        TRmgMapPosition first(points[i].m_x, points[i].m_y, sourceLevel);
        TRmgMapPosition second(points[i].m_x, points[i].m_y, destination.m_levelPosition.m_z);
        int a = indexOf(first), b = indexOf(second);
        queries.push_back(a);
        if (initial.m_map.cells[a].m_zoneState.m_zone != 0) continue;
        queries.push_back(b);
        if (initial.m_map.cells[b].m_zoneState.m_zone != 1) continue;
        int score = initial.m_map.cells[a].m_zoneState.m_score + initial.m_map.cells[b].m_zoneState.m_score;
        if (score < best) continue;
        placements.push_back(a * 8 + gate);
        if ((a + gate + initial.pattern) % 5 == 0) continue;
        placements.push_back(b * 8 + 3 + gate);
        if ((b + 3 + gate + initial.pattern) % 5 == 0) continue;
        if (score != best) { best = score; result.clear(); }
        result.push_back(first);
    }
}
template<class Generator> bool check() {
    for (int bits = 0; bits != 64; ++bits) for (int bounds = 0; bounds != 8; ++bounds)
    for (int levels = 0; levels != 3; ++levels) for (int water = 0; water != 2; ++water) {
        Generator actual; actual.setup(bits, bounds, levels, water);
        Environment before; before.setup(bits, bounds, levels, water);
        std::vector<TRmgMapPosition> chosen;
        std::vector<int> queries, placements;
        int draws = 0;
        expected(before, chosen, queries, placements, draws);
        bool result = actual.createSubterraneanGate(actual.zones, &actual.connection) != 0;
        if (actual.invalid || result != !chosen.empty() || actual.draws != draws ||
            actual.queries != queries || actual.placements != placements || !samePoints(actual.selected, chosen)) return false;
        for (int i = 0; i != 128; ++i) {
            if (actual.m_map.cells[i].m_zoneState.m_zone != before.m_map.cells[i].m_zoneState.m_zone ||
                actual.m_map.cells[i].m_zoneState.m_score != before.m_map.cells[i].m_zoneState.m_score ||
                actual.m_map.cells[i].m_zoneState.m_connectionEligibility != before.m_map.cells[i].m_zoneState.m_connectionEligibility) return false;
        }
    }
    return true;
}
int main() {
''']
    for i in range(len(bodies)):
        text.append('    if (' + ('!' if i < good else '') + 'check<Generator' + str(i) + '>()) { std::printf("failed form ' + str(i) + '\\n"); return 1; }\n')
    for i in range(good):
        text.append('    { Projection' + str(i) + ' projection; TRmgZone destination;\n'
                    '      for (int x = -2; x != 4; ++x) for (int y = -2; y != 4; ++y)\n'
                    '      for (int z = 0; z != 2; ++z) for (int other = 0; other != 2; ++other) {\n'
                    '        destination.m_levelPosition = TRmgMapPosition(77, 88, other);\n'
                    '        TRmgMapPosition p = projection.projectPlaced(TRmgMapPosition(x,y,z), &destination);\n'
                    '        if (p.m_x != x || p.m_y != y || p.m_z != other) return 1;\n'
                    '        p = projection.projectEntrance(TRmgMapPosition(x,y,z), &destination);\n'
                    '        if (p.m_x != x || p.m_y != y || p.m_z != other) return 1;\n'
                    '      } }\n')
    text.append('    std::puts("' + str(good) + ' subterranean prefixes: 3072 scenarios each; six bad controls rejected");\n}\n')
    with tempfile.TemporaryDirectory(prefix='rmg-subterranean-scan-') as folder:
        cpp, exe = Path(folder) / 'oracle.cpp', Path(folder) / 'oracle'
        cpp.write_text(''.join(text))
        subprocess.run(['g++', '-std=c++98', '-O2', '-I', str(HOMM3_DIR / 'include'), str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
