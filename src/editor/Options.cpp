// Options.cpp - the editor's saved settings (h3maped 0x492cba..0x4930c8,
// name inferred). An out-of-range zoom resets to full size, an autosave
// period over an hour turns autosave off, and a special tile frequency
// over 8 returns to its default 4.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Options.h"
#include "editor/Tile.h"

// This object's copies of terrain.h's ten masks open its code: each
// initializer (behind its five-byte jump) builds bitset<10>(1) << K.
VA_COMPGEN(0x00492cdb, 0x27, STATIC_CTOR, g_dirtMask)
VA_COMPGEN(0x00492d07, 0x27, STATIC_CTOR, g_sandMask)
VA_COMPGEN(0x00492d33, 0x27, STATIC_CTOR, g_grassMask)
VA_COMPGEN(0x00492d5f, 0x27, STATIC_CTOR, g_snowMask)
VA_COMPGEN(0x00492d8b, 0x27, STATIC_CTOR, g_swampMask)
VA_COMPGEN(0x00492db7, 0x27, STATIC_CTOR, g_roughMask)
VA_COMPGEN(0x00492de3, 0x27, STATIC_CTOR, g_subterraneanMask)
VA_COMPGEN(0x00492e0f, 0x27, STATIC_CTOR, g_lavaMask)
VA_COMPGEN(0x00492e3b, 0x27, STATIC_CTOR, g_waterMask)
VA_COMPGEN(0x00492e67, 0x27, STATIC_CTOR, g_rockMask)

// Address metadata for this object's .bss copies of terrain.h's masks. VC6
// must see only the header's definitions (an extern redeclaration makes it
// emit a second initializer), so only Clang's label pass reads these.
#ifdef __clang__
DATA(0x005a23fc) extern std::bitset<10> g_dirtMask;
DATA(0x005a23f8) extern std::bitset<10> g_sandMask;
DATA(0x005a23f4) extern std::bitset<10> g_grassMask;
DATA(0x005a23f0) extern std::bitset<10> g_snowMask;
DATA(0x005a23ec) extern std::bitset<10> g_swampMask;
DATA(0x005a23e8) extern std::bitset<10> g_roughMask;
DATA(0x005a23e4) extern std::bitset<10> g_subterraneanMask;
DATA(0x005a23e0) extern std::bitset<10> g_lavaMask;
DATA(0x005a23dc) extern std::bitset<10> g_waterMask;
DATA(0x005a23d8) extern std::bitset<10> g_rockMask;
#endif

// The registry names sit in this object's .rdata (0x53fb3c..0x53fb94, in
// this order and 4-aligned), while the map editor's string literals sit in
// .data: they are named constant arrays that load and save share.
namespace {
DATA(0x0053fb3c) const char kSection[] = "Settings";
DATA(0x0053fb48) const char kZoomEntry[] = "Zoom";
DATA(0x0053fb50) const char kGridEntry[] = "Grid";
DATA(0x0053fb58) const char kPassabilityEntry[] = "Passability";
DATA(0x0053fb64) const char kAnimationEntry[] = "Animation";
DATA(0x0053fb70) const char kCyclingEntry[] = "Cycling";
DATA(0x0053fb78) const char kAutosaveEntry[] = "Autosave";
DATA(0x0053fb84) const char kSpecialTileFrequencyEntry[] = "SpecialTileFreq";

DATA(0x005a2400) int zoomImp;
DATA(0x005a2404) bool bGridImp;
DATA(0x005a2405) bool bPassabilityImp;
DATA(0x005a2406) bool bAnimationImp;
DATA(0x005a2407) bool bCyclingImp;
DATA(0x005a2408) unsigned int autosaveMinutesImp;
DATA(0x0058d2a8) unsigned int specialTileFrequencyImp = 4;
}

namespace SOptions {
DATA(0x005a23d4) const int& kZoom = zoomImp;
DATA(0x005a23d0) const bool& kGrid = bGridImp;
DATA(0x005a23cc) const bool& kPassability = bPassabilityImp;
DATA(0x005a23c8) const bool& kAnimation = bAnimationImp;
DATA(0x005a23c4) const bool& kCycling = bCyclingImp;
DATA(0x005a23c0) const unsigned int& kAutosaveMinutes = autosaveMinutesImp;
DATA(0x005a23bc) const unsigned int& kSpecialTileFrequency = specialTileFrequencyImp;

VA(0x00492edb, 0x101)
void load()
{
    CWinApp* pApp = AfxGetApp();
    zoomImp = pApp->GetProfileInt(kSection, kZoomEntry, zoomImp);
    if (zoomImp < 0 || zoomImp >= kNumZooms)
        zoomImp = eZoom100;
    bGridImp = pApp->GetProfileInt(kSection, kGridEntry, bGridImp) != 0;
    bPassabilityImp = pApp->GetProfileInt(kSection, kPassabilityEntry, bPassabilityImp) != 0;
    bAnimationImp = pApp->GetProfileInt(kSection, kAnimationEntry, bAnimationImp) != 0;
    bCyclingImp = pApp->GetProfileInt(kSection, kCyclingEntry, bCyclingImp) != 0;
    autosaveMinutesImp = pApp->GetProfileInt(kSection, kAutosaveEntry, autosaveMinutesImp);
    if (autosaveMinutesImp > kMaxAutosaveMinutes)
        autosaveMinutesImp = 0;
    specialTileFrequencyImp = pApp->GetProfileInt(kSection, kSpecialTileFrequencyEntry, specialTileFrequencyImp);
    if (specialTileFrequencyImp > 8)
        specialTileFrequencyImp = 4;
}

VA(0x00492fdc, 0xa)
void setZoom(int zoom)
{
    zoomImp = zoom;
}

VA(0x00492fe6, 0xa)
void setGrid(bool bGrid)
{
    bGridImp = bGrid;
}

VA(0x00492ff0, 0xa)
void setPassability(bool bPassability)
{
    bPassabilityImp = bPassability;
}

VA(0x00492ffa, 0xa)
void setAnimation(bool bAnimation)
{
    bAnimationImp = bAnimation;
}

VA(0x00493004, 0xa)
void setCycling(bool bCycling)
{
    bCyclingImp = bCycling;
}

VA(0x0049300e, 0xa)
void setAutosaveMinutes(unsigned int minutes)
{
    autosaveMinutesImp = minutes;
}

VA(0x00493018, 0xa)
void setSpecialTileFrequency(unsigned int frequency)
{
    specialTileFrequencyImp = frequency;
}

VA(0x00493022, 0xa6)
void save()
{
    CWinApp* pApp = AfxGetApp();
    pApp->WriteProfileInt(kSection, kZoomEntry, kZoom);
    pApp->WriteProfileInt(kSection, kGridEntry, kGrid);
    pApp->WriteProfileInt(kSection, kPassabilityEntry, kPassability);
    pApp->WriteProfileInt(kSection, kAnimationEntry, kAnimation);
    pApp->WriteProfileInt(kSection, kCyclingEntry, kCycling);
    pApp->WriteProfileInt(kSection, kAutosaveEntry, kAutosaveMinutes);
    pApp->WriteProfileInt(kSection, kSpecialTileFrequencyEntry, kSpecialTileFrequency);
}
}
