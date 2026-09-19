#!/usr/bin/env python3
"""Scalar control plus eight genuine API models of the unsigned mask point.

Mask-record ownership recovers the checker's retail frame and block count but
not its bytes. Cross the existing coordinate read, initialization and update
interfaces in both footprint walkers; retain live prototype extents and world
cursor updates. The placement caller remains a held-out current-source control.
"""
import argparse
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    extract=generator('generate-rmg-position-family.py').definition
    originals={name:extract(source,'type_random_map::'+name)
               for name in ('isPlacementBlocked','addObject')}
    options=[dict(name='scalar_control',replace=source)]
    for read_api,init_api,step_api in itertools.product((False,True),repeat=3):
        edited=source
        for name,original in originals.items():
            body=original.replace('    TRmgMapPosition nearby = position;',
                '    TRmgGridPoint maskPoint;\n    TRmgMapPosition nearby = position;')
            assert body.count('unsigned int x')==body.count('unsigned int y')==1
            body=body.replace('unsigned int x','x').replace('unsigned int y','y')
            for coord in ('x','y'):
                body=re.sub(r'\b'+coord+r'\b','maskPoint.m_'+coord,body)
                member='maskPoint.m_'+coord
                getter='maskPoint.get'+coord.upper()+'()'
                setter='maskPoint.set'+coord.upper()
                if init_api:
                    body=body.replace(member+' = 0',setter+'(0)')
                if step_api:
                    body=body.replace('++'+member,setter+'('+ (getter if read_api else member)+' + 1)')
                if read_api:
                    body=body.replace(member+' < prototype.get',getter+' < prototype.get')
            if read_api:
                body=body.replace('CObjectType::getBitPos(maskPoint.m_x, maskPoint.m_y)',
                    'CObjectType::getBitPos(maskPoint.getX(), maskPoint.getY())')
            assert body.count('prototype.getWidth()')==original.count('prototype.getWidth()')
            assert body.count('prototype.getHeight()')==original.count('prototype.getHeight()')
            edited=edited.replace(original,body)
        label='+'.join(('getters' if read_api else 'field_reads',
            'setter_init' if init_api else 'field_init','setter_step' if step_api else 'field_step'))
        options.append(dict(name=label,replace=edited))
    payload=dict(schema=1,units=['rmg','rmg_support','rmg_terrain'],evidence=__doc__,
        axes=[dict(name='mask_point_api',source='src/rmg.cpp',find=source,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    _,loaded,axes=load_manifest(args.output,HOMM3_DIR)
    placement=extract(source,'type_random_map::canPlaceObject')
    for i in range(9):
        text=render(loaded,axes,(i,))['src/rmg.cpp']
        assert extract(text,'type_random_map::canPlaceObject')==placement
    print('9 finite states, both mask walkers share each genuine API model; placement caller held unchanged')


if __name__=='__main__':
    main()
