#!/usr/bin/env python3
"""Recover matching-mask point lifetimes while preserving ordered cache reads.

Retail 0x5b6540 retains center/cardinal reads and expands all diagonals, with
the south-east fill expanded. The prior function had these call decisions
but a 0x38 frame instead of 0x30 and different last-fill register ownership.
Earlier inliner evidence describes a cardinal-only point scope. Cross that
real lifetime with clamp-value ownership, corner construction and diagonal
temporary scope, keeping the center-first query and short-circuit order.
No Dreamcast counterpart; no helper changes, dummy work or inline pins.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    # Rebase the measured family after adopting copy-initialized corners and
    # the cardinal-only lifetime; keep that authored form as axis baseline.
    if '    {\n        TRmgGridPoint nearby;' in original:
        start = original.index('    {\n        TRmgGridPoint nearby;')
        end = original.index('    }\n    matches[TILE_DIR_NORTHWEST]', start)
        inner = original[start + len('    {\n'):end]
        original = original[:start] + ''.join(line[4:] for line in inner.splitlines(keepends=True)) + original[end + len('    }\n'):]
    for name, x, y in (('low', 'west', 'north'), ('high', 'east', 'south')):
        direct = '    TRmgGridPoint ' + name + '(' + x + ', ' + y + ');\n'
        original = original.replace('    TRmgGridPoint ' + name + ' = TRmgGridPoint(' + x + ', ' + y + ');\n', direct)
        original = original.replace('    TRmgGridPoint ' + name + ';\n    ' + name + ' = TRmgGridPoint(' + x + ', ' + y + ');\n', direct)
    for scope, clamp, corners, diagonals in itertools.product(
            ('function', 'cardinals'), ('value', 'const_value', 'const_reference'),
            ('direct', 'copy', 'assigned'), ('expression', 'named')):
        body = original
        if clamp != 'value':
            kind = 'const unsigned int&' if clamp == 'const_reference' else 'const unsigned int'
            for name in ('north', 'south', 'west', 'east'):
                body = body.replace('    unsigned int ' + name + ' =', '    ' + kind + ' ' + name + ' =', 1)
        if corners != 'direct':
            for name, x, y in (('low', 'west', 'north'), ('high', 'east', 'south')):
                old = '    TRmgGridPoint ' + name + '(' + x + ', ' + y + ');\n'
                if corners == 'copy':
                    new = '    TRmgGridPoint ' + name + ' = TRmgGridPoint(' + x + ', ' + y + ');\n'
                else:
                    new = '    TRmgGridPoint ' + name + ';\n    ' + name + ' = TRmgGridPoint(' + x + ', ' + y + ');\n'
                assert body.count(old) == 1
                body = body.replace(old, new)
        if scope == 'cardinals':
            start = body.index('    TRmgGridPoint nearby;')
            end = body.index('    matches[TILE_DIR_NORTHWEST]', start)
            body = body[:start] + '    {\n' + ''.join('    ' + line for line in body[start:end].splitlines(keepends=True)) + '    }\n' + body[end:]
        if diagonals == 'named':
            for direction, first, second, x, y in (
                    ('NORTHWEST', 'NORTH', 'WEST', 'low', 'low'),
                    ('NORTHEAST', 'NORTH', 'EAST', 'high', 'low'),
                    ('SOUTHWEST', 'SOUTH', 'WEST', 'low', 'high'),
                    ('SOUTHEAST', 'SOUTH', 'EAST', 'high', 'high')):
                condition = 'matches[TILE_DIR_' + first + '] || matches[TILE_DIR_' + second + ']'
                coordinate = x + '.getX(), ' + y + '.getY()'
                old = '    matches[TILE_DIR_' + direction + '] =\n        (' + condition + ')\n        && getTerrain(TRmgGridPoint(' + coordinate + ')) == terrain;\n'
                new = '    if (' + condition + ') {\n        TRmgGridPoint diagonal(' + coordinate + ');\n        matches[TILE_DIR_' + direction + '] = getTerrain(diagonal) == terrain;\n    } else {\n        matches[TILE_DIR_' + direction + '] = 0;\n    }\n'
                assert body.count(old) == 1
                body = body.replace(old, new)
        yield '+'.join((scope, clamp, corners, diagonals)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg_terrain.cpp').read_text(),
                                 'rmgTerrainPainter::buildMatchingNeighbourMask')
    axis = helper.axis('matching_mask_lifetimes', 'src/rmg_terrain.cpp', original, variants(original))
    payload = dict(schema=1, units=['rmg_terrain'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'matching-mask lifetime controls')


if __name__ == '__main__':
    main()
