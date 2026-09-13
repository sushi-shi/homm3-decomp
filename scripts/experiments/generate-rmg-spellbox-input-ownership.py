#!/usr/bin/env python3
"""Test input ownership at the single canonical inline spell-box constructor.

The roster has ten literal-only calls and no separately claimed retained
derived constructor. Those expansions expose the values but do not distinguish
value parameters from const-reference inputs. The ordinary four-int treasure
base ABI remains unchanged. Test all sixteen input ownership combinations in
the same inline declaration; keep its three body stores, all callers and the
seven-field object layout. The extra nested base call at roster +0x5e8 remains
the target; score all seven header consumers and verify actual constructors.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-spellbox-initialization.py')
    original = helper.constructor((HOMM3_DIR / 'include/rmg.h').read_text())
    arguments = 'int value, int minimumLevel, int maximumLevel, int schoolMask'
    if original.count(arguments) != 1:
        raise ValueError('review the current canonical input declaration')
    names = ('value', 'minimumLevel', 'maximumLevel', 'schoolMask')
    options = []
    for choices in itertools.product((0, 1), repeat=4):
        declared = ', '.join(('const int& ' if borrowed else 'int ') + name
                             for name, borrowed in zip(names, choices))
        options.append(dict(name='references_' + ''.join(str(c) for c in choices),
                            replace=original.replace(arguments, declared)))
    payload = dict(schema=1, units=['rmg', 'rmg_support', 'rmg_terrain', 'tiles',
                                  'singleselectionpopups', 'singleselectionwindow', 'scenarioinfo'],
                   evidence=__doc__, axes=[dict(name='spellbox_input_ownership', source='include/rmg.h',
                                               find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'canonical spell-box input ownership forms across seven consumers')


if __name__ == '__main__':
    main()
