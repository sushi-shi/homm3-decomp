#!/usr/bin/env python3
"""Recover the player-record lifetime and loss predicates in checkPlayerLoss."""
import argparse
import itertools
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('parent_checkpoint', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()

def function(text):
    start = text.index('static void checkPlayerLoss()\n{')
    return text[start:text.index('\n}\n', start) + 2]
body = function(source)
checkpoint = json.loads(args.parent_checkpoint.read_text())
parents = []
for row in checkpoint['elites']:
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/kb.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('player-loss body does not equal a reproduced parent')

start = body.index('    for (i = 0; i < g_gamePlayerCount; i++) {')
end = body.index('\n\n    if (g_unnamed691209', start)
loop = body[start:end]
first_condition = '''        if (g_game->m_players[i].m_numHeroes == 0) {
            if (g_game->m_players[i].m_numTowns != 0) {
                g_game->m_players[i].m_deathCountDown = -1;
                continue;
            }'''
assert loop.count(first_condition) == 1
options = []
for binding, condition, guard, counter in itertools.product(range(3), range(2), range(2), range(2)):
    candidate_loop = loop
    if condition:
        candidate_loop = candidate_loop.replace(first_condition, '''        if (g_game->m_players[i].m_numHeroes == 0
            && g_game->m_players[i].m_numTowns == 0) {''')
    if binding:
        declaration = 'playerData* player = &g_game->m_players[i];' if binding == 1 else 'playerData& player = g_game->m_players[i];'
        receiver = 'player->' if binding == 1 else 'player.'
        candidate_loop = candidate_loop.replace('g_game->m_players[i].', receiver)
        anchor = '            continue;\n'
        pos = candidate_loop.index(anchor) + len(anchor)
        candidate_loop = candidate_loop[:pos] + '        ' + declaration + '\n' + candidate_loop[pos:]
    if guard:
        anchor = '''        if (g_game->m_playerDisabled[i])
            continue;'''
        before, contents = candidate_loop.split(anchor)
        # The final brace closes the for; give the live-player predicate its
        # own source block and keep all player accesses within that lifetime.
        contents = contents.rsplit('\n    }', 1)[0]
        candidate_loop = before + '        if (!g_game->m_playerDisabled[i]) {\n' + '\n'.join('    ' + row for row in contents.lstrip('\n').splitlines()) + '\n        }\n    }'
    if counter:
        candidate_loop = candidate_loop.replace('for (i = 0;', 'for (int i = 0;', 1)
    candidate = body[:start] + candidate_loop + body[end:]
    if counter:
        candidate = candidate.replace('    int i;\n', '')
    row = {'name': f'player-{binding}-condition-{condition}-guard-{guard}-counter-{counter}'}
    if binding or condition or guard or counter:
        row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'player-record-and-loss-branch-scopes', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced source parents: ' + ', '.join(parents),
        'DC 2750 computes a player-record address before the 2752 heroes/towns predicate; r13 retains that record across the warning dialog to the 2776 countdown store.',
        'Retail retains the countdown address in EBX across that dialog and spills/reloads the player-record offset. Test an actual record pointer/reference through the source loop.',
        'DC 2752 joins heroes==0 and towns==0; 2767 handles the remaining townless cases; 2797 clears the countdown for the other players. Test that if/else shape instead of the current early continue.',
        'DC 2748 opens a live-player guard around the loop body. Test its scope together with a real loop-counter declaration.',
        'Current passive C2 trace: checkPlayerLoss cb723 consumes the first caller budget; getEnemyCount receives277, nested GetTeamMask109 receives56 and remains a call. A real record binding may explain excess repeated indexing cost; no budget padding is allowed.',
        '24 finite states preserve the ordinary helper boundary, byte control flag, all named helpers, dialog arguments and store order.',
    ],
}, indent=2) + '\n')
