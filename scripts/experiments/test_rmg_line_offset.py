#!/usr/bin/env python3
"""Actual neighbour-helper and both caller bodies against independent event fixtures."""
import argparse
from pathlib import Path
import subprocess
import tempfile
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def record(text, opening):
    start=text.index(opening)
    return text[start:text.index('\n};',start)+3]


def program(source,header,walker=False):
    fixture=generator('test_rmg_line_selection.py')
    extract=generator('generate-rmg-position-family.py').definition
    terrain=(HOMM3_DIR/'include/rmg_terrain.h').read_text()
    start=header.index('template<class Coordinate> struct TRmgCoordinatePoint {')
    end=header.index('\nstruct TRmgZoneBounds {',start)
    text=fixture.PRELUDE+header[start:end]+'\ntemplate<class Coordinate>\n'+extract(source,'TRmgCoordinatePoint<Coordinate>::TRmgCoordinatePoint')+'\n'
    text+=record(terrain,'struct rmgTerrainTile {')+'\n'
    neighbour=extract(source,'TRmgLinePainterInterface::getNeighbourLand')
    if not walker:
        boundaries=fixture.BOUNDARIES
        if 'const TPoint& offset' in neighbour:
            boundaries=boundaries.replace('getNeighbourLand(const TRmgGridPoint&,unsigned);','getNeighbourLand(const TRmgGridPoint&,const TPoint&);')
        text+=boundaries
        names=('TRmgLinePainterTile::TRmgLinePainterTile','TRmgLinePainterTile::getLand','TRmgLinePainterTile::getTile','TRmgLinePainterTile::setTile','TRmgLinePainterInterface::at')
        text+='\n'.join(extract(source,n) for n in names)+'\n'+neighbour
        text+='\n#define rand fixtureRand\n'+extract(source,'refreshRmgLinePoint')+'\n#undef rand\n'+fixture.CHECKS
        return text+'\nint main(){return check(refreshRmgLinePoint)?0:1;}\n'
    text=text.replace('enum { TILE_DIR_COUNT=8 };','enum { TILE_DIR_NORTH, TILE_DIR_NORTHEAST, TILE_DIR_EAST, TILE_DIR_SOUTHEAST, TILE_DIR_SOUTH, TILE_DIR_SOUTHWEST, TILE_DIR_WEST, TILE_DIR_NORTHWEST, TILE_DIR_COUNT };')
    text='#include <algorithm>\n'+text
    start=header.index('class TRmgLinePainterInterface {')
    end=header.index('SIZE(TRmgLinePainterTile,',start)
    text+='struct TRmgLinePatternTable {};\nstruct TRmgLinePainterTile;\n'+header[start:end]+'\n'
    text+=record(header,'struct TRmgGridRectangle {')+'\n'+record(header,'class TRmgLineWalker {')+'\n'
    text+='void refreshRmgLinePoint(TRmgLinePainterInterface*,const TRmgGridPoint&);\n'
    text+=extract((HOMM3_DIR/'src/tiles.cpp').read_text(),'buildTileNeighbourMask').replace('__fastcall ','')+'\n'
    text+=extract((HOMM3_DIR/'src/rmg_support.cpp').read_text(),'TRmgLinePainterInterface::TRmgLinePainterInterface')+'\n'
    names=('TRmgLinePainterTile::TRmgLinePainterTile','TRmgLinePainterTile::getLand','TRmgLinePainterTile::setTile','TRmgLinePainterTile::isBlocked','TRmgLinePainterTile::setOverlay','TRmgLinePainterInterface::at','TRmgLinePainterInterface::getNeighbourLand','TRmgGridRectangle::TRmgGridRectangle','clearRmgLineRectangle','TRmgLineWalker::TRmgLineWalker','TRmgLineWalker::paintPoint')
    text+='\n'.join(extract(source,n) for n in names)+'\n'
    oracle=(HOMM3_DIR/'scripts/experiments/rmg-line-painting-oracle.cpp').read_text()
    oracle=oracle.replace('TRmgGridRectangle rectangle(TRmgGridPoint(x, y), w, h);','TRmgGridRectangle rectangle(TRmgGridPoint(x, y), TRmgGridPoint(w, h));')
    return text+oracle+'\nint main(){return check();}\n'


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('manifest',type=Path)
    args=p.parse_args()
    _,sources,axes=source_families.load_manifest(args.manifest,HOMM3_DIR)
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    models=[source_families.render(sources,axes,(i,)) for i in range(len(axes[0].options))]
    tests=[]
    for i,m in enumerate(models):
        for walker in (False,True):tests.append((str(i)+('-walker' if walker else '-refresh'),program(m['src/rmg_terrain.cpp'],m.get('include/rmg.h',header),walker),True))
    base=models[0]['src/rmg_terrain.cpp']
    for name,old,new in [('land','== oldType','!= oldType'),('offset','point + g_tileDirections[direction]','point + g_tileDirections[(direction + 1) % TILE_DIR_COUNT]'),('mask','matches[direction] = 0;','matches[direction] = 1;')]:
        assert old in base
        tests.append(('wrong-'+name,program(base.replace(old,new),header),False))
    # The walker independently checks second-pass direction and nonpainting gates.
    tests.append(('wrong-walker',program(base.replace('oldType == m_riverType || tile.isBlocked()','oldType == m_riverType && tile.isBlocked()'),header,True),False))
    with tempfile.TemporaryDirectory(prefix='rmg-line-offset-oracle-') as folder:
        for name,text,positive in tests:
            src=Path(folder)/(name+'.cpp');binary=Path(folder)/name;src.write_text(text)
            subprocess.run(['g++','-std=c++98','-O1','-fsanitize=undefined','-fno-sanitize-recover=all',str(src),'-o',str(binary)],check=True)
            run=subprocess.run([str(binary)],capture_output=True,text=True)
            assert (run.returncode==0)==positive,(name,run.returncode,run.stderr)
            if positive:assert not run.stderr,(name,run.stderr)
    print('%d actual models: refresh 43200 cases/model plus independent walker/border event cases; four wrong controls rejected; UBSan clean'%len(models))


if __name__=='__main__':main()
