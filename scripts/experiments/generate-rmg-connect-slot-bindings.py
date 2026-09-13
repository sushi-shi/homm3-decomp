#!/usr/bin/env python3
"""Cross slot-object bindings with the measured canConnect scalar bindings.

Retail 0x532bd0 loads both slot pointers after sqrt/_ftol and loads otherSize
directly from [ecx+8]. Borrowing otherSize recovers ECX but previously splits
that load into address formation and indirection. Earlier slot-pointer and
slot-reference controls used copied scalar sizes only. Cross the two actual
slot owners independently with copied/borrowed scalar sizes, preserving the
distance, branch-local minimum, sum and signed predicates. This Complete-only
method has no Dreamcast counterpart. No extra helpers or source-only noise.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    old = '    int otherSize = other->m_slot->m_size;\n    int thisSize = m_slot->m_size;\n'
    if original.count(old) != 1:
        raise ValueError('review the current size declarations before rebasing')
    for other_owner, this_owner, borrowed in itertools.product(
            ('direct', 'pointer', 'reference'), ('direct', 'pointer', 'reference'), range(4)):
        lines, reads = [], []
        for name, owner, expression in (('otherSlot', other_owner, 'other->m_slot'),
                                        ('thisSlot', this_owner, 'm_slot')):
            if owner == 'pointer':
                lines.append('    const TRmgTownSlot* ' + name + ' = ' + expression + ';\n')
                reads.append(name + '->m_size')
            elif owner == 'reference':
                lines.append('    const TRmgTownSlot& ' + name + ' = *' + expression + ';\n')
                reads.append(name + '.m_size')
            else:
                reads.append(expression + '->m_size')
        for index, (name, read) in enumerate(zip(('otherSize', 'thisSize'), reads)):
            kind = 'const int&' if borrowed & (1 << index) else 'int'
            lines.append('    ' + kind + ' ' + name + ' = ' + read + ';\n')
        yield '+'.join((other_owner, this_owner, str(borrowed))), original.replace(old, ''.join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = generator('generate-rmg-connect-owners-family.py').definition((HOMM3_DIR / 'src/rmg.cpp').read_text())
    axis = generator('generate-rmg-position-family.py').axis(
        'connection_slot_bindings', 'src/rmg.cpp', original, variants(original))
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'slot/scalar ownership controls')


if __name__ == '__main__':
    main()
