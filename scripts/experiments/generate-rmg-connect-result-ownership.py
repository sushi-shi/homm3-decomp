#!/usr/bin/env python3
"""Test computed sum/clearance ownership at canConnect's register exchange.

Retail computes the sum in EBX, selects the signed minimum in EDX, and
subtracts distance from EBX. Whole-zone receiver bindings preserve the old
split-load plateau. Retain their reproduced controls, then test immutable
sum values/temporary references and clearance lifetimes on either side of
the minimum selection. Keep the distance prefix and public ABI unchanged.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    minimum = ('        int minimumSize = thisSize;\n'
               '        if (otherSize < minimumSize)\n'
               '            minimumSize = otherSize;\n')
    tail = '        combinedSize -= distance;\n        return combinedSize > minimumSize / 2;\n'
    for size, total, clearance, early in itertools.product(
            ('int', 'const int&'), ('int', 'const int', 'const int&'),
            ('direct', 'int', 'const int', 'const int&'), (False, True)):
        if clearance == 'direct' and early:
            continue
        body = original.replace('    int otherSize =', '    ' + size + ' otherSize =')
        body = body.replace('    int combinedSize =', '    ' + total + ' combinedSize =')
        if clearance == 'direct':
            replacement = minimum + '        return combinedSize - distance > minimumSize / 2;\n'
        else:
            declaration = '        ' + clearance + ' clearance = combinedSize - distance;\n'
            replacement = (declaration + minimum if early else minimum + declaration)
            replacement += '        return clearance > minimumSize / 2;\n'
        if body.count(minimum + tail) != 1:
            raise ValueError('review changed connection minimum/clearance')
        body = body.replace(minimum + tail, replacement)
        yield '+'.join((size, total, clearance, 'early' if early else 'late')), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parents-from', type=Path, required=True)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    owner = generator('generate-rmg-connect-owners-family.py')
    original = owner.definition((HOMM3_DIR / 'src/rmg.cpp').read_text())
    context = args.parents_from.parent
    checkpoint = json.loads(args.parents_from.read_text())
    if checkpoint.get('generation', 0) < 1:
        raise ValueError('unfinished parent search')
    for relative in ('src/rmg.cpp', 'include/rmg.h'):
        if (context / 'snapshot' / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError('stale parent snapshot: ' + relative)
    _, originals, axes = source_families.load_manifest(context / 'input.json', HOMM3_DIR)
    forms = [('original', original)]
    for elite in checkpoint['elites']:
        expected = source_families.render(originals, axes, elite['choices'])['src/rmg.cpp']
        for attempt in ('first', 'repeat'):
            folder = context / 'candidates' / elite['id'] / attempt
            result = json.loads((folder / 'result.json').read_text())
            for key in ('id', 'choices', 'scores', 'object_hash', 'source_hashes'):
                if result[key] != elite[key]:
                    raise ValueError('parent reproduction mismatch: ' + key)
            if (folder / 'tree/src/rmg.cpp').read_text() != expected:
                raise ValueError('parent rendered source mismatch')
        forms.append(('parent_' + elite['id'], owner.definition(expected)))
    forms += list(variants(original))
    seen = set()
    unique = []
    for name, body in forms:
        if body not in seen:
            unique.append((name, body))
            seen.add(body)
    axis = helper.axis('connection_result_ownership', 'src/rmg.cpp', original, unique)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(unique), 'parent/computed-result ownership forms')


if __name__ == '__main__':
    main()
