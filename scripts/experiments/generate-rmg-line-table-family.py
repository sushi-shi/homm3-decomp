#!/usr/bin/env python3
"""Joint pattern-table read interface and refresh caller ownership.

Retail refresh reads frame->pattern through the table's copied array. Test
that actual mapping as direct access or one ordinary const accessor returning
value/reference, crossed with pointer/reference table receivers and scalar
selection-copy/reference lifetimes. No alternate declarations coexist, no
helper body is pasted into the caller, and no false inline keyword is used.
The accessor is a source hypothesis, not a recovered DC name or retail claim.
"""
import argparse
import itertools
import json
import subprocess
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from experiments._support import generator


def variants(source,header):
    extract=generator('generate-rmg-position-family.py').definition
    original=extract(source,'refreshRmgLinePoint')
    start=header.index('struct TRmgLinePatternTable {')
    end=header.index('\n};',start)+3
    table=header[start:end]
    for interface,receiver,snapshot in itertools.product(range(3),range(4),range(5)):
        body=original
        edits=[]
        if interface:
            result='int' if interface==1 else 'const int&'
            declaration=f'    {result} patternForFrame(int frame) const;\n'
            changed=table.replace('    ~TRmgLinePatternTable();\n','    ~TRmgLinePatternTable();\n'+declaration)
            edits.append(dict(source='include/rmg.h',find=table,replace=changed))
            definition=f'{result} TRmgLinePatternTable::patternForFrame(int frame) const\n{{\n    return m_patterns[frame];\n}}\n\n'
            body=body.replace('table->m_patterns[current.getFrame()]','table->patternForFrame(current.getFrame())')
            anchor='// Neighbour query used by refresh 0x4f9f00 and the walker\'s first pass\n'
            assert source.count(anchor)==1
            edits.append(dict(source='src/rmg_terrain.cpp',find=anchor,replace=definition+anchor))
        if receiver:
            spelling=('const TRmgLinePatternTable* table = painter->getPattern(oldType);' if receiver==1 else
                      ('TRmgLinePatternTable&' if receiver==2 else 'const TRmgLinePatternTable&')+' table = *painter->getPattern(oldType);')
            body=body.replace('TRmgLinePatternTable* table = painter->getPattern(oldType);',spelling)
            if receiver>1:
                body=body.replace('table->','table.').replace('selectRmgLinePattern(matches, table,','selectRmgLinePattern(matches, &table,')
        replacement=('int pattern = selected;','const int pattern = selected;',
                     'int& pattern = selected;','const int& pattern = selected;','')[snapshot]
        if snapshot==4:
            body=body.replace('    int pattern = selected;\n','')
            body=body.replace('!= pattern','!= selected').replace('m_ranges[pattern]','m_ranges[selected]')
        else:body=body.replace('int pattern = selected;',replacement)
        yield dict(name=f'interface_{interface}+receiver_{receiver}+snapshot_{snapshot}',replace=body,extra_edits=edits)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg_terrain.cpp').read_text();header=(HOMM3_DIR/'include/rmg.h').read_text()
    original=generator('generate-rmg-position-family.py').definition(source,'refreshRmgLinePoint')
    options=list(variants(source,header));assert len(options)==60 and options[0]['replace']==original
    # All real consumers of the shared RMG header, including owners of the
    # retained table constructor/destructor, must participate.
    deps=subprocess.check_output(['ninja','-t','deps'],cwd=HOMM3_DIR,text=True)
    units=[]
    for block in deps.split('\n\n'):
        lines=block.splitlines()
        if lines and str(HOMM3_DIR/'include/rmg.h') in [line.strip() for line in lines[1:]]:
            units.append(Path(lines[0].split(':',1)[0]).stem)
    units=sorted(set(units))
    assert {'rmg','rmg_support','rmg_terrain'}<=set(units)
    payload=dict(schema=1,units=units,evidence=__doc__,axes=[dict(name='line_table',source='src/rmg_terrain.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('60 table-interface/receiver/snapshot states; units',units)


if __name__=='__main__':main()
