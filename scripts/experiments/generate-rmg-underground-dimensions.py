#!/usr/bin/env python3
"""Use the recovered dimension member in underground's borrowed-map setup.

Retail 0x5439e0 agrees in all 36 block sizes and ten calls, with only B0
load/store scheduling different. The map now owns one TRmgMapPosition size;
the former TPoint projection is no longer the only source model for passing
its width/height to the canonical buffer-view constructor. Cross borrowed,
const-value and mutable-value dimension bindings, placed before/after the
plane query and consumed by the view alone or by the first scan as well.
Keep the shared scan position, map/brush lifetime, two passes, all policies,
progress and cleanup order. RMG has no Dreamcast counterpart; locals are
hypotheses, not recovered text. Do not flatten either canonical constructor.
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
    original=generator('generate-rmg-position-family.py').definition(source,'type_random_map_generator::decorateUnderground')
    old='    TPoint dimensions(m_map.m_size.m_x, m_map.m_size.m_y);\n'
    query='    TRmgMapItem* item = m_map.getMapItem(0, 0, scan.m_z);\n'
    assert original.count(old)==original.count(query)==1
    options=[dict(name='point_projection_control',replace=original)]
    for binding,typename in (('borrowed','const TRmgMapPosition&'),('constant','const TRmgMapPosition'),('value','TRmgMapPosition')):
        declaration='    '+typename+' dimensions = m_map.m_size;\n'
        for location in ('after_query','before_query'):
            body=original.replace(old,declaration if location=='after_query' else '')
            if location=='before_query':body=body.replace(query,declaration+query)
            for use in ('view_only','view_and_scan'):
                candidate=body
                if use=='view_and_scan':
                    candidate=candidate.replace('scan.m_y < m_map.m_size.m_y','scan.m_y < dimensions.m_y').replace('scan.m_x < m_map.m_size.m_x','scan.m_x < dimensions.m_x')
                options.append(dict(name=binding+'+'+location+'+'+use,replace=candidate))
    payload=dict(schema=1,source='src/rmg.cpp',units=['rmg'],evidence=__doc__,axes=[dict(name='underground_dimensions',find=original,options=options)])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    load_manifest(args.output,HOMM3_DIR)
    print('13 underground dimension ownership/lifetime controls')


if __name__=='__main__':main()
