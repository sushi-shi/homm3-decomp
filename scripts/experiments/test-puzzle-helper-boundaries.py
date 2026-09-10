"""Actual recovered helpers: visible-owner, ordered-service and tile-grid checks.

Services and the tile constructor are reduced stand-ins. The fixture tests
helper plumbing and coordinate/array ownership, not engine AI or VC6 inlining.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, default=root / 'src/puzzlewindow.cpp')
args = parser.parse_args()
source = args.source.read_text()


def body(signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start)+2]


helpers = body('static unsigned char markAIPuzzle(') + '\n' + body('static void createAIPuzzleMap(')
fixture = r'''
#include <algorithm>
#include <bitset>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
struct Event { int kind,a,b,c; bool operator==(const Event& x) const {
    return kind==x.kind&&a==x.a&&b==x.b&&c==x.c; } };
static std::vector<Event> events;
static bool setupSuccess;
static int mapSize=36;
struct type_point {
    signed short m_x:10;
    signed short m_y:10;
    signed short m_z:4;
    type_point() {}
    bool isValid() const { return m_x>=0&&m_x<mapSize&&m_y>=0&&m_y<mapSize&&m_z>=0&&m_z<2; }
};
struct NewmapCell {int code;};
struct type_AI_puzzle_tile {
    int code;
    type_AI_puzzle_tile():code(-1) {}
    type_AI_puzzle_tile(NewmapCell* cell,type_point point) {
        code=cell->code + 10000 + point.m_x*17+point.m_y*19+point.m_z*23;
    }
};
struct Game {
    struct {long m_alignment[8];} m_setup;
    int m_ultimateArtifactX,m_ultimateArtifactPresent,m_ultimateArtifactZ;
    int setupPuzzlePieces(long player,int update) {events.push_back({0,int(player),update,0});return setupSuccess;}
    NewmapCell* getCell(type_point point) {
        static NewmapCell cell;
        events.push_back({4,point.m_x,point.m_y,point.m_z});
        cell.code=point.m_x+point.m_y*37+point.m_z*1800;return &cell;
    }
};
static Game game;
static Game* g_game=&game;
static std::bitset<48> g_puzzlePiecesRemoved;
static short g_puzzlePieceOrder[9*48],g_puzzlePieceX[9*96],g_puzzlePieceY[9*96];
struct Bitmap816 {
    int piece;
    void markPuzzle(unsigned char* visible,long x,long y) {
        events.push_back({2,piece,int(x),int(y)});visible[(piece*7)%323]=0;
    }
    void dispose() {events.push_back({3,piece,0,0});}
};
static Bitmap816* getPuzzleBitmap(long puzzle,long piece) {
    static Bitmap816 bitmap;
    events.push_back({1,int(puzzle),int(piece),0});bitmap.piece=piece;return &bitmap;
}
@HELPERS@
static int signed10(int x) {x&=1023;return x>=512?x-1024:x;}
static bool check() {
    unsigned cases=0;
    for(int puzzle=0;puzzle<9;++puzzle) {
        for(int i=0;i<48;++i)g_puzzlePieceOrder[puzzle*48+i]=(i*13+puzzle)%48;
        for(int i=0;i<96;++i) {g_puzzlePieceX[puzzle*96+i]=puzzle*97+i*7-80;
            g_puzzlePieceY[puzzle*96+i]=puzzle*53-i*3+40;}
    }
    for(int player:{-1,0,3,7})for(int alignment:{-1,0,4,8})
    for(int x:{-1,0,12})for(int present:{0,1})for(bool success:{false,true})for(int pattern=0;pattern<5;++pattern) {
        std::fill(game.m_setup.m_alignment,game.m_setup.m_alignment+8,alignment);
        game.m_ultimateArtifactX=x;game.m_ultimateArtifactPresent=present;setupSuccess=success;
        for(int i=0;i<48;++i)g_puzzlePiecesRemoved[i]=pattern==1||pattern==2&&(i%2)||pattern==3&&i%3==0||pattern==4&&i!=47;
        unsigned char visible[323],expected[323];std::memset(visible,0xa5,sizeof visible);std::memset(expected,0xa5,sizeof expected);
        events.clear();std::vector<Event> expectedEvents;
        bool valid=x>=0&&present;
        if(valid)expectedEvents.push_back({0,player,0,0});
        valid=valid&&success;
        if(valid) {
            std::memset(expected,1,sizeof expected);
            int puzzle=player<0||alignment==-1?0:alignment;
            for(int i=0;i<48;++i)if(!g_puzzlePiecesRemoved[i]) {
                int piece=(i*13+puzzle)%48;
                expectedEvents.push_back({1,puzzle,piece,0});
                expectedEvents.push_back({2,piece,puzzle*97+piece*7-88,puzzle*53-piece*3+32});
                expectedEvents.push_back({3,piece,0,0});expected[(piece*7)%323]=0;
            }
        }
        if(markAIPuzzle(player,visible)!=valid||std::memcmp(visible,expected,sizeof visible)||events!=expectedEvents)return false;
        ++cases;
    }
    for(int player:{-1,0,7})for(int ox:{-20,-1,0,18,35,510})for(int oy:{-17,-1,0,20,35,510})
    for(int z:{0,1,2})for(int pattern=0;pattern<4;++pattern) {
        game.m_ultimateArtifactZ=z;
        unsigned char visible[323];
        for(int i=0;i<323;++i)visible[i]=pattern==0?0:pattern==1?1:pattern==2?i%3==0:i%7==1;
        type_AI_puzzle_tile tiles[19][17];int expected[19][17];
        for(int col=0;col<19;++col)for(int row=0;row<17;++row)tiles[col][row].code=expected[col][row]=-(col*17+row+1);
        events.clear();std::vector<Event> expectedEvents;
        for(int row=0;row<17;++row)for(int col=0;col<19;++col) {
            int x=signed10(ox+col),y=signed10(oy+row);
            if(x>=0&&x<36&&y>=0&&y<36&&z<2&&visible[row*19+col]) {
                expectedEvents.push_back({4,x,y,z});expected[col][row]=10000+x*18+y*56+z*1823;
            }
        }
        createAIPuzzleMap(player,visible,ox,oy,tiles);
        if(events!=expectedEvents)return false;
        for(int col=0;col<19;++col)for(int row=0;row<17;++row)if(tiles[col][row].code!=expected[col][row])return false;
        ++cases;
    }
    std::printf("PASS %u puzzle-helper cases\n",cases);return true;
}
int main() {return check()?0:1;}
'''
variants = [('actual',helpers,0)]
for name,old,new in [
    ('skip-disposal','bitmap->dispose();',''),
    ('ignore-setup-failure','if (!g_game->setupPuzzlePieces(player, 0))','if (g_game->setupPuzzlePieces(player, 0))'),
    ('wrong-removed-piece','if (g_puzzlePiecesRemoved[i])','if (!g_puzzlePiecesRemoved[i])'),
    ('wrong-visible-owner','memset(visible, 1, 17 * 19);','memset(visible, 1, sizeof(visible));'),
    ('wrong-visible-index','visible[row * 19 + col]','visible[col * 17 + row]'),
    ('wrong-origin','point.m_x = puzzleX + col;','point.m_x = puzzleY + col;')]:
    assert old in helpers,name
    variants.append((name,helpers.replace(old,new),1))
with tempfile.TemporaryDirectory(prefix='homm3-puzzle-helper-') as directory:
    for name,body,expected in variants:
        for optimization in (['-O0','-O2'] if name=='actual' else ['-O2']):
            cpp=Path(directory)/(name+'.cpp')
            cpp.write_text(fixture.replace('@HELPERS@',body));binary=cpp.with_suffix('')
            subprocess.run(['c++','-std=c++17',optimization,str(cpp),'-o',str(binary)],check=True)
            result=subprocess.run([str(binary)],text=True,capture_output=True)
            print(name,optimization,result.returncode,result.stdout.strip(),flush=True)
            assert result.returncode==expected,(name,result)
