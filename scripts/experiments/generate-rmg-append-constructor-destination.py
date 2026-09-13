#!/usr/bin/env python3
"""Test coordinate temporary ownership after the append store-order frontier.

Retail still converts Y first and uses a 0x3c frame. Both reproduced XYZ
and YXZ store parents convert X first with a 0x48 frame. Test direct local
construction, temporary reference lifetime, and passing the constructed
coordinate directly to its existing destination/setter. Preserve the
canonical ordinary constructor and setter, all three accepted-value reads,
the cached center coordinate and the late radius maximum. Carry all ten
reproduced store-order elites as controls before exploring two parents.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def transform(original, first, last):
    body = original
    for level, policy in (('position.m_z', first), ('level', last)):
        expression = ('TRmgMapPosition(\n'
            '            static_cast<int>(position.m_x + radius * g_rmgDirectionCosines[direction]),\n'
            '            static_cast<int>(position.m_y + radius * g_rmgDirectionSines[direction]), ' + level + ')')
        constructor = '        candidate = ' + expression + ';\n'
        if body.count(constructor) != 1:
            raise ValueError('review ring constructor sites')
        start = body.index(constructor)
        stop = body.index('        if (canPlaceZone(zone))\n', start)
        stores = body[start + len(constructor):stop]
        if policy == 'working':
            continue
        if policy == 'direct_local':
            replacement = '        TRmgMapPosition candidate' + expression[len('TRmgMapPosition'):] + ';\n' + stores
        elif policy == 'temporary_reference':
            replacement = '        const TRmgMapPosition& candidate = ' + expression + ';\n' + stores
        elif policy == 'destination':
            replacement = '        zone->m_levelPosition = ' + expression + ';\n'
        elif policy == 'setter':
            replacement = '        zone->setLevelPosition(' + expression + ');\n'
        else:
            raise ValueError(policy)
        body = body[:start] + replacement + body[stop:]
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
        repeat = json.loads((args.parent / 'candidates' / row['id'] / 'repeat/result.json').read_text())
        if row['scores'] != repeat['scores'] or row['object_hash'] != repeat['object_hash']:
            raise ValueError('parent has not reproduced')
        rendered = render(originals, axes, tuple(row['choices']))
        for name, text in rendered.items():
            if (args.parent / 'candidates' / row['id'] / 'repeat/tree' / name).read_text() != text:
                raise ValueError('reproduced source differs from rendered parent')
        parents.append((row['labels']['append_store_order'], helper.definition(rendered['src/rmg.cpp'], function)))
    controls = list(parents)
    selected = [parents[0], next(p for p in parents if p[0] == 'yxz+yxz')]
    policies = ('working', 'direct_local', 'temporary_reference', 'destination', 'setter')
    forms = controls + [(label + '+' + first + '+' + last, transform(body, first, last))
                       for (label, body), first, last in itertools.product(selected, policies, policies)]
    axis = helper.axis('append_constructor_destination', 'src/rmg.cpp', baseline, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'constructor destination forms including ten reproduced parents')


if __name__ == '__main__':
    main()
