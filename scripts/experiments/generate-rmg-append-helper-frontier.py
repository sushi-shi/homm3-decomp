#!/usr/bin/env python3
"""Test canonical coordinate construction/setter calls in appendZonePositions.

The last insertion's retained _Ucopy body matches retail 0x434ba0, but its
caller expands the first copy and size differently. Earlier rings already
show the coordinate argument evaluation and field stores. Cross five
reproduced lifetime parents with the existing ordinary by-value zone setter
and three-coordinate constructor. Preserve all helper bodies/declarations,
the post-predicate read, registration order and all three insertion sites.
No new helper, inline keyword, pragma, or unused compiler-budget operation.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def transform(parent, setters, construction):
    body = parent
    stores = ('        zone->m_levelPosition.m_x = candidate.m_x;\n'
              '        zone->m_levelPosition.m_y = candidate.m_y;\n'
              '        zone->m_levelPosition.m_z = candidate.m_z;\n')
    middle = stores.replace('        ', '    ')
    if body.count(stores) != 2 or body.count(middle) != 1:
        raise ValueError('review coordinate setter sites')
    if setters in ('all', 'rings'):
        body = body.replace(stores, '        zone->setLevelPosition(candidate);\n')
    if setters in ('all', 'middle'):
        body = body.replace(middle, '    zone->setLevelPosition(candidate);\n')
    if construction != 'fields':
        for level in ('position.m_z', 'level'):
            anchor = ('        candidate.m_y = static_cast<int>(position.m_y + radius * g_rmgDirectionSines[direction]);\n'
                      '        candidate.m_x = static_cast<int>(position.m_x + radius * g_rmgDirectionCosines[direction]);\n'
                      '        candidate.m_z = ' + level + ';\n')
            if body.count(anchor) != 1:
                raise ValueError('review ring coordinate construction')
            if construction == 'constructor':
                replacement = ('        candidate = TRmgMapPosition(\n'
                    '            static_cast<int>(position.m_x + radius * g_rmgDirectionCosines[direction]),\n'
                    '            static_cast<int>(position.m_y + radius * g_rmgDirectionSines[direction]), ' + level + ');\n')
            else:
                replacement = ('        int y = static_cast<int>(position.m_y + radius * g_rmgDirectionSines[direction]);\n'
                    '        int x = static_cast<int>(position.m_x + radius * g_rmgDirectionCosines[direction]);\n'
                    '        candidate = TRmgMapPosition(x, y, ' + level + ');\n')
            body = body.replace(anchor, replacement)
        anchor = '    candidate.m_x = position.m_x;\n    candidate.m_y = position.m_y;\n    candidate.m_z = level;\n'
        if body.count(anchor) != 1:
            raise ValueError('review opposite-level center construction')
        body = body.replace(anchor, '    candidate = TRmgMapPosition(position.m_x, position.m_y, level);\n')
    return body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('parent', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    _, originals, axes = load_manifest(args.parent / 'input.json', HOMM3_DIR)
    for name, text in originals.items():
        if (args.parent / 'snapshot' / name).read_text() != text:
            raise ValueError('parent snapshot no longer matches current source')
    helper = generator('generate-rmg-position-family.py')
    function = 'type_random_map_generator::appendZonePositions'
    baseline = helper.definition(originals['src/rmg.cpp'], function)
    labels = ('baseline', 'separate_all+const int&+value', 'separate_all+const int&+reference',
              'shared_all+int+temporary', 'separate_all+const int+temporary')
    rows = json.loads((args.parent / 'checkpoint.json').read_text())['elites']
    parents = []
    for label in labels:
        row = next(row for row in rows if row['labels']['append_position_lifetimes'] == label)
        repeat = json.loads((args.parent / 'candidates' / row['id'] / 'repeat/result.json').read_text())
        if row['scores'] != repeat['scores'] or row['object_hash'] != repeat['object_hash']:
            raise ValueError('parent has not reproduced')
        parents.append((label, helper.definition(render(originals, axes, tuple(row['choices']))['src/rmg.cpp'], function)))
    forms = [(label + '+' + setters + '+' + construction, transform(body, setters, construction))
             for (label, body), setters, construction in itertools.product(parents,
                ('direct', 'all', 'rings', 'middle'), ('fields', 'constructor', 'coordinates'))]
    axis = helper.axis('append_helper_frontier', 'src/rmg.cpp', baseline, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'reproduced-parent canonical-helper forms')


if __name__ == '__main__':
    main()
