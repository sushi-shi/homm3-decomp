#!/usr/bin/env python3
"""Test appendZonePositions' three placement phases and accepted-value lifetime.

Retail 0x53ae80 has a 0x3c frame versus candidate 0x4c and retains a size call
inside the last insertion. The early returned-coordinate copies already agree
apart from homes. Test working-coordinate lifetime across the two rings and
opposite-level center, the captured alternate level, and the final accepted
position's value/reference lifetime or public value-insert interface. Preserve
both earlier push_back calls, the post-canPlaceZone position read, Y-before-X
floating conversion, center snapshot, late radius reads and all helper bodies.
There is no known Dreamcast counterpart. No inline directives or pasted helpers.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    first = '    for (int direction = 0; direction < 32; ++direction) {\n        TRmgMapPosition candidate;\n'
    middle = '    int level = 1 - position.m_z;\n    TRmgMapPosition candidate;\n'
    last = '    for (direction = 0; direction < 32; ++direction) {\n'
    append = '        if (canPlaceZone(zone))\n            candidates.push_back(zone->getLevelPosition());\n'
    if original.count(first) != 1 or original.count(middle) != 1 or original.count(last) != 1 or original.count(append) != 2:
        raise ValueError('review the three placement phases')
    for working, level, accepted in itertools.product(
            ('current', 'shared_all', 'separate_all', 'shared_first_middle'),
            ('int', 'const int', 'const int&'),
            ('temporary', 'value', 'reference', 'working_value', 'value_insert')):
        body = original
        if working in ('shared_all', 'shared_first_middle'):
            body = body.replace(first, '    TRmgMapPosition candidate;\n    for (int direction = 0; direction < 32; ++direction) {\n')
            body = body.replace(middle, '    int level = 1 - position.m_z;\n')
        if working in ('separate_all', 'shared_first_middle'):
            body = body.replace(last, last + '        TRmgMapPosition candidate;\n')
        body = body.replace('    int level = 1 - position.m_z;', '    ' + level + ' level = 1 - position.m_z;')
        if accepted != 'temporary':
            if accepted == 'value_insert':
                replacement = '        if (canPlaceZone(zone))\n            candidates.insert(candidates.end(), zone->getLevelPosition());\n'
            else:
                if accepted == 'working_value':
                    statements = '            candidate = zone->getLevelPosition();\n            candidates.push_back(candidate);\n'
                else:
                    kind = 'TRmgMapPosition' if accepted == 'value' else 'const TRmgMapPosition&'
                    statements = '            ' + kind + ' accepted = zone->getLevelPosition();\n            candidates.push_back(accepted);\n'
                replacement = '        if (canPlaceZone(zone)) {\n' + statements + '        }\n'
            at = body.rindex(append)
            body = body[:at] + replacement + body[at + len(append):]
        yield '+'.join((working, level, accepted)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::appendZonePositions')
    axis = helper.axis('append_position_lifetimes', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'placement-phase and accepted-value forms')


if __name__ == '__main__':
    main()
