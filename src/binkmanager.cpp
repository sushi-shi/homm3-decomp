#include <string.h>   // GetBinkFilePtr's inline strcpy/strcat/_strcmpi
#include <va.h>
#include "binkmanager.h"
#include "bitmap16.h"   // screenBitmap map/Pitch/Height
#include "inputmgr.h"   // gpInputManager, KEYCODE_F4
#include "kbwin.h"      // PollSound / Process1WindowsMessage
#include "message.h"
#include "mousemgr.h"   // gpMouseManager
#include "prefs.h"      // gUnnamed698758.soundVolume
#include "smackmgr.h"   // gVideoDescriptors, VideoDrawRects, VideoClose
#include "wingraph.h"   // gpDDSBack
#include "soundmgr.h"   // gpSoundManager->serviceSounds
#include "winmgr.h"

// Dreamcast publishes this exact name and the old 112-byte SDK type. Retail
// oldmain addresses the same object at 0x694ce8.
DATA(0x00694ce8)
BINKSUMMARY g_binkSummary;

// DC namespace globals SurfaceType, updateScreen, needsUpdate and PlayingBink.
// Raw publics use namespace storage class `3` and `_N` for the three bools.
// Retail stores the surface format at 0x694ca0, gates dirty-rectangle updates
// with 0x694ca8, publishes frame changes through 0x694ce0, and gates active
// playback with 0x694d5c. The latter is distinct from the playingBINK object.
DATA(0x00694ca0)
int BinkManager::g_surfaceType;
DATA(0x00694ca8)
bool BinkManager::g_updateScreen;
DATA(0x00694ce0)
bool BinkManager::g_needsUpdate;
DATA(0x00694d5c)
bool BinkManager::g_playingBinkActive;

// DC file-static bBinkSound; retail openBink writes this sound-enable gate.
DATA(0x00694d58)
static int g_binkSound;

// Dreamcast BinkManager::playingBINK, type BinkManagerStruct (48 bytes).
DATA(0x00694cb0)
BinkManager::BinkManagerStruct BinkManager::g_playingBink;

// DC SetPixelFormat(ulong, ulong, ulong), binkmanager.cpp:123, is a
// four-byte no-op. No retail body is identified. Windows selects Bink's
// format with BinkDDSurfaceType in openBink and videoRealignBuffers;
// display-mask dispatch calls only ResourceManager and SmackManager.
// Keep the active empty definition as the canonical positive DC source body;
// Complete can fold or discard it because no Windows caller survives.
// E:\gamedcs\binkmanager.cpp:123
DC_ONLY(0x50a80, 0x4)
void BinkManager::setPixelFormat(unsigned long redMask, unsigned long greenMask,
                                 unsigned long blueMask)
{
}

// The constant OR'd into every _BinkOpen flag word here; it is Bink's
// counterpart of smackmgr's SMACKOPEN_FROM_ARCHIVE and makes _BinkOpen take
// the already-open archive HANDLE in place of a file name.
static const int g_binkOpenFromArchive = 0x8000000;

// Retail retains four serviceSounds calls. The Windows body is source-local
// to soundmgr.cpp; its platform evidence comment explains that visibility.
VA(0x0044d5a0, 0x283)  // dc 0x50a7c
BINK* BinkManager::getBinkFilePtr(char* filename, int binkOptions)
{
    char name[40];
    int i;

    strcpy(name, filename);
    strcat(name, DATA_COMPGEN(0x00660b98, binkFileExtension, ".bik"));

    if (*g_videoGameState == VIDEO_GAME_STATE_EXPANSION_ARCHIVES) {
        for (i = 0; i < g_videoCount1; i++) {
            if (_strcmpi(g_videoHeader1[i].m_name, name) == 0) {
                SetFilePointer(g_videoFile1, g_videoHeader1[i].m_offset, 0,
                    FILE_BEGIN);
                g_soundManager->serviceSounds();
                return _BinkOpen(g_videoFile1,
                    binkOptions | g_binkOpenFromArchive);
            }
        }
    }

    for (i = 0; i < g_videoCount2; i++) {
        if (_strcmpi(g_videoHeader2[i].m_name, name) == 0) {
            SetFilePointer(g_videoFile2, g_videoHeader2[i].m_offset, 0, FILE_BEGIN);
            g_soundManager->serviceSounds();
            return _BinkOpen(g_videoFile2,
                binkOptions | g_binkOpenFromArchive);
        }
    }

    if (g_videoFile3) {
        for (i = 0; i < g_videoCount3; i++) {
            if (_strcmpi(g_videoHeader3[i].m_name, name) == 0) {
                SetFilePointer(g_videoFile3, g_videoHeader3[i].m_offset, 0,
                    FILE_BEGIN);
                g_soundManager->serviceSounds();
                return _BinkOpen(g_videoFile3,
                    binkOptions | g_binkOpenFromArchive);
            }
        }
    }

    if (*g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        for (i = 0; i < g_videoCount1; i++) {
            if (_strcmpi(g_videoHeader1[i].m_name, name) == 0) {
                SetFilePointer(g_videoFile1, g_videoHeader1[i].m_offset, 0,
                    FILE_BEGIN);
                g_soundManager->serviceSounds();
                return _BinkOpen(g_videoFile1,
                    binkOptions | g_binkOpenFromArchive);
            }
        }
    }
    return 0;
}

VA(0x0044D830, 0x1A3)  // dc 0x50a84
void BinkManager::openBink(int id, int x, int y, int w, int h, int loop,
                   bool useDirtyRects)
{
    if (g_unnamed699290 == 0 && g_soundManager->m_ds != 0
        && g_unnamed698758.m_soundVolume != 0)
        g_binkSound = 1;
    else
        g_binkSound = 0;

    videoClose();
    g_surfaceType = _BinkDDSurfaceType(g_ddsBack);
    g_playingBink.m_id = id;
    g_playingBink.m_paused = 0;

    if (g_videoDescriptors[id].m_smkAudioStem != "") {
        g_playingBink.m_bink2 = BinkManager::getBinkFilePtr(
            g_videoDescriptors[id].m_smkAudioStem, 0x400000);
        if (!g_playingBink.m_bink2) {
            BinkManager::closeBink();
            return;
        }
    }

    g_playingBink.m_bink = BinkManager::getBinkFilePtr(
        g_videoDescriptors[id].m_smkStem,
        g_videoDescriptors[id].m_noFrameSkip ? 0x400000 : 0);
    if (!g_playingBink.m_bink) {
        BinkManager::closeBink();
        return;
    }

    g_updateScreen = useDirtyRects;
    if (w <= 0)
        w = g_playingBink.m_bink->m_width;
    if (h <= 0)
        h = g_playingBink.m_bink->m_height;
    g_playingBink.m_loop = loop;
    g_playingBink.m_x = x;
    g_playingBink.m_y = y;
    g_playingBink.m_w = w;
    g_playingBink.m_h = h;
    g_playingBink.m_screen = g_windowManager->m_screenBitmap->getMap(x, y);
    g_playingBink.m_pitch = g_windowManager->m_screenBitmap->getPitch();
    g_playingBink.m_height = g_windowManager->m_screenBitmap->getHeight();
    g_playingBinkActive = 1;
}

// smackmgr.cpp's VideoDrawCurrentFrame uses this namespace-qualified call.
VA(0x0044d9e0, 0x6E)  // dc 0x50a88
void BinkManager::drawCurrentBinkFrame()
{
    Bink* video;
    if (g_playingBink.m_bink && g_playingBinkActive) {
        if (g_playingBink.m_bink->m_frameNum == 1)
            _BinkDoFrame(g_playingBink.m_bink);
        video = g_playingBink.m_bink;
    } else if (g_playingBink.m_bink2 && g_playingBinkActive) {
        if (g_playingBink.m_bink2->m_frameNum == 1)
            _BinkDoFrame(g_playingBink.m_bink2);
        video = g_playingBink.m_bink2;
    } else {
        return;
    }
    _BinkCopyToBuffer(video, g_playingBink.m_screen, g_playingBink.m_pitch, g_playingBink.m_height, 0, 0,
                      g_surfaceType);
}

VA(0x0044da50, 0x4D)  // dc 0x50a8c
void BinkManager::restartBink()
{
    if (g_playingBink.m_bink) {
        _BinkGoto(g_playingBink.m_bink, 1, 1);
        _BinkDoFrame(g_playingBink.m_bink);
        _BinkCopyToBuffer(g_playingBink.m_bink, g_playingBink.m_screen, g_playingBink.m_pitch, g_playingBink.m_height,
                          0, 0, g_surfaceType);
    }
}

// Like nextSmackerFrame, publish the complete readiness predicate in the
// real dirty-state byte before testing it. BinkWait precedes the store and
// paused frames remain dirty. This recovers the retail shared exit layout;
// separate true/false stores, readiness scopes and terminal guards did not.
// E:\gamedcs\binkmanager.cpp:252, dc 0x50a90
VA(0x0044DAA0, 0x21A)  // dc-order-map + caller (smackmgr VideoNextFrame), dc 0x50a90
void BinkManager::nextBinkFrame()
{
    Bink* video = g_playingBink.m_bink;
    if (!video)
        video = g_playingBink.m_bink2;
    g_needsUpdate = video && g_playingBinkActive && !_BinkWait(video);
    if (!g_needsUpdate)
        return;
    if (g_playingBink.m_paused)
        return;

    _BinkDoFrame(video);
    _BinkCopyToBuffer(video, g_playingBink.m_screen, g_playingBink.m_pitch, g_playingBink.m_height, 0, 0,
                      g_surfaceType);

    if (video->m_frameNum == video->m_frames) {
        if (g_playingBink.m_loop) {
            if (g_playingBink.m_bink && g_playingBink.m_bink2) {
                if (g_videoDescriptors[g_playingBink.m_id].m_fadeOnAbort)
                    g_windowManager->fadeScreen(1, 4, 0);
                g_soundManager->serviceSounds();
                _BinkClose(g_playingBink.m_bink);
                g_playingBink.m_bink = 0;
                video = g_playingBink.m_bink2;
                if (g_videoDescriptors[g_playingBink.m_id].m_fadeInSecondTrack) {
                    _BinkDoFrame(video);
                    _BinkCopyToBuffer(video, g_playingBink.m_screen, g_playingBink.m_pitch,
                                      g_playingBink.m_height, 0, 0, g_surfaceType);
                    g_windowManager->fadeScreen(0, 4, 0);
                }
            } else {
                _BinkNextFrame(video);
            }
        } else {
            _BinkGetSummary(video, &g_binkSummary);
            BinkManager::closeBink();
            if (g_videoDescriptors[g_playingBink.m_id].m_fadeOnAbort)
                g_windowManager->fadeScreen(1, 4, 0);
            else
                g_windowManager->updateScreen(0, 0, 800, 600);
            return;
        }
    } else {
        _BinkNextFrame(video);
    }
    if (g_updateScreen)
        videoDrawRects();
}

// E:\gamedcs\binkmanager.cpp:345 (dc 0x50a94) - namespace-qualified entry
VA(0x0044dcc0, 0x60)  // dc 0x50a94
void BinkManager::closeBink()
{
    if (g_playingBink.m_bink) {
        _BinkPause(g_playingBink.m_bink, 1);
        _BinkClose(g_playingBink.m_bink);
    }
    if (g_playingBink.m_bink2) {
        _BinkPause(g_playingBink.m_bink2, 1);
        _BinkClose(g_playingBink.m_bink2);
    }
    g_playingBink.m_bink2 = 0;
    g_playingBink.m_bink = 0;
    g_playingBink.m_paused = 0;
    g_playingBinkActive = 0;
    g_needsUpdate = 0;
}

// E:\gamedcs\binkmanager.cpp:376, dc 0x50a98 is a platform stub.
// Retail matches with the canonical playingBINK aggregate and ushort screen
// pointer. Enable sound before capturing dimensions, then publish centered
// coordinates before saving the redraw position. Fragmented globals reached
// 98.37%; scalar/POINT locals and byte-offset spellings did not recover the
// final coordinate registers. The aggregate and getMap call do.
VA(0x0044DD20, 0x227)  // dc-order-map + caller (smackmgr VideoPlay), dc 0x50a98
int BinkManager::playBink(int id, int x, int y, int w, int h)
{
    int vw, vh;
    int updateX;
    int updateY;
    unsigned char result;
    unsigned char aborted;

    g_soundManager->m_playSounds = 1;
    vh = h;
    vw = w;
    BinkManager::openBink(id, x, y, vw, vh, 0, 0);
    if (!g_playingBink.m_bink) {
        result = 0;
    } else {
        g_mouseManager->hidePointer();
        if (vw < 0)
            vw = g_playingBink.m_bink->m_width;
        if (vh < 0)
            vh = g_playingBink.m_bink->m_height;
        if (id != VIDEO_ID_OVERLAY_BLIT) {
            g_playingBink.m_x = x + (vw - g_playingBink.m_bink->m_width) / 2;
            g_playingBink.m_y = y + (vh - g_playingBink.m_bink->m_height) / 2;
            updateX = g_playingBink.m_x;
            updateY = g_playingBink.m_y;
        } else {
            updateX = 0;
            updateY = 0;
            vw = g_playingBink.m_bink->m_width;
            vh = g_playingBink.m_bink->m_height;
        }
        g_playingBink.m_screen = g_windowManager->m_screenBitmap->getMap(
            g_playingBink.m_x, g_playingBink.m_y);
        aborted = 0;
        g_inputManager->flush();
        while (1) {
            if (g_playingBink.m_bink == 0)
                break;
            pollSound();
            process1WindowsMessage();
            message msg = g_inputManager->getEvent();
            switch (msg.m_id) {
                case MESSAGE_KEY_DOWN:
                    if (msg.m_codeX == KEYCODE_F4)
                        break;
                    // fall through
                case MESSAGE_LEFT_BUTTON_DOWN:
                case MESSAGE_RIGHT_BUTTON_DOWN:
                    if (!g_videoNoSkip) {
                        aborted = 1;
                        break;
                    }
                    break;
            }
            if (aborted)
                break;
            if (videoNeedsUpdate())
                videoDrawRects();
        }
        BinkManager::closeBink();
        if (aborted && g_videoDescriptors[id].m_fadeOnAbort)
            g_windowManager->fadeScreen(1, 4, 0);
        else
            g_windowManager->updateScreen(updateX, updateY, vw, vh);
        g_mouseManager->showPointer(0);
        result = !aborted;
    }
    g_playingBink.m_paused = 0;
    g_playingBinkActive = 0;
    return result;
}
