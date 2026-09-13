#!/usr/bin/env python3
"""Cross reproduced refresh input scopes with pattern comparison ownership.

Retail B5 compares the table memory entry directly with EDI. The candidate
first loads it into EBX and copies the selected pattern through EAX to EDI.
Test comparison operand direction and the signed int/long local domain with
each reproduced input-scope parent. VC6 int and long are both signed 32-bit;
the selected output remains int, read after the virtual current-tile query.
All helper declarations, query order and conditional random behavior stay.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('parent', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    payload, originals, axes = load_manifest(args.parent / 'input.json', HOMM3_DIR)
    for name, text in originals.items():
        if (args.parent / 'snapshot' / name).read_text() != text:
            raise ValueError('parent snapshot no longer agrees with current sources')
    helper = generator('generate-rmg-position-family.py')
    source = 'src/rmg_terrain.cpp'
    function = 'refreshRmgLinePoint'
    baseline = helper.definition(originals[source], function)
    parents = [('current', baseline)]
    for row in json.loads((args.parent / 'checkpoint.json').read_text())['elites']:
        repeat = json.loads((args.parent / 'candidates' / row['id'] / 'repeat/result.json').read_text())
        if row['scores'] != repeat['scores'] or row['object_hash'] != repeat['object_hash']:
            raise ValueError('parent reproduction does not agree')
        body = helper.definition(render(originals, axes, tuple(row['choices']))[source], function)
        parents.append((row['id'], body))
    forms = []
    for (parent, body), order, kind in itertools.product(parents, ('table_first', 'pattern_first'), ('int', 'long')):
        if body.count('    int pattern = selected;\n') != 1:
            raise ValueError('review selected-pattern lifetime')
        changed = body.replace('    int pattern = selected;\n', '    ' + kind + ' pattern = selected;\n')
        if order == 'pattern_first':
            before = 'table->m_patterns[current.getFrame()] != pattern'
            if changed.count(before) != 1:
                raise ValueError('review table comparison')
            changed = changed.replace(before, 'pattern != table->m_patterns[current.getFrame()]')
        forms.append(('+'.join((parent, order, kind)), changed))
    axis = helper.axis('refresh_comparison_frontier', source, baseline, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg_terrain'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'reproduced-parent comparison states')


if __name__ == '__main__':
    main()
