#!/usr/bin/env python3
"""Check rendered matching masks and ordered, short-circuited terrain queries."""
import argparse
from pathlib import Path
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def block(header, kind, name):
    start = header.index(kind + ' ' + name + ' {')
    return header[start:header.index('\n};', start) + 3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    args = parser.parse_args()
    _, originals, axes = load_manifest(args.manifest, HOMM3_DIR)
    if len(axes) != 1:
        raise ValueError('expected one matching-mask source axis')
    helper = generator('generate-rmg-position-family.py')
    bodies = [helper.definition(render(originals, axes, (i,))['src/rmg_terrain.cpp'],
                                'rmgTerrainPainter::buildMatchingNeighbourMask')
              for i in range(len(axes[0].options))]
    count = len(bodies)
    for old, new in (
            ('point.m_y < m_size.m_y - 1', 'point.m_y < m_size.m_x - 1'),
            ('int terrain = getTerrain(point);', 'int terrain = 0;'),
            ('matches[TILE_DIR_NORTH] || matches[TILE_DIR_WEST]', 'matches[TILE_DIR_NORTH] && matches[TILE_DIR_WEST]'),
            ('TRmgGridPoint(high.getX(), high.getY())', 'TRmgGridPoint(low.getX(), high.getY())'),
            ('matches[TILE_DIR_NORTH] = getTerrain(nearby) == terrain;', 'matches[TILE_DIR_NORTH] = getTerrain(nearby) != terrain;')):
        if bodies[0].count(old) != 1:
            raise ValueError('stale negative-control anchor: ' + old)
        bodies.append(bodies[0].replace(old, new))
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    tiles = (HOMM3_DIR / 'include/tiles.h').read_text()
    program = ['#include <vector>\n#include <utility>\n#include <cstdio>\n',
               block(header, 'struct', 'TRmgVector'), '\n', block(header, 'struct', 'TPoint'), '\n',
               'template<class Coordinate>\n', block(header, 'struct', 'TRmgGridPointT'), '\n',
               'typedef TRmgGridPointT<unsigned int> TRmgGridPoint;\n',
               block(tiles, 'enum', 'ETileDirection'), '\n', r'''
typedef std::pair<unsigned int, unsigned int> Query;
struct Source {
    TRmgGridPoint m_size;
    std::vector<int> cells;
    std::vector<Query> queries;
    bool invalid;
    int getTerrain(const TRmgGridPoint& point) {
        queries.push_back(Query(point.m_x, point.m_y));
        if (point.m_x >= m_size.m_x || point.m_y >= m_size.m_y) { invalid = true; return 999; }
        return cells[point.m_y * m_size.m_x + point.m_x];
    }
};
''']
    for i, body in enumerate(bodies):
        program += ['struct Painter' + str(i) + ' : Source {\n', body.replace('rmgTerrainPainter::', ''), '\n};\n']
    program.append(r'''
template<class Painter> bool check() {
    const unsigned sizes[] = {1, 2, 3, 5};
    const int terrains[] = {0, 3, 8};
    const int directions[] = {TILE_DIR_NORTH, TILE_DIR_SOUTH, TILE_DIR_WEST, TILE_DIR_EAST,
                             TILE_DIR_NORTHWEST, TILE_DIR_NORTHEAST, TILE_DIR_SOUTHWEST, TILE_DIR_SOUTHEAST};
    for (unsigned wi = 0; wi != 4; ++wi) for (unsigned hi = 0; hi != 4; ++hi) {
        unsigned width = sizes[wi], height = sizes[hi];
        for (unsigned x = 0; x < width; ++x) for (unsigned y = 0; y < height; ++y)
        for (unsigned pattern = 0; pattern != 512; ++pattern) for (unsigned ti = 0; ti != 3; ++ti) {
            Painter painter;
            painter.m_size = TRmgGridPoint(width, height);
            painter.invalid = false;
            painter.cells.resize(width * height);
            for (unsigned row = 0; row < height; ++row) for (unsigned column = 0; column < width; ++column)
                painter.cells[row * width + column] = (pattern & (1u << ((column + 3 * row) % 9)))
                    ? terrains[ti] : terrains[ti] + 1 + (row + column) % 3;
            unsigned north = y ? y - 1 : y, south = y + 1 < height ? y + 1 : y;
            unsigned west = x ? x - 1 : x, east = x + 1 < width ? x + 1 : x;
            Query positions[] = {Query(x, north), Query(x, south), Query(west, y), Query(east, y),
                                 Query(west, north), Query(east, north), Query(west, south), Query(east, south)};
            int center = painter.cells[y * width + x];
            unsigned char expected[8] = {0, 0, 0, 0, 0, 0, 0, 0};
            std::vector<Query> queries;
            queries.push_back(Query(x, y));
            for (int i = 0; i != 4; ++i) {
                queries.push_back(positions[i]);
                expected[directions[i]] = painter.cells[positions[i].second * width + positions[i].first] == center;
            }
            const int pairs[][2] = {{0, 2}, {0, 3}, {1, 2}, {1, 3}};
            for (int i = 0; i != 4; ++i) {
                if (expected[directions[pairs[i][0]]] || expected[directions[pairs[i][1]]]) {
                    queries.push_back(positions[i + 4]);
                    expected[directions[i + 4]] = painter.cells[positions[i + 4].second * width + positions[i + 4].first] == center;
                }
            }
            unsigned char guarded[10];
            for (int i = 0; i != 10; ++i) guarded[i] = 0x5a;
            const std::vector<int> before = painter.cells;
            TRmgGridPoint point(x, y);
            painter.buildMatchingNeighbourMask(point, guarded + 1);
            if (painter.invalid || painter.queries != queries || painter.cells != before ||
                point.m_x != x || point.m_y != y || painter.m_size.m_x != width || painter.m_size.m_y != height ||
                guarded[0] != 0x5a || guarded[9] != 0x5a) return false;
            for (int i = 0; i != 8; ++i) if (guarded[i + 1] != expected[i]) return false;
        }
    }
    return true;
}
int main() {
''')
    for i in range(len(bodies)):
        program.append('if (' + ('!' if i < count else '') + 'check<Painter' + str(i) + '>()) '
                       '{ std::puts("failed case ' + str(i) + '"); return 1; }\n')
    program.append('std::puts("' + str(count) + ' matching-mask forms: 185856 scenarios each; five negative controls rejected");}\n')
    with tempfile.TemporaryDirectory(prefix='rmg-matching-mask-') as directory:
        path = Path(directory)
        (path / 'oracle.cpp').write_text(''.join(program))
        subprocess.run(['g++', '-std=c++98', '-O2', str(path / 'oracle.cpp'), '-o', str(path / 'oracle')], check=True)
        subprocess.run([str(path / 'oracle')], check=True)


if __name__ == '__main__':
    main()
