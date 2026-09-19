#!/usr/bin/env python3
"""Treasure selection control structure and its three retained helper boundaries.

Retail 0x546190 retains passability test and both erases. The verified C2
trace puts these close to the nested inline budget, so reconstruct the real
filter structure jointly with canonical public mask/container operations.
All filter evaluations preserve order and short circuiting. No invented
helper or dummy compiler-budget operation; no Dreamcast counterpart exists.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    # Each guard rejects the current definition. Keep virtual calls and value
    # reads in their original order; nesting only changes source ownership.
    guards = [
        '!primary && g_adventureObjectLandBlocked[objectType][0]\n            && !g_adventureObjectLandBlocked[objectType][2]',
        '!allowTerrainDependent && definition->isTerrainDependent()',
        'm_objectCountByType[objectType] >= g_rmgMapObjectLimits[objectType]',
        'zone->m_objectCountByType[objectType] >= g_rmgZoneObjectLimits[objectType]',
        'objectValue < 0 || objectValue < minimum || objectValue > maximum',
        '!candidate',
        'position.m_x >= 0 && m_map.isPlacementBlocked(candidate, position, zoneIndex, 1)',
    ]
    for flow, mask, reset in itertools.product(range(5), range(3), range(4)):
        body = original
        if flow == 1:
            old = ''.join('        if (' + guard + ')\n            continue;\n' for guard in guards[:4])
            new = '        if (' + '\n            || '.join('(' + guard + ')' for guard in guards[:4]) + ')\n            continue;\n'
            assert body.count(old) == 1
            body = body.replace(old, new)
        elif flow in (2, 3):
            selected = guards[:4] if flow == 2 else guards
            for guard in selected:
                old = '        if (' + guard + ')\n            continue;'
                assert body.count(old) == 1
                body = body.replace(old, '        if (!(' + guard + ')) {')
            tail = '        properties.push_back(candidate);\n'
            body = body.replace(tail, tail + '        }\n' * len(selected))
        elif flow == 4:
            begin = '        type_treasure_def* definition = m_objectGenerators[index];'
            body = body.replace(begin, '        {\n' + begin)
            body = body.replace('continue;', 'goto nextDefinition;')
            tail = '        properties.push_back(candidate);\n'
            body = body.replace(tail, tail + '        }\n    nextDefinition:;\n')
        passable = 'prototype->m_passableMask[CObjectType::getBitPos(x, y)]'
        if mask == 1:
            body = body.replace(passable, 'prototype->m_passableMask.test(CObjectType::getBitPos(x, y))')
        elif mask == 2:
            body = body.replace('!' + passable, '~' + passable)
        for bit, name in enumerate(('candidates', 'properties')):
            if reset & (1 << bit):
                body = body.replace(name + '.clear();', name + '.erase(' + name + '.begin(), ' + name + '.end());')
        yield dict(name=f'flow_{flow}+mask_{mask}+reset_{reset}', replace=body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-treasure-create-family.py')
    original = helper.helpers().definition((HOMM3_DIR / helper.SOURCE).read_text(), helper.FUNCTION)
    options = list(variants(original))
    assert len(options) == len({x['replace'] for x in options}) == 60
    assert options[0]['replace'] == original
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[dict(
        name='treasure_control_structure', source=helper.SOURCE, find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print('60 control-structure/mask/reset states')


if __name__ == '__main__':
    main()
