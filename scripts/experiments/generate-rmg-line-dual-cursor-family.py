#!/usr/bin/env python3
"""Retain four offset controls and test two consumed, synchronized induction states.

Retail increments the mask index and offset-table pointer independently. Deriving
the former from the latter produced a real SUB/SAR on every iteration. Both the
mask index and offset cursor are now consumed directly, with pointer ordering
matching retail's unsigned end comparison. No extra reads or dummy owners.
"""
import argparse
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest,render
from experiments._support import generator


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('parents',type=Path)
    p.add_argument('output',type=Path)
    args=p.parse_args()
    old=generator('generate-rmg-line-offset-family.py')
    extract=generator('generate-rmg-position-family.py').definition
    source=(HOMM3_DIR/'src/rmg_terrain.cpp').read_text()
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    cp=json.loads(args.parents.read_text());root=args.parents.parent
    parents=sorted(cp['elites'],key=lambda e:e['choices'])
    assert [e['choices'] for e in parents]==[[0],[1],[2],[3]]
    options=[];evidence=[]
    for e,(reference,traversal) in zip(parents,((False,False),(True,False),(False,True),(True,True))):
        tree=root/'candidates'/e['id']
        a=json.loads((tree/'first/result.json').read_text());b=json.loads((tree/'repeat/result.json').read_text())
        assert all(a[k]==b[k] for k in ('source_hashes','object_hash','scores'))
        body,decl=old.model(source,header,reference,traversal)
        assert body==(tree/'first/tree/src/rmg_terrain.cpp').read_text()
        assert decl==(tree/'first/tree/include/rmg.h').read_text()
        option=dict(name=e['id']+'+control',replace=body)
        if decl!=header:option['extra_edits']=[dict(source='include/rmg.h',find=header,replace=decl)]
        options.append(option);evidence.append(dict(id=e['id'],choices=e['choices'],object_hash=e['object_hash']))
    for reference in (False,True):
        body,decl=old.model(source,header,reference,False)
        for name in ('refreshRmgLinePoint','TRmgLineWalker::paintPoint'):
            original=extract(body,name);changed=original
            indent='    ' if name=='refreshRmgLinePoint' else '        '
            declaration='unsigned int ' if name=='refreshRmgLinePoint' else ''
            loop=indent+'for ('+declaration+'direction = 0; direction < TILE_DIR_COUNT; ++direction) {'
            assert loop in changed
            new=(indent+declaration+'direction = 0;\n'+indent+'for (const TPoint* offset = g_tileDirections;\n'+indent+'     offset < g_tileDirections + TILE_DIR_COUNT; ++offset, ++direction) {')
            changed=changed.replace(loop,new,1)
            if reference:
                changed=changed.replace('getNeighbourLand(point, g_tileDirections[direction])','getNeighbourLand(point, *offset)')
            body=body.replace(original,changed)
        option=dict(name=('offset_reference' if reference else 'direction_index')+'+synchronized_cursors',replace=body)
        if decl!=header:option['extra_edits']=[dict(source='include/rmg.h',find=header,replace=decl)]
        options.append(option)
    payload=dict(schema=1,units=['rmg','rmg_support','rmg_terrain','scenarioinfo','singleselectionpopups','singleselectionwindow','tiles'],evidence=__doc__,parent_context=root.name,reproduced_parents=evidence,axes=[dict(name='dual_cursor_owner',source='src/rmg_terrain.cpp',find=source,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    _,sources,axes=load_manifest(args.output,HOMM3_DIR)
    assert render(sources,axes,(0,))['src/rmg_terrain.cpp']==source
    print('four verified controls plus two synchronized index/offset states')


if __name__=='__main__':main()
