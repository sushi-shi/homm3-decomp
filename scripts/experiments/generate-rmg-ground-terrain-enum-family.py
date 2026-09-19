#!/usr/bin/env python3
"""Joint ground-field enum storage and connection caller ownership.

TRmgGroundTile signed-six extraction is proven by retained getters and river
code. Generic integer setters do not alone exclude a terrain-enum field with
natural assignment casts. Test the existing TTerrainType (including NONE=-1)
as field type, both actual setter conversions, and each of four existing local
types in buildZoneConnectionPaths. No new enum, overload, accessor or inline.
Preserve prior direct-field source controls and inspect signed getter semantics
as well as connection0x540701's unsigned mask. RMG has no DC compiland.
"""
import argparse
import itertools
import json
import subprocess
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path);args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text();header=(HOMM3_DIR/'include/rmg.h').read_text()
    extract=generator('generate-rmg-position-family.py').definition
    original=extract(source,'type_random_map_generator::buildZoneConnectionPaths')
    read='unsigned terrain = current->m_tile.m_landType;';assert original.count(read)==1
    field='    signed m_landType : 6;';assert header.count(field)==1
    options=[]
    for storage,kind in itertools.product(range(2),('unsigned','int','unsigned char','TTerrainType')):
        expression='current->m_tile.m_landType'
        if kind=='TTerrainType':expression='static_cast<TTerrainType>('+expression+')'
        edits=[]
        if storage:
            edits.append(dict(source='include/rmg.h',find=field,replace='    TTerrainType m_landType : 6;'))
            for name in ('type_random_map::setTile','TRmgMapItem::setTerrain'):
                body=extract(source,name);old='m_tile.m_landType = terrain;';assert body.count(old)==1
                edits.append(dict(source='src/rmg.cpp',find=body,replace=body.replace(old,'m_tile.m_landType = static_cast<TTerrainType>(terrain);')))
        options.append(dict(name=f'enum_field_{storage}+local_{kind}',replace=original.replace(read,f'{kind} terrain = {expression};'),extra_edits=edits))
    deps=subprocess.check_output(['ninja','-t','deps'],cwd=HOMM3_DIR,text=True);units=[]
    for block in deps.split('\n\n'):
        lines=block.splitlines()
        if lines and str(HOMM3_DIR/'include/rmg.h') in [line.strip() for line in lines[1:]]:units.append(Path(lines[0].split(':',1)[0]).stem)
    units=sorted(set(units));assert {'rmg','rmg_support','rmg_terrain'}<=set(units)
    payload=dict(schema=1,units=units,evidence=__doc__,axes=[dict(name='ground_terrain_enum',source='src/rmg.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n');load_manifest(args.output,HOMM3_DIR)
    print('eight field/setter/local models;',units)


if __name__=='__main__':main()
