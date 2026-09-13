#!/usr/bin/env python3
"""Refine the alternate-center copy after restoring append's vector calls.

The two reproduced 98.1270% parents use a whole assignment or the existing
setter for the middle position. Both recover the 0x3c frame, Y-first rings
and all twenty call decisions, but the middle copy clobbers ECX and adds
five bytes. Retail writes X/Y before calculating the opposite level.
Test direct/named constructor inputs and copying the cached center before
updating its level, with level declaration placement and constness. All
shared helper bodies remain untouched; all ten setter elites are controls.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def transform(original, level_type, form):
    start = original.index('    int level = 1 - position.m_z;\n')
    stop = original.index('    if (canPlaceZone(zone))\n', start)
    old = original[start:stop]
    setter = '    zone->setLevelPosition(candidate);\n' in old
    if not setter and '    zone->m_levelPosition = candidate;\n' not in old:
        raise ValueError('review middle destination')

    def write(value):
        return ('    zone->setLevelPosition(' + value + ');\n' if setter else
                '    zone->m_levelPosition = ' + value + ';\n')

    level = '    ' + level_type + ' level = 1 - position.m_z;\n'
    ctor = 'TRmgMapPosition(position.m_x, position.m_y, level)'
    if form == 'working':
        new = level + '    candidate = ' + ctor + ';\n' + write('candidate')
    elif form == 'direct_argument':
        new = level + write(ctor)
    elif form == 'const_value':
        new = level + '    const TRmgMapPosition opposite(position.m_x, position.m_y, level);\n' + write('opposite')
    elif form == 'copy_working':
        new = level + '    candidate = position;\n    candidate.m_z = level;\n' + write('candidate')
    elif form == 'copy_destination':
        new = level + write('position') + '    zone->m_levelPosition.m_z = level;\n'
    elif form == 'copy_then_level':
        new = write('position') + level + '    zone->m_levelPosition.m_z = level;\n'
    else:
        raise ValueError(form)
    return original[:start] + new + original[stop:]


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
    selected = [p for p in parents if p[0] in (
        'xyz+reference_int_y+both+fields+setter+fields',
        'xyz+reference_int_y+both+fields+assignment+fields')]
    if len(selected) != 2:
        raise ValueError('expected both reproduced middle-write parents')
    forms = parents + [('+'.join((label, level, form)), transform(body, level, form))
                       for (label, body), level, form in itertools.product(selected,
                           ('int', 'const int', 'const int&'),
                           ('working', 'direct_argument', 'const_value', 'copy_working',
                            'copy_destination', 'copy_then_level'))]
    axis = helper.axis('append_opposite_center', 'src/rmg.cpp', baseline, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'opposite-center forms with ten reproduced parents')


if __name__ == '__main__':
    main()
