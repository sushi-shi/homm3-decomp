#!/usr/bin/env python3
"""Test whole-zone receivers at the canConnect size-register mismatch.

Retail 0x532bd0 keeps otherSize in ECX and combinedSize in EBX; current VC6
exchanges them. Borrowing the scalar recovers those registers but splits the
field load. Earlier families bound slots and scalar values, not the two zone
objects themselves. Cross whole-zone references and pointer bindings with the
copied/borrowed other size. Preserve method cv/argument qualifiers, the exact
distance prefix expression, signed comparisons and branch-local minimum.
"""
import argparse
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    head, body = original.split('\n{\n', 1)
    policies = ('direct', 'reference', 'pointer', 'pointer_reference')
    for current, other, size in itertools.product(policies, policies, ('value', 'reference')):
        declarations = []
        result = body
        for name, policy, expression in (('source', current, 'this'), ('destination', other, 'other')):
            if policy == 'direct':
                continue
            if policy == 'reference':
                declarations.append('    const TRmgZone& ' + name + ' = *' + expression + ';\n')
                access = name + '.'
            else:
                kind = 'const TRmgZone* const&' if policy == 'pointer_reference' else 'const TRmgZone* const'
                declarations.append('    ' + kind + ' ' + name + ' = ' + expression + ';\n')
                access = name + '->'
            if name == 'destination':
                result = result.replace('other->', access)
            else:
                result = re.sub(r'(?<![.\w>])m_(levelPosition|slot)\b', lambda m: access + m[0], result)
        if size == 'reference':
            if result.count('    int otherSize =') != 1:
                raise ValueError('review current size binding')
            result = result.replace('    int otherSize =', '    const int& otherSize =')
        yield '+'.join((current, other, size)), head + '\n{\n' + ''.join(declarations) + result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'TRmgZone::canConnect')
    axis = helper.axis('connection_zone_bindings', 'src/rmg.cpp', original, variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'whole-zone/scalar binding forms')


if __name__ == '__main__':
    main()
