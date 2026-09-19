#!/usr/bin/env python3
"""Canonical grid ordering across its retained body and inlined tree lookup.

Retail find0x5b7fc0 agrees on all6 CFG blocks and lower_bound call, but loads
node-y before key-y. The retained free comparator0x5b8ca0 is already32B exact.
Test natural lexicographic control forms, operand order and actual coordinate
read interfaces together; audit all terrain consumers and the retained body.
No vendor STL edit, altered type, fake inline or new helper is introduced.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    signature=original[:original.index('\n{')]
    for flow,binding,order in itertools.product(range(5),range(4),range(3)):
        fields={a+c:a+'.m_'+c for a in ('left','right') for c in ('x','y')}
        prefix=''
        if binding==1:
            fields={a+c:a+'.get'+c.upper()+'()' for a in ('left','right') for c in ('x','y')}
        elif binding in (2,3):
            kind='const Coordinate' if binding==2 else 'const Coordinate&'
            prefix=f'    {kind} leftY = left.m_y;\n    {kind} rightY = right.m_y;\n'
            fields.update(lefty='leftY',righty='rightY')
        def less(c):
            a,b=fields['left'+c],fields['right'+c]
            return b+' > '+a if order==1 else a+' < '+b
        def greater(c):
            a,b=fields['left'+c],fields['right'+c]
            return b+' < '+a if order==1 else a+' > '+b
        def equal(op='=='):
            a,b=fields['lefty'],fields['righty']
            return (b+' '+op+' '+a) if order in (1,2) else (a+' '+op+' '+b)
        if flow==0:
            statements='    return '+less('y')+' || ('+equal()+' && '+less('x')+');\n'
        elif flow==1:
            statements='    return '+equal()+' ? '+less('x')+' : '+less('y')+';\n'
        elif flow==2:
            statements='    if ('+less('y')+')\n        return true;\n    if ('+equal()+')\n        return '+less('x')+';\n    return false;\n'
        elif flow==3:
            statements='    if ('+equal('!=')+')\n        return '+less('y')+';\n    return '+less('x')+';\n'
        else:
            statements='    if ('+greater('y')+')\n        return false;\n    if ('+equal()+')\n        return '+less('x')+';\n    return true;\n'
        yield dict(name=f'flow_{flow}+binding_{binding}+order_{order}',replace=signature+'\n{\n'+prefix+statements+'}')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg_terrain.cpp').read_text()
    original=generator('generate-rmg-position-family.py').definition(source,'operator<')
    options=list(variants(original))
    assert len(options)==len({o['replace'] for o in options})==60
    assert any(option['replace']==original for option in options)
    options.sort(key=lambda option: option['replace']!=original)
    assert options[0]['replace']==original
    payload=dict(schema=1,units=['rmg_terrain'],evidence=__doc__,axes=[dict(
        name='grid_lexicographic_order',source='src/rmg_terrain.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    source_families.load_manifest(args.output,HOMM3_DIR)
    print('60 canonical lexicographic comparator states')


if __name__=='__main__':
    main()
