#!/usr/bin/env python3
"""Recover the spell-box nested constructor expansion through initialization.

The adopted roster's first divergence is the (15000,1,5,4) spell box: retail
expands the ordinary treasure base constructor, while VC6 retains one call.
Keep the derived constructor inline, its value parameters, and the canonical
base construction. Compare member initializers and ordered body stores for
its three independent scalar fields. Retail stores minimum/maximum/mask at
0x14/0x18/0x1c. No Dreamcast counterpart was found. Measure all seven header
consumers, including the retained base constructor and the complete roster.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families


def constructor(header):
    start = header.index('    inline type_black_box_spells_def(')
    return header[start:header.index('\n    }', start) + len('\n    }')]


def variants(original):
    head = original[:original.index('        :')]
    fields = ('minimumLevel', 'maximumLevel', 'schoolMask')
    for mask in range(8):
        initialized = [field for index, field in enumerate(fields) if mask & (1 << index)]
        assigned = [field for field in fields if field not in initialized]
        for order in itertools.permutations(assigned):
            setup = '        : type_treasure_def(6, 0, value, 2)'
            setup += ''.join(',\n          m_' + field + '(' + field + ')' for field in initialized)
            stores = ''.join('        this->m_' + field + ' = ' + field + ';\n' for field in order)
            yield 'initializers_' + str(mask) + '+stores_' + '_'.join(order), head + setup + '\n    {\n' + stores + '    }'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = constructor((HOMM3_DIR / 'include/rmg.h').read_text())
    forms = list(variants(original))
    if forms[0][1] != original:
        raise ValueError('review original spell-box constructor')
    payload = dict(schema=1, units=['rmg', 'rmg_support', 'rmg_terrain', 'tiles',
                                  'singleselectionpopups', 'singleselectionwindow', 'scenarioinfo'],
                   evidence=__doc__, axes=[dict(name='spellbox_initialization', source='include/rmg.h',
                                               find=original, options=[dict(name=name, replace=body) for name, body in forms])])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(forms), 'canonical spell-box initialization forms across seven consumers')


if __name__ == '__main__':
    main()
