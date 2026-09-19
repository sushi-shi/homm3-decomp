#!/usr/bin/env python3
"""Six real mask-coordinate construction/lifetime models.

Retain scalar and prior whole-mask controls. Cross whole-loop/row-local scope
with the existing two-coordinate constructor or default plus assignment.
Constructed coordinates initialize the actual loop; no redundant for-init
overwrites them. Whole-loop x resets in the outer increment, preserving clipped
row continue behavior. Row-local points are created only for visited rows.
Prototype width/height remain live. No helper, signature or body is invented.
"""
import argparse
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator


def transform(original, scope, construction):
    if scope=='scalar':
        return original
    if scope=='control':
        body=original.replace('    TRmgMapPosition nearby = position;',
            '    TRmgGridPoint maskPoint;\n    TRmgMapPosition nearby = position;')
        body=body.replace('unsigned int x','x').replace('unsigned int y','y')
        for coord in ('x','y'):
            body=re.sub(r'\b'+coord+r'\b','maskPoint.m_'+coord,body)
        return body
    body=original
    outer='    for (unsigned int y = 0; y < prototype.getHeight(); ++y, --nearby.m_y) {'
    inner='        for (unsigned int x = 0; x < prototype.getWidth(); ++x, --nearby.m_x) {'
    assert body.count(outer)==body.count(inner)==1
    if scope=='whole':
        setup=('    TRmgGridPoint maskPoint(0U, 0U);\n' if construction=='ctor' else
               '    TRmgGridPoint maskPoint;\n    maskPoint.m_x = 0;\n    maskPoint.m_y = 0;\n')
        body=body.replace(outer,setup+
            '    for (; maskPoint.m_y < prototype.getHeight();\n'
            '            ++maskPoint.m_y, --nearby.m_y, maskPoint.m_x = 0) {')
    else:
        setup=('        TRmgGridPoint maskPoint(0U, y);\n' if construction=='ctor' else
               '        TRmgGridPoint maskPoint;\n        maskPoint.m_x = 0;\n        maskPoint.m_y = y;\n')
        body=body.replace(inner,setup+inner)
    body=body.replace(inner,
        '        for (; maskPoint.m_x < prototype.getWidth(); ++maskPoint.m_x, --nearby.m_x) {')
    body=body.replace('CObjectType::getBitPos(x, y)',
        'CObjectType::getBitPos(maskPoint.m_x, maskPoint.m_y)')
    assert body.count('prototype.getWidth()')==original.count('prototype.getWidth()')
    assert body.count('prototype.getHeight()')==original.count('prototype.getHeight()')
    return body


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    extract=generator('generate-rmg-position-family.py').definition
    originals={name:extract(source,'type_random_map::'+name)
               for name in ('isPlacementBlocked','addObject')}
    states=[('scalar','control'),('control','default'),('whole','assign'),
            ('whole','ctor'),('row','assign'),('row','ctor')]
    options=[]
    for scope,construction in states:
        edited=source
        for name,original in originals.items():
            edited=edited.replace(original,transform(original,scope,construction))
        options.append(dict(name=scope+'+'+construction,replace=edited))
    payload=dict(schema=1,units=['rmg','rmg_support','rmg_terrain'],evidence=__doc__,
        axes=[dict(name='mask_point_construction',source='src/rmg.cpp',find=source,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    _,loaded,axes=load_manifest(args.output,HOMM3_DIR)
    held=extract(source,'type_random_map::canPlaceObject')
    for i in range(len(states)):
        text=render(loaded,axes,(i,))['src/rmg.cpp']
        assert extract(text,'type_random_map::canPlaceObject')==held
    print('6 finite construction/lifetime models; scalar and prior mask controls retained')


if __name__=='__main__':
    main()
