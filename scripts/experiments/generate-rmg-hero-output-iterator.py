#!/usr/bin/env python3
"""Recover the hero initializer's iterator state and natural set calls.

Retail writeMapHeader's hero loops store an output owner and index together
at EBP-0x14/-0x10, convert each input byte with logical not and advance both
input and output. Test the existing canonical bitset iterator and std::transform
against the current scalar-index loop. Remove the old inline-depth directive
in every alternative. Preserve the ordinary initializer and both source calls.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    original = helper.definition(source, 'setAvailableRmgHeroes')
    head = original[:original.index('\n{\n')]
    # Preserve this genuine negative control after adopting the transform;
    # do not reconstruct or introduce the historical inline-depth directive.
    indexed = '''    int heroIndex = 0;
    while (heroFlag != end) {
        bool available = !*heroFlag;
        availableHeroes->set(heroIndex, available);
        ++heroFlag;
        ++heroIndex;
    }
'''
    options = [dict(name='baseline', replace=original)]
    seen = {original}
    for borrowed_const, algorithm, predicate in itertools.product(
            (False, True), ('indexed', 'iterator', 'transform_temporary', 'transform_named', 'transform_copy'),
            ('unsigned char', 'bool')):
        signature = head.replace('unsigned char*', 'const unsigned char*') if borrowed_const else head
        if algorithm == 'indexed':
            inner = indexed
        elif algorithm == 'iterator':
            inner = '''    bitset_iterator<N> output(*availableHeroes, 0);
    while (heroFlag != end) {
        *output = !*heroFlag;
        ++heroFlag;
        ++output;
    }
'''
        else:
            output = 'bitset_iterator<N>(*availableHeroes, 0)'
            inner = ''
            if algorithm == 'transform_named':
                inner = '    bitset_iterator<N> output(*availableHeroes, 0);\n'
                output = 'output'
            elif algorithm == 'transform_copy':
                inner = '    bitset_iterator<N> output = bitset_iterator<N>(*availableHeroes, 0);\n'
                output = 'output'
            inner += '    std::transform(heroFlag, end, ' + output + ', std::logical_not<' + predicate + '>());\n'
        body = signature + '\n{\n' + inner + '}'
        if body in seen:
            continue
        seen.add(body)
        option = dict(name=str(borrowed_const) + '+' + algorithm + '+' + predicate, replace=body)
        if algorithm.startswith('transform') and '#include <functional>' not in source:
            option['extra_edits'] = [dict(source='src/rmg.cpp', find='#include <algorithm>',
                                          replace='#include <algorithm>\n#include <functional>')]
        options.append(option)
    axis = dict(name='hero_output_iterator', source='src/rmg.cpp', find=original, options=options)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'hero output iterator controls')


if __name__ == '__main__':
    main()
