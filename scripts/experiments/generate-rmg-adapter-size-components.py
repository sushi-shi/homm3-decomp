#!/usr/bin/env python3
"""Recover component construction at the adapter's returned grid boundary.

Retail 0x532790 copies the virtual result pointer before loading the hidden
destination, then interleaves x/y loads and stores. Same-type copy returns
and a signed-domain conversion did not recover this. Keep the proven grid
type and implicit copy while testing its two-reference constructor and
existing component accessors, with temporary/result lifetimes. Both ICF
adapter owners keep the same source form and a single virtual size query.
No Dreamcast counterpart exists for this Complete-only boundary.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    for capture, components, result in itertools.product(
            ('value', 'const_value', 'reference', 'assigned'),
            ('fields', 'accessors', 'references'), ('direct', 'named', 'assigned')):
        lines = {
            'value': ['TRmgGridPoint size = m_map->getSize();'],
            'const_value': ['const TRmgGridPoint size = m_map->getSize();'],
            'reference': ['const TRmgGridPoint& size = m_map->getSize();'],
            'assigned': ['TRmgGridPoint size;', 'size = m_map->getSize();'],
        }[capture][:]
        x, y = 'size.m_x', 'size.m_y'
        if components == 'accessors':
            x, y = 'size.getX()', 'size.getY()'
        elif components == 'references':
            lines += ['const unsigned int& width = size.m_x;', 'const unsigned int& height = size.m_y;']
            x, y = 'width', 'height'
        constructor = 'TRmgGridPoint(' + x + ', ' + y + ')'
        if result == 'direct':
            lines += ['return ' + constructor + ';']
        elif result == 'named':
            lines += ['TRmgGridPoint result = ' + constructor + ';', 'return result;']
        else:
            lines += ['TRmgGridPoint result;', 'result = ' + constructor + ';', 'return result;']
        yield '+'.join((capture, components, result)), original[:original.index('{')] + '{\n' + ''.join('    ' + line + '\n' for line in lines) + '}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    river = helper.definition(source, 'TRmgMapAdapter::getSize')
    road = helper.definition(source, 'TRmgRoadMapAdapter::getSize')
    assert road == river.replace('TRmgMapAdapter::', 'TRmgRoadMapAdapter::')
    axis = helper.axis('adapter_size_components', 'src/rmg.cpp', river, variants(river))
    for option in axis['options']:
        option['extra_edits'] = [dict(source='src/rmg.cpp', find=road,
                                     replace=option['replace'].replace('TRmgMapAdapter::', 'TRmgRoadMapAdapter::'))]
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'adapter component-return controls')


if __name__ == '__main__':
    main()
