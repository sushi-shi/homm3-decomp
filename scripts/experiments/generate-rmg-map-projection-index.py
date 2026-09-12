#!/usr/bin/env python3
"""Cross the retained guard-call projection with canonical scalar index forms.

The reproduced projection parent restores both monolith map-position calls
and occupancy-size expansions, but its retained lookup loads level through
ESI. Its six projection-order controls alone cannot recover the 39-byte
callee. Compare the existing sixty row/index/result lifetime forms inside the
one canonical scalar lookup, with that parent's accessor interface unchanged.
No arithmetic is pasted into a caller, no new helper is added, and both map
ABIs remain fixed. Keep the current source as a separate first control and
verify the parent's first/repeat hashes and the relevant original declarations.
"""
import argparse
import hashlib
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
    parent=parents[0];directory=root/'candidates'/parent['id']
    repeat=json.loads((directory/'repeat/result.json').read_text())
    for key in ('object_hash','scores','source_hashes','choices'):assert repeat[key]==parent[key],key
    files={}
    for name in ('src/rmg.cpp','include/rmg.h'):
        files[name]=(directory/'first/tree'/name).read_text()
        assert hashlib.sha256(files[name].encode()).hexdigest()==parent['source_hashes'][name],name
    helper=generator('generate-rmg-position-family.py')
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    assert header==(root/'snapshot/include/rmg.h').read_text()
    name='type_random_map::getMapItem'
    original=helper.definition(source,name,parameters='TRmgMapPosition point')
    assert original==helper.definition((root/'snapshot/src/rmg.cpp').read_text(),name,parameters='TRmgMapPosition point')
    projected=helper.definition(files['src/rmg.cpp'],name,parameters='TRmgMapPosition point')
    scalar=generator('generate-rmg-map-accessor-family.py')
    formula=scalar.definition(header)
    assert formula==scalar.definition(files['include/rmg.h'])
    options=[dict(name='current_control',replace=header)]
    for label,body in scalar.variants(formula):
        options.append(dict(name=label,replace=files['include/rmg.h'].replace(formula,body,1),
                            extra_edits=[dict(source='src/rmg.cpp',find=original,replace=projected)]))
    payload=dict(schema=1,source='include/rmg.h',evidence=__doc__,parent_checkpoint=str(args.checkpoint.resolve()),
                 units=['rmg','rmg_support','rmg_terrain','tiles','singleselectionpopups','singleselectionwindow','scenarioinfo'],
                 axes=[dict(name='projected_scalar_index',find=header,options=options)])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('61 retained-projection/scalar-index controls')


if __name__=='__main__':main()
