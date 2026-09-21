#ifndef HOMM3_BINKMANAGER_H
#define HOMM3_BINKMANAGER_H

#include "va.h"

#include <bink.h>

// The active vendor header is patched from its preserved 1.0a source to the
// complete 0.5a layouts proved by Dreamcast CodeView. Retail independently
// confirms every BINK/BINKSUMMARY field consumed by the Windows game.
SIZE(BINKIO, 180);
SIZE(BINKSND, 184);
SIZE(BINKRECT, 16);
SIZE(BINK, 788);
SIZE(BINKSUMMARY, 112);

extern BINKSUMMARY g_binkSummary;

// Raw DC publics encode namespace functions (`YA`) and globals (`3`),
// unlike class-static `SA` functions and `2` data. The sole named type
// is BinkManagerStruct, the namespace-owned 48-byte state type.
namespace BinkManager {
    // DC BinkManagerStruct / playingBINK: members bink, bink2, screen,
    // pitch, height, x, y, w, h, id, loop, paused. Retail's 48-byte state
    // at 0x694cb0 has the same offsets; campaign previews copy all 12 words.
    struct BinkManagerStruct {
        HBINK m_bink;             // +0x00
        HBINK m_bink2;            // +0x04
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
    extern BinkManagerStruct g_playingBink;
    // DC SurfaceType, updateScreen, needsUpdate and PlayingBink.
    extern int g_surfaceType;
    extern bool g_updateScreen;
    extern bool g_needsUpdate;
    extern bool g_playingBinkActive;

    // Raw DC GetBinkFilePtr public uses PAD (char*), matching its typed argument.
    HBINK getBinkFilePtr(char* filename, int binkOptions);
    void setPixelFormat(unsigned long redMask,
                        unsigned long greenMask,
                        unsigned long blueMask);
    // Raw DC OpenBink public proves bool; VideoOpen forwards the same
    // Boolean flag, and retail stores its low byte without normalization.
    void openBink(int id, int x, int y, int w, int h, int loop,
                  bool useDirtyRects);
    void drawCurrentBinkFrame();
    void restartBink();
    void nextBinkFrame();
    void closeBink();
    int playBink(int id, int x, int y, int w, int h);
} // namespace BinkManager
SIZE(BinkManager::BinkManagerStruct, 48);

#endif  /* HOMM3_BINKMANAGER_H */
