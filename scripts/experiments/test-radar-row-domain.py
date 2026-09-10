"""Check actual radar row/write switches against its fixed screen allocation.

Only cell/color selection is omitted. Native cursor proxies check additions
and stores before forming pointers. Unsupported bottom-edge placement is a
negative control, not a newly supported radar layout.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'src/advmgr.cpp').read_text()
start = source.index('    int rowPhase = 0;', source.index('void advManager::updateRadar(type_point'))
end = source.index('    // Radar-icon frame', start)
walk = source[start:end]
first = walk.index('            NewmapCell* cell')
last = walk.index('            // The write side')
walk = walk[:first] + '            unsigned short colour = 1;\n' + walk[last:]
walk = walk.replace('unsigned short*', 'Cursor')
fixture = r'''
#include <cstddef>
#include <cstdio>
#include <stdexcept>
enum { MAP_DIMENSION_SMALL=36,MAP_DIMENSION_MEDIUM=72,
       MAP_DIMENSION_LARGE=108,MAP_DIMENSION_EXTRA_LARGE=144 };
static ptrdiff_t largestPointer,largestStore;
struct Cursor {
    ptrdiff_t offset;
    Cursor operator+(ptrdiff_t n) const {
        auto result=offset+n;
        if(result<0||result>800*600)throw std::out_of_range("radar pointer");
        if(result>largestPointer)largestPointer=result;
        return {result};
    }
    void operator+=(ptrdiff_t n) {*this=*this+n;}
    unsigned short& operator[](ptrdiff_t n) const {
        auto result=offset+n;
        if(result<0||result>=800*600)throw std::out_of_range("radar store");
        if(result>largestStore)largestStore=result;
        static unsigned short pixel;return pixel;
    }
};
struct Bitmap { Cursor m_map;int m_pitch; };
struct Manager { Bitmap* m_screenBitmap; };
static Manager* g_windowManager;
static int g_mapHeight,g_mapWidth;
static unsigned char g_mapVisibilityBit=1;
static void walk(int rectX,int rectY) {
    int lastRow=g_mapHeight-1,lastColumn=g_mapWidth-1;
@WALK@
}
int main() {
    Bitmap bitmap={{0},1600};Manager manager={&bitmap};g_windowManager=&manager;
    try {
      for(int size:{36,72,108,144}) {
        g_mapHeight=g_mapWidth=size;largestPointer=largestStore=0;walk(630,26);
        if(largestPointer!=251830||largestStore>=251830)throw std::logic_error("fixed radar extent");
        std::printf("PASS %d: max pointer %td, max store %td of 480000 pixels\n",size,largestPointer,largestStore);
      }
      try { walk(630,456);return 1; }
      catch(const std::out_of_range&) {std::puts("PASS unsupported bottom placement rejected");}
    } catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
'''.replace('@WALK@', walk)
with tempfile.TemporaryDirectory(prefix='homm3-radar-domain-') as directory:
    cpp = Path(directory) / 'radar.cpp'
    cpp.write_text(fixture)
    for optimization in ['-O0', '-O2']:
        executable = cpp.with_suffix('')
        subprocess.run(['c++', '-std=c++17', optimization, str(cpp), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
