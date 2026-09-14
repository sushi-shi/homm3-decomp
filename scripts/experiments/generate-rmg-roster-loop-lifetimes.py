#!/usr/bin/env python3
"""Test genuine roster-loop count lifetimes while retaining push_back(new ...).

Retail 0x538b10 and the candidate agree through the key-tent loop. The first
nonmatching expansion is the following ordinary treasure registration: retail
retains value insert, candidate expands it to count insert. Later the candidate
also retains begin and the treasure-base constructor inside the quest roster.
Test captured/induction count lifetimes and equivalent descending loop headers
for the two creature passes, key colors and dwellings. Preserve every factory,
argument, registration order and canonical container/helper declaration. No
Dreamcast counterpart or source-line evidence is available for this function.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    creature = ('        int creatureCount = m_mapVersion >= 1 ? 145 : 118;\n'
                '        for (int creature = creatureCount; creature--;) {')
    key = ('        int player = m_objectPrototypes[10].size();\n'
           '        m_disabledKeyTents.resize(player);\n'
           '        for (; player--;) {')
    dwelling = ('    int dwelling = 80;\n    if (m_mapVersion < 1)\n'
                '        dwelling = 58;\n    for (; dwelling--;)')
    if original.count(creature) != 2 or original.count(key) != 1 or original.count(dwelling) != 1:
        raise ValueError('review current roster loops')
    creatures = [creature,
        creature.replace('int creatureCount', 'const int creatureCount'),
        '        for (int creature = m_mapVersion >= 1 ? 145 : 118; creature--;) {',
        '        int creature = m_mapVersion >= 1 ? 145 : 118;\n        for (; creature--;) {',
        creature.replace('creature = creatureCount; creature--;', 'creature = creatureCount - 1; creature >= 0; --creature'),
    ]
    keys = [key]
    for kind in ('int', 'const int', 'const unsigned int'):
        keys.append('        ' + kind + ' colorCount = m_objectPrototypes[10].size();\n'
                    '        m_disabledKeyTents.resize(colorCount);\n'
                    '        for (int player = colorCount; player--;) {')
    dwellings = [dwelling,
        '    int dwelling = m_mapVersion < 1 ? 58 : 80;\n    for (; dwelling--;)',
        '    const int dwellingCount = m_mapVersion < 1 ? 58 : 80;\n'
        '    for (int dwelling = dwellingCount; dwelling--;)',
    ]
    for c, k, d in itertools.product(range(5), range(4), range(3)):
        body = original.replace(creature, creatures[c]).replace(key, keys[k]).replace(dwelling, dwellings[d])
        yield 'creatures_' + str(c) + '+keys_' + str(k) + '+dwellings_' + str(d), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::initializeObjectGenerators')
    axis = helper.axis('roster_loop_lifetimes', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'roster count/lifetime states')


if __name__ == '__main__':
    main()
