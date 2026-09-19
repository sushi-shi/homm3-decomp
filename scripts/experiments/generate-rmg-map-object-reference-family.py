#!/usr/bin/env python3
"""Map placement object interface × consumed cell-mask coordinate.

Retail creates pointer temporaries before both STL reference-taking APIs. An
object reference naturally supplies &object prvalues; generator virtual pointer
interfaces stay unchanged. All five direct map callers dereference a valid
object: null was already outside this unconditionally dereferencing map body.
Keep both calibrated pointer parents and score every actual header consumer.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('context',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    for folder in ('src','include','vendor'):
        for path in (args.context/'snapshot'/folder).rglob('*'):
            if path.is_file():
                assert path.read_bytes()==(HOMM3_DIR/path.relative_to(args.context/'snapshot')).read_bytes(),path
    checkpoint=json.loads((args.context/'checkpoint.json').read_text())
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    probe=generator('probe-rmg-map-object-reference.py')
    extract=generator('generate-rmg-position-family.py').definition
    options=[];parents=[]
    for choice in (0,9):
        row=next(r for r in checkpoint['elites'] if r['choices']==[choice])
        path=args.context/'candidates'/row['id']
        first=json.loads((path/'first/result.json').read_text());repeat=json.loads((path/'repeat/result.json').read_text())
        for key in ('scores','object_hash','source_hashes','choices'):
            assert first[key]==repeat[key]==row[key]
        parent=(path/'first/tree/src/rmg.cpp').read_text()
        assert parent==(path/'repeat/tree/src/rmg.cpp').read_text()
        assert hashlib.sha256(parent.encode()).hexdigest()==row['source_hashes']['src/rmg.cpp']
        parents.append({key:row[key] for key in ('id','choices','source_hashes','object_hash')})
        for reference in (False,True):
            edited=parent
            if reference:
                body=extract(edited,'type_random_map::addObject')
                edited=edited.replace(body,probe.reference_body(body))
                sites=re.findall(r'\bm_map\.addObject\((\w+), position\)',edited)
                assert sites==['guard','object','object','object','questObject'],sites
                edited=re.sub(r'\bm_map\.addObject\((\w+), position\)',r'm_map.addObject(*\1, position)',edited)
            options.append(dict(name=('cell_mask' if choice else 'scalar')+('reference' if reference else 'pointer'),
                replace=probe.reference_header(header) if reference else header,
                extra_edits=[dict(source='src/rmg.cpp',find=source,replace=edited)]))
    runner=Path(__file__).with_name('run-rmg-map-object-reference-family.py')
    payload=dict(schema=1,units=['rmg','rmg_support','rmg_terrain','scenarioinfo',
        'singleselectionpopups','singleselectionwindow','tiles'],evidence=__doc__,
        parent_context=args.context.name,reproduced_parents=parents,
        diagnostic_runner_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
        axes=[dict(name='map_object_interface',source='include/rmg.h',find=header,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    _,sources,axes=load_manifest(args.output,HOMM3_DIR)
    assert render(sources,axes,(0,))['src/rmg.cpp']==source
    for choice in range(4):
        edited=render(sources,axes,(choice,))
        assert edited['include/rmg.h'].count('virtual void addObject(type_object* object, TRmgMapPosition position);')==2
        assert extract(edited['src/rmg.cpp'],'type_random_map::canPlaceObject')==extract(source,'type_random_map::canPlaceObject')
    print('4 interface/mask models; five direct callers; seven actual header consumers')

if __name__=='__main__':main()
