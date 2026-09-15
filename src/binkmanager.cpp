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

#if 0  // @carcass

// E:\gamedcs\binkmanager.cpp:123
DC_ONLY(0x50a80, 0x4)
void BinkManager::setPixelFormat()
{
    // @stub
}

// E:\gamedcs\binkmanager.cpp:232
#endif  // @carcass

// The constant OR'd into every _BinkOpen flag word here; it is Bink's
// counterpart of smackmgr's SMACKOPEN_FROM_ARCHIVE and makes _BinkOpen take
// the already-open archive HANDLE in place of a file name.
static const int g_binkOpenFromArchive = 0x8000000;

VA(0x0044d5a0, 0x283)  // dc 0x50a7c
BINK* BinkManager::getBinkFilePtr(const char* filename, int binkOptions)
{
    char name[40];
    int i;

    strcpy(name, filename);
    strcat(name, DATA_COMPGEN(0x00660b98, binkFileExtension, ".bik"));

    if (*g_videoGameState == VIDEO_GAME_STATE_EXPANSION_ARCHIVES) {
        for (i = 0; i < g_videoCount1; i++) {
            if (strcmpi(g_videoHeader1[i].m_name, name) == 0) {
                SetFilePointer(g_videoFile1, g_videoHeader1[i].m_offset, 0,
                    FILE_BEGIN);
                g_soundManager->serviceSounds();
                return binkopen(g_videoFile1,
                    binkOptions | g_binkOpenFromArchive);
            }
        }
    }

    for (i = 0; i < g_videoCount2; i++) {
        if (strcmpi(g_videoHeader2[i].m_name, name) == 0) {
            SetFilePointer(g_videoFile2, g_videoHeader2[i].m_offset, 0, FILE_BEGIN);
            g_soundManager->serviceSounds();
            return binkopen(g_videoFile2,
                binkOptions | g_binkOpenFromArchive);
        }
    }

    if (g_videoFile3) {
        for (i = 0; i < g_videoCount3; i++) {
            if (strcmpi(g_videoHeader3[i].m_name, name) == 0) {
                SetFilePointer(g_videoFile3, g_videoHeader3[i].m_offset, 0,
                    FILE_BEGIN);
                g_soundManager->serviceSounds();
                return binkopen(g_videoFile3,
                    binkOptions | g_binkOpenFromArchive);
            }
        }
    }

    if (*g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        for (i = 0; i < g_videoCount1; i++) {
            if (strcmpi(g_videoHeader1[i].m_name, name) == 0) {
                SetFilePointer(g_videoFile1, g_videoHeader1[i].m_offset, 0,
                    FILE_BEGIN);
                g_soundManager->serviceSounds();
                return binkopen(g_videoFile1,
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
        g_binkSoundReady = 1;
    else
        g_binkSoundReady = 0;

    videoClose();
    g_binkSurfaceType = binkddsurfacetype(g_ddsBack);
    g_binkVideoId = id;
    g_binkPaused = 0;

    if (g_videoDescriptors[id].m_smkAudioStem != "") {
        g_binkVideo2 = BinkManager::getBinkFilePtr(
            g_videoDescriptors[id].m_smkAudioStem, 0x400000);
        if (!g_binkVideo2) {
            BinkManager::closeBink();
            return;
        }
    }

    g_binkVideo = BinkManager::getBinkFilePtr(
        g_videoDescriptors[id].m_smkStem,
        g_videoDescriptors[id].m_noFrameSkip ? 0x400000 : 0);
    if (!g_binkVideo) {
        BinkManager::closeBink();
        return;
    }

    g_binkUseDirtyRects = useDirtyRects;
    if (w <= 0)
        w = g_binkVideo->m_width;
    if (h <= 0)
        h = g_binkVideo->m_height;
    g_binkChainTrack = loop;
    g_binkX = x;
    g_binkY = y;
    g_binkUpdateWidth = w;
    g_binkUpdateHeight = h;
    g_binkBuffer = 2 * x + g_windowManager->m_screenBitmap->getPitch() * y
        + static_cast<unsigned char*>(
              static_cast<void*>(g_windowManager->m_screenBitmap->getMap(0, 0)));
    g_binkPitch = g_windowManager->m_screenBitmap->getPitch();
    g_binkHeight = g_windowManager->m_screenBitmap->getHeight();
    g_binkFrameReady = 1;
}

// smackmgr.cpp's VideoDrawCurrentFrame calls this one by the static-member
VA(0x0044d9e0, 0x6E)  // dc 0x50a88
void BinkManager::drawCurrentBinkFrame()
{
    Bink* video;
    if (g_binkVideo && g_binkFrameReady) {
        if (g_binkVideo->m_frameNum == 1)
            binkdoframe(g_binkVideo);
        video = g_binkVideo;
    } else if (g_binkVideo2 && g_binkFrameReady) {
        if (g_binkVideo2->m_frameNum == 1)
            binkdoframe(g_binkVideo2);
        video = g_binkVideo2;
    } else {
        return;
    }
    binkcopytobuffer(video, g_binkBuffer, g_binkPitch, g_binkHeight, 0, 0,
                      g_binkSurfaceType);
}

VA(0x0044da50, 0x4D)  // dc 0x50a8c
void BinkManager::restartBink()
{
    if (g_binkVideo) {
        binkgoto(g_binkVideo, 1, 1);
        binkdoframe(g_binkVideo);
        binkcopytobuffer(g_binkVideo, g_binkBuffer, g_binkPitch, g_binkHeight,
                          0, 0, g_binkSurfaceType);
    }
}

// E:\gamedcs\binkmanager.cpp:252, dc 0x50a90
VA(0x0044DAA0, 0x21A)  // dc-order-map + caller (smackmgr VideoNextFrame), dc 0x50a90
void BinkManager::nextBinkFrame()
{
    Bink* video = g_binkVideo;
    if (!video)
        video = g_binkVideo2;
    if (video && g_binkFrameReady && !binkwait(video)) {
        g_binkDirty = 1;
        if (g_binkPaused)
            return;

        binkdoframe(video);
        binkcopytobuffer(video, g_binkBuffer, g_binkPitch, g_binkHeight, 0, 0,
                          g_binkSurfaceType);

        if (video->m_frameNum == video->m_frames) {
            if (g_binkChainTrack) {
                if (g_binkVideo && g_binkVideo2) {
                    if (g_videoDescriptors[g_binkVideoId].m_fadeOnAbort)
                        g_windowManager->fadeScreen(1, 4, 0);
                    g_soundManager->serviceSounds();
                    binkclose(g_binkVideo);
                    g_binkVideo = 0;
                    video = g_binkVideo2;
                    if (g_videoDescriptors[g_binkVideoId].m_fadeInSecondTrack) {
                        binkdoframe(video);
                        binkcopytobuffer(video, g_binkBuffer, g_binkPitch,
                                          g_binkHeight, 0, 0, g_binkSurfaceType);
                        g_windowManager->fadeScreen(0, 4, 0);
                    }
                } else {
                    binknextframe(video);
                }
            } else {
                binkgetsummary(video, &g_binkSummary);
                BinkManager::closeBink();
                if (g_videoDescriptors[g_binkVideoId].m_fadeOnAbort)
                    g_windowManager->fadeScreen(1, 4, 0);
                else
                    g_windowManager->updateScreen(0, 0, 800, 600);
                return;
            }
        } else {
            binknextframe(video);
        }
        if (g_binkUseDirtyRects)
            videoDrawRects();
        return;
    }

    g_binkDirty = 0;
}

// E:\gamedcs\binkmanager.cpp:345 (dc 0x50a94) - the static-member spelling
VA(0x0044dcc0, 0x60)  // dc 0x50a94
void BinkManager::closeBink()
{
    if (g_binkVideo) {
        binkpause(g_binkVideo, 1);
        binkclose(g_binkVideo);
    }
    if (g_binkVideo2) {
        binkpause(g_binkVideo2, 1);
        binkclose(g_binkVideo2);
    }
    g_binkVideo2 = 0;
    g_binkVideo = 0;
    g_binkPaused = 0;
    g_binkFrameReady = 0;
    g_binkDirty = 0;
}

// E:\gamedcs\binkmanager.cpp:376 (dc 0x50a98) - the compiland's last row and
// smackmgr.cpp's VideoPlay tail-calls it by this static-member spelling. The
// SMACKER TWIN is VideoPlay's own non-bink arm, statement for statement:
// the same field_84 latch, the same `if (w < 0) vw = video->Width` pair, the
// same F4-exempt abort filter around PollSound/Process1WindowsMessage, and
// the same `aborted && fadeOnAbort` tail.
// Residual (88.1839%): the register-homing family, and it is a clean MIRROR.
// The call stream agrees 15 = 15, the branch count and the single return
// agree, and every value lands in the right place - retail just keeps `vh` in
// EBX with `vw` recycled into the `h` parameter home at [ebp+0x10], where our
// CL keeps `vw` in EBX with `vh` in the `w` home at [ebp+0xc], and the zero it
// compares against materialises in EAX on one side and not the other.
// Tried: swapping the vw/vh declaration order (+0.22 and no slot change),
// handing OpenBinkVideo `vw, vh` instead of `w, h` and testing `vw < 0`
// instead of `w < 0` - the twin's own spelling, kept - both byte-flat.
// The shared closeBink call replaces the duplicated pause/close/reset block
// at the same 88.1802%. Retail retains the four SDK calls in that expansion;
// the native cleanup oracle verifies both track orders and all flag resets.
VA(0x0044DD20, 0x227)  // dc-order-map + caller (smackmgr VideoPlay), dc 0x50a98
int BinkManager::playBink(int id, int x, int y, int w, int h)
{
    // Preserve the switch and use its existing aborted flag to break the
    // playback loop. This removes the cleanup jump at unchanged 88.1839%;
    // replacing the switch with a combined event condition loses matching.
    int vh = h;
    int vw = w;
    int updateX;
    int updateY;
    unsigned char result;
    unsigned char aborted;

    g_soundManager->m_playSounds = 1;
    BinkManager::openBink(id, x, y, vw, vh, 0, 0);
    if (!g_binkVideo) {
        result = 0;
    } else {
        g_mouseManager->hidePointer();
        if (vw < 0)
            vw = g_binkVideo->m_width;
        if (vh < 0)
            vh = g_binkVideo->m_height;
        if (id != VIDEO_ID_OVERLAY_BLIT) {
            updateX = x + (vw - g_binkVideo->m_width) / 2;
            g_binkX = updateX;
            updateY = y + (vh - g_binkVideo->m_height) / 2;
            g_binkY = updateY;
        } else {
            updateX = 0;
            updateY = 0;
            vw = g_binkVideo->m_width;
            vh = g_binkVideo->m_height;
        }
        g_binkBuffer = 2 * g_binkX
            + g_windowManager->m_screenBitmap->getPitch() * g_binkY
            + static_cast<unsigned char*>(
                  static_cast<void*>(g_windowManager->m_screenBitmap->getMap(0, 0)));
        aborted = 0;
        g_inputManager->flush();
        while (1) {
            if (g_binkVideo == 0)
                break;
            pollSound();
            process1WindowsMessage();
            {
                Message msg = g_inputManager->getEvent();
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
            }
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
    g_binkPaused = 0;
    g_binkFrameReady = 0;
    return result;
}
