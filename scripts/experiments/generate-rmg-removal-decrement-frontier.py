#!/usr/bin/env python3
"""Test the remaining removal decrement after recovering trigger lifetimes.

The reproduced 98.4% trigger/counter family has the retail register schedule
outside its zone-counter block. Revisit discarded-result decrement and
assignment expressions on the same lvalue in that changed context, carrying
all ten reproduced parents. The counter, array ownership, enum index, query,
global count and footprint remain semantically unchanged.
"""
import argparse
import json
from pathlib import Path
import re

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(parent):
    matches = list(re.finditer(r'^            --(.+);$', parent, re.M))
    if len(matches) != 1:
        raise ValueError('expected one zone decrement')
    match = matches[0]
    expression = match.group(1)
    statements = (
        ('postfix', '(' + expression + ')--;'),
        ('subtract', expression + ' -= 1;'),
        ('assign', expression + ' = ' + expression + ' - 1;'),
        ('add_negative', expression + ' += -1;'),
        ('previous_value', '{\n                int previousCount = ' + expression
         + ';\n                ' + expression + ' = previousCount - 1;\n            }'),
    )
    for label, statement in statements:
        yield label, parent[:match.start()] + '            ' + statement + parent[match.end():]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('checkpoint', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    retained = generator('generate-rmg-removal-trigger-frontier.py').parents(
        args.checkpoint, expected_states=58)
    original = generator('generate-rmg-object-removal-family.py').definition(
        (HOMM3_DIR / 'src/rmg.cpp').read_text())
    forms = [('original', original)] + [(identity + '+parent', body) for identity, body in retained]
    for identity, parent in retained:
        forms += [(identity + '+' + label, body) for label, body in variants(parent)]
    axis = generator('generate-rmg-position-family.py').axis(
        'removal_decrement_forms', 'src/rmg.cpp', original, forms)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'decrement states;', len(retained), 'reproduced parents')


if __name__ == '__main__':
    main()
