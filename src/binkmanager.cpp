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

// DC BinkManager::SurfaceType, updateScreen, needsUpdate and PlayingBink.
// Retail stores the surface format at 0x694ca0, gates dirty-rectangle updates
// with 0x694ca8, publishes frame changes through 0x694ce0, and gates active
// playback with 0x694d5c. The latter is distinct from the playingBINK object.
DATA(0x00694ca0)
int BinkManager::s_surfaceType;
DATA(0x00694ca8)
unsigned char BinkManager::s_updateScreen;
DATA(0x00694ce0)
unsigned char BinkManager::s_needsUpdate;
DATA(0x00694d5c)
unsigned char BinkManager::s_playingBinkActive;

// DC file-static bBinkSound; retail openBink writes this sound-enable gate.
DATA(0x00694d58)
static int g_binkSound;

// Dreamcast BinkManager::playingBINK, type BinkManagerStruct (48 bytes).
DATA(0x00694cb0)
BinkManager::BinkManagerStruct BinkManager::s_playingBink;

#if 0  // @carcass

// E:\gamedcs\binkmanager.cpp:123
DC_ONLY(0x50a80, 0x4)
void BinkManager::setPixelFormat(unsigned long redMask, unsigned long greenMask,
                                 unsigned long blueMask)
{
    // @stub
}

// E:\gamedcs\binkmanager.cpp:232
#endif  // @carcass

// The constant OR'd into every _BinkOpen flag word here; it is Bink's
// counterpart of smackmgr's SMACKOPEN_FROM_ARCHIVE and makes _BinkOpen take
// the already-open archive HANDLE in place of a file name.
static const int g_binkOpenFromArchive = 0x8000000;

// Retail retains four serviceSounds calls; the canonical header helper still
// expands here. Correcting the Miles stream interface preserves this residual.
VA(0x0044d5a0, 0x283)  // dc 0x50a7c
BINK* BinkManager::getBinkFilePtr(const char* filename, int binkOptions)
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
                   unsigned char useDirtyRects)
{
    if (g_unnamed699290 == 0 && g_soundManager->m_ds != 0
        && g_unnamed698758.m_soundVolume != 0)
        g_binkSound = 1;
    else
        g_binkSound = 0;

    videoClose();
    s_surfaceType = _BinkDDSurfaceType(g_ddsBack);
    s_playingBink.m_id = id;
    s_playingBink.m_paused = 0;

    if (g_videoDescriptors[id].m_smkAudioStem != "") {
        s_playingBink.m_bink2 = BinkManager::getBinkFilePtr(
            g_videoDescriptors[id].m_smkAudioStem, 0x400000);
        if (!s_playingBink.m_bink2) {
            BinkManager::closeBink();
            return;
        }
    }

    s_playingBink.m_bink = BinkManager::getBinkFilePtr(
        g_videoDescriptors[id].m_smkStem,
        g_videoDescriptors[id].m_noFrameSkip ? 0x400000 : 0);
    if (!s_playingBink.m_bink) {
        BinkManager::closeBink();
        return;
    }

    s_updateScreen = useDirtyRects;
    if (w <= 0)
        w = s_playingBink.m_bink->m_width;
    if (h <= 0)
        h = s_playingBink.m_bink->m_height;
    s_playingBink.m_loop = loop;
    s_playingBink.m_x = x;
    s_playingBink.m_y = y;
    s_playingBink.m_w = w;
    s_playingBink.m_h = h;
    s_playingBink.m_screen = g_windowManager->m_screenBitmap->getMap(x, y);
    s_playingBink.m_pitch = g_windowManager->m_screenBitmap->getPitch();
    s_playingBink.m_height = g_windowManager->m_screenBitmap->getHeight();
    s_playingBinkActive = 1;
}

// smackmgr.cpp's VideoDrawCurrentFrame calls this one by the static-member
VA(0x0044d9e0, 0x6E)  // dc 0x50a88
void BinkManager::drawCurrentBinkFrame()
{
    Bink* video;
    if (s_playingBink.m_bink && s_playingBinkActive) {
        if (s_playingBink.m_bink->m_frameNum == 1)
            _BinkDoFrame(s_playingBink.m_bink);
        video = s_playingBink.m_bink;
    } else if (s_playingBink.m_bink2 && s_playingBinkActive) {
        if (s_playingBink.m_bink2->m_frameNum == 1)
            _BinkDoFrame(s_playingBink.m_bink2);
        video = s_playingBink.m_bink2;
    } else {
        return;
    }
    _BinkCopyToBuffer(video, s_playingBink.m_screen, s_playingBink.m_pitch, s_playingBink.m_height, 0, 0,
                      s_surfaceType);
}

VA(0x0044da50, 0x4D)  // dc 0x50a8c
void BinkManager::restartBink()
{
    if (s_playingBink.m_bink) {
        _BinkGoto(s_playingBink.m_bink, 1, 1);
        _BinkDoFrame(s_playingBink.m_bink);
        _BinkCopyToBuffer(s_playingBink.m_bink, s_playingBink.m_screen, s_playingBink.m_pitch, s_playingBink.m_height,
                          0, 0, s_surfaceType);
    }
}

// A temporary retained-service-call diagnostic recovers the historical 92.92%:
// the idle return and completion arm are ordered differently, and the final
// videoDrawRects call becomes a tail jump. Explicit readiness arms and paused
// work scopes are byte-flat, both with and without that diagnostic. Preserve
// the canonical serviceSounds inline; no call-boundary override remains.
// E:\gamedcs\binkmanager.cpp:252, dc 0x50a90
VA(0x0044DAA0, 0x21A)  // dc-order-map + caller (smackmgr VideoNextFrame), dc 0x50a90
void BinkManager::nextBinkFrame()
{
    Bink* video = s_playingBink.m_bink;
    if (!video)
        video = s_playingBink.m_bink2;
    if (video && s_playingBinkActive && !_BinkWait(video)) {
        s_needsUpdate = 1;
        if (s_playingBink.m_paused)
            return;

        _BinkDoFrame(video);
        _BinkCopyToBuffer(video, s_playingBink.m_screen, s_playingBink.m_pitch, s_playingBink.m_height, 0, 0,
                          s_surfaceType);

        if (video->m_frameNum == video->m_frames) {
            if (s_playingBink.m_loop) {
                if (s_playingBink.m_bink && s_playingBink.m_bink2) {
                    if (g_videoDescriptors[s_playingBink.m_id].m_fadeOnAbort)
                        g_windowManager->fadeScreen(1, 4, 0);
                    g_soundManager->serviceSounds();
                    _BinkClose(s_playingBink.m_bink);
                    s_playingBink.m_bink = 0;
                    video = s_playingBink.m_bink2;
                    if (g_videoDescriptors[s_playingBink.m_id].m_fadeInSecondTrack) {
                        _BinkDoFrame(video);
                        _BinkCopyToBuffer(video, s_playingBink.m_screen, s_playingBink.m_pitch,
                                          s_playingBink.m_height, 0, 0, s_surfaceType);
                        g_windowManager->fadeScreen(0, 4, 0);
                    }
                } else {
                    _BinkNextFrame(video);
                }
            } else {
                _BinkGetSummary(video, &g_binkSummary);
                BinkManager::closeBink();
                if (g_videoDescriptors[s_playingBink.m_id].m_fadeOnAbort)
                    g_windowManager->fadeScreen(1, 4, 0);
                else
                    g_windowManager->updateScreen(0, 0, 800, 600);
                return;
            }
        } else {
            _BinkNextFrame(video);
        }
        if (s_updateScreen)
            videoDrawRects();
        return;
    }

    s_needsUpdate = 0;
}

// E:\gamedcs\binkmanager.cpp:345 (dc 0x50a94) - the static-member spelling
VA(0x0044dcc0, 0x60)  // dc 0x50a94
void BinkManager::closeBink()
{
    if (s_playingBink.m_bink) {
        _BinkPause(s_playingBink.m_bink, 1);
        _BinkClose(s_playingBink.m_bink);
    }
    if (s_playingBink.m_bink2) {
        _BinkPause(s_playingBink.m_bink2, 1);
        _BinkClose(s_playingBink.m_bink2);
    }
    s_playingBink.m_bink2 = 0;
    s_playingBink.m_bink = 0;
    s_playingBink.m_paused = 0;
    s_playingBinkActive = 0;
    s_needsUpdate = 0;
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
    if (!s_playingBink.m_bink) {
        result = 0;
    } else {
        g_mouseManager->hidePointer();
        if (vw < 0)
            vw = s_playingBink.m_bink->m_width;
        if (vh < 0)
            vh = s_playingBink.m_bink->m_height;
        if (id != VIDEO_ID_OVERLAY_BLIT) {
            s_playingBink.m_x = x + (vw - s_playingBink.m_bink->m_width) / 2;
            s_playingBink.m_y = y + (vh - s_playingBink.m_bink->m_height) / 2;
            updateX = s_playingBink.m_x;
            updateY = s_playingBink.m_y;
        } else {
            updateX = 0;
            updateY = 0;
            vw = s_playingBink.m_bink->m_width;
            vh = s_playingBink.m_bink->m_height;
        }
        s_playingBink.m_screen = g_windowManager->m_screenBitmap->getMap(
            s_playingBink.m_x, s_playingBink.m_y);
        aborted = 0;
        g_inputManager->flush();
        while (1) {
            if (s_playingBink.m_bink == 0)
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
    s_playingBink.m_paused = 0;
    s_playingBinkActive = 0;
    return result;
}
