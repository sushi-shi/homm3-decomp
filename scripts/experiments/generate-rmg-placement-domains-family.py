#!/usr/bin/env python3
"""Four placement models coupling mask-domain ownership and trigger translation.

Retail walks unsigned ascending mask indices alongside descending signed world
coordinates. The existing unsigned grid point can own those object-local mask
coordinates without changing live dimension reads or mask checks. Existing
position compound subtraction can translate the trigger using an explicitly
constructed canonical TPoint: TObjectType::TPoint is a distinct source type.
No new helper, declaration, inline marker, cached extent or range guard.
"""
import argparse
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR/'src/rmg.cpp').read_text()
    extract = generator('generate-rmg-position-family.py').definition
    originals = {name: extract(source,'type_random_map::'+name)
                 for name in ('isPlacementBlocked','addObject','canPlaceObject')}
    options = []
    for mask_record, translation in ((False,False),(True,False),(False,True),(True,True)):
        edited = source
        for name in ('isPlacementBlocked','addObject'):
            body = originals[name]
            if mask_record:
                body = body.replace('    TRmgMapPosition nearby = position;',
                    '    TRmgGridPoint maskPoint;\n    TRmgMapPosition nearby = position;')
                assert body.count('unsigned int y') == body.count('unsigned int x') == 1
                body = body.replace('unsigned int y','y').replace('unsigned int x','x')
                for coordinate in ('x','y'):
                    body = re.sub(r'\b'+coordinate+r'\b', 'maskPoint.m_'+coordinate, body)
            edited = edited.replace(originals[name],body)
        body = originals['canPlaceObject']
        if translation:
            old = ('    int x = position.m_x - prototype.m_triggerCell.m_x;\n'
                   '    int y = position.m_y - prototype.m_triggerCell.m_y;\n'
                   '    ++y;\n    TRmgMapPosition entrance(x, y, position.m_z);\n'
                   '    if (y >= m_mapHeight)')
            new = ('    TRmgMapPosition entrance = position;\n'
                   '    entrance -= TPoint(prototype.m_triggerCell.m_x, prototype.m_triggerCell.m_y);\n'
                   '    ++entrance.m_y;\n    if (entrance.m_y >= m_mapHeight)')
            assert body.count(old) == 1
            body = body.replace(old,new)
        edited = edited.replace(originals['canPlaceObject'],body)
        options.append(dict(name=('mask_record' if mask_record else 'scalar_mask')+'+'+
            ('compound_translation' if translation else 'scalar_trigger'),replace=edited))
    payload = dict(schema=1,units=['rmg','rmg_support','rmg_terrain'],evidence=__doc__,
        axes=[dict(name='placement_coordinate_domains',source='src/rmg.cpp',find=source,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    _,loaded,axes=load_manifest(args.output,HOMM3_DIR)
    for i in range(4):
        produced=render(loaded,axes,(i,))['src/rmg.cpp']
        for name in originals:
            assert produced.count('type_random_map::'+name+'(')==1
    print('4 finite states; two mask owners and one canonical trigger translation, all3 RMG units scored')


if __name__=='__main__':
    main()
