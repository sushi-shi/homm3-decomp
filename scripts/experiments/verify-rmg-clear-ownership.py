#!/usr/bin/env python3
"""Check rendered reset bodies against retail masks, including untouched state.

The host compiler checks semantics only. The five packed declarations and
cell member order come from the current header; expected words come from
retail 0x530f10, independently of the candidate's bitfield expressions.
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
        raise ValueError('expected one packed-field ownership axis')
    helper = generator('generate-rmg-position-family.py')
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    names = ('TRmgVector', 'TPoint', 'TRmgMapPosition', 'TRmgMovementCost',
             'TRmgZoneCellState', 'TRmgGroundTile', 'TRmgGroundTileData',
             'TRmgConnectionDecoration')
    declarations = '\n'.join(block(header, name) for name in names)
    cell = block(header, 'TRmgMapItem')
    cell = cell[:cell.index('    TRmgMapItem();')] + '    void clear();\n};\n'
    count = len(axes[0].options)
    bodies = [helper.definition(render(originals, axes, (i,))['src/rmg.cpp'],
                                'TRmgMapItem::clear') for i in range(count)]
    # Normalize only the measured negative controls so that their mutation
    # anchors also work when the baseline is the adopted direct-terrain form.
    negative_base = generator('generate-rmg-clear-lifetime-family.py').copied_control(bodies[0])
    # Each bad control violates an independent part of the retail contract.
    for old, new in (
            ('    m_objects.erase(m_objects.begin(), m_objects.end());', ''),
            ('tile.m_terrainFrame = 21;', 'tile.m_terrainFrame = 20;'),
            ('    m_tileData = tileData;', '    tileData.m_connectionVisited = 0;\n    m_tileData = tileData;'),
            ('connection.m_present = 0;', 'connection.m_present = 1;'),
            ('m_zoneState.m_connectionEligibility = -1;', 'm_zoneState.m_connectionEligibility = 0;'),
            ('m_previousTile.m_x = -1;', 'm_previousTile.m_x = -1;\n    m_previousTile.m_y = -1;')):
        if negative_base.count(old) != 1:
            raise ValueError('stale negative-control anchor: ' + old)
        bodies.append(negative_base.replace(old, new))
    program = ['#include <vector>\n#include <cstring>\n#include <cstdio>\n'
               'enum { eTerrainWater = 8 };\n'
               'static int destroyed = 0;\n'
               'struct type_object { int marker; ~type_object() { ++destroyed; } };\n',
               declarations, '\n']
    for i, body in enumerate(bodies):
        program += ['namespace Case' + str(i) + ' {\n', cell, body, '\n}\n']
    program.append(r'''
template<class T> unsigned word(const T& value) {
    unsigned result = 0;
    std::memcpy(&result, &value, sizeof result);
    return result;
}
template<class T> void seed(T& value, unsigned bits) {
    std::memcpy(&value, &bits, sizeof bits);
}
template<class Cell> bool check() {
    type_object objects[9];
    const int lengths[] = {0, 1, 3, 9};
    for (unsigned state = 0; state < 4096; ++state) {
        // Mix all bits, explicitly cycling the one preserved tileData bit
        // and the two preserved high terrain bits as well.
        unsigned connection = state * 2654435761u;
        unsigned tile = (state * 2246822519u & 0x3fffffffu) | ((state & 3u) << 30);
        unsigned data = (state * 3266489917u & 0xfeffffffu) | ((state & 1u) << 24);
        for (unsigned li = 0; li != 4; ++li) {
            Cell cell;
            cell.m_objects.reserve(12);
            for (int i = 0; i < lengths[li]; ++i) {
                objects[i].marker = 100 + i;
                cell.m_objects.push_back(&objects[i]);
            }
            const std::size_t capacity = cell.m_objects.capacity();
            seed(cell.m_connection, connection);
            seed(cell.m_tile, tile);
            seed(cell.m_tileData, data);
            seed(cell.m_movement, state * 17u);
            seed(cell.m_zoneState, state * 19u);
            cell.m_previousTile.m_x = state;
            cell.m_previousTile.m_y = 7000 + state;
            cell.m_previousTile.m_z = 9000 + state;
            int oldDestroyed = destroyed;
            cell.clear();
            if (!cell.m_objects.empty() || cell.m_objects.capacity() != capacity || destroyed != oldDestroyed) return false;
            for (int i = 0; i < lengths[li]; ++i)
                if (objects[i].marker != 100 + i) return false;
            if (word(cell.m_connection) != (connection & ~1u) ||
                word(cell.m_tile) != ((tile & 0xc0000548u) | 0x548u) ||
                word(cell.m_tileData) != ((data & 0x01000000u) | 0x0a000000u) ||
                word(cell.m_movement) != 0x7fbc7fbcu ||
                word(cell.m_zoneState) != 0xffff7fbcu ||
                cell.m_previousTile.m_x != -1 ||
                cell.m_previousTile.m_y != int(7000 + state) ||
                cell.m_previousTile.m_z != int(9000 + state)) return false;
        }
    }
    return true;
}
int main() {
    if (sizeof(unsigned) != 4 || sizeof(TRmgMovementCost) != 4 ||
        sizeof(TRmgZoneCellState) != 4 || sizeof(TRmgGroundTile) != 4 ||
        sizeof(TRmgGroundTileData) != 4 || sizeof(TRmgConnectionDecoration) != 4) return 2;
''')
    for i in range(len(bodies)):
        program.append('if (' + ('!' if i < count else '') + 'check<Case' + str(i) + '::TRmgMapItem>()) '
                       '{ std::puts("failed case ' + str(i) + '"); return 1; }\n')
    program.append('std::puts("' + str(count) + ' cell reset forms: 16384 cases each; six negative controls rejected");}\n')
    with tempfile.TemporaryDirectory(prefix='rmg-clear-ownership-') as directory:
        path = Path(directory)
        (path / 'oracle.cpp').write_text(''.join(program))
        subprocess.run(['g++', '-std=c++98', '-O2', str(path / 'oracle.cpp'),
                        '-o', str(path / 'oracle')], check=True)
        subprocess.run([str(path / 'oracle')], check=True)


if __name__ == '__main__':
    main()
