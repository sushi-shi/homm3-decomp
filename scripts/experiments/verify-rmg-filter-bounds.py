#!/usr/bin/env python3
"""Verify complete filtering and setter order against independent ranking.

Uses the rendered filter/count helper and actual coordinate, setter, size
and connection-predicate source. Integer lattice distance and a separate
candidate-ranking model provide expected surviving points and setter calls.
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    args = parser.parse_args()
    _, originals, axes = load_manifest(args.manifest, HOMM3_DIR)
    if len(axes) != 1:
        raise ValueError('expected one filter source axis')
    helper = generator('generate-rmg-position-family.py')
    files = [render(originals, axes, (i,)) for i in range(len(axes[0].options))]
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    count = len(files)
    bodies = [helper.definition(f['src/rmg.cpp'], 'type_random_map_generator::filterZonePositions') for f in files]
    counts = [helper.definition(f['src/rmg.cpp'], 'type_random_map_generator::countPlacedZoneConnections') for f in files]
    for old, new in (
            ('if (candidate > 0)', 'if (candidate >= 0)'),
            ('countPlacedZoneConnections(zone) < bestConnections', 'countPlacedZoneConnections(zone) > bestConnections'),
            ('int bestSize = 32000;', 'int bestSize = 0;'),
            ('if (bestSize < candidateSize)', 'if (bestSize <= candidateSize)'),
            ('position.m_x + size + 1', 'position.m_x - size + 1')):
        if bodies[0].count(old) != 1:
            raise ValueError('stale negative-control anchor: ' + old)
        bodies.append(bodies[0].replace(old, new))
        counts.append(counts[0])
    program = [r'''
#include <vector>
#include <cmath>
#include <cstdio>
using std::sqrt;
namespace std {
template<class T> const T& _cpp_min(const T& a, const T& b) { return b < a ? b : a; }
template<class T> const T& _cpp_max(const T& a, const T& b) { return a < b ? b : a; }
}
''', '#include "homm3_minmax.h"\n']
    program += [block(header, name) + '\n' for name in ('TRmgVector', 'TPoint', 'TRmgMapPosition', 'TRmgZoneBounds')]
    program += [helper.definition(source, 'TRmgMapPosition::TRmgMapPosition'), r'''
static std::vector<TRmgMapPosition> updates;
struct TRmgTownSlot;
struct TRmgZoneConnection { TRmgTownSlot* m_destination; };
struct TRmgTownSlot { int m_zoneIndex; int m_size; std::vector<TRmgZoneConnection> m_connections; };
struct TRmgZone {
    TRmgTownSlot* m_slot;
    TRmgMapPosition m_levelPosition;
    TRmgMapPosition getLevelPosition() const;
    void setLevelPosition(TRmgMapPosition position);
    int getSize() const;
    unsigned char canConnect(const TRmgZone* other) const;
};
''', helper.definition(block(header, 'TRmgZone'), 'getSize').replace('getSize()', 'TRmgZone::getSize()', 1), '\n',
        helper.definition(source, 'TRmgZone::getLevelPosition'), '\n',
        helper.definition(source, 'TRmgZone::setLevelPosition').replace('\n{\n', '\n{\n    updates.push_back(position);\n'), '\n',
        helper.definition(source, 'TRmgZone::canConnect'), '\n']
    for i, (body, counting) in enumerate(zip(bodies, counts)):
        # VC6 exposes this for-init variable to the following filter passes.
        # Spell that same lifetime explicitly for the host compiler.
        body = body.replace('    for (int candidate = 0;', '    int candidate;\n    for (candidate = 0;')
        program += ['struct Generator' + str(i) + ' { struct Map { TRmgMapPosition m_size; } m_map;\n'
                    'std::vector<TRmgZone*> m_zones;\n',
                    counting.replace('type_random_map_generator::', ''), '\n',
                    body.replace('type_random_map_generator::', ''), '\n};\n']
    program.append(r'''
bool equal(const TRmgMapPosition& a, const TRmgMapPosition& b) {
    return a.m_x == b.m_x && a.m_y == b.m_y && a.m_z == b.m_z;
}
bool equal(const std::vector<TRmgMapPosition>& a, const std::vector<TRmgMapPosition>& b) {
    if (a.size() != b.size()) return false;
    for (unsigned i = 0; i < a.size(); ++i) if (!equal(a[i], b[i])) return false;
    return true;
}
int connectionCount(const std::vector<TRmgZone*>& zones, TRmgZone* source, const TRmgMapPosition& position) {
    int count = 0;
    const std::vector<TRmgZoneConnection>& connections = source->m_slot->m_connections;
    for (unsigned i = 0; i < connections.size(); ++i) {
        int index = connections[i].m_destination->m_zoneIndex;
        if (index < 0 || index >= int(zones.size())) continue;
        const TRmgZone* other = zones[index];
        TRmgMapPosition otherPosition = other == source ? position : other->m_levelPosition;
        int dx = otherPosition.m_x - position.m_x, dy = otherPosition.m_y - position.m_y;
        int distance = 0;
        while ((distance + 1) * (distance + 1) <= dx * dx + dy * dy) ++distance;
        int left = source->m_slot->m_size, right = other->m_slot->m_size;
        int small = left < right ? left : right;
        bool accepted = otherPosition.m_z == position.m_z
            ? 11 * (left + right) >= 10 * distance
            : left + right >= distance && left + right - distance > small / 2;
        if (accepted) ++count;
    }
    return count;
}
int extent(const std::vector<TRmgZone*>& zones, TRmgZone* source, const TRmgMapPosition& candidate, int mapSize) {
    int lowX = 0, lowY = 0, highX = 0, highY = 0;
    for (unsigned i = 0; i <= zones.size(); ++i) {
        if (i < zones.size() && zones[i] == source) continue;
        const TRmgZone* zone = i == zones.size() ? source : zones[i];
        TRmgMapPosition p = i == zones.size() ? candidate : zone->m_levelPosition;
        int size = zone->m_slot->m_size;
        if (p.m_x - size < lowX) lowX = p.m_x - size;
        if (p.m_y - size < lowY) lowY = p.m_y - size;
        if (p.m_x + size + 1 > highX) highX = p.m_x + size + 1;
        if (p.m_y + size + 1 > highY) highY = p.m_y + size + 1;
    }
    int answer = mapSize;
    if (highY - lowY > answer) answer = highY - lowY;
    if (highX - lowX > answer) answer = highX - lowX;
    return answer;
}
template<class Generator> bool check() {
    for (int seed = 0; seed != 128; ++seed)
    for (int zoneCount = 0; zoneCount != 7; ++zoneCount)
    for (int length = 0; length != 7; ++length)
    for (int levels = 1; levels <= 2; ++levels)
    for (int mapChoice = 0; mapChoice != 3; ++mapChoice) {
        const int mapSizes[] = {0, 4, 12};
        int mapSize = mapSizes[mapChoice];
        Generator generator;
        generator.m_map.m_size = TRmgMapPosition(36, 36, levels);
        TRmgTownSlot slots[9], sourceSlot;
        TRmgZone zones[7], source;
        source.m_slot = &sourceSlot;
        sourceSlot.m_size = seed % 6;
        source.m_levelPosition = TRmgMapPosition(21, 22, 0);
        for (int i = 0; i != 9; ++i) {
            slots[i].m_zoneIndex = i - 1;
            slots[i].m_size = (seed + i * 3) % 7;
        }
        for (int i = 0; i < zoneCount; ++i) {
            zones[i].m_slot = &slots[i];
            zones[i].m_levelPosition = TRmgMapPosition((seed + i * 3) % 17 - 8,
                (seed * 3 + i * 7) % 17 - 8, (seed + i) % levels);
            generator.m_zones.push_back(i == 0 && seed % 3 == 0 ? &source : &zones[i]);
        }
        for (int i = 0; i != 5; ++i) {
            TRmgZoneConnection connection;
            connection.m_destination = &slots[(seed + i * 5) % 9];
            sourceSlot.m_connections.push_back(connection);
        }
        std::vector<TRmgMapPosition> input;
        for (int i = 0; i < length; ++i)
            input.push_back(TRmgMapPosition((seed * 7 + i * 5) % 19 - 9,
                (seed * 11 + i * 7) % 19 - 9, (seed + i) % levels));
        std::vector<TRmgMapPosition> expected = input, expectedUpdates;
        if (levels > 1) {
            bool used[2] = {false, false};
            for (unsigned i = 0; i < generator.m_zones.size(); ++i)
                if (generator.m_zones[i] != &source) used[generator.m_zones[i]->m_levelPosition.m_z] = true;
            int lastUnused = -1;
            for (unsigned i = 0; i < expected.size(); ++i) if (!used[expected[i].m_z]) lastUnused = i;
            // Retail's >0 guard deliberately leaves the only-unused-index-0
            // case untouched; a >=0 negative control must be rejected.
            if ((!used[0] || !used[1]) && lastUnused > 0) {
                std::vector<TRmgMapPosition> free;
                for (unsigned i = 0; i < expected.size(); ++i) if (!used[expected[i].m_z]) free.push_back(expected[i]);
                expected = free;
            }
        }
        int best = 0;
        for (unsigned i = 0; i < expected.size(); ++i) {
            expectedUpdates.push_back(expected[i]);
            int count = connectionCount(generator.m_zones, &source, expected[i]);
            if (count > best) best = count;
        }
        for (int i = int(expected.size()) - 1; i >= 0; --i) {
            expectedUpdates.push_back(expected[i]);
            if (connectionCount(generator.m_zones, &source, expected[i]) < best) expected.erase(expected.begin() + i);
        }
        int bestExtent = 32000;
        for (unsigned i = 0; i < expected.size(); ++i) {
            int size = extent(generator.m_zones, &source, expected[i], mapSize);
            if (size < bestExtent) bestExtent = size;
        }
        for (int i = int(expected.size()) - 1; i >= 0; --i)
            if (extent(generator.m_zones, &source, expected[i], mapSize) > bestExtent) expected.erase(expected.begin() + i);
        TRmgMapPosition finalPosition = expectedUpdates.empty() ? source.m_levelPosition : expectedUpdates.back();
        updates.clear();
        generator.filterZonePositions(&source, input, mapSize);
        if (!equal(input, expected) || !equal(updates, expectedUpdates) || !equal(source.m_levelPosition, finalPosition)) return false;
    }
    return true;
}
int main() {
''')
    for i in range(len(bodies)):
        program.append('if (' + ('!' if i < count else '') + 'check<Generator' + str(i) + '>()) '
                       '{ std::puts("failed case ' + str(i) + '"); return 1; }\n')
    program.append('std::puts("' + str(count) + ' filter forms: 37632 scenarios each; five negative controls rejected");}\n')
    with tempfile.TemporaryDirectory(prefix='rmg-filter-bounds-') as directory:
        path = Path(directory)
        (path / 'oracle.cpp').write_text(''.join(program))
        subprocess.run(['g++', '-std=c++98', '-O2', '-I', str(HOMM3_DIR / 'include'),
                        str(path / 'oracle.cpp'), '-o', str(path / 'oracle')], check=True)
        subprocess.run([str(path / 'oracle')], check=True)


if __name__ == '__main__':
    main()
