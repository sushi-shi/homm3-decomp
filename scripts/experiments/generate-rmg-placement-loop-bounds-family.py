#!/usr/bin/env python3
"""Separate bounds nesting from loop form in retail map addObject0x531ea0.

The loop-entry16 family coupled guarded do loops with positive bounds nesting;
those states retain vector insertion where retail expands it. This factorial
keeps eight distinct earlier controls and separates each dimension: for versus
guarded do, negative continue versus positive body, whole position copy versus
needed y/z initialization. In do/continue, comma-condition updates ensure every
iteration advances even when skipped. No loop bound is snapshotted, no helper
is copied or marked inline, and no dummy guard or operation is added.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from experiments._support import generator


def indent(text,n):
    return ''.join(' '*n+line+'\n' if line else '\n' for line in text.splitlines())


def loop(mode,axis,initialization,body):
    extent='Height' if axis=='y' else 'Width'
    bound=f'{axis} < prototype.get{extent}()'
    negative=f'nearby.m_{axis} < 0 || nearby.m_{axis} >= m_map{extent}'
    positive=f'nearby.m_{axis} >= 0 && nearby.m_{axis} < m_map{extent}'
    if mode in (0,3):
        active=f'if ({negative})\n    continue;\n'+body
    else:
        active=f'if ({positive}) {{\n'+indent(body,4)+'}\n'
    if mode<2:
        return initialization+f'for (unsigned int {axis} = 0; {bound}; ++{axis}, --nearby.m_{axis}) {{\n'+indent(active,4)+'}\n'
    result=f'unsigned int {axis} = 0;\nif ({bound}) {{\n'+indent(initialization,4)+'    do {\n'+indent(active,8)
    if mode==2:
        result+=f'        ++{axis};\n        --nearby.m_{axis};\n    }} while ({bound});\n'
    else:
        result+=f'    }} while (++{axis}, --nearby.m_{axis}, {bound});\n'
    return result+'}\n'


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path);args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text();original=generator('generate-rmg-position-family.py').definition(source,'type_random_map::addObject')
    prefix=original[:original.index('    TRmgMapPosition nearby = position;')]
    start=original.index('            TRmgGridPoint maskPoint(x, y);');end=original.rindex('\n        }\n    }\n}')
    cell='\n'.join(line[12:] for line in original[start:end].splitlines())+'\n'
    options=[]
    for outer,inner,initial in itertools.product(range(4),range(4),range(2)):
        initialize=('TRmgMapPosition nearby = position;\n' if initial==0 else 'TRmgMapPosition nearby;\nnearby.m_y = position.m_y;\nnearby.m_z = position.m_z;\n')
        body=loop(outer,'y',initialize,loop(inner,'x','nearby.m_x = position.m_x;\n',cell))
        options.append(dict(name=f'outer_{outer}+inner_{inner}+initial_{initial}',replace=prefix+indent(body,4)+'}'))
    assert len(options)==32 and options[0]['replace']==original
    # The eight prior structural models are literal controls, not approximations.
    old=generator('generate-rmg-placement-loop-entry-family.py')
    prior=list(old.variants(original))
    for outer,inner,initial in itertools.product(range(2),repeat=3):
        oldindex=outer*8+inner*4+initial*2
        newindex=(outer*2)*8+(inner*2)*2+initial
        assert options[newindex]['replace']==prior[oldindex]['replace'],(oldindex,newindex)
    payload=dict(schema=1,units=['rmg'],evidence=__doc__,axes=[dict(name='placement_loop_bounds',source='src/rmg.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n');load_manifest(args.output,HOMM3_DIR)
    print('32 loop/bounds/initialization models; eight exact prior source controls')


if __name__=='__main__':main()
