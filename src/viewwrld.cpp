// 26 Dreamcast functions in link order; 20 compiler-generated $-thunks
// omitted. Complete adds the two retail-only level-button callbacks below.
#include "va.h"
#include "objnames.h"
#include "includes.h"

#include "viewwrld.h"

#include "advmgr.h"
#include "bitmap16.h"
#include "border.h"
#include "button.h"
#include "csprite.h"
#include "game.h"
#include "iconwdgt.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "message.h"
#include "mousemgr.h"
#include "recruit.h"
#include "resourcemanager.h"
#include "soundmgr.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// Dreamcast publishes this source-private renderer state by name. Retail
// independently fixes each address through the repeated view-world draw
// family: the scale and sampling table feed every scaled blit, the two center
// offsets form each destination origin, and the buffer/sprite pair are owned
// by ViewWorld's setup/teardown path.
// THE VIEW-WORLD CHECKBOX STATE, hoisted here because VWDrawSymbols gates
// each of its five arms on one of them. advManager::ViewWorld's Dreamcast
// body keeps view_mines / view_heroes / view_towns as locals (S_REGREL32
// rows at dc 0x195b48); Complete's level callbacks below read them from
// viewwrld.obj's own .bss, which is what made them file statics. The
// Dreamcast ALSO publishes all five as globals - iVWMines, iVWResources,
// iVWArtifacts, iVWTowns, iVWHeroes, every one of them `bool` - and their
// RELATIVE order survives the port intact: iVWResources sits immediately
// after iVWTerrains in both images (dc 0x38010/0x38014 against retail
// 0x6aab78/0x6aab79) and mines < artifacts < towns < heroes ascends the
// same way in both. That correspondence is what fixes the two addresses
// this file had no reader for until now.
DATA(0x0068c6bc) int g_viewWorldScale;
DATA(0x006aab68)
static unsigned char g_viewMines;
DATA(0x006aab78) bool g_vwTerrains;
DATA(0x006aab79)
static unsigned char g_viewResources;
// The half-extents init derives from the two viewable dimensions and the
// three view-world readers below consume. Only viewwrld.obj references
// either address (four dir32 sites each: init writes both, and
// update_view_world / update_radar / WindowHandler read them), which is
// what makes them file statics rather than shared globals. The names are
// role-based house placeholders; neither CodeView corpus spells them.
DATA(0x006aab7c)
static int g_viewHalfHeight;
DATA(0x006aab80)
static int g_viewHalfWidth;
DATA(0x006aab84) int g_scaleLine[32];
DATA(0x006aac08)
static unsigned char g_viewArtifacts;
DATA(0x006aac14)
static unsigned char g_viewTowns;
DATA(0x006aac18) int g_vwCenterOffsetW;
DATA(0x006aac1c) int g_vwCenterOffsetH;
DATA(0x006aac20) CSprite* g_csVwIcons;
DATA(0x006aac28) Bitmap16Bit* g_memoryBuffer;
DATA(0x006aac30)
static unsigned char g_viewHeroes;

// E:\gamedcs\viewwrld.cpp:100
// The magic-number float-to-int conversion. Retail emits NO body for it:
// the Dreamcast row is `static`, TViewWorldWindow::init's scale-table loop
// is its only call site, and VC6 expands a single-call-site static without
// retaining it (the surrounding carve rows leave no ~98 B slot for one).
// Dreamcast names the local `const unsigned long magic`; its 0x59c00000 bits
// are read as a float. Retail materialises those bits on the stack and loads
// the float before adding the double argument. A named double temporary gives
// init's inlined copy the retail stack layout and exact VC6 bytes.

static long ftol(double d)
{
    const unsigned long magic = 0x59c00000;
    double adjusted = d;
    adjusted = *reinterpret_cast<const float*>(&magic) + adjusted;
    return *reinterpret_cast<long*>(&adjusted);
}

// E:\gamedcs\viewwrld.cpp:110
// The view-world symbol blit, shared by all five VWDrawSymbols arms.
// Retail owns a body at 0x5f73b0 that the Dreamcast roster order maps onto
// exactly: VWDrawSprite, the two hero-part rows, the two boat-part rows and
// VWDrawSymbols occupy six consecutive carve rows between two functions this
// file already claims, and the SH4:x86 size ratios (320:333, 844:1015 twice,
// 424:481 twice) hold across the whole run.

// Two facts here are the retail bytes' and not the Dreamcast's. The 32/2
// float pool constants and the .data 0x68c6b8 tile scale give the centering
// offset through __ftol, and the DESTINATION passed to CSprite::Draw is the
// UNCLIPPED origin: the four clip statements trim the source rectangle and
// leave a separate copy of the origin untouched, which retail proves by
// keeping x and y live in ESI/EDI across the whole clip block and pushing
// those, never the clamped copies.
// The reviewed Mac body retains adjusted x/y separately and updates frame
// before the draw; that source shape aligns 99/107 Mac instructions at -O3.
// Its left clip computes width as 32 - tilex, while Windows retail emits
// baseX + 24. Substituting the former here changed Windows control flow and
// lowered its byte match, so the retail expression remains below.
VA(0x005f73b0, 0x14D)  // exhaustive dc-order-map inside the VWDrawAdvObj bracket, dc 0x192f4c
void vwDrawSprite(CSprite* srcIcon, NewmapCell* thisCell, int frame, int x, int y, int z)
{
    int offset = (32.0f - g_viewWorldScaleFloat) / 2.0f;
    int drawY = y - offset;
    int drawX = x - offset;

    int tilex = 0;
    int tiley = 0;
    int tilew = 32;
    int tileh = 32;
    int baseX = drawX;
    int baseY = drawY;

    if (baseX < 8) {
        tilex = 8 - baseX;
        tilew = baseX + 24;
        baseX = 8;
    }
    if (baseY < 0) {
        tiley = -baseY;
        tileh = baseY + 32;
        baseY = 0;
    }
    if (baseX + tilew > 600)
        tilew = 600 - baseX;
    if (baseY + tileh > 544)
        tileh = 544 - baseY;
    if (tilew <= 0 || tileh <= 0)
        return;

    int owner = -1;
    if (thisCell->m_type == HERO)
        owner = g_game->getHero(thisCell->m_extraInfo)->m_owner;
    else if (hasFlag(thisCell->m_type))
        owner = getFlaggedObjectOwner(thisCell);

    if (owner >= 0)
        frame += owner * 19;
    else
        frame += 8 * 19;

    srcIcon->draw(0, frame, tilex, tiley, tilew, tileh,
                  g_windowManager->m_screenBitmap, drawX, drawY, false, true);
}

inline void vwClipScaleToScreenBuffer(int destX, int destY)
{
    if (destX + g_viewWorldScale < 8 || destX >= 600)
        return;
    if (destY + g_viewWorldScale < 8 || destY >= 552)
        return;

    int mwidth = g_memoryBuffer->getWidth();
    int swidth = g_windowManager->m_screenBitmap->getWidth();

    int screenX = destX;
    int screenY = destY;
    if (screenX < 8)
        screenX = 8;
    else if (screenX > 600)
        screenX = 600;
    if (screenY < 8)
        screenY = 8;
    else if (screenY > 552)
        screenY = 552;

    unsigned short* screenBufferLineStart =
        g_windowManager->m_screenBitmap->getMap(screenX, screenY);
    unsigned short* sourceBufferLineStart = g_memoryBuffer->getMap(0, 0);

    for (int y = 0; y < g_viewWorldScale; ++y) {
        if (destY + y < 8 || destY + y >= 552)
            continue;

        unsigned short* screenBuffer = screenBufferLineStart;
        for (int x = 0; x < g_viewWorldScale; ++x) {
            if (destX + x >= 8 && destX + x < 600) {
                unsigned short* sourcePixel =
                    sourceBufferLineStart + g_scaleLine[x];
                if (*sourcePixel)
                    *screenBuffer = *sourcePixel;
                ++screenBuffer;
            }
        }
        // Keep the row offset before the buffer lookup, as in the expanded
        // river, road and object-shadow callers.
        int sourceLine = mwidth * g_scaleLine[y];
        sourceBufferLineStart =
            g_memoryBuffer->getMap(0, 0) + sourceLine;
        screenBufferLineStart += swidth;
    }
}

// E:\gamedcs\viewwrld.cpp:226
// This is the caller-facing half of the same source boundary. Retail expands
// it into VWDrawAdvObj, including the nested clipped helper above; keeping the
// real helpers visible lets VC6 make that decision without a synthetic gate.
inline void vwScaleToScreenBuffer(int destX, int destY)
{
    if (destX < 8 || destX + g_viewWorldScale >= 600
        || destY < 8 || destY + g_viewWorldScale >= 552) {
        vwClipScaleToScreenBuffer(destX, destY);
        return;
    }

    int mwidth = g_memoryBuffer->getWidth();
    int swidth = g_windowManager->m_screenBitmap->getWidth();
    unsigned short* screenBufferLineStart =
        g_windowManager->m_screenBitmap->getMap(destX, destY);
    unsigned short* sourceBufferLineStart = g_memoryBuffer->getMap(0, 0);

    for (int y = 0; y < g_viewWorldScale; ++y) {
        unsigned short* screenBuffer = screenBufferLineStart;
        for (int x = 0; x < g_viewWorldScale; ++x) {
            unsigned short* sourcePixel = sourceBufferLineStart + g_scaleLine[x];
            if (*sourcePixel)
                *screenBuffer = *sourcePixel;
            ++screenBuffer;
        }
        sourceBufferLineStart =
            g_memoryBuffer->getMap(0, 0) + mwidth * g_scaleLine[y];
        screenBufferLineStart += swidth;
    }
}

VA(0x005f7500, 0x3F7)  // dc 0x19308c
void advManager::vwDrawHeroPart(int part, TDrawParts& heroParts, int baseX, int baseY, int tilex, int tiley, int tilew, int tileh)
{
    hero* currHero = g_game->getHero(heroParts.m_id);

    int heroCellY = part % 3;
    int heroCellX = part / 3;

    if (currHero->m_flags & 0x40000) {
        boat* currBoat = g_game->getHeroBoat(currHero->m_id, true);
        NewmapCell* heroCell = getCell(currHero->getLocation());

        if (!(heroCell->m_flags0011 & 0x200)) {
            m_boatFrothIcons[currBoat->m_type]->drawHero(
                currHero->getStandSequence(),
                m_animCtr
                    % m_boatFrothIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
                tilex + (2 - heroCellY) * 32,
                tiley - heroCellX * 32 + 32, tilew, tileh,
                g_memoryBuffer, 0, 0,
                currHero->getHflip());
        }

        m_boatFlagIcons[currBoat->m_type][currBoat->m_playerOwner]->drawHero(
            currHero->getStandSequence(),
            m_animCtr % m_boatFlagIcons[currBoat->m_type][currBoat->m_playerOwner]
                                ->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_memoryBuffer, 0, 0,
            currHero->getHflip());

        m_boatIcons[currBoat->m_type]->drawHero(
            currHero->getStandSequence(),
            m_animCtr
                % m_boatIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_memoryBuffer, 0, 0,
            currHero->getHflip());
    } else {
        m_flagIcons[currHero->m_owner]->drawHero(
            currHero->getStandSequence(),
            m_animCtr % m_flagIcons[currHero->m_owner]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_memoryBuffer, 0, 0,
            currHero->getHflip());

        m_cursorIcons[currHero->m_heroClass]->drawHero(
            currHero->getStandSequence(),
            m_animCtr
                % m_cursorIcons[currHero->m_heroClass]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_memoryBuffer, 0, 0,
            currHero->getHflip());
    }
}

VA(0x005f7900, 0x3F7)  // dc 0x1933d8
void advManager::vwDrawHeroPartShadow(int part, TDrawParts& heroParts, int baseX, int baseY, int tilex, int tiley, int tilew, int tileh)
{
    hero* currHero = g_game->getHero(heroParts.m_id);

    int heroCellY = part % 3;
    int heroCellX = part / 3;

    if (currHero->m_flags & 0x40000) {
        boat* currBoat = g_game->getHeroBoat(currHero->m_id, true);
        NewmapCell* heroCell = getCell(currHero->getLocation());

        if (!(heroCell->m_flags0011 & 0x200)) {
            m_boatFrothIcons[currBoat->m_type]->drawHeroShadow(
                currHero->getStandSequence(),
                m_animCtr
                    % m_boatFrothIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
                tilex + (2 - heroCellY) * 32,
                tiley - heroCellX * 32 + 32, tilew, tileh,
                g_memoryBuffer, 0, 0,
                currHero->getHflip());
        }

        m_boatFlagIcons[currBoat->m_type][currBoat->m_playerOwner]->drawHeroShadow(
            currHero->getStandSequence(),
            m_animCtr % m_boatFlagIcons[currBoat->m_type][currBoat->m_playerOwner]
                                ->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_memoryBuffer, 0, 0,
            currHero->getHflip());

        m_boatIcons[currBoat->m_type]->drawHeroShadow(
            currHero->getStandSequence(),
            m_animCtr
                % m_boatIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_memoryBuffer, 0, 0,
            currHero->getHflip());
    } else {
        m_flagIcons[currHero->m_owner]->drawHeroShadow(
            currHero->getStandSequence(),
            m_animCtr % m_flagIcons[currHero->m_owner]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_memoryBuffer, 0, 0,
            currHero->getHflip());

        m_cursorIcons[currHero->m_heroClass]->drawHeroShadow(
            currHero->getStandSequence(),
            m_animCtr
                % m_cursorIcons[currHero->m_heroClass]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_memoryBuffer, 0, 0,
            currHero->getHflip());
    }
}

VA(0x005f7d00, 0x1E1)  // dc 0x193724
void advManager::vwDrawBoatPart(int part, TDrawParts& boatParts, int baseX, int baseY, int tilex, int tiley, int tilew, int tileh)
{
    boat* currBoat = &g_game->m_boats[boatParts.m_id];
    int boatCellY = part % 3;
    int boatCellX = part / 3;
    NewmapCell* boatCell = getCell(
        type_point(currBoat->m_x, currBoat->m_y, currBoat->m_z));

    if (!(boatCell->m_flags0011 & 0x200)) {
        m_boatFrothIcons[currBoat->m_type]->drawHero(
            currBoat->getStandSequence(),
            m_animCtr
                % m_boatFrothIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
            tilex + (2 - boatCellY) * 32,
            tiley - boatCellX * 32 + 32, tilew, tileh,
            g_memoryBuffer, 0, 0,
            currBoat->getHflip());
    }

    m_boatIcons[currBoat->m_type]->drawHero(
        currBoat->getStandSequence(),
        m_animCtr % m_boatIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
        tilex + (2 - boatCellY) * 32,
        tiley - boatCellX * 32 + 32, tilew, tileh,
        g_memoryBuffer, 0, 0,
        currBoat->getHflip());
}

VA(0x005f7ef0, 0x1E1)  // dc 0x1938cc
void advManager::vwDrawBoatPartShadow(int part, TDrawParts& boatParts, int baseX, int baseY, int tilex, int tiley, int tilew, int tileh)
{
    boat* currBoat = &g_game->m_boats[boatParts.m_id];
    int boatCellY = part % 3;
    int boatCellX = part / 3;
    NewmapCell* boatCell = getCell(
        type_point(currBoat->m_x, currBoat->m_y, currBoat->m_z));

    if (!(boatCell->m_flags0011 & 0x200)) {
        m_boatFrothIcons[currBoat->m_type]->drawHeroShadow(
            currBoat->getStandSequence(),
            m_animCtr
                % m_boatFrothIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
            tilex + (2 - boatCellY) * 32,
            tiley - boatCellX * 32 + 32, tilew, tileh,
            g_memoryBuffer, 0, 0,
            currBoat->getHflip());
    }

    m_boatIcons[currBoat->m_type]->drawHeroShadow(
        currBoat->getStandSequence(),
        m_animCtr % m_boatIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
        tilex + (2 - boatCellY) * 32,
        tiley - boatCellX * 32 + 32, tilew, tileh,
        g_memoryBuffer, 0, 0,
        currBoat->getHflip());
}

// The dispatch is a jump table, so the emitted arm order IS the source case
// order.
VA(0x005f80e0, 0x1F6)  // dc 0x193a74
void advManager::vwDrawSymbols(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    NewmapCell* thisCell = getCell(type_point(srcX, srcY, z));

    if (!thisCell->m_isTrigger)
        return;

    unsigned char explored =
        (getMapExtra(srcX, srcY, z) & g_mapVisibilityBit) != 0;

    int baseX = destX * g_viewWorldScale + g_vwCenterOffsetW;
    int baseY = destY * g_viewWorldScale + g_vwCenterOffsetH;

    switch (thisCell->m_type) {
    case ARTIFACT:
    case RANDOM_ARTIFACT:
    case RANDOM_ARTIFACT_1:
    case RANDOM_ARTIFACT_2:
    case RANDOM_ARTIFACT_3:
    case RANDOM_ARTIFACT_4:
        if (g_viewArtifacts || explored)
            vwDrawSprite(g_csVwIcons, thisCell, 2, baseX, baseY + 8, z);
        break;
    case HERO:
    case RANDOM_HERO:
        if (g_viewHeroes || explored)
            vwDrawSprite(g_csVwIcons, thisCell, 1, baseX, baseY + 8, z);
        break;
    case MINE:
        if (g_viewMines || explored)
            vwDrawSprite(g_csVwIcons, thisCell, thisCell->m_objectIndex + 5,
                         baseX, baseY + 8, z);
        break;
    case RANDOM_RESOURCE:
    case RESOURCE:
        if (g_viewResources || explored)
            vwDrawSprite(g_csVwIcons, thisCell, thisCell->m_objectIndex + 12,
                         baseX, baseY + 8, z);
        break;
    case RANDOM_TOWN:
    case TOWN:
        if (g_viewTowns || explored)
            vwDrawSprite(g_csVwIcons, thisCell, 0, baseX, baseY + 8, z);
        break;
    }
}

// Dreamcast supplies the original helper boundaries, locals, scope nesting
// and statement order. Complete's retail body independently corroborates the
// same six-layer object walk, separate hero/boat part scopes, flagged-object
// path, cursor cases, and final scaled-buffer helper expansion.
VA(0x005f82e0, 0x8F1)  // link order + signature/callee/CFG corroboration, dc 0x193c74
void advManager::vwDrawAdvObj(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    NewmapCell* thisCell = getCell(type_point(srcX, srcY, z));
    int playerBit = 1 << g_game->getLocalPlayerGamePos();
    playerBit &= getMapExtra(srcX, srcY, z);

    int baseX = destX * g_viewWorldScale + g_vwCenterOffsetW,
        baseY = destY * g_viewWorldScale + g_vwCenterOffsetH;

    TDrawParts heroParts[6];
    TDrawParts boatParts[6];
    unsigned char foundHero =
        scanForHeroOrBoat(srcX, srcY, z, HERO, heroParts);
    unsigned char foundBoat =
        scanForHeroOrBoat(srcX, srcY, z, BOAT, boatParts);

    memset(g_memoryBuffer->getMap(0, 0), 0,
           g_memoryBuffer->getHeight() * g_memoryBuffer->getPitch());
    int drewSomething = 0;

    if (thisCell->m_objects.size() > 0) {
        for (unsigned row = 0; row < 6; ++row) {
            for (int numObj = 0; numObj < thisCell->m_objects.size();
                 ++numObj) {
                NewmapCell::TObjectCell* objCell =
                    &thisCell->m_objects[numObj];
                if (objCell->m_layer != row)
                    continue;

                CObjectType* objType =
                    &m_fullMap->m_objectTypes[
                        m_fullMap->m_objects[objCell->m_objectIndex].m_typeIndex];
                CSprite* sprPtr = m_fullMap->m_sprites[
                    m_fullMap->m_objects[objCell->m_objectIndex].m_typeIndex];
                // Dreamcast line 620 performs this third CObject lookup as a
                // standalone source statement. Its value is optimized out of
                // Complete, but the statement is part of the original shape.
                unsigned short objectTypeIndex =
                    m_fullMap->m_objects[objCell->m_objectIndex].m_typeIndex;

                if (!playerBit
                    && (!g_vwTerrains
                        || !g_adventureObjectTraits[objType->m_objectType].m_trait3))
                    continue;

                if (!objType->m_drawCells[
                        CObjectType::getBitPos(objCell->m_cellX,
                                                objCell->m_cellY)]
                    || objType->m_suppressDraw)
                    continue;

                drewSomething = 1;
                if (hasFlag(objType->m_objectType)) {
                    int triggerX;
                    int triggerY;
                    m_fullMap->m_objects[objCell->m_objectIndex].findTrigger(
                        triggerX, triggerY);
                    int owner = getFlaggedObjectOwner(
                        getCell(type_point(triggerX, triggerY, z)));

                    sprPtr->drawAdvObjWithFlag(
                        (m_animCtr
                         + m_fullMap->m_objects[objCell->m_objectIndex]
                               .m_animationOffset)
                            % sprPtr->getNumFrames(0),
                        (objType->m_width - objCell->m_cellX - 1) * 32,
                        (objType->m_height - objCell->m_cellY - 1) * 32,
                        32, 32, g_memoryBuffer, 0, 0,
                        g_systemPalette->m_data[64 + owner], false);
                } else {
                    sprPtr->drawAdvObj(
                        (m_animCtr
                         + m_fullMap->m_objects[objCell->m_objectIndex]
                               .m_animationOffset)
                            % sprPtr->getNumFrames(0),
                        (objType->m_width - objCell->m_cellX - 1) * 32,
                        (objType->m_height - objCell->m_cellY - 1) * 32,
                        32, 32, g_memoryBuffer, 0, 0, false);
                }
            }

            if (foundHero && playerBit) {
                int part;
                int partHigh;
                int partLow;
                if (row == OBJECT_DRAW_LAYER_HERO_FRONT) {
                    partLow = 0;
                    partHigh = 2;
                } else if (row == OBJECT_DRAW_LAYER_HERO_BACK) {
                    partLow = 3;
                    partHigh = 5;
                } else {
                    continue;
                }

                for (part = partLow; part <= partHigh; ++part) {
                    if (heroParts[part].m_isValid) {
                        drewSomething = 1;
                        vwDrawHeroPart(part, heroParts[part], baseX, baseY,
                                       0, 0, 32, 32);
                    }
                }
            }

            if (foundBoat && playerBit) {
                int boatPart;
                int boatPartHigh;
                int boatPartLow;
                if (row == OBJECT_DRAW_LAYER_HERO_FRONT) {
                    boatPartLow = 0;
                    boatPartHigh = 2;
                } else if (row == OBJECT_DRAW_LAYER_HERO_BACK) {
                    boatPartLow = 3;
                    boatPartHigh = 5;
                } else {
                    continue;
                }

                for (boatPart = boatPartLow; boatPart <= boatPartHigh;
                     ++boatPart) {
                    if (boatParts[boatPart].m_isValid) {
                        drewSomething = 1;
                        vwDrawBoatPart(boatPart, boatParts[boatPart], baseX, baseY,
                                       0, 0, 32, 32);
                    }
                }
            }

            if (row == OBJECT_DRAW_LAYER_HERO_BACK
                && destY == CURSOR_DEST_Y0
                && this->m_drawCursor && !::g_drawingPuzzle) {
                if (destX == CURSOR_DEST_X0) {
                    this->drawCursor(0, 0);
                } else if (destX == CURSOR_DEST_X1) {
                    this->drawCursor(1, 0);
                } else if (destX == CURSOR_DEST_X2) {
                    this->drawCursor(2, 0);
                }
            } else if (row == OBJECT_DRAW_LAYER_HERO_FRONT
                       && destY == CURSOR_DEST_Y1
                       && this->m_drawCursor && !::g_drawingPuzzle) {
                if (destX == CURSOR_DEST_X0) {
                    this->drawCursor(0, 1);
                } else if (destX == CURSOR_DEST_X1) {
                    this->drawCursor(1, 1);
                } else if (destX == CURSOR_DEST_X2) {
                    this->drawCursor(2, 1);
                }
            }
        }
    } else {
        if (foundHero && playerBit) {
            int fallbackHeroPart;
            for (fallbackHeroPart = 0; fallbackHeroPart <= 5;
                 ++fallbackHeroPart) {
                if (heroParts[fallbackHeroPart].m_isValid) {
                    drewSomething = 1;
                    vwDrawHeroPart(fallbackHeroPart,
                                   heroParts[fallbackHeroPart],
                                   baseX, baseY, 0, 0, 32, 32);
                }
            }
        }

        if (foundBoat && playerBit) {
            int fallbackBoatPart;
            for (fallbackBoatPart = 0; fallbackBoatPart <= 5;
                 ++fallbackBoatPart) {
                if (boatParts[fallbackBoatPart].m_isValid) {
                    drewSomething = 1;
                    vwDrawBoatPart(fallbackBoatPart,
                                   boatParts[fallbackBoatPart],
                                   baseX, baseY, 0, 0, 32, 32);
                }
            }
        }

        if (destY == CURSOR_DEST_Y0 && this->m_drawCursor && playerBit) {
            if (destX == CURSOR_DEST_X0) {
                this->drawCursor(0, 0);
            } else if (destX == CURSOR_DEST_X1) {
                this->drawCursor(1, 0);
            } else if (destX == CURSOR_DEST_X2) {
                this->drawCursor(2, 0);
            }
        } else if (destY == CURSOR_DEST_Y1 && this->m_drawCursor && playerBit) {
            if (destX == CURSOR_DEST_X0) {
                this->drawCursor(0, 1);
            } else if (destX == CURSOR_DEST_X1) {
                this->drawCursor(1, 1);
            } else if (destX == CURSOR_DEST_X2) {
                this->drawCursor(2, 1);
            }
        }
    }

    if (drewSomething)
        vwScaleToScreenBuffer(baseX, baseY + 8);
}

// E:\gamedcs\viewwrld.cpp:822
// The scaled shadow layer. It is VWDrawAdvObj without the six-row draw-layer
// split: one flat pass over the cell's objects behind the same two filters,
// then the cursor shadows, then the hero and boat part shadows. advmgr.cpp's
// DrawAdvObjShadow supplies the cursor block verbatim with `playerBit` where
// the full-size renderer tests gbInViewWorld, and retail corroborates the
// whole shape - the two TDrawParts arrays cleared six entries at a time, the
// two ScanForHeroOrBoat calls, the drawCells/suppressDraw pair, and the
// `part <= 5` bound on both part loops.
// Residual (87.06%): `baseY` is DECLARED FIRST here - retail loads both
// iVWCenterOffsetW and iVWCenterOffsetH before either imul and multiplies
// destY first (`mov esi,eax / imul eax,[ebp+0x18] / imul esi,[ebp+0x14]`),
// which the baseX-first comma declaration cannot produce; the swap alone is
// worth 83.9809 -> 87.0563 and also fixes the prologue (retail loads srcX
// before `push ebx`) and srcX's register. MEASURED AND REJECTED at this
// plateau: splitting playerBit into VWDrawAdvObj's two-statement `1 << pos`
// then `&= GetMapExtra` form (byte-flat, 87.0563); the same baseY-first swap
// in VWDrawAdvObj itself (byte-flat, 96.4955). What is left is srcX/srcY:
// retail RE-READS both parameter homes for the inlined type_point
// (`mov ecx,[ebp+8] / mov esi,[ebp+0xc]` between the two bitfield words)
// where we still hold them in registers across the guard, plus the one
// under-inlined `memoryBuffer->GetMap(0,0)` inside VWScaleToScreenBuffer's
// row loop, which retail folds to `mov ebx,[memoryBuffer] / mov ebx,[ebx+0x30]`.
// THAT UNDER-INLINE IS NOW PRICED EXACTLY (`predict-inline --trace`): this body
// has cb 977, so budget 1954; by the VWScaleToScreenBuffer site it is down to
// 1383, giving the nested pool (1383-241)/1 = 1142 and then
// (1142-356)/6 = 131 at VWClipScaleToScreenBuffer's depth. Its three GetMap
// expansions cost 45 each and the third is refused with 41 left - short by 4.
// Any of these closes it: this body's cb >= 989 (+12), the depth-1 spend before
// the scale call 24 lower, one fewer call site in VWScaleToScreenBuffer
// (divisor 6 -> 5 gives 157), or VWClipScaleToScreenBuffer's cb <= 332.
// Measured and byte-flat: merging the two clip guards, plain-`if` clamps,
// splitting the entry guard into two or four ifs, and naming the type_point
// local. Measured worse: caching GetMap(0,0) in a local across the row loop
// (unit 96.76 -> 95.77 - retail reloads it), and dropping the clamp upper
// bounds (this body +1.36, unit -2.03).
VA(0x005f8be0, 0x636)  // exhaustive dc-order-map + VWCompleteDraw call order (5th layer), dc 0x1943ec
void advManager::vwDrawAdvObjShadow(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    NewmapCell* thisCell = getCell(type_point(srcX, srcY, z));
    int playerBit = (1 << g_game->getLocalPlayerGamePos())
        & getMapExtra(srcX, srcY, z);

    int baseY = destY * g_viewWorldScale + g_vwCenterOffsetH,
        baseX = destX * g_viewWorldScale + g_vwCenterOffsetW;

    TDrawParts heroParts[6];
    TDrawParts boatParts[6];
    unsigned char foundHero =
        scanForHeroOrBoat(srcX, srcY, z, HERO, heroParts);
    unsigned char foundBoat =
        scanForHeroOrBoat(srcX, srcY, z, BOAT, boatParts);

    int drewSomething = 0;
    memset(g_memoryBuffer->getMap(0, 0), 0,
           g_memoryBuffer->getHeight() * g_memoryBuffer->getPitch());

    for (int numObj = 0; numObj < thisCell->m_objects.size(); ++numObj) {
        NewmapCell::TObjectCell* objCell = &thisCell->m_objects[numObj];

        CObjectType* objType =
            &m_fullMap->m_objectTypes[
                m_fullMap->m_objects[objCell->m_objectIndex].m_typeIndex];
        CSprite* sprPtr = m_fullMap->m_sprites[
            m_fullMap->m_objects[objCell->m_objectIndex].m_typeIndex];

        if (!playerBit
            && (!g_vwTerrains
                || !g_adventureObjectTraits[objType->m_objectType].m_trait3))
            continue;

        if (!objType->m_drawCells[
                CObjectType::getBitPos(objCell->m_cellX, objCell->m_cellY)]
            || objType->m_suppressDraw)
            continue;

        drewSomething = 1;
        sprPtr->drawAdvObjShadow(
            (m_animCtr
             + m_fullMap->m_objects[objCell->m_objectIndex].m_animationOffset)
                % sprPtr->getNumFrames(0),
            (objType->m_width - objCell->m_cellX - 1) * 32,
            (objType->m_height - objCell->m_cellY - 1) * 32,
            32, 32, g_memoryBuffer, 0, 0, false);
    }

    if (destY == CURSOR_DEST_Y0) {
        if (m_drawCursor && playerBit) {
            if (destX == CURSOR_DEST_X0)
                drawCursorShadow(0, 0);
            else if (destX == CURSOR_DEST_X1)
                drawCursorShadow(1, 0);
            else if (destX == CURSOR_DEST_X2)
                drawCursorShadow(2, 0);
        }
    } else if (destY == CURSOR_DEST_Y1) {
        if (m_drawCursor && playerBit) {
            if (destX == CURSOR_DEST_X0)
                drawCursorShadow(0, 1);
            else if (destX == CURSOR_DEST_X1)
                drawCursorShadow(1, 1);
            else if (destX == CURSOR_DEST_X2)
                drawCursorShadow(2, 1);
        }
    }

    if (foundHero && playerBit) {
        for (int part = 0; part <= 5; ++part) {
            if (heroParts[part].m_isValid) {
                drewSomething = 1;
                vwDrawHeroPartShadow(part, heroParts[part], baseX, baseY,
                                     0, 0, 32, 32);
            }
        }
    }

    if (foundBoat && playerBit) {
        for (int boatPart = 0; boatPart <= 5; ++boatPart) {
            if (boatParts[boatPart].m_isValid) {
                drewSomething = 1;
                vwDrawBoatPartShadow(boatPart, boatParts[boatPart],
                                     baseX, baseY, 0, 0, 32, 32);
            }
        }
    }

    if (drewSomething)
        vwScaleToScreenBuffer(baseX, baseY + 8);
}

// E:\gamedcs\viewwrld.cpp:946
// The scaled river layer. Dreamcast supplies the statement order, the two
// guards and the single tile blit; Complete's retail body corroborates every
// element independently - the same four bounds, the GetMapExtra/iVWTerrains
// disjunction, the RiverSet test through the bitfield unit, the scaled origin
// pair, the inlined clear of the scratch buffer and the DrawTile through
// riverTileset. advmgr.cpp's full-size DrawRiver is the unscaled twin.
VA(0x005f9220, 0x38A)  // exhaustive dc-order-map + VWCompleteDraw call order (2nd layer), dc 0x194850
void advManager::vwDrawRiver(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    NewmapCell* thisCell = getCell(type_point(srcX, srcY, z));

    int playerBit = 1 << g_game->getLocalPlayerGamePos();

    if (!(playerBit & getMapExtra(srcX, srcY, z)) && !g_vwTerrains)
        return;

    if (!thisCell->m_riverSet)
        return;

    int baseX = destX * g_viewWorldScale + g_vwCenterOffsetW;
    int baseY = destY * g_viewWorldScale + g_vwCenterOffsetH;

    memset(g_memoryBuffer->getMap(0, 0), 0,
           g_memoryBuffer->getHeight() * g_memoryBuffer->getPitch());

    m_riverTileset[thisCell->m_riverSet]->drawTile(
        thisCell->m_riverIndex, 0, 0, 32, 32, g_memoryBuffer, 0, 0,
        (thisCell->m_flags0011 >> 2) & 1,
        (thisCell->m_flags0011 >> 3) & 1);

    vwScaleToScreenBuffer(baseX, baseY + 8);
}

// E:\gamedcs\viewwrld.cpp:985
// The scaled road layer - the river layer's twin, differing only in the
// tested bitfield (RoadSet, byte-aligned at the head of its own allocation
// unit), the tileset array and the two flip bits (4 and 5 instead of 2 and
// 3). Retail's identical instruction stream either side of those four
// differences is what pairs the two.
// Residual (98.2455%, MAX 99.5374%): all 45 blocks, 31 branches, seven calls
// and 281 instructions align; the 14 unpaired masked slots are scratch-order
// choices in the two expanded scaler row latches. The twice-used sourcePixel
// pointer remains the source-true spelling that established MAX. Reversing
// `mwidth * g_scaleLine[y]` independently in the clipped and unclipped
// helpers is byte-flat, while why-reg's six guided local/declaration probes
// are all worse. Keep the canonical shared helpers and their exact retained
// clipped body rather than forcing this caller's register assignment.
VA(0x005f95b0, 0x38B)  // exhaustive dc-order-map + VWCompleteDraw call order (3rd layer), dc 0x1949cc
void advManager::vwDrawRoad(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    NewmapCell* thisCell = getCell(type_point(srcX, srcY, z));

    int playerBit = 1 << g_game->getLocalPlayerGamePos();

    if (!(playerBit & getMapExtra(srcX, srcY, z)) && !g_vwTerrains)
        return;

    if (!thisCell->m_roadSet)
        return;

    int baseX = destX * g_viewWorldScale + g_vwCenterOffsetW;
    int baseY = destY * g_viewWorldScale + g_vwCenterOffsetH;

    memset(g_memoryBuffer->getMap(0, 0), 0,
           g_memoryBuffer->getHeight() * g_memoryBuffer->getPitch());

    m_roadTileset[thisCell->m_roadSet]->drawTile(
        thisCell->m_roadIndex, 0, 0, 32, 32, g_memoryBuffer, 0, 0,
        (thisCell->m_flags0011 >> 4) & 1,
        (thisCell->m_flags0011 >> 5) & 1);

    vwScaleToScreenBuffer(baseX, baseY + 8);
}

// E:\gamedcs\viewwrld.cpp:1026
// The scaled fog layer. advmgr.cpp's exact DrawShroud (0x412220) supplies
// the whole cloud/star decision - the same gCompleteDrawAllCells bypass, the
// same GetCloudLookup, the same >=100 flip offset and the two frame fixups -
// and its `goto draw_stars` idiom is the spelling that reproduces retail's
// block layout here too. Two things are this body's own: the shroud decision
// is LATCHED in a flag rather than returned on, so the fog tile is reached
// through a join the non-visible path jumps straight into, and the leading
// GetCell result is discarded exactly as the full-size renderer discards its
// own type_point.
// The star arm is a POSITIVE `if (!lookup) { ...; return; }` block, not a
// forward `goto adjust_cloud` over it. Both spellings describe the same CFG
// - 54 blocks, 38 branches, 3 returns either way - but the goto leaves the
// star block with two jump predecessors and VC6 SINKS it past the fog draw,
// which cost 70.6 points (20.5312 -> 91.0938 on the inversion alone). With
// the guard positive, B12 falls into the star block and every block lands
// where retail put it: 49 of 54 exact, 0 target-shift, 0 flow-kind.
// Residual (92.8835%): the size-only blocks left are inside the two
// VWScaleToScreenBuffer expansions, not this body - one extra out-of-line
// GetMap in the star expansion, which is a per-site /Ob2 decision inside
// the helper and not a statement of this function.
VA(0x005f9940, 0x44A)  // exhaustive dc-order-map + VWCompleteDraw call order (the iVWTerrains-gated layer), dc 0x194b48
void advManager::vwDrawShroud(int srcX, int srcY, int z, int destX, int destY)
{
    // Keep the two shroud-selection scopes and their common draw checks.
    // An else arm removes the reconstructed join label at unchanged 97.9205%.
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth)
        return;
    if (srcY >= g_mapHeight && !g_completeDrawAllCells)
        return;

    getCell(type_point(srcX, srcY, z));

    int playerBit = 1 << g_game->getLocalPlayerGamePos();

    if (!(playerBit & getMapExtra(srcX, srcY, z)) && !g_vwTerrains)
        return;

    int baseX = destX * g_viewWorldScale + g_vwCenterOffsetW;
    int baseY = destY * g_viewWorldScale + g_vwCenterOffsetH;

    int lookup = 0;
    unsigned char hflip = false;
    unsigned char drawShroud;

    if (!g_completeDrawAllCells
        && ((getMapExtra(srcX, srcY, z) & g_mapVisibilityBit)
            || g_drawingPuzzle)) {
        drawShroud = false;
    } else {
        drawShroud = true;
        if (!g_completeDrawAllCells)
            lookup = getCloudLookup(srcX, srcY, z);
        if (!lookup) {
            memset(g_memoryBuffer->getMap(0, 0), 0,
                   g_memoryBuffer->getHeight() * g_memoryBuffer->getPitch());
            m_starTileset->drawShroudTile(
                ((srcX * 85 ^ srcY * 85) / 64) & 3, 0, 0, 32, 32, g_memoryBuffer,
                0, 0, false, false);
            vwScaleToScreenBuffer(baseX, baseY + 8);
            return;
        }

        if (lookup >= CLOUD_DRAW_FLIPPED_OFFSET) {
            hflip = true;
            lookup -= CLOUD_DRAW_FLIPPED_OFFSET;
        }
        if ((lookup == CLOUD_DRAW_FRAME_1 || lookup == CLOUD_DRAW_FRAME_5)
            && (srcX & 1))
            ++lookup;
        if (lookup == CLOUD_DRAW_FRAME_3 && (srcY & 1))
            lookup = CLOUD_DRAW_FRAME_4;
    }

    if (g_drawingPuzzle)
        return;
    if (!drawShroud)
        return;

    memset(g_memoryBuffer->getMap(0, 0), 0,
           g_memoryBuffer->getHeight() * g_memoryBuffer->getPitch());
    m_cloudIcons->drawShroudTile(
        lookup - 1, 0, 0, 32, 32, g_memoryBuffer, 0, 0, hflip, false);
    vwScaleToScreenBuffer(baseX, baseY + 8);
}

VA(0x005f9d90, 0x13C)  // dc 0x1968d0
void vwClipScaleToScreenBuffer(int destX, int destY);

VA(0x005f9ed0, 0x310)  // dc 0x194dcc
void advManager::vwDrawUnderlay(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    NewmapCell* thisCell = getCell(type_point(srcX, srcY, z));

    int playerBit = 1 << g_game->getLocalPlayerGamePos();

    if (!(playerBit & getMapExtra(srcX, srcY, z)) && !g_vwTerrains)
        return;

    int baseX = destX * g_viewWorldScale + g_vwCenterOffsetW;
    int baseY = destY * g_viewWorldScale + g_vwCenterOffsetH;

    memset(g_memoryBuffer->getMap(0, 0), 0,
           g_memoryBuffer->getHeight() * g_memoryBuffer->getPitch());

    int drewSomething = 0;

    if (thisCell->m_objects.size() > 0) {
        for (int numObj = 0; numObj < thisCell->m_objects.size(); ++numObj) {
            NewmapCell::TObjectCell* objCell = &thisCell->m_objects[numObj];

            CObjectType* objType =
                &m_fullMap->m_objectTypes[
                    m_fullMap->m_objects[objCell->m_objectIndex].m_typeIndex];
            CSprite* sprPtr = m_fullMap->m_sprites[
                m_fullMap->m_objects[objCell->m_objectIndex].m_typeIndex];

            if (objType->m_suppressDraw) {
                drewSomething = 1;

                sprPtr->drawAdvObj(
                    (m_animCtr
                     + m_fullMap->m_objects[objCell->m_objectIndex]
                           .m_animationOffset)
                        % sprPtr->getNumFrames(0),
                    (objType->m_width - objCell->m_cellX - 1) * 32,
                    (objType->m_height - objCell->m_cellY - 1) * 32,
                    32, 32, g_memoryBuffer, 0, 0, false);
            }
        }
    }

    if (drewSomething)
        vwScaleToScreenBuffer(baseX, baseY + 8);
}

VA(0x005fa1e0, 0x41F)  // dc 0x194fb0
void advManager::vwDrawGround(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    NewmapCell* thisCell = getCell(type_point(srcX, srcY, z));

    int playerBit = 1 << g_game->getLocalPlayerGamePos();

    if (!(playerBit & getMapExtra(srcX, srcY, z)) && !g_vwTerrains)
        return;

    int baseX = destX * g_viewWorldScale + g_vwCenterOffsetW;
    int baseY = destY * g_viewWorldScale + g_vwCenterOffsetH;

    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth
        || srcY >= g_mapHeight) {
        int frame = -1;
        if (srcX == -1) {
            if (srcY == -1)
                frame = 16;
            else if (srcY == g_mapHeight)
                frame = 19;
            else if (srcY >= 0 && srcY < g_mapHeight)
                frame = 32 + (srcY & 3);
        } else if (srcX == g_mapWidth) {
            if (srcY == -1)
                frame = 17;
            else if (srcY == g_mapHeight)
                frame = 18;
            else if (srcY >= 0 && srcY < g_mapHeight)
                frame = 24 + (srcY & 3);
        } else if (srcY == -1) {
            if (srcX >= 0 && srcX < g_mapWidth)
                frame = 20 + (srcX & 3);
        } else if (srcY == g_mapHeight) {
            if (srcX >= 0 && srcX < g_mapHeight)
                frame = 28 + (srcX & 3);
        }

        if (frame == -1)
            frame = (srcX + 16) % 4 + 4 * ((srcY + 16) % 4);

        memset(g_memoryBuffer->getMap(0, 0), 0,
               g_memoryBuffer->getHeight() * g_memoryBuffer->getPitch());

        m_borderTileset->drawTile(
            frame, 0, 0, 32, 32, g_memoryBuffer, 0, 0, false, false);

        vwScaleToScreenBuffer(baseX, baseY + 8);
    } else {
        memset(g_memoryBuffer->getMap(0, 0), 0,
               g_memoryBuffer->getHeight() * g_memoryBuffer->getPitch());

        m_groundTileset[thisCell->m_groundSet]->drawTile(
            thisCell->m_groundIndex, 0, 0, 32, 32, g_memoryBuffer, 0, 0,
            thisCell->m_flags0011 & 1,
            (thisCell->m_flags0011 >> 1) & 1);

        vwScaleToScreenBuffer(baseX, baseY + 8);
    }
}

// E:\gamedcs\viewwrld.cpp:1307
// The shared source shape is Dreamcast's 41-line constructor dossier: popup
// base, explicit window bounds, vector reserve, ordered widget construction,
// AddWidget/MemError sweep, and the final MESSAGE_WIDGET broadcast. Complete
// expands the five icon rows to nineteen, the five text rows to fourteen, and
// adds the two address-taken surface/underground controls. Retail corroborates
// every revised row through the 45 operator-new EH states and constructor-call
// stream. The ViewWorld caller's adjacent stack local proves sizeof(*this)=0x78.
// Residual (96.4425%) PRICED AS CALLER MASS, polish 16, and the direction is
// DOWN.  The visible defect is an UNDER-inline: at the first two widget sites
// retail expands `vector<widget*>::push_back` into its `insert(_Last, 1, x)`
// call (`push edx / push 1 / push eax`) where this compile CALLS the
// out-of-line `push_back` COMDAT with one argument, which is the /Ob2
// `budget / sites-remaining` quotient starving the EARLIEST sites. The
// textbook fix is to grow caller_cb, and it does not work here: an `if (0)`
// carrier after `Widgets.reserve(NWIDGETS)` - the measuring instrument, not a
// fix, and deliberately not shipped - gives
//   N = 1,2,3,5 -> 96.30 | 10 -> 96.09 | 20 -> 90.59 | 30 -> 90.31 |
//   40 -> 84.95
// monotone down with no plateau anywhere, so more mass over-inlines faster
// than it recovers push_back. The lever this body wants is the numerator from
// the other side (SHRINK the caller), and the Dreamcast roster names no helper
// to lift these blocks into, so it is out of reach without invented source.
VA(0x005fa600, 0x1726)  // caller stack extent + vtable 0x643c54, dc 0x1952b8
TViewWorldWindow::TViewWorldWindow()
    : CAdvPopup(0, 0, 800, 600, 0)
{
    m_x = 0;
    m_y = 0;
    m_width = 800;
    m_height = 600;

    m_widgets.reserve(NWIDGETS);

    m_widgets.push_back(new bitmapBorder(
        607, 195, 190, 381, 14, "VWorld.pcx", 0x800));
    m_widgets.push_back(new border(630, 26, 144, 144, 20, 1));
    m_widgets.push_back(new textWidget(
        608, 194, 188, 49, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD), "bigfont.fnt",
        font::HEADING, 15, 1, 0, 8));

    int firstFrame = g_game->getLocalPlayerGamePos() * 19;
    m_widgets.push_back(new iconWidget(
        612, 254, 32, 32, 21, "VWsymbol.def", firstFrame, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 274, 32, 32, 21, "VWsymbol.def", firstFrame + 1, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 294, 32, 32, 21, "VWsymbol.def", firstFrame + 2, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 314, 32, 32, 21, "VWsymbol.def", firstFrame + 3, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 334, 32, 32, 21, "VWsymbol.def", firstFrame + 4, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 378, 32, 32, 21, "VWsymbol.def", firstFrame + 5, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 398, 32, 32, 21, "VWsymbol.def", firstFrame + 6, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 418, 32, 32, 21, "VWsymbol.def", firstFrame + 7, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 438, 32, 32, 21, "VWsymbol.def", firstFrame + 8, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 458, 32, 32, 21, "VWsymbol.def", firstFrame + 9, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 478, 32, 32, 21, "VWsymbol.def", firstFrame + 10, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        612, 498, 32, 32, 21, "VWsymbol.def", firstFrame + 11, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        761, 378, 32, 32, 21, "VWsymbol.def", firstFrame + 12, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        761, 398, 32, 32, 21, "VWsymbol.def", firstFrame + 13, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        761, 418, 32, 32, 21, "VWsymbol.def", firstFrame + 14, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        761, 438, 32, 32, 21, "VWsymbol.def", firstFrame + 15, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        761, 458, 32, 32, 21, "VWsymbol.def", firstFrame + 16, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        761, 478, 32, 32, 21, "VWsymbol.def", firstFrame + 17, 0, 0, 0, 16));
    m_widgets.push_back(new iconWidget(
        761, 498, 32, 32, 21, "VWsymbol.def", firstFrame + 18, 0, 0, 0, 16));

    m_widgets.push_back(new textWidget(
        650, 260, 130, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_TOWN), "Calli10R.fnt",
        font::PRIMARY, 0, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        650, 280, 130, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_HERO), "Calli10R.fnt",
        font::PRIMARY, 1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        650, 300, 130, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_ARTIFACT), "Calli10R.fnt",
        font::PRIMARY, 2, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        650, 320, 130, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_TELEPORTER), "Calli10R.fnt",
        font::PRIMARY, 3, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        650, 340, 130, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_GATE), "Calli10R.fnt",
        font::PRIMARY, 4, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        614, 368, 60, 18, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_MINE), "Calli10R.fnt",
        font::PRIMARY, 12, 0, 0, 8));
    m_widgets.push_back(new textWidget(
        722, 368, 70, 18, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_RESOURCE), "Calli10R.fnt",
        font::PRIMARY, 13, 2, 0, 8));
    m_widgets.push_back(new textWidget(
        648, 384, 120, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_WOOD), "Calli10R.fnt",
        font::PRIMARY, 6, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        648, 404, 120, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_MERCURY), "Calli10R.fnt",
        font::PRIMARY, 7, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        648, 424, 120, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_ORE), "Calli10R.fnt",
        font::PRIMARY, 8, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        648, 444, 120, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_SULFUR), "Calli10R.fnt",
        font::PRIMARY, 9, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        648, 464, 120, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_CRYSTAL), "Calli10R.fnt",
        font::PRIMARY, 10, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        648, 484, 120, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_GEMS), "Calli10R.fnt",
        font::PRIMARY, 11, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        648, 504, 120, 20, g_generalText->getText(GENERAL_TEXT_VIEW_WORLD_GOLD), "Calli10R.fnt",
        font::PRIMARY, 5, 5, 0, 8));

    m_widgets.push_back(new button(
        608, 218, 60, 32, 16, "VWMag1.def", 0, 1, 0, 2, 2));
    m_widgets.push_back(new button(
        671, 218, 60, 32, 17, "VWMag2.def", 0, 1, 0, 3, 2));
    m_widgets.push_back(new button(
        735, 218, 60, 32, 18, "VWMag4.def", 0, 1, 0, 5, 2));

    m_widgets.push_back(new bitmapBorder(
        611, 537, 68, 34, -1, "box66x32.pcx", 0x800));
    // RETAIL BOUNDARY: TViewWorldWindow::TViewWorldWindow ->
    // vector<widget*>::insert. Dreamcast 0x1952b8 proves the ordered widget
    // append family; Complete adds this puzzle row and retail retains its
    // insert call at ctor+0xfd0. The old 88.42% flattening control belonged
    // to an earlier source state: removing this pin now improves the ctor
    // from 97.1358% to 97.3652% with no tracked-TU losses (2026-09-09).
    m_widgets.push_back(new button(
        612, 538, 66, 32, 19, "VWPuz.def", 0, 1, 0, 25, 2));

    m_undergroundButton = new type_func_button(
        686, 538, 32, 32, -1, "iam010.def",
        viewWorldUndergroundHandler, 0, 1);
    m_surfaceButton = new type_func_button(
        686, 538, 32, 32, -1, "iam003.def",
        viewWorldSurfaceHandler, 0, 1);
    // The level controls are Complete-only; Dreamcast 0x1952b8 proves the
    // surrounding append family uses push_back. The two widget::hide calls
    // below are the real header-helper sites that select retail's frontier:
    // the first append retains vector::insert and the second expands it.
    m_widgets.push_back(m_undergroundButton);
    m_widgets.push_back(m_surfaceButton);

    // DC lines 1370 and 1373 prove these are push_back calls too. Together
    // with both DC Widget.h::hide helpers below, the natural source reproduces
    // the complete 215-block retail constructor exactly; no depth pin or
    // direct vector::insert spelling is needed.
    m_widgets.push_back(new bitmapBorder(
        725, 537, 68, 34, -1, "box66x32.pcx", 0x800));
    button* ok = new button(
        726, 538, 66, 32, 0x7802, "iOkay32.def", 0, 1, 0, 1, 2);
    ok->setHotkey(28);
    m_widgets.push_back(ok);

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeY = 17;
    msg.m_codeX = 5;
    msg.m_extra = 16;
    broadcastMessage(msg);

    m_undergroundButton->sendMessage(
        widget::WIDGET_SET_PLAYER_PALETTE_COLORS,
        g_game->getLocalPlayerGamePos());
    m_surfaceButton->sendMessage(
        widget::WIDGET_SET_PLAYER_PALETTE_COLORS,
        g_game->getLocalPlayerGamePos());

    if (m_origin.m_z == 1 || g_game->getNumMapLevels() == 1)
        m_undergroundButton->hide();
    if (m_origin.m_z == 0)
        m_surfaceButton->hide();
}

VA_COMPGEN(0x005fbd30, 0x21, SCALAR_DELETING_DTOR, TViewWorldWindow)

// The two retained vector<widget*>::insert overloads at 0x5fd390 and
// 0x5fdd60 both call this guarded four-byte fill loop. viewwrld.obj emits
// the same specialization from the recovered widget-vector operations.
VA_COMPGEN(0x005fdf20, 0x26, VECTOR_UFILL, widget)

VA(0x005fbd60, 0x86)  // dc 0x195ac4
TViewWorldWindow::~TViewWorldWindow()
{
    delete g_memoryBuffer;
    g_csVwIcons->dispose();

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// The type_func_button click code both callbacks answer, the same 13
// TViewArmyWindow's cast-spell callback tests (viewarmywindow.cpp).
static const int g_levelButtonClick = 13;

// Complete-only address-taken callback. The constructor passes this entry to
// the iAm003 surface button; the body clears origin.z and redraws the map.
// Both level callbacks answer a left click (codeX 13 without the RIGHT
// modifier) only: swap which of the two level buttons is pressed, redraw
// the released one, repaint the world and the radar, and flip the
// whole screen.
VA(0x005fbdf0, 0xC6)
int viewWorldSurfaceHandler(message& msg)
{
    if (msg.m_codeX != g_levelButtonClick
        || (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT))
        return 0;
    TViewWorldWindow* window = static_cast<TViewWorldWindow*>(msg.m_window);
    window->m_origin.m_z = 0;
    window->m_surfaceButton->sendMessage(widget::WIDGET_CLEAR_STATUS, 6);
    window->m_undergroundButton->sendMessage(widget::WIDGET_SET_STATUS, 6);
    window->m_undergroundButton->draw();
    window->drawWindow();
    g_advManager->updateRadar(window->m_origin, 1, 1, g_viewMines, g_viewHeroes,
                              g_viewTowns);
    g_windowManager->updateScreen(0, 0, 800, 600);
    return 1;
}

// Complete-only address-taken callback. The constructor passes this entry to
// the iAm010 underground button; the body sets origin.z and redraws the map.
VA(0x005fbec0, 0xD0)
int viewWorldUndergroundHandler(message& msg)
{
    if (msg.m_codeX != g_levelButtonClick
        || (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT))
        return 0;
    TViewWorldWindow* window = static_cast<TViewWorldWindow*>(msg.m_window);
    window->m_origin.m_z = 1;
    window->m_surfaceButton->sendMessage(widget::WIDGET_SET_STATUS, 6);
    window->m_undergroundButton->sendMessage(widget::WIDGET_CLEAR_STATUS, 6);
    window->m_surfaceButton->draw();
    window->drawWindow();
    g_advManager->updateRadar(window->m_origin, 1, 1, g_viewMines, g_viewHeroes,
                              g_viewTowns);
    g_windowManager->updateScreen(0, 0, 800, 600);
    return 1;
}

VA(0x005fbf90, 0x2A3)  // dc 0x195b48
void advManager::viewWorld(int whatToDraw, TSkillMastery level)
{
    g_inViewWorld = 1;
    g_viewArtifacts = 0;
    g_viewTowns = 0;
    g_viewHeroes = 0;
    g_viewResources = 0;
    g_vwTerrains = 0;
    g_viewMines = 0;

    switch (whatToDraw) {
    case SPELL_VIEW_EARTH:
        switch (level) {
        case eMasteryAdvanced:
            g_viewMines = 1;
            g_viewResources = 1;
            break;
        case eMasteryExpert:
            g_vwTerrains = 1;
            g_viewMines = 1;
            g_viewResources = 1;
            break;
        default:
            g_viewResources = 1;
            break;
        }
        break;
    case SPELL_VIEW_AIR:
        switch (level) {
        case eMasteryAdvanced:
            g_viewHeroes = 1;
            g_viewArtifacts = 1;
            break;
        case eMasteryExpert:
            g_viewTowns = 1;
            g_viewHeroes = 1;
            g_viewArtifacts = 1;
            break;
        default:
            g_viewArtifacts = 1;
            break;
        }
        break;
    }

    g_viewWorldScaleFloat = VIEW_WORLD_TILE_SCALE_MID;
    g_viewWorldScale = 11;
    g_csVwIcons = ResourceManager::getSprite("VWsymbol.def");
    g_memoryBuffer = new Bitmap16Bit(64, 64);
    g_advManager->demobilizeCurrHero(0, 1);
    g_windowManager->m_colorCyclingOn = 0;
    g_combatActive = 2;
    {
        TViewWorldWindow viewWorldWindow;
        type_point mapCenter(m_radarOrigin.m_x + 9, m_radarOrigin.m_y + 8,
                              m_radarOrigin.m_z);

        viewWorldWindow.init(mapCenter, 0);
        viewWorldWindow.drawWindow();
        g_windowManager->m_colorCyclingOn = 1;
        viewWorldWindow.doModal(0);
    }
    g_inViewWorld = 0;
    updateRadar(0, 1, g_viewMines, g_viewHeroes, g_viewTowns);
    g_windowManager->m_colorCyclingOn = 0;
    redrawAdvScreen(1, 0);
    g_combatActive = 0;
    g_windowManager->m_colorCyclingOn = 1;
}

// E:\gamedcs\viewwrld.cpp:1496
// Recentres the view-world map on new_center and repaints the radar.
// Every division by two here is retail's bare `sar reg,1` - an arithmetic
// shift, not the cdq/sub pair a signed `/ 2` compiles to - so the source
// wrote `>> 1`. The scale table is the one call site of the file's `ftol`,
// and the two clamped extents are compared against MAP_WIDTH/MAP_HEIGHT
// for EQUALITY: a map that exactly fills the view keeps the origin at
// (0, 0, new_center.z) with no clamping at all.
// iSkipLevel's DECLARATION SITE is worth 13.5 points (83.37 -> 96.87):
// retail issues the `fdiv` after both integer divisions, so the float
// local is declared BELOW the two extent assignments, not above them.
// The inlined ftol helper's double temporary occupies retail [ebp-8], with
// skipLevel at [ebp-0xc] and its magic constant at [ebp-0x10]. The earlier
// union spelling allocated four extra bytes and held this body at 98.8939%.
VA(0x005fc240, 0x274)  // anchor-caller ViewWorld, anchor-callee UpdateRadar, dc 0x195d30
void TViewWorldWindow::init(type_point newCenter, unsigned char updateFlag)
{
    int i;

    m_viewableWidth = 608 / g_viewWorldScale;
    m_viewableHeight = 544 / g_viewWorldScale;
    float skipLevel = 32.0f / g_viewWorldScaleFloat;
    for (i = 0; i < g_viewWorldScale; i++)
        g_scaleLine[i] = ftol(static_cast<float>(i) * skipLevel);

    if (m_viewableWidth > g_mapWidth)
        m_viewableWidth = g_mapWidth;
    if (m_viewableHeight > g_mapHeight)
        m_viewableHeight = g_mapHeight;
    g_viewHalfWidth = m_viewableWidth >> 1;
    g_viewHalfHeight = m_viewableHeight >> 1;

    m_origin.m_y = 0;
    m_origin.m_x = 0;
    m_origin.m_z = newCenter.m_z;
    if (m_viewableWidth != g_mapWidth) {
        m_origin.m_x = newCenter.m_x - (m_viewableWidth >> 1);
        if (m_origin.m_x < 0)
            m_origin.m_x = 0;
        if (m_origin.m_x + m_viewableWidth >= g_mapWidth)
            m_origin.m_x = g_mapWidth - m_viewableWidth;
    }
    if (m_viewableHeight != g_mapHeight) {
        m_origin.m_y = newCenter.m_y - (m_viewableHeight >> 1);
        if (m_origin.m_y < 0)
            m_origin.m_y = 0;
        if (m_origin.m_y + m_viewableHeight >= g_mapHeight)
            m_origin.m_y = g_mapHeight - m_viewableHeight;
    }

    g_vwCenterOffsetW = (608 - m_viewableWidth * g_viewWorldScale) >> 1;
    g_vwCenterOffsetH = (544 - m_viewableHeight * g_viewWorldScale) >> 1;
    g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
    g_advManager->updateRadar(m_origin, updateFlag, 1, g_viewMines, g_viewHeroes,
                              g_viewTowns);
    if (g_game->getNumMapLevels() > 1) {
        if (m_origin.m_z == 1) {
            m_undergroundButton->sendMessage(widget::WIDGET_CLEAR_STATUS, 6);
            m_surfaceButton->sendMessage(widget::WIDGET_SET_STATUS, 6);
        } else {
            m_surfaceButton->sendMessage(widget::WIDGET_CLEAR_STATUS, 6);
            m_undergroundButton->sendMessage(widget::WIDGET_SET_STATUS, 6);
        }
    }
}

// E:\gamedcs\viewwrld.cpp:1549, dc 0x195ffc. This ordinary method's
// only source operation is the five-argument adventure repaint. Complete
// expands this method at some call sites; the Mac build retains six calls
// across the level callbacks, viewWorld, updateViewWorld, updateRadar and
// the puzzle path in the window handler.
void TViewWorldWindow::drawWindow()
{
    g_advManager->vwCompleteDraw(m_origin.m_x, m_origin.m_y, m_origin.m_z,
                                m_viewableWidth, m_viewableHeight);
}

VA(0x005fc4c0, 0x2E0)  // dc 0x196040
void advManager::vwCompleteDraw(int startX, int startY, int z, int drawwidth,
                                int drawheight)
{
    int row;
    int col;

    g_windowManager->m_screenBitmap->fillRect(8, 8, 592, 544, 0);
    for (row = -1; row <= drawheight; row++) {
        for (col = -1; col <= drawwidth; col++)
            vwDrawGround(startX + col, startY + row, z, col, row);
    }
    for (row = -1; row <= drawheight; row++) {
        for (col = -1; col <= drawwidth; col++)
            vwDrawRiver(startX + col, startY + row, z, col, row);
    }
    for (row = -1; row <= drawheight; row++) {
        for (col = -1; col <= drawwidth; col++)
            vwDrawRoad(startX + col, startY + row, z, col, row);
    }
    for (row = -1; row <= drawheight; row++) {
        for (col = -1; col <= drawwidth; col++)
            vwDrawUnderlay(startX + col, startY + row, z, col, row);
    }
    for (row = -1; row <= drawheight; row++) {
        for (col = -1; col <= drawwidth; col++)
            vwDrawAdvObjShadow(startX + col, startY + row, z, col, row);
    }
    for (row = -1; row <= drawheight; row++) {
        for (col = -1; col <= drawwidth; col++)
            vwDrawAdvObj(startX + col, startY + row, z, col, row);
    }
    if (!g_vwTerrains) {
        for (row = -1; row <= drawheight; row++) {
            for (col = -1; col <= drawwidth; col++)
                vwDrawShroud(startX + col, startY + row, z, col, row);
        }
    }
    for (row = -1; row <= drawheight; row++) {
        for (col = -1; col <= drawwidth; col++)
            vwDrawSymbols(startX + col, startY + row, z, col, row);
    }
    drawAdventureMapGems();
}

VA(0x005fc7a0, 0x147)  // dc 0x196228
void TViewWorldWindow::updateViewWorld(message* msg)
{
    message msg2;
    int i;

    for (i = 0; i < 3; i++) {
        msg2.m_id = MESSAGE_WIDGET;
        msg2.m_codeY = i + 16;
        msg2.m_codeX = 6;
        msg2.m_extra = 16;
        broadcastMessage(msg2);
    }
    msg2.m_id = MESSAGE_WIDGET;
    msg2.m_codeY = msg->m_codeY;
    msg2.m_codeX = 5;
    msg2.m_extra = 16;
    broadcastMessage(msg2);

    type_point center(m_origin.m_x + g_viewHalfWidth,
                      m_origin.m_y + g_viewHalfHeight, m_origin.m_z);

    init(center, 1);
    drawWindow();
    drawWindow(1, 0xffff0001, 0xffff);
    g_windowManager->updateScreen(0, 0, 800, 600);
}

VA(0x005fc8f0, 0x213)  // dc 0x1962fc
void TViewWorldWindow::updateRadar(int mrx, int mry, float radarDivisor)
{
    widget* radar = g_advManager->m_advWindow->m_radarWidget;
    int rx = radar->m_x;
    int ry = radar->m_y;
    int rw = radar->m_width;
    int rh = radar->m_height;

    if (mrx < rx)
        mrx = rx;
    if (mrx >= rx + rw)
        mrx = rx + g_mapWidth * 2 - 1;
    if (mry < ry)
        mry = ry;
    if (mry >= ry + rh)
        mry = ry + g_mapHeight * 2 - 1;

    m_origin.m_x = static_cast<int>((mrx - rx) / radarDivisor) - g_viewHalfWidth;
    m_origin.m_y = static_cast<int>((mry - ry) / radarDivisor) - g_viewHalfHeight;
    m_origin.m_x = max(static_cast<int>(m_origin.m_x), 0);
    m_origin.m_y = max(static_cast<int>(m_origin.m_y), 0);
    m_origin.m_x = min(static_cast<int>(m_origin.m_x),
                        g_mapWidth - m_viewableWidth);
    m_origin.m_y = min(static_cast<int>(m_origin.m_y),
                        g_mapHeight - m_viewableHeight);

    g_advManager->updateRadar(m_origin, 1, 1, g_viewMines, g_viewHeroes,
                              g_viewTowns);
    drawWindow();
    g_windowManager->updateScreen(8, 8, 592, 544);
}

// E:\gamedcs\viewwrld.cpp:1710
// Both dispatch levels are the compiler's own tells. The message-id test
// is an IF-CHAIN (`cmp`, arms emitted in place); the two codeX tests and
// the widget-id test are SWITCHES (`sub`/`dec` descent, arms sunk behind
// the default), and the map-dimension divisor is a switch too - its three
// arms sit after the 1.0f default in reverse-ascending order, which no
// if-chain produces. The accept arm and the key arm share their last two
// statements, which is why retail duplicates only the dialogReturn store.
// The radar drag is a pump: hold the button, keep the LAST mouse-move
// seen, and re-centre once per outer pass until the button comes up.
VA(0x005fcb10, 0x37F)  // vtable slot 9 + anchor-callee update_view_world/update_radar, dc 0x1964dc
int TViewWorldWindow::windowHandler(message& msg)
{
    message rMsg;
    message rSaveMsg;
    type_point center;
    float radarDivisor;
    int handled;

    handled = CAdvPopup::windowHandler(msg);
    if (handled)
        return handled;

    if (!g_soundManager->musicPlaying())
        g_soundManager->switchAmbientMusic(
            g_terrainMusicIds[g_advManager->m_lastTerrain]);

    if (msg.m_id == MESSAGE_KEY_DOWN) {
        switch (msg.m_codeX) {
        case KEYCODE_ESCAPE:
        case KEYCODE_ENTER:
            g_windowManager->m_dialogReturn = msg.m_codeY;
            msg.m_codeX = msg.m_codeY = widget::WIDGET_END_DIALOG;
            return MESSAGE_DISPATCH_FORWARD;
        }
    } else if (msg.m_id == MESSAGE_WIDGET) {
        switch (msg.m_codeX) {
        case widget::WIDGET_SELECT:
            if (msg.m_codeY != RADAR_ID)
                break;
            if (m_viewableWidth == g_mapWidth && m_viewableHeight == g_mapHeight)
                break;
            switch (g_mapHeight) {
            case MAP_DIMENSION_SMALL:
                radarDivisor = 4.0f;
                break;
            case MAP_DIMENSION_MEDIUM:
                radarDivisor = 2.0f;
                break;
            case MAP_DIMENSION_LARGE:
                radarDivisor = 1.3333f;
                break;
            default:
                radarDivisor = 1.0f;
                break;
            }
            updateRadar(msg.m_mouseX, msg.m_mouseY, radarDivisor);
            do {
                process1WindowsMessage();
                rSaveMsg = rMsg = g_inputManager->getEvent();
                while (rMsg.m_id != MESSAGE_LEFT_BUTTON_UP
                       && rMsg.m_id != MESSAGE_NONE) {
                    if (rMsg.m_id == MESSAGE_MOUSE_MOVE)
                        rSaveMsg = rMsg;
                    process1WindowsMessage();
                    rMsg = g_inputManager->getEvent();
                }
                if (rSaveMsg.m_id == MESSAGE_MOUSE_MOVE)
                    updateRadar(rSaveMsg.m_codeX, rSaveMsg.m_codeY,
                                 radarDivisor);
            } while (rMsg.m_id != MESSAGE_LEFT_BUTTON_UP);
            break;
        case widget::WIDGET_DESELECT:
            switch (msg.m_codeY) {
            case MAGNIFY_FAR_ID:
                g_viewWorldScaleFloat = VIEW_WORLD_TILE_SCALE_FAR;
                g_viewWorldScale = 7;
                updateViewWorld(&msg);
                return MESSAGE_DISPATCH_CONSUME;
            case MAGNIFY_MID_ID:
                g_viewWorldScaleFloat = VIEW_WORLD_TILE_SCALE_MID;
                g_viewWorldScale = 11;
                updateViewWorld(&msg);
                return MESSAGE_DISPATCH_CONSUME;
            case MAGNIFY_FULL_ID:
                g_viewWorldScaleFloat = VIEW_WORLD_TILE_SCALE_FULL;
                g_viewWorldScale = 16;
                updateViewWorld(&msg);
                return MESSAGE_DISPATCH_CONSUME;
            case PUZZLE_ID:
                g_windowManager->fadeScreen(1, 4, 0);
                g_advManager->viewPuzzle();
                g_windowManager->fadeScreen(1, 4, 0);
                g_advManager->redrawAdvScreen(0, 0);
                drawWindow(0, 0xffff0001, 0xffff);
                center = type_point(m_origin.m_x + g_viewHalfWidth,
                                    m_origin.m_y + g_viewHalfHeight, m_origin.m_z);
                init(center, 0);
                drawWindow();
                g_windowManager->updateScreen(0, 0, 800, 600);
                return MESSAGE_DISPATCH_CONSUME;
            case ACCEPT_ID:
                g_windowManager->m_dialogReturn = msg.m_codeY;
                msg.m_codeX = msg.m_codeY = widget::WIDGET_END_DIALOG;
                return MESSAGE_DISPATCH_FORWARD;
            }
            break;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}
