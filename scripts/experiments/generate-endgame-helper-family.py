#!/usr/bin/env python3
"""Refine the restored end-game helpers without flattening either boundary."""
import argparse
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()

def function(signature):
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}\n', start) + 2]

enemy = function('int getEnemyCount()')
player = function('static void checkPlayerLoss()')
axes = []
options = [{'name': 'control'}]
for binding in ('direct', 'named-player', 'game-reference', 'game-pointer'):
    for scan in ('joined-condition', 'nested-condition'):
        prefix = ''
        expr = 'g_game->getTeamMask(g_game->getLocalPlayerGamePos())'
        if binding == 'named-player':
            prefix = '    int player = g_game->getLocalPlayerGamePos();\n'
            expr = 'g_game->getTeamMask(player)'
        elif binding == 'game-reference':
            prefix = '    game& world = *g_game;\n'
            expr = 'world.getTeamMask(world.getLocalPlayerGamePos())'
        elif binding == 'game-pointer':
            prefix = '    game* world = g_game;\n'
            expr = 'world->getTeamMask(world->getLocalPlayerGamePos())'
        loop = '''    for (int i = 0; i < g_gamePlayerCount; ++i)
        if (!g_game->m_playerDisabled[i] && !(teamMask & (1 << i)))
            ++enemyCount;'''
        if scan == 'nested-condition':
            loop = '''    for (int i = 0; i < g_gamePlayerCount; ++i) {
        if (!g_game->m_playerDisabled[i]) {
            int playerBit = 1 << i;
            if (!(teamMask & playerBit))
                ++enemyCount;
        }
    }'''
        replacement = 'int getEnemyCount()\n{\n' + prefix + \
            '    unsigned char teamMask = ' + expr + ';\n    int enemyCount = 0;\n' + loop + \
            '\n    return enemyCount;\n}'
        options.append({'name': binding + '-' + scan, 'replace': replacement})
axes.append({'name': 'enemy-counter-and-nested-call-lifetimes', 'find': enemy, 'options': options})

start = player.index('        } else if (g_game->m_players[i].m_numTowns == 0) {')
end = player.index('\n        } else {\n            g_game->m_players[i].m_deathCountDown = -1;', start)
arm = player[start:end]
prefix = '        } else if (g_game->m_players[i].m_numTowns == 0) {\n'
field = 'g_game->m_players[i].m_deathCountDown'
options = [{'name': 'control'}]
for kind, decl, use in (
    ('reference', 'char& countdown = ' + field + ';', 'countdown'),
    ('pointer', 'char* countdown = &' + field + ';', '*countdown'),
):
    tail = arm[len(prefix):].replace(field, use)
    options.append({'name': kind, 'replace': prefix + '            ' + decl + '\n' + tail})
axes.append({'name': 'countdown-storage-reference', 'find': arm, 'options': options})

# Keep this independent of the countdown axis: the first death notification
# precedes that arm. Its format value can be bound or consumed directly.
anchor = '''                const char* deadFormat = (*g_generalText)[6];
                sprintf(g_text, deadFormat, g_game->getPlayerName(i));'''
axes.append({'name': 'dead-player-format-result', 'find': anchor, 'options': [
    {'name': 'control'},
    {'name': 'direct-format', 'replace': '                sprintf(g_text, (*g_generalText)[6], g_game->getPlayerName(i));'},
]})

for axis in axes:
    if source.count(axis['find']) != 1:
        raise ValueError('source anchor is stale or ambiguous: ' + axis['name'])
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'], 'axes': axes,
    'evidence': [
        'DC 0xe24a0 proves the static player-loss helper; DC 0xe34c4 proves the ordinary enemy-count helper and its nested GetTeamMask call.',
        'Both canonical definitions and checkEndGame calls remain; no fabricated inline declaration, gate, or budget operation.',
        'Retail 0x4f2ce0 retains a countdown-field address across the warning dialog; test its actual storage lifetime.',
        'Retail expands GetTeamMask but calls GetTeam inside it. The restored parent currently keeps GetTeamMask out of line.',
        '54 finite combinations of real receiver, counter, field-reference and format-result lifetimes.',
    ],
}, indent=2) + '\n')
