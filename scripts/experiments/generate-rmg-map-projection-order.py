#!/usr/bin/env python3
"""Recover the retained lookup body beside the corrected guard call boundaries.

The reproduced values+references parent retains both map-position lookups in
monolith and expands both occupancy size calls, as retail does (92.6892%).
Its own lookup uses ESI for the projected level (76% instead of the exact
39-byte body). Test the six real scalar projection orders with direct/named
returns, preserving all accessor calls, reference lifetimes and delegation.
The original field projection remains the first control. Parent source and
repeat hashes must agree; no source is adopted solely for the caller score.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('checkpoint',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    root=args.checkpoint.parent
    checkpoint=json.loads(args.checkpoint.read_text())
    parents=[r for r in checkpoint['elites'] if r['labels'].get('position_projection')=='values+references+direct']
    assert len(parents)==1
    parent=parents[0]
    directory=root/'candidates'/parent['id']
    repeat=json.loads((directory/'repeat/result.json').read_text())
    for key in ('object_hash','scores','source_hashes','choices'):assert repeat[key]==parent[key],key
    files={}
    for name in ('src/rmg.cpp','include/rmg.h'):
        current=(HOMM3_DIR/name).read_text()
        assert current==(root/'snapshot'/name).read_text(),name
        files[name]=(directory/'first/tree'/name).read_text()
        assert hashlib.sha256(files[name].encode()).hexdigest()==parent['source_hashes'][name],name
    helper=generator('generate-rmg-position-family.py')
    original=helper.definition((HOMM3_DIR/'src/rmg.cpp').read_text(),'type_random_map::getMapItem',parameters='TRmgMapPosition point')
    selected=helper.definition(files['src/rmg.cpp'],'type_random_map::getMapItem',parameters='TRmgMapPosition point')
    declarations=''.join('    const int& '+axis+' = point.get'+axis.upper()+'();\n' for axis in 'xyz')
    assert selected.count(declarations)==1
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    options=[dict(name='current_control',replace=original)]
    for order in itertools.permutations('xyz'):
        body=selected.replace(declarations,''.join('    const int& '+axis+' = point.get'+axis.upper()+'();\n' for axis in order))
        for result in ('direct','named'):
            changed=body if result=='direct' else body.replace('    return getMapItem(x, y, z);','    TRmgMapItem* item = getMapItem(x, y, z);\n    return item;')
            options.append(dict(name=''.join(order)+'+'+result,replace=changed,extra_edits=[dict(source='include/rmg.h',find=header,replace=files['include/rmg.h'])]))
    payload=dict(schema=1,source='src/rmg.cpp',evidence=__doc__,parent_checkpoint=str(args.checkpoint.resolve()),
                 units=['rmg','rmg_support','rmg_terrain','tiles','singleselectionpopups','singleselectionwindow','scenarioinfo'],
                 axes=[dict(name='projection_order',find=original,options=options)])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('13 coordinate projection order/return controls from '+parent['id'])


if __name__=='__main__':main()
