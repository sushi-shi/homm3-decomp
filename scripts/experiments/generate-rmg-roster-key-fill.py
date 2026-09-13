#!/usr/bin/env python3
"""Test the key-tent resize's zero-fill value and its real reset lifetime.

The first insertion mismatch in the single-reference control is immediately
after this loop; changing later roster lifetimes also changes earlier inline
decisions. Keep resize and the full reset loop, comparing its implicit zero
fill with explicit zero and a named disabled-state value used by both operations.
Preserve all allocation/registration expressions and the current constructor
interfaces. The allocation/registration/key-state oracle checks equivalence.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::initializeObjectGenerators')
    resize = '        m_disabledKeyTents.resize(player);'
    reset = '            m_disabledKeyTents[player] = 0;'
    if original.count(resize) != 1 or original.count(reset) != 1:
        raise ValueError('review current key-tent initialization')
    forms = [('implicit_zero', original)]
    for label, value in (('integer_zero', '0'), ('byte_zero', 'static_cast<unsigned char>(0)')):
        forms.append((label, original.replace(resize, '        m_disabledKeyTents.resize(player, ' + value + ');')))
    for label, kind, value in (('byte_value', 'unsigned char', '0'),
                               ('constant_byte', 'const unsigned char', '0'),
                               ('byte_reference', 'const unsigned char&', 'static_cast<unsigned char>(0)'),
                               ('constant_flag', 'const bool', 'false'),
                               ('constant_integer', 'const int', '0')):
        body = original.replace(resize, '        ' + kind + ' disabled = ' + value + ';\n'
                                '        m_disabledKeyTents.resize(player, disabled);')
        body = body.replace(reset, '            m_disabledKeyTents[player] = disabled;')
        forms.append((label, body))
    axis = helper.axis('roster_key_fill', 'src/rmg.cpp', original, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'key-state fill/reset value forms')


if __name__ == '__main__':
    main()
