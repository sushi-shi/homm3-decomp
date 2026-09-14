#!/usr/bin/env python3
"""Recover child-region lifetimes at the exact noise-subdivision CFG.

Retail 0x53e9e0 and the candidate have the same 32 blocks and all operations;
only the center/X/Y stack homes are permuted. Four quadrant records currently
reuse one mutable region. Test ownership by one work scope, two X halves or
four quadrant scopes, preserving complete copies and append order. Cross
ordinary initialization with immutable midpoint/input values; these are real
read-only math inputs, not added operations. No Dreamcast counterpart exists.
The independent sample-lattice oracle must preserve every corner and reject
degenerate, changed-sample and changed-variation negative controls.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    original=generator('generate-rmg-position-family.py').definition(source,'subdivideRmgNoiseRegion')
    at=original.index('    TRmgNoiseRegion part = region;')
    head=original[:at]
    parts=original[at:original.rindex('\n}')].split('\n\n')
    assert len(parts)==4 and all(p.count('pending.push_back(part);')==1 for p in parts)
    region_initializers={'copy':'TRmgNoiseRegion part = region;', 'direct':'TRmgNoiseRegion part(region);',
                         'assigned':'TRmgNoiseRegion part;\n    part = region;'}
    arguments='int centerValue, TRmgNoiseRegion region, TRmgNoiseMidpoints midpoints'
    options=[]
    for inputs,midpoints,scope,construction in itertools.product((False,True),(False,True),('shared','work_scope','halves','quadrants'),('copy','direct','assigned')):
        prefix=head
        if inputs:prefix=prefix.replace(arguments,'const int centerValue, const TRmgNoiseRegion region, const TRmgNoiseMidpoints midpoints')
        if midpoints:prefix=prefix.replace('    int middle','    const int middle')
        groups=[parts] if scope in ('shared','work_scope') else ([parts[:2],parts[2:]] if scope=='halves' else [[p] for p in parts])
        rendered=[]
        for group in groups:
            copied=list(group)
            first=copied[0]
            before='TRmgNoiseRegion part = region;' if first.startswith('    TRmgNoiseRegion') else 'part = region;'
            copied[0]=first.replace(before,region_initializers[construction],1)
            chunk='\n\n'.join(copied)
            if scope!='shared':chunk='    {\n'+ '\n'.join('    '+line if line else '' for line in chunk.splitlines())+'\n    }'
            rendered.append(chunk)
        body=prefix+'\n\n'.join(rendered)+'\n}'
        option=dict(name=('const_inputs' if inputs else 'value_inputs')+'+'+('const_midpoints' if midpoints else 'value_midpoints')+'+'+scope+'+'+construction,replace=body)
        if inputs:option['extra_edits']=[dict(source='include/rmg.h',find='    '+arguments+');',replace='    const int centerValue, const TRmgNoiseRegion region, const TRmgNoiseMidpoints midpoints);')]
        options.append(option)
    assert options[0]['replace']==original
    payload=dict(schema=1,source='src/rmg.cpp',evidence=__doc__,
                 units=['rmg','rmg_support','rmg_terrain','tiles','singleselectionpopups','singleselectionwindow','scenarioinfo'],
                 axes=[dict(name='noise_region_lifetimes',find=original,options=options)])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('48 noise input/midpoint/child-region lifetime controls')


if __name__=='__main__':main()
