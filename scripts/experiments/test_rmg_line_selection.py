#!/usr/bin/env python3
"""Actual refresh caller and proxy/coordinate helpers against a traced oracle.

Reduced host painter/selector boundaries test query order, neighbour masks,
selection outputs, random draws, table mutation at getTile, and full tile
preservation. They do not claim the retail ABI or selector algorithm is proved.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

PRELUDE=r'''
#include <vector>
#include <cstdio>
#define VA(a,b)
#define SIZE(a,b)
struct TPoint {
    int m_x,m_y;
    TPoint(int x,int y):m_x(x),m_y(y){}
    TPoint& operator+=(const TPoint& p) {m_x+=p.m_x;m_y+=p.m_y;return *this;}
};
TPoint operator+(const TPoint& a,const TPoint& b) {TPoint r=a;return r+=b;}
enum { TILE_DIR_COUNT=8 };
const TPoint g_tileDirections[8]={TPoint(0,-1),TPoint(1,-1),TPoint(1,0),TPoint(1,1),TPoint(0,1),TPoint(-1,1),TPoint(-1,0),TPoint(-1,-1)};
'''
BOUNDARIES=r'''
struct TRmgLinePatternRange {unsigned m_firstIndex,m_valueCount;};
struct TRmgLinePatternTable {unsigned m_patternCount;int* m_patterns;TRmgLinePatternRange m_ranges[9];};
struct TRmgLinePainterTile;
struct TRmgLinePainterInterface {
    TRmgGridPoint m_size;
    TRmgLinePainterInterface(const TRmgGridPoint& s):m_size(s){}
    virtual TRmgLinePatternTable* getPattern(int)=0;
    virtual void setTile(const TRmgGridPoint&,const rmgTerrainTile&)=0;
    virtual void getTile(const TRmgGridPoint&,rmgTerrainTile&)=0;
    virtual int getLand(const TRmgGridPoint&)=0;
    TRmgLinePainterTile at(const TRmgGridPoint&);
    int getNeighbourLand(const TRmgGridPoint&,unsigned);
};
struct TRmgLinePainterTile {
    TRmgLinePainterInterface* m_painter;
    TRmgGridPoint m_point;
    TRmgLinePainterTile(TRmgLinePainterInterface*,const TRmgGridPoint&);
    int getLand();
    void getTile(rmgTerrainTile&);
    void setTile(const rmgTerrainTile&);
};
void buildTileNeighbourMask(unsigned w,unsigned h,unsigned x,unsigned y,unsigned char* out) {
    for(int i=0;i<8;++i) {
        int a=int(x)+g_tileDirections[i].m_x,b=int(y)+g_tileDirections[i].m_y;
        out[i]=a>=0 && b>=0 && unsigned(a)<w && unsigned(b)<h;
    }
}
struct Painter : TRmgLinePainterInterface {
    int kinds[16],mapping[32],selection,flipX,flipY,mutation,randomValue;
    TRmgLinePatternTable table;
    rmgTerrainTile current,written;
    std::vector<int> events;
    Painter(unsigned w,unsigned h,int distribution,int selected,int flips,int mode,int mutate,int random):
        TRmgLinePainterInterface(TRmgGridPoint(w,h)),selection(selected),flipX(flips&1),flipY(flips>>1),mutation(mutate),randomValue(random) {
        for(int i=0;i<16;++i)kinds[i]=(i*(distribution+1)+distribution)%3;
        for(int i=0;i<32;++i)mapping[i]=(i+selection)%9;
        table.m_patterns=mapping;table.m_patternCount=32;
        for(int i=0;i<9;++i){table.m_ranges[i].m_firstIndex=i*3;table.m_ranges[i].m_valueCount=1+i%3;}
        current.m_terrain=101+distribution;current.m_frame=4;
        mapping[4]=mode==1?(selection+1)%9:selection;
        current.m_flipX=flipX^(mode==2);current.m_flipY=flipY^(mode==3);
        written=current;
    }
    void event(int e,const TRmgGridPoint& p){events.push_back(e);events.push_back(p.m_x);events.push_back(p.m_y);}
    int getLand(const TRmgGridPoint& p){event(1,p);return kinds[p.m_y*4+p.m_x];}
    TRmgLinePatternTable* getPattern(int land){events.push_back(2);events.push_back(land);return &table;}
    void getTile(const TRmgGridPoint& p,rmgTerrainTile& tile){
        event(3,p);tile=current;
        if(mutation){mapping[4]=(mapping[4]+1)%9;table.m_ranges[selection].m_firstIndex+=2;}
    }
    void setTile(const TRmgGridPoint& p,const rmgTerrainTile& tile){event(4,p);written=tile;}
};
Painter* active;
void selectRmgLinePattern(const unsigned char* matches,const TRmgLinePatternTable* table,int& pattern,unsigned char& x,unsigned char& y){
    active->events.push_back(5);int bits=0;
    for(int i=0;i<8;++i)bits|=int(matches[i])<<i;
    active->events.push_back(bits);active->events.push_back(table==&active->table);
    pattern=active->selection;x=active->flipX;y=active->flipY;
}
int fixtureRand(){active->events.push_back(6);return active->randomValue;}
'''
CHECKS=r'''
void reference(Painter& painter,const TRmgGridPoint& point){
    int old=painter.getLand(point);
    unsigned char matches[8];
    for(int d=0;d<8;++d){
        int x=int(point.m_x)+g_tileDirections[d].m_x,y=int(point.m_y)+g_tileDirections[d].m_y;
        matches[d]=0;
        if(x>=0 && y>=0 && unsigned(x)<painter.m_size.m_x && unsigned(y)<painter.m_size.m_y)
            matches[d]=painter.getLand(TRmgGridPoint(x,y))==old;
    }
    TRmgLinePatternTable* table=painter.getPattern(old);
    int selected;unsigned char flipX,flipY;
    selectRmgLinePattern(matches,table,selected,flipX,flipY);
    rmgTerrainTile tile;painter.getTile(point,tile);
    bool changed=table->m_patterns[tile.m_frame]!=selected;
    changed=changed || tile.m_flipX!=flipX || tile.m_flipY!=flipY;
    if(changed){
        unsigned random=fixtureRand();
        tile.m_frame=table->m_ranges[selected].m_firstIndex+random%table->m_ranges[selected].m_valueCount;
        tile.m_flipX=flipX;tile.m_flipY=flipY;painter.setTile(point,tile);
    }
}
bool same(const Painter& a,const Painter& b){
    return a.events==b.events && a.written.m_terrain==b.written.m_terrain && a.written.m_frame==b.written.m_frame
        && a.written.m_flipX==b.written.m_flipX && a.written.m_flipY==b.written.m_flipY;
}
bool check(void (*run)(TRmgLinePainterInterface*,const TRmgGridPoint&)){
    for(unsigned w=2;w<=3;++w)for(unsigned h=2;h<=3;++h)
    for(unsigned x=0;x<w;++x)for(unsigned y=0;y<h;++y)
    for(int distribution=0;distribution<3;++distribution)for(int selected=0;selected<9;++selected)
    for(int flips=0;flips<4;++flips)for(int mode=0;mode<4;++mode)
    for(int mutation=0;mutation<2;++mutation)for(int random=0;random<2;++random){
        Painter a(w,h,distribution,selected,flips,mode,mutation,random*32767);
        Painter b(w,h,distribution,selected,flips,mode,mutation,random*32767);
        TRmgGridPoint point(x,y);
        active=&a;run(&a,point);active=&b;reference(b,point);
        if(!same(a,b)||point.m_x!=x||point.m_y!=y)return false;
    }
    return true;
}
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest',type=Path,required=True)
    args=parser.parse_args()
    extract=generator('generate-rmg-position-family.py').definition
    _,originals,axes=source_families.load_manifest(args.manifest,HOMM3_DIR)
    source=(HOMM3_DIR/'src/rmg_terrain.cpp').read_text()
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    tileheader=(HOMM3_DIR/'include/rmg_terrain.h').read_text()
    start=header.index('template<class Coordinate> struct TRmgCoordinatePoint {')
    end=header.index('\nstruct TRmgZoneBounds {',start)
    text=PRELUDE+header[start:end]+'\ntemplate<class Coordinate>\n'+extract(source,'TRmgCoordinatePoint<Coordinate>::TRmgCoordinatePoint')+'\n'
    start=tileheader.index('struct rmgTerrainTile {');end=tileheader.index('\n// BuildNeighbourKinds',start)
    text+=tileheader[start:end]+BOUNDARIES
    for name in ('TRmgLinePainterTile::TRmgLinePainterTile','TRmgLinePainterTile::getLand','TRmgLinePainterTile::getTile','TRmgLinePainterTile::setTile','TRmgLinePainterInterface::at','TRmgLinePainterInterface::getNeighbourLand'):
        text+=extract(source,name)+'\n'
    groups={}
    for i in range(len(axes[0].options)):
        state=source_families.render(originals,axes,(i,))
        candidate=state['src/rmg_terrain.cpp']
        body=extract(candidate,'refreshRmgLinePoint')
        candidate_header=state.get('include/rmg.h',header)
        declaration=next((line.strip() for line in candidate_header.splitlines() if 'patternForFrame(int frame) const;' in line),'')
        helper=extract(candidate,'TRmgLinePatternTable::patternForFrame') if declaration else ''
        groups.setdefault((declaration,helper),[]).append(body)
    positive=sum(len(forms) for forms in groups.values());negative=0
    with tempfile.TemporaryDirectory(prefix='rmg-line-selection-oracle-') as folder:
        for group,((declaration,helper),forms) in enumerate(groups.items()):
            count=len(forms)
            if group==0:
                base=forms[0]
                for old,new in [('matches[direction] = 0;','matches[direction] = 1;'),
                                ('== oldType','!= oldType'),
                                ('current.m_flipY = flipY;','current.m_flipY = flipX;'),
                                ('current.m_frame = frame;','current.m_frame = frame + 1;'),
                                ('tile.setTile(current);','current.m_terrain = 0; tile.setTile(current);'),
                                ('int pattern = selected;','int pattern = (selected + 1) % 9;')]:
                    assert old in base
                    forms.append(base.replace(old,new));negative+=1
            program=text
            if declaration:
                program=program.replace('TRmgLinePatternRange m_ranges[9];};',
                    'TRmgLinePatternRange m_ranges[9]; '+declaration+' };')
                program+=helper+'\n'
            program+='\n#define rand fixtureRand\n'
            for i,body in enumerate(forms):
                program+=body.replace('refreshRmgLinePoint','run%d'%i)+'\n'
                if 'TRmgLinePainterInterface& painter' in body:
                    program+='void dispatch%d(TRmgLinePainterInterface* p,const TRmgGridPoint& q){run%d(*p,q); }\n'%(i,i)
                else:
                    program+='void dispatch%d(TRmgLinePainterInterface* p,const TRmgGridPoint& q){run%d(p,q); }\n'%(i,i)
            program+='\n#undef rand\n'+CHECKS+'\nint main(){\n'
            for i in range(len(forms)):
                program+='if(%scheck(dispatch%d)){std::printf("failed model %d\\n");return 1;}\n'%('!' if i<count else '',i,i)
            program+='return 0;}\n'
            src,binary=Path(folder)/('oracle%d.cpp'%group),Path(folder)/('oracle%d'%group);src.write_text(program)
            subprocess.run(['g++','-std=c++98','-O1','-fsanitize=undefined','-fno-sanitize-recover=all',str(src),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
    print('%d actual caller/helper forms x43200 scenarios; %d wrong controls rejected; UBSan clean'%(positive,negative))


if __name__=='__main__':main()
