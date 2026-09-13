#!/usr/bin/env python3
"""Test existing zone-size calls and scalar ownership in canConnect.

Retail 0x532bd0 reads both slot sizes after sqrt/_ftol. Its seven-block CFG
and calls agree with the candidate, but otherSize/combinedSize exchange ECX
and EBX. Prior direct-field borrowing recovers those homes with an unwanted
split field load. Test the canonical getSize value accessor independently
on both receivers, including lifetime-extended returned scalars, with the
two real size-query and sum orders. No Dreamcast counterpart is known.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    anchor = ('    int otherSize = other->m_slot->m_size;\n'
              '    int thisSize = m_slot->m_size;\n'
              '    int combinedSize = thisSize + otherSize;\n')
    if original.count(anchor) != 1:
        raise ValueError('review changed size ownership before rebasing')
    for other, current, order, addition in itertools.product(range(4), range(4), range(2), range(2)):
        lines = []
        for name, mode, receiver in (('otherSize', other, 'other->'),
                                     ('thisSize', current, '')):
            kind = 'const int&' if mode % 2 else 'int'
            read = receiver + ('getSize()' if mode >= 2 else 'm_slot->m_size')
            lines.append('    ' + kind + ' ' + name + ' = ' + read + ';\n')
        if order:
            lines.reverse()
        operands = 'otherSize + thisSize' if addition else 'thisSize + otherSize'
        lines.append('    int combinedSize = ' + operands + ';\n')
        yield '+'.join(map(str, (other, current, order, addition))), original.replace(anchor, ''.join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = generator('generate-rmg-connect-owners-family.py').definition(
        (HOMM3_DIR / 'src/rmg.cpp').read_text())
    axis = generator('generate-rmg-position-family.py').axis(
        'connection_accessor_bindings', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'accessor/scalar ownership states')


if __name__ == '__main__':
    main()
