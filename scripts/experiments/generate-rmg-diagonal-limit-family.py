#!/usr/bin/env python3
"""Canonical clamp call identity and unsigned-grid conversion ownership.

DC includes.h:124..131 proves the reference selector's three inputs/returns;
line134 and dc:0x1ef5c prove the value wrapper's copied parameters and call to
that selector. Prior diagonal probes changed a flat local clamp or the canonical
selector body, not this existing wrapper boundary. Preserve all six sites and
both callers, comparing cast-before arithmetic with conversion at the existing
int helper/local boundary. Valid terrain grids have positive dimensions and
bounded signed offsets; no unsigned comparison clamp is introduced.
"""
import argparse
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg_terrain.cpp').read_text()
    extract=generator('generate-rmg-position-family.py').definition
    names=('checkFirstDiagonal','checkSecondDiagonal')
    originals={name:extract(source,'rmgTerrainPainter::'+name) for name in names}
    assert [originals[name].count('tLimit(') for name in names]==[4,2]
    options=[]
    for wrapper,conversion in ((False,False),(True,False),(False,True),(True,True)):
        edited=source
        for original in originals.values():
            body=original
            if conversion:
                for expression in ('point.getX()','point.getY()','getWidth()','getHeight()'):
                    body=body.replace('static_cast<int>('+expression+')',expression)
            callee='limit' if wrapper else ('tLimit<int>' if conversion else 'tLimit')
            body=body.replace('tLimit(',callee+'(')
            edited=edited.replace(original,body)
        options.append(dict(name=('value_wrapper' if wrapper else 'reference_selector')+'+'+
                            ('boundary_conversion' if conversion else 'cast_before'),replace=edited))
    payload=dict(schema=1,units=['rmg','rmg_support','rmg_terrain'],evidence=__doc__,
        axes=[dict(name='diagonal_limit_identity',source='src/rmg_terrain.cpp',find=source,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    _,sources,axes=load_manifest(args.output,HOMM3_DIR)
    assert render(sources,axes,(0,))['src/rmg_terrain.cpp']==source
    print('4 canonical caller models; six clamp sites; shared helpers unchanged')

if __name__=='__main__':main()
