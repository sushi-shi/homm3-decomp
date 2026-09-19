#!/usr/bin/env python3
"""Joint object-reference contract and meaningful mask/world ownership.

Keep all four reproduced interface controls. Apply the supported pointer-prvalue
interface to the remaining eleven coordinate parents. Three further controls
use the existing by-value position as the mutable world cursor, preserving only
origin-x, without rereading mutable object storage. No new helper or dummy local.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator


def verified(context):
    for folder in ('src','include','vendor'):
        for path in (context/'snapshot'/folder).rglob('*'):
            if path.is_file():
                assert path.read_bytes()==(HOMM3_DIR/path.relative_to(context/'snapshot')).read_bytes(),path
    rows=sorted(json.loads((context/'checkpoint.json').read_text())['elites'],key=lambda r:r['choices'])
    result=[]
    for row in rows:
        path=context/'candidates'/row['id'];first=json.loads((path/'first/result.json').read_text());repeat=json.loads((path/'repeat/result.json').read_text())
        for key in ('scores','object_hash','source_hashes','choices'):
            assert first[key]==repeat[key]==row[key]
        sources={}
        for name,sha in row['source_hashes'].items():
            text=(path/'first/tree'/name).read_text()
            assert text==(path/'repeat/tree'/name).read_text()
            assert hashlib.sha256(text.encode()).hexdigest()==sha
            sources[name]=text
        result.append((row,sources))
    return result


def make_reference(source):
    probe=generator('probe-rmg-map-object-reference.py')
    extract=generator('generate-rmg-position-family.py').definition
    body=extract(source,'type_random_map::addObject')
    source=source.replace(body,probe.reference_body(body))
    assert re.findall(r'\bm_map\.addObject\((\w+), position\)',source)==['guard','object','object','object','questObject']
    return re.sub(r'\bm_map\.addObject\((\w+), position\)',r'm_map.addObject(*\1, position)',source)


def parameter_cursor(body):
    old='    TRmgMapPosition nearby = position;'
    reset='        nearby.m_x = position.m_x;'
    assert body.count(old)==body.count(reset)==1
    body=body.replace(old,'    int originX = position.m_x;')
    body=body.replace(reset,'        position.m_x = originX;')
    body=body.replace('nearby.m_x','position.m_x').replace('nearby.m_y','position.m_y')
    body=body.replace('getMapItem(nearby)','getMapItem(position)')
    assert 'nearby' not in body
    return body


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('interface_context',type=Path)
    parser.add_argument('coordinate_context',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    interfaces=verified(args.interface_context);coordinates=verified(args.coordinate_context)
    assert [r['choices'] for r,s in interfaces]==[[i] for i in range(4)]
    assert [r['choices'] for r,s in coordinates]==[[i] for i in range(13)]
    source=(HOMM3_DIR/'src/rmg.cpp').read_text();header=(HOMM3_DIR/'include/rmg.h').read_text()
    probe=generator('probe-rmg-map-object-reference.py');ref_header=probe.reference_header(header)
    states=[('interface_parent:'+r['id'],s['include/rmg.h'],s['src/rmg.cpp']) for r,s in interfaces]
    for row,sources in coordinates:
        states.append(('coordinate_parent:'+row['id']+'+reference',ref_header,make_reference(sources['src/rmg.cpp'])))
    extract=generator('generate-rmg-position-family.py').definition
    for index in (1,5,9):
        row,sources=coordinates[index];edited=sources['src/rmg.cpp']
        for name in ('isPlacementBlocked','addObject'):
            body=extract(edited,'type_random_map::'+name);edited=edited.replace(body,parameter_cursor(body))
        states.append(('parameter_cursor:'+row['id']+'+reference',ref_header,make_reference(edited)))
    seen=set();options=[]
    for name,h,s in states:
        key=(h,s)
        if key in seen:continue
        seen.add(key);options.append(dict(name=name,replace=h,
            extra_edits=[dict(source='src/rmg.cpp',find=source,replace=s)]))
    assert len(options)==18
    runner=Path(__file__).with_name('run-rmg-map-object-reference-family.py')
    payload=dict(schema=1,units=['rmg','rmg_support','rmg_terrain','scenarioinfo',
        'singleselectionpopups','singleselectionwindow','tiles'],evidence=__doc__,
        parent_contexts=[args.interface_context.name,args.coordinate_context.name],
        reproduced_parents=[{key:row[key] for key in ('id','choices','source_hashes','object_hash')}
                            for row,sources in interfaces+coordinates],
        diagnostic_runner_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
        axes=[dict(name='map_reference_world',source='include/rmg.h',find=header,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    _,sources,axes=load_manifest(args.output,HOMM3_DIR)
    assert render(sources,axes,(0,))['src/rmg.cpp']==source
    held=extract(source,'type_random_map::canPlaceObject')
    for index in range(18):
        edited=render(sources,axes,(index,))
        assert extract(edited['src/rmg.cpp'],'type_random_map::canPlaceObject')==held
        assert edited['include/rmg.h'].count('virtual void addObject(type_object* object, TRmgMapPosition position);')==2
    print('18 finite states: four interface controls, eleven further coordinate parents, three parameter cursors')

if __name__=='__main__':main()
