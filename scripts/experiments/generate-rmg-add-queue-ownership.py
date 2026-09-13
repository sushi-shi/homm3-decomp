#!/usr/bin/env python3
"""Test joint ownership/lifetime of addObject's two parallel queue vectors.

Retail 0x5402a0 has a smaller frame and a direct-delete queue cleanup; current
VC6 retains the position-vector destructor and different seed/pop boundaries.
Earlier experiments varied public operations and coordinates, not a single
owner for both queue vectors. Test std::pair ownership, member order, direct
member versus reference access and four meaningful construction points.
Keep canonical base registration, sorted insertion, coordinates and costs.
"""
import argparse
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    old = '        std::vector<TRmgMapPosition> positions;\n        std::vector<int> costs;\n'
    if original.count(old) != 1:
        raise ValueError('review current queue ownership')
    yield dict(name='separate_vectors', replace=original)
    anchors = (
        '        TObjectType::TPoint trigger = prototype->m_triggerCell;',
        '        TRmgMapPosition currentPosition;',
        '        TRmgMapItem* seed = m_map.getMapItem(currentPosition);',
        '        positions.push_back(currentPosition);')
    for reverse, references, lifetime in itertools.product((False, True), (False, True), range(4)):
        first, second = ('std::vector<int>', 'std::vector<TRmgMapPosition>') if reverse else ('std::vector<TRmgMapPosition>', 'std::vector<int>')
        position_member, cost_member = ('pending.second', 'pending.first') if reverse else ('pending.first', 'pending.second')
        declaration = '        std::pair<' + first + ', ' + second + ' > pending;\n'
        body = original.replace(old, '')
        anchor = anchors[lifetime]
        if references:
            declaration += '        std::vector<TRmgMapPosition>& positions = ' + position_member + ';\n'
            declaration += '        std::vector<int>& costs = ' + cost_member + ';\n'
        else:
            for name, expression in (('positions', position_member), ('costs', cost_member)):
                body = re.sub(r'\b' + name + r'\b', expression, body)
                anchor = re.sub(r'\b' + name + r'\b', expression, anchor)
        if body.count(anchor) != 1:
            raise ValueError('review queue construction point')
        body = body.replace(anchor, declaration + anchor)
        yield dict(name='reverse_%d+references_%d+lifetime_%d' % (reverse, references, lifetime),
                   replace=body, extra_edits=[dict(source='src/rmg.cpp', find='#include <algorithm>',
                       replace='#include <algorithm>\n#include <utility>')])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = generator('generate-rmg-add-object-family.py').definition((HOMM3_DIR / 'src/rmg.cpp').read_text())
    options = list(variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
        axes=[dict(name='object_queue_ownership', source='src/rmg.cpp', find=original, options=options)]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'queue ownership/lifetime states')


if __name__ == '__main__':
    main()
