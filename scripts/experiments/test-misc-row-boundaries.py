"""Exercise actual puzzle/Victor bodies with pre-formation row checks.

Portable helpers stand in for Win32 I/O and the separately tested Victor
kernels. This tests row/control plumbing, not malformed PCX streams or the
Win32 ABI. Both independent pixel allocations and combined header/palette/
pixel allocations are registered at their complete allocation boundaries.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--variant', nargs=2, action='append', default=[], metavar=('MANIFEST', 'CHOICE'))
args = parser.parse_args()
sources = {}
for manifest_path, choice in args.variant:
    manifest = json.loads(Path(manifest_path).read_text())
    path = manifest['source']
    source = sources.get(path, (root / path).read_text())
    assert len(manifest['axes']) == 1
    axis = manifest['axes'][0]
    assert source.count(axis['find']) == 1
    sources[path] = source.replace(axis['find'], axis['options'][int(choice)].get('replace', axis['find']))


def body(path, signature):
    source = sources.get(path, (root / path).read_text())
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


puzzle = body('src/puzzlewindow.cpp', 'void Bitmap816::markPuzzle(')
flip = body('src/victor_flip.cpp', 'int __stdcall flipimage(')
pcx = body('src/victor_loadpcx.cpp', 'int __stdcall loadpcx(')
constants = (root / 'src/victor_loadpcx.cpp').read_text()
constants = constants[constants.index('DATA('):constants.index('// Dreamcast')]
constants = re.sub(r'DATA\([^)]*\) ', '', constants)
victor = (root / 'include/victor.h').read_text()
enums = '\n'.join(re.findall(r'enum[^;]+;', victor, re.S))


def instrument(source):
    source = re.sub(r'\b(source|destination|sourceTop|sourceBottom|destinationTop|destinationBottom) = (\w+RowBase) ([+-]) ([^;]+);',
        lambda m: f'{m[1]} = step({m[2]}, {"-" if m[3] == "-" else ""}ptrdiff_t({m[4]}));', source)
    return re.sub(r'(\b(?:sourceBlock|source|destination|sourceTop|sourceBottom|destinationTop|destinationBottom)) ([+-])= ([^;]+);',
                  lambda m: f'{m[1]} = step({m[1]}, {"-" if m[2] == "-" else ""}ptrdiff_t({m[3]}));', source)


def after_loop(source, loop, statement):
    opening = source.index('{', source.index(loop))
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[:end] + '\n' + statement + source[end:]


fixture = r'''
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <vector>
#define __stdcall
#define OF_SHARE_DENY_WRITE 0
struct Region { uintptr_t base; ptrdiff_t size; };
static std::vector<Region> regions;
static void track(std::vector<unsigned char>& v) {
    regions.push_back({reinterpret_cast<uintptr_t>(v.data()),ptrdiff_t(v.size())});
}
static unsigned char* step(unsigned char* p,ptrdiff_t delta) {
    uintptr_t a=reinterpret_cast<uintptr_t>(p);
    for(auto r:regions) if(a>=r.base && a-r.base<=uintptr_t(r.size)) {
        ptrdiff_t o=a-r.base;
        if(delta < -o || delta > r.size-o) throw std::out_of_range("row pointer formation");
        return p+delta;
    }
    throw std::out_of_range("unknown allocation");
}
struct Bitmap816 {
    int m_width,m_height,m_pitch; unsigned char* m_map;
    void markPuzzle(unsigned char*,long,long);
};
struct RGBQUAD { unsigned char rgbBlue,rgbGreen,rgbRed,rgbReserved; };
struct Header { int biWidth,biHeight; unsigned short biBitCount; };
struct imgdes {
    unsigned char* m_ibuff;
    unsigned m_stx,m_sty,m_endx,m_endy,m_buffwidth;
    RGBQUAD* m_palette; int m_colors,m_imgtype; Header* m_bmh; void* m_bitmap;
};
struct PcxData {
    int m_pcXvers; unsigned m_width,m_length;
    int m_bpPixel,m_nplanes,m_bytesPerLine,m_palInt,m_vbitcount;
};
struct OFSTRUCT {};
using HFILE=int;
@ENUMS@
@CONSTANTS@
static PcxData metadata;
static std::vector<unsigned char> fileData;
static size_t filePosition;
static int validationError,infoError,openError,closeCalls,paletteCalls,uploadCalls,decodeCalls;
static int victorValidateBitmap(imgdes*) { return validationError; }
static int pcxinfo(const char*,PcxData* d) { *d=metadata; return infoError; }
static int OpenFile(const char*,OFSTRUCT*,int) { filePosition=0;return openError?-1:1; }
static int _llseek(int,int,int) { return 128; }
static int _lread(int,void* p,int n) {
    size_t count=std::min(size_t(n),fileData.size()-filePosition);
    std::memcpy(p,fileData.data()+filePosition,count);filePosition+=count;return count;
}
static void _lclose(int) { ++closeCalls; }
static int victorReadPcxPalette(const char*,RGBQUAD*) { ++paletteCalls;return 0; }
static void victorInitializePalette(imgdes*) { ++paletteCalls; }
static void victorUploadPalette(imgdes*) { ++uploadCalls; }
static void victorMinimumDimensions(imgdes* a,imgdes* b,unsigned* h,unsigned* w) {
    *h=std::min(a->m_endy-a->m_sty+1,b->m_endy-b->m_sty+1);
    *w=std::min(a->m_endx-a->m_stx+1,b->m_endx-b->m_stx+1);
}
static int bit(const unsigned char* p,int n) { return (p[n/8]>>(7-n%8))&1; }
static void putBit(unsigned char* p,int n,int value) {
    unsigned char mask=128>>(n%8); p[n/8]=(p[n/8]&~mask)|(value?mask:0);
}
static void victorExtractBits(unsigned char* d,const unsigned char* s,int offset,int count) {
    for(int i=0;i<count;++i) putBit(d,i,bit(s,i+(offset&7)));
}
static void victorInsertBits(unsigned char* d,const unsigned char* s,int offset,int count) {
    for(int i=0;i<count;++i) putBit(d,i+(offset&7),bit(s,i));
}
static int victorDecodeRleBytes(unsigned char* d,unsigned char* s,int count) {
    auto start=s; ++decodeCalls;
    while(count>0) { int value=*s++,n=1;
        if((value&192)==192) {n=value&63;value=*s++;}
        if(n>count) throw std::logic_error("fixture RLE");
        std::memset(d,value,n);d+=n;count-=n;
    }
    return s-start;
}
static void victorUnpackFourPlanes(unsigned char* d,const unsigned char* s,int stride,int pixels) {
    for(int i=0;i<pixels;++i) {d[i]=0;for(int p=0;p<4;++p)d[i]|=bit(s+p*stride,i)<<p;}
}
static void victorInterleaveRgbPlanes(unsigned char* d,const unsigned char* s,int stride) {
    for(int i=0;i<stride;++i) for(int p=0;p<3;++p) d[i*3+(2-p)]=s[p*stride+i];
}
@BODIES@
static const char* stage;
static void same(const std::vector<unsigned char>& a,const std::vector<unsigned char>& b) {
    if(a!=b) throw std::logic_error(stage);
}
static void encode(const std::vector<unsigned char>& row) {
    for(auto v:row) { if(v>=192) fileData.push_back(193);fileData.push_back(v); }
}
int main() {
    unsigned cases=0;
    try {
      stage="puzzle pixel output";
      for(int w : {1,16,31,32,33,65,97}) for(int h : {1,16,31,32,33,65})
      for(int pad : {0,3}) for(int x : {-16,-8,0,7,31,567,583,599,608})
      for(int y : {-16,-8,0,7,31,503,519,535,544}) {
        std::vector<unsigned char> map((w+pad)*h),visible(19*17,1),expected=visible;
        for(size_t i=0;i<map.size();++i)map[i]=(i%3)!=0;
        for(int sy=0;sy<h;++sy)for(int sx=0;sx<w;++sx) {
          int dx=x+sx,dy=y+sy;
          // Retail's signed /32 truncates the -16 edge sample toward zero.
          if(((dx+16)&31)==0 && ((dy+16)&31)==0 && dx>=-16&&dy>=-16&&dx<608&&dy<544&&map[sy*(w+pad)+sx])
            expected[(((y+((-16-y)&31))/32)+(sy-((-16-y)&31))/32)*19
                     +(x+((-16-x)&31))/32+(sx-((-16-x)&31))/32]=0;
        }
        Bitmap816 b={w,h,w+pad,map.data()}; regions.clear();track(map);track(visible);
        b.markPuzzle(visible.data(),x,y);same(visible,expected);++cases;
      }
      stage="flip pixel output";
      for(int depth : {1,8,24}) for(int width : {1,7,8,9,17}) for(int height : {1,2,3,6})
      for(int prefix : {0,40,1064}) for(int x : {0,1,8}) for(int y : {0,1}) for(bool inplace : {false,true}) {
        int sw=width+x+2,sh=height+y,ss=((sw*depth+31)/32)*4,ds=ss+(inplace?0:4);
        Header ah={sw,sh,(unsigned short)depth},bh=ah;
        std::vector<unsigned char> a(prefix+ss*sh),b(prefix+ds*sh);
        for(size_t i=0;i<a.size();++i)a[i]=(i*71+13)&255;
        for(size_t i=0;i<b.size();++i)b[i]=(i*19+23)&255;
        auto old=a;auto expected=inplace?a:b;
        imgdes ai={a.data()+prefix,(unsigned)x,(unsigned)y,(unsigned)(x+width-1),(unsigned)(y+height-1),(unsigned)ss,0,0,0,&ah,0};
        imgdes bi={b.data()+prefix,(unsigned)x,(unsigned)y,(unsigned)(x+width-1),(unsigned)(y+height-1),(unsigned)ds,0,0,0,&bh,0};
        for(int row=0;row<height;++row)for(int col=0;col<width*depth;++col)
          putBit(expected.data()+prefix+(sh-y-row-1)*ds,x*depth+col,
                 bit(old.data()+prefix+(sh-y-(height-1-row)-1)*ss,x*depth+col));
        regions.clear();track(a);track(b);
        if(flipimage(&ai,inplace?&ai:&bi))throw std::logic_error("flip status");
        same(inplace?a:b,expected);if(!inplace)same(a,old);++cases;
      }
      stage="PCX pixel output";
      for(int mode=1;mode<=5;++mode) for(int width : {1,7,8,9,17}) for(int height : {1,2,5})
      for(int prefix : {0,40,1064}) for(int x : {0,1,8}) for(int y : {0,1}) {
        int depth=mode==2?1:mode==5?24:8;
        int planes=mode==3?4:mode==5?3:1;
        int bp=mode==2||mode==3?1:mode==4?4:8;
        int line=((width*bp+15)/16)*2;
        metadata={5,(unsigned)width,(unsigned)height,bp,planes,line,0,depth};
        int sw=x+width+2,sh=y+height,stride=((sw*depth+31)/32)*4;
        Header header={sw,sh,(unsigned short)depth}; RGBQUAD palette[256]={};
        std::vector<unsigned char> image(prefix+stride*sh,0xa5),expected=image;
        imgdes d={image.data()+prefix,(unsigned)x,(unsigned)y,(unsigned)(x+width-1),(unsigned)(y+height-1),(unsigned)stride,palette,0,1,&header,(void*)1};
        fileData.clear(); closeCalls=paletteCalls=uploadCalls=decodeCalls=0;
        for(int row=0;row<height;++row) {
          std::vector<unsigned char> raw(line*planes);
          for(size_t i=0;i<raw.size();++i)raw[i]=(row*43+i*67+197)&255;
          auto out=expected.data()+prefix+(sh-y-row-1)*stride;
          for(int col=0;col<width;++col) {
            if(mode==2)putBit(out,x+col,bit(raw.data(),col));
            else if(mode==3) {int v=0;for(int p=0;p<4;++p)v|=bit(raw.data()+p*line,col)<<p;out[x+col]=v;}
            else if(mode==4)out[x+col]=(raw[col/2]>>(col%2?0:4))&15;
            else if(mode==5)for(int p=0;p<3;++p)out[(x+col)*3+2-p]=raw[p*line+col];
            else out[x+col]=raw[col];
          }
          encode(raw);
        }
        regions.clear();track(image);
        if(loadpcx("fixture",&d)||closeCalls!=1||uploadCalls!=1||decodeCalls!=height*(mode==5?3:1))
          throw std::logic_error("PCX lifecycle");
        same(image,expected);++cases;
        // Early validation/info/open errors must not write or close an unopened file.
        auto saved=image;
        for(int error=0;error<3;++error) {
          validationError=error==0?-1:0;infoError=error==1?-2:0;openError=error==2;
          closeCalls=0;int rc=loadpcx("fixture",&d);
          if(rc!=(error==0?-1:error==1?-2:-4)||closeCalls)throw std::logic_error("PCX error path");
          same(image,saved);
        }
        validationError=infoError=openError=0;
      }
      std::printf("PASS %u puzzle/Victor cases\n",cases);
    } catch(const std::out_of_range& e) {std::fprintf(stderr,"%s\n",e.what());return 2;}
      catch(const std::exception& e) {std::fprintf(stderr,"%s\n",e.what());return 1;}
}
'''
fixture = fixture.replace('@ENUMS@', enums).replace('@CONSTANTS@', constants)
variants = {'actual': puzzle + '\n' + flip + '\n' + pcx,
    'unchecked-puzzle': after_loop(puzzle, 'for (int y = 0;', '    source += m_pitch * 32;') + '\n' + flip + '\n' + pcx,
    'unchecked-flip': puzzle + '\n' + after_loop(flip, 'while (rows--)', '                    sourceTop -= source->m_buffwidth;') + '\n' + pcx,
    'unchecked-pcx': puzzle + '\n' + flip + '\n' + after_loop(pcx, 'while (rowsRemaining)', '            destination -= image->m_buffwidth;'),
    'wrong-puzzle-cell': puzzle.replace('*destinationBlock = 0;', '*destinationBlock = 1;') + '\n' + flip + '\n' + pcx,
    'wrong-flip-row': puzzle + '\n' + flip.replace('memcpy(destinationTop, sourceBottom, rowBytes)', 'memcpy(destinationTop, sourceTop, rowBytes)') + '\n' + pcx}
with tempfile.TemporaryDirectory(prefix='homm3-misc-rows-') as directory:
    for name, source in variants.items():
        for optimization in (['-O0', '-O2'] if name == 'actual' else ['-O2']):
            cpp = Path(directory) / (name + '.cpp')
            cpp.write_text(fixture.replace('@BODIES@', instrument(source)))
            executable = cpp.with_suffix('')
            subprocess.run(['c++', '-std=c++17', optimization, str(cpp), '-o', str(executable)], check=True)
            result = subprocess.run([str(executable)], text=True, capture_output=True)
            expected = 0 if name == 'actual' else 2 if name.startswith('unchecked') else 1
            print(name, optimization, result.returncode, result.stdout.strip(), result.stderr.strip(), flush=True)
            assert result.returncode == expected, (name, result)
