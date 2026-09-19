#!/usr/bin/env python3
"""Line refresh's selection ownership, snapshot lifetime and decision structure.

The 0x4f9f00 caller has all retained calls and ten CFG blocks, but a scalar
selection copy emits two extra moves before the frame comparison and entry
painter/point registers are exchanged. Cross three natural output owners
(separate outputs, existing flip pair, one selection record), four pattern
snapshot lifetimes, and five equivalent paint decisions. Keep canonical
neighbour/proxy helpers, virtual queries and random draw timing unchanged.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    anchor='    unsigned char flipX, flipY;\n'
    prefix=original[:original.index(anchor)]
    for owner,lifetime,flow in itertools.product(range(3),range(4),range(5)):
        if owner==0:
            declaration='    unsigned char flipX, flipY;\n    int selected;\n'
            selected,x,y='selected','flipX','flipY'
        elif owner==1:
            declaration='    TRmgTerrainFlip flip;\n    int selected;\n'
            selected,x,y='selected','flip.m_flipX','flip.m_flipY'
        else:
            declaration=('    struct Selection {\n        int pattern;\n'
                         '        unsigned char flipX, flipY;\n    } selection;\n')
            selected,x,y='selection.pattern','selection.flipX','selection.flipY'
        setup=declaration+f'    selectRmgLinePattern(matches, table, {selected}, {x}, {y});\n'
        snapshot=('    '+('const int' if lifetime==1 else 'int')+' pattern = '+selected+';\n')
        if lifetime==2:
            snapshot=''
            pattern=selected
        else:
            pattern='pattern'
        if lifetime==3:
            setup+=snapshot
        setup+='    rmgTerrainTile current;\n    tile.getTile(current);\n'
        if lifetime!=3:
            setup+=snapshot
        frame='table->m_patterns[current.getFrame()]'
        a,b='current.getFlipX()','current.getFlipY()'
        if flow==4:
            setup+='    int currentPattern = '+frame+';\n    unsigned char currentFlipX = '+a+';\n    unsigned char currentFlipY = '+b+';\n'
            frame,a,b='currentPattern','currentFlipX','currentFlipY'
        condition=f'{frame} != {pattern}\n        || {a} != {x} || {b} != {y}'
        if flow==1:
            condition=f'{pattern} != {frame}\n        || {x} != {a} || {y} != {b}'
        rendering=(f'        unsigned int frame = table->m_ranges[{pattern}].m_firstIndex\n'
                   f'            + rand() % table->m_ranges[{pattern}].m_valueCount;\n'
                   '        current.m_frame = frame;\n'
                   f'        current.m_flipX = {x};\n        current.m_flipY = {y};\n'
                   '        tile.setTile(current);\n')
        if flow==2:
            tail=(f'    if ({frame} == {pattern}\n        && {a} == {x} && {b} == {y})\n'
                  '        return;\n'+''.join(line[4:]+'\n' for line in rendering.splitlines()))
        else:
            if flow==3:
                setup+='    bool changed = '+condition+';\n'
                condition='changed'
            tail='    if ('+condition+') {\n'+rendering+'    }\n'
        yield dict(name=f'owner_{owner}+snapshot_{lifetime}+decision_{flow}',replace=prefix+setup+tail+'}')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg_terrain.cpp').read_text()
    original=generator('generate-rmg-position-family.py').definition(source,'refreshRmgLinePoint')
    options=list(variants(original))
    assert len(options)==len({o['replace'] for o in options})==60
    assert options[0]['replace']==original
    payload=dict(schema=1,units=['rmg_terrain'],evidence=__doc__,axes=[dict(
        name='line_selection',source='src/rmg_terrain.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('60 line selection ownership/lifetime/decision states')


if __name__=='__main__':
    main()
