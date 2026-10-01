// 21 functions in link order.

// DC publics identify SmackManager as a namespace. Windows uses two
// Smacker handles (video and audio-only tracks); Bink's parallel state
// belongs to the binkmanager.cpp namespace.
// The vendored RAD headers own the Smacker and Bink import interfaces.
#include "va.h"

#include "platform.h"
#include <ddraw.h>
#include <string>
#include <string.h>

#include "smackmgr.h"

#include "binkmanager.h"
#include "bitmap16.h"
#include "inputmgr.h"
#include "kbwin.h"
#include "message.h"
#include "mousemgr.h"
#include "prefs.h"
#include "soundmgr.h"
#include "terrain.h"
#include "textresource.h"
#include "wingraph.h"
#include "winmgr.h"

// Initial contents recovered from the pinned Complete image.
DATA(0x006839c0) SVideoDescriptor g_videoDescriptors[141] = {
    { "Win3", "", 1, 0, 0, 0, { 0 } },
    { "LoseCstl", "LoseCslp", 1, 0, 0, 0, { 0 } },
    { "rtstart", "rtloop", 1, 0, 0, 0, { 0 } },
    { "Surrender", "", 1, 0, 0, 0, { 0 } },
    { "Defendall", "defendloop", 1, 0, 0, 0, { 0 } },
    { "lbstart", "lbloop", 1, 0, 0, 0, { 0 } },
    { "Tavern", "", 1, 0, 0, 0, { 0 } },
    { "cgood1", "", 1, 0, 0, 1, { 0 } },
    { "cgood2", "", 1, 0, 0, 1, { 0 } },
    { "cgood3", "", 1, 0, 0, 1, { 0 } },
    { "cneutral", "", 1, 0, 0, 1, { 0 } },
    { "cevil1", "", 1, 0, 0, 1, { 0 } },
    { "cevil2", "", 1, 0, 0, 1, { 0 } },
    { "csecret", "", 1, 0, 0, 1, { 0 } },
    { "C1ab7", "", 1, 0, 0, 1, { 0 } },
    { "C1db2", "", 1, 0, 0, 1, { 0 } },
    { "C1ds1", "", 1, 0, 0, 1, { 0 } },
    { "C1fl3", "", 1, 0, 0, 1, { 0 } },
    { "C1fw1", "", 1, 0, 0, 1, { 0 } },
    { "C1pf2", "", 1, 0, 0, 1, { 0 } },
    { "hack", "", 1, 0, 0, 1, { 0 } },
    { "birth", "", 1, 0, 0, 1, { 0 } },
    { "new", "", 1, 0, 0, 1, { 0 } },
    { "elixir", "", 1, 0, 0, 1, { 0 } },
    { "rise", "", 1, 0, 0, 1, { 0 } },
    { "unholy", "", 1, 0, 0, 1, { 0 } },
    { "spectre", "", 1, 0, 0, 1, { 0 } },
    { "Testing", "", 1, 0, 0, 0, { 0 } },
    { "3DOLogo", "", 0, 0, 1, 0, { 0 } },
    { "NWCLogo", "", 1, 0, 1, 0, { 0 } },
    { "H3Intro", "", 1, 0, 1, 0, { 0 } },
    { "H3x1intr", "", 1, 0, 1, 0, { 0 } },
    { "Endgame", "", 1, 0, 0, 0, { 0 } },
    { "Credits", "", 0, 0, 0, 0, { 0 } },
    { "LoseGame", "", 0, 0, 0, 0, { 0 } },
    { "hsAnim", "hsloop", 0, 0, 0, 0, { 0 } },
    { "PgTrnLft", "", 0, 0, 0, 0, { 0 } },
    { "PgTrnRgh", "", 0, 0, 0, 0, { 0 } },
    { "Good1A", "", 0, 0, 0, 0, { 0 } },
    { "Good1B", "", 0, 0, 0, 0, { 0 } },
    { "Good1C", "", 0, 0, 0, 0, { 0 } },
    { "NeutralA", "", 0, 0, 0, 0, { 0 } },
    { "NeutralB", "", 0, 0, 0, 0, { 0 } },
    { "NeutralC", "", 0, 0, 0, 0, { 0 } },
    { "NeutralC", "", 0, 0, 0, 0, { 0 } },
    { "Evil1A", "", 0, 0, 0, 0, { 0 } },
    { "Evil1B", "", 0, 0, 0, 0, { 0 } },
    { "Evil1C", "", 0, 0, 0, 0, { 0 } },
    { "Good2A", "", 0, 0, 0, 0, { 0 } },
    { "Good2B", "", 0, 0, 0, 0, { 0 } },
    { "Good2C", "", 0, 0, 0, 0, { 0 } },
    { "Good2D", "", 0, 0, 0, 0, { 0 } },
    { "Good3A", "", 0, 0, 0, 0, { 0 } },
    { "Good3B", "", 0, 0, 0, 0, { 0 } },
    { "Good3C", "", 0, 0, 0, 0, { 0 } },
    { "Evil2A", "", 0, 0, 0, 0, { 0 } },
    { "Evil2B", "", 0, 0, 0, 0, { 0 } },
    { "Evil2C", "", 0, 0, 0, 0, { 0 } },
    { "Evil2D", "", 0, 0, 0, 0, { 0 } },
    { "SecretA", "", 0, 0, 0, 0, { 0 } },
    { "SecretB", "", 0, 0, 0, 0, { 0 } },
    { "SecretC", "", 0, 0, 0, 0, { 0 } },
    { "Evil2ap1", "Evil2ap2", 0, 1, 1, 0, { 0 } },
    { "ProgressBar", "", 0, 0, 0, 1, { 0 } },
    { "h3abab1", "", 0, 0, 0, 0, { 0 } },
    { "h3abab2", "", 0, 0, 0, 0, { 0 } },
    { "h3abab3", "", 0, 0, 0, 0, { 0 } },
    { "h3abab4", "", 0, 0, 0, 0, { 0 } },
    { "h3abab5", "", 0, 0, 0, 0, { 0 } },
    { "h3abab6", "", 0, 0, 0, 0, { 0 } },
    { "h3abab7", "", 0, 0, 0, 0, { 0 } },
    { "h3abab8", "", 0, 0, 0, 0, { 0 } },
    { "h3abab9", "", 0, 0, 0, 0, { 0 } },
    { "h3abdb1", "", 0, 0, 0, 0, { 0 } },
    { "h3abdb2", "", 0, 0, 0, 0, { 0 } },
    { "h3abdb3", "", 0, 0, 0, 0, { 0 } },
    { "h3abdb4", "h3abdb4b", 0, 0, 0, 0, { 0 } },
    { "h3abdb5", "", 0, 0, 0, 0, { 0 } },
    { "h3abds1", "", 0, 0, 0, 0, { 0 } },
    { "h3abds2", "", 0, 0, 0, 0, { 0 } },
    { "h3abds3", "", 0, 0, 0, 0, { 0 } },
    { "h3abds4", "", 0, 0, 0, 0, { 0 } },
    { "h3abds5", "", 0, 0, 0, 0, { 0 } },
    { "h3abfl1", "", 0, 0, 0, 0, { 0 } },
    { "h3abfl2", "", 0, 0, 0, 0, { 0 } },
    { "h3abfl3", "", 0, 0, 0, 0, { 0 } },
    { "h3abfl4", "", 0, 0, 0, 0, { 0 } },
    { "h3abfl5", "", 0, 0, 0, 0, { 0 } },
    { "h3abfw1", "", 0, 0, 0, 0, { 0 } },
    { "h3abfw2", "", 0, 0, 0, 0, { 0 } },
    { "h3abfw3", "", 0, 0, 0, 0, { 0 } },
    { "h3abfw4", "", 0, 0, 0, 0, { 0 } },
    { "h3abfw5", "", 0, 0, 0, 0, { 0 } },
    { "h3abpf1", "", 0, 0, 0, 0, { 0 } },
    { "h3abpf2", "", 0, 0, 0, 0, { 0 } },
    { "h3abpf3", "", 0, 0, 0, 0, { 0 } },
    { "h3abpf4", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_HSa", "", 0, 0, 0, 0, { 0 } },
    { "Evil2C", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_HSc", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_HSd", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_HSe", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_BBa", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_BBb", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_BBc", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_BBd", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_BBe", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_BBf", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_NBa", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_NBb", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_NBc", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_NBd", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_NBe", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_ELa", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_ELb", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_ELc", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_ELd", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_ELe", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_RNa", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_RNb", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_RNc", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_RNd", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_RNe1", "H3x2_RNe2", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAa", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAb", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAc", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAd", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAe", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAf", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAg", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAh", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAi", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAj", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAk", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAl", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_UAm", "", 1, 0, 0, 0, { 0 } },
    { "H3x2_SPa", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_SPb", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_SPc", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_SPd", "", 0, 0, 0, 0, { 0 } },
    { "H3x2_SPe", "", 0, 0, 0, 0, { 0 } }
};

// Retail scalar state; startup initial values come from the pinned image.
DATA(0x006839b8) char g_archiveDriveLetter = 'd';

// The drive letter the misc.obj install scan settled on. It is WRITTEN at
// 0x50c278 inside that scan (still unclaimed) and READ only there and by
// GetDriveArchivePath below, which is why the definition is left with its
// owning TU: 0x6839b8's .data neighbours are singleselectionwindow's and
// spellbookwindow's blocks, not smackmgr's. Retail initialises it to 'd'.


// The Smack/Bink handle views and import declarations come from the SDK
// headers included by smackmgr.h / binkmanager.h.

void showVideo(int id, int x, int y, int w, int h, int a6, bool a7, bool a8);
namespace SmackManager {
void nextSmackerFrame();
void closeSmacker();
}

// smackmgr.obj .bss cluster 0x69fdf5..0x69fe5c. Raw DC file-static
// records prove Red/Green/BlueBits/Shift, bSmackSound, insideNextFrame and
// bSmackNum. The retail bindings follow their mask-counting, audio-gating,
// reentry and descriptor-index roles; all observed retail references stay
// inside SmackMgr. bSmackNum is unsigned char in DC and retail consumes only
// its low byte, though the specific Windows address remains role-correlated.
// Raw DC namespace globals updateScreen, needsUpdate and PlayingSmacker
// are bool. Their Windows addresses below are inferred from the matching
// auto-draw, pending-frame and active-playback roles, parallel to Bink;
// the CE stubs contain no data accesses that could prove these VA bindings.
DATA(0x0069fdf4) bool SmackManager::g_updateScreen;     // ShowVideo arg 7: pump its own VideoDrawRects
DATA(0x0069fdf5) bool SmackManager::g_needsUpdate;        // decoded frame awaits a blit
// Mac and Windows agree on this eleven-field playback record; see
// SmackManagerStruct in smackmgr.h.
DATA(0x0069fdf8) SmackManager::SmackManagerStruct SmackManager::g_playingSmack;
// DC bSmackNum. ShowVideo uses this descriptor row's audio-track flag; it is
// distinct from the video id latched above.
DATA(0x0069fdec) static unsigned char g_smackNum;
// DC bSmackSound. ShowVideo publishes whether Miles and user sound are live,
// then uses it to select the 0xfe000 Smacker track mask.
DATA(0x0069fe58) static int g_smackSound;
DATA(0x0069fde4) int g_videoCount1;
DATA(0x0069fe34) int g_videoCount2;
DATA(0x0069fe3c) int g_videoCount3;
DATA(0x0069fddc) static int g_redShift;
DATA(0x0069fdd8) static int g_redBits;
DATA(0x0069fe40) static int g_greenShift;
DATA(0x0069fe30) static int g_greenBits;
DATA(0x0069fde8) static int g_blueShift;
DATA(0x0069fe38) static int g_blueBits;
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
DATA(0x0069fe54) static int g_insideNextFrame;       // DC insideNextFrame: reentry latch
DATA(0x0069fe5c) bool SmackManager::g_playingSmacker;   // SmackDoFrame is allowed

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

// Retail has separate Smacker and Bink service tails at 0x5971e5/0x5971da.
// Preserve the codec branches and canonical serviceSounds calls: merging
// all four guards removes one tail here and in videoPause/videoResume.
// The WinCE counterpart is a four-byte stub and proves no Windows body.
VA(0x005971b0, 0x3B)
DC_ADDRESS(0x14ac30, 0x4)
MAC_ADDRESS(0x25e3f8, 0x6c)
void videoSoundOnOff(int on)
{
    if (SmackManager::g_playingSmack.m_smack || SmackManager::g_playingSmack.m_smack2)
        g_soundManager->serviceSounds();
    else if (BinkManager::g_playingBink.m_bink || BinkManager::g_playingBink.m_bink2)
        g_soundManager->serviceSounds();
}

VA(0x005971f0, 0xD9)
DC_ADDRESS(0x14ac34, 0x4)
MAC_ADDRESS(0x25e464, 0x104)
void videoRealignBuffers()
{
    SmackManager::g_playingSmack.m_bufferFlags = (g_greenBits == VIDEO_PIXEL_FORMAT_RGB565)
                            ? SMACKBUFFER565 : SMACKBUFFER555;
    if (SmackManager::g_playingSmack.m_smack)
        SmackToBuffer(SmackManager::g_playingSmack.m_smack, SmackManager::g_playingSmack.m_x, SmackManager::g_playingSmack.m_y,
            g_windowManager->m_screenBitmap->getPitch(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getMap(0, 0), SmackManager::g_playingSmack.m_bufferFlags);
    if (SmackManager::g_playingSmack.m_smack2)
        SmackToBuffer(SmackManager::g_playingSmack.m_smack2, SmackManager::g_playingSmack.m_x, SmackManager::g_playingSmack.m_y,
            g_windowManager->m_screenBitmap->getPitch(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getMap(0, 0), SmackManager::g_playingSmack.m_bufferFlags);
    BinkManager::g_surfaceType = BinkDDSurfaceType(g_ddsBack);
    BinkManager::g_playingBink.m_screen = g_windowManager->m_screenBitmap->getMap(
        BinkManager::g_playingBink.m_x, BinkManager::g_playingBink.m_y);
    BinkManager::g_playingBink.m_pitch = g_windowManager->m_screenBitmap->getPitch();
    BinkManager::g_playingBink.m_height = g_windowManager->m_screenBitmap->getHeight();
}

VA(0x005972d0, 0x29D)
DC_ADDRESS(0x14ac38, 0x4)
MAC_ADDRESS(0x25e568, 0x7c)
int videoPlay(int id, int x, int y, int w, int h)
{
    if (id >= VIDEO_ID_FIRST_TABLED
        && (!g_videoDescriptors[id].m_useBink || !g_config.m_binkVideo
            || (id == VIDEO_ID_STATE_GATED
                && g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_LOW
                && g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {
        return SmackManager::playSmacker(id, x, y, w, h);
    }
    return BinkManager::playBink(id, x, y, w, h);
}

VA(0x00597570, 0x75)
DC_ADDRESS(0x14ac3c, 0x4)
MAC_ADDRESS(0x25e5e4, 0x7c)
void videoOpen(int id, int x, int y, int w, int h, int a6, bool a7, bool a8)
{
    if (id >= VIDEO_ID_FIRST_TABLED
        && (!g_videoDescriptors[id].m_useBink || !g_config.m_binkVideo
            || (id == VIDEO_ID_STATE_GATED
                && g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_LOW
                && g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH)))
        showVideo(id, x, y, w, h, a6, a7, a8);
    else
        BinkManager::openBink(id, x, y, w, h, a6, a7);
}

// Retail's 225-byte body follows this canonical helper chain:
VA(0x005975f0, 0xE1)
DC_ADDRESS(0x14ac40, 0x4)
MAC_ADDRESS(0x25e660, 0x50)
void videoClose()
{
    while (g_videoPauseCount != 0)
        videoResume();
    g_soundManager->serviceSounds();
    SmackManager::closeSmacker();
    BinkManager::closeBink();
}

VA(0x005976e0, 0x5E)
DC_ADDRESS(0x14ac44, 0x4)
MAC_ADDRESS(0x25e6b0, 0x98)
void videoNextFrame()
{
    if (g_insideNextFrame)
        return;
    g_insideNextFrame = 1;
    if (SmackManager::g_playingSmack.m_smack || SmackManager::g_playingSmack.m_smack2) {
        if (!SmackManager::g_playingSmack.m_paused)
            SmackManager::nextSmackerFrame();
    }
    if (BinkManager::g_playingBink.m_bink || BinkManager::g_playingBink.m_bink2) {
        if (!BinkManager::g_playingBink.m_paused)
            BinkManager::nextBinkFrame();
    }
    g_insideNextFrame = 0;
}

// Mac retains this call from videoDrawCurrentFrame at 0:0x25e788 and updates
// the screen from this body at 0:0x25f9dc. Windows retail 0x598e80 only
// calls SmackDoFrame; videoDrawRects owns the screen updates on this port.
// Keep the helper visible before the caller so VC6 can expand it.
VA(0x00598e80, 0x25)
MAC_ADDRESS(0x25f948, 0xc8)
void SmackManager::drawSmackerFrame()
{
    if (SmackManager::g_playingSmack.m_smack && SmackManager::g_playingSmacker && !SmackManager::g_playingSmack.m_paused)
        SmackDoFrame(SmackManager::g_playingSmack.m_smack);
}

VA(0x00597740, 0x53)
DC_ADDRESS(0x14ac48, 0x4)
MAC_ADDRESS(0x25e748, 0x80)
void videoDrawCurrentFrame()
{
    if (SmackManager::g_playingSmack.m_smack || SmackManager::g_playingSmack.m_smack2) {
        if (!SmackManager::g_playingSmack.m_paused)
            SmackManager::drawSmackerFrame();
    }
    if (BinkManager::g_playingBink.m_bink || BinkManager::g_playingBink.m_bink2) {
        if (!BinkManager::g_playingBink.m_paused)
            BinkManager::drawCurrentBinkFrame();
    }
}

VA(0x005977a0, 0xA6)
DC_ADDRESS(0x14ac4c, 0x4)
MAC_ADDRESS(0x25e7c8, 0xbc)
void videoPause()
{
    if (++g_videoPauseCount > 1)
        return;
    if (SmackManager::g_playingSmack.m_smack || SmackManager::g_playingSmack.m_smack2)
        SmackManager::g_playingSmack.m_paused = 1;
    if (BinkManager::g_playingBink.m_bink) {
        BinkManager::setTrackPaused(BinkManager::g_playingBink.m_bink, 1);
    }
    if (BinkManager::g_playingBink.m_bink2) {
        BinkManager::setTrackPaused(BinkManager::g_playingBink.m_bink2, 1);
    }
    videoSoundOnOff(0);
}

VA(0x00597850, 0xAB)
DC_ADDRESS(0x14ac50, 0x4)
MAC_ADDRESS(0x25e884, 0xc4)
// The implicit zero tests are byte-identical to `== 0`/`!= 0` here but cost
// 4 fewer /Ob2 IL units; showVideo's third videoClose expansion only admits
// this body at that cost. Splitting the guard into two returns costs more.
void videoResume()
{
    if (!g_videoPauseCount || --g_videoPauseCount)
        return;
    if (SmackManager::g_playingSmack.m_smack || SmackManager::g_playingSmack.m_smack2)
        SmackManager::g_playingSmack.m_paused = 0;
    if (BinkManager::g_playingBink.m_bink) {
        BinkManager::setTrackPaused(BinkManager::g_playingBink.m_bink, 0);
    }
    if (BinkManager::g_playingBink.m_bink2) {
        BinkManager::setTrackPaused(BinkManager::g_playingBink.m_bink2, 0);
    }
    videoSoundOnOff(1);
}

VA(0x00597900, 0x23)
DC_ADDRESS(0x14ac54, 0x4)
MAC_ADDRESS(0x25e948, 0x24)
void videoRestart()
{
    SmackManager::restartSmacker();
    BinkManager::restartBink();
}

VA(0x00597930, 0x5A)
DC_ADDRESS(0x14ac58, 0x4)
MAC_ADDRESS(0x25e96c, 0x90)
bool videoNeedsUpdate()
{
    if (SmackManager::g_playingSmack.m_smack || SmackManager::g_playingSmack.m_smack2)
        return SmackManager::g_needsUpdate && !SmackManager::g_playingSmack.m_paused;
    else if (BinkManager::g_playingBink.m_bink || BinkManager::g_playingBink.m_bink2)
        return BinkManager::g_needsUpdate && !BinkManager::g_playingBink.m_paused;
    return 0;
}

VA(0x00597990, 0x3F)
DC_ADDRESS(0x14ac5c, 0x4)
MAC_ADDRESS(0x25e9fc, 0x68)
bool videoPlaying()
{
    if ((SmackManager::g_playingSmack.m_smack || SmackManager::g_playingSmack.m_smack2) && !SmackManager::g_playingSmack.m_paused)
        return 1;
    if ((BinkManager::g_playingBink.m_bink || BinkManager::g_playingBink.m_bink2) && !BinkManager::g_playingBink.m_paused)
        return 1;
    return 0;
}

// E:\gamedcs\smackmgr.cpp:328
// Mac 0x25ea64 takes an update flag and presents individual dirty rectangles
// from its indexed conversion buffer. Windows decodes directly into the
// back surface and has the nullary, merged-rectangle path below.
// Merges the pending dirty rects into one union rect and pushes it to
// the screen; the id-0x1d bink instead Blts the whole 800x600 back
// surface to the primary around an Unlock/Lock pair.
// Residual (90.3379%): the SDK BinkRect bounds object and field references
// remain; POINT, source RECT and DDSURFACEDESC belong to function scope,
// while the destination RECT stays in the overlay branch. This natural
// declaration-scope candidate improves the previous 89.9772% reconstruction.
// DC retains a four-byte platform stub; it proves no body/local scopes.
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
// Naming both incoming endpoints before the comparisons adds alternate
// coordinate-reload branches, spills Bink x/y and advances ESI instead of
// retail's ECX rectangle cursor. Both aggregate/scalar models failed the
// predicted register and frame recovery; retain the ordered expressions.
// Separate arm-local signed scalars likewise preserve the CFG/calls but fall
// to 89.6347%, so the shared SDK rectangle remains the strongest model.
// Mac 0x25ea64 additionally consumes an incoming redraw flag and uses
// native display-point/blit helpers. Its platform body cannot determine
// the Windows Smack/Bink union-loop locals or DirectDraw frame.
// Naming Unlock's HRESULT at branch/function scope is likewise byte-flat
// or worse; it does not recover the missing union-loop register homes.
VA(0x005979d0, 0x294)
DC_ADDRESS(0x14ac60, 0x4)
MAC_ADDRESS(0x25ea64, 0x26c)  // anchor-global
void videoDrawRects()
{
    POINT pt;
    RECT src;
    DDSURFACEDESC ddsd;

    BINKRECT bounds;
    long& x = bounds.Left;
    long& y = bounds.Top;
    long& w = bounds.Width;
    long& h = bounds.Height;

    if ((SmackManager::g_playingSmack.m_smack || SmackManager::g_playingSmack.m_smack2) && !SmackManager::g_playingSmack.m_paused) {
        Smack* smk;

        smk = 0;
        if (SmackManager::g_playingSmack.m_smack)
            smk = SmackManager::g_playingSmack.m_smack;
        else if (SmackManager::g_playingSmack.m_smack2)
            smk = SmackManager::g_playingSmack.m_smack2;
        SmackToBufferRect(smk, SmackManager::g_playingSmack.m_bufferFlags);
        w = smk->LastRectw;
        h = smk->LastRecth;
        x = smk->LastRectx;
        y = smk->LastRecty;
        while (SmackToBufferRect(smk, SmackManager::g_playingSmack.m_bufferFlags)) {
            if (smk->LastRectx < x)
                x = smk->LastRectx;
            if (smk->LastRecty < y)
                y = smk->LastRecty;
            if (smk->LastRectw + smk->LastRectx > w + x)
                w = smk->LastRectw + smk->LastRectx - x;
            if (smk->LastRecth + smk->LastRecty > h + y)
                h = smk->LastRecth + smk->LastRecty - y;
        }
        g_windowManager->updateScreen(x, y, w, h);
    } else if ((BinkManager::g_playingBink.m_bink || BinkManager::g_playingBink.m_bink2) && !BinkManager::g_playingBink.m_paused) {
        HBINK bnk;

        bnk = BinkManager::g_playingBink.m_bink2;
        if (BinkManager::g_playingBink.m_bink)
            bnk = BinkManager::g_playingBink.m_bink;
        if (BinkManager::g_playingBink.m_id != VIDEO_ID_OVERLAY_BLIT) {
            int i;

            BinkGetRects(bnk, BinkManager::g_surfaceType);
            w = bnk->FrameRects[0].Width;
            h = bnk->FrameRects[0].Height;
            x = bnk->FrameRects[0].Left;
            y = bnk->FrameRects[0].Top;
            for (i = 1; i < bnk->NumRects; i++) {
                if (bnk->FrameRects[i].Left < x)
                    x = bnk->FrameRects[i].Left;
                if (bnk->FrameRects[i].Top < y)
                    y = bnk->FrameRects[i].Top;
                if (bnk->FrameRects[i].Width + bnk->FrameRects[i].Left > w + x)
                    w = bnk->FrameRects[i].Width + bnk->FrameRects[i].Left - x;
                if (bnk->FrameRects[i].Height + bnk->FrameRects[i].Top > h + y)
                    h = bnk->FrameRects[i].Height + bnk->FrameRects[i].Top - y;
            }
            g_windowManager->updateScreen(BinkManager::g_playingBink.m_x + x, BinkManager::g_playingBink.m_y + y, w, h);
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
            src.right = bnk->Width;
            src.bottom = bnk->Height;
            if (g_ddsBack->Unlock(NULL) != 0)
                return;
            g_ddsPrimary->Blt(&dst, g_ddsBack, &src, DDBLT_WAIT, NULL);
            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            g_ddsBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
        }
    }
    BinkManager::g_needsUpdate = 0;
    SmackManager::g_needsUpdate = 0;
}

VA(0x00597c70, 0x84)
DC_ADDRESS(0x14ac64, 0x4)
MAC_ADDRESS(0x25ed20, 0x4c)
void videoShutDown()
{
    SmackManager::closeSmacker();
    BinkManager::closeBink();
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

// Project-inferred archive directory operations. Keep references to the
// published handle/count/pointer cells and the caller's read-count storage.
template <class Header>
static void readArchiveHeaders(HANDLE& file, int& count, Header*& headers,
                               DWORD& bytesRead)
{
    ReadFile(file, &count, 4, &bytesRead, 0);
    headers = new Header[count + 2];
    ReadFile(file, headers, static_cast<int>(sizeof(Header)) * count,
             &bytesRead, 0);
}

template <class Header>
static void clearArchiveHeaders(Header*& headers)
{
    if (headers) {
        delete[] headers;
        headers = 0;
    }
}

static void closeSoundArchive(HANDLE& file, SoundHeaderStruct*& headers)
{
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
        file = INVALID_HANDLE_VALUE;
    }
    clearArchiveHeaders(headers);
}

VA(0x00598210, 0x223)
DC_ADDRESS(0x14ac68, 0x4)
MAC_ADDRESS(0x25ed6c, 0x2c4)
unsigned char loadAnimHeaders()
{
    DWORD nread;

    g_videoFile3 = CreateFileA(getDriveArchivePath().c_str(), GENERIC_READ,
        FILE_SHARE_READ, 0, OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
    if (g_videoFile3 != INVALID_HANDLE_VALUE) {
        readArchiveHeaders(g_videoFile3, g_videoCount3, g_videoHeader3, nread);
    } else {
        g_videoFile3 = 0;
    }

    if (g_videoGameState == VIDEO_GAME_STATE_EXPANSION_ARCHIVES
        || g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        g_videoFile1 = CreateFileA("data\\h3ab_ahd.vid", GENERIC_READ,
            FILE_SHARE_READ, 0, OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
        if (g_videoFile1 == INVALID_HANDLE_VALUE) {
            MessageBoxA(g_hwndApp, g_generalText->getText(GENERAL_TEXT_VIDEO_FILE_OPEN_ERROR),
                g_generalText->getText(GENERAL_TEXT_VIDEO_FILE_ERROR), 0);
            return 0;
        }
        readArchiveHeaders(g_videoFile1, g_videoCount1, g_videoHeader1, nread);
    }

    g_videoFile2 = CreateFileA("data\\Video.vid", GENERIC_READ, FILE_SHARE_READ,
        0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
    if (g_videoFile2 == INVALID_HANDLE_VALUE) {
        MessageBoxA(g_hwndApp, g_generalText->getText(GENERAL_TEXT_VIDEO_FILE_OPEN_ERROR),
            g_generalText->getText(GENERAL_TEXT_VIDEO_FILE_ERROR), 0);
        return 0;
    }
    readArchiveHeaders(g_videoFile2, g_videoCount2, g_videoHeader2, nread);
    return 1;
}

VA(0x00598440, 0x55)
DC_ADDRESS(0x14ac6c, 0x32)
MAC_ADDRESS(0x25f030, 0x88)
void deleteAnimHeaders()
{
    clearArchiveHeaders(g_videoHeader3);
    clearArchiveHeaders(g_videoHeader2);
    clearArchiveHeaders(g_videoHeader1);
}

VA(0x005984a0, 0x240)
DC_ADDRESS(0x14aca0, 0x26)
MAC_ADDRESS(0x25f0b8, 0x31c)
unsigned char loadSoundHeaders()
{
    DWORD nread;
    char path[52];

    g_soundFile = CreateFileA("data\\heroes3.snd", GENERIC_READ, FILE_SHARE_READ,
        0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
    if (g_soundFile == INVALID_HANDLE_VALUE) {
        MessageBoxA(g_hwndApp, g_generalText->getText(GENERAL_TEXT_SOUND_ARCHIVE_OPEN_ERROR),
            g_generalText->getText(GENERAL_TEXT_SOUND_FILE_ERROR), 0);
        return 0;
    }
    readArchiveHeaders(g_soundFile, g_soundCount,
                       g_soundHeader, nread);

    if (g_videoGameState == VIDEO_GAME_STATE_EXPANSION_ARCHIVES
        || g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        g_soundFileCd = CreateFileA("data\\h3ab_ahd.snd", GENERIC_READ,
            FILE_SHARE_READ, 0, OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
        if (g_soundFileCd == INVALID_HANDLE_VALUE) {
            MessageBoxA(g_hwndApp, g_generalText->getText(GENERAL_TEXT_SOUND_ARCHIVE_OPEN_ERROR),
                g_generalText->getText(GENERAL_TEXT_SOUND_FILE_ERROR), 0);
            return 0;
        }
        readArchiveHeaders(g_soundFileCd, g_soundCountCd,
                           g_soundHeaderCd, nread);
    }

    strcpy(path, getDriveArchivePath().c_str());
    strtok(path, ".");
    strcat(path, ".snd");
    g_soundFileCampaign = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);
    if (g_soundFileCampaign != INVALID_HANDLE_VALUE) {
        readArchiveHeaders(g_soundFileCampaign, g_soundCountCampaign,
                           g_soundHeaderCampaign, nread);
    }
    return 1;
}

VA(0x005986e0, 0xA2)
DC_ADDRESS(0x14acc8, 0x5a)
MAC_ADDRESS(0x25f3d4, 0xa0)
void deleteSoundHeaders()
{
    closeSoundArchive(g_soundFile, g_soundHeader);
    closeSoundArchive(g_soundFileCd, g_soundHeaderCd);
    closeSoundArchive(g_soundFileCampaign, g_soundHeaderCampaign);
}

VA(0x00598790, 0x2AB)
MAC_ADDRESS(0x25f474, 0x270)
Smack* openSmackerTrack(const char* stem, unsigned long flags,
                        unsigned long extraFlags)
{
    char name[40];
    int i;

    strcpy(name, stem);
    strcat(name, ".smk");

    if (g_videoGameState == VIDEO_GAME_STATE_EXPANSION_ARCHIVES) {
        for (i = 0; i < g_videoCount1; i++) {
            if (_strcmpi(g_videoHeader1[i].m_name, name) == 0) {
                g_soundManager->serviceSounds();
                SetFilePointer(g_videoFile1, g_videoHeader1[i].m_offset, 0,
                    FILE_BEGIN);
                return SmackOpen(static_cast<const char*>(g_videoFile1),
                    flags | extraFlags | SMACKFILEHANDLE | SMACKLOADEXTRA
                        | SMACKNEEDVOLUME, SMACKAUTOEXTRA);
            }
        }
    }

    for (i = 0; i < g_videoCount2; i++) {
        if (_strcmpi(g_videoHeader2[i].m_name, name) == 0) {
            g_soundManager->serviceSounds();
            SetFilePointer(g_videoFile2, g_videoHeader2[i].m_offset, 0, FILE_BEGIN);
            return SmackOpen(static_cast<const char*>(g_videoFile2),
                flags | extraFlags | SMACKFILEHANDLE | SMACKLOADEXTRA
                    | SMACKNEEDVOLUME, SMACKAUTOEXTRA);
        }
    }

    if (g_videoFile3) {
        for (i = 0; i < g_videoCount3; i++) {
            if (_strcmpi(g_videoHeader3[i].m_name, name) == 0) {
                g_soundManager->serviceSounds();
                SetFilePointer(g_videoFile3, g_videoHeader3[i].m_offset, 0,
                    FILE_BEGIN);
                return SmackOpen(static_cast<const char*>(g_videoFile3),
                    flags | extraFlags | SMACKFILEHANDLE | SMACKLOADEXTRA
                        | SMACKNEEDVOLUME, SMACKAUTOEXTRA);
            }
        }
    }

    if (g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        for (i = 0; i < g_videoCount1; i++) {
            if (_strcmpi(g_videoHeader1[i].m_name, name) == 0) {
                g_soundManager->serviceSounds();
                SetFilePointer(g_videoFile1, g_videoHeader1[i].m_offset, 0,
                    FILE_BEGIN);
                return SmackOpen(static_cast<const char*>(g_videoFile1),
                    flags | extraFlags | SMACKFILEHANDLE | SMACKLOADEXTRA
                        | SMACKNEEDVOLUME, SMACKAUTOEXTRA);
            }
        }
    }
    return 0;
}

VA(0x00598a40, 0xA2)
DC_ADDRESS(0x14ad24, 0xd8)
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
MAC_ADDRESS(0x25f6e4, 0x264)
// VideoOpen's DC-proven Boolean flags pass unchanged into this Windows
// opener. Retail reads autoDraw as a byte and widens advance's low byte.
// Retail refuses videoResume at the first two videoClose expansions and
// accepts it at the third, then calls closeSmacker and videoSoundOnOff.
// That split needs both the SmackManagerStruct member accesses here (caller
// IL) and videoResume's implicit pause-count tests (callee IL, 112 against a
// nested budget of 114). With them the two plain descriptor subscripts give
// retail's id*20 index; the former const-reference row (76.24%) put a
// pointer on the frame instead.
void showVideo(int id, int x, int y, int w, int h, int loop, bool autoDraw,
               bool advance)
{
    if (g_noSound == 0 && g_soundManager->m_ds != 0
        && g_config.m_soundVolume != 0)
        g_smackSound = 1;
    else
        g_smackSound = 0;

    videoClose();
    SmackUseMMX(1);

    unsigned long trackMask = g_smackSound ? SMACKTRACKS : 0;
    unsigned long videoMode =
        g_videoDescriptors[g_smackNum].m_noFrameSkip
            ? SMACKPRELOADALL : 0;

    SmackManager::g_playingSmack.m_id = id;
    SmackManager::g_playingSmack.m_paused = 0;
    SmackManager::g_playingSmack.m_smack2 = 0;
    SmackManager::g_playingSmack.m_bufferFlags = (g_greenBits == VIDEO_PIXEL_FORMAT_RGB565)
                            ? SMACKBUFFER565 : SMACKBUFFER555;
    SmackManager::g_playingSmack.m_advance = advance;

    if (g_videoDescriptors[id].m_smkAudioStem != "") {
        SmackManager::g_playingSmack.m_smack2 = openSmackerTrack(g_videoDescriptors[id].m_smkAudioStem,
            trackMask, SMACKPRELOADALL);
        if (!SmackManager::g_playingSmack.m_smack2) {
            videoClose();
            return;
        }
        SmackVolumePan(SmackManager::g_playingSmack.m_smack2, SMACKTRACKS,
            3640 * g_config.m_soundVolume, 0x8000);
        SmackToBuffer(SmackManager::g_playingSmack.m_smack2, x, y,
            g_windowManager->m_screenBitmap->getPitch(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getMap(0, 0), SmackManager::g_playingSmack.m_bufferFlags);
    }

    SmackManager::g_playingSmack.m_smack = openSmackerTrack(g_videoDescriptors[id].m_smkStem,
        trackMask, videoMode);
    if (!SmackManager::g_playingSmack.m_smack) {
        videoClose();
        return;
    }

    SmackManager::g_updateScreen = autoDraw;
    if (w <= 0)
        w = SmackManager::g_playingSmack.m_smack->Width;
    if (h <= 0)
        h = SmackManager::g_playingSmack.m_smack->Height;
    SmackManager::g_playingSmack.m_w = w;
    SmackManager::g_playingSmack.m_h = h;
    SmackManager::g_playingSmack.m_loop = loop;
    SmackManager::g_playingSmack.m_x = x;
    SmackManager::g_playingSmack.m_y = y;
    SmackVolumePan(SmackManager::g_playingSmack.m_smack, SMACKTRACKS,
        3640 * g_config.m_soundVolume, 0x8000);
    SmackToBuffer(SmackManager::g_playingSmack.m_smack, x, y,
        g_windowManager->m_screenBitmap->getPitch(),
        g_windowManager->m_screenBitmap->getHeight(),
        g_windowManager->m_screenBitmap->getMap(0, 0), SmackManager::g_playingSmack.m_bufferFlags);
    if (SmackManager::g_playingSmack.m_smack2)
        SmackToBuffer(SmackManager::g_playingSmack.m_smack2, x, y,
            g_windowManager->m_screenBitmap->getPitch(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getMap(0, 0), SmackManager::g_playingSmack.m_bufferFlags);
    SmackManager::g_playingSmacker = 1;
}

// SmackManager is a namespace in DC's raw publics (YA functions, 3 globals).
// Retail VideoDrawCurrentFrame (0x597740) and VideoClose (0x5975f0)
// expand the draw/close helpers; those bytes prove their Windows guard
// and store order. The CE CloseSmacker body itself is only rts/nop.
namespace SmackManager {

// Mac 0:0x25fa10 retains this helper immediately before nextSmackerFrame.
// videoRestart calls it at 0:0x25e954; Windows expands the same guarded pair.
MAC_ADDRESS(0x25fa10, 0x44)
void restartSmacker()
{
    if (SmackManager::g_playingSmack.m_smack) {
        SmackGoto(SmackManager::g_playingSmack.m_smack, 1);
        SmackDoFrame(SmackManager::g_playingSmack.m_smack);
    }
}

VA(0x00598eb0, 0x193)
DC_ADDRESS(0x14adfc, 0x4)
MAC_ADDRESS(0x25fa54, 0x224)
void nextSmackerFrame()
{
    Smack* smk = SmackManager::g_playingSmack.m_smack;
    if (!smk)
        smk = SmackManager::g_playingSmack.m_smack2;
    SmackManager::g_needsUpdate = smk && SmackManager::g_playingSmacker && !SmackWait(smk) && SmackManager::g_playingSmack.m_advance;
    if (!SmackManager::g_needsUpdate)
        return;
    if (SmackManager::g_playingSmack.m_paused)
        return;
    SmackDoFrame(smk);
    if (smk->FrameNum < smk->Frames - 1) {
        SmackNextFrame(smk);
    } else if (SmackManager::g_playingSmack.m_loop) {
        if (SmackManager::g_playingSmack.m_smack && SmackManager::g_playingSmack.m_smack2) {
            if (g_videoDescriptors[SmackManager::g_playingSmack.m_id].m_fadeOnAbort)
                g_windowManager->fadeScreen(1, 4, 0);
            g_soundManager->serviceSounds();
            SmackClose(SmackManager::g_playingSmack.m_smack);
            SmackManager::g_playingSmack.m_smack = 0;
            if (g_videoDescriptors[SmackManager::g_playingSmack.m_id].m_fadeInSecondTrack) {
                SmackDoFrame(SmackManager::g_playingSmack.m_smack2);
                g_windowManager->fadeScreen(0, 4, 0);
            }
        } else {
            SmackNextFrame(smk);
        }
    } else {
        closeSmacker();
        if (g_videoDescriptors[SmackManager::g_playingSmack.m_id].m_fadeOnAbort)
            g_windowManager->fadeScreen(1, 4, 0);
        else
            g_windowManager->updateScreen(0, 0, 0x320, 0x258);
        return;
    }
    if (SmackManager::g_updateScreen)
        videoDrawRects();
}

VA(0x00599050, 0x43)
DC_ADDRESS(0x14ae00, 0x4c)
MAC_ADDRESS(0x25fc78, 0x74)
void closeSmacker()
{
    if (SmackManager::g_playingSmack.m_smack)
        SmackClose(SmackManager::g_playingSmack.m_smack);
    if (SmackManager::g_playingSmack.m_smack2)
        SmackClose(SmackManager::g_playingSmack.m_smack2);
    SmackManager::g_playingSmack.m_smack2 = 0;
    SmackManager::g_playingSmack.m_smack = 0;
    SmackManager::g_playingSmack.m_paused = 0;
    SmackManager::g_playingSmacker = 0;
    SmackManager::g_needsUpdate = 0;
}

// Mac separates playback setup/state cleanup from its modal loop.
static unsigned char playSmackerCore(int id, int x, int y, int w, int h);

MAC_ADDRESS(0x25fcec, 0x54)
int playSmacker(int id, int x, int y, int w, int h)
{
    g_soundManager->setPlaybackState(1);
    unsigned char result = playSmackerCore(id, x, y, w, h);
    SmackManager::g_playingSmack.m_paused = 0;
    g_playingSmacker = 0;
    return result;
}

VA(0x005990a0, 0x1C)
MAC_ADDRESS(0x25fd40, 0x44)
void gotoSmackerFrame(unsigned long frame)
{
    if (SmackManager::g_playingSmack.m_smack && SmackManager::g_playingSmacker)
        SmackGoto(SmackManager::g_playingSmack.m_smack, frame);
}

// Mac 0:0x25fd84 retains this body; Windows expands it through playSmacker.
MAC_ADDRESS(0x25fd84, 0x24c)
static unsigned char playSmackerCore(int id, int x, int y, int w, int h)
{
    POINT pos;
    int vw, vh;
    unsigned char result;
    unsigned char aborted;
    vh = h;
    vw = w;
    showVideo(id, x, y, vw, vh, 0, 0, 1);
    if (!SmackManager::g_playingSmack.m_smack) {
        result = 0;
    } else {
        g_mouseManager->hidePointer();
        if (vw < 0)
            vw = SmackManager::g_playingSmack.m_smack->Width;
        if (vh < 0)
            vh = SmackManager::g_playingSmack.m_smack->Height;
        SmackManager::g_playingSmack.m_x = x + (vw - SmackManager::g_playingSmack.m_smack->Width) / 2;
        SmackManager::g_playingSmack.m_y = y + (vh - SmackManager::g_playingSmack.m_smack->Height) / 2;
        pos.x = SmackManager::g_playingSmack.m_x;
        pos.y = SmackManager::g_playingSmack.m_y;
        SmackToBuffer(SmackManager::g_playingSmack.m_smack, SmackManager::g_playingSmack.m_x, SmackManager::g_playingSmack.m_y,
            g_windowManager->m_screenBitmap->getPitch(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getMap(0, 0), SmackManager::g_playingSmack.m_bufferFlags);
        aborted = 0;
        g_inputManager->flush();
        while (1) {
            if (SmackManager::g_playingSmack.m_smack == 0)
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
                        if (!g_firstTimeThrough) {
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
    return result;
}

}  // namespace SmackManager

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
