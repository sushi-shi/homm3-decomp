#!/usr/bin/env python3
"""Restore DC's condition-handled and final outcome bookkeeping statements."""
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
    start = text.index('void checkEndGame(int forceWin)\n{')
    return text[start:text.index('\n}\n', start) + 2]
body = function(source)
checkpoint = json.loads(args.parent_checkpoint.read_text())
parents = []
for row in checkpoint['elites']:
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/kb.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('end-game body does not equal a reproduced parent')

condition = '''    if (!displayVCWinLoss(g_game->m_mapHeader.m_victoryCondition, gameWon,
                          gameLost, 0))
        displayLCWinLoss(g_game->m_mapHeader.m_lossCondition, gameWon,
                         gameLost, 0);'''
handled = '''    if (displayVCWinLoss(g_game->m_mapHeader.m_victoryCondition, gameWon,
                         gameLost, 0)) {
        conditionHandled = 1;
    }
    if (!conditionHandled) {
        displayLCWinLoss(g_game->m_mapHeader.m_lossCondition, gameWon,
                         gameLost, 0);
    }'''
start = body.index('    if (forceWin == END_GAME_FORCE_VICTORY) {')
end = body.index('    g_inCheckEndGame = 0;', start)
tail = body[start:end]
forces = '''    if (forceWin == END_GAME_FORCE_VICTORY) {
        gameWon = 1;
        g_gameOver = 1;
        g_defeatedAllPlayers = 1;
    }
    if (forceWin == END_GAME_FORCE_DEFEAT) {
        gameLost = 1;
        g_gameOver = 1;
        g_defeatedAllPlayers = 0;
    }
'''
sync = '''    if (g_defeatedAllPlayers == 1 && g_gameOver) {
        gameWon = 1;
    }
    if (g_defeatedAllPlayers == 0 && g_gameOver) {
        gameLost = 1;
    }
'''
restore = '''    if (!g_gameOver) {
        if (g_unnamed691209 && g_netLocalGamePos == g_unnamed69120c
            && tookLocalControl) {
            g_currentPlayer->m_isLocal = 0;
            g_currentPlayer->m_isHuman = 0;
        }
    }
'''
assert body.count(condition) == 1
options = []
for flag, outcome in itertools.product(range(3), range(3)):
    candidate = body
    if flag:
        # The zero in r12 is established before check_player_loss at e37c8;
        # 3634 sets it after DisplayVCWinLoss and 3637 tests it separately.
        typ = 'int' if flag == 1 else 'bool'
        candidate = candidate.replace('    g_inCheckEndGame = 1;\n', '    g_inCheckEndGame = 1;\n    ' + typ + ' conditionHandled = 0;\n')
        candidate = candidate.replace(condition, handled)
    if outcome:
        candidate = candidate.replace(tail, forces + (sync if outcome == 2 else '') + restore)
    row = {'name': f'handled-{flag}-outcome-{outcome}'}
    if flag or outcome:
        row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'condition-and-final-result-statements', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced source parents: ' + ', '.join(parents),
        'DC e37c8 initializes a real handled flag; line3634/e3824 sets it after a true DisplayVCWinLoss result and 3637/e3826 tests it before DisplayLCWinLoss. Its scalar type is not recorded; test int/bool lifetimes.',
        'DC 3728 and3735 store gameWon/gameLost in the two independent forced-result arms. Lines3742/e39a6 and3747/e39ba synchronize those locals from the global end sequence and game-over flag. These are emitted DC statements, not guesses from line gaps.',
        'DC 3750 separately guards restoration on !gameOver, then3752 checks the temporary local-control state. The source currently folds this into the force-result else chain.',
        'Preserve Complete globals and byte tookLocalControl. The restored assignments have real outcome meanings even when VC6 removes their dead stores; do not add diagnostics or synthetic budget operations.',
        'After the player-record recovery, C2 reports root499/budget1000, checkPlayerLoss667, getEnemyCount109/budget333, and nested GetTeamMask109/budget74. Test missing positive source statements before changing helper boundaries.',
        'Nine finite states separate the handled-flag and outcome bookkeeping contributions; full positive source recovery is preferred subject to retail semantics/ABI/CFG.',
    ],
}, indent=2) + '\n')
