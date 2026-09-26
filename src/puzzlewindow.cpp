#include "va.h"
#include "includes.h"

#include <bitset>
#include <stdio.h>
#include <stdlib.h>

#include "puzzlewindow.h"

#include "advmgr_objects.h"
#include "bitmap816.h"
#include "border.h"
#include "button.h"
#include "exec.h"
#include "game.h"
#include "kb.h"
#include "message.h"
#include "resourcedisplay.h"
#include "resourcemanager.h"
#include "soundmgr.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"
#include "includes.h"

// Retail coordinates occupy nine 192-byte rows: 48 X words then 48 Y
// words; the old g_puzzlePieceY symbol was a +0x60 view of the same array.
DATA(0x006818a4) TPuzzleCoordinates g_puzzleCoordinates[9] = {
    { { 8, 8, 8, 8, 8, 8, 17, 23, 71, 73, 102, 107, 107, 115, 127, 129, 153, 155, 158, 167, 186, 213, 215, 218, 236, 246, 267, 289, 299, 322, 347, 355, 356, 376, 383, 409, 409, 422, 423, 427, 437, 459, 487, 488, 518, 521, 525, 526 },
      { 8, 30, 102, 156, 202, 320, 8, 406, 301, 194, 332, 8, 31, 60, 329, 191, 347, 239, 429, 470, 127, 335, 191, 226, 147, 77, 384, 288, 8, 177, 67, 459, 397, 162, 255, 32, 111, 147, 466, 8, 238, 336, 8, 144, 145, 68, 234, 327 } },
    { { 8, 8, 8, 8, 8, 8, 62, 98, 99, 109, 116, 130, 135, 158, 161, 163, 165, 175, 179, 188, 191, 216, 256, 266, 278, 279, 293, 295, 311, 331, 340, 340, 345, 362, 364, 399, 401, 405, 422, 430, 431, 463, 470, 487, 500, 512, 517, 526 },
      { 8, 101, 195, 310, 378, 449, 8, 42, 201, 308, 461, 366, 8, 188, 309, 441, 126, 390, 188, 258, 24, 272, 443, 323, 45, 383, 196, 266, 8, 493, 8, 167, 108, 239, 385, 310, 130, 436, 127, 8, 106, 393, 270, 8, 481, 255, 48, 169 } },
    { { 8, 8, 8, 8, 27, 28, 28, 29, 29, 38, 60, 77, 92, 115, 119, 133, 164, 172, 193, 213, 229, 232, 294, 298, 298, 299, 299, 313, 321, 341, 351, 351, 356, 358, 377, 389, 408, 422, 437, 446, 446, 464, 478, 498, 500, 504, 538, 557 },
      { 8, 52, 243, 486, 391, 31, 89, 303, 336, 234, 77, 462, 245, 31, 323, 87, 370, 255, 8, 483, 95, 205, 380, 190, 260, 8, 89, 462, 261, 17, 121, 174, 371, 469, 289, 8, 45, 284, 159, 8, 211, 422, 29, 153, 108, 281, 418, 215 } },
    { { 8, 8, 8, 8, 8, 8, 17, 42, 51, 52, 81, 82, 92, 116, 142, 154, 165, 174, 174, 188, 195, 201, 205, 240, 241, 272, 277, 297, 298, 307, 318, 328, 349, 349, 371, 402, 408, 422, 454, 456, 461, 476, 489, 505, 516, 518, 533, 557 },
      { 8, 16, 95, 271, 308, 464, 164, 378, 471, 101, 260, 48, 143, 8, 360, 269, 55, 101, 492, 160, 388, 373, 282, 469, 8, 163, 255, 428, 281, 8, 17, 84, 142, 342, 405, 103, 40, 508, 215, 377, 170, 319, 412, 8, 67, 211, 305, 335 } },
    { { 8, 8, 8, 8, 9, 15, 16, 35, 56, 56, 58, 95, 109, 120, 125, 132, 140, 146, 149, 176, 201, 202, 211, 248, 251, 263, 294, 304, 319, 346, 357, 358, 363, 383, 383, 422, 423, 429, 430, 444, 453, 466, 470, 477, 538, 548, 560, 559 },
      { 8, 188, 329, 403, 8, 138, 8, 374, 82, 150, 281, 188, 344, 424, 256, 8, 92, 371, 42, 200, 291, 66, 482, 98, 227, 8, 373, 286, 173, 444, 8, 386, 38, 8, 119, 164, 249, 52, 101, 132, 239, 441, 300, 20, 249, 430, 140, 8 } },
    { { 8, 8, 8, 8, 31, 34, 58, 63, 64, 84, 95, 100, 114, 157, 166, 173, 178, 195, 205, 237, 239, 246, 248, 248, 254, 302, 324, 324, 329, 330, 340, 358, 375, 375, 393, 401, 402, 423, 437, 439, 450, 454, 472, 478, 481, 486, 537, 542 },
      { 8, 125, 353, 394, 101, 219, 171, 8, 90, 471, 117, 8, 258, 146, 288, 388, 36, 235, 502, 320, 8, 75, 396, 459, 152, 233, 8, 178, 342, 428, 8, 141, 8, 236, 439, 291, 103, 381, 8, 336, 131, 161, 267, 64, 456, 8, 197, 22 } },
    { { 8, 8, 8, 8, 13, 33, 33, 37, 40, 48, 50, 71, 102, 112, 124, 139, 141, 145, 150, 159, 192, 203, 219, 220, 223, 263, 280, 280, 304, 321, 327, 334, 363, 366, 381, 393, 428, 446, 447, 460, 464, 485, 489, 490, 530, 530, 559, 564 },
      { 8, 229, 405, 465, 8, 245, 277, 337, 15, 115, 178, 8, 35, 311, 156, 423, 224, 136, 452, 475, 68, 12, 349, 285, 96, 8, 166, 425, 314, 109, 146, 160, 26, 441, 297, 242, 275, 85, 424, 347, 53, 210, 8, 303, 8, 421, 87, 261 } },
    { { 8, 8, 8, 8, 8, 24, 33, 43, 48, 57, 72, 88, 91, 105, 117, 135, 176, 192, 201, 217, 222, 226, 240, 246, 249, 262, 263, 298, 310, 321, 324, 327, 332, 346, 354, 359, 382, 401, 429, 449, 452, 454, 463, 466, 487, 493, 518, 550 },
      { 8, 152, 306, 388, 434, 417, 232, 137, 440, 19, 8, 219, 26, 397, 345, 215, 168, 428, 326, 98, 398, 235, 8, 40, 208, 439, 134, 352, 99, 262, 404, 200, 20, 178, 8, 290, 399, 65, 160, 293, 94, 424, 397, 8, 163, 184, 304, 8 } },
    { { 8, 8, 8, 8, 16, 46, 49, 87, 94, 100, 102, 105, 108, 125, 135, 182, 183, 190, 193, 193, 202, 204, 229, 236, 243, 276, 279, 291, 292, 309, 311, 313, 318, 324, 328, 331, 350, 350, 408, 422, 429, 468, 482, 490, 505, 505, 508, 543 },
      { 8, 54, 227, 426, 48, 375, 249, 500, 55, 245, 354, 175, 14, 296, 8, 466, 200, 381, 40, 364, 124, 330, 293, 39, 335, 488, 202, 80, 115, 225, 158, 24, 8, 443, 253, 36, 330, 426, 191, 430, 246, 90, 13, 346, 113, 190, 8, 436 } }
};
DATA(0x006976e8) std::bitset<48> g_puzzlePiecesRemoved;

// Retail initial data; dimensions follow the typed table consumers.
DATA(0x00681f64) short g_puzzlePieceOrder[432] = {
    2, 7, 47, 39, 4, 19, 45, 11,
    10, 38, 46, 13, 5, 14, 36, 9,
    31, 44, 25, 8, 37, 28, 15, 40,
    3, 16, 32, 12, 18, 33, 20, 17,
    43, 24, 21, 34, 35, 6, 41, 30,
    1, 26, 42, 0, 27, 29, 23, 22,
    10, 0, 43, 44, 3, 12, 47, 22,
    7, 30, 42, 11, 1, 28, 41, 15,
    2, 46, 29, 4, 6, 38, 5, 8,
    36, 25, 13, 31, 45, 16, 40, 33,
    17, 32, 9, 37, 39, 14, 34, 20,
    19, 35, 23, 24, 21, 18, 26, 27,
    0, 42, 33, 11, 10, 35, 43, 46,
    2, 30, 22, 4, 1, 25, 44, 34,
    7, 39, 45, 3, 13, 40, 17, 41,
    5, 32, 8, 27, 12, 18, 37, 6,
    9, 26, 15, 47, 29, 38, 20, 19,
    31, 14, 16, 36, 28, 21, 23, 24,
    5, 1, 27, 30, 18, 11, 44, 47,
    4, 2, 29, 37, 8, 9, 45, 23,
    21, 35, 28, 34, 20, 6, 33, 43,
    46, 3, 13, 31, 39, 14, 0, 36,
    38, 10, 17, 24, 42, 7, 16, 40,
    15, 41, 32, 12, 22, 26, 25, 19,
    1, 29, 43, 25, 12, 36, 30, 6,
    10, 45, 38, 15, 13, 31, 37, 16,
    2, 22, 47, 0, 33, 46, 18, 3,
    41, 35, 8, 17, 40, 23, 5, 7,
    44, 34, 11, 26, 14, 42, 19, 9,
    39, 21, 32, 4, 20, 27, 28, 24,
    1, 45, 18, 32, 3, 46, 11, 40,
    44, 26, 9, 47, 6, 38, 23, 41,
    20, 36, 5, 43, 29, 30, 10, 39,
    19, 31, 21, 22, 33, 13, 28, 0,
    2, 34, 4, 12, 37, 16, 15, 42,
    8, 14, 35, 7, 27, 17, 25, 24,
    11, 37, 45, 2, 0, 32, 19, 4,
    25, 47, 3, 20, 46, 33, 1, 17,
    41, 18, 29, 27, 7, 42, 5, 40,
    9, 28, 10, 35, 22, 24, 38, 44,
    6, 43, 8, 15, 39, 21, 13, 34,
    12, 23, 36, 30, 14, 31, 16, 26,
    10, 41, 34, 4, 46, 43, 25, 22,
    8, 37, 30, 23, 17, 32, 36, 15,
    20, 19, 28, 27, 16, 26, 33, 18,
    3, 40, 1, 39, 2, 47, 11, 45,
    13, 38, 0, 42, 5, 7, 44, 6,
    12, 35, 14, 9, 31, 21, 29, 24,
    0, 32, 47, 7, 1, 14, 25, 3,
    18, 31, 33, 9, 27, 37, 46, 19,
    28, 43, 2, 4, 44, 15, 12, 45,
    40, 10, 23, 41, 36, 6, 42, 39,
    17, 11, 35, 5, 8, 30, 24, 13,
    34, 21, 16, 38, 22, 26, 29, 20
};
DATA(0x006822c8) double g_puzzleGuessThreshold[5] = { 1.1, 0.5, 0.25, 0.0, 0.0 };
DATA(0x00681880) const char* g_puzzleFilePrefixes[9] = { "cas", "ram", "tow", "inf", "nec", "dun", "str", "for", "Ele" };

// E:\gamedcs\puzzlewindow.cpp:103
MAC_ADDRESS(0x14723c, 0x40)
static Bitmap816* getPuzzleBitmap(long puzzle, long piece)
{
    char pieceName[40];
    sprintf(pieceName, "puz%s%02d.pcx", g_puzzleFilePrefixes[puzzle], piece);
    return ResourceManager::getBitmap816(pieceName);
}

VA(0x0052c1e0, 0x388) MAC_ADDRESS(0x14727c, 0x548)  // dc 0x114f40
TPuzzleWindow::TPuzzleWindow(int puzzlenum)
    : CAdvPopup(0, 0, 800, 600, 0)
{
    m_x = 0;
    m_y = 0;
    m_width = 800;
    m_height = 600;
    m_puzWhich = puzzlenum;

    m_widgets.reserve(NWIDGETS);

    bitmapBorder* background =
        new bitmapBorder(0, 0, 800, 600, BACKGROUND_ID,
                         "puzzle.pcx", 0x800);
    background->setPlayerPaletteColors(g_game->getLocalPlayerGamePos());
    m_widgets.push_back(background);

    m_widgets.push_back(new bitmapBorder(607, 3, 190, 71, -1,
                                       "puzzlogo.pcx", 0x800));

    m_widgets.push_back(new textWidget(
        607, 73, 190, 40,
        (*g_generalText)[GENERAL_TEXT_PUZZLE_WINDOW],
        "Bigfont.fnt", font::HEADING, -1,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));

    m_widgets.push_back(new bitmapBorder(669, 537, 68, 34, -1,
                                       "box66x32.pcx", 0x800));

    button* accept = new button(670, 538, 66, 32, ACCEPT_ID,
                                "iOkay32.def", 0, 1, 0, 28, 2);
    accept->setHotkey(1);
    m_widgets.push_back(accept);

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    g_soundManager->stopAllSamples(1);

    for (int i = 0; i < 48; ++i) {
        m_puzzlePieces[i] = getPuzzleBitmap(m_puzWhich, i);
    }

    m_puzzleResourceBar = new TResourceDisplay(this, 0);
    drawWindow(0, -65535, 65535);
}

VA_COMPGEN(0x0052c570, 0x21, SCALAR_DELETING_DTOR, TPuzzleWindow)

VA(0x0052c5a0, 0x96) MAC_ADDRESS(0x1477c4, 0x10c)  // dc 0x115268
TPuzzleWindow::~TPuzzleWindow()
{
    for (int i = 0; i < 48; ++i)
        m_puzzlePieces[i]->dispose();

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }

    if (m_puzzleResourceBar) {
        delete m_puzzleResourceBar;
        m_puzzleResourceBar = 0;
    }
}

// Original: TPuzzleWindow::convertID2HelpID; puzzlewindow.cpp:179, dc 0x11530c.
// The old help-index query remains separate from Complete's window handler,
// whose hover handling now delegates to CAdvPopup.
int TPuzzleWindow::convertID2HelpID(int id) const
{
    if (id < 0)
        return -1;
    switch (id) {
    case ACCEPT_ID: return ACCEPT_HELP_ID;
    default: return -1;
    }
}

// E:\gamedcs\puzzlewindow.cpp:203
VA(0x0052c640, 0x78) MAC_ADDRESS(0x1478d0, 0xfc)  // vtable slot 9 + CAdvPopup delegation, dc 0x115328
int TPuzzleWindow::windowHandler(message& msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    pollSound();
    if (msg.m_id == MESSAGE_KEY_DOWN) {
        switch (msg.m_codeX) {
        case DIALOG_CLOSE_KEY:
        case DIALOG_ACCEPT_KEY:
            msg.m_codeY = ACCEPT_ID;
            break;
        default:
            return MESSAGE_DISPATCH_CONSUME;
        }
    } else if (msg.m_id != MESSAGE_WIDGET ||
               msg.m_codeX != widget::WIDGET_DESELECT ||
               msg.m_codeY != ACCEPT_ID) {
        return MESSAGE_DISPATCH_CONSUME;
    }

    msg.m_id = MESSAGE_WIDGET;
    g_windowManager->m_dialogReturn = msg.m_codeY;
    msg.m_codeY = widget::WIDGET_END_DIALOG;
    msg.m_codeX = widget::WIDGET_END_DIALOG;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x0052c6c0, 0xAD) MAC_ADDRESS(0x1479cc, 0xec)  // dc 0x1153a8
int TPuzzleWindow::updatePuzzle(int full)
{
    int piecesNotFound = 0;

    for (int i = 0; i < 48; ++i) {
        if (full || !g_puzzlePiecesRemoved.test(i)) {
            int piece = g_puzzlePieceOrder[m_puzWhich * 48 + i];
            Bitmap816* bitmap = m_puzzlePieces[piece];
            const short* xCoordinate = g_puzzleCoordinates[m_puzWhich].m_x;
            const short* yCoordinate = g_puzzleCoordinates[m_puzWhich].m_y;
            bitmap->draw(0, 0, bitmap->getWidth(), bitmap->getHeight(),
                         g_windowManager->m_screenBitmap,
                         xCoordinate[piece], yCoordinate[piece], 1);
            ++piecesNotFound;
        }
    }

    m_puzzleResourceBar->update(1, 0);
    return piecesNotFound;
}

// Retail preserves the Dreamcast record's four packed allocation units:
// a 10-bit object type, two signed four-bit object offsets, three terrain
// descriptors, and the diggable/grail/visible flag trio.
struct type_AI_puzzle_tile {
    int m_objectType : 10;
    int m_paddingAfterObjectType : 22;
    signed char m_objectX : 4;
    signed char m_objectY : 4;
    char m_paddingBeforeTerrain[3];
    int m_terrain : 5;
    int m_river : 4;
    int m_road : 4;
    int m_paddingAfterRoad : 19;
    unsigned char m_diggable : 1;
    unsigned char m_hasGrail : 1;
    unsigned char m_visible : 1;
    unsigned char m_paddingAfterVisible : 5;
    char m_tailPadding[3];

    // Retail expands these stores in AI_attempt_puzzle_guess's array loop.
    // E:\gamedcs\puzzlewindow.cpp:279, dc 0x1154c4
    MAC_ADDRESS(0x147ab8, 0x70)
    type_AI_puzzle_tile()
    {
        m_objectType = 0;
        m_objectX = -1;
        m_objectY = -1;
        m_terrain = -1;
        m_river = 0;
        m_road = 0;
        m_diggable = 1;
        m_visible = 0;
    }
    type_AI_puzzle_tile(NewmapCell* cell, type_point point);
    unsigned char operator==(const type_AI_puzzle_tile* arg) const;
};
SIZE(type_AI_puzzle_tile, 0x10);

type_point matchPuzzle(long player, type_AI_puzzle_tile (*puzzleMap)[17]);

VA(0x0052c770, 0x140) MAC_ADDRESS(0x147b28, 0x1e0)  // dc 0x115538
type_AI_puzzle_tile::type_AI_puzzle_tile(NewmapCell* cell, type_point point)
{
    m_objectType = 0;
    m_visible = 1;
    m_objectX = -1;
    m_objectY = -1;

    m_terrain = cell->m_groundSet;
    m_river = cell->m_riverSet;
    m_road = cell->m_roadSet;
    m_diggable = cell->isDiggable();

    m_hasGrail = point.m_x == g_game->m_ultimateArtifactX &&
                point.m_y == g_game->m_ultimateArtifactY &&
                point.m_z == g_game->m_ultimateArtifactZ;

    if (cell->m_objectTypeIndex >= 0) {
        CObject& cellObject =
            g_game->m_worldMap.m_objects[cell->m_objectTypeIndex];
        m_objectType = cellObject.getType();
        m_objectX = cellObject.m_x - point.m_x;
        m_objectY = cellObject.m_y - point.m_y;
    }
}

// E:\gamedcs\puzzlewindow.cpp:319. Retail keeps NO out-of-line row: the one
// caller is check_match below, and /Ob2 expands the whole chain there. The
// comparison is byte-proven by that expansion, which XORs the packed words
// and masks each group - `test ecx,0x3ff` for object_type, `test cl,0xff` for
// the two four-bit offsets together, `test cl,0x1f` for terrain, `test
// ecx,0x1fe0` for river and road together, and `test dl,1` for diggable.
// has_grail and visible are deliberately NOT compared.

unsigned char type_AI_puzzle_tile::operator==(
    const type_AI_puzzle_tile* arg) const
{
    return m_objectType == arg->m_objectType
        && m_objectX == arg->m_objectX
        && m_objectY == arg->m_objectY
        && m_terrain == arg->m_terrain
        && m_river == arg->m_river
        && m_road == arg->m_road
        && m_diggable == arg->m_diggable;
}

// E:\gamedcs\puzzlewindow.cpp:334
// DC puzzlewindow.cpp:390/391/393 and retail advance sample/row cursors
// unconditionally. This exact reconstruction retains that original defect:
// a 33x33 bitmap at dest(16,16) has valid samples but forms a final source
// row at byte offset 2112 beyond its 1089-byte allocation. A clipped bottom
// puzzle row can similarly form visible+324 beyond visible[17*19]. These
// final cursors are not dereferenced, but their formation is not valid portable
// C++. The earlier guarded repair scored 60.63%; bounded offset alternatives
// remain non-exact. No extra allocation or padding guarantee is claimed.
VA(0x0052c8b0, 0xFC) MAC_ADDRESS(0x147e80, 0xf0)  // bracketed between tile ctor and AI attempt, dc 0x11577c
void Bitmap816::markPuzzle(unsigned char* visible, long destX, long destY)
{
    int offsetX = (-16 - destX) & 31;
    int offsetY = (-16 - destY) & 31;
    int width = m_width - offsetX;
    int height = m_height - offsetY;

    destX += offsetX;
    destY += offsetY;

    if (destX + width > 608)
        width = 608 - destX;
    if (destY + height > 544)
        height = 544 - destY;

    if (width <= 0 || height <= 0)
        return;

    unsigned char* source = m_map + m_pitch * offsetY + offsetX;
    int row = destY / 32;
    unsigned char* destination = visible + 18 * row;
    destination += row;
    destination += destX / 32;

    for (int y = 0; y < height; y += 32) {
        unsigned char* destinationBlock = destination;
        unsigned char* sourceBlock = source;

        for (int x = 0; x < width; x += 32) {
            if (*sourceBlock)
                *destinationBlock = 0;
            sourceBlock += 32;
            ++destinationBlock;
        }

        destination += 19;
        source += m_pitch * 32;
    }
}

// E:\gamedcs\puzzlewindow.cpp:403.
// Complete reads setup alignment directly and disposes through the bitmap
// vtable; those retail operations override the older DC callees.

MAC_ADDRESS(0x147f70, 0x148)
static unsigned char markAIPuzzle(long player, unsigned char* visible)
{
    long puzzle;
    if (player < 0 || (puzzle = g_game->m_setup.m_alignment[player]) == -1)
        puzzle = 0;
    if (g_game->m_ultimateArtifactX < 0 || !g_game->m_ultimateArtifactPresent)
        return 0;
    if (!g_game->setupPuzzlePieces(player, 0))
        return 0;
    memset(visible, 1, 17 * 19);
    for (int i = 0; i < 48; ++i) {
        if (g_puzzlePiecesRemoved[i])
            continue;
        int piece = g_puzzlePieceOrder[puzzle * 48 + i];
        Bitmap816* bitmap = getPuzzleBitmap(puzzle, piece);
        const short* xCoordinate = g_puzzleCoordinates[puzzle].m_x;
        const short* yCoordinate = g_puzzleCoordinates[puzzle].m_y;
        bitmap->markPuzzle(visible, xCoordinate[piece] - 8,
                            yCoordinate[piece] - 8);
        bitmap->dispose();
    }
    return 1;
}

// E:\gamedcs\puzzlewindow.cpp:445.
// DC proves the array reference and point local.
// Complete's tile dimensions are 19x17, independently fixed by retail strides.

MAC_ADDRESS(0x1480b8, 0x190)
static void createAIPuzzleMap(long player, unsigned char* visible,
                            long puzzleX, long puzzleY,
                            type_AI_puzzle_tile (&puzzleMap)[19][17])
{
    type_point point;
    point.m_z = g_game->m_ultimateArtifactZ;

    for (int row = 0; row < 17; ++row) {
        point.m_y = puzzleY + row;
        for (int col = 0; col < 19; ++col) {
            point.m_x = puzzleX + col;
            if (point.isValid() && visible[row * 19 + col]) {
                NewmapCell* mapCell = g_game->getCell(point);
                puzzleMap[col][row] =
                    type_AI_puzzle_tile(mapCell, point);
            }
        }
    }
}

// E:\gamedcs\puzzlewindow.cpp:614, dc 0x115838
VA(0x0052c9b0, 0x55B) MAC_ADDRESS(0x1489fc, 0x428)  // anchor-caller, dc 0x115f64
type_point aiAttemptPuzzleGuess(long player)
{
    type_point result;

    int found = g_game->setupPuzzlePieces(player, 1);
    double uncovered =
        found / static_cast<double>(TPuzzleWindow::PUZZLE_PIECE_COUNT);
    if (g_puzzleGuessThreshold[g_game->m_setup.m_difficulty] <= uncovered) {
        unsigned char visible[17 * 19];
        if (markAIPuzzle(player, visible)) {
            type_point origin = g_game->getPuzzleOrigin();
            type_AI_puzzle_tile puzzleMap[19][17];

            createAIPuzzleMap(player, visible, origin.m_x, origin.m_y, puzzleMap);

            type_point guess = matchPuzzle(player, puzzleMap);
            if (guess.m_x < 0)
                return guess;

            result = type_point(-1, -1, -1);

            int best = 0x7fff;
            type_point current;
            current.m_z = guess.m_z;
            for (current.m_x = guess.m_x - 2; current.m_x <= guess.m_x + 2;
                 ++current.m_x) {
                for (current.m_y = guess.m_y - 2; current.m_y <= guess.m_y + 2;
                     ++current.m_y) {
                    if (!current.isValid())
                        continue;
                    if (!g_game->getCell(current)->isDiggable())
                        continue;

                    type_point index;
                    index.m_x = current.m_x - guess.m_x + 9;
                    index.m_y = current.m_y - guess.m_y + 8;

                    if (puzzleMap[index.m_x][index.m_y].m_terrain >= 0) {
                        if (puzzleMap[index.m_x][index.m_y].m_hasGrail)
                            return current;
                    } else {
                        int distance = abs(guess.m_y - current.m_y)
                                       + abs(guess.m_x - current.m_x);
                        if (result.m_x < 0 || distance < best) {
                            best = distance;
                            result = current;
                        }
                    }
                }
            }

            return result;
        }
    }

    return type_point(-1, -1, -1);
}

// E:\gamedcs\puzzlewindow.cpp:472. Retail expands this file static into
// match_puzzle below - its only call site - so no out-of-line retail row
// exists; the Dreamcast build kept one at dc 0x115a70. `origin` arrives BY
// VALUE and is rebased in place (the dword copy at match_puzzle+0x26c then
// two bitfield writes), and both index parameters are reused as the scan's
// own counters: `first_y` keeps the caller's value as its lower bound while
// `first_x` restarts at zero. A single mismatched tile answers zero
// immediately, which is why retail's failure edge jumps straight past the
// caller's score test.

MAC_ADDRESS(0x148248, 0x240)
static long checkMatch(long player, long firstX, long firstY,
                        type_point origin,
                        type_AI_puzzle_tile (*puzzleMap)[17])
{
    long playerMask = 1 << player;
    origin.m_x = origin.m_x - firstX;
    origin.m_y = origin.m_y - firstY;
    long matches = 0;

    type_point point;
    point.m_z = origin.m_z;
    for (; firstY < 17; ++firstY) {
        point.m_y = origin.m_y + firstY;
        for (firstX = 0; firstX < 19; ++firstX) {
            point.m_x = origin.m_x + firstX;
            if (!point.isValid())
                continue;
            if (!puzzleMap[firstX][firstY].m_visible)
                continue;
            if (!(getMapExtra(point) & playerMask))
                continue;
            type_AI_puzzle_tile tile(g_game->getCell(point), point);
            if (puzzleMap[firstX][firstY] == &tile)
                ++matches;
            else
                return 0;
        }
    }
    return matches;
}

// E:\gamedcs\puzzlewindow.cpp:520. The one AI/puzzle helper retail keeps out
// of line, called from AI_attempt_puzzle_guess above; the declaration lives
// in puzzlewindow.h so both sides of that call agree.

// DC puzzlewindow.cpp:523-579 names first, result, point, and the two RECT
// locals extents/rect. Lines 547-550 update left/right/top/bottom in that
// order; lines 560-577 clamp each rect member through the shared min/max
// wrappers. Their by-value operands explain retail's temporary copies.
// Complete scans 19x17 cells instead of DC's 13x12. Retail and DC both use
// map width in the Y window's first upper bound; preserve that asymmetry.
// Restoring the rectangles, point constructors and map-level accessor gives
// 95.3093%, from 92.4089% with scalar carriers and explicit long selectors.
// Omitting DC's unused first-tile snapshot gives the same score. The remaining
// differences are four size-only blocks with matching branch/call structure.
// VC6 resolves the RECT LONG/LONG min calls to the integer wrapper; Clang
// considers the integer/double overloads ambiguous, leaving an audit gap.
VA(0x0052cf10, 0x5B4) MAC_ADDRESS(0x148488, 0x574)  // anchor-caller AI_attempt_puzzle_guess +0x39d, dc 0x115be8
type_point matchPuzzle(long player, type_AI_puzzle_tile (*puzzleMap)[17])
{
    type_AI_puzzle_tile first;
    unsigned char found = 0;
    type_point result(-1, -1, -1);
    int firstX;
    int firstY;
    RECT extents;
    extents.left = 19;
    extents.right = 0;
    extents.top = 17;
    extents.bottom = 0;

    for (int x = 0; x < 19; ++x) {
        for (int y = 0; y < 17; ++y) {
            if (puzzleMap[x][y].m_visible) {
                if (!found) {
                    first = puzzleMap[x][y];
                    firstX = x;
                    firstY = y;
                    found = 1;
                }
                extents.left = min(extents.left, x);
                extents.right = max(extents.right, x + 1);
                extents.top = min(extents.top, y);
                extents.bottom = max(extents.bottom, y + 1);
            }
        }
    }

    if (!found)
        return result;

    type_point point;
    int ties = 0;
    int best = 0;

    RECT rect;
    rect.left = firstX - 9;
    rect.left = max(rect.left, firstX - extents.left);
    rect.left = max(rect.left, 0);
    rect.right = g_mapWidth + firstX - 9;
    rect.right = min(rect.right, g_mapWidth - extents.right + firstX);
    rect.right = min(rect.right, g_mapWidth);
    rect.top = firstY - 8;
    rect.top = max(rect.top, firstY - extents.top);
    rect.top = max(rect.top, 0);
    rect.bottom = g_mapHeight + firstY - 8;
    rect.bottom = min(rect.bottom, g_mapWidth - extents.bottom + firstY);
    rect.bottom = min(rect.bottom, g_mapHeight);

    for (point.m_z = 0; point.m_z < g_game->getNumMapLevels(); ++point.m_z) {
        for (point.m_y = rect.top; point.m_y < rect.bottom; ++point.m_y) {
            for (point.m_x = rect.left; point.m_x < rect.right; ++point.m_x) {
                int count =
                    checkMatch(player, firstX, firstY, point, puzzleMap);
                if (count != 0 && count >= best && count * 2 >= best) {
                    ++ties;
                    if (count > best) {
                        if (count > best * 2)
                            ties = 1;
                        best = count;
                        result.m_x = point.m_x - firstX + 9;
                        result.m_y = point.m_y - firstY + 8;
                        result.m_z = point.m_z;
                    }
                }
            }
        }
    }

    if (ties > 2)
        return type_point(-1, -1, -1);
    return result;
}

// COMDAT pairing: bitset<48>::test, agreement 1.000 at an exactly equal
// 52-byte extent.
VA_COMPGEN(0x005067e0, 0x34, BITSET_TEST, Bitset48)
