#!/usr/bin/env python3
"""Recover the four-value midpoint construction at the island-mask calls.

Retail 0x53ed00 reserves a 16-byte outgoing midpoint record and stores four
edge samples into it before copying the nine-dword region. That is positive
evidence for keeping the record ABI, rather than replacing it with four
scalar formal arguments. Its fields occupy separate registers at the initial
call. Compare one persistent record with four scalar sample locals that feed
a canonical four-value constructor at each call. The ordinary constructor
lives beside the subdivision helper, before its only caller; no inline pin
or pasted constructor body is used. Cross genuine member initializers/body
stores and the scalar declaration orders suggested by record and draw order.
Both retained subdivision and mask-generator bodies must be reviewed.
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
    source=(HOMM3_DIR/'src/rmg.cpp').read_text();header=(HOMM3_DIR/'include/rmg.h').read_text()
    helper=generator('generate-rmg-position-family.py')
    caller=helper.definition(source,'generateRmgIslandMask')
    subdivide=helper.definition(source,'subdivideRmgNoiseRegion')
    begin=header.index('struct TRmgNoiseMidpoints {');end=header.index('\n};',begin)+3
    declaration=header[begin:end]
    parameters='int minYValue, int minXValue, int maxYValue, int maxXValue'
    changed_decl=declaration[:-3]+'\n\n    TRmgNoiseMidpoints('+parameters+');\n};'
    fields=('minYValue','minXValue','maxYValue','maxXValue')
    old='    TRmgNoiseMidpoints edges = { 0, 0, 0, 0 };'
    assert caller.count(old)==1 and caller.count(', patch, edges);')==2
    options=[dict(name='aggregate_record_control',replace=caller)]
    for construction in ('initializers','stores'):
        constructor='TRmgNoiseMidpoints::TRmgNoiseMidpoints('+parameters+')\n'
        if construction=='initializers':constructor+='    : '+', '.join('m_'+field+'('+field+')' for field in fields)+'\n{\n}'
        else:constructor+='{\n'+''.join('    m_'+field+' = '+field+';\n' for field in fields)+'}'
        for storage,order in (('record',None),('record_order',fields),('draw_order',('minXValue','minYValue','maxXValue','maxYValue')),
                              ('axis_pairs',('minXValue','maxXValue','minYValue','maxYValue'))):
            body=caller
            if order is None:body=body.replace(old,'    TRmgNoiseMidpoints edges(0, 0, 0, 0);')
            else:
                body=body.replace(old,'\n'.join('    int '+field+' = 0;' for field in order))
                for field in fields:body=body.replace('edges.m_'+field,field)
                body=body.replace(', patch, edges);',', patch, TRmgNoiseMidpoints('+', '.join(fields)+'));')
            options.append(dict(name=construction+'+'+storage,replace=body,extra_edits=[
                dict(source='include/rmg.h',find=declaration,replace=changed_decl),
                dict(source='src/rmg.cpp',find=subdivide,replace=subdivide+'\n\n'+constructor)]))
    payload=dict(schema=1,source='src/rmg.cpp',evidence=__doc__,
                 units=['rmg','rmg_support','rmg_terrain','tiles','singleselectionpopups','singleselectionwindow','scenarioinfo'],
                 axes=[dict(name='noise_edge_construction',find=caller,options=options)])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(payload,indent=2)+'\n');load_manifest(args.output,HOMM3_DIR)
    print('9 midpoint-record construction and sample-local controls')


if __name__=='__main__':main()
