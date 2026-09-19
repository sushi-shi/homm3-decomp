#!/usr/bin/env python3
"""Truth-only byte parameter types at createTreasureObject0x546190.

Retail proves three byte inputs at the retained ABI, but their only uses are
truth tests, so byte width alone does not distinguish bool from unsigned char.
Cross their canonical types with unchanged filtering/container/source bodies;
update exactly one declaration and definition atomically and inspect callers.
No DC counterpart exists. Comparison-only aliases preserve frozen target names;
any adoption requires real source labels and a complete fresh delink.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from experiments._support import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-treasure-create-family.py')
    original = helper.helpers().definition((HOMM3_DIR/helper.SOURCE).read_text(),helper.FUNCTION)
    header = (HOMM3_DIR/'include/rmg.h').read_text()
    start = header.index('    type_object* createTreasureObject(')
    declaration = header[start:header.index(';',start)+1]
    flags = ['primary','allowTerrainDependent','compact']
    options = []
    for types in itertools.product(('unsigned char','bool'),repeat=3):
        body, decl = original, declaration
        for name, typ in zip(flags,types):
            old = 'unsigned char '+name
            assert body.count(old)==decl.count(old)==1
            body=body.replace(old,typ+' '+name);decl=decl.replace(old,typ+' '+name)
        row = dict(name='+'.join(types),replace=body)
        if decl != declaration:
            row['extra_edits']=[dict(source='include/rmg.h',find=declaration,replace=decl)]
        options.append(row)
    runner=Path(__file__).with_name('run-rmg-treasure-flag-family.py')
    payload=dict(schema=1,units=['rmg','rmg_support','rmg_terrain','scenarioinfo','singleselectionpopups','singleselectionwindow','tiles'],
                 evidence=__doc__,diagnostic_runner_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
                 axes=[dict(name='treasure_flag_types',source=helper.SOURCE,find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    source_families.load_manifest(args.output,HOMM3_DIR)
    print('8 canonical byte-flag type states')


if __name__=='__main__':
    main()
