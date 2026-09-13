#!/usr/bin/env python3
"""Test one translated vector value at canFitObject's entry and three scans.

Retail 0x5355e0 loads placement X/Y before the prototype kind and trigger,
then keeps the translated coordinates in ESI/EDI across the three scans.
The current scalar reconstruction delays Y and gives the kind a different
register. No Dreamcast counterpart is mapped. Test a named displacement,
including the existing point-subtraction operation, with nested trigger
copy/reference ownership and kind-before/after construction. Preserve the
original placement argument and every scan, failure join and helper call.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    anchor = ('    int objectType = prototype->m_objectType;\n'
              '    int x = position.m_x;\n    int y = position.m_y;\n'
              '    x -= prototype->m_triggerCell.m_x;\n'
              '    y -= prototype->m_triggerCell.m_y;\n')
    if original.count(anchor) != 1 or original.count('TRmgVector(x, y)') != 3:
        raise ValueError('review the translated origin and its three consumers')
    yield 'current_scalars', original
    for construction, trigger, kind_first, reuse in itertools.product(
            range(4), ('field', 'copy', 'reference'), (True, False), (False, True)):
        trigger_setup = ''
        tx, ty = 'prototype->m_triggerCell.m_x', 'prototype->m_triggerCell.m_y'
        if trigger != 'field':
            declaration = 'TObjectType::TPoint' if trigger == 'copy' else 'const TObjectType::TPoint&'
            trigger_setup = '    ' + declaration + ' trigger = prototype->m_triggerCell;\n'
            tx, ty = 'trigger.m_x', 'trigger.m_y'
        forms = (
            '    TRmgVector origin(position.m_x - ' + tx + ', position.m_y - ' + ty + ');\n',
            '    TRmgVector origin;\n    origin.m_x = position.m_x - ' + tx + ';\n'
            '    origin.m_y = position.m_y - ' + ty + ';\n',
            '    TRmgVector origin(position.m_x, position.m_y);\n'
            '    origin.m_x -= ' + tx + ';\n    origin.m_y -= ' + ty + ';\n',
            '    TRmgVector origin = TPoint(position.m_x, position.m_y) - TPoint(' + tx + ', ' + ty + ');\n')
        kind = '    int objectType = prototype->m_objectType;\n'
        setup = trigger_setup + forms[construction]
        setup = kind + setup if kind_first else setup + kind
        body = original.replace(anchor, setup).replace(
            'TRmgVector(x, y)', 'origin' if reuse else 'TRmgVector(origin.m_x, origin.m_y)')
        yield 'construct_%d+trigger_%s+kind_first_%d+reuse_%d' % (
            construction, trigger, kind_first, reuse), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'TRmgTreasureGroup::canFitObject')
    fit = generator('generate-rmg-group-fit-family.py')
    forms = list(variants(fit.early_failure(fit.body(1, 5, 0))))
    if original not in {body for _, body in forms}:
        raise ValueError('review changed group-fit semantics before rebasing')
    axis = helper.axis('fit_origin_vector', 'src/rmg.cpp', original, forms)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'translated-vector ownership states')


if __name__ == '__main__':
    main()
