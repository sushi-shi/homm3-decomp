#!/usr/bin/env python3
"""Test a provisional ordinary accessor for the selector's prototype range.

Retail 0x546040 computes an object-type vector address in ESI; direct field
and local range bindings keep EDI. Test a canonical range accessor on its
actual TRmgGeneratorBase owner, returning the same vector by reference.
Mutable and const views preserve the selector's read-only use. Compare
value/reference index parameters and definition positions, without changing
the public selector ABI, vector operations or checked admission logic.
The helper name/boundary is a retail codegen hypothesis, not DC evidence.
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
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::selectObjectPrototype')
    if original.count('m_objectPrototypes[objectType]') != 2:
        raise ValueError('review selector range accesses')
    caller = original.replace('m_objectPrototypes[objectType]', 'getObjectPrototypes(objectType)')
    options = [dict(name='direct', replace=original)]
    for const, parameter, location in itertools.product((False, True), ('int', 'const int&'), ('before', 'after')):
        result = ('const ' if const else '') + 'std::vector<TRmgObjectPropertiesRef*>&'
        qualifier = ' const' if const else ''
        signature = 'getObjectPrototypes(' + parameter + ' objectType)' + qualifier
        declaration = '    ' + result + ' ' + signature + ';\n'
        definition = ('// Provisional ordinary prototype-range accessor: selector 0x546040\n'
                      '// expands the range address; the direct-field control is 99.6581%.\n'
                      + result + ' TRmgGeneratorBase::' + signature + '\n'
                      '{\n    return m_objectPrototypes[objectType];\n}\n\n')
        anchor = ('// Retail 0x546040 filters the object-type vector by subtype, admitting slot'
                  if location == 'before' else
                  '// Weighted treasure selection returns a newly generated object and writes')
        options.append(dict(name=('const' if const else 'mutable') + '+' + parameter + '+' + location,
            replace=caller, extra_edits=[
                dict(source='include/rmg.h', insert_before='    void loadObjectPrototypes();', text=declaration),
                dict(source='src/rmg.cpp', insert_before=anchor, text=definition)]))
    args.output.write_text(json.dumps(dict(schema=1,
        units=generator('generate-rmg-map-accessor-family.py').UNITS, evidence=__doc__,
        axes=[dict(name='selector_range_helper', source='src/rmg.cpp', find=original, options=options)]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'ordinary range-accessor controls across seven consumers')


if __name__ == '__main__':
    main()
