#!/usr/bin/env python3
"""Test the recovered dimension-query interface inside treasure reset.

Retail0x535040 retains both vector destructors; the current reconstruction
expands the outline destructor and spills its end pointer. The later surface
dimension product also reverses the width/height operands. Earlier reset
families predate recovery of the actual output-reference getSize interface.
Test consuming that canonical query through the owned map or its existing
reference, with direct output or returned-reference ownership. Preserve the
two-coordinate origin query and all reset phases. No false inline or pins.
RMG has no Dreamcast counterpart. A retained virtual query absent in retail
would contradict this model even if its extra inline site helps the erasure.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from experiments._support import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR/'src/rmg.cpp').read_text()
    original = generator('generate-rmg-position-family.py').definition(source, 'TRmgTreasureGroup::reset')
    old = '    int width = map.m_mapWidth;\n    int height = map.m_mapHeight;'
    assert original.count(old) == 1
    options = [dict(name='field_control', replace=original)]
    for receiver, returned in itertools.product(('m_map', 'map'), (False, True)):
        if returned:
            setup = f'    TRmgGridPoint output;\n    const TRmgGridPoint& size = {receiver}.getSize(output);'
        else:
            setup = f'    TRmgGridPoint size;\n    {receiver}.getSize(size);'
        setup += '\n    int width = size.getX();\n    int height = size.getY();'
        options.append(dict(name=f'{receiver}+returned_{int(returned)}', replace=original.replace(old, setup)))
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[dict(
        name='reset_dimension_query', source='src/rmg.cpp', find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2)+'\n')
    load_manifest(args.output, HOMM3_DIR)
    print('five reset dimension-query states')


if __name__ == '__main__':
    main()
