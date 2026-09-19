#!/usr/bin/env python3
"""Joint terrain projection, first-cell lookup and reset predecessor ownership.

The masked parent has the correct terrain extraction but keeps this in EDI
from entry, spills one reset predecessor coordinate and grows the zone latch.
Test the existing inline scalar lookup for the actual first cell and natural
predecessor lifetimes under both projection controls. Historical hoisting and
direct-store probes predate the recovered ordinary three-int constructor body
visibility in rmg.cpp. Preserve the canonical resetMovement call each time.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest,render
from homm3.vc6.test_rmg_families import generator


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);args=p.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text();extract=generator('generate-rmg-position-family.py').definition
    original=extract(source,'type_random_map_generator::buildZoneConnectionPaths')
    read='unsigned terrain = current->m_tile.m_landType;'
    previous='''        TRmgMapPosition previous;
        previous.m_x = -1;
        previous.m_y = -1;
        previous.m_z = -1;
        item->resetMovement(previous);'''
    assert original.count(previous)==1 and original.count(read)==1
    options=[]
    for mask,lookup,owner in itertools.product(range(2),range(2),range(4)):
        body=original
        if mask:body=body.replace(read,'unsigned terrain = current->m_tile.m_landType & 0x3f;')
        if lookup:body=body.replace('TRmgMapItem* item = m_map.m_mapItems;','TRmgMapItem* item = m_map.getMapItem(0, 0, 0);')
        if owner==1:body=body.replace(previous,'        TRmgMapPosition previous(-1, -1, -1);\n        item->resetMovement(previous);')
        elif owner==2:body=body.replace(previous,'        item->resetMovement(TRmgMapPosition(-1, -1, -1));')
        elif owner==3:
            body=body.replace(previous,'        item->resetMovement(previous);')
            body=body.replace('    while (count--) {','    const TRmgMapPosition previous(-1, -1, -1);\n    while (count--) {',1)
            start=body.index('    int count =');end=body.index('    for (unsigned int zoneIndex',start)
            body=body[:start]+'    {\n'+''.join('    '+line+'\n' for line in body[start:end].splitlines())+'    }\n'+body[end:]
        options.append(dict(name=f'mask_{mask}+lookup_{lookup}+reset_owner_{owner}',replace=body))
    payload=dict(schema=1,units=['rmg','rmg_support','rmg_terrain'],evidence=__doc__,axes=[dict(name='connection_reset_owner',source='src/rmg.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n');_,sources,axes=load_manifest(args.output,HOMM3_DIR)
    assert render(sources,axes,(0,))['src/rmg.cpp']==source
    print('sixteen projection/lookup/reset-ownership states')


if __name__=='__main__':main()
