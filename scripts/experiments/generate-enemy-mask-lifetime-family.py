#!/usr/bin/env python3
"""Enemy-mask value width and counter/receiver lifetimes after helper recovery."""
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
    start = text.index('int getEnemyCount()\n{')
    return text[start:text.index('\n}\n', start) + 2]
body = function(source)
checkpoint = json.loads(args.parent_checkpoint.read_text())
parents = []
for row in checkpoint['elites']:
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/kb.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('enemy body does not equal a reproduced parent')
start = body.index('    for (int i = 0;')
loop_and_return = body[start:]
options = []
for receiver, mask_type, counter in itertools.product(range(4), range(4), range(2)):
    if receiver == 0:
        prefix = '    game& world = *g_game;\n'
        call = 'world.getTeamMask(world.getLocalPlayerGamePos())'
    elif receiver == 1:
        prefix = ''
        call = 'g_game->getTeamMask(g_game->getLocalPlayerGamePos())'
    elif receiver == 2:
        prefix = '    int localPlayer = g_game->getLocalPlayerGamePos();\n'
        call = 'g_game->getTeamMask(localPlayer)'
    else:
        prefix = '    game* world = g_game;\n'
        call = 'world->getTeamMask(world->getLocalPlayerGamePos())'
    typ = ('unsigned char', 'int', 'unsigned int', 'unsigned short')[mask_type]
    count = '    int enemyCount = 0;\n'
    candidate = 'int getEnemyCount()\n{\n' + (count if counter else '') + prefix + '    ' + typ + ' teamMask = ' + call + ';\n' + ('' if counter else count) + loop_and_return
    row = {'name': f'receiver-{receiver}-mask-{mask_type}-counter-{counter}'}
    if receiver or mask_type or counter:
        row['replace'] = candidate
    elif candidate != body:
        raise ValueError('control does not reproduce the source')
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'enemy-mask-and-counter-lifetimes', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced parents: ' + ', '.join(parents),
        'After restoring CheckEndGame outcome statements all25 named calls agree; remaining target enemy scan zero-extends the returned byte mask into a full-width test while the candidate tests BL/DL.',
        'GetEnemyCount has no recorded locals/types. Preserve game::getTeamMask unsigned-char return; test the consuming local as byte/int/unsigned/short, all representing exactly the same mask0..255.',
        'Retail reloads g_game after GetLocalPlayerGamePos; DC3420 has both calls on one source line and SH4 caches the receiver. Direct nested calls and an explicit returned player distinguish real evaluation lifetimes.',
        'Retail initializes/spills the counter before the nested lookup. DC also materializes zero before the calls at e34d0, despite the interleaved3420/3422 attribution; counter declaration/initialization placement is a bounded hypothesis, not recovered source text.',
        '32 finite states retain the ordinary helper, disabled-seat guard, explicit playerBit statement, eight-seat loop, and all recovered outcome statements.',
    ],
}, indent=2) + '\n')
