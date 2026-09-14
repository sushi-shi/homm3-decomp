#!/usr/bin/env python3
"""Recover player-array lifetimes around map-header team serialization.

After selector recovery retail places teams at EBP-0x30, human eligibility
at -0x48 and computer eligibility at -0x50; the candidate uses -0x4c, -0x30
and -0x1c respectively. Test real array declaration ordering, the geometry
records' last-use scope and the team array's lifetime. Keep all zero fills,
player queries, stream writes and team-assignment calls in their original
order; never change array layout or add optimizer padding.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

DECLARATIONS = {
    'human': 'unsigned char canBeHuman[8];',
    'computer': 'unsigned char canBeComputer[8];',
    'alignments': 'int legalAlignments[8];',
    'towns': 'TRmgMapPosition mainTowns[8];',
}
ORDERS = (
    ('human', 'alignments', 'towns', 'computer'),
    ('human', 'computer', 'alignments', 'towns'),
    ('alignments', 'towns', 'human', 'computer'),
    ('towns', 'alignments', 'human', 'computer'),
)


def variants(original):
    start = original.index('    {\n        unsigned char canBeHuman[8];')
    end = original.index('\n    if (m_mapVersion >= 1) {\n        std::bitset<156> availableHeroes;', start)
    region = original[start:end]
    assert region.endswith('    }\n')
    old_declarations = ''.join('        ' + DECLARATIONS[key] + '\n' for key in ORDERS[0])
    for order, scope, team_scope in itertools.product(
            ORDERS, ('players', 'geometry', 'function'),
            ('selection', 'first', 'after_human', 'after_computer', 'before_records')):
        body = region
        assert body.count(old_declarations) == 1
        body = body.replace(old_declarations, ''.join('        ' + DECLARATIONS[key] + '\n' for key in order))
        if team_scope != 'selection':
            assert body.count('            char teams[8];\n') == 1
            body = body.replace('            char teams[8];\n', '')
            if team_scope == 'first':
                body = body.replace('    {\n', '    {\n        char teams[8];\n', 1)
            elif team_scope in ('after_human', 'after_computer'):
                old = '        ' + DECLARATIONS[team_scope[len('after_'):]] + '\n'
                body = body.replace(old, old + '        char teams[8];\n', 1)
            else:
                old = '        for (int serializedPlayer = 0;'
                assert body.count(old) == 1
                body = body.replace(old, '        char teams[8];\n' + old)
        if scope == 'geometry':
            # The eligibility arrays survive through assignRmgTeams; geometry
            # arrays are only needed to prepare and serialize player records.
            geometry = ''.join('            ' + DECLARATIONS[key] + '\n' for key in order if key in ('alignments', 'towns'))
            for key in ('alignments', 'towns'):
                body = body.replace('        ' + DECLARATIONS[key] + '\n', '', 1)
            # A hoisted team array must stay visible after the geometry block.
            if team_scope == 'before_records':
                body = body.replace('        char teams[8];\n', '', 1)
                body = body.replace('    {\n', '    {\n        char teams[8];\n', 1)
            a = body.index('        memset(canBeHuman,')
            b = body.index('        if (!m_computerTeamCount)', a)
            body = body[:a] + '        {\n' + geometry + ''.join('    ' + line if line.strip() else line for line in body[a:b].splitlines(keepends=True)) + '        }\n' + body[b:]
        elif scope == 'function':
            body = ''.join(line[4:] if line.startswith('    ') else line for line in body[len('    {\n'):-len('    }\n')].splitlines(keepends=True))
        yield '+'.join((','.join(order), scope, team_scope)), original[:start] + body + original[end:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'type_random_map_generator::writeMapHeader')
    axis = helper.axis('header_player_lifetimes', 'src/rmg.cpp', original, variants(original))
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'header player-array lifetime controls')


if __name__ == '__main__':
    main()
