#!/usr/bin/env python3
"""Test scalar constructor-input lifetimes in appendZonePositions.

The reproduced constructor/store path has retail's block and call positions,
but hoists X before Y and reserves a 0x48 rather than 0x3c frame. Retail's
first Y conversion uses the spent output-reference argument slot. Explore
named Y or Y/X integer and double inputs, including references to scalar
temporaries, independently in the first, last or both rings. Keep canonical
constructors, center snapshots, late radius reads and post-predicate copies.
Both the adopted Y/X/Z writes and the reproduced former X/Y/Z writes are
controls. No helper declaration/body or compiler option changes.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


POLICIES = (
    ('int_y', 'int', 'y'),
    ('const_int_y', 'const int', 'y'),
    ('reference_int_y', 'const int&', 'y'),
    ('double_y', 'double', 'y'),
    ('reference_double_y', 'const double&', 'y'),
    ('int_yx', 'int', 'yx'),
    ('reference_int_yx', 'const int&', 'yx'),
    ('double_yx', 'double', 'yx'),
    ('reference_double_yx', 'const double&', 'yx'),
)


def transform(original, scalar_type, fields, rings):
    body = original
    for index, level in enumerate(('position.m_z', 'level')):
        if rings == 'first' and index == 1 or rings == 'last' and index == 0:
            continue
        expressions = {c: 'position.m_' + c + ' + radius * g_rmgDirection' + table + '[direction]'
                       for c, table in (('x', 'Cosines'), ('y', 'Sines'))}
        arguments = {c: 'static_cast<int>(' + e + ')' for c, e in expressions.items()}
        anchor = '        candidate = TRmgMapPosition(\n            ' + arguments['x'] + ',\n            ' + arguments['y'] + ', ' + level + ');\n'
        if body.count(anchor) != 1:
            raise ValueError('review constructor input sites')
        declarations = ''
        for c in fields:
            floating = 'double' in scalar_type
            declarations += '        ' + scalar_type + ' ' + c + ' = ' + (expressions[c] if floating else arguments[c]) + ';\n'
            arguments[c] = 'static_cast<int>(' + c + ')' if floating else c
        replacement = declarations + '        candidate = TRmgMapPosition(\n            ' + arguments['x'] + ',\n            ' + arguments['y'] + ', ' + level + ');\n'
        body = body.replace(anchor, replacement)
    return body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::appendZonePositions')
    yxz = '        zone->m_levelPosition.m_y = candidate.m_y;\n        zone->m_levelPosition.m_x = candidate.m_x;\n'
    xyz = '        zone->m_levelPosition.m_x = candidate.m_x;\n        zone->m_levelPosition.m_y = candidate.m_y;\n'
    if original.count(yxz) != 2:
        raise ValueError('review adopted ring write order')
    parents = [('yxz', original), ('xyz', original.replace(yxz, xyz))]
    forms = list(parents)
    forms += [(parent + '+' + policy + '+' + rings, transform(body, scalar_type, fields, rings))
              for (parent, body), (policy, scalar_type, fields), rings in itertools.product(
                  parents, POLICIES, ('first', 'last', 'both'))]
    axis = helper.axis('append_scalar_inputs', 'src/rmg.cpp', original, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'scalar input lifetime forms')


if __name__ == '__main__':
    main()
