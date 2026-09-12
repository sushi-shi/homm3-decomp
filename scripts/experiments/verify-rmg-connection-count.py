#!/usr/bin/env python3
"""Check rendered connection-count helpers, including query order and aliases."""
import argparse
from pathlib import Path
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    args = parser.parse_args()
    _, originals, axes = load_manifest(args.manifest, HOMM3_DIR)
    if len(axes) != 1:
        raise ValueError('expected one helper source axis')
    helper = generator('generate-rmg-position-family.py')
    bodies = [helper.definition(render(originals, axes, (i,))['src/rmg.cpp'],
                                'type_random_map_generator::countPlacedZoneConnections')
              for i in range(len(axes[0].options))]
    count = len(bodies)
    for old, new in (
            ('++connection', 'connection += 2'),
            ('destination < m_zones.size()', 'destination > 0 && destination < m_zones.size()'),
            ('m_zones[destination]->canConnect(zone)', 'm_zones[destination]->canConnect(m_zones[destination])'),
            ('++result;', 'result += 2;')):
        if bodies[0].count(old) != 1:
            raise ValueError('stale negative-control anchor: ' + old)
        bodies.append(bodies[0].replace(old, new))
    program = [r'''
#include <vector>
#include <cstdio>
struct TRmgTownSlot;
struct TRmgZoneConnection { TRmgTownSlot* m_destination; };
struct TRmgTownSlot { int m_zoneIndex; std::vector<TRmgZoneConnection> m_connections; };
static std::vector<int> calls;
static int policy;
struct TRmgZone {
    TRmgTownSlot* m_slot;
    int id;
    unsigned char canConnect(TRmgZone* other) const {
        calls.push_back(id * 32 + other->id);
        return (id * 5 + other->id * 3 + policy) % 3 != 0;
    }
};
''']
    for i, body in enumerate(bodies):
        program += ['struct Generator' + str(i) + ' { std::vector<TRmgZone*> m_zones;\n',
                    body.replace('type_random_map_generator::', ''), '\n};\n']
    program.append(r'''
template<class Generator> bool check() {
    const int lengths[] = {0, 1, 2, 5, 9, 12};
    for (unsigned size = 0; size != 9; ++size)
    for (int pattern = 0; pattern != 64; ++pattern)
    for (int seed = 0; seed != 8; ++seed)
    for (int li = 0; li != 6; ++li) {
        Generator generator;
        TRmgTownSlot slots[12];
        TRmgZone zones[9];
        TRmgTownSlot sourceSlot;
        TRmgZone source;
        source.m_slot = &sourceSlot;
        source.id = seed;
        policy = pattern % 7;
        for (int i = 0; i != 9; ++i) {
            zones[i].m_slot = &slots[i];
            zones[i].id = i;
        }
        // Repeated pointers exercise distinct vector indexes aliasing a zone.
        for (unsigned i = 0; i != size; ++i)
            generator.m_zones.push_back(&zones[(i * 3 + seed) % 9]);
        std::vector<int> expected;
        int result = 0;
        for (int i = 0; i != lengths[li]; ++i) {
            int destination = (i * 7 + pattern) % 13 - 2;
            slots[i].m_zoneIndex = destination;
            TRmgZoneConnection connection;
            connection.m_destination = &slots[i];
            sourceSlot.m_connections.push_back(connection);
            if (destination >= 0 && destination < int(size)) {
                int id = (destination * 3 + seed) % 9;
                expected.push_back(id * 32 + seed);
                if ((id * 5 + seed * 3 + policy) % 3 != 0) ++result;
            }
        }
        calls.clear();
        if (generator.countPlacedZoneConnections(&source) != result || calls != expected) return false;
        if (generator.m_zones.size() != size || sourceSlot.m_connections.size() != unsigned(lengths[li])) return false;
        for (int i = 0; i != lengths[li]; ++i)
            if (slots[i].m_zoneIndex != (i * 7 + pattern) % 13 - 2 ||
                sourceSlot.m_connections[i].m_destination != &slots[i]) return false;
    }
    return true;
}
int main() {
''')
    for i in range(len(bodies)):
        program.append('if (' + ('!' if i < count else '') + 'check<Generator' + str(i) + '>()) '
                       '{ std::puts("failed case ' + str(i) + '"); return 1; }\n')
    program.append('std::puts("' + str(count) + ' connection-count forms: 27648 scenarios each; four negative controls rejected");}\n')
    with tempfile.TemporaryDirectory(prefix='rmg-connection-count-') as directory:
        path = Path(directory)
        (path / 'oracle.cpp').write_text(''.join(program))
        subprocess.run(['g++', '-std=c++98', '-O2', str(path / 'oracle.cpp'), '-o', str(path / 'oracle')], check=True)
        subprocess.run([str(path / 'oracle')], check=True)


if __name__ == '__main__':
    main()
