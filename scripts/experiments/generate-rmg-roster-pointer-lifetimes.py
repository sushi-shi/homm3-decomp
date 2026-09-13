#!/usr/bin/env python3
"""Test allocated-pointer ownership without changing roster operations.

Retail 0x538b10 stores each new pointer in a stack home before insertion.
The first mismatch, just after the key-tent loop, expands value insert one
level too far; the later quest/tail boundary retains an unwanted begin and
treasure constructor. A named allocated pointer is a source hypothesis for
those homes, not a recovered declaration. Preserve each constructor, argument,
allocation order, loop and canonical push_back call. Compare direct new
arguments, one shared base pointer, and scoped base/derived pointer values,
constant pointers and references to pointer temporaries. No helper is copied
or redeclared and no insertion overload or inline policy changes.
"""
import argparse
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


REGISTRATION = re.compile(r'm_objectGenerators\.push_back\(\s*new (type_\w+)\(([^()]*)\)\);')


def variants(original):
    yield 'direct', original
    registrations = list(REGISTRATION.finditer(original))
    if len(registrations) != original.count('m_objectGenerators.push_back('):
        raise ValueError('review every registration expression')
    def replace(policy, declared_type):
        def one(match):
            concrete, arguments = match.groups()
            kind = concrete if declared_type == 'derived' else 'type_treasure_def'
            expression = 'new ' + concrete + '(' + arguments + ')'
            if policy == 'shared':
                declaration = 'allocated = ' + expression + ';'
            else:
                suffix = {'value': '*', 'constant': '* const', 'reference': '* const&'}[policy]
                declaration = kind + suffix + ' allocated = ' + expression + ';'
            return '{\n            ' + declaration + '\n            m_objectGenerators.push_back(allocated);\n        }'
        result = REGISTRATION.sub(one, original)
        if policy == 'shared':
            result = result.replace('\n{\n', '\n{\n    type_treasure_def* allocated;\n', 1)
        return result
    yield 'shared_base', replace('shared', 'base')
    for policy, kind in itertools.product(('value', 'constant', 'reference'), ('base', 'derived')):
        yield policy + '_' + kind, replace(policy, kind)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::initializeObjectGenerators')
    axis = helper.axis('roster_pointer_lifetimes', 'src/rmg.cpp', original, variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'roster allocated-pointer lifetime forms')


if __name__ == '__main__':
    main()
