#!/usr/bin/env python3
"""QuickInfo's generator evaluations and fountain statement groups."""
import argparse
import itertools
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/advmgr.cpp').read_text()
a = source.index('void advManager::quickInfo(int cellX, int cellY, int z)\n{')
body = source[a:source.index('\n}\n', a) + 2]

def arm(text, case, following):
    a = text.index('            case ' + case)
    b = text.index('            case ' + following, a)
    return text[a:b]

generators = [arm(body, 'CREATURE_GENERATOR_1:', 'CREATURE_GENERATOR_4:'),
              arm(body, 'CREATURE_GENERATOR_4:', 'DEAD_GUY:')]
receiver = '''                generator* mapGenerator =
                    &g_game->m_generators[cell->m_extraInfo];
                int owner = mapGenerator->getOwner();
                int generatorType = mapGenerator->m_genType;'''
direct = '''                int owner = g_game->m_generators[cell->m_extraInfo].getOwner();
                int type = g_game->m_generators[cell->m_extraInfo].m_genType;'''
assert all(g.count(receiver) == 1 for g in generators)
fountain = arm(body, 'FOUNTAIN_OF_FORTUNE:', 'FOUNTAIN_OF_YOUTH:')
known = '                    if (cell->playerKnowsCell(g_unnamed69778c)) {'
assert fountain.count(known) == 1
high = '''                            (currHero->m_flags & 0x20000000UL)
                            + (currHero->m_flags & 0x10000000UL)
                            + (currHero->m_flags & 0x08000000UL)
                            + (currHero->m_flags & 0x20UL);'''
low = '''                            (currHero->m_flags & 0x20UL)
                            + (currHero->m_flags & 0x08000000UL)
                            + (currHero->m_flags & 0x10000000UL)
                            + (currHero->m_flags & 0x20000000UL);'''
ternary = '''                        sprintf(tempText, visitFormat,
                            testFlag
                                ? (*g_generalText)[
                                      GENERAL_TEXT_VISITED_OBJECT]
                                : (*g_generalText)[
                                      GENERAL_TEXT_UNVISITED_OBJECT]);'''
branches = '''                        if (testFlag)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);'''
assert fountain.count(high) == fountain.count(ternary) == 1
options = []
for choices in itertools.product(range(2), repeat=6):
    gen1, gen4, query, carrier, order, tail = choices
    candidate = body
    for choice, old in zip((gen1, gen4), generators):
        if choice:
            candidate = candidate.replace(old, old.replace(receiver, direct).replace('generatorType', 'type'))
    f = fountain
    if query:
        f = f.replace(known, '                    infolevel = g_game->getInfoFlag(FountainOfFortuneInfo, playerId);\n' + known)
    if order:
        f = f.replace(high, low)
    if tail:
        f = f.replace(ternary, branches)
    if carrier:
        f = f.replace('testFlag', 'visited')
    candidate = candidate.replace(fountain, f)
    row = {'name': '-'.join(map(str, choices))}
    if any(choices):
        row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/advmgr.cpp', 'units': ['advmgr'],
    'axes': [{'name': 'generator-evaluations-fountain-statements', 'find': body, 'options': options}],
    'evidence': [
        'Fresh full-build control after exact Hall closure; full DC show/lines/asm/inline/audit and retail summary/structure/source/calls for0x4137c0 completed before generating alternatives.',
        'DC7739 and7740 separately evaluate vector operator[] for owner and type;7757/7758 repeats this in generator4. No persistent generator receiver is observed in these groups, unlike rollover. Preserve get_owner, int locals and owner-before-type initialization.',
        'DC7890 explicitly queries GetInfoFlag(FountainOfFortuneInfo, iPlayer) into infolevel, then7892 independently tests PlayerKnowsCell. The meaningful unused query belongs in source even if release eliminates it.',
        'DC7904 adds four masked terms low-to-high into visited at sp+0x40 (r14+12);7906 tests visited,7908/7912 are separate sprintf groups followed by7914 strcat. Retail also sums four terms and stores the result to the parameter scratch before shared sprintf branches.',
        'Six binary axes measure each generator separately, query restoration, visited carrier, term order and two-format branch form. Adopt positive facts together even through temporary score dips unless retail semantics, ABI or CFG contradict them.',
        'No helper flattening, diagnostic padding, forced inlining, arbitrary scope additions or shared-header edits. All93 exact advmgr siblings are scored.',
    ],
}, indent=2) + '\n')
