// Options.cpp - the editor's saved settings (h3maped 0x492cba..0x4930c8,
// name inferred). An out-of-range zoom resets to full size, an autosave
// period over an hour turns autosave off, and a special tile frequency
// over 8 returns to its default 4.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Options.h"
#include "editor/Tile.h"

namespace {
DATA(0x005a2400) int zoomImp;
DATA(0x005a2404) bool bGridImp;
DATA(0x005a2405) bool bPassabilityImp;
DATA(0x005a2406) bool bAnimationImp;
DATA(0x005a2407) bool bCyclingImp;
DATA(0x005a2408) unsigned int autosaveMinutesImp;
DATA(0x0058d2a8) unsigned int specialTileFrequencyImp = 4;
}

namespace SOptions {
const int& kZoom = zoomImp;
const bool& kGrid = bGridImp;
const bool& kPassability = bPassabilityImp;
const bool& kAnimation = bAnimationImp;
const bool& kCycling = bCyclingImp;
const unsigned int& kAutosaveMinutes = autosaveMinutesImp;
const unsigned int& kSpecialTileFrequency = specialTileFrequencyImp;

VA(0x00492edb, 0x101)
void load()
{
    CWinApp* pApp = AfxGetApp();
    zoomImp = pApp->GetProfileInt("Settings", "Zoom", zoomImp);
    if (zoomImp < 0 || zoomImp >= kNumZooms)
        zoomImp = eZoom100;
    bGridImp = pApp->GetProfileInt("Settings", "Grid", bGridImp) != 0;
    bPassabilityImp = pApp->GetProfileInt("Settings", "Passability", bPassabilityImp) != 0;
    bAnimationImp = pApp->GetProfileInt("Settings", "Animation", bAnimationImp) != 0;
    bCyclingImp = pApp->GetProfileInt("Settings", "Cycling", bCyclingImp) != 0;
    autosaveMinutesImp = pApp->GetProfileInt("Settings", "Autosave", autosaveMinutesImp);
    if (autosaveMinutesImp > 60)
        autosaveMinutesImp = 0;
    specialTileFrequencyImp = pApp->GetProfileInt("Settings", "SpecialTileFreq", specialTileFrequencyImp);
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
    pApp->WriteProfileInt("Settings", "Zoom", kZoom);
    pApp->WriteProfileInt("Settings", "Grid", kGrid);
    pApp->WriteProfileInt("Settings", "Passability", kPassability);
    pApp->WriteProfileInt("Settings", "Animation", kAnimation);
    pApp->WriteProfileInt("Settings", "Cycling", kCycling);
    pApp->WriteProfileInt("Settings", "Autosave", kAutosaveMinutes);
    pApp->WriteProfileInt("Settings", "SpecialTileFreq", kSpecialTileFrequency);
}
}
