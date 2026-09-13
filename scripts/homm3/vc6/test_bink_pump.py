"""Check Bink pump event order, live track flags and canonical teardown."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class BinkPumpTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'requires native C++ compiler')
    def test_pump_guards_callbacks_and_cleanup(self):
        spec=importlib.util.spec_from_file_location('bink_family',
            ROOT/'scripts/experiments/generate-bink-pump-family.py')
        family=importlib.util.module_from_spec(spec);spec.loader.exec_module(family)
        source=(ROOT/family.SOURCE).read_text()
        actual=family.body_at(source,family.PUMP)
        close=family.body_at(source,'void BinkManager::closeBink()')
        variants=[('Actual',actual,True)]
        variants += [(f'Family{i}',body,True) for i,(_,body) in enumerate(family.pump_variants(actual))]
        variants += [
            ('SkipWait',actual.replace('!_BinkWait(video)','true'),False),
            ('KeepDirty',actual.replace('    g_binkDirty = 0;','    g_binkDirty = 1;'),False),
            ('KeepPrimary',actual.replace('                    g_binkVideo = 0;', ''),False),
            ('StaleFlags',actual.replace('g_videoDescriptors[g_binkVideoId].m_fadeInSecondTrack',
                                         'g_videoDescriptors[0].m_fadeInSecondTrack'),False),
            ('SkipSummary',actual.replace('_BinkGetSummary(video, &g_binkSummary);',''),False),
        ]
        fixture=r'''
#include <vector>
#include <cstdio>
struct Bink {int id;unsigned m_frameNum,m_frames;};
struct BINKSUMMARY {int id;};
struct Descriptor {unsigned char m_fadeOnAbort,m_fadeInSecondTrack;};
std::vector<int> effects;
Bink firstVideo,secondVideo;
Bink* g_binkVideo;
Bink* g_binkVideo2;
int g_binkFrameReady,g_binkPaused,g_binkDirty,g_binkChainTrack,g_binkUseDirtyRects;
int g_binkVideoId,g_binkBuffer,g_binkPitch,g_binkHeight,g_binkSurfaceType;
int waitResult,mutate;
Descriptor g_videoDescriptors[2];
BINKSUMMARY g_binkSummary;
int id(Bink* video) {return video?video->id:0;}
int _BinkWait(Bink* video) {effects.push_back(10+id(video));return waitResult;}
void _BinkDoFrame(Bink* video) {effects.push_back(100+id(video));}
void _BinkCopyToBuffer(Bink* video,int buffer,int pitch,int height,int x,int y,int format) {
    effects.push_back(200+id(video));
    if(buffer!=101 || pitch!=202 || height!=303 || x || y || format!=404) effects.push_back(9999);
}
void _BinkNextFrame(Bink* video) {effects.push_back(300+id(video));}
void _BinkPause(Bink* video,int paused) {effects.push_back(400+id(video));if(paused!=1)effects.push_back(9998);}
void _BinkClose(Bink* video) {effects.push_back(500+id(video));}
void _BinkGetSummary(Bink* video,BINKSUMMARY* summary) {effects.push_back(600+id(video));summary->id=id(video);}
void videoDrawRects() {effects.push_back(1000);}
struct WindowManager {
    void fadeScreen(int direction,int steps,int flag) {
        effects.push_back(700+direction);
        if(steps!=4 || flag!=0)effects.push_back(9997);
    }
    void updateScreen(int x,int y,int w,int h) {
        effects.push_back(800);
        if(x || y || w!=800 || h!=600)effects.push_back(9996);
    }
} windowManager;
WindowManager* g_windowManager=&windowManager;
struct SoundManager {
    void serviceSounds() {effects.push_back(900);if(mutate)g_binkVideoId=1;}
} soundManager;
SoundManager* g_soundManager=&soundManager;
struct BinkManager {
    static void closeBink();
    // @DECLARATIONS@
};
// @CLOSE@
// @FUNCTIONS@
void setup(unsigned bits,int callback) {
    effects.clear();mutate=callback;
    firstVideo.id=1;secondVideo.id=2;
    firstVideo.m_frames=secondVideo.m_frames=10;
    firstVideo.m_frameNum=secondVideo.m_frameNum=(bits&32)?10:1;
    g_binkVideo=(bits&1)?&firstVideo:0;g_binkVideo2=(bits&2)?&secondVideo:0;
    g_binkFrameReady=!!(bits&4);g_binkPaused=!!(bits&8);waitResult=!!(bits&16);
    g_binkChainTrack=!!(bits&64);g_binkUseDirtyRects=!!(bits&512);
    g_videoDescriptors[0].m_fadeOnAbort=!!(bits&128);
    g_videoDescriptors[0].m_fadeInSecondTrack=!!(bits&256);
    g_videoDescriptors[1].m_fadeOnAbort=!(bits&128);
    g_videoDescriptors[1].m_fadeInSecondTrack=!(bits&256);
    g_binkVideoId=0;g_binkDirty=7;g_binkSummary.id=-1;
    g_binkBuffer=101;g_binkPitch=202;g_binkHeight=303;g_binkSurfaceType=404;
}
bool check(void (*pump)()) {
    for(unsigned bits=0;bits<1024;++bits)
    for(int callback=0;callback<2;++callback) {
        setup(bits,callback);
        std::vector<int> expected;
        int primary=id(g_binkVideo),secondary=id(g_binkVideo2);
        int selected=primary?primary:secondary;
        int dirty=0,paused=g_binkPaused,ready=g_binkFrameReady,descriptor=0,summary=-1;
        if(selected && ready) {
            expected.push_back(10+selected);
            if(!waitResult) {
                dirty=1;
                if(!paused) {
                    expected.push_back(100+selected);expected.push_back(200+selected);
                    bool finished=false;
                    if(bits&32) {
                        if(bits&64) {
                            if(primary && secondary) {
                                if(bits&128)expected.push_back(701);
                                expected.push_back(900);descriptor=callback;
                                expected.push_back(500+primary);primary=0;
                                bool fade=(bits&256)!=0;if(callback)fade=!fade;
                                if(fade) {
                                    expected.push_back(100+secondary);expected.push_back(200+secondary);
                                    expected.push_back(700);
                                }
                            } else expected.push_back(300+selected);
                        } else {
                            expected.push_back(600+selected);summary=selected;
                            if(primary) {expected.push_back(400+primary);expected.push_back(500+primary);}
                            if(secondary) {expected.push_back(400+secondary);expected.push_back(500+secondary);}
                            primary=secondary=dirty=paused=ready=0;
                            expected.push_back((bits&128)?701:800);finished=true;
                        }
                    } else expected.push_back(300+selected);
                    if(!finished && (bits&512))expected.push_back(1000);
                }
            }
        }
        pump();
        if(effects!=expected || id(g_binkVideo)!=primary || id(g_binkVideo2)!=secondary
           || g_binkDirty!=dirty || g_binkPaused!=paused || g_binkFrameReady!=ready
           || g_binkVideoId!=descriptor || g_binkSummary.id!=summary) return false;
    }
    return true;
}
bool cleanupCheck(void (*cleanup)()) {
    for(unsigned bits=0;bits<16;++bits) {
        setup(bits,0);std::vector<int> expected;
        if(bits&1) {expected.push_back(401);expected.push_back(501);}
        if(bits&2) {expected.push_back(402);expected.push_back(502);}
        cleanup();
        if(effects!=expected || g_binkVideo || g_binkVideo2 || g_binkDirty
           || g_binkPaused || g_binkFrameReady) return false;
    }
    return true;
}
void oldCleanup() {
// @OLD_CLEANUP@
}
int main() {
    if(!cleanupCheck(oldCleanup) || !cleanupCheck(BinkManager::closeBink)) return 1;
// @CHECKS@
}
'''
        declarations,functions,checks=[],[],[]
        for name,body,expected in variants:
            declarations.append(f'static void pump{name}();')
            functions.append(body.replace('BinkManager::nextBinkFrame()',f'BinkManager::pump{name}()',1))
            checks.append(f'if(check(BinkManager::pump{name}) != {str(expected).lower()}) '
                          f'{{std::puts("{name}");return 1;}}')
        fixture=fixture.replace('// @DECLARATIONS@','\n'.join(declarations)).replace('// @CLOSE@',close)
        fixture=fixture.replace('// @FUNCTIONS@','\n'.join(functions)).replace('// @CHECKS@','\n'.join(checks))
        fixture=fixture.replace('// @OLD_CLEANUP@',family.CLOSE)
        with tempfile.TemporaryDirectory(prefix='homm3-bink-pump-') as temp:
            cpp=Path(temp)/'test.cpp';binary=Path(temp)/'test';cpp.write_text(fixture)
            built=subprocess.run(['g++','-std=c++17','-O1',str(cpp),'-o',str(binary)],capture_output=True,text=True)
            self.assertEqual(built.returncode,0,built.stderr)
            result=subprocess.run([str(binary)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)


if __name__=='__main__':unittest.main()
