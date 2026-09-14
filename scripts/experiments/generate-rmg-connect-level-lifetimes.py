#!/usr/bin/env python3
"""Test meaningful level-query locals at canConnect's register boundary.

Retail 0x532bd0 reads the other level between slot-pointer and size loads,
then compares the levels before summing sizes. The current seven-block CFG
and sqrt/_ftol calls agree, but size/sum registers differ at +0x4f. Keep
the distance and size expressions; vary copied/borrowed level values or
a boolean predicate, their live ranges after sqrt, and comparison order.
There is no known Dreamcast counterpart. No invented helper or inline pin.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    condition = 'other->m_levelPosition.m_z != m_levelPosition.m_z'
    anchors = ('    int otherSize =', '    int thisSize =',
               '    int combinedSize =', '    if (' + condition + ')')
    if any(original.count(anchor) != 1 for anchor in anchors):
        raise ValueError('review changed connection locals before rebasing')
    yield 'original', original
    for mode, location, reverse in itertools.product(
            ('other_value', 'this_value', 'both_values', 'other_reference',
             'both_references', 'predicate'), range(4), range(2)):
        other, current = 'other->m_levelPosition.m_z', 'm_levelPosition.m_z'
        declarations = []
        if mode != 'predicate':
            kind = 'const int&' if 'reference' in mode else 'int'
            if mode != 'this_value':
                declarations.append('    ' + kind + ' otherLevel = ' + other + ';\n')
                other = 'otherLevel'
            if mode.startswith('both') or mode == 'this_value':
                declarations.append('    ' + kind + ' thisLevel = ' + current + ';\n')
                current = 'thisLevel'
        comparison = (current + ' != ' + other) if reverse else (other + ' != ' + current)
        if mode == 'predicate':
            declarations.append('    bool differentLevels = ' + comparison + ';\n')
            comparison = 'differentLevels'
        body = original.replace(anchors[location], ''.join(declarations) + anchors[location], 1)
        body = body.replace('    if (' + condition + ')', '    if (' + comparison + ')')
        yield '+'.join((mode, str(location), str(reverse))), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = generator('generate-rmg-connect-owners-family.py').definition(
        (HOMM3_DIR / 'src/rmg.cpp').read_text())
    axis = generator('generate-rmg-position-family.py').axis(
        'connection_level_lifetimes', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'level-query lifetime states')


if __name__ == '__main__':
    main()
