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
#include "includes.h"

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
            const short* xCoordinate = g_puzzlePieceX + m_puzWhich * 96;
            const short* yCoordinate = g_puzzlePieceY + m_puzWhich * 96;
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
        const short* xCoordinate = g_puzzlePieceX + puzzle * 96;
        const short* yCoordinate = g_puzzlePieceY + puzzle * 96;
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
VA(0x0052cf10, 0x5B4)  // anchor-caller AI_attempt_puzzle_guess +0x39d, dc 0x115be8
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
