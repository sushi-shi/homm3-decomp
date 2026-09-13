#!/usr/bin/env python3
"""Check actual refresh bodies against an independent ordered-callback model.

Uses authored mask and proxy methods, with controlled painter, pattern-selector
and random dependencies. Covers borrowed input mutation and selector-output
mutation during callbacks. This checks refresh semantics, not those dependencies.
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
enum { TILE_DIR_NORTH, TILE_DIR_NORTHEAST, TILE_DIR_EAST, TILE_DIR_SOUTHEAST,
       TILE_DIR_SOUTH, TILE_DIR_SOUTHWEST, TILE_DIR_WEST, TILE_DIR_NORTHWEST, TILE_DIR_COUNT };
struct TRmgGridPoint { unsigned m_x, m_y; };
@TILE@
struct TRmgLinePatternTable {
    int m_patterns[4];
    struct Range { unsigned m_firstIndex, m_valueCount; } m_ranges[4];
};
struct Event {
    int kind, x, y, value;
    Event(int k, int a, int b, int v) : kind(k), x(a), y(b), value(v) {}
    bool operator==(const Event& e) const {
        return kind == e.kind && x == e.x && y == e.y && value == e.value;
    }
};
static std::vector<Event> events;
static TRmgGridPoint* livePoint;
static int neighbourBits, mutation, mismatch, randomSeed, *selectedOutput;
static rmgTerrainTile currentTile, writtenTile;
static int writeCount;
static TRmgLinePatternTable table;
static void record(int kind, const TRmgGridPoint& p, int value = 0) {
    events.push_back(Event(kind, p.m_x, p.m_y, value));
}
static void moveInput() {
    livePoint->m_x = (livePoint->m_x + 1) % 9;
    livePoint->m_y = (livePoint->m_y + 2) % 9;
}
struct TRmgLinePainterTile;
struct TRmgLinePainterInterface {
    TRmgGridPoint m_size;
    TRmgLinePainterTile at(const TRmgGridPoint&);
    int getLand(const TRmgGridPoint& p) {
        record(1, p);
        if (mutation == 1) moveInput();
        return 7;
    }
    int getNeighbourLand(const TRmgGridPoint& p, unsigned d) {
        record(2, p, d);
        if (mutation == 2) moveInput();
        return neighbourBits & (1 << d) ? 7 : 9;
    }
    TRmgLinePatternTable* getPattern(int oldType) {
        events.push_back(Event(3, 0, 0, oldType));
        return &table;
    }
    void getTile(const TRmgGridPoint& p, rmgTerrainTile& result) {
        record(5, p);
        if (mutation == 3) *selectedOutput = (*selectedOutput + 1) % 4;
        result = currentTile;
    }
    void setTile(const TRmgGridPoint& p, const rmgTerrainTile& result) {
        record(7, p, result.m_frame);
        writtenTile = result;
        ++writeCount;
    }
};
struct TRmgLinePainterTile {
    TRmgLinePainterInterface* m_painter;
    TRmgGridPoint m_point;
    TRmgLinePainterTile(TRmgLinePainterInterface*, const TRmgGridPoint&);
    int getLand();
    void getTile(rmgTerrainTile&);
    void setTile(const rmgTerrainTile&);
};
@HELPERS@
static void selectRmgLinePattern(unsigned char* matches, TRmgLinePatternTable* which,
                                 int& selected, unsigned char& flipX, unsigned char& flipY) {
    int bits = 0;
    for (int d = 0; d < 8; ++d) bits |= int(matches[d] != 0) << d;
    events.push_back(Event(4, 0, 0, bits));
    selected = bits % 4;
    flipX = (bits >> 2) & 1;
    flipY = (bits >> 3) & 1;
    selectedOutput = &selected;
    currentTile.m_terrain = 77;
    currentTile.m_frame = mismatch == 1 ? (selected + 1) % 4 : selected;
    currentTile.m_flipX = flipX ^ (mismatch == 2);
    currentTile.m_flipY = flipY ^ (mismatch == 3);
}
static int controlledRandom() {
    events.push_back(Event(6, 0, 0, randomSeed));
    return randomSeed;
}
#define rand controlledRandom
@BODIES@
#undef rand
typedef void (*Refresh)(TRmgLinePainterInterface*, const TRmgGridPoint&);
static bool check(Refresh run) {
    for (int b = 0; b < 256; ++b) for (int y = 0; y <= 8; y += 4)
    for (int x = 0; x <= 8; x += 4) for (int mut = 0; mut < 4; ++mut)
    for (int wrong = 0; wrong < 4; ++wrong) for (int random = 0; random < 3; ++random) {
        neighbourBits = b; mutation = mut; mismatch = wrong; randomSeed = random * 17;
        TRmgGridPoint point = {unsigned(x), unsigned(y)};
        livePoint = &point;
        TRmgLinePainterInterface painter;
        painter.m_size.m_x = painter.m_size.m_y = 9;
        for (int i = 0; i < 4; ++i) {
            table.m_patterns[i] = i;
            table.m_ranges[i].m_firstIndex = 100 + i * 10;
            table.m_ranges[i].m_valueCount = 3 + i;
        }
        events.clear(); writeCount = 0;
        run(&painter, point);

        std::vector<Event> wanted;
        wanted.push_back(Event(1, x, y, 0));
        int qx = mut == 1 ? (x + 1) % 9 : x;
        int qy = mut == 1 ? (y + 2) % 9 : y;
        const int dx[8] = {0,1,1,1,0,-1,-1,-1};
        const int dy[8] = {-1,-1,0,1,1,1,0,-1};
        int allowed = 0, matched = 0;
        for (int d = 0; d < 8; ++d)
            if (qx + dx[d] >= 0 && qx + dx[d] < 9 && qy + dy[d] >= 0 && qy + dy[d] < 9)
                allowed |= 1 << d;
        for (int d = 0; d < 8; ++d) if (allowed & (1 << d)) {
            wanted.push_back(Event(2, qx, qy, d));
            if (b & (1 << d)) matched |= 1 << d;
            if (mut == 2) { qx = (qx + 1) % 9; qy = (qy + 2) % 9; }
        }
        wanted.push_back(Event(3, 0, 0, 7));
        wanted.push_back(Event(4, 0, 0, matched));
        wanted.push_back(Event(5, x, y, 0));
        int selected = matched % 4;
        int currentFrame = wrong == 1 ? (selected + 1) % 4 : selected;
        if (mut == 3) selected = (selected + 1) % 4;
        bool writes = currentFrame != selected || wrong == 2 || wrong == 3;
        int frame = 100 + selected * 10 + randomSeed % (3 + selected);
        if (writes) {
            wanted.push_back(Event(6, 0, 0, randomSeed));
            wanted.push_back(Event(7, x, y, frame));
        }
        if (events != wanted || point.m_x != unsigned(qx) || point.m_y != unsigned(qy)
            || writeCount != int(writes)) return false;
        if (writes && (writtenTile.m_terrain != 77 || writtenTile.m_frame != frame
            || writtenTile.m_flipX != ((matched >> 2) & 1)
            || writtenTile.m_flipY != ((matched >> 3) & 1))) return false;
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
    args = parser.parse_args()
    _, originals, axes = load_manifest(args.manifest, HOMM3_DIR)
    if len(axes) != 1:
        raise ValueError('this verifier expects one complete-function axis')
    helper = generator('generate-rmg-position-family.py')
    source = (HOMM3_DIR / 'src/rmg_terrain.cpp').read_text()
    methods = list(dict.fromkeys(helper.definition(render(originals, axes, (i,))['src/rmg_terrain.cpp'],
                        'refreshRmgLinePoint') for i in range(len(axes[0].options))))
    baseline = helper.definition(source, 'refreshRmgLinePoint')
    controls = [
        ('input_snapshot', '    TRmgLinePainterTile tile', '    const TRmgGridPoint inputCopy = point;\n    TRmgLinePainterTile tile'),
        ('reversed_mask', '== oldType;', '!= oldType;'),
        ('early_pattern', '    tile.getTile(current);\n    int pattern = selected;', '    int pattern = selected;\n    tile.getTile(current);'),
        ('wrong_range', 'rand() % table->m_ranges[pattern].m_valueCount', 'rand() % 1'),
        ('wrong_flip', 'current.m_flipY = flipY;', 'current.m_flipY = !flipY;'),
    ]
    bodies, checks = [], []
    for i, method in enumerate(methods):
        name = 'candidate' + str(i)
        bodies.append(method.replace('refreshRmgLinePoint(', name + '('))
        checks.append('    if (!check(' + name + ')) { std::fprintf(stderr, "failed ' + name + '\\n"); return 1; }')
    for name, before, after in controls:
        if baseline.count(before) != 1:
            raise ValueError('review control ' + name)
        method = baseline.replace(before, after)
        if name == 'input_snapshot':
            method = method.replace('point.m_', 'inputCopy.m_').replace('getNeighbourLand(point,', 'getNeighbourLand(inputCopy,')
        bodies.append(method.replace('refreshRmgLinePoint(', name + '('))
        checks.append('    if (check(' + name + ')) { std::fprintf(stderr, "missed ' + name + '\\n"); return 2; }')
    tile_header = (HOMM3_DIR / 'include/rmg_terrain.h').read_text()
    at = tile_header.index('struct rmgTerrainTile {')
    tile = tile_header[at:tile_header.index('\n};', at) + 3]
    helpers = [helper.definition(source, name) for name in (
        'TRmgLinePainterTile::TRmgLinePainterTile', 'TRmgLinePainterTile::getLand',
        'TRmgLinePainterTile::getTile', 'TRmgLinePainterTile::setTile', 'TRmgLinePainterInterface::at')]
    helpers.append(helper.definition((HOMM3_DIR / 'src/tiles.cpp').read_text(),
                                     'buildTileNeighbourMask').replace('__fastcall ', ''))
    program = FIXTURE.replace('@TILE@', tile).replace('@HELPERS@', '\n'.join(helpers))
    program = program.replace('@BODIES@', '\n'.join(bodies)).replace('@CHECKS@', '\n'.join(checks))
    with tempfile.TemporaryDirectory(prefix='rmg-refresh-scopes-') as directory:
        cpp = Path(directory) / 'check.cpp'
        exe = Path(directory) / 'check'
        cpp.write_text(program)
        subprocess.run(['g++', '-std=c++98', '-O2', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print(len(methods), 'source bodies x 110592 scenarios; five negative controls rejected')


if __name__ == '__main__':
    main()
