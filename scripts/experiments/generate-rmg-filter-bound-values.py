#!/usr/bin/env python3
"""Refine filterZonePositions' bounds after recovering count-index borrowing.

The reproduced 99.6892% count parent retains all 110 retail blocks. Remaining
differences include bounds initialization and the maximum-X addition, beside
two element-address SIB operand orders. The adjacent getInitialZoneBounds
uses ordinary position assignment and long reference selectors. Test those
same source choices in the filter's four-bound accumulation, along with its
existing TRmgZoneBounds aggregate and operand order. Keep candidate ranking,
all three passes and every helper call outside that accumulation unchanged.
Retail-only; no Dreamcast counterpart establishes these local source choices.
"""
import argparse
import itertools
import json
from pathlib import Path
import re

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    prior = generator('generate-rmg-connection-count-access.py')
    count_original, count_parent = prior.checked_parent(args.parent)
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::filterZonePositions')
    begin = original.index('    int bestSize = 32000;')
    end = original.index('    int size = zone->getSize();', begin)
    region = original[begin:end]
    old_position = '            TRmgMapPosition position = m_zones[other]->getLevelPosition();\n'
    forms = [dict(name='baseline', replace=original)]
    positions = [('copy', old_position),
                 ('assigned', '            TRmgMapPosition position;\n            position = m_zones[other]->getLevelPosition();\n'),
                 ('direct', '            TRmgMapPosition position(m_zones[other]->getLevelPosition());\n'),
                 ('reference', '            const TRmgMapPosition& position = m_zones[other]->getLevelPosition();\n')]
    for (position_name, position), selector, storage, order in itertools.product(
            positions, ('value_wrapper', 'int_reference', 'long_reference'),
            ('scalars', 'aggregate'), ('position_first', 'size_first')):
        changed = region.replace(old_position, position)
        if selector != 'value_wrapper':
            kind = 'long' if selector == 'long_reference' else 'int'
            changed = changed.replace('= min(', '= std::_cpp_min<' + kind + '>(').replace('= max(', '= std::_cpp_max<' + kind + '>(')
        if order == 'size_first':
            for coordinate in ('x', 'y'):
                changed = changed.replace('position.m_' + coordinate + ' + size + 1',
                                          'size + position.m_' + coordinate + ' + 1')
        body = original[:begin] + changed + original[end:]
        if storage == 'aggregate':
            declarations = ''.join('    int ' + field + ' = 0;\n' for field in ('minimumY', 'minimumX', 'maximumY', 'maximumX'))
            assert body.count(declarations) == 1
            body = body.replace(declarations, '    TRmgZoneBounds bounds = {0, 0, 0, 0};\n')
            for field in ('minimumY', 'minimumX', 'maximumY', 'maximumX'):
                body = re.sub(r'\b' + field + r'\b', 'bounds.m_' + field, body)
        forms.append(dict(name='+'.join((position_name, selector, storage, order)), replace=body,
                          extra_edits=[dict(source='src/rmg.cpp', find=count_original, replace=count_parent)]))
    axis = dict(name='filter_bound_values', source='src/rmg.cpp', find=original, options=forms)
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(forms), 'filter bound-value controls')


if __name__ == '__main__':
    main()
