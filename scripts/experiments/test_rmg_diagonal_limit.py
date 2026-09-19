#!/usr/bin/env python3
"""Actual diagonal bodies versus independent signed-coordinate/read oracle.

Valid grids: dimensions1..8, every point, all four flips, eight terrain fields.
Additional helper-domain tests approach INT_MAX without signed overflow.
Unsigned-to-int conversion is the target's two's-complement implementation
behavior; native host assumptions are checked explicitly. No claim is made for
zero-sized grids, values outside the grid, native VC6 layout or x86 EH.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def record(text,name):
    start=text.index('struct '+name+' {');end=text.index('\n};',start)+3
    return text[start:end]+'\n'

FIXTURE=r'''
struct rmgTerrainPainter {
    TRmgGridPoint m_size;
    unsigned seed;
    std::vector<TRmgGridPoint> reads;
    TRmgPackedTerrainCell cell;
    unsigned int getWidth() const;
    unsigned int getHeight() const;
    int getTerrain(const TRmgGridPoint& point);
    TRmgPackedTerrainCell* getPackedCell(const TRmgGridPoint& point) {
        assert(point.m_x < m_size.m_x && point.m_y < m_size.m_y);
        reads.push_back(point);
        cell.m_terrain = (point.m_x*17 + point.m_y*13 + seed*(point.m_x+point.m_y+1)) % 3;
        return &cell;
    }
    unsigned char checkFirstDiagonal(const TRmgGridPoint&,const TRmgTerrainFlip&);
    unsigned char checkSecondDiagonal(const TRmgGridPoint&,const TRmgTerrainFlip&);
};
long clipped(long value,unsigned extent) {
    if(value<0)return 0;
    if(value>=long(extent))return extent-1;
    return value;
}
int land(unsigned x,unsigned y,unsigned seed) {
    return (x*17+y*13+seed*(x+y+1))%3;
}
int main() {
    assert(sizeof(int)==4 && sizeof(unsigned)==4);
    assert(static_cast<int>(UINT_MAX)==-1);
    unsigned dims[]={1,2,3,8,36,72,144,INT_MAX-2U,INT_MAX-1U};
    for(unsigned d=0;d<sizeof(dims)/sizeof(dims[0]);++d) {
        unsigned width=dims[d];
        unsigned points[]={0,width/2,width-1};
        for(unsigned i=0;i<3;++i)for(int offset=-2;offset<=2;++offset) {
            unsigned point=points[i];
            int expected=int(clipped(static_cast<long long>(point)+offset,width));
            assert(tLimit(0,static_cast<int>(point)+offset,static_cast<int>(width)-1)==expected);
            assert(limit(0,static_cast<int>(point)+offset,static_cast<int>(width)-1)==expected);
            assert(tLimit<int>(0,point+offset,width-1)==expected);
            assert(limit(0,point+offset,width-1)==expected);
        }
    }
    for(unsigned w=1;w<=8;++w)for(unsigned h=1;h<=8;++h)
    for(unsigned x=0;x<w;++x)for(unsigned y=0;y<h;++y)
    for(unsigned fx=0;fx<2;++fx)for(unsigned fy=0;fy<2;++fy)
    for(unsigned seed=0;seed<8;++seed)for(int which=0;which<2;++which) {
        rmgTerrainPainter p;p.m_size=TRmgGridPoint(w,h);p.seed=seed;
        TRmgGridPoint point(x,y);TRmgTerrainFlip flip(fx,fy);
        std::vector<TRmgGridPoint> expected;expected.push_back(point);
        int terrain=land(x,y,seed);bool result;
        if(!which) {
            int dx=fx?1:-1;int dy=fy?-1:1;
            unsigned nx=unsigned(clipped(long(x)+dx,w));
            unsigned ny=unsigned(clipped(long(y)+dy,h));
            expected.push_back(TRmgGridPoint(nx,ny));result=land(nx,ny,seed)==terrain;
            if(!result) {
                nx=unsigned(clipped(long(x)-dx,w));ny=unsigned(clipped(long(y)-dy,h));
                expected.push_back(TRmgGridPoint(nx,ny));result=land(nx,ny,seed)==terrain;
            }
        } else {
            int dx=fx?-2:2;int dy=fy?-2:2;
            unsigned nx=unsigned(clipped(long(x)+dx,w));
            expected.push_back(TRmgGridPoint(nx,y));result=land(nx,y,seed)!=terrain;
            if(!result) {
                unsigned ny=unsigned(clipped(long(y)+dy,h));
                expected.push_back(TRmgGridPoint(x,ny));result=land(x,ny,seed)!=terrain;
            }
        }
        unsigned char got=which?p.checkSecondDiagonal(point,flip):p.checkFirstDiagonal(point,flip);
        assert(bool(got)==result && p.reads.size()==expected.size());
        for(unsigned i=0;i<expected.size();++i)
            assert(p.reads[i].m_x==expected[i].m_x && p.reads[i].m_y==expected[i].m_y);
        assert(point.m_x==x && point.m_y==y && flip.m_flipX==fx && flip.m_flipY==fy);
    }
}
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('manifest',type=Path);args=parser.parse_args()
    _,originals,axes=load_manifest(args.manifest,HOMM3_DIR)
    header=(HOMM3_DIR/'include/rmg.h').read_text();terrain_header=(HOMM3_DIR/'include/rmg_terrain.h').read_text()
    source=(HOMM3_DIR/'src/rmg_terrain.cpp').read_text();includes=(HOMM3_DIR/'include/includes.h').read_text()
    extract=generator('generate-rmg-position-family.py').definition
    text='#include <cassert>\n#include <climits>\n#include <vector>\n#define VA(a,b)\n#define DATA(a)\n'
    for name in ('TRmgVector','TPoint'):text+=record(header,name)
    start=header.index('template<class Coordinate> struct TRmgCoordinatePoint {')
    end=header.index('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')+len('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')
    text+=header[start:end]+'\n'
    for name in ('rmgTerrainTile','TRmgTerrainFlip','TRmgPackedTerrainCell'):text+=record(terrain_header,name)
    text+=generator('generate-rmg-clamp-structure-family.py').definition(includes)+'\n'
    text+=extract(includes,'limit')+'\n'+FIXTURE
    for name in ('getTerrain','getWidth','getHeight'):text+=extract(source,'rmgTerrainPainter::'+name)+'\n'
    cases=[]
    for i in range(len(axes[0].options)):
        src=render(originals,axes,(i,))['src/rmg_terrain.cpp']
        body='\n'.join(extract(src,'rmgTerrainPainter::'+name) for name in ('checkFirstDiagonal','checkSecondDiagonal'))
        cases.append((str(i),text+body,True))
    positive=cases[-1][1]
    bad=[('wrong_zero','return minimum;','return maximum;'),
         ('wrong_upper','return maximum;','return value;'),
         ('wrong_axis','TPoint(2, 2), TPoint(-2, 2)','TPoint(-2, 2), TPoint(2, 2)'),
         ('wrong_flip','(flip.m_flipY << 1) | flip.m_flipX','(flip.m_flipX << 1) | flip.m_flipY'),
         ('wrong_early','if (getTerrain(nearby) != terrain)','if (getTerrain(nearby) == terrain)')]
    for label,old,new in bad:
        assert old in positive
        cases.append((label,positive.replace(old,new),False))
    with tempfile.TemporaryDirectory(prefix='rmg-diagonal-limit-') as tmp:
        for label,cpp,expected in cases:
            path=Path(tmp)/label;path.with_suffix('.cpp').write_text(cpp)
            subprocess.run(['g++','-std=c++98','-O1','-fsanitize=undefined',str(path.with_suffix('.cpp')),'-o',str(path)],check=True)
            proc=subprocess.run([str(path)],capture_output=True,text=True)
            assert (proc.returncode==0 and not proc.stderr)==expected,(label,proc.stderr)
    print('%d actual models × 82944 diagonal cases +135 boundary-conversion cases;5 wrong controls rejected;UBSan clean'%len(axes[0].options))

if __name__=='__main__':main()
