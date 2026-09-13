#!/usr/bin/env python3
"""Recover the randomized-priority expression after sharing the zone local.

The reproduced shared-zone parent restores EBP-0x10 and the retail receiver,
index and distance registers. In its non-unit-distance arm, retail reloads
the zone into ECX immediately after IDIV then computes the priority in EAX;
the parent computes it in ECX then reloads the zone into EDX. Test real
remainder/result lifetimes and commutative addition order with signed int
or long distance captures. Keep both conditional random-call sites and the
same stable insertion/placement phases. No helper or ABI changes.
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
    _, originals, axes = load_manifest(args.parent / 'input.json', HOMM3_DIR)
    for name, text in originals.items():
        if (args.parent / 'snapshot' / name).read_text() != text:
            raise ValueError('parent snapshot differs from current source')
    helper = generator('generate-rmg-position-family.py')
    function = 'type_random_map_generator::placeQuestGroup'
    baseline = helper.definition(originals['src/rmg.cpp'], function)
    rows = json.loads((args.parent / 'checkpoint.json').read_text())['elites']
    row = next(row for row in rows if row['labels']['quest_group_zone_lifetime'] == 'phases_012+after_vector')
    repeat = json.loads((args.parent / 'candidates' / row['id'] / 'repeat/result.json').read_text())
    if repeat['scores'] != row['scores'] or repeat['object_hash'] != row['object_hash']:
        raise ValueError('parent did not reproduce')
    parent = helper.definition(render(originals, axes, tuple(row['choices']))['src/rmg.cpp'], function)
    anchor = '        else\n            sharedZone->m_questPlacementScore = distance * 10 + rand() % 10;\n'
    if parent.count(anchor) != 1:
        raise ValueError('review priority expression')
    bodies = [
        ('expression', anchor),
        ('random_first', '        else\n            sharedZone->m_questPlacementScore = rand() % 10 + distance * 10;\n'),
        ('remainder', '        else {\n            int remainder = rand() % 10;\n            sharedZone->m_questPlacementScore = distance * 10 + remainder;\n        }\n'),
        ('remainder_first', '        else {\n            int remainder = rand() % 10;\n            sharedZone->m_questPlacementScore = remainder + distance * 10;\n        }\n'),
        ('priority', '        else {\n            int priority = distance * 10 + rand() % 10;\n            sharedZone->m_questPlacementScore = priority;\n        }\n'),
    ]
    forms = [('current', baseline)]
    for (expression, replacement), kind in itertools.product(bodies, ('int', 'const int', 'long', 'const long')):
        changed = parent.replace(anchor, replacement)
        changed = changed.replace('        int distance = sharedZone->m_questPlacementScore;',
                                  '        ' + kind + ' distance = sharedZone->m_questPlacementScore;')
        forms.append((expression + '+' + kind, changed))
    axis = helper.axis('quest_group_priority_frontier', 'src/rmg.cpp', baseline, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'shared-zone priority forms')


if __name__ == '__main__':
    main()
