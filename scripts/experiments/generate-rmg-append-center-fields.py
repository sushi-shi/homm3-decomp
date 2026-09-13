#!/usr/bin/env python3
"""Recover the direct alternate-center fields without an intermediate value.

Retail writes the cached center's X/Y directly to the zone, then calculates
and stores its opposite level. The reproduced whole-coordinate copy path
retains the vector size call but clobbers a register; copying then changing
Z emits a redundant Z store absent from retail. Test direct X/Y writes and
early/late level declarations on both reproduced Y-reference input parents.
All shared helpers stay canonical, and both radial constructor calls and
the three post-predicate getters remain. Carry the ten setter elites.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def transform(original, level_type, order, timing):
    start = original.index('    int level = 1 - position.m_z;\n')
    stop = original.index('    if (canPlaceZone(zone))\n', start)
    level = '    ' + level_type + ' level = 1 - position.m_z;\n'
    copies = ''.join('    zone->m_levelPosition.m_' + c + ' = position.m_' + c + ';\n' for c in order)
    middle = (level + copies if timing == 'early' else copies + level)
    middle += '    zone->m_levelPosition.m_z = level;\n'
    return original[:start] + middle + original[stop:]


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
        parents.append((row['labels']['append_reference_setters'], helper.definition(rendered['src/rmg.cpp'], function)))
    selected = next(body for label, body in parents if label == 'xyz+reference_int_y+both+fields+setter+fields')
    variants = [('reference_y', selected)]
    # The second scalar-input parent's additional X references are a
    # separately reproduced source shape; retain the same ring write policy.
    other = next(body for label, body in parents if label == 'xyz+reference_int_yx+both+fields+assignment+assignment')
    last_assignment = '        zone->m_levelPosition = candidate;\n'
    if other.count(last_assignment) != 1:
        raise ValueError('review second parent last-ring destination')
    fields = ''.join('        zone->m_levelPosition.m_' + c + ' = candidate.m_' + c + ';\n' for c in 'xyz')
    variants.append(('reference_yx', other.replace(last_assignment, fields)))
    forms = parents + [('+'.join((label, level, order, timing)), transform(body, level, order, timing))
                       for (label, body), level, order, timing in itertools.product(variants,
                           ('int', 'const int', 'const int&'), ('xy', 'yx'), ('early', 'late'))]
    axis = helper.axis('append_center_fields', 'src/rmg.cpp', baseline, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'direct center-field forms with ten reproduced parents')


if __name__ == '__main__':
    main()
