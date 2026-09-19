#!/usr/bin/env python3
"""Calibrated ordinary map-addObject pointer/reference interface control.

Actual headers, placement body and later getMapItem body are used in their
observed relative source order. A reference parameter produces consumed pointer
prvalues for the existing STL APIs. No helper/copy constructor/inline is added.
The two pointer controls must reproduce selected bodies from actual full TUs.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from homm3.core.common import HOMM3_DIR
from homm3.vc6._unit import flags_for_unit
from experiments._support import generator
from homm3.build.canonicalize_data_symbols import CoffObject

POINTER = '?addObject@type_random_map@@QAEXPAVtype_object@@UTRmgMapPosition@@@Z'
REFERENCE = '?addObject@type_random_map@@QAEXAAVtype_object@@UTRmgMapPosition@@@Z'


def function(payload, symbol):
    coff = CoffObject(payload)
    selected = [s for s in coff.symbols.values() if s.section > 0 and s.name == symbol]
    assert len(selected) == 1, (symbol, len(selected))
    s = selected[0]
    data = coff.section_bytes(coff.sections[s.section-1])[s.value:].rstrip(b'\x90')
    relocs = [(r.site-s.value, r.typ, coff.symbols[r.symbol_index].name)
              for r in coff.relocations if r.section == s.section]
    return dict(bytes=data.hex(), relocations=relocs)


def reference_body(body):
    assert body.count('type_object* object,') == 1
    body = body.replace('type_object* object,', 'type_object& object,')
    body = body.replace('object->', 'object.')
    assert body.count('push_back(object)') == body.count('end(), object)') == 1
    return body.replace('push_back(object)', 'push_back(&object)').replace('end(), object)', 'end(), &object)')


def reference_header(header):
    start = header.index('class type_random_map :')
    end = header.index('\n};', start)
    group = header[start:end]
    old = 'void addObject(type_object* object, TRmgMapPosition position);'
    assert group.count(old) == 1
    return header[:start] + group.replace(old, old.replace('type_object*', 'type_object&')) + header[end:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('context', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    source = (HOMM3_DIR/'src/rmg.cpp').read_text()
    header = (HOMM3_DIR/'include/rmg.h').read_text()
    assert source == (args.context/'snapshot/src/rmg.cpp').read_text()
    assert header == (args.context/'snapshot/include/rmg.h').read_text()
    definition = generator('generate-rmg-position-family.py').definition
    prefix = source[:source.index('typedef std::set<TPoint>')]
    lookup = definition(source, 'type_random_map::getMapItem', parameters='TRmgMapPosition point')
    rows = []
    for choice in (0, 9):
        checkpoint = json.loads((args.context/'checkpoint.json').read_text())
        parent = next(row for row in checkpoint['elites'] if row['choices'] == [choice])
        directory = args.context/'candidates'/parent['id']
        first = json.loads((directory/'first/result.json').read_text())
        repeat = json.loads((directory/'repeat/result.json').read_text())
        for key in ('scores','object_hash','source_hashes'):
            assert first[key] == repeat[key] == parent[key]
        actual = function((directory/'first/rmg/candidate.obj').read_bytes(), POINTER)
        parent_source = (directory/'first/tree/src/rmg.cpp').read_text()
        assert hashlib.sha256(parent_source.encode()).hexdigest() == parent['source_hashes']['src/rmg.cpp']
        original = definition(parent_source, 'type_random_map::addObject')
        for reference in (False, True):
            name = ('cell_mask' if choice else 'scalar') + ('_reference' if reference else '_pointer')
            work = args.output/name
            work.mkdir(exist_ok=True)
            (work/'rmg.h').write_text(reference_header(header) if reference else header)
            cpp = work/'control.cpp'
            cpp.write_text(prefix + '\n' + (reference_body(original) if reference else original) + '\n\n' + lookup + '\n')
            results = []
            for trial in ('first','repeat'):
                obj = work/(trial+'.obj')
                command = [sys.executable,'-m','homm3.core.cc_wrap','--out',str(obj),'--src',str(cpp),'--',*flags_for_unit('rmg')]
                (work/(trial+'-argv.json')).write_text(json.dumps(command,indent=2)+'\n')
                proc = subprocess.run(command,capture_output=True,text=True,env=dict(os.environ,HOMM3_DIR=str(HOMM3_DIR)))
                (work/(trial+'.log')).write_text(proc.stdout+proc.stderr)
                if proc.returncode:raise RuntimeError(proc.stdout+proc.stderr)
                results.append(function(obj.read_bytes(), REFERENCE if reference else POINTER))
            assert results[0] == results[1]
            row = dict(state=name,parent=parent['id'],reproduced=True,actual_pointer_control=actual,**results[0])
            if not reference:
                row['calibrated_actual_body'] = results[0] == actual
            rows.append(row)
            print(name, len(bytes.fromhex(row['bytes'])), 'calibrated',row.get('calibrated_actual_body'),flush=True)
    (args.output/'results.json').write_text(json.dumps(rows,indent=2)+'\n')

if __name__ == '__main__':
    main()
