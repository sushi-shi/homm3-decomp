"""Exercise actual sprite renderers with a pixel oracle and checked row steps.

Like homm3-def's C++ parity adapter, compile the real TU/header with native
resource/palette stubs. A diagnostic hook checks row pointer displacements
before forming the result; it is not production address arithmetic. Encoded
streams are valid by construction. Malformed-stream and aliasing debt are
separate from this bounded row traversal test.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, default=root / 'src/cspriteframe.cpp')
parser.add_argument('--family', type=Path)
parser.add_argument('--choices', default='')
args = parser.parse_args()
source = args.source.read_text()
if args.family:
    axes = json.loads(args.family.read_text())['axes']
    choices = [int(value) for value in args.choices.split(',')]
    assert len(axes) == len(choices)
    for axis, choice in zip(axes, choices):
        assert source.count(axis['find']) == 1
        source = source.replace(axis['find'], axis['options'][choice].get('replace', axis['find']))

# Every byte-pitch destination step, plus raw-source row steps. Initial row
# construction and pixel +/- operations are independently bounded by the
# generated rectangle/stream and verified output; the defect is the final
# unused row cursor, which memory-access sanitizers alone may not diagnose.
step_re = re.compile(r'(?P<var>lineDst|dst) =\s*'
    r'static_cast<unsigned short\*>\(static_cast<void\*>\(\s*'
    r'static_cast<unsigned char\*>\(\s*static_cast<void\*>\((?P=var)\)\)\s*'
    r'(?P<sign>[+-])\s*dpitch\)\);')


def instrument(candidate):
    candidate, count = step_re.subn(lambda m: m['var'] + ' = checkedStep('
        + m['var'] + ', ' + ('-' if m['sign'] == '-' else '') + 'dpitch);', candidate)
    assert count == 24, count
    return candidate.replace('line += m_pitch;', 'line = checkedStep(line, m_pitch);')


adapter = (root / 'tools/homm3-def/cxx/oracle.cpp').read_text()
adapter = adapter[:adapter.index('extern "C" int homm3_cxx_draw_frame')]
adapter = adapter.replace('#include "src/cspriteframe.cpp"', '@IMPLEMENTATION@')
fixture = r'''
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>
struct Region { uintptr_t base; ptrdiff_t size; };
static std::vector<Region> regions;
template<class T> void addRegion(T* p, size_t count) {
    regions.push_back({reinterpret_cast<uintptr_t>(p),ptrdiff_t(count*sizeof(T))});
}
template<class T> T* checkedStep(T* p, ptrdiff_t bytes) {
    uintptr_t a=reinterpret_cast<uintptr_t>(p);
    for(auto region:regions) if(a>=region.base && a-region.base<=uintptr_t(region.size)) {
        ptrdiff_t offset=ptrdiff_t(a-region.base);
        if(bytes < -offset || bytes > region.size-offset) throw std::out_of_range("row pointer formation");
        return reinterpret_cast<T*>(a+bytes);
    }
    throw std::out_of_range("unregistered row pointer");
}
@ADAPTER@
struct Pixel { int code; unsigned char index; };
struct Stream { std::vector<unsigned char> bytes; std::vector<Pixel> pixels; };
static void offset(std::vector<unsigned char>& bytes,int at,int value,int size) {
    for(int i=0;i<size;++i) bytes[at+i]=(value>>(8*i))&255;
}
static Stream encode(int encoding,int width,int height) {
    Stream result;
    const int offsetSize=encoding==1?4:2;
    const int cells=encoding==3?width/32:1;
    if(encoding) result.bytes.resize(offsetSize*cells*height);
    result.pixels.resize(width*height);
    for(int y=0;y<height;++y) {
        for(int cell=0;cell<cells;++cell) {
            if(encoding) offset(result.bytes,(y*cells+cell)*offsetSize,result.bytes.size(),offsetSize);
            const int first=cell*width/cells,last=(cell+1)*width/cells;
            int x=first,packet=0;
            while(x<last) {
                int code=(packet+y)%9;
                if(encoding==0 || code==8 || (encoding>=2 && code==7)) code=-1;
                int run=std::min(1+(packet*7+y)%11,last-x);
                if(encoding==1) { result.bytes.push_back(code<0?255:code); result.bytes.push_back(run-1); }
                if(encoding>=2) result.bytes.push_back(((code<0?7:code)<<5)|(run-1));
                for(int i=0;i<run;++i) {
                    unsigned char index=(x+i+y*17)*13+29;
                    result.pixels[y*width+x+i]={code,index};
                    if(code<0) result.bytes.push_back(index);
                }
                x+=run; ++packet;
            }
        }
    }
    return result;
}
static unsigned short half(unsigned short p) { return (p>>1)&CSpriteFrame::s_div2mask.m_word; }
static unsigned short threeQuarters(unsigned short p) {
    return half(p)+((p>>2)&CSpriteFrame::s_div4mask);
}
static unsigned short shade(int method,int encoding,Pixel p,unsigned short dst,
                            const unsigned short* palette,bool flag,bool alpha,bool transparent) {
    const unsigned short outline=flag?0xace1:0;
    // Resolve exactly the supported dispatch paths; all primary decoder
    // bodies are exercised. General-RLE calls with the retail sw-as-sx quirk
    // are excluded here and remain covered by their existing byte evidence.
    if(method==0 && encoding!=1) { method=encoding==3?2:5; flag=false; }
    if(method==1 && !alpha && encoding!=1) method=encoding==3?2:5;
    if(method==2 && encoding!=3) method=5;
    if(method==7) {
        if(!alpha) { method=encoding==1?0:encoding==3?2:5; transparent=true; flag=false; }
        else if(encoding!=1) { method=encoding==3?3:5; flag=false; }
    }
    const unsigned short color=palette[p.index];
    if(method==0) return p.code<0?color:transparent?dst:palette[p.code];
    if(method==1) {
        if(p.code<0) return alpha?half(color)+half(dst):color;
        if(p.code==1 || (!outline && p.code==7)) return threeQuarters(dst);
        if(p.code==4 || (!outline && p.code==6)) return half(dst);
        if(outline && p.code>=5 && p.code<=7) return outline;
        return dst;
    }
    if(method==2) return p.code<0?color:p.code==5&&flag?outline:dst;
    if(method==3) return p.code<0?half(color)+half(dst):p.code==5&&flag?half(outline)+half(dst):dst;
    if(method==4) return p.code==1?threeQuarters(dst):p.code==4?half(dst):dst;
    if(method==5) return p.code<0?color:dst;
    if(method==6) return encoding==0?dst:(p.code==1||p.code==2)?threeQuarters(dst):(p.code==3||p.code==4)?half(dst):dst;
    if(method==7) return p.code<0?half(color)+half(dst):dst;
    throw std::logic_error("oracle method");
}
int main() {
    unsigned long cases=0;
    int method=-1,encoding=-1,sx=0,sy=0,sw=0,sh=0,dx=0,dy=0,hf=0,vf=0;
    try {
      g_rleLiteralRunCode=255;
      unsigned short colors[256]; for(int i=0;i<256;++i) colors[i]=(i*257)^0x5a5a;
      TPalette16 palette(colors);
      for(bool rgb565:{false,true}) {
        unsigned r=rgb565?0xf800:0x7c00,g=rgb565?0x7e0:0x3e0,b=0x1f;
        CSpriteFrame::s_div2mask.m_dword=((r>>1)&r)|((g>>1)&g)|((b>>1)&b);
        CSpriteFrame::s_div4mask=((r>>2)&r)|((g>>2)&g)|((b>>2)&b);
        for(encoding=0;encoding<4;++encoding) for(int ch:{1,4}) for(int crop:{0,2}) {
          const int cw=64,width=cw+crop*2,height=ch+crop*2;
          Stream stream=encode(encoding,cw,ch);
          CSpriteFrame frame("rows",width,height,stream.bytes.data(),stream.bytes.size(),
              static_cast<TEncodingMethod>(encoding),cw,ch,crop,crop);
          for(int padding:{0,3}) {
            const int dw=67,dh=ch,stride=dw+padding;
            std::vector<unsigned short> image(stride*dh),seed(image.size()),expected;
            for(size_t i=0;i<seed.size();++i) seed[i]=(i*3571+811)&0xffff;
            regions.clear();addRegion(image.data(),image.size());addRegion(frame.m_map,frame.m_dataSize);
            for(method=0;method<8;++method) for(int flags=0;flags<16;++flags) {
              hf=flags&1;vf=(flags>>1)&1;bool flag=flags&4,alpha=flags&8;
              if(vf && method!=5 && method!=6) continue;
              if(method==1 && alpha && encoding!=1) continue;
              if(method==2 && encoding==1) continue;
              if((method==3 || method==4) && encoding!=3) continue;
              if(method==5 && encoding!=0 && encoding!=2) continue;
              if(method==6 && encoding!=0 && encoding!=2) continue;
              for(int scenario=0;scenario<14;++scenario) {
                sx=crop+(scenario%4==3?31:scenario%4); sy=crop;
                sw=scenario<9?scenario+1:scenario==9?17:scenario==10?33:64-(sx-crop);
                sh=ch;dx=scenario%3==0?-2:scenario%3==1?1:dw-5;dy=scenario%4==0?-1:0;
                image=seed;expected=seed;
                // General mirror coordinates are relative to the full
                // frame, before cropping (CSpriteFrame::clip contract).
                for(int iy=0;iy<sh;++iy) for(int ix=0;ix<sw;++ix) {
                  int tx=dx+ix,ty=dy+iy;
                  int px=hf?width-1-(sx+ix):sx+ix;
                  int py=vf?height-1-(sy+iy):sy+iy;
                  if(tx<0||tx>=dw||ty<0||ty>=dh||px<crop||px>=crop+cw||py<crop||py>=crop+ch) continue;
                  auto& pixel=expected[ty*stride+tx];
                  pixel=shade(method,encoding,stream.pixels[(py-crop)*cw+px-crop],pixel,colors,flag,alpha,flag);
                }
                auto p=image.data();unsigned short outline=flag?0xace1:0;
                switch(method) {
                case 0: frame.draw(sx,sy,sw,sh,p,dx,dy,dw,dh,stride*2,palette,hf,flag);break;
                case 1: frame.drawCreatureImpl(sx,sy,sw,sh,p,dx,dy,dw,dh,stride*2,palette,hf,outline,alpha);break;
                case 2: frame.drawAdvObjImpl(sx,sy,sw,sh,p,dx,dy,dw,dh,stride*2,palette,hf,outline);break;
                case 3: frame.drawAdvObjWithFlagAlpha(sx,sy,sw,sh,p,dx,dy,dw,dh,stride*2,palette,outline,hf);break;
                case 4: frame.drawAdvObjShadowImpl(sx,sy,sw,sh,p,dx,dy,dw,dh,stride*2,palette,hf);break;
                case 5: frame.drawTile(sx,sy,sw,sh,p,dx,dy,dw,dh,stride*2,palette,hf,vf);break;
                case 6: frame.drawTileShadow(sx,sy,sw,sh,p,dx,dy,dw,dh,stride*2,palette,hf,vf);break;
                case 7: frame.drawSpellEffect(sx,sy,sw,sh,p,dx,dy,dw,dh,stride*2,palette,hf,alpha);break;
                }
                if(image!=expected) throw std::logic_error("pixel output");
                if(std::memcmp(frame.m_map,stream.bytes.data(),stream.bytes.size())) throw std::logic_error("source modified");
                ++cases;
              }
            }
          }
        }
      }
    } catch(const std::exception& e) {
      std::fprintf(stderr,"%s method=%d enc=%d sx=%d sy=%d sw=%d sh=%d dx=%d dy=%d hf=%d vf=%d\n",
          e.what(),method,encoding,sx,sy,sw,sh,dx,dy,hf,vf);
      return dynamic_cast<const std::out_of_range*>(&e)?2:1;
    }
    std::printf("%lu sprite cases\n",cases);return cases?0:3;
}
'''.replace('@ADAPTER@', adapter)

# Restore the old destination step in Draw alone for a precise negative
# control: valid pixels are identical, but the unused final pointer is not.
start = source.index('void CSpriteFrame::draw(', source.index('VA(0x0047c570'))
end = source.index('\n}', start)+2
draw = source[start:end]
old_draw, count = re.subn(r'                if \(y \+ 1 == sy \+ sh\)\n                    break;\n', '', draw)
assert count == 2, 'negative control expects the reviewed break-before-step draw'
unguarded = source[:start]+old_draw+source[end:]
reverse_pattern = re.compile(r'(?P<indent>^[ ]*)if \(y \+ 1 == sy \+ sh\)\n'
    r'[ ]+break;\n(?P<step>[ ]*lineDst =\s*static_cast<unsigned short\*>\('
    r'static_cast<void\*>\(\s*static_cast<unsigned char\*>\(\s*'
    r'static_cast<void\*>\(lineDst\)\)\s*-\s*dpitch\)\);)', re.M)
reverse_unguarded, reverse_count = reverse_pattern.subn(r'\g<step>', source)
assert reverse_count == 2, reverse_count
variants = [('actual', source, 0), ('original-final-draw-step', unguarded, 2),
    ('original-final-reverse-step', reverse_unguarded, 2),
    ('wrong-palette-index', source.replace('palette[*src++]', 'palette[0]'), 1),
    ('wrong-shadow-mask', source.replace('(*out >> 2)', '(*out >> 3)'), 1)]
with tempfile.TemporaryDirectory(prefix='homm3-sprite-rows-') as directory:
    scratch = Path(directory)
    for opt in ('-O0', '-O2'):
        for name, candidate, expected in variants:
            implementation = scratch / (name + '.inc')
            implementation.write_text(instrument(candidate))
            path = scratch / (name + '.cpp')
            path.write_text(fixture.replace('@IMPLEMENTATION@', '#include "'+str(implementation)+'"'))
            binary = scratch / name
            subprocess.run(['c++', '-std=c++17', opt, '-U__clang__', '-D__declspec(x)=',
                '-D__cdecl=', '-fno-strict-aliasing', '-I'+str(root/'include'), '-I'+str(root),
                str(path), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], timeout=60)
            assert result.returncode == expected, (name,opt,result.returncode,expected)
            print(opt,name,'PASS' if not expected else 'correctly rejected',flush=True)
