#!/usr/bin/env python3
"""Recover map-header team selector operands and argument ownership.

Retail's two upper-bound selectors compare player counts against team counts
in the opposite operand order to the current candidate and exchange their
two stack homes. The three lower-bound selectors already have the correct
comparison shape. Test both min operand orders independently, canonical
reference versus value selectors, and copied/immutable/direct field inputs.
All field updates and the later team assignment remain in their original
order. This Complete-only map writer has no Dreamcast counterpart.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    # Preserve the adopted direct-wrapper source as baseline and retain the
    # former copied-reference form as a reproducible control when rebasing.
    if 'm_humanTeamCount = min(' in original:
        for side, limit in (('human', 2), ('human', 1), ('computer', 1)):
            field = 'm_' + side + 'TeamCount'
            indent = '            ' if limit == 2 else '                '
            old = indent + field + ' = max(' + field + ', ' + str(limit) + ');'
            new = indent + 'int teamCount = ' + field + ';\n' + indent + field + ' = std::_cpp_max(teamCount, ' + str(limit) + ');'
            assert original.count(old) == 1
            original = original.replace(old, new)
        for side in ('human', 'computer'):
            team, player = 'm_' + side + 'TeamCount', 'm_' + side + 'PlayerCount'
            old = '                ' + team + ' = min(' + team + ', ' + player + ');'
            new = '                int playerCount = ' + player + ';\n                int teamCount = ' + team + ';\n                ' + team + ' = std::_cpp_min(playerCount, teamCount);'
            assert original.count(old) == 1
            original = original.replace(old, new)
    for selectors, captures, human, computer in itertools.product(
            ('reference', 'value'), ('copies', 'const_copies', 'fields'), (False, True), (False, True)):
        body = original
        for side, reverse in (('human', human), ('computer', computer)):
            member = 'm_' + side + 'TeamCount'
            old = member + ' = std::_cpp_min(playerCount, teamCount);'
            new = member + ' = std::_cpp_min(' + ('teamCount, playerCount' if reverse else 'playerCount, teamCount') + ');'
            assert body.count(old) == 1
            body = body.replace(old, new)
        if captures == 'const_copies':
            body = body.replace('int teamCount = m_', 'const int teamCount = m_')
            body = body.replace('int playerCount = m_', 'const int playerCount = m_')
        elif captures == 'fields':
            # Each selector owns its own copied inputs in the current source;
            # replacing whole local blocks avoids crossing output-write calls.
            for side, limit in (('human', 2), ('human', 1), ('computer', 1)):
                field = 'm_' + side + 'TeamCount'
                old = '                int teamCount = ' + field + ';\n                ' + field + ' = std::_cpp_max(teamCount, ' + str(limit) + ');'
                if limit == 2:
                    old = old.replace('                ', '            ')
                new = old[:len(old) - len(old.lstrip())] + field + ' = std::_cpp_max(' + field + ', ' + str(limit) + ');'
                assert body.count(old) == 1
                body = body.replace(old, new)
            for side, reverse in (('human', human), ('computer', computer)):
                team, player = 'm_' + side + 'TeamCount', 'm_' + side + 'PlayerCount'
                order = 'teamCount, playerCount' if reverse else 'playerCount, teamCount'
                old = '                int playerCount = ' + player + ';\n                int teamCount = ' + team + ';\n                ' + team + ' = std::_cpp_min(' + order + ');'
                new = '                ' + team + ' = std::_cpp_min(' + (team + ', ' + player if reverse else player + ', ' + team) + ');'
                assert body.count(old) == 1
                body = body.replace(old, new)
        if selectors == 'value':
            assert body.count('std::_cpp_max(') == 3 and body.count('std::_cpp_min(') == 2
            body = body.replace('std::_cpp_min(', 'min(').replace('std::_cpp_max(', 'max(')
        yield '+'.join((selectors, captures, str(human), str(computer))), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'type_random_map_generator::writeMapHeader')
    axis = helper.axis('header_team_selectors', 'src/rmg.cpp', original, variants(original))
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'map-header team-selector controls')


if __name__ == '__main__':
    main()
