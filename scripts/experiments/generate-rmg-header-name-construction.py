#!/usr/bin/env python3
"""Recover canonical map-name construction and length-query boundaries.

Retail initializes the string with _Tidy(false), computes the literal length,
then calls assign(ptr, length). Compare direct/copy construction and default
construction followed by assignment/assign; read-only direct/copy locals are
also valid because the name never changes. Independently compare length and
size at the two actual stream queries. Keep the literal, write sequence and
function-scope lifetime; never alter library definitions or add inline pins.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    literal = 'DATA_COMPGEN(0x00682900, rmgMapName, "Random Map")'
    old = '    std::string mapName(\n        ' + literal + ');'
    assert original.count(old) == 1
    forms = {
        'direct': old,
        'copy': '    std::string mapName = ' + literal + ';',
        'assigned': '    std::string mapName;\n    mapName = ' + literal + ';',
        'assign': '    std::string mapName;\n    mapName.assign(' + literal + ');',
        'const_direct': old.replace('std::string', 'const std::string'),
        'const_copy': '    const std::string mapName = ' + literal + ';',
    }
    for (name, construction), first, second in itertools.product(forms.items(), ('length', 'size'), ('length', 'size')):
        body = original.replace(old, construction)
        a = 'int intBuffer = mapName.length();'
        b = 'outfile->write(mapName.c_str(), mapName.length());'
        assert body.count(a) == body.count(b) == 1
        body = body.replace(a, 'int intBuffer = mapName.' + first + '();')
        body = body.replace(b, 'outfile->write(mapName.c_str(), mapName.' + second + '());')
        yield '+'.join((name, first, second)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'type_random_map_generator::writeMapHeader')
    axis = helper.axis('header_name_construction', 'src/rmg.cpp', original, variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'map-name construction controls')


if __name__ == '__main__':
    main()
