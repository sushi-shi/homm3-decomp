#!/usr/bin/env python3
"""Recover refresh's table/current-pattern ownership without changing queries.

Retail 0x4f9f00 binds painter/point to EDI/ESI, then compares the selected
pattern in EDI directly against the table's memory entry. The current body
swaps the parameter homes and materializes that entry in EBX first. Keep
the selected-pattern copy after the virtual tile read and test actual table
pointer/reference ownership, scalar initialization and compared-entry
bindings. Preserve proxy/neighbour helpers, checked pattern selection and
the conditional random draw. No Dreamcast counterpart or inline qualifier.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    table = '    TRmgLinePatternTable* table = painter->getPattern(oldType);\n'
    pattern = '    int pattern = selected;\n'
    compare = '    if (table->m_patterns[current.getFrame()] != pattern\n'
    for owner, value, entry in itertools.product(
            ('pointer', 'const_pointer', 'reference', 'const_reference'),
            ('copy', 'const_value', 'direct', 'assigned', 'reference'),
            ('expression', 'value', 'reference')):
        body = original
        if value == 'const_value':
            declaration = '    const int pattern = selected;\n'
        elif value == 'direct':
            declaration = '    int pattern(selected);\n'
        elif value == 'assigned':
            declaration = '    int pattern;\n    pattern = selected;\n'
        elif value == 'reference':
            declaration = '    const int& pattern = selected;\n'
        else:
            declaration = pattern
        assert body.count(pattern) == 1
        body = body.replace(pattern, declaration)
        if entry != 'expression':
            kind = 'const int&' if entry == 'reference' else 'int'
            replacement = '    ' + kind + ' oldPattern = table->m_patterns[current.getFrame()];\n    if (oldPattern != pattern\n'
            assert body.count(compare) == 1
            body = body.replace(compare, replacement)
        if owner == 'const_pointer':
            body = body.replace(table, '    const TRmgLinePatternTable* table = painter->getPattern(oldType);\n')
        elif owner in ('reference', 'const_reference'):
            kind = 'const TRmgLinePatternTable&' if owner == 'const_reference' else 'TRmgLinePatternTable&'
            body = body.replace(table, '    ' + kind + ' table = *painter->getPattern(oldType);\n')
            body = body.replace('matches, table, selected', 'matches, &table, selected').replace('table->', 'table.')
        yield '+'.join((owner, value, entry)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg_terrain.cpp').read_text(), 'refreshRmgLinePoint')
    axis = helper.axis('refresh_pattern_values', 'src/rmg_terrain.cpp', original, variants(original))
    payload = dict(schema=1, units=['rmg_terrain'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'refresh table/pattern ownership controls')


if __name__ == '__main__':
    main()
