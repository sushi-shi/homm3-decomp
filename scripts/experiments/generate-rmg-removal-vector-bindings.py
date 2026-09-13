#!/usr/bin/env python3
"""Test zone-vector ownership at removeObject's split zone decrement.

Retail B8 loads the vector's zone pointer and directly decrements the typed
count. Earlier zone/scalar ownership and decrement-expression controls keep
a load/decrement/store. Bind the actual vector, preserving its operator[],
and test the existing counter lvalues in that context. Lifetimes start
before or after the map query, or inside the valid-zone branch. No new
helper, layout, public signature or iterator predicate is introduced.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    query = '        int zone = m_map.getMapItem('
    branch = '        if (zone >= 0) {\n'
    update = ('            int* counts = m_zones[zone]->m_objectCountByType;\n'
              '            --counts[objectType];')
    if any(original.count(anchor) != 1 for anchor in (query, branch, update)):
        raise ValueError('review changed removal vector/counter anchors')
    owners = [('direct', 'inside')] + list(itertools.product(
        ('reference', 'const_reference', 'pointer', 'const_pointer'),
        ('before_query', 'after_query', 'inside')))
    for (owner, location), counter in itertools.product(owners,
            ('pointer', 'array_reference', 'element_reference', 'element_pointer')):
        body = original
        access = 'm_zones'
        if owner != 'direct':
            kind = 'std::vector<TRmgZone*>'
            if owner.startswith('const_'):
                kind = 'const ' + kind
            pointer = owner.endswith('pointer')
            declaration = kind + ('* zones = &m_zones;\n' if pointer else '& zones = m_zones;\n')
            access = '(*zones)' if pointer else 'zones'
            if location == 'before_query':
                body = body.replace(query, '        ' + declaration + query)
            elif location == 'after_query':
                body = body.replace(branch, '        ' + declaration + branch)
            else:
                body = body.replace(branch, branch + '            ' + declaration)
        array = access + '[zone]->m_objectCountByType'
        if counter == 'pointer':
            replacement = 'int* counts = ' + array + ';\n            --counts[objectType];'
        elif counter == 'array_reference':
            replacement = 'int (&counts)[232] = ' + array + ';\n            --counts[objectType];'
        elif counter == 'element_reference':
            replacement = 'int& count = ' + array + '[objectType];\n            --count;'
        else:
            replacement = 'int* count = &' + array + '[objectType];\n            --*count;'
        body = body.replace(update, '            ' + replacement)
        yield '+'.join((owner, location, counter)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = generator('generate-rmg-object-removal-family.py').definition(
        (HOMM3_DIR / 'src/rmg.cpp').read_text())
    axis = generator('generate-rmg-position-family.py').axis(
        'removal_vector_bindings', 'src/rmg.cpp', original, variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'vector/counter binding states')


if __name__ == '__main__':
    main()
