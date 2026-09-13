#!/usr/bin/env python3
"""Recover the three scalar homes in noise subdivision through value ownership.

Retail 0x53e9e0 saves center/Y/X at EBP-4/-0xc/-8; the current body uses
EBP-8/-4/-0xc with the same instruction schedule and 32 CFG blocks. Earlier
value-copy and work-scope families did not recover these homes. Independently
bind each computed midpoint as a value, const value or const reference, and
keep or name/borrow the incoming center sample before or after the midpoints.
All references outlive their uses, all quadrant copies remain complete, and
the public vector operations, function ABI and signed division are unchanged.
No Dreamcast counterpart was found for this Complete-only helper.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    anchor = ('    int middleY = (region.m_bounds.m_minimumY + region.m_bounds.m_maximumY) / 2;\n'
              '    int middleX = (region.m_bounds.m_minimumX + region.m_bounds.m_maximumX) / 2;\n')
    if original.count(anchor) != 1:
        raise ValueError('review midpoint setup before generating')
    prefix, tail = original.split(anchor)
    kinds = (('value', 'int'), ('constant', 'const int'), ('reference', 'const int&'))
    for (y_label, y_type), (x_label, x_type), center in itertools.product(
            kinds, kinds, ('direct', 'value_before', 'value_after', 'reference_before', 'reference_after')):
        setup = anchor.replace('int middleY', y_type + ' middleY').replace('int middleX', x_type + ' middleX')
        ending = tail
        if center != 'direct':
            kind = 'const int&' if center.startswith('reference') else 'const int'
            declaration = '    ' + kind + ' center = centerValue;\n'
            setup = declaration + setup if center.endswith('before') else setup + declaration
            ending = tail.replace('= centerValue;', '= center;')
            if ending.count('= center;') != 4:
                raise ValueError('review all four center uses')
        yield '+'.join((y_label, x_label, center)), prefix + setup + ending


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'subdivideRmgNoiseRegion')
    axis = helper.axis('noise_scalar_bindings', 'src/rmg.cpp', original, variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'noise midpoint/center scalar binding forms')


if __name__ == '__main__':
    main()
