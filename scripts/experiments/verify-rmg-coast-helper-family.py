#!/usr/bin/env python3
"""Check helper-changing families with the independent affine coast oracle.

Each rendered source gets its own namespace containing the actual coordinate
types and helpers. This is necessary when the manifest changes operator+:
checking only the coastal caller under the current helper would miss the edit.
"""
import argparse
import re
import subprocess
import tempfile
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifests', type=Path, nargs='+')
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    old_family = generator('generate-rmg-river-coast-family.py')
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    terrain = (HOMM3_DIR / 'include/terrain_type.h').read_text()
    def block(name):
        start = header.index('struct ' + name + ' {')
        return header[start:header.index('\n};', start) + 3]
    types = '\n'.join(block(n) for n in ('TRmgVector', 'TPoint', 'TRmgMapPosition',
                                         'TRmgGroundTile', 'TRmgGroundTileData'))
    types += '\nenum { eTerrainWater = ' + re.search(r'eTerrainWater\s*=\s*(\d+)', terrain)[1] + ' };'
    template = (HOMM3_DIR / 'scripts/experiments/rmg-river-coast-oracle.cpp').read_text()
    includes = '\n'.join(re.findall(r'^#include[^\n]+', template, re.M))
    template = re.sub(r'^#include[^\n]+\n', '', template, flags=re.M).replace('int main()', 'int run()')
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    bodies = [source]
    for manifest in args.manifests:
        _, originals, axes = source_families.load_manifest(manifest, HOMM3_DIR)
        if len(axes) != 1 or set(originals) != {'src/rmg.cpp'}:
            raise ValueError('expected one RMG source-only helper axis')
        bodies.extend(source_families.render(originals, axes, (i,))['src/rmg.cpp']
                      for i in range(len(axes[0].options)))
    bodies = list(dict.fromkeys(bodies))
    units, calls = [], []
    for index, body in enumerate(bodies):
        coordinate = helper.definition(body, 'TRmgMapPosition::TRmgMapPosition')
        coordinate += '\n' + '\n'.join(helper.definition(body, 'TRmgMapPosition::operator' + op)
                                        for op in ('+', '+='))
        methods, checks = [], []
        def candidate(name, text, good):
            if 'for (count = 0;' in text and '    int count;' not in text:
                text = text.replace('    for (int count = 0;', '    int count;\n    for (count = 0;', 1)
            methods.append('struct ' + name + ' : Root { void markRiverCoastTarget(TRmgMapPosition, int); };\n'
                           + text.replace('type_random_map_generator::', name + '::'))
            checks.append('if (' + ('!' if good else '') + 'check<' + name + '>()) return 1;')
        candidate('Candidate', helper.definition(body, 'type_random_map_generator::markRiverCoastTarget'), True)
        if index == 0:
            for name, before, after in (
                    ('Water', '!= eTerrainWater', '== eTerrainWater'),
                    ('Entrance', 'item->isRoadEntrance()', 'false'),
                    ('Inland', 'count < 4', 'count < 3'),
                    ('Shore', 'count < 3', 'count < 2'),
                    ('Bound', 'point.m_x > m_map.m_size.m_x', 'point.m_x >= m_map.m_size.m_x'),
                    ('Direction', 'direction - 4', 'direction - 2'),
                    ('Target', 'm_riverTarget = 1', 'm_hasRiver = 1')):
                candidate('Wrong' + name, old_family.baseline().replace(before, after), False)
        direction = re.search(r'TPoint g_rmgDirections\[[^]]+\] = \{.*?\n};', body, re.S)[0].replace('RMG_DIRECTION_COUNT', '8')
        program = template
        for marker, value in (
                ('TYPES', types), ('HELPERS', coordinate), ('DIRECTIONS', direction),
                ('ENTRANCE', re.search(r'unsigned char isRoadEntrance\(\) const\s*\{[^}]+}', header)[0]),
                ('ACCESSOR', re.search(r'inline TRmgMapItem\* getMapItem\(int x, int y, int z\)\s*\{[^}]+}', header)[0]),
                ('CANDIDATES', '\n'.join(methods)), ('CHECKS', '\n'.join(checks))):
            program = program.replace('// @' + marker + '@', value)
        units.append('namespace Case' + str(index) + ' {\n' + program + '\n}\n')
        calls.append('if (Case' + str(index) + '::run()) { std::fprintf(stderr, "failed case '
                     + str(index) + '\\n"); return 1; }')
    program = includes + '\n' + '\n'.join(units) + '\nint main() {\n' + '\n'.join(calls) + '\n}\n'
    with tempfile.TemporaryDirectory(prefix='rmg-coast-helpers-') as raw:
        path = Path(raw) / 'oracle.cpp'
        path.write_text(program)
        executable = Path(raw) / 'oracle'
        subprocess.run(['g++', '-std=c++98', '-O1', '-fno-elide-constructors', str(path), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    print(len(bodies), 'rendered helper/caller sources passed 1800 affine scenarios each; seven incorrect controls rejected')


if __name__ == '__main__':
    main()
