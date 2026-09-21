// 2 functions in link order.
#include <stdlib.h>
#include <vector>

#include "monframeinfo.h"

#include "resourcemanager.h"
#include "textresource.h"
#include "va.h"

static void initializeCreatureAnimationTraits(int id,
    const std::vector<char*, std::allocator<char*> >& row);

// The parse target. File-static (the Dreamcast dump publishes only the
// gMonFrameInfo reference below, never the array); name provisional.
// Extent proof: bss 0x6998e0 up to mousemgr.cpp's timer latches at
// 0x69ca18 is exactly 150 * 0x54.
DATA(0x006998e0)
static SMonFrameInfo g_monFrameInfoTable[150];

// Dreamcast public ?gMonFrameInfo@@3AAY0HK@$$CBUSMonFrameInfo@@A - the
// const-reference view the rest of the game reads. Retail keeps it as
// the .data cell 0x67ff24 -> 0x6998e0, directly before this TU's
// "cranim.txt" literal (monframeinfo.obj's whole .data contribution).
// The DC bound is 122 (RoE-era roster); retail's extent proves 150.
DATA(0x0067ff24)
const SMonFrameInfo (&g_monFrameInfo)[150] = g_monFrameInfoTable;

VA(0x0050c810, 0x1E9)  // dc 0xfe598
unsigned char initializeCreatureAnimationTraitsTable()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x0067ff28, cranimSpreadsheetName, "cranim.txt"));
    if (!sheet)
        return 0;
    if (sheet->getNumberOfRows() < 179) {
        sheet->dispose();
        return 0;
    }
    int id = 0;
    int row = 2;
    { for (int t = 0; t < 14; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 14; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 14; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 14; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 14; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 14; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 14; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 14; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 6; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 14; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 13; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    row += 3;
    { for (int t = 0; t < 4; ++t) {
        initializeCreatureAnimationTraits(id, sheet->getRow(row));
        ++id;
        ++row;
    } }
    sheet->dispose();
    return 1;
}

VA(0x0050ca00, 0x126)  // dc 0xfe764
static void initializeCreatureAnimationTraits(int id,
    const std::vector<char*, std::allocator<char*> >& row)
{
    SMonFrameInfo& traits = g_monFrameInfoTable[id];

    traits.m_fidgetFrequency = static_cast<int>(atof(row[0])
        * DATA_COMPGEN(0x00640020, fidgetFrequencyScale, 9000.0));
    traits.m_walkCycleTime = static_cast<int>(atof(row[1]) * 500.0);
    traits.m_attackStartCycleTime = static_cast<int>(atof(row[2]) * 500.0);
    traits.m_flightPixelSpan = static_cast<int>(atof(row[3])
        * DATA_COMPGEN(0x00640018, flightPixelScale, 115.0));
    traits.m_missileOffset[0] = static_cast<short>(atoi(row[4]));
    traits.m_missileOffset[1] = static_cast<short>(atoi(row[5]));
    traits.m_missileOffset[2] = static_cast<short>(atoi(row[6]));
    traits.m_missileOffset[3] = static_cast<short>(atoi(row[7]));
    traits.m_missileOffset[4] = static_cast<short>(atoi(row[8]));
    traits.m_missileOffset[5] = static_cast<short>(atoi(row[9]));
    { for (int i = 10; i < 22; ++i)
        traits.m_arrowAngle[i - 10] = static_cast<float>(atof(row[i])); }
    traits.m_extraNumTroopsXOffset = atoi(row[22]);
    traits.m_attackFrames = atoi(row[23]);
}
