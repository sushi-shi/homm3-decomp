#!/usr/bin/env python3
"""Tiny VC6 signed-map/unsigned-adapter coordinate-domain diagnostic.

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
    parser.add_argument('--named-results', action='store_true')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    extract = generator('generate-rmg-position-family.py').definition
    point_start = header.index('template<class Coordinate> struct TRmgCoordinatePoint {')
    point_end = header.index('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')
    point_end += len('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')
    prefix = '#define VA(address, size)\nstruct rmgTerrainTile;\nstruct TRmgMapItem;\n'
    for name in ('TRmgVector', 'TPoint'):
        start = header.index('struct ' + name + ' {')
        end = header.index('\n};', start) + len('\n};')
        prefix += header[start:end] + '\n'
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
    terrain = (HOMM3_DIR / 'src/rmg_terrain.cpp').read_text()
    begin = terrain.index('template<class Coordinate>\n// VA instance: TRmgCoordinatePoint<unsigned int>::TRmgCoordinatePoint(const TPoint&)')
    end = terrain.index('\nVA(0x004FA540', begin)
    conversion = terrain[begin:end]
    results = []
    states = ([(True, True, 'signed'), (True, True, 'unsigned')] if args.named_results else
              [(signed, visible, 'direct') for signed, visible in ((False, False), (True, False), (False, True), (True, True))])
    for signed, visible, binding in states:
        text = original
        if visible:
            anchor = 'typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;'
            text = text.replace(anchor, anchor + '\n' + conversion)
        if signed:
            for owner in ('TRmgMapInterface', 'type_random_map'):
                start = text.index('class ' + owner + ' ')
                end = text.index('\n};', start)
                group = text[start:end]
                assert group.count('virtual TRmgGridPoint getSize()') == 1
                text = text[:start] + group.replace('virtual TRmgGridPoint getSize()', 'virtual TPoint getSize()') + text[end:]
            base = extract(source, 'type_random_map::getSize')
            text = text.replace(base, base.replace('TRmgGridPoint', 'TPoint'))
            for owner in ('TRmgMapAdapter', 'TRmgRoadMapAdapter'):
                old = extract(source, owner + '::getSize')
                expression = '    return m_map->getSize();'
                if binding != 'direct':
                    expression = '    ' + ('TPoint' if binding == 'signed' else 'TRmgGridPoint') + ' size = m_map->getSize();\n    return size;'
                text = text.replace(old, 'TRmgGridPoint ' + owner + '::getSize()\n{\n' + expression + '\n}')
        state = ('signed_map' if signed else 'unsigned_control') + ('+visible' if visible else '+hidden') + '+' + binding
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
                    data = coff.section_bytes(section)[symbol.value:].rstrip(b'\x90')
                    relocs = [(r.site-symbol.value, r.typ, coff.symbols[r.symbol_index].name)
                              for r in coff.relocations if r.section == symbol.section]
                    functions[symbol.name] = dict(bytes=data.hex(), relocations=relocs)
            result = dict(state=state, trial=trial,
                          source_hash=hashlib.sha256(text.encode()).hexdigest(), functions=functions)
            results.append(result)
            print(state, trial, functions, flush=True)
        assert results[-1]['functions'] == results[-2]['functions']
    (args.output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')


if __name__ == '__main__':
    main()
