"""Restore the accidentally active markEnemy stub from source/retail evidence.

DC 0xa0a44 (findpath.cpp:1172) obtains the combat-cell flag and get_hex result,
returns only for an already marked cell with cost <= the incoming long, then
sets validMove and narrows the new cost to unsigned short. This is also the
ordinary helper body before integration; restore it in its canonical source
position. Retail findCombatPath contains four expansions retaining getHex,
and markTeleport contains the two corresponding marks. No alternate helper,
inline override, or caller edit is needed. The unchanged stub is a negative
control, not an acceptable implementation.
"""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SIGNATURE = 'void searchArray::markEnemy(long hex, long cost)'
BODY = '''void searchArray::markEnemy(long hex, long cost)
{
    // Before normalization (locals): combat_cell.
    hexcell* combatCell = &g_combatManager->m_cells[hex];
    pathCell* cell = getHex(hex);
    if (combatCell->m_validMove) {
        if (cell->m_cost <= cost)
            return;
    }
    combatCell->m_validMove = 1;
    cell->m_cost = static_cast<unsigned short>(cost);
}'''


def make_manifest():
    source = (ROOT / 'src/findpath.cpp').read_text()
    start = source.index(SIGNATURE + '\n{')
    actual = source[start:source.index('\n}', start) + 2]
    stub = SIGNATURE + '\n{\n    // @stub\n}'
    if actual != stub:
        raise ValueError('This restoration control requires the reviewed pre-fix stub checkpoint')
    return {'schema': 1, 'source': 'src/findpath.cpp', 'units': ['findpath'], 'evidence': __doc__,
            'axes': [{'name': 'canonical-mark-body', 'find': actual, 'options': [
                {'name': 'unchanged'}, {'name': 'restore-mark', 'replace': BODY}]}]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + '\n')
