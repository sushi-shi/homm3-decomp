"""Actual fizzle/fade bodies; checked row formation and per-frame oracles.

The fixture supplies bitmap ownership, pixel-indexed grab/draw, blit/time
stubs, and Win32 unsigned-long width. It checks the supported 800x600,
positive-pitch contract, not DirectDraw itself or aliasing/lifetime debt.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'src/winmgr.cpp').read_text()
parser = argparse.ArgumentParser()
parser.add_argument('--variant', nargs=2, metavar=('MANIFEST', 'CHOICE'))
args = parser.parse_args()
if args.variant:
    manifest = json.loads(Path(args.variant[0]).read_text())
    assert manifest['source'] == 'src/winmgr.cpp' and len(manifest['axes']) == 1
    axis = manifest['axes'][0]
    assert source.count(axis['find']) == 1
    source = source.replace(axis['find'], axis['options'][int(args.variant[1])].get('replace', axis['find']))


def body(name):
    start = source.index('void heroWindowManager::' + name + '(')
    return source[start:source.index('\n}', start) + 2]


bodies = '\n'.join(body(n) for n in ['saveFizzleSourceX', 'fizzleForwardX',
    'releaseFizzleSource', 'fadeToBlack', 'fadeFromBlack'])
bodies = bodies.replace('unsigned long', 'uint32_t')


def instrument(text):
    text = re.sub(r'(\w+\.m_bytes) = (\w+RowBase) \+ ([^;]+);',
                  r'\1 = step(\2, \3);', text)
    return re.sub(r'(\b(?:\w+\.m_bytes|sourceBytes|destinationBytes)) \+= ([^;]+);',
                  r'\1 = step(\1, \2);', text)


fixture = r'''
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
enum { WINDOW_SCREEN_WIDTH=800, WINDOW_SCREEN_HEIGHT=600 };
struct Region {uintptr_t base;ptrdiff_t size;};
static std::vector<Region> regions;
static void addRegion(void* p,size_t n) {regions.push_back({reinterpret_cast<uintptr_t>(p),ptrdiff_t(n)});}
template<class T> T* step(T* p,ptrdiff_t delta) {
    uintptr_t a=reinterpret_cast<uintptr_t>(p);
    for(auto r:regions)if(a>=r.base&&a-r.base<=uintptr_t(r.size)) {
        ptrdiff_t o=a-r.base;
        if(delta < -o || delta > r.size-o)throw std::out_of_range("row pointer formation");
        return reinterpret_cast<T*>(a+delta);
    }
    throw std::out_of_range("unknown row allocation");
}
static unsigned g_colorMaskRed,g_colorMaskGreen,g_colorMaskBlue;
static bool g_completeDrawEnabled=true;
union Bitmap16MapPointer {unsigned short* m_pixels;unsigned char* m_bytes;};
union Bitmap16ConstMapPointer {const unsigned short* m_pixels;const unsigned char* m_bytes;};
struct RECT {int left,top,right,bottom;};
static int destroyed;
struct Bitmap16Bit {
    int m_width,m_height,m_pitch;unsigned short* m_map;
    std::vector<unsigned short> storage;
    Bitmap16Bit(int w,int h,int padding=0):m_width(w),m_height(h),m_pitch(2*(w+padding)),storage((w+padding)*h) {
        m_map=storage.data();addRegion(m_map,storage.size()*2);
    }
    ~Bitmap16Bit() {
        auto base=reinterpret_cast<uintptr_t>(m_map);
        regions.erase(std::remove_if(regions.begin(),regions.end(),[=](Region r){return r.base==base;}),regions.end());
        ++destroyed;
    }
    unsigned short* getMap(int x,int y) {return m_map+y*(m_pitch/2)+x;}
    int getPitch() const {return m_pitch;}
    void grab(const unsigned short* s,int sx,int sy,int sw,int sh,int pitch) {
        for(int y=0;y<m_height;++y)for(int x=0;x<m_width;++x)
            if(sx+x>=0&&sx+x<sw&&sy+y>=0&&sy+y<sh)m_map[y*(m_pitch/2)+x]=s[(sy+y)*(pitch/2)+sx+x];
    }
    void draw(int sx,int sy,int w,int h,unsigned short* d,int dx,int dy,int dw,int dh,int pitch,bool) {
        for(int y=0;y<h;++y)for(int x=0;x<w;++x)
            if(dx+x>=0&&dx+x<dw&&dy+y>=0&&dy+y<dh)d[(dy+y)*(pitch/2)+dx+x]=m_map[(sy+y)*(m_pitch/2)+sx+x];
    }
    void fillRect(int x,int y,int w,int h,unsigned short c) {
        for(int iy=0;iy<h;++iy)for(int ix=0;ix<w;++ix)m_map[(y+iy)*(m_pitch/2)+x+ix]=c;
    }
};
struct heroWindowManager {
    Bitmap16Bit* m_screenBitmap; Bitmap16Bit* m_bmpFizzleSource;int m_colorCyclingOn;
    void saveFizzleSourceX(int,int,int,int);
    void fizzleForwardX(int,int,int,int,int);
    void releaseFizzleSource();
    void fadeToBlack(int,unsigned char);
    void fadeFromBlack(int);
};
static heroWindowManager* g_windowManager;
struct GameTime {
    static uint32_t get(){return 100;}
    static void delayTil(uint32_t){}
};
static void pollSound(){}
static std::vector<unsigned short> from,to;
static int phase,mode,rx,ry,rw,rh;
static unsigned short blend(unsigned short a,unsigned short b,int frame) {
    unsigned result=0;
    for(unsigned mask:{g_colorMaskRed,g_colorMaskGreen,g_colorMaskBlue}) {
        int from=a&mask,to=b&mask;
        // Independent integer formula, including floor for negative deltas.
        int numerator=(to-from)*frame;
        int change=numerator>=0?numerator/8:-((-numerator+7)/8);
        result|=(from+change)&mask;
    }
    return result;
}
static void blit(RECT* r) {
    auto* b=g_windowManager->m_screenBitmap;
    if(mode==0) {
        if(r->left!=rx||r->top!=ry||r->right!=rx+rw||r->bottom!=ry+rh)throw std::logic_error("fizzle rect");
        if(g_windowManager->m_colorCyclingOn!=0)throw std::logic_error("cycling during fizzle");
        for(int y=0;y<600;++y)for(int x=0;x<b->m_pitch/2;++x) {
            size_t i=y*(b->m_pitch/2)+x;unsigned short wanted=to[i];
            if(x>=rx&&x<rx+rw&&y>=ry&&y<ry+rh&&phase<8)wanted=blend(from[i],to[i],phase);
            if(b->m_map[i]!=wanted)throw std::logic_error("fizzle pixel output");
        }
    } else {
        if(r->left||r->top||r->right!=800||r->bottom!=600)throw std::logic_error("fade rect");
        int shift=mode==1?phase:2-phase;
        for(int y=0;y<600;++y)for(int x=0;x<b->m_pitch/2;++x) {
            size_t i=y*(b->m_pitch/2)+x;unsigned short wanted=from[i];
            if(x<800) {
                if(mode==1&&phase==3)wanted=0;
                else if(mode==2&&phase==2)wanted=from[i];
                else wanted=((from[i]&g_colorMaskRed)>>shift&g_colorMaskRed)
                    |((from[i]&g_colorMaskGreen)>>shift&g_colorMaskGreen)
                    |((from[i]&g_colorMaskBlue)>>shift&g_colorMaskBlue);
            }
            if(b->m_map[i]!=wanted)throw std::logic_error("fade pixel output");
        }
    }
    ++phase;
}
static void robAppBlit(RECT* r){blit(r);}
static void ddAppBlit(RECT* r){blit(r);}
@BODIES@
int main() {
    unsigned cases=0;
    try {
      for(bool rgb565:{false,true})for(int padding:{0,3}) {
        g_colorMaskRed=rgb565?0xf800:0x7c00;g_colorMaskGreen=rgb565?0x7e0:0x3e0;g_colorMaskBlue=0x1f;
        Bitmap16Bit screen(800,600,padding);
        heroWindowManager manager={&screen,0,7};g_windowManager=&manager;
        for(size_t i=0;i<screen.storage.size();++i)screen.storage[i]=(i*3571+717)&0xffff;
        from=screen.storage;to=from;for(auto& p:to)p^=0x739b;
        struct Box {int x,y,w,h;};
        for(auto box:{Box{0,0,1,1},Box{1,599,799,1},Box{799,598,2,5},Box{-1,598,3,5},Box{1,-1,2,602},Box{8,8,592,544}}) {
          screen.storage=from;rx=std::max(box.x,0);ry=std::max(box.y,0);
          rw=std::min(box.x+box.w,800)-rx;rh=std::min(box.y+box.h,600)-ry;
          manager.saveFizzleSourceX(box.x,box.y,box.w,box.h);
          if(!manager.m_bmpFizzleSource||manager.m_bmpFizzleSource->m_width!=rw||manager.m_bmpFizzleSource->m_height!=rh)
            throw std::logic_error("fizzle source dimensions");
          screen.storage=to;mode=0;phase=0;int before=destroyed;
          manager.fizzleForwardX(box.x,box.y,box.w,box.h,-1);
          if(phase!=9||manager.m_bmpFizzleSource||manager.m_colorCyclingOn!=7||destroyed!=before+2||screen.storage!=to)
            throw std::logic_error("fizzle lifecycle");
          ++cases;
        }
        for(bool restore:{false,true}) {
          screen.storage=from;mode=1;phase=0;manager.fadeToBlack(1,restore);
          if(phase!=4)throw std::logic_error("fade out count");
          for(int y=0;y<600;++y)for(int x=0;x<800;++x) {
            size_t i=y*(screen.m_pitch/2)+x;
            if(screen.m_map[i]!=(restore?from[i]:0))throw std::logic_error("fade restore");
          }
          ++cases;
        }
        screen.storage=from;mode=2;phase=0;manager.fadeFromBlack(1);
        if(phase!=3||screen.storage!=from)throw std::logic_error("fade in restore");
        ++cases;
        // Disabled and empty fizzle must not consume the saved source.
        screen.storage=from;manager.saveFizzleSourceX(0,0,1,1);auto saved=manager.m_bmpFizzleSource;
        phase=0;g_completeDrawEnabled=false;manager.fizzleForwardX(0,0,1,1,33);
        g_completeDrawEnabled=true;manager.fizzleForwardX(800,600,1,1,33);
        if(manager.m_bmpFizzleSource!=saved||phase||manager.m_colorCyclingOn!=7)throw std::logic_error("empty fizzle");
        manager.releaseFizzleSource();
      }
      std::printf("PASS %u fizzle/fade cases, all frames checked\n",cases);
    } catch(const std::out_of_range& e){std::fprintf(stderr,"%s\n",e.what());return 2;}
      catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
'''
opening = bodies.index('{', bodies.index('for (int row = 0; row < height; row++)'))
depth, end = 1, opening + 1
while depth:
    depth += (bodies[end] == '{') - (bodies[end] == '}')
    end += 1
unchecked = bodies[:end] + '\n                screen.m_bytes += m_screenBitmap->getPitch();' + bodies[end:]
variants = {'actual': bodies,
    'unchecked-fizzle': unchecked,
    'wrong-blend': bodies.replace('int alpha = (frame << 16) / 8;', 'int alpha = (frame << 16) / 9;'),
    'missing-release': bodies.replace('            releaseFizzleSource();', '')}
with tempfile.TemporaryDirectory(prefix='homm3-window-rows-') as directory:
    for name, text in variants.items():
        for optimization in (['-O0', '-O2'] if name == 'actual' else ['-O2']):
            cpp = Path(directory) / (name + '.cpp')
            cpp.write_text(fixture.replace('@BODIES@', instrument(text)))
            executable = cpp.with_suffix('')
            subprocess.run(['c++', '-std=c++17', optimization, str(cpp), '-o', str(executable)], check=True)
            result = subprocess.run([str(executable)], text=True, capture_output=True)
            expected = 0 if name == 'actual' else 2 if name == 'unchecked-fizzle' else 1
            print(name, optimization, result.returncode, result.stdout.strip(), result.stderr.strip(), flush=True)
            assert result.returncode == expected, (name, result)
