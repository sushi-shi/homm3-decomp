#!/usr/bin/env python3
"""Placement loop entry owns coordinate state only when the loop has work.

Retail0x531ea0 delays its EBX save/world-y setup until nonempty height and
world-x until nonempty width; current source eagerly copies a whole position
and has an inner entry reload trampoline. Keep the adopted object reference,
consumed mask coordinate and all canonical helpers. Compare for versus guarded
do loops, whole position copy versus initialization of the needed y/z fields,
and mask-point ownership immediately before/after the pure map lookup.
The guards replace existing loop-entry tests; no dummy condition is introduced.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def indent(text,amount):
    return ''.join(' '*amount+line+'\n' if line else '\n' for line in text.splitlines())


def variants(original):
    prefix=original[:original.index('    TRmgMapPosition nearby = position;')]
    start=original.index('            TRmgGridPoint maskPoint(x, y);')
    end=original.rindex('\n        }\n    }\n}')
    cell='\n'.join(line[12:] for line in original[start:end].splitlines())+'\n'
    for outer,inner,initial,order in itertools.product(range(2),repeat=4):
        active=cell
        if order:
            before='TRmgGridPoint maskPoint(x, y);\nTRmgMapItem* item = getMapItem(nearby);'
            after='TRmgMapItem* item = getMapItem(nearby);\nTRmgGridPoint maskPoint(x, y);'
            assert active.count(before)==1
            active=active.replace(before,after)
        if inner==0:
            inner_loop=('nearby.m_x = position.m_x;\n'
                'for (unsigned int x = 0; x < prototype.getWidth(); ++x, --nearby.m_x) {\n'
                '    if (nearby.m_x < 0 || nearby.m_x >= m_mapWidth)\n        continue;\n'
                +indent(active,4)+'}\n')
        else:
            inner_loop=('unsigned int x = 0;\nif (x < prototype.getWidth()) {\n'
                '    nearby.m_x = position.m_x;\n    do {\n'
                '        if (nearby.m_x >= 0 && nearby.m_x < m_mapWidth) {\n'
                +indent(active,12)+'        }\n        ++x;\n        --nearby.m_x;\n'
                '    } while (x < prototype.getWidth());\n}\n')
        initialization=('TRmgMapPosition nearby = position;\n' if initial==0 else
            'TRmgMapPosition nearby;\nnearby.m_y = position.m_y;\nnearby.m_z = position.m_z;\n')
        if outer==0:
            body=initialization+'for (unsigned int y = 0; y < prototype.getHeight(); ++y, --nearby.m_y) {\n'
            body+='    if (nearby.m_y < 0 || nearby.m_y >= m_mapHeight)\n        continue;\n'+indent(inner_loop,4)+'}\n'
        else:
            body='unsigned int y = 0;\nif (y < prototype.getHeight()) {\n'+indent(initialization,4)
            body+='    do {\n        if (nearby.m_y >= 0 && nearby.m_y < m_mapHeight) {\n'
            body+=indent(inner_loop,12)+'        }\n        ++y;\n        --nearby.m_y;\n'
            body+='    } while (y < prototype.getHeight());\n}\n'
        yield dict(name=f'outer_{outer}+inner_{inner}+initial_{initial}+mask_order_{order}',replace=prefix+indent(body,4)+'}')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    original=generator('generate-rmg-position-family.py').definition(source,'type_random_map::addObject')
    assert 'addObject(type_object& object,' in original
    options=list(variants(original));assert len(options)==16 and options[0]['replace']==original
    payload=dict(schema=1,units=['rmg'],evidence=__doc__,axes=[dict(
        name='placement_loop_entry',source='src/rmg.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('16 meaningful loop-entry/coordinate ownership states')


if __name__=='__main__':main()
