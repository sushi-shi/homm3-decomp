#!/usr/bin/env python3
"""Test the canonical position-addition path for treasure guard construction.

Retail canPlaceTreasureGroup 0x546c70 retains TRmgMapPosition's constructor
at +0xbd; exposing its proven ordinary body in rmg.cpp currently expands it.
The existing position-plus-point helper constructs precisely this translated
coordinate. Test that source call and direct construction with real guard
point and returned-value lifetimes, keeping the canonical constructor's
signature, body and original TU location. Preserve shared workingPosition,
all guard checks, object-list timing, scans and placement/outline calls.
No Dreamcast counterpart is known. No alternate helper or inline pragma.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    anchor = ('        TPoint localGuard = group->m_guardPosition;\n'
              '        workingPosition = TRmgMapPosition(localGuard.m_x + position.m_x,\n'
              '            localGuard.m_y + position.m_y, position.m_z);\n')
    if original.count(anchor) != 1:
        raise ValueError('review changed guard construction')
    for guard, expression, result in itertools.product(
            ('copy', 'const_copy', 'reference', 'member'), ('constructor', 'addition'),
            ('assigned', 'copy_result', 'borrowed_result')):
        read = 'group->m_guardPosition' if guard == 'member' else 'localGuard'
        declarations = {'copy': 'TPoint', 'const_copy': 'const TPoint', 'reference': 'const TPoint&'}
        prefix = '' if guard == 'member' else '        ' + declarations[guard] + ' localGuard = group->m_guardPosition;\n'
        value = ('TRmgMapPosition(' + read + '.m_x + position.m_x,\n'
                 '            ' + read + '.m_y + position.m_y, position.m_z)')
        if expression == 'addition':
            value = 'position + ' + read
        if result == 'assigned':
            prefix += '        workingPosition = ' + value + ';\n'
        else:
            kind = 'TRmgMapPosition' if result == 'copy_result' else 'const TRmgMapPosition&'
            prefix += '        ' + kind + ' translatedGuard = ' + value + ';\n'
            prefix += '        workingPosition = translatedGuard;\n'
        yield '+'.join((guard, expression, result)), original.replace(anchor, prefix)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = generator('generate-rmg-position-family.py').definition(
        (HOMM3_DIR / 'src/rmg.cpp').read_text(), 'type_random_map_generator::canPlaceTreasureGroup')
    axis = generator('generate-rmg-position-family.py').axis(
        'group_guard_translation', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'canonical guard translation states')


if __name__ == '__main__':
    main()
