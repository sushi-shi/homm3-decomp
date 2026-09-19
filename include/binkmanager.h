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
// DC0x8016c and the RAD SDK both return signed long (s32).
__declspec(dllimport) long __stdcall _BinkGetRects(Bink* bnk, unsigned long flags);
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
    // DC BinkManagerStruct / playingBINK: members bink, bink2, screen,
    // pitch, height, x, y, w, h, id, loop, paused. Retail's 48-byte state
    // at 0x694cb0 has the same offsets; campaign previews copy all 12 words.
    struct BinkManagerStruct {
        BINK* m_bink;             // +0x00
        BINK* m_bink2;            // +0x04
        unsigned short* m_screen; // +0x08, DC T_32PUSHORT
        int m_pitch;              // +0x0c, byte stride
        int m_height;             // +0x10
        int m_x;                  // +0x14
        int m_y;                  // +0x18
        int m_w;                  // +0x1c
        int m_h;                  // +0x20
        int m_id;                 // +0x24
        int m_loop;               // +0x28
        int m_paused;             // +0x2c
    };
    static BinkManagerStruct s_playingBink;
    // DC SurfaceType, updateScreen, needsUpdate and PlayingBink.
    static int s_surfaceType;
    static unsigned char s_updateScreen;
    static unsigned char s_needsUpdate;
    static unsigned char s_playingBinkActive;

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
SIZE(BinkManager::BinkManagerStruct, 48);

#endif  /* HOMM3_BINKMANAGER_H */
