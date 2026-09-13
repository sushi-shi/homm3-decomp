#!/usr/bin/env python3
"""Test canFitObject's prototype and object-kind ownership at its entry.

Retail 0x5355e0 and the candidate retain the same two placement calls and
25-block CFG. Retail loads position Y before the object kind and keeps that
kind in EBX; the candidate loads Y later and keeps the kind in EDX. Prior
position/trigger copies did not close this entry. Cross the existing enum's
copied/borrowed int and enum forms with real prototype pointer/reference
qualifiers and the two kind/coordinate declaration orders. Preserve every
scan, helper signature, surface query and the proven first-failure join.
No Dreamcast counterpart is known.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    anchor = ('    TObjectType* prototype = properties->m_prototype;\n'
              '    int objectType = prototype->m_objectType;\n'
              '    int x = position.m_x;\n    int y = position.m_y;\n'
              '    x -= prototype->m_triggerCell.m_x;\n'
              '    y -= prototype->m_triggerCell.m_y;\n')
    if original.count(anchor) != 1:
        raise ValueError('review changed group-fit entry before rebasing')
    for prototype, kind, order in itertools.product(
            ('pointer', 'const_pointer', 'fixed_pointer', 'reference', 'const_reference'),
            ('int', 'const int', 'const int&', 'TAdventureObjectType',
             'const TAdventureObjectType', 'const TAdventureObjectType&'), range(2)):
        types = {'pointer': 'TObjectType*', 'const_pointer': 'const TObjectType*',
                 'fixed_pointer': 'TObjectType* const', 'reference': 'TObjectType&',
                 'const_reference': 'const TObjectType&'}
        reference = 'reference' in prototype
        access = 'prototype.' if reference else 'prototype->'
        prefix = '    ' + types[prototype] + ' prototype = ' + ('*' if reference else '') + 'properties->m_prototype;\n'
        declaration = '    ' + kind + ' objectType = ' + access + 'm_objectType;\n'
        coordinates = '    int x = position.m_x;\n    int y = position.m_y;\n'
        prefix += coordinates + declaration if order else declaration + coordinates
        prefix += '    x -= ' + access + 'm_triggerCell.m_x;\n    y -= ' + access + 'm_triggerCell.m_y;\n'
        yield '+'.join((prototype, kind, str(order))), original.replace(anchor, prefix)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = generator('generate-rmg-position-family.py').definition(
        (HOMM3_DIR / 'src/rmg.cpp').read_text(), 'TRmgTreasureGroup::canFitObject')
    axis = generator('generate-rmg-position-family.py').axis(
        'fit_kind_ownership', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'object-kind/prototype ownership states')


if __name__ == '__main__':
    main()
