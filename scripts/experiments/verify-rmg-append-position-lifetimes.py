#!/usr/bin/env python3
"""Check append-position bodies against integer radial geometry and call order.

Uses the authored direction tables and zone-position accessor, native vector,
and a controlled placement predicate. Integer arithmetic independently models
the four-decimal direction coefficients in this bounded, non-overflowing domain.
Predicate mutations check snapshot timing and accepted-position reads; the
placement predicate's own terrain/spacing algorithm is outside this fixture.
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
struct TRmgMapPosition {
    int m_x, m_y, m_z;
    TRmgMapPosition() {}
    TRmgMapPosition(int, int, int);
};
static TRmgMapPosition pos(int x, int y, int z) { return TRmgMapPosition(x,y,z); }
static bool operator==(const TRmgMapPosition& a, const TRmgMapPosition& b) {
    return a.m_x == b.m_x && a.m_y == b.m_y && a.m_z == b.m_z;
}
struct TRmgTownSlot { int m_size; };
struct TRmgZone {
    TRmgTownSlot* m_slot;
    TRmgMapPosition m_levelPosition;
    TRmgMapPosition getLevelPosition() const;
    void setLevelPosition(TRmgMapPosition);
};
@HELPERS@
static bool accepts(int policy, int n) { return policy == 1 || (policy == 2 && n % 2 == 0) || (policy == 3 && n % 3 != 0); }
static TRmgMapPosition adjusted(TRmgMapPosition p, int mode, int n) {
    if (mode & 1) { p.m_x += n % 3 + 1; p.m_y -= n % 2 + 1; p.m_z = 1 - p.m_z; }
    return p;
}
struct Root {
    struct Map { TRmgMapPosition m_size; } m_map;
    TRmgZone* center;
    TRmgZone* expectedZone;
    int mode, policy, bad;
    std::vector<TRmgMapPosition> queries;
    unsigned char canPlaceZone(TRmgZone* zone) {
        if (zone != expectedZone) ++bad;
        int n = queries.size();
        queries.push_back(zone->m_levelPosition);
        zone->m_levelPosition = adjusted(zone->m_levelPosition, mode, n);
        if (mode & 2) {
            if (n == 0) {
                center->m_levelPosition.m_x += 100;
                center->m_levelPosition.m_y -= 100;
                center->m_slot->m_size += 5; zone->m_slot->m_size += 3;
            }
            if (n == 32) { center->m_slot->m_size += 2; zone->m_slot->m_size += 7; }
        }
        return accepts(policy, n);
    }
};
@BODIES@
static int coefficient(double x) { return int(x * 10000 + (x < 0 ? -0.5 : 0.5)); }
template<class Candidate> static bool check() {
    const int centers[] = {-17, 0, 31}, sizes[] = {0, 1, 7, 19};
    for (int ix = 0; ix < 3; ++ix) for (int iy = 0; iy < 3; ++iy)
    for (int cs = 0; cs < 4; ++cs) for (int zs = 0; zs < 4; ++zs)
    for (int z = 0; z < 2; ++z) for (int layers = 1; layers <= 2; ++layers)
    for (int policy = 0; policy < 4; ++policy) for (int mode = 0; mode < 4; ++mode)
    for (int prefix = 0; prefix <= 3; prefix += 3) {
        Candidate c;
        TRmgTownSlot centerSlot = {sizes[cs]}, zoneSlot = {sizes[zs]};
        TRmgZone center = {&centerSlot, pos(centers[ix], centers[iy], z)};
        TRmgZone zone = {&zoneSlot, pos(-100, -200, 0)};
        c.center = &center; c.expectedZone = &zone; c.bad = 0; c.mode = mode; c.policy = policy;
        c.m_map.m_size = pos(144, 144, layers);
        std::vector<TRmgMapPosition> result(prefix, pos(-90, 90, 2));
        c.appendZonePositions(&center, &zone, result);
        std::vector<TRmgMapPosition> wanted(prefix, pos(-90, 90, 2)), queries;
        const int count = layers == 1 ? 32 : 65;
        TRmgMapPosition last;
        for (int n = 0; n < count; ++n) {
            TRmgMapPosition p;
            if (n == 32) p = pos(centers[ix], centers[iy], 1 - z);
            else {
                int direction = n < 32 ? n : n - 33;
                int radius = n < 32 ? sizes[cs] + sizes[zs]
                    : std::max(sizes[cs] + ((mode & 2) ? 7 : 0), sizes[zs] + ((mode & 2) ? 10 : 0));
                p = pos((centers[ix] * 10000 + radius * coefficient(g_rmgDirectionCosines[direction])) / 10000,
                        (centers[iy] * 10000 + radius * coefficient(g_rmgDirectionSines[direction])) / 10000,
                        n < 32 ? z : 1 - z);
            }
            queries.push_back(p);
            last = adjusted(p, mode, n);
            if (accepts(policy, n)) wanted.push_back(last);
        }
        if (c.bad || c.queries != queries || result != wanted || !(zone.m_levelPosition == last)) return false;
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
    parser.add_argument('--snapshot', type=Path)
    args = parser.parse_args()
    _, originals, axes = load_manifest(args.manifest, args.snapshot or HOMM3_DIR)
    if len(axes) != 1:
        raise ValueError('expected one full-function axis')
    helper = generator('generate-rmg-position-family.py')
    function = 'type_random_map_generator::appendZonePositions'
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    baseline = helper.definition(originals['src/rmg.cpp'], function)
    methods = [helper.definition(render(originals, axes, (i,))['src/rmg.cpp'], function)
               for i in range(len(axes[0].options))]
    methods = list(dict.fromkeys([*methods, helper.definition(source, function)]))
    bodies, checks = [], []

    def add(name, body, valid):
        body = body.replace('type_random_map_generator::', name + '::')
        body = body.replace('    for (int direction = 0;', '    int direction;\n    for (direction = 0;', 1)
        bodies.append('struct ' + name + ' : Root { void appendZonePositions(TRmgZone*, TRmgZone*, std::vector<TRmgMapPosition>&); };\n' + body)
        checks.append('    if (' + ('!' if valid else '') + 'check<' + name + '>()) { std::fprintf(stderr, "failed ' + name + '\\n"); return 1; }')

    for i, body in enumerate(methods):
        add('Candidate' + str(i), body, True)
    controls = (
        ('WrongLevel', 'int level = 1 - position.m_z;', 'int level = position.m_z;'),
        ('WrongRadius', 'int radius = center->m_slot->m_size + zone->m_slot->m_size;', 'int radius = center->m_slot->m_size;'),
        ('WrongMaximum', 'if (radius < zone->m_slot->m_size)', 'if (radius > zone->m_slot->m_size)'),
        ('WrongAcceptedRead', 'candidates.push_back(zone->getLevelPosition());', 'candidates.push_back(candidate);'),
        ('WrongSnapshot', 'position.m_y + radius *', 'center->m_levelPosition.m_y + radius *'),
    )
    for name, before, after in controls:
        if before not in baseline:
            raise ValueError('review negative control ' + name)
        add(name, baseline.replace(before, after), False)
    helpers = [helper.definition(source, name) for name in
               ('TRmgMapPosition::TRmgMapPosition', 'TRmgZone::getLevelPosition', 'TRmgZone::setLevelPosition')]
    for name in ('g_rmgDirectionCosines', 'g_rmgDirectionSines'):
        at = source.index('double ' + name + '[32] = {')
        helpers.append(source[at:source.index('\n};', at) + 3])
    program = FIXTURE.replace('@HELPERS@', '\n'.join(helpers)).replace('@BODIES@', '\n'.join(bodies)).replace('@CHECKS@', '\n'.join(checks))
    with tempfile.TemporaryDirectory(prefix='rmg-append-positions-') as directory:
        cpp, exe = Path(directory) / 'check.cpp', Path(directory) / 'check'
        cpp.write_text(program)
        subprocess.run(['g++', '-std=c++98', '-O2', '-ffp-contract=off', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print(len(methods), 'bodies x 18432 scenarios; five negative controls rejected')


if __name__ == '__main__':
    main()
