#!/usr/bin/env python3
"""Water compatibility combines terrain representation with actual rejection phases.

The zone's terrain is compared with the cell terrain only after the latter is
known to be Water. A constant-Water comparison preserves every predicate even
for signed negative six-bit encodings. Test that value knowledge alongside the
observed six-bit projection and provisional signed/enum field models, including
the two legitimate generic-int setter conversions for enum storage. A third
model expresses seed rejection with continue and a real bottom increment.
"""
import argparse
import itertools
import json
import subprocess
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest,render
from homm3.vc6.test_rmg_families import generator


def predicate(body,kind):
    if kind==0:return body
    old='terrain != eTerrainWater || zone->m_terrain == terrain'
    assert body.count(old)==1
    if kind==1:return body.replace(old,'terrain != eTerrainWater || zone->m_terrain == eTerrainWater')
    start=body.index('                do {\n')
    end=body.index('                } while (1);',start)+len('                } while (1);')
    oldloop=body[start:end]
    read=next(line.strip() for line in oldloop.splitlines() if 'unsigned terrain =' in line)
    new='''                do {
                    TRmgMapItem* current = m_map.getMapItem(x, pathPosition.m_y, position.m_z);
                    if (current->m_zoneState.m_zone != zoneIndex)
                        continue;
                    READ
                    if (terrain == eTerrainWater && zone->m_terrain != eTerrainWater)
                        continue;
                    if (static_cast<int>(current->m_objects.size()) > 0)
                        continue;
                    seed.m_x = x;
                    seed.m_y = pathPosition.m_y;
                    seed.m_z = position.m_z;
                    if (current->hasSubterraneanGate() && current->m_tileData.m_roadPassable
                        && terrain != eTerrainRock) {
                        found = 1;
                        break;
                    }
                } while (++x < bounds.m_maximumX);'''.replace('READ',read)
    return body[:start]+new+body[end:]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);args=p.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text();header=(HOMM3_DIR/'include/rmg.h').read_text()
    extract=generator('generate-rmg-position-family.py').definition
    original=extract(source,'type_random_map_generator::buildZoneConnectionPaths')
    read='unsigned terrain = current->m_tile.m_landType;';assert original.count(read)==1
    field='    signed m_landType : 6;';assert header.count(field)==1
    options=[]
    for enum,mask,guard in itertools.product(range(2),range(2),range(3)):
        body=original.replace(read,'unsigned terrain = current->m_tile.m_landType & 0x3f;') if mask else original
        body=predicate(body,guard);edits=[]
        if enum:
            edits.append(dict(source='include/rmg.h',find=field,replace='    TTerrainType m_landType : 6;'))
            for name in ('type_random_map::setTile','TRmgMapItem::setTerrain'):
                setter=extract(source,name);old='m_tile.m_landType = terrain;';assert setter.count(old)==1
                edits.append(dict(source='src/rmg.cpp',find=setter,replace=setter.replace(old,'m_tile.m_landType = static_cast<TTerrainType>(terrain);')))
        options.append(dict(name=f'enum_{enum}+mask_{mask}+guard_{guard}',replace=body,extra_edits=edits))
    deps=subprocess.check_output(['ninja','-t','deps'],cwd=HOMM3_DIR,text=True);units=[]
    for block in deps.split('\n\n'):
        lines=block.splitlines()
        if lines and str(HOMM3_DIR/'include/rmg.h') in [line.strip() for line in lines[1:]]:units.append(Path(lines[0].split(':',1)[0]).stem)
    units=sorted(set(units));assert len(units)==7
    payload=dict(schema=1,units=units,evidence=__doc__,axes=[dict(name='water_compatibility',source='src/rmg.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    _,sources,axes=load_manifest(args.output,HOMM3_DIR)
    assert render(sources,axes,(0,))['src/rmg.cpp']==source
    print('twelve field/projection/predicate-phase states, seven consumers')


if __name__=='__main__':main()
