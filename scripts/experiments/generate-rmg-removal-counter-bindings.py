#!/usr/bin/env python3
"""Test removeObject's zone-counter lvalue and object-type lifetimes.

Retail 0x54bc50 loads the object type before the signed zone test and emits
one indexed memory decrement. The current candidate loads that type inside
the branch, then emits load/decrement/address/store. Prior decrement spelling
and owner-pointer controls did not recover it. Preserve the map query and
global decrement; bind the actual count array or element, and distinguish
copied int/enum and borrowed enum indices before/after the query or in its
successful branch. The array extent is the proven 232-int zone member.
Complete-only: no Dreamcast counterpart is known. No added helper boundary.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    query = ('        int zone = m_map.getMapItem(position.m_x - prototype->m_triggerCell.m_x,\n'
             '            position.m_y - prototype->m_triggerCell.m_y, position.m_z)->m_zoneState.m_zone;\n')
    tail = ('        if (zone >= 0)\n'
            '            --m_zones[zone]->m_objectCountByType[prototype->m_objectType];')
    anchor = query + tail
    if original.count(anchor) != 1:
        raise ValueError('review changed zone counter/query before rebasing')
    yield 'original', original
    index_forms = [('field', None)] + list(itertools.product(
        ('int', 'TAdventureObjectType', 'const TAdventureObjectType&'), range(3)))
    for (kind, location), binding in itertools.product(index_forms,
            ('direct', 'array_pointer', 'array_reference', 'element_reference', 'element_pointer')):
        before, after, inside = '', '', ''
        index = 'prototype->m_objectType'
        if kind != 'field':
            declaration = kind + ' objectType = ' + index + ';\n'
            index = 'objectType'
            if location == 0:
                before = '        ' + declaration
            elif location == 1:
                after = '        ' + declaration
            else:
                inside = '            ' + declaration
        array = 'm_zones[zone]->m_objectCountByType'
        element = array + '[' + index + ']'
        if binding == 'array_pointer':
            inside += '            int* counts = ' + array + ';\n'
            element = 'counts[' + index + ']'
        elif binding == 'array_reference':
            inside += '            int (&counts)[232] = ' + array + ';\n'
            element = 'counts[' + index + ']'
        elif binding == 'element_reference':
            inside += '            int& count = ' + element + ';\n'
            element = 'count'
        elif binding == 'element_pointer':
            inside += '            int* count = &' + element + ';\n'
            element = '*count'
        block = before + query + after + '        if (zone >= 0) {\n' + inside
        block += '            --' + element + ';\n        }'
        yield '+'.join((kind, str(location), binding)), original.replace(anchor, block)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = generator('generate-rmg-object-removal-family.py').definition(
        (HOMM3_DIR / 'src/rmg.cpp').read_text())
    axis = generator('generate-rmg-position-family.py').axis(
        'removal_counter_bindings', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'counter/index ownership states')


if __name__ == '__main__':
    main()
