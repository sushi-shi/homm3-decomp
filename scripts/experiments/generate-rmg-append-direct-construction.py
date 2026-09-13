#!/usr/bin/env python3
"""Recover direct coordinate construction and Y-first evaluation in append.

The adopted ordinary-constructor path matches all 31 retail block boundaries
and 20 call positions, but has a 0x48 frame versus 0x3c and evaluates X before
Y. Test direct initialization instead of assignment from a temporary, with
zero/one/two staged scalar constructor inputs for each ring independently.
Every form retains the canonical constructor, first/middle/last geometry,
late radius reads, post-predicate getter and original push_back operations.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    outer = '    TRmgMapPosition candidate;\n    for (int direction = 0; direction < 32; ++direction) {\n'
    middle = '    candidate = TRmgMapPosition(position.m_x, position.m_y, level);\n'
    if original.count(outer) != 1 or original.count(middle) != 1:
        raise ValueError('review adopted coordinate lifetime')
    for lifetime, first_input, last_input in itertools.product(
            ('assigned', 'ring_direct', 'all_direct', 'first_middle_direct'),
            ('expressions', 'y_value', 'xy_values'), ('expressions', 'y_value', 'xy_values')):
        body = original
        if lifetime != 'assigned':
            body = body.replace(outer, '    for (int direction = 0; direction < 32; ++direction) {\n')
            if lifetime == 'ring_direct':
                body = body.replace(middle, '    TRmgMapPosition candidate;\n' + middle)
            else:
                body = body.replace(middle, '    TRmgMapPosition candidate(position.m_x, position.m_y, level);\n')
        for index, (level, inputs) in enumerate((('position.m_z', first_input), ('level', last_input))):
            x = 'static_cast<int>(position.m_x + radius * g_rmgDirectionCosines[direction])'
            y = 'static_cast<int>(position.m_y + radius * g_rmgDirectionSines[direction])'
            anchor = '        candidate = TRmgMapPosition(\n            ' + x + ',\n            ' + y + ', ' + level + ');\n'
            if body.count(anchor) != 1:
                raise ValueError('review ring constructor expression')
            prefix = ''
            if inputs != 'expressions':
                prefix += '        int y = ' + y + ';\n'
                y = 'y'
            if inputs == 'xy_values':
                prefix += '        int x = ' + x + ';\n'
                x = 'x'
            direct = lifetime != 'assigned' and (index == 0 or lifetime != 'first_middle_direct')
            start = '        TRmgMapPosition candidate(' if direct else '        candidate = TRmgMapPosition('
            body = body.replace(anchor, prefix + start + '\n            ' + x + ',\n            ' + y + ', ' + level + ');\n')
        yield '+'.join((lifetime, first_input, last_input)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::appendZonePositions')
    axis = helper.axis('append_direct_construction', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'direct/staged constructor forms')


if __name__ == '__main__':
    main()
