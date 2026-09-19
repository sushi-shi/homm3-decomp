#!/usr/bin/env python3
"""Canonical neighbour query: direction index versus consumed offset reference.

Retail refresh carries a TPoint table cursor through its neighbour pass, but
the authored helper currently owns the index-to-offset lookup. Its original
signature is unknown and it has no retained retail body. Cross that ownership
with a real pointer traversal in both callers, preserving mask indexes, live
original-point reads, named converted neighbour, proxy factory and query order.
"""
import argparse
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator


def model(source, header, reference, traversal):
    extract=generator('generate-rmg-position-family.py').definition
    declaration='int getNeighbourLand(const TRmgGridPoint& point, unsigned int direction);'
    if reference:
        assert header.count(declaration)==1
        header=header.replace(declaration,declaration.replace('unsigned int direction','const TPoint& offset'))
        original=extract(source,'TRmgLinePainterInterface::getNeighbourLand')
        body=original.replace('unsigned int direction','const TPoint& offset').replace('g_tileDirections[direction]','offset')
        source=source.replace(original,body)
    for name in ('refreshRmgLinePoint','TRmgLineWalker::paintPoint'):
        original=extract(source,name)
        body=original
        arg='g_tileDirections[direction]' if reference else 'direction'
        if traversal:
            indent='    ' if name=='refreshRmgLinePoint' else '        '
            loop=indent+'for ('+('unsigned int ' if name=='refreshRmgLinePoint' else '')+'direction = 0; direction < TILE_DIR_COUNT; ++direction) {'
            assert loop in body
            replacement=(indent+'for (const TPoint* offset = g_tileDirections;\n'+indent+'     offset != g_tileDirections + TILE_DIR_COUNT; ++offset) {\n'+indent+'    '+('unsigned int ' if name=='refreshRmgLinePoint' else '')+'direction = offset - g_tileDirections;')
            body=body.replace(loop,replacement,1)
            if reference:arg='*offset'
        old='painter->getNeighbourLand(point, direction)'
        assert body.count(old)==1
        body=body.replace(old,'painter->getNeighbourLand(point, '+arg+')')
        source=source.replace(original,body)
    return source,header


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg_terrain.cpp').read_text()
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    options=[]
    for reference,traversal in ((False,False),(True,False),(False,True),(True,True)):
        body,decl=model(source,header,reference,traversal)
        option=dict(name=('offset_reference' if reference else 'direction_index')+'+'+('pointer_walk' if traversal else 'index_walk'),replace=body)
        if decl!=header:option['extra_edits']=[dict(source='include/rmg.h',find=header,replace=decl)]
        options.append(option)
    payload=dict(schema=1,units=['rmg','rmg_support','rmg_terrain','scenarioinfo','singleselectionpopups','singleselectionwindow','tiles'],evidence=__doc__,axes=[dict(name='neighbour_offset_owner',source='src/rmg_terrain.cpp',find=source,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    _,sources,axes=load_manifest(args.output,HOMM3_DIR)
    assert render(sources,axes,(0,))['src/rmg_terrain.cpp']==source
    print('four neighbour-interface/traversal states, both callers, seven consumers')


if __name__=='__main__':main()
