#!/usr/bin/env python3
"""Test line-refresh receiver aliases and neighbour-mask lifetimes.

Retail 0x4f9f00 retains the same eleven calls as the current body and binds
the input point/painter to ESI/EDI, opposite the candidate. Its masks are
consumed in sequence; only matches survives the neighbour loop. Test real
receiver/reference aliases and ending the availability lifetime there.
Keep the point borrowed across virtual callbacks, all query order, the
post-tile-read selected value, canonical helpers and their declarations.
No Dreamcast counterpart is known. No copied inputs or inline directives.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    head, body = original.split('\n{\n', 1)
    available = '    unsigned char available[TILE_DIR_COUNT];\n'
    matches = '    unsigned char matches[TILE_DIR_COUNT];\n'
    end = '    TRmgLinePatternTable* table = painter->getPattern(oldType);\n'
    for receiver, point, scope in itertools.product(
            ('parameter', 'constant_pointer', 'reference', 'after_tile'),
            ('parameter', 'reference', 'pointer'),
            ('original', 'matches_first', 'availability_block', 'loop_counter')):
        changed = body
        if scope != 'original':
            assert changed.count(matches) == 1
            changed = changed.replace(matches, '')
            if scope in ('matches_first', 'loop_counter'):
                changed = changed.replace(available, matches + available)
            else:
                start = changed.index(available)
                stop = changed.index(end)
                block = changed[start:stop]
                changed = (changed[:start] + matches + '    {\n'
                           + ''.join('    ' + line for line in block.splitlines(True))
                           + '    }\n' + changed[stop:])
        if scope == 'loop_counter':
            changed = changed.replace('    for (unsigned int direction = 0;',
                                      '    unsigned int direction;\n    for (direction = 0;')
        if point != 'parameter':
            # Only the body is changed: the evidenced fastcall ABI is fixed.
            if point == 'reference':
                changed = changed.replace('point', 'queryPoint')
                changed = '    const TRmgGridPoint& queryPoint = point;\n' + changed
            else:
                changed = changed.replace('point.m_', 'queryPoint->m_')
                changed = changed.replace('at(point)', 'at(*queryPoint)')
                changed = changed.replace('getNeighbourLand(point,', 'getNeighbourLand(*queryPoint,')
                changed = '    const TRmgGridPoint* const queryPoint = &point;\n' + changed
        if receiver != 'parameter':
            if receiver == 'after_tile':
                at = changed.index('    int oldType = tile.getLand();\n')
                changed = (changed[:at] + '    TRmgLinePainterInterface* const receiver = painter;\n'
                           + changed[at:].replace('painter->', 'receiver->'))
            elif receiver == 'constant_pointer':
                changed = ('    TRmgLinePainterInterface* const receiver = painter;\n'
                           + changed.replace('painter->', 'receiver->'))
            else:
                changed = ('    TRmgLinePainterInterface& receiver = *painter;\n'
                           + changed.replace('painter->', 'receiver.'))
        yield '+'.join((receiver, point, scope)), head + '\n{\n' + changed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg_terrain.cpp').read_text(), 'refreshRmgLinePoint')
    axis = helper.axis('refresh_input_scopes', 'src/rmg_terrain.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg_terrain'],
                                          evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'borrowed-input/mask-scope states')


if __name__ == '__main__':
    main()
