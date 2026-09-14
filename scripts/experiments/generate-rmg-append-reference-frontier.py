#!/usr/bin/env python3
"""Recover vector inlining after the Y-reference conversion correction.

The reproduced XYZ/reference-int-Y/both parent matches the retail first
loop's instructions and 0x3c frame. It still expands the first vector size
at +0x1de, adding three blocks; the alternate-level slot is also different.
Test alternate-level bindings, working-coordinate lifetimes and final
accepted-value ownership. Carry all ten reproduced scalar-input parents,
including the higher-scoring double form whose spill contradicts retail.
All canonical helpers and the post-predicate getter remain in the source.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def transform(original, level_type, lifetime, accepted):
    body = original
    anchor = '    int level = 1 - position.m_z;\n'
    if body.count(anchor) != 1:
        raise ValueError('review alternate-level binding')
    body = body.replace(anchor, '    ' + level_type + ' level = 1 - position.m_z;\n')
    if lifetime != 'shared':
        anchor = '    TRmgMapPosition candidate;\n'
        if body.count(anchor) != 1:
            raise ValueError('review shared coordinate declaration')
        body = body.replace(anchor, '')
        body = body.replace('        candidate = TRmgMapPosition(\n', '        TRmgMapPosition candidate(\n')
        anchor = '    candidate = TRmgMapPosition(position.m_x, position.m_y, level);\n'
        if lifetime == 'ring_direct':
            replacement = '    TRmgMapPosition candidate;\n' + anchor
        else:
            replacement = '    TRmgMapPosition candidate(position.m_x, position.m_y, level);\n'
        if body.count(anchor) != 1:
            raise ValueError('review alternate-center construction')
        body = body.replace(anchor, replacement)
    if accepted != 'temporary':
        anchor = '        if (canPlaceZone(zone))\n            candidates.push_back(zone->getLevelPosition());\n'
        if body.count(anchor) != 2:
            raise ValueError('review accepted ring values')
        start = body.rindex(anchor)
        if accepted == 'working':
            value = 'candidate'
            statement = '            candidate = zone->getLevelPosition();\n'
        else:
            value = 'acceptedPosition'
            statement = '            ' + accepted + ' acceptedPosition = zone->getLevelPosition();\n'
        replacement = ('        if (canPlaceZone(zone)) {\n' + statement +
                       '            candidates.push_back(' + value + ');\n        }\n')
        body = body[:start] + body[start:].replace(anchor, replacement, 1)
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
    parents = []
    for row in rows:
        repeated = json.loads((args.parent / 'candidates' / row['id'] / 'repeat/result.json').read_text())
        if row['scores'] != repeated['scores'] or row['object_hash'] != repeated['object_hash']:
            raise ValueError('parent has not reproduced')
        rendered = render(originals, axes, tuple(row['choices']))
        for name, text in rendered.items():
            if (args.parent / 'candidates' / row['id'] / 'repeat/tree' / name).read_text() != text:
                raise ValueError('reproduced source differs from rendered parent')
        parents.append((row['labels']['append_scalar_inputs'], helper.definition(rendered['src/rmg.cpp'], function)))
    selected = next(body for name, body in parents if name == 'xyz+reference_int_y+both')
    forms = parents + [('+'.join((level, lifetime, accepted)), transform(selected, level, lifetime, accepted))
                       for level, lifetime, accepted in itertools.product(
                           ('int', 'const int', 'const int&'),
                           ('shared', 'ring_direct', 'all_direct'),
                           ('temporary', 'TRmgMapPosition', 'const TRmgMapPosition',
                            'const TRmgMapPosition&', 'working'))]
    axis = helper.axis('append_reference_frontier', 'src/rmg.cpp', baseline, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'reference-input frontier forms with ten reproduced parents')


if __name__ == '__main__':
    main()
