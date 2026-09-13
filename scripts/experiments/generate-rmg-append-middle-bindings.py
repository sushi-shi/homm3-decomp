#!/usr/bin/env python3
"""Test the middle setter's coordinate/receiver lifetimes without helper edits.

Retail keeps this in ECX through the one-level check and direct center
stores. The current inlined setter uses ECX for its coordinate destination,
requiring two extra moves. Explore scoped value/reference coordinate
arguments and borrowed receiver bindings at this single call. Compare
direct level subtraction with a seeded level updated by subtraction.
Preserve both corrected Y-reference rings, the by-value ordinary setter,
canonical constructors, cached center and all post-predicate getter calls.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    anchor = ('    int level = 1 - position.m_z;\n'
              '    candidate = TRmgMapPosition(position.m_x, position.m_y, level);\n'
              '    zone->setLevelPosition(candidate);\n')
    if original.count(anchor) != 1:
        raise ValueError('review current middle setter sequence')
    for coordinate, receiver, level_form in itertools.product(
            ('working', 'local_assigned', 'local_direct', 'const_value',
             'temporary_reference', 'local_copy', 'working_copy'),
            ('direct', 'reference', 'pointer', 'pointer_reference'),
            ('difference', 'seed_subtract')):
        level = ('    int level = 1 - position.m_z;\n' if level_form == 'difference'
                 else '    int level = 1;\n    level -= position.m_z;\n')
        value = 'opposite'
        if coordinate == 'working':
            construction = '    candidate = TRmgMapPosition(position.m_x, position.m_y, level);\n'
            value = 'candidate'
        elif coordinate == 'local_assigned':
            construction = '    TRmgMapPosition opposite;\n    opposite = TRmgMapPosition(position.m_x, position.m_y, level);\n'
        elif coordinate == 'local_direct':
            construction = '    TRmgMapPosition opposite(position.m_x, position.m_y, level);\n'
        elif coordinate == 'const_value':
            construction = '    const TRmgMapPosition opposite(position.m_x, position.m_y, level);\n'
        elif coordinate == 'temporary_reference':
            construction = '    const TRmgMapPosition& opposite = TRmgMapPosition(position.m_x, position.m_y, level);\n'
        elif coordinate == 'local_copy':
            construction = '    TRmgMapPosition opposite = position;\n    opposite.m_z = level;\n'
        else:
            construction = '    candidate = position;\n    candidate.m_z = level;\n'
            value = 'candidate'
        if receiver == 'direct':
            binding, destination = '', 'zone->'
        elif receiver == 'reference':
            binding, destination = '    TRmgZone& destination = *zone;\n', 'destination.'
        elif receiver == 'pointer':
            binding, destination = '    TRmgZone* const destination = zone;\n', 'destination->'
        else:
            binding, destination = '    TRmgZone* const& destination = zone;\n', 'destination->'
        middle = level + construction + binding + '    ' + destination + 'setLevelPosition(' + value + ');\n'
        yield '+'.join((coordinate, receiver, level_form)), original.replace(anchor, middle)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::appendZonePositions')
    axis = helper.axis('append_middle_bindings', 'src/rmg.cpp', original, variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'middle argument/receiver binding forms')


if __name__ == '__main__':
    main()
