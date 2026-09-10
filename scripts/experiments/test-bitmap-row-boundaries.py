"""Run actual bitmap bodies with output oracles and checked pointer steps.

The fixture instruments pointer addition BEFORE forming its result. Integer
addresses are used only by this diagnostic, never by production. It models
the Win32 widths of floating-point helper storage, not VC6 FP/codegen or the
separate union/type-punning debt. HSV uses a one-row metamorphic oracle;
copy/frame/darken/conversion use independent pixel-indexed expectations.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--source16', type=Path, default=root / 'src/bitmap16.cpp')
parser.add_argument('--source24', type=Path, default=root / 'src/bitmap24.cpp')
parser.add_argument('--family16', type=Path)
parser.add_argument('--family24', type=Path)
parser.add_argument('--choices16', default='')
parser.add_argument('--choices24', default='')
args = parser.parse_args()
s16, s24 = args.source16.read_text(), args.source24.read_text()


def select(source, family, choices):
    if not family:
        return source
    axes = json.loads(family.read_text())['axes']
    choices = [int(value) for value in choices.split(',')]
    assert len(choices) == len(axes)
    for axis, choice in zip(axes, choices):
        assert source.count(axis['find']) == 1
        source = source.replace(axis['find'], axis['options'][choice].get('replace', axis['find']))
    return source


s16 = select(s16, args.family16, args.choices16)
s24 = select(s24, args.family24, args.choices24)


def body(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


def instrument(source):
    # Row and pixel += sites, including the offset-family's base + row*pitch.
    source = re.sub(r'(\b(?:\w+\.m_bytes|maskRow|src|pixel|in)) \+= ([^;]+);',
                    r'\1 = checkedStep(\1, \2);', source)
    source = re.sub(r'(\w+\.m_bytes) = (\w+Base) \+ ([^;]+);',
                    r'\1 = checkedStep(\2, \3);', source)
    source = re.sub(
        r'dst = static_cast<unsigned short\*>\(static_cast<void\*>\(\s*'
        r'static_cast<unsigned char\*>\(static_cast<void\*>\(dst\)\)\s*'
        r'\+ ([^;]+)\)\);', r'dst = checkedStep(dst, \1);', source)
    return source


signatures16 = ['void Bitmap16Bit::draw(', 'void Bitmap16Bit::grab(',
    'void Bitmap16Bit::frameRect(',
    'void Bitmap16Bit::darken(int x, int y, int w, int h)\n',
    'void Bitmap16Bit::darken(int x, int y, int w, int h, Bitmap816*',
    'void Bitmap16Bit::colorize(int x, int y, int w, int h, float']
signatures24 = ['void Bitmap24Bit::draw(int sx, int sy, int sw, int sh, unsigned short*',
    'void Bitmap24Bit::adjustHSV(int x, int y, int w, int h, float hue,\n']
bodies16 = '\n'.join(body(s16, sig) for sig in signatures16)
bodies24 = '\n'.join(body(s24, sig) for sig in signatures24)
helpers16 = s16[s16.index('union TFloatLongBits'):s16.index('#if 0')]
helpers24 = s24[s24.index('static __forceinline long ftol'):s24.index('#if 0')]


def win32_helpers(source):
    return source.replace('__forceinline', 'inline').replace(
        'unsigned long', 'uint32_t').replace('long', 'int32_t')


fixture = r'''
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#include <cstdio>
@HSV@
struct Region { uintptr_t base; ptrdiff_t size; };
static std::vector<Region> regions;
template<class T> void addRegion(std::vector<T>& v) {
    regions.push_back({reinterpret_cast<uintptr_t>(v.data()), ptrdiff_t(v.size()*sizeof(T))});
}
template<class T> T* checkedStep(T* pointer, ptrdiff_t bytes) {
    const uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
    for (const auto& region : regions) {
        if (address >= region.base && address - region.base <= uintptr_t(region.size)) {
            const ptrdiff_t offset = ptrdiff_t(address - region.base);
            if (bytes < -offset || bytes > region.size-offset)
                throw std::out_of_range("pointer formation");
            return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pointer) + bytes);
        }
    }
    throw std::out_of_range("unregistered pointer");
}
static unsigned int g_colorMaskRed, g_colorMaskGreen, g_colorMaskBlue;
union Bitmap16MapPointer { unsigned short* m_pixels; unsigned char* m_bytes; };
union Bitmap16ConstMapPointer { const unsigned short* m_pixels; const unsigned char* m_bytes; };
struct Bitmap816 {
    int m_width, m_height, m_pitch; unsigned char* m_map;
    unsigned char* getMap(int x,int y) { return m_map + y*m_pitch + x; }
    int getPitch() const { return m_width; }
};
struct Bitmap16Bit {
    int m_width, m_height, m_pitch; unsigned short* m_map;
    unsigned short* getMap(int x,int y) const { return m_map + y*(m_pitch/2) + x; }
    void draw(int,int,int,int,unsigned short*,int,int,int,int,int,bool) const;
    void grab(const unsigned short*,int,int,int,int,int);
    void frameRect(int,int,int,int,unsigned short);
    void darken(int,int,int,int);
    void darken(int,int,int,int,Bitmap816*,int,int);
    void colorize(int,int,int,int,float,float);
};
struct Bitmap24Bit {
    int m_width, m_height, pitch; unsigned char* m_data;
    int getPitch() const { return pitch; }
    void draw(int,int,int,int,unsigned short*,int,int,int,int,int) const;
    void adjustHSV(int,int,int,int,float,float,float,float);
};
namespace helper16 {
@HELPERS16@
}
using helper16::ftol;
@BODIES16@
namespace helper24 {
@HELPERS24@
}
using helper24::rgbToHSV;
using helper24::hsvToRGB;
@BODIES24@
template<class T> void same(const std::vector<T>& actual,const std::vector<T>& expected) {
    if (actual != expected) throw std::logic_error("pixel output");
}
int main() {
    unsigned long cases = 0;
    try {
      for (bool rgb565 : {false,true}) {
        g_colorMaskRed=rgb565?0xf800:0x7c00; g_colorMaskGreen=rgb565?0x7e0:0x3e0; g_colorMaskBlue=0x1f;
        const unsigned shiftMask=((g_colorMaskRed>>1)&g_colorMaskRed)
           |((g_colorMaskGreen>>1)&g_colorMaskGreen)|((g_colorMaskBlue>>1)&g_colorMaskBlue);
        for (int width=2;width<=6;++width) for (int height=1;height<=5;++height)
        for (int pad : {0,2}) {
          const int stride=width+pad, srcStride=width+(pad?0:3), maskStride=width+2;
          std::vector<unsigned short> image(stride*height), src(srcStride*height), expected, seed(image.size());
          std::vector<unsigned char> maskData(maskStride*height), data24((width*3+pad)*height);
          for (size_t i=0;i<seed.size();++i) seed[i]=(i*7919+12345)&0xffff;
          for (size_t i=0;i<src.size();++i) src[i]=i%3==0?1:(i*3571+717)&0xffff;
          for (size_t i=0;i<maskData.size();++i) maskData[i]=(i%3==0)?0:1;
          for (size_t i=0;i<data24.size();++i) data24[i]=(i*31+71)&255;
          auto savedSource=src; auto saved24=data24; auto savedMask=maskData;
          Bitmap16Bit bitmap={width,height,stride*2,image.data()}, source={width,height,srcStride*2,src.data()};
          Bitmap816 mask={width,height,maskStride,maskData.data()};
          Bitmap24Bit b24={width,height,width*3+pad,data24.data()};
          regions.clear(); addRegion(image); addRegion(src); addRegion(maskData); addRegion(data24);
          for (int x=0;x<width;++x) for(int y=0;y<height;++y)
          for(int w : {1,width-x,width+1}) for(int h : {1,height-y,height+1}) {
            const int cw=std::min(w,width-x), ch=std::min(h,height-y);
            for(int operation=0;operation<3;++operation) {
              image=seed; expected=seed;
              for(int iy=0;iy<ch;++iy) for(int ix=0;ix<cw;++ix) {
                auto& pixel=expected[(y+iy)*stride+x+ix];
                if(operation==0) { if(iy==0||iy==ch-1||ix==0||ix==cw-1) pixel=0xace1; }
                else if(operation==1||maskData[iy*width+ix]) pixel=(pixel>>1)&shiftMask;
              }
              if(operation==0) bitmap.frameRect(x,y,w,h,0xace1);
              if(operation==1) bitmap.darken(x,y,w,h);
              if(operation==2) bitmap.darken(x,y,w,h,&mask,0,0);
              same(image,expected); ++cases;
            }
          }
          for(int sx : {0,1}) for(int sy=0;sy<height;++sy)
          for(int dx : {-1,0,1}) for(int dy : {-1,0,height-1}) for(bool key : {false,true}) {
            image=seed; expected=seed;
            for(int iy=0;iy<height-sy;++iy) for(int ix=0;ix<width-sx;++ix) {
              int tx=dx+ix,ty=dy+iy;
              if(tx>=0&&tx<width&&ty>=0&&ty<height) {
                auto p=src[(sy+iy)*srcStride+sx+ix];
                if(!key||p!=1) expected[ty*stride+tx]=p;
              }
            }
            source.draw(sx,sy,width-sx,height-sy,image.data(),dx,dy,width,height,stride*2,key);
            same(image,expected); same(src,savedSource); ++cases;
            image=seed;expected=seed;
            for(int iy=0;iy<height-sy;++iy) for(int ix=0;ix<width-sx;++ix) {
              int tx=dx+ix,ty=dy+iy;
              if(tx>=0&&tx<width&&ty>=0&&ty<height) {
                const auto* p=&data24[(sy+iy)*b24.pitch+(sx+ix)*3];
                unsigned r=((p[0]*((g_colorMaskRed<<1)&~g_colorMaskRed))>>8)&g_colorMaskRed;
                unsigned g=((p[1]*((g_colorMaskGreen<<1)&~g_colorMaskGreen))>>8)&g_colorMaskGreen;
                unsigned b=((p[2]*((g_colorMaskBlue<<1)&~g_colorMaskBlue))>>8)&g_colorMaskBlue;
                expected[ty*stride+tx]=r|g|b;
              }
            }
            b24.draw(sx,sy,width-sx,height-sy,image.data(),dx,dy,width,height,stride*2);
            same(image,expected); same(data24,saved24); ++cases;
          }
          for(int sx : {-1,0,1}) for(int sy : {-1,0,height-1}) {
            image=seed; expected=seed;
            for(int iy=0;iy<height;++iy) for(int ix=0;ix<width;++ix) {
              int tx=sx+ix,ty=sy+iy;
              if(tx>=0&&tx<width&&ty>=0&&ty<height) expected[iy*stride+ix]=src[ty*srcStride+tx];
            }
            bitmap.grab(src.data(),sx,sy,width,height,srcStride*2);
            same(image,expected); same(src,savedSource); ++cases;
          }
          // Same pixel math, independently indexed single-row visits.
          for(int sector=0;sector<6;++sector) for(float saturation : {0.0f,0.5f,1.0f}) {
            float hue=(sector+0.25f)/6;
            image=seed; bitmap.colorize(1,0,width-1,height,hue,saturation); expected=image;
            image=seed; for(int y=0;y<height;++y) bitmap.colorize(1,y,width-1,1,hue,saturation);
            same(image,expected); ++cases;
            for(float adjust : {-1.0f,0.0f,0.5f,1.0f,2.0f}) {
              data24=saved24; b24.adjustHSV(1,0,width-1,height,hue,saturation,adjust,adjust);
              auto expected24=data24;
              data24=saved24; for(int y=0;y<height;++y) b24.adjustHSV(1,y,width-1,1,hue,saturation,adjust,adjust);
              same(data24,expected24); ++cases;
            }
            data24=saved24;
          }
          same(maskData,savedMask);
        }
      }
    } catch(const std::out_of_range& e) { std::fprintf(stderr,"%s\n",e.what()); return 2; }
      catch(const std::logic_error& e) { std::fprintf(stderr,"%s\n",e.what()); return 1; }
    std::printf("%lu bitmap cases\n",cases); return cases?0:3;
}
'''

replacements = {'HSV': (root / 'include/hsv.h').read_text(),
    'HELPERS16': win32_helpers(helpers16), 'HELPERS24': win32_helpers(helpers24),
    'BODIES16': instrument(bodies16), 'BODIES24': instrument(bodies24)}
for name, value in replacements.items():
    fixture = fixture.replace('@' + name + '@', value)

frame = body(bodies16, 'void Bitmap16Bit::frameRect(')
frameStep = '            if (row)\n                dst.m_bytes += m_pitch;\n'
assert frameStep in frame, 'negative control expects selected next-row frame'
unguardedFrame = frame.replace(frameStep, '').replace('        }\n    }\n}',
    '            dst.m_bytes += m_pitch;\n        }\n    }\n}')
variants = [('actual', fixture, 0),
    ('wrong-frame-color', fixture.replace('dst.m_pixels[col] = color;',
                                         'dst.m_pixels[col] = color ^ 1;'), 1),
    ('wrong-mask-stride', fixture.replace('int getPitch() const { return m_width; }',
                                        'int getPitch() const { return m_pitch; }'), 1),
    ('original-final-frame-step', fixture.replace(instrument(frame),
                                                 instrument(unguardedFrame)), 2)]
with tempfile.TemporaryDirectory(prefix='homm3-bitmap-rows-') as directory:
    scratch = Path(directory)
    for opt in ('-O0', '-O2'):
        for name, candidate, expected in variants:
            path = scratch / (name + '.cpp')
            path.write_text(candidate)
            binary = scratch / name
            subprocess.run(['c++', '-std=c++17', opt, '-U_FORTIFY_SOURCE', '-fno-strict-aliasing',
                            str(path), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], timeout=60)
            assert result.returncode == expected, (name, opt, result.returncode, expected)
            print(opt, name, 'PASS' if not expected else 'correctly rejected', flush=True)
