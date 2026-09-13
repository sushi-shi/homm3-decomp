#!/usr/bin/env python3
"""Test independently constructed ring and alternate-center coordinates.

The all-constructor path restores retail vector inlining but converts X
before Y. The earlier all-field path converts Y first but over-expands the
last insertion. Test the three placement stages independently, retaining
both XYZ and reproduced YXZ write parents and all ten store-order elites.
Field construction and copying the center before changing its level are
ordinary value operations; shared constructor/setter declarations and
bodies stay untouched. No padding, fake call or inline pragma is used.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def transform(original, first, last, middle):
    body = original
    for level, policy in (('position.m_z', first), ('level', last)):
        x = 'static_cast<int>(position.m_x + radius * g_rmgDirectionCosines[direction])'
        y = 'static_cast<int>(position.m_y + radius * g_rmgDirectionSines[direction])'
        anchor = '        candidate = TRmgMapPosition(\n            ' + x + ',\n            ' + y + ', ' + level + ');\n'
        if body.count(anchor) != 1:
            raise ValueError('review ring constructor sites')
        if policy != 'constructor':
            values = {'x': x, 'y': y, 'z': level}
            replacement = ''.join('        candidate.m_' + c + ' = ' + values[c] + ';\n'
                                  for c in policy)
            body = body.replace(anchor, replacement)
    anchor = '    candidate = TRmgMapPosition(position.m_x, position.m_y, level);\n'
    if body.count(anchor) != 1:
        raise ValueError('review alternate center')
    if middle == 'fields':
        body = body.replace(anchor, '    candidate.m_x = position.m_x;\n    candidate.m_y = position.m_y;\n    candidate.m_z = level;\n')
    elif middle == 'copy_level':
        body = body.replace(anchor, '    candidate = position;\n    candidate.m_z = level;\n')
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
    rows = json.loads((args.parent / 'checkpoint.json').read_text())['elites']
    parents = [('baseline', baseline)]
    for row in rows:
        repeated = json.loads((args.parent / 'candidates' / row['id'] / 'repeat/result.json').read_text())
        if row['scores'] != repeated['scores'] or row['object_hash'] != repeated['object_hash']:
            raise ValueError('parent has not reproduced')
        rendered = render(originals, axes, tuple(row['choices']))
        for name, text in rendered.items():
            if (args.parent / 'candidates' / row['id'] / 'repeat/tree' / name).read_text() != text:
                raise ValueError('reproduced source differs from rendered parent')
        parents.append((row['labels']['append_store_order'], helper.definition(rendered['src/rmg.cpp'], function)))
    selected = [parents[0], next(p for p in parents if p[0] == 'yxz+yxz')]
    forms = parents + [(label + '+' + first + '+' + last + '+' + middle,
                        transform(body, first, last, middle))
                       for (label, body), first, last, middle in itertools.product(selected,
                           ('constructor', 'yxz', 'xyz'), ('constructor', 'yxz', 'xyz'),
                           ('constructor', 'fields', 'copy_level'))]
    axis = helper.axis('append_mixed_construction', 'src/rmg.cpp', baseline, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'mixed construction forms including ten reproduced parents')


if __name__ == '__main__':
    main()
