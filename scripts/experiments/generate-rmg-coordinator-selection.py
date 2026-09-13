#!/usr/bin/env python3
"""Recover generation's selected-index lifetime without caching live templates.

Retail 0x549930 agrees in all 69 blocks and uses EBX for the selected index
across string assignment; the candidate uses EDI. Earlier signed-index and
sum-order controls were neutral. Test immutable value/reference selection
and real preparation scopes. Every later m_templates[selected] lookup remains
live: the existing callback oracle deliberately mutates the template vector
and rejects replacing those queries with the earlier mapTemplate snapshot.
Keep both slot-buffer memsets, nine-entry mapping fill and all pipeline calls.
This Complete-only coordinator has no Dreamcast counterpart.
"""
import argparse
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    original=generator('generate-rmg-coordinator-family.py').definition(source)
    declaration='    unsigned int selected = rand() % m_templates.size();\n'
    declarations=[('mutable_unsigned',declaration)]
    for label,typename in (('constant_unsigned','const unsigned int'),('constant_signed','const int'),
                           ('borrowed_unsigned','const unsigned int&'),('borrowed_signed','const int&')):
        declarations.append((label,'    '+typename+' selected = rand() % m_templates.size();\n'))
    declarations += [('direct_initialized','    unsigned int selected(rand() % m_templates.size());\n'),
                     ('assigned','    unsigned int selected;\n    selected = rand() % m_templates.size();\n'),
                     ('mutable_signed','    int selected = rand() % m_templates.size();\n')]
    assert original.count(declaration)==1
    options=[]
    for scope in ('function','template_preparation','player_mapping'):
        body=original
        if scope=='template_preparation':
            start=body.index(declaration)
            end=body.index('    paintZoneTerrain();',start)
            region=body[start:end]
            body=body[:start]+'    {\n'+region+'    }\n'+body[end:]
            body=body.replace('    paintZoneTerrain();\n    for (zone =', '    paintZoneTerrain();\n    for (unsigned int zone =',1)
        elif scope=='player_mapping':
            start=body.index('    int players[8];')
            end=body.index('    initializeZones(',start)
            body=body[:start]+'    {\n'+body[start:end]+'    }\n'+body[end:]
        for label,new in declarations:
            options.append(dict(name=scope+'+'+label,replace=body.replace(declaration,new,1)))
    assert options[0]['replace']==original
    payload=dict(schema=1,source='src/rmg.cpp',units=['rmg'],evidence=__doc__,axes=[dict(name='selection_lifetime',find=original,options=options)])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('24 generation selection/scoping controls')


if __name__=='__main__':main()
