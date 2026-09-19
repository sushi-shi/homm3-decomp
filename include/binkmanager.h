// binkmanager.h - binkmanager.cpp (compiland binkmanager.obj)
#ifndef HOMM3_BINKMANAGER_H
#define HOMM3_BINKMANAGER_H

#include <va.h>

// ddraw's interface is only ever passed through here; the forward
// declaration keeps <ddraw.h> out of every includer that does not
// itself touch DirectDraw (smackmgr.cpp includes the real header
// first).
struct IDirectDrawSurface;

// Partial byte-proven view of the Bink handle: dirty rects at +0x30
// (stride 0x10), count at +0xb0.
struct BinkRect {
    long m_left;
    long m_top;
    long m_width;
    long m_height;
};
struct BINK {
    unsigned long m_width;      // +0x00
    unsigned long m_height;     // +0x04
    // Pad slice, in place: the per-frame pump compares +0x0c against +0x08
    // and stops the video when they meet, and DrawCurrentBinkFrame runs
    // BinkDoFrame only while +0x0c is 1 - the first-frame test.  That is
    // the SDK's Frames / FrameNum pair and nothing else fits.
    unsigned long m_frames;     // +0x08
    unsigned long m_frameNum;   // +0x0c
    // Dreamcast BINK (type 0x5548): this eight-word sequence occupies
    // +0x10..+0x2f between FrameNum and FrameRects. Retail confirms both
    // bounding fields (+0x0c and +0x30), including the rectangle stride/count.
    // The bundled newer SDK preserves these names/types but inserts stretch
    // dimensions and LastFrameNum earlier; its offsets do not apply here.
    unsigned long m_frameRate;     // +0x10
    unsigned long m_frameRateDiv;  // +0x14
    unsigned long m_readError;     // +0x18
    unsigned long m_openFlags;     // +0x1c
    unsigned long m_binkType;      // +0x20
    unsigned long m_size;          // +0x24
    unsigned long m_frameSize;     // +0x28
    unsigned long m_sndSize;       // +0x2c
    BinkRect m_frameRects[8];   // +0x30
    long m_numRects;            // +0xb0
};
typedef BINK Bink;

// The game shipped against an older Bink SDK than vendor/bink-0.5a.  The
// Dreamcast CodeView type is 112 bytes and, unlike the later header, has no
// SkippedBlits member and reports one combined TotalDecompTime.  Retail
// oldmain independently proves the fields it reads at +8/+20/+32/+48/+52/+56.
// Keep this ABI here instead of including the incompatible later SDK type.
struct BINKSUMMARY {
    unsigned long m_width;
    unsigned long m_height;
    unsigned long m_totalTime;
    unsigned long m_fileFrameRate;
    unsigned long m_fileFrameRateDiv;
    unsigned long m_frameRate;
    unsigned long m_frameRateDiv;
    unsigned long m_totalOpenTime;
    unsigned long m_totalFrames;
    unsigned long m_totalPlayedFrames;
    unsigned long m_skippedFrames;
    unsigned long m_soundSkips;
    unsigned long m_totalBlitTime;
    unsigned long m_totalReadTime;
    unsigned long m_totalDecompTime;
    unsigned long m_totalBackReadTime;
    unsigned long m_totalReadSpeed;
    unsigned long m_slowestFrameTime;
    unsigned long m_slowest2FrameTime;
    unsigned long m_slowestFrameNum;
    unsigned long m_slowest2FrameNum;
    unsigned long m_averageDataRate;
    unsigned long m_averageFrameSize;
    unsigned long m_highestMemAmount;
    unsigned long m_totalIoMemory;
    unsigned long m_highestIoUsed;
    unsigned long m_highest1SecRate;
    unsigned long m_highest1SecFrame;
};
SIZE(BINKSUMMARY, 112);

extern BINKSUMMARY g_binkSummary;

// The binkw32 import surface (leading underscore, the RAD convention -
// see smackmgr.h; smackmgr.cpp aliases the names back).
extern "C" {
__declspec(dllimport) int __stdcall _BinkPause(Bink* bnk, int pause);
__declspec(dllimport) int __stdcall _BinkDDSurfaceType(IDirectDrawSurface* dds);
__declspec(dllimport) int __stdcall _BinkGetRects(Bink* bnk, unsigned long flags);
__declspec(dllimport) int __stdcall _BinkGoto(Bink* bnk,
                                              unsigned long frame,
                                              int flags);
__declspec(dllimport) int __stdcall _BinkDoFrame(Bink* bnk);
__declspec(dllimport) int __stdcall _BinkCopyToBuffer(
    Bink* bnk, void* destination, int pitch, unsigned long height,
    unsigned long x, unsigned long y, unsigned long flags);
__declspec(dllimport) Bink* __stdcall _BinkOpen(void* handle,
                                                unsigned long flags);
__declspec(dllimport) void __stdcall _BinkClose(Bink* bnk);
__declspec(dllimport) int __stdcall _BinkWait(Bink* bnk);
__declspec(dllimport) void __stdcall _BinkNextFrame(Bink* bnk);
__declspec(dllimport) void __stdcall _BinkGetSummary(Bink* bnk,
                                                     BINKSUMMARY* sum);
}

// Dreamcast proves these are static members (the `YA` decorated forms and
// static data roster); retail supplies the full PC implementations.
class BinkManager {
public:
    static BINK* getBinkFilePtr(const char* filename, int binkOptions);
    static void setPixelFormat(unsigned long redMask,
                               unsigned long greenMask,
                               unsigned long blueMask);
    // DC dc:0x50a84 gives six ints followed by unsigned char, not bool.
    // Retail stores the final low byte in the dirty-rectangle flag.
    static void openBink(int id, int x, int y, int w, int h, int loop,
                         unsigned char useDirtyRects);
    static void drawCurrentBinkFrame();
    static void restartBink();
    static void nextBinkFrame();
    static void closeBink();
    static int playBink(int id, int x, int y, int w, int h);
};

// Bink TU globals (.bss 0x694ca0..0x694ce0, owned by the 0x44dxxx TU;
// names provisional, mirrored from smackmgr.cpp's smack set).
extern int g_binkSurfaceType;         // 0x694ca0 (BinkDDSurfaceType result)
extern Bink* g_binkVideo;             // 0x694cb0
extern Bink* g_binkVideo2;            // 0x694cb4
extern unsigned char* g_binkBuffer;   // 0x694cb8 (screen pixels at the bink origin)
extern int g_binkPitch;               // 0x694cbc
extern int g_binkHeight;              // 0x694cc0
extern int g_binkX;                   // 0x694cc4
extern int g_binkY;                   // 0x694cc8
// The two dwords between gBinkY and gBinkVideoId: CampaignWindowHandler
// hands 0x694cc4/0x694cc8/0x694ccc/0x694cd0 straight to
// heroWindowManager::UpdateScreen(x, y, w, h), which pairs them with the
// already-named gBinkX/gBinkY as that call's width and height. Ordinal
// names until a producer in the bink TU proves stronger ones.
extern int g_binkUpdateWidth;         // 0x694ccc
extern int g_binkUpdateHeight;        // 0x694cd0
extern int g_binkVideoId;             // 0x694cd4 (VIDEO_ID_OVERLAY_BLIT plays via the overlay Blt)
// 0x694cd8 closes the 12-dword snapshot campaignwindow.cpp copies out of
// gBinkVideo. NextBinkFrame consults it when the first track reaches its last
// frame: with it set AND both handles live it closes track one and carries on
// with track two, otherwise it ends the playback outright. Provisional name.
extern int g_binkChainTrack;          // 0x694cd8
extern int g_binkPaused;              // 0x694cdc
extern unsigned char g_binkDirty;     // 0x694ce0
// The dirty-rect gate the per-frame pump tests before calling
// VideoDrawRects; the mirror of smackmgr's own rect switch. Provisional.
extern unsigned char g_binkUseDirtyRects;  // 0x694ca8
// Raised while a bink is actually running: DrawCurrentBinkFrame and
// NextBinkFrame both refuse to touch the handles without it, and CloseBink
// drops it. The mirror of smackmgr's gSmackFrameReady. Provisional.
extern unsigned char g_binkFrameReady;     // 0x694d5c
// The bink twin of smackmgr's gVideoSoundReady, raised by OpenBinkVideo
// out of exactly the same three-way gate (gUnnamed699290 == 0 &&
// gpSoundManager->ds != 0 && gUnnamed698758.soundVolume != 0). Provisional.
extern int g_binkSoundReady;               // 0x694d58

#endif  /* HOMM3_BINKMANAGER_H */
