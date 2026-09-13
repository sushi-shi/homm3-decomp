#!/usr/bin/env python3
"""Test translated coordinate inputs and returned-value lifetimes in operator+.

Retail markRiverCoastTarget (0x548a40) calls the ordinary three-int position
constructor twice on translated X/Y, at 0x548a75 and 0x548b2c. The current
canonical addition helper expands, including both nested constructors. Its
constructor owner and by-value TPoint interface remain unchanged. Compare
named scalar sums and returned coordinate lifetimes inside that one helper;
each form constructs the already translated coordinate exactly once. There is
no Dreamcast TRmgMapPosition counterpart. Score every RMG caller for collateral.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants():
    inputs = [('direct', '', 'm_x + offset.m_x', 'm_y + offset.m_y')]
    for kind, spelling in (('value', 'int'), ('constant', 'const int'),
                           ('reference', 'const int&')):
        for order in ('xy', 'yx'):
            declarations = ''.join('    ' + spelling + ' ' + field + ' = m_'
                                   + field + ' + offset.m_' + field + ';\n'
                                   for field in order)
            inputs.append((kind + '_' + order, declarations, 'x', 'y'))
    for kind, spelling in (('value', 'int'), ('reference', 'const int&')):
        for field in 'xy':
            declaration = ('    ' + spelling + ' ' + field + ' = m_' + field
                           + ' + offset.m_' + field + ';\n')
            inputs.append((kind + '_' + field, declaration,
                           'x' if field == 'x' else 'm_x + offset.m_x',
                           'y' if field == 'y' else 'm_y + offset.m_y'))
    for (label, declarations, x, y), returned in itertools.product(
            inputs, ('direct', 'named', 'copy_initialized', 'constant', 'reference')):
        arguments = x + ', ' + y + ', m_z'
        expression = 'TRmgMapPosition(' + arguments + ')'
        if returned == 'direct':
            body = '    return ' + expression + ';\n'
        elif returned == 'named':
            body = '    TRmgMapPosition result(' + arguments + ');\n    return result;\n'
        elif returned == 'constant':
            body = '    const TRmgMapPosition result(' + arguments + ');\n    return result;\n'
        else:
            kind = 'const TRmgMapPosition&' if returned == 'reference' else 'TRmgMapPosition'
            body = '    ' + kind + ' result = ' + expression + ';\n    return result;\n'
        yield label + '+' + returned, ('TRmgMapPosition TRmgMapPosition::operator+(TPoint offset) const\n{\n'
                                      + declarations + body + '}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'TRmgMapPosition::operator+')
    axis = helper.axis('position_add_inputs', 'src/rmg.cpp', original, variants())
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'coordinate-addition input/return forms')


if __name__ == '__main__':
    main()
