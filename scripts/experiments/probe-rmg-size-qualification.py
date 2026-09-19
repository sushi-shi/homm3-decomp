#!/usr/bin/env python3
"""Tiny ordinary VC6 control of the three real getSize interface families.

Uses the actual generic coordinate class, all three seven-slot interfaces,
actual map member layout, and the three authored size definitions. Other map
operations remain inherited pure declarations; no objects are instantiated and
no template emission is forced. This is an isolated ABI/codegen probe only.
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6._unit import flags_for_unit
from experiments._support import generator
from homm3.build.canonicalize_data_symbols import CoffObject


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    extract = generator('generate-rmg-position-family.py').definition
    point_start = header.index('template<class Coordinate> struct TRmgCoordinatePoint {')
    point_end = header.index('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')
    point_end += len('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')
    prefix = '''#define VA(address, size)
struct TPoint { int m_x, m_y; TPoint(int x, int y):m_x(x),m_y(y){} };
struct rmgTerrainTile;
struct TRmgMapItem;
'''
    prefix += header[point_start:point_end] + '\n'
    for name in ('TRmgMapInterface', 'TRmgMapAdapterInterface', 'TRmgRoadMapAdapterInterface'):
        start = header.index('class ' + name + ' {')
        end = header.index('\n};', start) + len('\n};')
        prefix += header[start:end] + '\n'
    fields_start = header.index('    unsigned char m_ownsMapItems;', header.index('class type_random_map :'))
    fields_end = header.index('    // Owning constructor', fields_start)
    prefix += 'class type_random_map : public TRmgMapInterface {\npublic:\n'
    prefix += header[fields_start:fields_end]
    prefix += '    virtual TRmgGridPoint getSize();\n};\n'
    for name, base in (('TRmgMapAdapter', 'TRmgMapAdapterInterface'),
                       ('TRmgRoadMapAdapter', 'TRmgRoadMapAdapterInterface')):
        prefix += ('class %s : public %s {\npublic:\n'
                   '    type_random_map* m_map;\n'
                   '    virtual TRmgGridPoint getSize();\n};\n') % (name, base)
    body = '\n\n'.join(extract(source, name + '::getSize') for name in
                         ('type_random_map', 'TRmgRoadMapAdapter', 'TRmgMapAdapter'))
    original = prefix + '\n' + body + '\n'
    results = []
    for receiver in (False, True):
        for returned in (False, True):
            text = original
            if receiver:
                assert text.count('virtual TRmgGridPoint getSize()') == 6
                text = text.replace('virtual TRmgGridPoint getSize()',
                                    'virtual TRmgGridPoint getSize() const')
                for name in ('type_random_map', 'TRmgRoadMapAdapter', 'TRmgMapAdapter'):
                    anchor = 'TRmgGridPoint ' + name + '::getSize()'
                    assert text.count(anchor) == 1
                    text = text.replace(anchor, anchor + ' const')
            if returned:
                assert text.count('virtual TRmgGridPoint getSize') == 6
                text = text.replace('virtual TRmgGridPoint getSize', 'virtual const TRmgGridPoint getSize')
                for name in ('type_random_map', 'TRmgRoadMapAdapter', 'TRmgMapAdapter'):
                    anchor = 'TRmgGridPoint ' + name + '::getSize'
                    assert text.count(anchor) == 1
                    text = text.replace(anchor, 'const ' + anchor)
            state = '%s_receiver+%s_result' % ('const' if receiver else 'mutable',
                                             'const' if returned else 'mutable')
            directory = args.output / state
            directory.mkdir(exist_ok=True)
            cpp = directory / 'control.cpp'
            cpp.write_text(text)
            for trial in ('first', 'repeat'):
                obj = directory / (trial + '.obj')
                command = [sys.executable, '-m', 'homm3.core.cc_wrap', '--out', str(obj),
                           '--src', str(cpp), '--', *flags_for_unit('rmg')]
                (directory / (trial + '-argv.json')).write_text(json.dumps(command, indent=2) + '\n')
                proc = subprocess.run(command, capture_output=True, text=True,
                                      env=dict(os.environ, HOMM3_DIR=str(HOMM3_DIR)))
                (directory / (trial + '.log')).write_text(proc.stdout + proc.stderr)
                if proc.returncode:
                    raise RuntimeError(proc.stdout + proc.stderr)
                coff = CoffObject(obj.read_bytes())
                functions = {}
                for symbol in coff.symbols.values():
                    if symbol.typ == 32 and symbol.section > 0 and symbol.name.startswith('?getSize@'):
                        section = coff.sections[symbol.section-1]
                        functions[symbol.name] = coff.section_bytes(section)[symbol.value:].hex()
                result = dict(state=state, trial=trial,
                              source_hash=hashlib.sha256(text.encode()).hexdigest(), functions=functions)
                results.append(result)
                print(state, trial, functions, flush=True)
            assert results[-1]['functions'] == results[-2]['functions']
    (args.output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')


if __name__ == '__main__':
    main()
