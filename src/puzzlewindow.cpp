#include "va.h"

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
static Bitmap816* getPuzzleBitmap(long puzzle, long piece)
{
    char pieceName[40];
    sprintf(pieceName, "puz%s%02d.pcx", g_puzzleFilePrefixes[puzzle], piece);
    return ResourceManager::getBitmap816(pieceName);
}

VA(0x0052c1e0, 0x388)  // dc 0x114f40
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
        g_generalText->getText(GENERAL_TEXT_PUZZLE_WINDOW),
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

VA(0x0052c5a0, 0x96)  // dc 0x115268
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
VA(0x0052c640, 0x78)  // vtable slot 9 + CAdvPopup delegation, dc 0x115328
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

VA(0x0052c6c0, 0xAD)  // dc 0x1153a8
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

VA(0x0052c770, 0x140)  // dc 0x115538
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
VA(0x0052c8b0, 0xFC)  // bracketed between tile ctor and AI attempt, dc 0x11577c
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
VA(0x0052c9b0, 0x55B)  // anchor-caller, dc 0x115f64
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

            result.m_x = -1;
            result.m_y = -1;
            result.m_z = -1;

            int best = 0x7fff;
            type_point current;
            current.m_z = guess.m_z;
            for (current.m_x = guess.m_x - 2; current.m_x <= guess.m_x + 2;
                 ++current.m_x) {
                for (current.m_y = guess.m_y - 2; current.m_y <= guess.m_y + 2;
                     ++current.m_y) {
                    if (!current.isValid())
                        continue;
                    if (!g_game->m_worldMap.cell(current)->isDiggable())
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

    result.m_x = -1;
    result.m_y = -1;
    result.m_z = -1;
    return result;
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
            if (!(getMapExtra(point.m_x, point.m_y, point.m_z) & playerMask))
                continue;
            type_AI_puzzle_tile tile(
                g_game->m_worldMap.cell(point.m_x, point.m_y, point.m_z), point);
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

// Three phases. First a bounding-box sweep of the uncovered puzzle tiles that
// also remembers the FIRST visible one in row-major order - `found` is a
// separate latch because first_x/first_y are the scan's anchor, not the box's
// corner. Then the scan window, four nested _cpp_min/_cpp_max pairs whose
// const-reference temporaries are what put every operand in its own stack
// slot; the +9/+8 biases are the same puzzle-centre offsets
// AI_attempt_puzzle_guess re-applies to the answer. Finally the map sweep:
// every (x, y, z) the anchor could sit on is scored by check_match, the best
// score wins, and a tie count above two answers (-1,-1,-1) - retail builds
// that refusal in the dead `puzzle_map` parameter home rather than reusing
// `result`.

// The MAP_WIDTH in the Y window's first bound is retail's own: the second
// operand of that pair reads MAP_HEIGHT, and the two globals are loaded
// separately, so no CSE could have produced it. Transcribed as found.

// Residual (79.1843%): ONE structural fact, and everything else is its
// shift. Retail strength-reduces the bounding-box sweep's inner subscript
// into a running pointer - `[ebp-8] = puzzle_map + x*0x110 + 0xc`, then
// `add edi,0x10` per column - so its inner-loop head is a single block that
// the outer head falls into. Ours keeps `x*17` in a slot and rebuilds
// `(that + y) << 4` every iteration, which needs a rotated entry (`jmp`
// past the reload) and puts our whole block list one ahead of retail's from
// B1 on; 49 of the 70 blocks then pair as flow-kind mismatches purely from
// that offset. The blocker is that our `_cpp_min`/`_cpp_max` calls bind
// their const references DIRECTLY to `x`, `y` and the four accumulators -
// `lea ecx,[ebp-0x24]` on the counter's own home - which takes the
// induction variable's address and forbids the rewrite, while retail copies
// every operand into the same two temporaries ([ebp-0x30]/[ebp-0x2c]).

// 79.1843 -> 85.9430 (2026-09-05), and the note above named the answer
// without reaching it: the operand form IS the lever, and the spelling that
// supplies it is a NAMED LOCAL, not a cast.
//  - `int cur_x = x; int cur_y = y;` inside the visible arm, with the four
//    reducers reading those instead of the induction variables: 79.1843 ->
//    82.6674, and the block skeleton goes from ZERO exact blocks to 57 of
//    70 with flow-kind and target-shift both at 0. Copying only x (77.12)
//    or only y (82.22) is worse than copying both; copying the four
//    accumulators as well is worse again (80.76); naming `x + 1` / `y + 1`
//    costs 2.9. `x + 0` scores 82.78 and is not shipped - it is an invented
//    token for the same effect the local gets honestly.
//  - the two window bounds are then NESTED reducer calls whose INNER result
//    retail lands in a temp of its own: `int span_x = _cpp_max(first_x - 9,
//    first_x - min_x); int start_x = _cpp_max(span_x, 0);` and the same for
//    limit_x/end_x/span_y/limit_y. Naming the two start bounds' inners is
//    +2.16, naming all four is +2.99 (-> 85.6551), and it retires the last
//    two branch-polarity flips: branches now AGREE 40 = 40.
//  - the four reducers are then emitted MINS FIRST: min_x, min_y, max_x,
//    max_y is 85.9430 against 85.6551 for the x-then-y pairing, 85.81 for
//    two per-axis blocks, 85.68 for maxes first and 85.17 for interleaved.

// 85.9430 -> 92.4118 (2026-09-06), and again the note below named the answer
// without reaching it. "Retail runs every reducer operand through ONE pair of
// temporaries" is what VC6 emits when BOTH const-reference arguments need a
// CONVERSION - an `int` lvalue pair binds directly and copies nothing, however
// it is spelled. The explicit `long` template argument supplies the conversion
// on both sides: `std::_cpp_min<long>` / `std::_cpp_max<long>` at the four
// reducers and the four window bounds takes the skeleton to 70 = 70 blocks
// with 64 exact, 0 flow-kind, 0 target-shift, branches 40 = 40 and calls
// agreeing. Swapping the two min reducers' argument order is worth a further
// 0.02 (92.3936 -> 92.4118) and is kept because it also puts retail's
// `cmp _Y,_X` operand order back; swapping the maxes as well loses 0.01.

// Residual (92.41%): 6 size-only blocks, all of it slot layout - retail's
// frame is 0x80 against our 0x84 and it memory-homes `min_x` at [ebp-0x80],
// re-reading it per iteration, where we spread the copies over four slots and keep
// min_x in ESI. Our frame is 0x84 against retail's 0x80 for that reason.
VA(0x0052cf10, 0x5B4)  // anchor-caller AI_attempt_puzzle_guess +0x39d, dc 0x115be8
type_point matchPuzzle(long player, type_AI_puzzle_tile (*puzzleMap)[17])
{
    type_point result;
    result.m_x = -1;
    result.m_y = -1;
    result.m_z = -1;

    unsigned char found = 0;
    int firstX;
    int firstY;
    int minX = 19;
    int maxX = 0;
    int minY = 17;
    int maxY = 0;

    // MAX 92.4725 was measured with `x != 19` - an unnamed domain compare
    // that fails the cleanliness floor (docs/vc6/behavior-catalog.md D24).
    for (int x = 0; x < 19; ++x) {
        for (int y = 0; y < 17; ++y) {
            if (puzzleMap[x][y].m_visible) {
                if (!found) {
                    firstX = x;
                    firstY = y;
                    found = 1;
                }
                int curX = x;
                int curY = y;
                minX = std::_cpp_min<long>(minX, curX);
                minY = std::_cpp_min<long>(minY, curY);
                maxX = std::_cpp_max<long>(maxX, curX + 1);
                maxY = std::_cpp_max<long>(maxY, curY + 1);
            }
        }
    }

    if (!found)
        return result;

    int ties = 0;
    int best = 0;

    int spanX = std::_cpp_max<long>(firstX - 9, firstX - minX);
    int startX = std::_cpp_max<long>(spanX, 0);
    int limitX = std::_cpp_min<long>(g_mapWidth - maxX + firstX,
                                g_mapWidth + firstX - 9);
    int endX = std::_cpp_min<long>(g_mapWidth, limitX);
    int spanY = std::_cpp_max<long>(firstY - 8, firstY - minY);
    int startY = std::_cpp_max<long>(spanY, 0);
    int limitY = std::_cpp_min<long>(g_mapWidth - maxY + firstY,
                                g_mapHeight + firstY - 8);
    int endY = std::_cpp_min<long>(g_mapHeight, limitY);

    type_point scan;
    for (scan.m_z = 0; scan.m_z < g_game->m_worldMap.getNumLevels(); ++scan.m_z) {
        for (scan.m_y = startY; scan.m_y < endY; ++scan.m_y) {
            for (scan.m_x = startX; scan.m_x < endX; ++scan.m_x) {
                int count =
                    checkMatch(player, firstX, firstY, scan, puzzleMap);
                if (count != 0 && count >= best && count * 2 >= best) {
                    ++ties;
                    if (count > best) {
                        if (count > best * 2)
                            ties = 1;
                        best = count;
                        result.m_x = scan.m_x - firstX + 9;
                        result.m_y = scan.m_y - firstY + 8;
                        result.m_z = scan.m_z;
                    }
                }
            }
        }
    }

    if (ties > 2) {
        type_point ambiguous;
        ambiguous.m_x = -1;
        ambiguous.m_y = -1;
        ambiguous.m_z = -1;
        return ambiguous;
    }
    return result;
}

// COMDAT pairing: bitset<48>::test, agreement 1.000 at an exactly equal
// 52-byte extent.
VA_COMPGEN(0x005067e0, 0x34, BITSET_TEST, Bitset48)
