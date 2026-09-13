// smackmgr.cpp - E:\gamedcs\smackmgr.cpp (compiland smackmgr.obj)
// 21 functions in link order.

// Retail dropped DC's SmackManager object for a flat global scheme:
// two Smacker handles (a video track and a separate audio-only track)
// plus a mirrored pair of Bink handles owned by the 0x44dxxx bink TU.
// The smackw32/binkw32 imports carry their own leading underscore
// (retail IAT: __imp___SmackToBuffer@28), the same RAD convention
// soundmgr.h documents for Miles (_AIL_*).
#include "terrain.h"
#include <va.h>
#include <windows.h>
#include <ddraw.h>
#include <string.h>
#include <string>
#include "smackmgr.h"
#include "binkmanager.h"
#include "wingraph.h"
#include "soundmgr.h"
#include "winmgr.h"
#include "mousemgr.h"
#include "inputmgr.h"
#include "kbwin.h"
#include "bitmap16.h"
#include "message.h"
#include "prefs.h"
#include "textresource.h"

// The drive letter the misc.obj install scan settled on. It is WRITTEN at
// 0x50c278 inside that scan (still unclaimed) and READ only there and by
// GetDriveArchivePath below, which is why the definition is left with its
// owning TU: 0x6839b8's .data neighbours are singleselectionwindow's and
// spellbookwindow's blocks, not smackmgr's. Retail initialises it to 'd'.
DATA(0x006839b8) extern char g_archiveDriveLetter;

// The Smack/Bink handle views and the smackw32/binkw32 dllimport
// surface live in smackmgr.h / binkmanager.h. Calls use the exact
// underscored RAD import names, without TU-local spelling aliases.
// Removed aliases: SmackToBuffer, SmackToBufferRect, SmackDoFrame, SmackGoto,
// SmackClose, BinkPause, BinkDDSurfaceType, BinkGetRects, SmackWait,
// SmackNextFrame, SmackOpen, SmackUseMMX, SmackVolumePan.

// SmackToBuffer surface-format flags (radlib smack.h).
static const unsigned int g_smackBuffer555 = 0x80000000;
static const unsigned int g_smackBuffer565 = 0xC0000000;

// The constant half of OpenSmackerTrack's SmackOpen mask; the 0x1000 bit is
// what makes SmackOpen read from an already-open archive HANDLE.
static const int g_smackOpenFromArchive = 0x1140;

// The Smacker track selection mask ShowVideo hands SmackOpen and
// SmackVolumePan. The extra bit follows the same no-frame-skip policy
// as Bink's SDK-defined BINKNOSKIP in the parallel opener.
static const int g_smackTrackMask = 0xfe000;
static const int g_smackOpenNoFrameSkip = 0x200;

void showVideo(int id, int x, int y, int w, int h, int a6, int a7, int a8);
namespace SmackManager {
void nextSmackerFrame();
void closeSmacker();
}

// smackmgr.obj .bss cluster 0x69fdf5..0x69fe5c (names provisional;
// the unreferenced gaps belong to members only the 0x198xxx TU
// touches).
DATA(0x0069fdf4) unsigned char g_smackAutoDraw;     // ShowVideo arg 7: pump its own VideoDrawRects
DATA(0x0069fdf5) unsigned char g_smackDirty;        // decoded frame awaits a blit
DATA(0x0069fdf8) Smack* g_smackVideo;               // video track handle
DATA(0x0069fdfc) Smack* g_smackVideo2;              // audio-only track handle
DATA(0x0069fe00) int g_smackX;                      // blit origin on the screen bitmap
// ShowVideo latches its own arguments into this quartet for the pump to
// read back: the requested extent, the id it dispatched on, whether the
// last frame loops, and whether NextSmackerFrame may advance at all.
DATA(0x0069fe04) int g_smackY;
DATA(0x0069fe08) int g_smackReqWidth;               // ShowVideo arg 4
DATA(0x0069fe0c) int g_smackReqHeight;              // ShowVideo arg 5
DATA(0x0069fe10) int g_smackVideoId;                // ShowVideo arg 1
DATA(0x0069fe14) int g_smackLoop;                   // ShowVideo arg 6
// The descriptor row ShowVideo takes its audio-track flag from, which is
// NOT the id it is opening (that goes to gSmackVideoId above). Provisional.
DATA(0x0069fe1c) int g_smackAdvance;                // ShowVideo arg 8 (byte-narrowed on the way in)
// Nonzero once ShowVideo has decided the Miles driver is live and the user
// has sound turned up; it is what selects the 0xfe000 track mask on both
// SmackOpen calls. Provisional.
DATA(0x0069fdec) int g_videoDescriptorIndex;
// Entry counts, one per archive directory; each is the dword LoadAnimHeaders
// reads before sizing its (count + 2) allocation.
DATA(0x0069fe58) int g_videoSoundReady;
DATA(0x0069fde4) int g_videoCount1;
DATA(0x0069fe34) int g_videoCount2;
DATA(0x0069fe3c) int g_videoCount3;
DATA(0x0069fe18) int g_smackPaused;
DATA(0x0069fe20) unsigned long g_smackBufferFlags;  // g_smackBuffer555/565
DATA(0x0069fddc) int g_redShift;
DATA(0x0069fdd8) int g_redBits;
DATA(0x0069fe40) int g_greenShift;
DATA(0x0069fe30) int g_greenBits;
DATA(0x0069fde8) int g_blueShift;
DATA(0x0069fe38) int g_blueBits;
DATA(0x0069fe44) int g_videoPauseCount;
DATA(0x0069fe48) HANDLE g_videoFile1;               // VideoShutDown's CloseHandle trio
DATA(0x0069fe4c) HANDLE g_videoFile2;
// The three archive directories the handles above index. LoadAnimHeaders
// (0x598210) pairs them one for one - it stores 0x69fe2c right after opening
// the file it puts in gVideoFile3, 0x69fe28 with gVideoFile1 and 0x69fde0
// with gVideoFile2 - and DeleteAnimHeaders then frees them 3, 2, 1, exactly
// the order VideoShutDown closes the handles in. Names provisional.
DATA(0x0069fe50) HANDLE g_videoFile3;
DATA(0x0069fde0) VideoHeaderStruct* g_videoHeader2;
DATA(0x0069fe28) VideoHeaderStruct* g_videoHeader1;
DATA(0x0069fe2c) VideoHeaderStruct* g_videoHeader3;
DATA(0x0069fe54) int g_inVideoNextFrame;            // reentry latch
DATA(0x0069fe5c) unsigned char g_smackFrameReady;   // SmackDoFrame is allowed

// Dreamcast proves the first two file/header pairs and their types in
// DeleteSoundHeaders (dc 0x14acc8). Complete's retail loader at 0x5984a0
// adds a third archive built from the active campaign name. The retail
// teardown below independently proves all six addresses and the sentinel
// states: file handles live in .data as INVALID_HANDLE_VALUE; header arrays
// live in .bss as null pointers.
DATA(0x00682e80) HANDLE g_soundFile = INVALID_HANDLE_VALUE;
DATA(0x0069d848) SoundHeaderStruct* g_soundHeader;
DATA(0x00682e7c) HANDLE g_soundFileCd = INVALID_HANDLE_VALUE;
DATA(0x0069d850) SoundHeaderStruct* g_soundHeaderCd;
DATA(0x00682e84) HANDLE g_soundFileCampaign = INVALID_HANDLE_VALUE;
DATA(0x0069d84c) SoundHeaderStruct* g_soundHeaderCampaign;
DATA(0x0069e5a8) int g_soundCount;
DATA(0x0069e5a4) int g_soundCountCd;
DATA(0x0069e5ac) int g_soundCountCampaign;

VA(0x005971b0, 0x3B)  // dc 0x14ac30
void videoSoundOnOff(int on)
{
    if (g_smackVideo || g_smackVideo2)
        g_soundManager->serviceSounds();
    else if (g_binkVideo || g_binkVideo2)
        g_soundManager->serviceSounds();
}

VA(0x005971f0, 0xD9)  // dc 0x14ac34
void videoRealignBuffers()
{
    g_smackBufferFlags = (g_greenBits == VIDEO_PIXEL_FORMAT_RGB565)
                            ? g_smackBuffer565 : g_smackBuffer555;
    if (g_smackVideo)
        _SmackToBuffer(g_smackVideo, g_smackX, g_smackY,
            g_windowManager->m_screenBitmap->getPitch(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getMap(0, 0), g_smackBufferFlags);
    if (g_smackVideo2)
        _SmackToBuffer(g_smackVideo2, g_smackX, g_smackY,
            g_windowManager->m_screenBitmap->getPitch(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getMap(0, 0), g_smackBufferFlags);
    g_binkSurfaceType = _BinkDDSurfaceType(g_ddsBack);
    g_binkBuffer = static_cast<unsigned char*>(static_cast<void*>(g_windowManager->m_screenBitmap->getMap(g_binkX, g_binkY)));
    g_binkPitch = g_windowManager->m_screenBitmap->getPitch();
    g_binkHeight = g_windowManager->m_screenBitmap->getHeight();
}

VA(0x005972d0, 0x29D)  // dc 0x14ac38
int videoPlay(int id, int x, int y, int w, int h)
{
    POINT pos;
    int vw, vh;
    unsigned char result;
    unsigned char aborted;

    if (id >= VIDEO_ID_FIRST_TABLED
        && (!g_videoDescriptors[id].m_useBink || !g_unnamed698758.m_binkVideo
            || (id == VIDEO_ID_STATE_GATED
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_LOW
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {
        vh = h;
        vw = w;
        g_soundManager->m_playSounds = 1;
        showVideo(id, x, y, vw, vh, 0, 0, 1);
        if (!g_smackVideo) {
            result = 0;
        } else {
            g_mouseManager->hidePointer();
            if (vw < 0)
                vw = g_smackVideo->m_width;
            if (vh < 0)
                vh = g_smackVideo->m_height;
            g_smackX = x + (vw - g_smackVideo->m_width) / 2;
            g_smackY = y + (vh - g_smackVideo->m_height) / 2;
            pos.x = g_smackX;
            pos.y = g_smackY;
            _SmackToBuffer(g_smackVideo, g_smackX, g_smackY,
                g_windowManager->m_screenBitmap->getPitch(),
                g_windowManager->m_screenBitmap->getHeight(),
                g_windowManager->m_screenBitmap->getMap(0, 0), g_smackBufferFlags);
            aborted = 0;
            g_inputManager->flush();
            while (1) {
                if (g_smackVideo == 0)
                    break;
                pollSound();
                process1WindowsMessage();
                {
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
                }
                if (videoNeedsUpdate())
                    videoDrawRects();
            }
            SmackManager::closeSmacker();
            if (aborted && g_videoDescriptors[id].m_fadeOnAbort)
                g_windowManager->fadeScreen(1, 4, 0);
            else
                g_windowManager->updateScreen(pos.x, pos.y, vw, vh);
            g_mouseManager->showPointer(0);
            result = !aborted;
        }
        g_smackPaused = 0;
        g_smackFrameReady = 0;
        return result;
    }
    return playBinkVideo(id, x, y, w, h);
}

VA(0x00597570, 0x75)  // dc 0x14ac3c
void videoOpen(int id, int x, int y, int w, int h, int a6, int a7, int a8)
{
    if (id >= VIDEO_ID_FIRST_TABLED
        && (!g_videoDescriptors[id].m_useBink || !g_unnamed698758.m_binkVideo
            || (id == VIDEO_ID_STATE_GATED
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_LOW
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH)))
        showVideo(id, x, y, w, h, a6, a7, a8);
    else
        openBinkVideo(id, x, y, w, h, a6, a7);
}

VA(0x005975f0, 0xE1)  // dc 0x14ac40
void videoClose()
{
    while (g_videoPauseCount != 0)
        videoResume();
    g_soundManager->serviceSounds();
    SmackManager::closeSmacker();
    closeBinkVideo();
}

VA(0x005976e0, 0x5E)  // dc 0x14ac44
void videoNextFrame()
{
    if (g_inVideoNextFrame)
        return;
    g_inVideoNextFrame = 1;
    if (g_smackVideo || g_smackVideo2) {
        if (!g_smackPaused)
            SmackManager::nextSmackerFrame();
    }
    if (g_binkVideo || g_binkVideo2) {
        if (!g_binkPaused)
            nextBinkFrame();
    }
    g_inVideoNextFrame = 0;
}

VA(0x00597740, 0x53)  // dc 0x14ac48
void videoDrawCurrentFrame()
{
    if (g_smackVideo || g_smackVideo2) {
        if (!g_smackPaused && g_smackVideo && g_smackFrameReady)
            _SmackDoFrame(g_smackVideo);
    }
    if (g_binkVideo || g_binkVideo2) {
        if (!g_binkPaused)
            drawCurrentBinkFrame();
    }
}

VA(0x005977a0, 0xA6)  // dc 0x14ac4c
void videoPause()
{
    if (++g_videoPauseCount > 1)
        return;
    if (g_smackVideo || g_smackVideo2)
        g_smackPaused = 1;
    if (g_binkVideo) {
        g_binkPaused = 1;
        _BinkPause(g_binkVideo, 1);
    }
    if (g_binkVideo2) {
        g_binkPaused = 1;
        _BinkPause(g_binkVideo2, 1);
    }
    videoSoundOnOff(0);
}

VA(0x00597850, 0xAB)  // dc 0x14ac50
void videoResume()
{
    if (g_videoPauseCount == 0)
        return;
    if (--g_videoPauseCount != 0)
        return;
    if (g_smackVideo || g_smackVideo2)
        g_smackPaused = 0;
    if (g_binkVideo) {
        g_binkPaused = 0;
        _BinkPause(g_binkVideo, 0);
    }
    if (g_binkVideo2) {
        g_binkPaused = 0;
        _BinkPause(g_binkVideo2, 0);
    }
    videoSoundOnOff(1);
}

VA(0x00597900, 0x23)  // dc 0x14ac54
void videoRestart()
{
    if (g_smackVideo) {
        _SmackGoto(g_smackVideo, 1);
        _SmackDoFrame(g_smackVideo);
    }
    restartBinkVideo();
}

VA(0x00597930, 0x5A)  // dc 0x14ac58
unsigned char videoNeedsUpdate()
{
    if (g_smackVideo || g_smackVideo2)
        return g_smackDirty && !g_smackPaused;
    else if (g_binkVideo || g_binkVideo2)
        return g_binkDirty && !g_binkPaused;
    return 0;
}

VA(0x00597990, 0x3F)  // dc 0x14ac5c
unsigned char videoPlaying()
{
    if ((g_smackVideo || g_smackVideo2) && !g_smackPaused)
        return 1;
    if ((g_binkVideo || g_binkVideo2) && !g_binkPaused)
        return 1;
    return 0;
}

// E:\gamedcs\smackmgr.cpp:328
// Merges the pending dirty rects into one union rect and pushes it to
// the screen; the id-0x1d bink instead Blts the whole 800x600 back
// surface to the primary around an Unlock/Lock pair.
// Residual (90.3379%): the SDK BinkRect bounds object and field references
// remain; POINT, source RECT and DDSURFACEDESC belong to function scope,
// while the destination RECT stays in the overlay branch. This natural
// declaration-scope candidate improves the previous 89.9772% reconstruction.
// DC0x14ac60 is a four-byte platform stub; it proves no body/local scopes.
// The 36-state SDK field-result/geometry-scope family emitted 18 objects,
// and the 16-state individual-object-scope family emitted eight. Capturing
// rectangle fields in extra locals lost matching; the retained scope
// reduces the frame from all-function 0xa8 to 0x98 (retail 0x9c).
// A 25-state scope/bound-type family emitted 12 objects. Its long-scalar
// parent db3d1eb678b25767ba0c04a1 reaches 89.7123% but reproduces every
// DirectDraw address (POINT -0x10, dst -0x20, src -0x30, ddsd -0x9c).
// Keep that reproduced parent as a lead: registers/scheduling still differ.
// Recombining both parents with independent SDK captures and source-RECT
// initialization stages exhausted 73 states/17 objects, with no further
// gain. All 28 exact smackmgr siblings remain exact. The earlier 48-state
// bound-storage/selection family, rect walkers and cached handles also
// failed to close the union-loop register homes and Bink induction base.
// Keep the original update order: changing x/y before the extent compares
// is retail behavior, even though it is not a general rectangle-union API.
VA(0x005979d0, 0x294)  // anchor-global, dc 0x14ac60
void videoDrawRects()
{
    POINT pt;
    RECT src;
    DDSURFACEDESC ddsd;

    BinkRect bounds;
    long& x = bounds.m_left;
    long& y = bounds.m_top;
    long& w = bounds.m_width;
    long& h = bounds.m_height;

    if ((g_smackVideo || g_smackVideo2) && !g_smackPaused) {
        Smack* smk;

        smk = 0;
        if (g_smackVideo)
            smk = g_smackVideo;
        else if (g_smackVideo2)
            smk = g_smackVideo2;
        _SmackToBufferRect(smk, g_smackBufferFlags);
        w = smk->m_lastRectw;
        h = smk->m_lastRecth;
        x = smk->m_lastRectx;
        y = smk->m_lastRecty;
        while (_SmackToBufferRect(smk, g_smackBufferFlags)) {
            if (smk->m_lastRectx < x)
                x = smk->m_lastRectx;
            if (smk->m_lastRecty < y)
                y = smk->m_lastRecty;
            if (smk->m_lastRectw + smk->m_lastRectx > w + x)
                w = smk->m_lastRectw + smk->m_lastRectx - x;
            if (smk->m_lastRecth + smk->m_lastRecty > h + y)
                h = smk->m_lastRecth + smk->m_lastRecty - y;
        }
        g_windowManager->updateScreen(x, y, w, h);
    } else if ((g_binkVideo || g_binkVideo2) && !g_binkPaused) {
        Bink* bnk;

        bnk = g_binkVideo2;
        if (g_binkVideo)
            bnk = g_binkVideo;
        if (g_binkVideoId != VIDEO_ID_OVERLAY_BLIT) {
            int i;

            _BinkGetRects(bnk, g_binkSurfaceType);
            w = bnk->m_frameRects[0].m_width;
            h = bnk->m_frameRects[0].m_height;
            x = bnk->m_frameRects[0].m_left;
            y = bnk->m_frameRects[0].m_top;
            for (i = 1; i < bnk->m_numRects; i++) {
                if (bnk->m_frameRects[i].m_left < x)
                    x = bnk->m_frameRects[i].m_left;
                if (bnk->m_frameRects[i].m_top < y)
                    y = bnk->m_frameRects[i].m_top;
                if (bnk->m_frameRects[i].m_width + bnk->m_frameRects[i].m_left > w + x)
                    w = bnk->m_frameRects[i].m_width + bnk->m_frameRects[i].m_left - x;
                if (bnk->m_frameRects[i].m_height + bnk->m_frameRects[i].m_top > h + y)
                    h = bnk->m_frameRects[i].m_height + bnk->m_frameRects[i].m_top - y;
            }
            g_windowManager->updateScreen(g_binkX + x, g_binkY + y, w, h);
        } else {
            RECT dst;

            pt.x = 0;
            pt.y = 0;
            ClientToScreen(g_hwndApp, &pt);
            dst.left = 0;
            dst.top = 0;
            dst.right = 800;
            dst.bottom = 600;
            OffsetRect(&dst, pt.x, pt.y);
            src.left = 0;
            src.top = 0;
            src.right = bnk->m_width;
            src.bottom = bnk->m_height;
            if (g_ddsBack->Unlock(NULL) != 0)
                return;
            g_ddsPrimary->Blt(&dst, g_ddsBack, &src, DDBLT_WAIT, NULL);
            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            g_ddsBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
        }
    }
    g_binkDirty = 0;
    g_smackDirty = 0;
}

VA(0x00597c70, 0x84)  // dc 0x14ac64
void videoShutDown()
{
    SmackManager::closeSmacker();
    closeBinkVideo();
    if (g_videoFile3)
        CloseHandle(g_videoFile3);
    if (g_videoFile2)
        CloseHandle(g_videoFile2);
    if (g_videoFile1)
        CloseHandle(g_videoFile1);
    g_videoFile1 = 0;
    g_videoFile2 = 0;
    g_videoFile3 = 0;
}

// The base archive's path on the drive the misc.obj install scan selected.
// That scan (in the unclaimed 0x50c1cb body) walks GetLogicalDrives, writes
// each candidate letter to gArchiveDriveLetter, and _open()s what this
// function returns; smackmgr's own two loaders then open their third archive
// from it, the sound loader after swapping the extension for .snd.

VA(0x00597d00, 0x50E)
std::string getDriveArchivePath()
{
    std::string path;
    path = g_archiveDriveLetter;
    path += '\xe3';
    path += '\xad';
    path += '\xa9';
    path += '\xac';
    path += '\xb2';
    path += '\xae';
    path += '\xab';
    path += '\xfc';
    path += '\xd6';
    path += '\xbe';
    path += '\xa2';
    path += '\xac';
    path += '\xaa';
    path += '\x82';
    path += '\xa7';
    path += '\xbc';
    path += '\xf8';
    path += '\xb6';
    path += '\xba';
    path += '\xba';
    path += '\xf2';
    path += '\xa4';
    path += '\xbd';
    path += '\xb0';
    path += '\xbd';
    path[1] ^= '\xd9';
    path[2] ^= '\xc5';
    path[3] ^= '\xcc';
    path[4] ^= '\xde';
    path[5] ^= '\xdd';
    path[6] ^= '\xcb';
    path[7] ^= '\xd8';
    path[8] ^= '\xcf';
    path[9] ^= '\x8a';
    path[10] ^= '\xda';
    path[11] ^= '\xc3';
    path[12] ^= '\xd8';
    path[13] ^= '\xcb';
    path[14] ^= '\xde';
    path[15] ^= '\xcf';
    path[16] ^= '\xd9';
    path[17] ^= '\x8a';
    path[18] ^= '\xd9';
    path[19] ^= '\xdf';
    path[20] ^= '\xc9';
    path[21] ^= '\xc1';
    path[22] ^= '\x8a';
    path[23] ^= '\xcb';
    path[24] ^= '\xd9';
    path[25] ^= '\xd9';
    return path;
}

VA(0x00598210, 0x223)  // dc 0x14ac68
unsigned char loadAnimHeaders()
{
    DWORD nread;

    g_videoFile3 = CreateFileA(getDriveArchivePath().c_str(), GENERIC_READ,
        FILE_SHARE_READ, 0, OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
    if (g_videoFile3 != INVALID_HANDLE_VALUE) {
        ReadFile(g_videoFile3, &g_videoCount3, 4, &nread, 0);
        g_videoHeader3 = new VideoHeaderStruct[g_videoCount3 + 2];
        ReadFile(g_videoFile3, g_videoHeader3, 44 * g_videoCount3, &nread, 0);
    } else {
        g_videoFile3 = 0;
    }

    if (*g_videoGameState == VIDEO_GAME_STATE_EXPANSION_ARCHIVES
        || *g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        g_videoFile1 = CreateFileA("data\\h3ab_ahd.vid", GENERIC_READ,
            FILE_SHARE_READ, 0, OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
        if (g_videoFile1 == INVALID_HANDLE_VALUE) {
            MessageBoxA(g_hwndApp, g_generalText->getText(535),
                g_generalText->getText(536), 0);
            return 0;
        }
        ReadFile(g_videoFile1, &g_videoCount1, 4, &nread, 0);
        g_videoHeader1 = new VideoHeaderStruct[g_videoCount1 + 2];
        ReadFile(g_videoFile1, g_videoHeader1, 44 * g_videoCount1, &nread, 0);
    }

    g_videoFile2 = CreateFileA("data\\Video.vid", GENERIC_READ, FILE_SHARE_READ,
        0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
    if (g_videoFile2 == INVALID_HANDLE_VALUE) {
        MessageBoxA(g_hwndApp, g_generalText->getText(535),
            g_generalText->getText(536), 0);
        return 0;
    }
    ReadFile(g_videoFile2, &g_videoCount2, 4, &nread, 0);
    g_videoHeader2 = new VideoHeaderStruct[g_videoCount2 + 2];
    ReadFile(g_videoFile2, g_videoHeader2, 44 * g_videoCount2, &nread, 0);
    return 1;
}

VA(0x00598440, 0x55)  // dc 0x14ac6c
void deleteAnimHeaders()
{
    if (g_videoHeader3) {
        delete[] g_videoHeader3;
        g_videoHeader3 = 0;
    }
    if (g_videoHeader2) {
        delete[] g_videoHeader2;
        g_videoHeader2 = 0;
    }
    if (g_videoHeader1) {
        delete[] g_videoHeader1;
        g_videoHeader1 = 0;
    }
}

VA(0x005984a0, 0x240)  // dc 0x14aca0
unsigned char loadSoundHeaders()
{
    DWORD nread;
    char path[52];

    g_soundFile = CreateFileA("data\\heroes3.snd", GENERIC_READ, FILE_SHARE_READ,
        0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
    if (g_soundFile == INVALID_HANDLE_VALUE) {
        MessageBoxA(g_hwndApp, g_generalText->getText(664),
            g_generalText->getText(665), 0);
        return 0;
    }
    ReadFile(g_soundFile, &g_soundCount, 4, &nread, 0);
    g_soundHeader = new SoundHeaderStruct[g_soundCount + 2];
    ReadFile(g_soundFile, g_soundHeader, 48 * g_soundCount, &nread, 0);

    if (*g_videoGameState == VIDEO_GAME_STATE_EXPANSION_ARCHIVES
        || *g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        g_soundFileCd = CreateFileA("data\\h3ab_ahd.snd", GENERIC_READ,
            FILE_SHARE_READ, 0, OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
        if (g_soundFileCd == INVALID_HANDLE_VALUE) {
            MessageBoxA(g_hwndApp, g_generalText->getText(664),
                g_generalText->getText(665), 0);
            return 0;
        }
        ReadFile(g_soundFileCd, &g_soundCountCd, 4, &nread, 0);
        g_soundHeaderCd = new SoundHeaderStruct[g_soundCountCd + 2];
        ReadFile(g_soundFileCd, g_soundHeaderCd, 48 * g_soundCountCd, &nread, 0);
    }

    strcpy(path, getDriveArchivePath().c_str());
    strtok(path, ".");
    strcat(path, ".snd");
    g_soundFileCampaign = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
    if (g_soundFileCampaign != INVALID_HANDLE_VALUE) {
        ReadFile(g_soundFileCampaign, &g_soundCountCampaign, 4, &nread, 0);
        g_soundHeaderCampaign = new SoundHeaderStruct[g_soundCountCampaign + 2];
        ReadFile(g_soundFileCampaign, g_soundHeaderCampaign,
            48 * g_soundCountCampaign, &nread, 0);
    }
    return 1;
}

VA(0x005986e0, 0xA2)
void deleteSoundHeaders()
{
    if (g_soundFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_soundFile);
        g_soundFile = INVALID_HANDLE_VALUE;
    }
    if (g_soundHeader) {
        delete[] g_soundHeader;
        g_soundHeader = 0;
    }
    if (g_soundFileCd != INVALID_HANDLE_VALUE) {
        CloseHandle(g_soundFileCd);
        g_soundFileCd = INVALID_HANDLE_VALUE;
    }
    if (g_soundHeaderCd) {
        delete[] g_soundHeaderCd;
        g_soundHeaderCd = 0;
    }
    if (g_soundFileCampaign != INVALID_HANDLE_VALUE) {
        CloseHandle(g_soundFileCampaign);
        g_soundFileCampaign = INVALID_HANDLE_VALUE;
    }
    if (g_soundHeaderCampaign) {
        delete[] g_soundHeaderCampaign;
        g_soundHeaderCampaign = 0;
    }
}

VA(0x00598790, 0x2AB)
Smack* openSmackerTrack(const char* stem, unsigned long flags,
                        unsigned long extraFlags)
{
    char name[40];
    int i;

    strcpy(name, stem);
    strcat(name, ".smk");

    if (*g_videoGameState == VIDEO_GAME_STATE_EXPANSION_ARCHIVES) {
        for (i = 0; i < g_videoCount1; i++) {
            if (_strcmpi(g_videoHeader1[i].m_name, name) == 0) {
                g_soundManager->serviceSounds();
                SetFilePointer(g_videoFile1, g_videoHeader1[i].m_offset, 0,
                    FILE_BEGIN);
                return _SmackOpen(g_videoFile1,
                    flags | extraFlags | g_smackOpenFromArchive, -1);
            }
        }
    }

    for (i = 0; i < g_videoCount2; i++) {
        if (_strcmpi(g_videoHeader2[i].m_name, name) == 0) {
            g_soundManager->serviceSounds();
            SetFilePointer(g_videoFile2, g_videoHeader2[i].m_offset, 0, FILE_BEGIN);
            return _SmackOpen(g_videoFile2,
                flags | extraFlags | g_smackOpenFromArchive, -1);
        }
    }

    if (g_videoFile3) {
        for (i = 0; i < g_videoCount3; i++) {
            if (_strcmpi(g_videoHeader3[i].m_name, name) == 0) {
                g_soundManager->serviceSounds();
                SetFilePointer(g_videoFile3, g_videoHeader3[i].m_offset, 0,
                    FILE_BEGIN);
                return _SmackOpen(g_videoFile3,
                    flags | extraFlags | g_smackOpenFromArchive, -1);
            }
        }
    }

    if (*g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        for (i = 0; i < g_videoCount1; i++) {
            if (_strcmpi(g_videoHeader1[i].m_name, name) == 0) {
                g_soundManager->serviceSounds();
                SetFilePointer(g_videoFile1, g_videoHeader1[i].m_offset, 0,
                    FILE_BEGIN);
                return _SmackOpen(g_videoFile1,
                    flags | extraFlags | g_smackOpenFromArchive, -1);
            }
        }
    }
    return 0;
}

VA(0x00598a40, 0xA2)
void SmackManager::setPixelFormat(unsigned long redMask,
                                  unsigned long greenMask,
                                  unsigned long blueMask)
{
    g_redShift = 0;
    while (!(redMask & 1) && redMask) {
        ++g_redShift;
        redMask >>= 1;
    }
    g_redBits = 0;
    while (redMask) {
        ++g_redBits;
        redMask >>= 1;
    }

    g_greenShift = 0;
    while (!(greenMask & 1) && greenMask) {
        ++g_greenShift;
        greenMask >>= 1;
    }
    g_greenBits = 0;
    while (greenMask) {
        ++g_greenBits;
        greenMask >>= 1;
    }

    g_blueShift = 0;
    while (!(blueMask & 1) && blueMask) {
        ++g_blueShift;
        blueMask >>= 1;
    }
    g_blueBits = 0;
    while (blueMask) {
        ++g_blueBits;
        blueMask >>= 1;
    }
}

VA(0x00598af0, 0x385)
void showVideo(int id, int x, int y, int w, int h, int loop, int autoDraw,
               int advance)
{
    if (g_unnamed699290 == 0 && g_soundManager->m_ds != 0
        && g_unnamed698758.m_soundVolume != 0)
        g_videoSoundReady = 1;
    else
        g_videoSoundReady = 0;

    videoClose();
    _SmackUseMMX(1);

    unsigned long trackMask = g_videoSoundReady ? g_smackTrackMask : 0;
    unsigned long videoMode =
        g_videoDescriptors[static_cast<unsigned char>(g_videoDescriptorIndex)].m_noFrameSkip
            ? g_smackOpenNoFrameSkip : 0;

    g_smackVideoId = id;
    g_smackPaused = 0;
    g_smackVideo2 = 0;
    g_smackBufferFlags = (g_greenBits == VIDEO_PIXEL_FORMAT_RGB565)
                            ? g_smackBuffer565 : g_smackBuffer555;
    g_smackAdvance = static_cast<unsigned char>(advance);

    if (g_videoDescriptors[id].m_smkAudioStem != "") {
        g_smackVideo2 = openSmackerTrack(g_videoDescriptors[id].m_smkAudioStem,
            trackMask, g_smackOpenNoFrameSkip);
        if (!g_smackVideo2) {
            videoClose();
            return;
        }
        _SmackVolumePan(g_smackVideo2, g_smackTrackMask,
            3640 * g_unnamed698758.m_soundVolume, 0x8000);
        _SmackToBuffer(g_smackVideo2, x, y,
            g_windowManager->m_screenBitmap->getPitch(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getMap(0, 0), g_smackBufferFlags);
    }

    g_smackVideo = openSmackerTrack(g_videoDescriptors[id].m_smkStem,
        trackMask, videoMode);
    if (!g_smackVideo) {
        videoClose();
        return;
    }

    g_smackAutoDraw = autoDraw;
    if (w <= 0)
        w = g_smackVideo->m_width;
    if (h <= 0)
        h = g_smackVideo->m_height;
    g_smackReqWidth = w;
    g_smackReqHeight = h;
    g_smackLoop = loop;
    g_smackX = x;
    g_smackY = y;
    _SmackVolumePan(g_smackVideo, g_smackTrackMask,
        3640 * g_unnamed698758.m_soundVolume, 0x8000);
    _SmackToBuffer(g_smackVideo, x, y,
        g_windowManager->m_screenBitmap->getPitch(),
        g_windowManager->m_screenBitmap->getHeight(),
        g_windowManager->m_screenBitmap->getMap(0, 0), g_smackBufferFlags);
    if (g_smackVideo2)
        _SmackToBuffer(g_smackVideo2, x, y,
            g_windowManager->m_screenBitmap->getPitch(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getMap(0, 0), g_smackBufferFlags);
    g_smackFrameReady = 1;
}

// The three flat Smacker-track wrappers that close out smackmgr.obj.
// Complete's smack layer is a pair of module globals rather than the
// Dreamcast's SmackManager object, so these keep the DC compiland's
// namespace spelling. Retail's own VideoDrawCurrentFrame (0x597740)
// and VideoClose (0x5975f0) expand the first and third bodies inline,
// which is what proves the guard order and the store order here.
namespace SmackManager {

VA(0x00598e80, 0x25)
void drawSmackerFrame()
{
    if (g_smackVideo && g_smackFrameReady && !g_smackPaused)
        _SmackDoFrame(g_smackVideo);
}

VA(0x00598eb0, 0x193)  // dc 0x14adfc
void nextSmackerFrame()
{
    Smack* smk = g_smackVideo;
    if (!smk)
        smk = g_smackVideo2;
    g_smackDirty = smk && g_smackFrameReady && !_SmackWait(smk) && g_smackAdvance;
    if (!g_smackDirty)
        return;
    if (g_smackPaused)
        return;
    _SmackDoFrame(smk);
    if (smk->m_frameNum < smk->m_frames - 1) {
        _SmackNextFrame(smk);
    } else if (g_smackLoop) {
        if (g_smackVideo && g_smackVideo2) {
            if (g_videoDescriptors[g_smackVideoId].m_fadeOnAbort)
                g_windowManager->fadeScreen(1, 4, 0);
            g_soundManager->serviceSounds();
            _SmackClose(g_smackVideo);
            g_smackVideo = 0;
            if (g_videoDescriptors[g_smackVideoId].m_fadeInSecondTrack) {
                _SmackDoFrame(g_smackVideo2);
                g_windowManager->fadeScreen(0, 4, 0);
            }
        } else {
            _SmackNextFrame(smk);
        }
    } else {
        if (g_smackVideo)
            _SmackClose(g_smackVideo);
        if (g_smackVideo2)
            _SmackClose(g_smackVideo2);
        g_smackVideo2 = 0;
        g_smackVideo = 0;
        g_smackPaused = 0;
        g_smackFrameReady = 0;
        g_smackDirty = 0;
        if (g_videoDescriptors[g_smackVideoId].m_fadeOnAbort)
            g_windowManager->fadeScreen(1, 4, 0);
        else
            g_windowManager->updateScreen(0, 0, 0x320, 0x258);
        return;
    }
    if (g_smackAutoDraw)
        videoDrawRects();
}

VA(0x00599050, 0x43)  // dc 0x14ae00
void closeSmacker()
{
    if (g_smackVideo)
        _SmackClose(g_smackVideo);
    if (g_smackVideo2)
        _SmackClose(g_smackVideo2);
    g_smackVideo2 = 0;
    g_smackVideo = 0;
    g_smackPaused = 0;
    g_smackFrameReady = 0;
    g_smackDirty = 0;
}

VA(0x005990a0, 0x1C)
void gotoSmackerFrame(unsigned long frame)
{
    if (g_smackVideo && g_smackFrameReady)
        _SmackGoto(g_smackVideo, frame);
}

}  // namespace SmackManager

#if 0  // @carcass

// E:\gamedcs\smackmgr.cpp:569
DC_ONLY(0x14ac6c, 0x32)
void deleteAnimHeaders()
{
    // @stub
}

#endif  // @carcass

// COMDAT pairing: basic_string<char>::append(size_t, char) - the fill form,
// which the mangled suffix `@ID@Z` separates from the already-modelled
// pointer form `@PBDI@Z`. Agreement 0.994 here against 0.834 for the
// pointer-form objects, so the suffix and the score agree.
VA_COMPGEN(0x004b5f40, 0xCF, BASIC_STRING_APPEND_COUNT, char)

// COMDAT pairing: basic_string<char>::assign(size_t, char) - `ret 8` for its
// two stack arguments, against the `@ABV12@II@Z` form's three and the
// `@PBDI@Z` form's two pointers. smackmgr.obj emits exactly these two assign
// overloads and the other is claimed at 0x4860.
VA_COMPGEN(0x0051a780, 0xBC, BASIC_STRING_ASSIGN_COUNT, char)

// COMDAT pairing: basic_string<char>::operator[](size_t), `ret 4`.
VA_COMPGEN(0x0051a840, 0x98, BASIC_STRING_SUBSCRIPT, char)

// COMDAT pairing: basic_string<char>::basic_string(const basic_string&).
// smackmgr.obj emits exactly ONE string constructor, so the ctor group has a
// single member and binds directly - no zip to get wrong. `ret 4` matches the
// one reference argument, and the callers span customcampaign, game (twice),
// this unit and three further segments.
VA_COMPGEN(0x00460700, 0x125, CLASS_CTOR, basic_string)
