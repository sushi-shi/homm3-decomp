// mapcell.cpp - E:\gamedcs\mapcell.cpp (compiland mapcell.obj)
// The ten-bit mask CObjectType carries at +0x34. Its own gate: only this
// TU constructs a CObjectType, and this header's include closure is
// measured.
// SPAN AUDIT 2026-08-19, updated 2026-08-22 - the small compiler-generated
// rows inside this compiland's claimed span (0x4fbf90..0x505b20) are:
//   0x4fd1c0 24 B   header-inline ctor COMDAT (char + three zeroed dwords)
//   0x4fca60 62 B   implicit TreasureData destructor - now claimed exact
//   0x4fd460 88 B   `??_ENewmapCell` vector deleting destructor - now
//                   claimed exact
//   0x504260 46 B   excluded .CRT$XCU initializer for the file-scope
//                   vector<int> InvalidPlacementList at 0x699690
//   0x504290 42 B   its registered static destructor - now claimed exact
//   0x5042c0 421 B  retail-only per-class object-type-index rebuild - now
//                   claimed at its 99.8854 scheduling/pooled-symbol wall
// The two larger rows (0x500de0, 0x502b60) are real game code with NO
// Dreamcast counterpart: the DC roster for mapcell.cpp is exhausted (90
// rows, every name present in this file bar two $E thunks), and 0x502b60 in
// particular sits between readGarrisonData (dc line 3229) and readObject (dc
// line 3290) where the roster has nothing at all. They require retail body
// evidence, not an order-map. Both are now exact: 0x500de0 is
// LoadShipyards, and 0x502b60 is SoD_transformRandomDwellings, which walks
// the 16-byte-element vector at NewfullMap+0xc0, builds a `generator`, and
// resolves each entry to a town alignment either by finding a matching
// record in gpGame's 136-byte-stride pool at +0x98 or by calling
// pick_alignment.

#include <stdio.h>
#include <string.h>
#include <va.h>
#include <windows.h>
#include "advmgr_objects.h"
#include "csprite.h"
#include "advmgr.h"
#include "game.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "monsterdata.h"
#include "newgame.h"
#include "resourcemanager.h"
#include "smackmgr.h"
#include <stdexcept>

static int getTeam(game* thisGame, int playerNum)
{
    if (playerNum < 0)
        return playerNum;
    return thisGame->m_mapHeader.m_teamInfo[playerNum];
}

// E:\gamedcs\mapcell.cpp:1119. Dreamcast retains this source helper as an
// out-of-line SH4 body; Complete expands it into every admitted retail use.
inline type_point CObject::getTrigger() const
{
    int resultX;
    int resultY;
    findTrigger(resultX, resultY);
    return type_point(resultX, resultY, m_z);
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:46
#endif  // @carcass

VA(0x004fbf90, 0x61)  // dc 0xeb6a4
void ExtraInfoUnion::setCellVisited(short player)
{
    if (player < 0 || player >= 8)
        return;

    int team = getTeam(g_game, player);

    for (int i = 0; i < 8; ++i) {
        if (g_game->m_mapHeader.m_teamInfo[i] == team)
            m_cellVisitedInfo.m_visited |= 1 << i;
    }
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:62
#endif  // @carcass

VA(0x004fc000, 0x19A)  // dc 0xeb73c
int NewfullMap::readTimedEventList(TAbstractFile* infile, int saveVersion)
{
    int count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_timedEventList.resize(count);
    for (unsigned int i = 0; i < m_timedEventList.size(); ++i) {
        if (m_timedEventList[i].read(infile, saveVersion) < 0)
            return -1;
    }
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:85
#endif  // @carcass

// The timed-event record.  Field order is fixed independently by Save
// (0x4fc440, exact): message, seven resource deltas, the player mask, the
// two apply-to flags, then the first-occurrence day and the repeat interval.

// Three details are retail's and none of them is tidy.  The record carries
// TWO strings and the first is read into a throwaway - `readMapString` runs
// once against a local and once against Message, which is why this body owns
// a std::string it never reads and why every error path below the first read
// releases it.  The first-occurrence day is incremented after reading
// (`inc word ptr [ebx]`), so the stream stores it zero-based.  And the record
// ends with sixteen discarded bytes.

// The apply-to-human flag is version-gated at save version 28, the same
// boundary game::LoadGarrisonPool uses for its removable-units flag; below it
// the flag is forced on rather than read.
// DC locals prove int count, string throwAway, and char padding[16]. The
// read/guard pairs and failure scopes are separate at mapcell.cpp:93..120.
// The 24-state family emitted four reproduced code results: recovered count
// placement, char padding and error braces keep 99.4737%. Removing the old
// cleanup pin gives 72.9210% in every corresponding control. That retained
// site still calls the string destructor where retail calls _Tidy(true);
// default depth expands the child too, so this boundary remains unresolved.
VA(0x004fc1a0, 0x1EE)  // order-map: callers readTimedEventList + readTownData (inlined TTownEvent::Read), calls readString 0x4c6010; EH-bearing, dc 0xeb7d0
int TTimedEvent::read(TAbstractFile* infile, int saveVersion)
{
    int count;
    std::string throwAway;
    NewSMapHeader::readString(infile, throwAway);
    NewSMapHeader::readString(infile, m_message);

    count = infile->read(m_resQty, sizeof(m_resQty));
    if (count < sizeof(m_resQty)) {
        return -1;
    }
    count = infile->read(&m_playerFlags, sizeof(m_playerFlags));
    if (count < sizeof(m_playerFlags)) {
#pragma inline_depth(0)
        return -1;
#pragma inline_depth()
    }

    if (saveVersion >= 28) {
        unsigned char value;
        infile->read(&value, sizeof(value));
        m_applyToHuman = value != 0;
    } else {
        m_applyToHuman = 1;
    }

    count = infile->read(&m_applyToComputer, sizeof(m_applyToComputer));
    if (count < sizeof(m_applyToComputer)) {
        return -1;
    }
    count = infile->read(&m_firstTime, sizeof(m_firstTime));
    if (count < sizeof(m_firstTime)) {
        return -1;
    }
    ++m_firstTime;
    count = infile->read(&m_interval, sizeof(m_interval));
    if (count < sizeof(m_interval)) {
        return -1;
    }

    char padding[16];
    count = infile->read(padding, sizeof(padding));
    if (count < sizeof(padding)) {
        return -1;
    }
    return 0;
}

VA(0x004fc390, 0xA5)  // dc 0xeb9a0
int NewfullMap::saveTimedEventList(TAbstractFile* outfile)
{
    int count = m_timedEventList.size();
    if (static_cast<unsigned>(outfile->write(&count, sizeof(count)))
        < sizeof(count))
        return -1;

    for (unsigned int i = 0; i < m_timedEventList.size(); ++i) {
        if (m_timedEventList[i].save(outfile) < 0)
            return -1;
    }
    return 0;
}

VA(0x004fc440, 0xB7)  // dc 0xeba38
int TTimedEvent::save(TAbstractFile* outfile)
{
    if (game::saveString(outfile, &m_message) < 0)
        return -1;
    if (static_cast<unsigned>(outfile->write(m_resQty, sizeof(m_resQty)))
        < sizeof(m_resQty))
        return -1;
    if (static_cast<unsigned>(outfile->write(&m_playerFlags, 1)) < 1)
        return -1;

    unsigned char count = m_applyToHuman;
    outfile->write(&count, 1);

    if (static_cast<unsigned>(outfile->write(&m_applyToComputer, 1)) < 1)
        return -1;
    if (static_cast<unsigned>(outfile->write(&m_firstTime, 2)) < 2)
        return -1;
    return static_cast<unsigned>(outfile->write(&m_interval, 2)) < 2 ? -1 : 0;
}

// E:\gamedcs\mapcell.cpp:179
#if 0  // @carcass -- located/reconstruction-pending bodies

#endif  // @carcass

VA(0x004fc500, 0x19A)  // dc 0xebb0c
int NewfullMap::loadTimedEventList(TAbstractFile* infile, int saveVersion)
{
    int count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_timedEventList.resize(count);
    for (unsigned int i = 0; i < m_timedEventList.size(); ++i) {
        if (m_timedEventList[i].load(infile, saveVersion) < 0)
            return -1;
    }
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

#endif  // @carcass -- located/reconstruction-pending bodies

VA(0x004fc6a0, 0xC8)  // dc 0xebbbc
int TTimedEvent::load(TAbstractFile* infile, int saveVersion)
{
    if (game::loadString(infile, m_message) < 0)
        return -1;
    if (static_cast<unsigned>(infile->read(m_resQty, sizeof(m_resQty)))
        < sizeof(m_resQty))
        return -1;
    if (static_cast<unsigned>(infile->read(&m_playerFlags, 1)) < 1)
        return -1;

    if (saveVersion >= 42) {
        unsigned char count;
        infile->read(&count, 1);
        m_applyToHuman = count != 0;
    } else {
        m_applyToHuman = 1;
    }

    if (static_cast<unsigned>(infile->read(&m_applyToComputer, 1)) < 1)
        return -1;
    if (static_cast<unsigned>(infile->read(&m_firstTime, 2)) < 2)
        return -1;
    return static_cast<unsigned>(infile->read(&m_interval, 2)) < 2 ? -1 : 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:232
DC_ONLY(0xebc90, 0x92)
int TTownEvent::read(void* infile)
{
    // @stub
}

#endif  // @carcass -- located/reconstruction-pending bodies

VA(0x004fc770, 0xFA)  // dc 0xebd24
int NewfullMap::saveTownEventList(TAbstractFile* outfile)
{
    int count = m_townEventList.size();
    if (static_cast<unsigned>(outfile->write(&count, sizeof(count)))
        < sizeof(count))
        return -1;

    for (unsigned int i = 0; i < m_townEventList.size(); ++i) {
        TTownEvent& thisEvent = m_townEventList[i];
        thisEvent.TTimedEvent::save(outfile);
        if (static_cast<unsigned>(outfile->write(&thisEvent.m_townNum, 1)) < 1)
            return -1;
        if (static_cast<unsigned>(outfile->write(&thisEvent.m_buildBuildings, 8)) < 8)
            return -1;
        if (static_cast<unsigned>(outfile->write(thisEvent.m_generatorBonuses, 14)) < 14)
            return -1;
    }
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:283
DC_ONLY(0xebdbc, 0x7E)
int TTownEvent::save(void* outfile)
{
    // @stub
}

#endif  // @carcass

VA(0x004fc870, 0x1E4)  // dc 0xebe3c
int NewfullMap::loadTownEventList(TAbstractFile* infile, int saveVersion)
{
    int count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_townEventList.resize(count);
    for (unsigned int i = 0; i < m_townEventList.size(); ++i) {
        TTownEvent& thisEvent = m_townEventList[i];
        thisEvent.TTimedEvent::load(infile, saveVersion);
        if (static_cast<unsigned>(infile->read(&thisEvent.m_townNum, 1)) < 1)
            return -1;
        if (static_cast<unsigned>(infile->read(&thisEvent.m_buildBuildings, 8)) < 8)
            return -1;
        if (static_cast<unsigned>(infile->read(thisEvent.m_generatorBonuses, 14)) < 14)
            return -1;
    }
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:329
DC_ONLY(0xebeec, 0x7E)
int TTownEvent::load(void* infile)
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\mapcell.cpp:354
// Dreamcast retains an out-of-line copy. Retail's corresponding source-order
// slot is twelve bytes of NOP padding, while is_diggable contains this exact
// vector lookup expanded in place, so the Windows helper is inline-only.
inline CObject* NewmapCell::TObjectCell::getObject() const
{
    return &g_game->m_worldMap.m_objects[m_objectIndex];
}

VA_COMPGEN(0x004fca60, 0x3E, IMPLICIT_DTOR, TreasureData)

VA(0x004fcaa0, 0x12F)  // dc 0xebf98
NewmapCell* NewmapCell::getTriggerCell()
{
    if (m_isTrigger)
        return this;

    if (m_type == NOTHING || m_type == ANCHOR_POINT || m_type == EVENT
        || m_type == HOLY_GRAIL)
        return 0;

    if (m_objectTypeIndex < 0
        || m_objectTypeIndex >= g_game->m_worldMap.m_objects.size())
        return 0;

    CObject* object = &g_game->m_worldMap.m_objects[m_objectTypeIndex];
    type_point location = object->getTrigger();

    if (location.m_x < 0)
        return 0;
    return g_game->getCell(location);
}

#if 0  // @carcass -- located/reconstruction-pending bodies
#endif  // @carcass

VA(0x004fcbd0, 0x5C)  // dc 0xec098
TAdventureObjectType NewmapCell::getMapObject()
{
    if (m_type == HERO) {
        hero* currentHero = g_game->getHero(m_extraInfo);
        return currentHero->getObscuredObject();
    }
    if (m_type == BOAT) {
        boat* currentBoat = g_game->getBoat(m_extraInfo);
        return currentBoat->getObscuredObject();
    }
    return m_type;
}

VA(0x004fcc30, 0x4D)  // dc 0xec114
unsigned long NewmapCell::getMapExtraInfo()
{
    if (m_type == HERO)
        return g_game->getHero(m_extraInfo)->getObscuredExtraInfo();
    if (m_type == BOAT)
        return g_game->m_boats[m_extraInfo].getObscuredExtraInfo();
    return m_extraInfo;
}

VA(0x004fcc80, 0x65)  // dc 0xec1c8
unsigned char NewmapCell::cellIsTrigger()
{
    if (m_type == HERO) {
        hero* obscurer = g_game->getHero(m_extraInfo);
        return obscurer->isOnMap() && obscurer->obscuredIsTrigger();
    }
    if (m_type == BOAT) {
        boat* obscurer = &g_game->m_boats[m_extraInfo];
        return obscurer->isOnMap() && obscurer->obscuredIsTrigger();
    }
    return m_isTrigger;
}

VA(0x004fccf0, 0xD0)  // dc 0xec254
unsigned char NewmapCell::isDiggable()
{
    if (m_groundSet == eTerrainWater || m_groundSet == eTerrainRock)
        return 0;
    if (!(m_flags0011 & 0x40))
        return 0;

    TAdventureObjectType objectType = getMapObject();
    if (objectType != ANCHOR_POINT) {
        if (objectType != HOLY_GRAIL && objectType != NOTHING)
            return 0;
    } else {
        for (long i = 0; i < m_objects.size(); ++i) {
            if (m_objects[i].getObject()->getType() == TERRAIN_HOLE)
                return 0;
        }
    }
    return 1;
}

VA(0x004fcdc0, 0x58)  // dc 0xec324
const unsigned char NewmapCell::hasTriggerableEvent()
{
    if (m_type == EVENT) {
        if (g_currentPlayer->isLocalHuman()
            && ((m_extraInfo >> 10) & g_mapVisibilityBit))
            return 1;

        if (!g_currentPlayer->isLocalHuman()
            && (m_extraInfo & 0x40000)
            && ((m_extraInfo >> 10) & g_mapVisibilityBit))
            return 1;
    }
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:490
#endif  // @carcass

VA(0x004fce20, 0x116)  // dc 0xec3b4
TAdventureObjectType NewmapCell::getSpecialTerrain() const
{
    if (m_type == HERO && (m_cellFlags & 0x1000)) {
        hero* ourHero = g_game->getHero(m_extraInfo);
        if (ourHero->getObscuredObject() == GARRISON
                && ourHero->obscuredIsTrigger()
                && m_objectIndex == 1)
            return GARRISON;
    }

    if (m_type == GARRISON
            && (m_cellFlags & 0x1000)
            && m_objectIndex == 1)
        return m_type;

    for (int i = m_objects.size(); i-- > 0;) {
        CObject* object = m_objects[i].getObject();
        CObjectType* objectType = object->getObjectTypePtr();
        if (objectType->m_objectType == CURSED_GROUND
                || objectType->m_objectType == MAGIC_PLAINS
                || objectType->m_objectType == HOLY_GROUND
                || objectType->m_objectType == EVIL_FOG
                || objectType->m_objectType == CLOVER_FIELD_2
                || objectType->m_objectType == FAVORABLE_WINDS
                || objectType->m_objectType == LUCID_POOLS
                || objectType->m_objectType == FIERY_FIELDS
                || objectType->m_objectType == ROCKLANDS
                || objectType->m_objectType == MAGIC_CLOUDS)
            return objectType->m_objectType;
    }
    return NOTHING;
}

VA(0x004fcf40, 0x112)  // hd-crossbuild, anchor-callee
int NewmapCell::getMagicTerrainType()
{
    switch (getSpecialTerrain()) {
    case MAGIC_PLAINS:
        return MAGIC_TERRAIN_MAGIC_PLAINS;
    case LUCID_POOLS:
        return MAGIC_TERRAIN_LUCID_POOLS;
    case FIERY_FIELDS:
        return MAGIC_TERRAIN_FIERY_FIELDS;
    case ROCKLANDS:
        return MAGIC_TERRAIN_ROCKLANDS;
    case MAGIC_CLOUDS:
        return MAGIC_TERRAIN_MAGIC_CLOUDS;
    default:
        return MAGIC_TERRAIN_INVALID;
    }
}

VA(0x004fd060, 0x15C)  // dc 0xec4fc
NewfullMap::NewfullMap()
    : m_cellData(0)
{
}

VA_COMPGEN(0x004fd1c0, 0x18, DEFAULT_CTOR_CLOSURE, CObjectType)

// E:\gamedcs\mapcell.cpp:530
// RECONSTRUCTED 2026-08-20. This is NewfullMap::Close's body inlined ahead
// of the fourteen member destructions, which is what the note under Close
// below already predicted.

// Three things, in retail's order:
//   * the cell array, freed through NewmapCell's `vector deleting
//     destructor' at 0x4fd460 with flags 3 and then nulled;
//   * every mapObjectData entry deleted through virtual slot zero (`push 1
//     / call [edx]`, the scalar deleting destructor - the null test in
//     front of it is `delete p`'s own, not a source guard), then the vector
//     cleared through the out-of-line vector<CMapObjectData*>::clear COMDAT
//     at 0x48b4a0 that Init already reaches;
//   * every sprite disposed through virtual slot ONE - CSprite::Dispose,
//     `call [edx+4]` with no argument, NOT a delete - then that vector
//     cleared too. This one expands to `erase(begin(), end())` (0x54cdb0)
//     rather than an out-of-line clear, which is VC6's own choice per
//     instantiation and not a spelling difference.
// Both loops re-read `size()` across the back edge, complete with
// Dinkumware's `_First == 0` guard, exactly as Init's already-exact
// mapObjectData sweep does. Do not hoist either bound.

// Residual (95.8833%): ONE block, the `delete[]`. Retail emits
// `push 3 / call ??_ENewmapCell` - the compiler-generated vector deleting
// destructor at 0x4fd460 - and our CL expands it in place (array cookie,
// `vector destructor iterator', operator delete). Every other row of the
// diff is a reloc NAME on an unclaimed COMDAT, including two the retail
// link FOLDED: ~vector<CSprite*> and ~vector<CObject*> both resolve to
// 0x46a650, so one synth label answers for two of our symbols.

// AND IT IS A BUDGET SEE-SAW, NOT A MISSING SPELLING - three ways measured,
// all worse than leaving it:
//   * `#pragma inline_depth(0)` on the `delete[]` statement DOES produce
//     retail's `call ??_ENewmapCell` exactly, and still scores 89.2893,
//     because the budget it frees is then spent over-inlining
//     ~vector<BlackBoxData> (0x506350), which retail CALLS. predict-inline
//     confirms the trade: 23 out-of-line calls on both sides, ours carrying
//     one extra `operator delete`. Pinning an EARLY site enlarges
//     budget/sites-remaining for the later ones - the documented rule that
//     a LATER pin cannot shrink an earlier site's divisor does not run
//     backwards.
//   * a pin before the closing brace, to reach the fourteen implicit member
//     destructions, de-inlines all fourteen: 46.6142.
//   * writing the body as a single-call-site `closeMap(this)` helper - the
//     readQuestGuardArm lever, and the shape NewfullMap::Close's note below
//     predicts - scores 87.7157.
VA(0x004fd1e0, 0x271)  // anchor-global, dc 0xec6a4
NewfullMap::~NewfullMap()
{
    if (m_cellData) {
        delete[] m_cellData;
        m_cellData = 0;
    }

    unsigned int i;
    for (i = 0; i < m_mapObjectData.size(); ++i)
        delete m_mapObjectData[i];
    // OVER-INLINE PIN, +2.34 (93.5482 -> 95.8833). Retail calls the
    // out-of-line vector<CMapObjectData*>::clear COMDAT at 0x48b4a0 here;
    // unpinned our CL expands clear() into erase(begin(), end()) and calls
    // erase instead. The sprites clear() below is retail's OWN expansion of
    // the same member into erase (0x54cdb0) and must stay unpinned - the
    // two instantiations really do diverge in retail.
#pragma inline_depth(0)
    m_mapObjectData.clear();
#pragma inline_depth()

    for (i = 0; i < m_sprites.size(); ++i)
        m_sprites[i]->dispose();
    m_sprites.clear();
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:537
// NO RETAIL SLOT. The old VA claim at 0x4fd460 was false: retail takes a
// destructor-flags argument, tests bits 1 and 2, reads the array cookie at
// [this-4], calls NewmapCell::~NewmapCell (0x4fd4c0) through the vector
// destructor iterator, and conditionally calls operator delete. It is the
// compiler-generated NewmapCell deleting destructor, not this no-argument
// Dreamcast method. Retail NewfullMap::~NewfullMap and Init inline Close.
// The retail body is claimed below through the already-emitted ??_E public.
void NewfullMap::Close()
{
    // @stub
}

// E:\gamedcs\mapcell.cpp:544 - moved here from DC tail position (dc 0xf4bdc):
// retail places this COMDAT right after Close, which calls it and takes its
// address for the `vector destructor iterator' (also address-taken in Init).
#endif  // @carcass

VA_COMPGEN(0x004fd460, 0x58, VECTOR_DELETING_DTOR, NewmapCell)

VA(0x004fd4c0, 0x26)  // dc 0xf4bdc
NewmapCell::~NewmapCell()
{
}

VA(0x004fd4f0, 0x160)  // dc 0xec80c
void NewfullMap::init(int size, unsigned char twoLayers)
{
    m_size = size;
    m_hasTwoLevels = twoLayers;

    if (m_cellData) {
        delete[] m_cellData;
        m_cellData = 0;
    }

    for (unsigned int i = 0; i < m_mapObjectData.size(); ++i)
        delete m_mapObjectData[i];
    m_mapObjectData.clear();

    int cellCount = (twoLayers != 0) + 1;
    cellCount *= m_size;
    cellCount *= m_size;
    m_cellData = new NewmapCell[cellCount];

    if (g_mapExtra)
        delete[] g_mapExtra;
    g_mapExtra = new unsigned short[cellCount];
    memset(g_mapExtra, 0, cellCount * sizeof(*g_mapExtra));
}

VA(0x004fd650, 0x3E)  // dc 0xf49a4
NewmapCell::NewmapCell()
{
    m_groundSet = 0;
    m_groundIndex = 0;
    m_riverSet = 0;
    m_riverIndex = 0;
    m_roadSet = 0;
    m_roadIndex = 0;
    m_flags0011 = 0;
    m_isTrigger = 0;
    m_flags1315 = 0;
    m_type = NOTHING;
    m_objectIndex = -1;
    m_extraInfo = 0;
    m_objectTypeIndex = -1;
}

// Everything else in the body is byte-identical; the remaining diff rows are
// reloc NAMES only, on the ten STL erase COMDATs the clears reach, which are
// excluded class and will never carry a claim.

VA(0x004fd690, 0x2B3)  // dc 0xec8f4
int NewfullMap::read(TAbstractFile* infile, int size, unsigned char twoLayers,
                     int mapVersion)
{
    init(size, twoLayers);

    if (readMapLayer(infile, size, 0) < 0)
        return -1;
    if (twoLayers) {
        if (readMapLayer(infile, size, 1) < 0)
            return -1;
    }

    incProgressBar(1);

    m_objectTypes.clear();
    m_objects.clear();
    m_randomDwellings.clear();
    m_customTreasure.clear();
    m_customMonsterList.clear();
    m_blackBoxes.clear();
    m_seerHutList.clear();
    m_questGuardList.clear();
    m_timedEventList.clear();
    m_townEventList.clear();
    m_heroPlaceholders.clear();
    g_game->m_towns.clear();
    g_game->m_scenarioTowns.clear();
    g_game->m_signs.clear();
    g_game->m_mines.clear();
    g_game->m_generators.clear();
    g_game->m_garrisons.clear();
    g_game->m_boats.clear();
    g_game->m_blackMarkets.clear();
    g_game->m_universities.clear();
    g_game->m_creatureBanks.clear();

    if (readMapObjects(infile, mapVersion) < 0)
        return -1;
    if (readTimedEventList(infile, mapVersion) < 0)
        return -1;

    soDTransformRandomDwellings();

    unsigned int i;
#pragma inline_depth(0)
    for (i = 0; i < m_objects.size(); ++i)
#pragma inline_depth()
        placeObject(i, 1);

    loadShipyards();
    return 0;
}

// Savegame-only quest-guard list loader. Load calls this only for version 25
// and newer, immediately after the older seer-hut list. Retail reads the
// 16-bit count without branching on the byte count, resizes the five-byte
// TQuestGuard vector, loads every row, and registers each non-null polymorphic
// quest record in mapObjectData just as the seer-hut loop in Load does.

// Residual (90.4916%, compiler allocation wall): `sema --branches` agrees on
// all 19 branch decisions. The remaining differences are register/stack
// scheduling across the inlined resize and push_back bodies. An unsigned
// short loop was 89.7479%; masking an int and using for/while loops reached
// 88.3698/89.8403%; a second promoted count was 88.6135%; and explicitly
// hoisting the quest pointer was 82.2605%. The guarded do/while below is the
// best source-faithful spelling measured.

// Retail retains this helper as a call from NewfullMap::load at +0x328.
// The former auto_inline(off) forced that decision but did not recover its
// source cause. Removing it with the current caller expands this helper,
// changes load from 77.26911% to 56.67278%, and stops emitting the previously
// exact std::copy<type_university>; no other claimed body's score changes.
// The parked std::copy<garrison> still does not emit. The retained helper's
// own body stays at its prior score. Ordinary resize(count) then recovers
// load to 56.72171%. Keep this source call while recovering its boundary;
// the university-copy MAX/HIST and load's all-time HIST remain preserved.
// An older caller's call-site inline_depth(0) control reached only 84.27%
// versus that caller's pinned-helper 92.201836%; neither pin is source proof.

// [polish-45] The first `!!` names the remaining shape precisely: retail
// SINKS the masked count.  `mov esi,[ebp-0xc] / and esi,0xffff` keeps the
// value in ESI across the whole inlined resize and only writes it back to
// count's own address-taken slot just before the loop; this compile does the
// read-modify-write `mov ecx,[ebp-0x4] / and ecx,0xffff / mov [ebp-0x4],ecx`
// at the statement and reloads for every later use.  The stack ordering is
// the mirror image too - retail assigns -0x4 to the quest pointer, -0x8 to
// `i` and -0xc to `count`, this compile assigns -0x4/-0x8/-0xc to
// count/quest/i - so the two compiles walked the same symbols in opposite
// handle order (docs/vc6/regalloc.md 0).  Frames are the same size (0x1c),
// so no local is missing; the earlier "second promoted count" probe (88.6135)
// is the right family but the wrong direction.

// [polish-47] The slot ledger is now read off the bytes, and BOTH sides home
// the same three values - the difference is only WHICH slot each got.
// Retail: quest at [ebp-0x4] (stored in the branch shadow of
// `mov eax,[ebx+eax] / test eax,eax / je`), i at -0x8, count at -0xc.
// This compile: count at -0x4, quest at -0x8, i at -0xc. Two source
// orderings measured against that, do not retry:
//   * declaring `int i; int count;` as a top-of-function block (retail's
//     order for those two) - BYTE-FLAT at 90.4930. It does move i from
//     -0x4's neighbour down to -0xc, so declaration order DOES drive the
//     numbering, but count stays at -0x4 and nothing else follows.
//   * the full DC-style block `type_quest* quest; int i; int count;` with
//     the loop body reading `quest = QuestGuardList[i].quest` once -
//     90.4930 -> 82.2581, and the branch polarity flips. Naming the quest
//     pointer lengthens its live range across the load() call and costs far
//     more than the slot it buys.
// So the residual is the handle NUMBERING with the same local set, not a
// missing or extra local: docs/vc6/handle-order.md's C1-capped class.
VA(0x004fd950, 0x268)  // caller Load 0xfdbc0; TQuestGuard ctor/load + vector resize/push_back
void NewfullMap::newfullMapFn004FD950(
    TAbstractFile* infile, int saveVersion)
{
    int count;
    infile->read(&count, 2);
    count &= 0xFFFF;
    m_questGuardList.resize(count);

    if (count > 0) {
        int i = 0;
        do {
            m_questGuardList[i].load(infile, saveVersion);
            if (m_questGuardList[i].m_quest)
                m_mapObjectData.push_back(static_cast<CMapObjectData*>(
                    static_cast<void*>(m_questGuardList[i].m_quest)));
            ++i;
        } while (--count);
    }
}

// E:\gamedcs\mapcell.cpp:679, dc 0xecb94. Original names: two_layers,
// sprite_num. The clear calls retain their Dreamcast-proven public APIs.
// Retail reads a signed short seer count and operates on this map's list.
// The older TSeerHut::LoadSeerList (dc 0x12d854, seerhut.cpp:503) instead
// accesses the global map and checks an int-returning seer loader; retail's
// instance-relative list and void load(infile, saveVersion) contradict that
// interface. The seer block therefore stays in this Complete caller.
// The former resizeSeerHutList wrapper was a synthetic inline-budget probe;
// it and the loop's inline-depth pin are removed. The discarded twoLayers
// expression had no evidence of a meaningful release VERIFY and is removed.
// Retail's short-read failure shares the forward load_failure tail, and
// repeated seer subscripts preserve its reloads after the virtual load call.
// Natural-source controls: 45 valid count/default-value/append/index forms
// and 32 failure-scope/quest-scope/loop forms reach 77.26911%; 27 additional
// for-initializer forms are rejected by VC6 because load_failure skips the
// initializer. The selected scoped default value improves the direct-call
// baseline (74.24159%); these controls still used the legacy quest-helper
// pin described above. Its removal leaves the named default at 56.67278%;
// ordinary resize(count) and an explicit temporary both reach 56.72171%.
// A const named default is flat; none of these emits either copy helper.
// Use the ordinary default-argument form in this unpinned context.
// No valid candidate emits std::copy<garrison> at
// 0x5095a0. Retail expands garrison clear through erase/copy, then retains
// seer resize and its size queries; our remaining nested decisions differ.
// The former 93.4037% peak depended on the removed synthetic boundaries and
// stays in HIST. Keep the real clear/resize/append calls through this dip.
// The seer/event phase uses a single failure scope: short-read and town-
// event failures break to one -1 return. Both do/while(0) and for(;;) forms
// remove two gotos at 56.7217%, with all sibling scores unchanged. Direct
// returns at either site instead score 53.5994%; keep the common boundary
// and the original seer-object lifetime inside this scope.
VA(0x004fdbc0, 0x371)  // order-map: calls loadTimedEventList 0xfc500, loadTownEventList 0xfc870, Init 0xfd4f0, loadMapLayer 0xfe920 x2, loadBlackBoxList/loadMonsterList/loadMapObjects, dc 0xecb94
int NewfullMap::load(TAbstractFile* infile, int size, unsigned char twoLayers,
                     int saveVersion)
{
    init(size, twoLayers);

    if (loadMapLayer(infile, size, 0, saveVersion) < 0)
        return -1;
    if (twoLayers) {
        if (loadMapLayer(infile, size, 1, saveVersion) < 0)
            return -1;
    }

    incProgressBar(1);

    m_objectTypes.clear();
    m_objects.clear();
    m_customTreasure.clear();
    m_customMonsterList.clear();
    m_blackBoxes.clear();
    m_seerHutList.clear();
    m_questGuardList.clear();
    m_timedEventList.clear();
    m_townEventList.clear();
    g_game->m_towns.clear();
    g_game->m_scenarioTowns.clear();
    g_game->m_signs.clear();
    g_game->m_mines.clear();
    g_game->m_generators.clear();
    g_game->m_garrisons.clear();
    g_game->m_boats.clear();
    g_game->m_universities.clear();
    g_game->m_creatureBanks.clear();

    if (loadMapObjects(infile) < 0)
        return -1;
    if (loadBlackBoxList(infile, saveVersion) < 0)
        return -1;
    if (loadTreasureList(infile) < 0)
        return -1;
    if (loadMonsterList(infile) < 0)
        return -1;

    do {
        {
            short count;
            if (infile->read(&count, sizeof(count)) < sizeof(count))
                break;

            m_seerHutList.resize(count);
            int spriteNum;
            for (spriteNum = 0; spriteNum < m_seerHutList.size(); ++spriteNum)
            {
                m_seerHutList[spriteNum].load(infile, saveVersion);
                if (m_seerHutList[spriteNum].m_quest)
                    m_mapObjectData.push_back(static_cast<CMapObjectData*>(
                        static_cast<void*>(m_seerHutList[spriteNum].m_quest)));
            }
        }

        if (saveVersion >= 25)
            newfullMapFn004FD950(infile, saveVersion);

        if (loadTimedEventList(infile, saveVersion) < 0)
            return -1;
        if (loadTownEventList(infile, saveVersion) < 0)
            break;

        incProgressBar(1);
        return 0;
    } while (0);

    return -1;
}

// Complete adds a separate quest-guard pool; its save loop is unchanged.
static void saveQuestGuardList(NewfullMap* map, TAbstractFile* outfile)
{
    // The mirror of the seer-hut helper, one call further: retail calls
    // vector<TQuestGuard>::size THREE times (Save+0x254 for this count,
    // +0x26e and +0x297 for the loop) where our CL only left the loop's
    // two out of line. The `inline_depth(0)` pin that forced this one out
    // of line is now a LOSS: removing it is NewfullMap::Save 91.88699 ->
    // 93.71233, a new MAX, with no other row moving (2026-09-06, polish
    // lane 50).
    int count = map->m_questGuardList.size();
    outfile->write(&count, 2);
    for (unsigned int i = 0; i < map->m_questGuardList.size(); ++i)
        map->m_questGuardList[i].save(outfile);
}

// E:\gamedcs\mapcell.cpp:759
// The save-game driver: two layers, the objects, then the five custom
// record sets in the order their vectors sit in the class - black boxes,
// treasure, monsters, seer huts, quest guards - and finally the two event
// lists. Every list header is a two-byte count taken from size().

// The first three sets retain their DC-proven ordinary member helpers.
// Complete's seer pool belongs to this map, whereas DC SaveSeerList is a
// static TSeerHut helper over the global pool. NewfullMap's existing friend
// relationship permits the later loop to call private TSeerHut::save.

// The former free saveSeerHutList helper and its inline_depth pin recovered
// 93.7123% but had no source ownership evidence. Removing them leaves 57.36%:
// VC6 still expands saveTimedEventList and disagrees on vector::size calls.
// This is an unresolved inliner boundary, not permission to widen save's
// access or reinstate a budget-only split. HIST retains the previous peak.
// Quest-guard count writes and per-record saves retain retail's unchecked
// results, unlike the short-write gates for the preceding lists.
VA(0x004fdf40, 0x2D1)  // order-map: calls saveTimedEventList 0xfc390, saveTownEventList 0xfc770, saveMapLayer 0xfe490 x2, saveMapObjects 0x104a40, TQuestGuard::save, dc 0xecdf8
int NewfullMap::save(TAbstractFile* outfile, int size, unsigned char twoLayers)
{
    if (saveMapLayer(outfile, size, 0) < 0)
        return -1;
    if (twoLayers) {
        if (saveMapLayer(outfile, size, 1) < 0)
            return -1;
    }
    if (saveMapObjects(outfile) < 0)
        return -1;

    if (saveBlackBoxList(outfile) < 0)
        return -1;
    if (saveTreasureList(outfile) < 0)
        return -1;
    if (saveMonsterList(outfile) < 0)
        return -1;

    // DC calls TSeerHut::SaveSeerList, a static helper over the global map.
    // Complete reads this map's vector instead. Keep the loop in the proven
    // friend NewfullMap rather than inventing a free helper to steer /Ob2.
    // The former helper's inline_depth pin has also been removed.
    int seerCount = m_seerHutList.size();
    if (static_cast<unsigned>(outfile->write(&seerCount, 2)) < 2)
        return -1;
    for (unsigned int seer = 0; seer < m_seerHutList.size(); ++seer)
        m_seerHutList[seer].save(outfile);
    saveQuestGuardList(this, outfile);

    if (saveTimedEventList(outfile) < 0)
        return -1;
    return saveTownEventList(outfile) >= 0 ? 0 : -1;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

#endif  // @carcass

// E:\gamedcs\mapcell.cpp:809
// The .h3m map-layer reader, and NOT the twin of saveMapLayer: the map
// format carries seven bytes per cell where the save format carries the
// whole record.  Six of them are the terrain/river/road set-index pairs;
// the seventh is a flag byte whose low six bits are the mirror flags, one
// bit each, and whose bit 6 marks a coastal square.

// Everything after that byte is derived, not read, which is why this body
// writes flags the stream never mentions:
//   - a coastal square on anything but water becomes an ANCHOR_POINT and
//     loses its trigger bit;
//   - rock is the one impassable ground;
//   - shallow water (index below 20) is beach border;
//   - water, lava, rivers and roads are the animated surfaces.
// The flag byte's own bit 6 and the cell's Passable bit share a position
// by coincidence of the DC bit roster, not by assignment - the byte is
// tested with `& 0x40` and never stored whole.

// Residual (95.4717%): every read, every derived flag and every block of
// the CFG is retail's; what differs is where the flag byte is materialized.
// Retail promotes it straight out of memory (`movsx eax, byte`) for the
// five shifted bits, then RELOADS the byte for bit 0 and the `& 0x40`
// test, and folds bit 0 into the preserved high bits with the xor-form
// insert; this compile loads the byte into cl first, promotes out of the
// register, keeps cl live to the end, and uses the or-form.  Everything
// after that is the same permutation carried downstream, which is why the
// three flag blocks below diff as register renames on identical shapes.
// Measured and rejected: ascending bit order with a plain char (86.9764),
// descending with a plain char (82.5519), and a per-use
// static_cast<unsigned int> in place of the named hoist (95.4717, byte-
// identical to this).  The unsigned hoist is what buys the 32-bit shift
// domain at all - without it VC6 narrows every extraction to 8 bits.
// why-reg v2 reports the bindings agreeing at every first definition, so
// the divergence is past the B1 minimum slice: a caller-independent
// scheduling/homing cap, not a spelling.
// 2026-09-06, the xor-insert re-examined against the bytes. Retail's tail is
// `mov cx,[esi+0xc] / and ecx,0xffc0` scheduled EARLY (between the bit-4 and
// bit-3 extractions), then `mov al,[ebp+0x10] / mov bl,al / and bl,1 /
// movsx bx,bl / xor ebx,ecx / or edx,ebx` - i.e. `acc5 | (bit0 ^ preserved)`,
// exactly the association C precedence gives `a | b ^ c`. This compile emits
// `(acc5 | bit0) | preserved` with the word load LAST, and no source
// grouping reaches the other association: four more spellings measured this
// lane, all worse - the last field read through `flags` rather than `value`
// (93.62), ascending bit order WITH the unsigned hoist (95.40, so the note's
// earlier ascending measurement was not just the char), the `& 0x40` test
// read through `flags` (92.00), a separate `signed char flagByte` local for
// the seventh read (95.03), and the bit-0 store hoisted above the other five
// (95.40). The six stores are one merged read-modify-write either way; what
// moves is only which pair VC6 combines first.
VA(0x004fe220, 0x26B)  // order-map: leaf (file I/O devirtualized-inline); called x2 by Read 0xfd690 in the layer slot, dc 0xecf98
int NewfullMap::readMapLayer(TAbstractFile* infile, int size, int layer)
{
    NewmapCell* thisCell = &m_cellData[m_size * m_size * layer];

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            signed char value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_groundSet = value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_groundIndex = value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_riverSet = value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_riverIndex = value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_roadSet = value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_roadIndex = value;

            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            unsigned int flags = value;
            thisCell->m_roadFlippedVertical = (flags >> 5) & 1;
            thisCell->m_roadFlippedHorizontal = (flags >> 4) & 1;
            thisCell->m_riverFlippedVertical = (flags >> 3) & 1;
            thisCell->m_riverFlippedHorizontal = (flags >> 2) & 1;
            thisCell->m_groundFlippedVertical = (flags >> 1) & 1;
            thisCell->m_groundFlippedHorizontal = value & 1;

            if ((value & 0x40) && thisCell->m_groundSet != eTerrainWater) {
                thisCell->m_type = ANCHOR_POINT;
                thisCell->m_isTrigger = 0;
                thisCell->m_isBeachBorder = 1;
            }

            thisCell->m_isBlocked = 0;
            thisCell->m_passable = 1;
            if (thisCell->m_groundSet == eTerrainRock) {
                thisCell->m_passable = 0;
                thisCell->m_isBlocked = 1;
            }

            if (thisCell->m_groundSet == eTerrainWater
                && thisCell->m_groundIndex < 20)
                thisCell->m_isBeachBorder = 1;

            if (thisCell->m_groundSet == eTerrainWater
                || thisCell->m_groundSet == eTerrainLava
                || thisCell->m_riverSet != 0 || thisCell->m_roadSet != 0)
                thisCell->m_animated = 1;

            ++thisCell;
        }
    }
    return size * size;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

#endif  // @carcass

VA(0x004fe490, 0x22A)  // dc 0xed384
int NewfullMap::saveMapLayer(TAbstractFile* outfile, int size, int layer)
{
    NewmapCell* thisCell = &m_cellData[m_size * m_size * layer];

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            char byteValue;
            byteValue = static_cast<char>(thisCell->m_groundSet);
            if (static_cast<unsigned>(outfile->write(&byteValue, 1)) < 1)
                return -1;
            byteValue = static_cast<char>(thisCell->m_groundIndex);
            if (static_cast<unsigned>(outfile->write(&byteValue, 1)) < 1)
                return -1;
            byteValue = static_cast<char>(thisCell->m_riverSet);
            if (static_cast<unsigned>(outfile->write(&byteValue, 1)) < 1)
                return -1;
            byteValue = static_cast<char>(thisCell->m_riverIndex);
            if (static_cast<unsigned>(outfile->write(&byteValue, 1)) < 1)
                return -1;
            byteValue = static_cast<char>(thisCell->m_roadSet);
            if (static_cast<unsigned>(outfile->write(&byteValue, 1)) < 1)
                return -1;
            byteValue = static_cast<char>(thisCell->m_roadIndex);
            if (static_cast<unsigned>(outfile->write(&byteValue, 1)) < 1)
                return -1;

            short wordValue;
            wordValue = static_cast<short>(thisCell->m_cellFlags);
            if (static_cast<unsigned>(outfile->write(&wordValue, 2)) < 2)
                return -1;
            wordValue = static_cast<short>(thisCell->m_type);
            if (static_cast<unsigned>(outfile->write(&wordValue, 2)) < 2)
                return -1;
            short indexValue;
            indexValue = thisCell->m_objectIndex;
            if (static_cast<unsigned>(outfile->write(&indexValue, 2)) < 2)
                return -1;
            indexValue = thisCell->m_objectTypeIndex;
            if (static_cast<unsigned>(outfile->write(&indexValue, 2)) < 2)
                return -1;

            unsigned long extra = thisCell->m_extraInfo;
            if (static_cast<unsigned>(outfile->write(&extra, 4)) < 4)
                return -1;

            int count = thisCell->m_objects.size();
            if (static_cast<unsigned>(outfile->write(&count, 4)) < 4)
                return -1;

            for (unsigned int i = 0; i < thisCell->m_objects.size(); ++i) {
                if (static_cast<unsigned>(
                        outfile->write(&thisCell->m_objects[i], 4)) < 4)
                    return -1;
            }
            ++thisCell;
        }
    }
    return size * size;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

#endif  // @carcass

union LegacyUpgradeExtraInfo {
    unsigned long m_value;
    LegacyArtifactInfo m_artifactInfo;
    LegacySkeletonInfo m_skeletonInfo;
    LegacyMonsterInfo m_monsterInfo;
    LegacyPyramidInfo m_pyramidInfo;
    LegacyTreasureInfo m_treasureInfo;
    LegacyWagonInfo m_wagonInfo;
    LegacyTombInfo m_tombInfo;
};

union CurrentUpgradeExtraInfo {
    unsigned long m_value;
    CurrentArtifactInfo m_artifactInfo;
    type_skeleton_info m_skeletonInfo;
    MonsterInfo m_monsterInfo;
    type_pyramid_info m_pyramidInfo;
    TreasureInfo m_treasureInfo;
    CurrentUpgradeWagonInfo m_wagonInfo;
    type_tomb_info m_tombInfo;
    CurrentVisitedInfo m_visitedInfo;
};

// Retail-only compatibility pass called once for every loaded map cell.
// The seven non-default lookup-table entries are object ids 5, 22, 54, 63,
// 101, 105 and 108; the case spellings below are therefore byte-proven.

// Residual (96.26%, compiler register-allocation wall): base and retail have
// the same 14-block flow, and the ARTIFACT, DEAD_GUY and PYRAMID arms are
// instruction-exact. MONSTER, TREASURE_CHEST, WAGON and WARRIOR_TOMB perform
// the same masked field transfers but assign the source/result roles to
// EAX/EDX/ESI differently. That makes the code one byte longer and moves the
// otherwise-identical lookup/jump tables by the following four-byte
// alignment boundary.

// The legacy dword must be a distinct value: viewing old and new layouts as
// aliases lets C2 delete the identity field copies and scores 69.53%. One
// snapshot before the switch scores 92.55%; reversing the byte-indicated
// monster index conversion scores 94.52%; case-local storage, `register`,
// and declaration-order probes are byte-flat. The separate per-arm snapshot
// below is the best source-faithful form measured.
VA(0x004fe6c0, 0x254)  // caller loadMapLayer; v25 gate + seven-arm bitfield upgrade
void upgradeCellExtraInfo(NewmapCell* cell, int saveVersion)
{
    if (saveVersion >= 25)
        return;

    LegacyUpgradeExtraInfo legacy;
    CurrentUpgradeExtraInfo* current =
        static_cast<CurrentUpgradeExtraInfo*>(static_cast<void*>(cell));

    switch (cell->m_type) {
    case ARTIFACT:
        legacy.m_value = cell->m_extraInfo;
        current->m_artifactInfo.m_custom = legacy.m_artifactInfo.m_custom;
        if (!current->m_artifactInfo.m_custom) {
            current->m_artifactInfo.m_guard = legacy.m_artifactInfo.m_guard;
            current->m_artifactInfo.m_resourcePrice =
                legacy.m_artifactInfo.m_resourcePrice;
            current->m_artifactInfo.m_guardQty =
                legacy.m_artifactInfo.m_guardQty;
        }
        break;

    case DEAD_GUY:
        legacy.m_value = cell->m_extraInfo;
        current->m_skeletonInfo.m_artifact = legacy.m_skeletonInfo.m_artifact;
        current->m_skeletonInfo.m_hasTreasure =
            legacy.m_skeletonInfo.m_hasTreasure;
        current->m_skeletonInfo.m_id = legacy.m_skeletonInfo.m_id;
        break;

    case MONSTER:
        legacy.m_value = cell->m_extraInfo;
        current->m_monsterInfo.m_qty = legacy.m_monsterInfo.m_qty;
        current->m_monsterInfo.m_disposition = legacy.m_monsterInfo.m_disposition;
        current->m_monsterInfo.m_neverFlee = legacy.m_monsterInfo.m_neverFlee;
        current->m_monsterInfo.m_dontGrow = legacy.m_monsterInfo.m_dontGrow;
        current->m_monsterInfo.m_index = legacy.m_monsterInfo.m_index;
        current->m_monsterInfo.m_unused27 = 0;
        current->m_monsterInfo.m_custom = legacy.m_monsterInfo.m_custom;
        break;

    case PYRAMID:
        legacy.m_value = cell->m_extraInfo;
        current->m_pyramidInfo.m_guarded = legacy.m_pyramidInfo.m_guarded;
        current->m_visitedInfo.m_visitedBits =
            legacy.m_pyramidInfo.m_visitedBits;
        current->m_pyramidInfo.m_spell = legacy.m_pyramidInfo.m_spell;
        break;

    case TREASURE_CHEST:
        legacy.m_value = cell->m_extraInfo;
        current->m_treasureInfo.m_artifact = legacy.m_treasureInfo.m_artifact;
        current->m_treasureInfo.m_hasArtifact =
            legacy.m_treasureInfo.m_isArtifact;
        current->m_treasureInfo.m_gold = legacy.m_treasureInfo.m_goldAmount;
        break;

    case WAGON:
        legacy.m_value = cell->m_extraInfo;
        current->m_wagonInfo.m_artifact = legacy.m_wagonInfo.m_artifact;
        current->m_wagonInfo.m_full = legacy.m_wagonInfo.m_full;
        current->m_wagonInfo.m_hasArtifact = legacy.m_wagonInfo.m_hasArtifact;
        current->m_wagonInfo.m_resource = legacy.m_wagonInfo.m_resource;
        current->m_wagonInfo.m_resourceAmount =
            legacy.m_wagonInfo.m_resourceAmount;
        current->m_visitedInfo.m_visitedBits = legacy.m_wagonInfo.m_visitedBits;
        break;

    case WARRIOR_TOMB:
        legacy.m_value = cell->m_extraInfo;
        current->m_tombInfo.m_hasArtifact = legacy.m_tombInfo.m_full;
        current->m_visitedInfo.m_visitedBits = legacy.m_tombInfo.m_visitedBits;
        current->m_tombInfo.m_artifact = legacy.m_tombInfo.m_artifact;
        break;
    }
}

// The two-byte object type widens into the four-byte enum field.  That is
// the one crossing this body cannot spell without a cast: the stream
// carries sixteen bits and TAdventureObjectType is a full int, so the
// widening is a real domain crossing, not a modelling slip.
VA(0x004fe920, 0x2E5)  // dc 0xed688
int NewfullMap::loadMapLayer(TAbstractFile* infile, int size, int layer,
                             int saveVersion)
{
    NewmapCell* thisCell = &m_cellData[m_size * m_size * layer];

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            signed char value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_groundSet = value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_groundIndex = value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_riverSet = value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_riverIndex = value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_roadSet = value;
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisCell->m_roadIndex = value;

            unsigned short wordValue;
            if (infile->read(&wordValue, sizeof(wordValue)) < sizeof(wordValue))
                return -1;
            thisCell->m_cellFlags = wordValue;
            if (infile->read(&wordValue, sizeof(wordValue)) < sizeof(wordValue))
                return -1;
            thisCell->m_typeValue = wordValue;

            unsigned short indexValue;
            if (infile->read(&indexValue, sizeof(indexValue))
                < sizeof(indexValue))
                return -1;
            thisCell->m_objectIndex = indexValue;
            if (infile->read(&indexValue, sizeof(indexValue))
                < sizeof(indexValue))
                return -1;
            thisCell->m_objectTypeIndex = indexValue;

            unsigned long extra;
            if (infile->read(&extra, sizeof(extra)) < sizeof(extra))
                return -1;
            thisCell->m_extraInfo = extra;

            int count;
            if (infile->read(&count, sizeof(count)) < sizeof(count))
                return -1;
            thisCell->m_objects.resize(count);

            for (unsigned int i = 0; i < thisCell->m_objects.size(); ++i) {
                if (infile->read(&thisCell->m_objects[i],
                                 sizeof(thisCell->m_objects[i]))
                    < sizeof(thisCell->m_objects[i]))
                    return -1;
            }

            upgradeCellExtraInfo(thisCell, saveVersion);
            ++thisCell;
        }
    }
    return size * size;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:1095
DC_ONLY(0xed984, 0x98)
int NewfullMap::readBoatData(void* infile, CObject* boatObject)
{
    // @stub
}

#endif  // @carcass

VA(0x004fec10, 0x1D)  // dc 0xeda1c
CObjectType* CObject::getObjectTypePtr() const
{
    return &g_game->m_worldMap.m_objectTypes[m_typeIndex];
}

VA(0x004fec30, 0x106)  // dc 0xedab8
void CObject::findTrigger(int& resultX, int& resultY) const
{
    resultX = -1;
    resultY = -1;

    CObjectType* objType = &g_game->m_worldMap.m_objectTypes[m_typeIndex];
    for (int vert = 0; vert < objType->m_height; ++vert) {
        if (m_y - vert < 0 || m_y - vert >= g_mapHeight)
            continue;

        for (int horiz = 0; horiz < objType->m_width; ++horiz) {
            if (m_x - horiz < 0 || m_x - horiz >= g_mapWidth)
                continue;

            if (objType->m_triggerCells[47 - vert * 8 - horiz]) {
                resultX = m_x - horiz;
                resultY = m_y - vert;
                return;
            }
        }
    }
}

VA(0x004fed40, 0x102)  // dc 0xedbf8
int NewfullMap::readGeneratorData(
    TAbstractFile* infile, CObject* generatorObject)
{
    generator tempGenerator;
    signed char owner;
    char padding[3];

    if (infile->read(&owner, 1) != 1)
        return -1;
    if (static_cast<unsigned>(infile->read(padding, 3)) < 3)
        return -1;

    CObjectType& objectType = m_objectTypes[generatorObject->m_typeIndex];
    tempGenerator.m_genType = static_cast<char>(objectType.m_extra);
    tempGenerator.m_genClass = static_cast<char>(objectType.m_objectType);

    int x;
    int y;
    generatorObject->findTrigger(x, y);
    tempGenerator.m_mapX = static_cast<unsigned char>(x);
    tempGenerator.m_mapY = static_cast<unsigned char>(y);
    tempGenerator.m_mapZ = generatorObject->m_z;
    tempGenerator.initialize(owner);

    g_game->m_generators.push_back(tempGenerator);
    generatorObject->m_extraInfo = g_game->m_generators.size() - 1;
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:1199
DC_ONLY(0xedd14, 0xD2)
int NewfullMap::readHolyGrailData(void* infile, CObject* grailObject)
{
    // @stub
}

// E:\gamedcs\mapcell.cpp:1224
DC_ONLY(0xedde8, 0x70)
int NewfullMap::readShrineData(void* infile, CObject* shrineObject)
{
    // @stub
}

#endif  // @carcass

VA(0x004fee50, 0xBC)  // dc 0xede58
int NewfullMap::readTreasureData(TAbstractFile* infile, TreasureData* treasure)
{
    NewSMapHeader::readString(infile, treasure->m_message);

    unsigned char charBuffer;
    if (static_cast<unsigned>(infile->read(&charBuffer, 1)) < 1)
        return -1;
    treasure->m_hasCustomGuardians = charBuffer != 0;

    if (treasure->m_hasCustomGuardians) {
        for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
            if (g_game->m_mapHeader.m_version
                == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
                signed char creature;
                infile->read(&creature, sizeof(creature));
                treasure->m_guardians.m_armies[i] = creature;
            } else {
                short creature;
                infile->read(&creature, sizeof(creature));
                treasure->m_guardians.m_armies[i] = creature;
            }

            short amount;
            if (static_cast<unsigned>(infile->read(&amount, sizeof(amount)))
                < sizeof(amount))
                return -1;
            treasure->m_guardians.m_numTroops[i] = amount;
        }
    }

    int ignored;
    return static_cast<unsigned>(infile->read(&ignored, sizeof(ignored)))
             < sizeof(ignored) ? -1 : 0;
}

// DC mapcell.cpp:1293 records this ordinary public member.
// Complete expands it in Save; retained-body absence does not make it static.
// The later file interface replaces DC gzwrite through void*.
int NewfullMap::saveTreasureList(TAbstractFile* outfile)
{
    int count = m_customTreasure.size();
    if (static_cast<unsigned>(outfile->write(&count, 2)) < 2)
        return -1;
    for (unsigned int i = 0; i < m_customTreasure.size(); ++i) {
        if (saveTreasureData(outfile, &m_customTreasure[i]) < 0)
            return -1;
    }
    return 0;
}

VA(0x004fef10, 0x4D)  // dc 0xee020
int NewfullMap::saveTreasureData(TAbstractFile* outfile, TreasureData* thisTreasure)
{
    game::saveString(outfile, &thisTreasure->m_message);

    unsigned char charBuffer = thisTreasure->m_hasCustomGuardians;
    if (static_cast<unsigned>(outfile->write(&charBuffer, 1)) < 1)
        return -1;
    if (thisTreasure->m_hasCustomGuardians)
        thisTreasure->m_guardians.save(outfile);
    return 0;
}

VA(0x004fef60, 0x1BD)  // dc 0xee090
int NewfullMap::loadTreasureList(TAbstractFile* infile)
{
    short count;
    if (static_cast<unsigned>(infile->read(&count, 2)) < 2)
        return -1;

    m_customTreasure.resize(count);
    for (int i = 0; i < m_customTreasure.size(); ++i) {
        TreasureData& treasure = m_customTreasure[i];
        game::loadString(infile, treasure.m_message);

        unsigned char hasGuardians;
        if (static_cast<unsigned>(infile->read(&hasGuardians, 1)) < 1)
            return -1;
        treasure.m_hasCustomGuardians = hasGuardians != 0;
        if (treasure.m_hasCustomGuardians)
            treasure.m_guardians.load(infile);
    }
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:1362
DC_ONLY(0xee12c, 0x76)
int NewfullMap::loadTreasureData(void* infile, TreasureData* thisTreasure)
{
    // @stub
}

#endif  // @carcass

VA(0x004ff120, 0x1C9)  // dc 0xee1a4
int NewfullMap::readArtifactData(TAbstractFile* infile, CObject* artifactObject)
{
    int treasureIndex = m_customTreasure.size();

    unsigned char charBuffer;
    artifactObject->m_extraInfo = 0;
    if (static_cast<unsigned>(infile->read(&charBuffer, 1)) < 1)
        return -1;

    if (charBuffer) {
        TreasureData tempTreasure;
        if (readTreasureData(infile, &tempTreasure) == 0) {
            if (treasureIndex < 4000) {
                m_customTreasure.push_back(tempTreasure);
                artifactObject->m_extraInfo = ((treasureIndex | 0xfffff000) << 19)
                    | (artifactObject->m_extraInfo & 0x7ffff);
                return 1;
            }
        } else {
            return -1;
        }
    }
    return 0;
}

VA(0x004ff2f0, 0x1D8)  // dc 0xee2e0
int NewfullMap::readSpellScrollData(TAbstractFile* infile, CObject* scrollObject)
{
    int listSize = m_customTreasure.size();

    char charBuffer;
    int count;
    scrollObject->m_extraInfo = 0;
    count = infile->read(&charBuffer, 1);
    if (static_cast<unsigned>(count) < 1)
        return -1;

    if (charBuffer) {
        TreasureData tempTreasure;
        if (readTreasureData(infile, &tempTreasure) == 0) {
            if (listSize < 4000) {
                m_customTreasure.push_back(tempTreasure);
                scrollObject->m_extraInfo = ((listSize | 0xfffff000) << 19)
                    | (scrollObject->m_extraInfo & 0x7ffff);
            }
        } else {
            return -1;
        }
    }

    // The scroll's own payload, read whether or not a custom treasure
    // record preceded it: the spell id into the low byte of extraInfo,
    // then three bytes of padding whose short count is the only failure
    // this tail reports.
    count = infile->read(&charBuffer, 1);
    if (static_cast<unsigned>(count) < 1)
        return -1;
    scrollObject->m_extraInfo ^= (scrollObject->m_extraInfo ^ charBuffer) & 0xff;

    unsigned char padding[3];
    count = infile->read(padding, 3);
    if (static_cast<unsigned>(count) < 3)
        return -1;
    return 0;
}

VA(0x004ff4d0, 0x1DA)  // dc 0xee410
int NewfullMap::readResourceData(TAbstractFile* infile, CObject* resourceObject)
{
    int listSize = m_customTreasure.size();

    unsigned char charBuffer;
    int count;
    resourceObject->m_extraInfo = 0;
    count = infile->read(&charBuffer, 1);
    if (static_cast<unsigned>(count) < 1)
        return -1;

    if (charBuffer) {
        TreasureData tempTreasure;
        if (readTreasureData(infile, &tempTreasure) == 0) {
            if (listSize < 4000) {
                m_customTreasure.push_back(tempTreasure);
                resourceObject->m_extraInfo = ((listSize | 0xfffff000) << 19)
                    | (resourceObject->m_extraInfo & 0x7ffff);
            }
        } else {
            return -1;
        }
    }

    int intBuffer;
    count = infile->read(&intBuffer, 4);
    if (static_cast<unsigned>(count) < 4)
        return -1;
    resourceObject->m_extraInfo ^= (resourceObject->m_extraInfo ^ intBuffer) & 0x7ffff;

    unsigned char padding[4];
    count = infile->read(padding, 4);
    if (static_cast<unsigned>(count) < 4)
        return -1;
    return 0;
}

// E:\gamedcs\mapcell.cpp:1524
// The map-file twin of loadBlackBox (0x500430), field for field, and the
// blocker recorded against it - "BlackBoxData is only forward-declared" - was
// wrong: saveBlackBox and loadBlackBox have been dereferencing the modelled
// type in game.h all along.

// Two things separate it from the save-file reader.  readTreasureData is
// CALLED where loadBlackBox inlines its twin, and the two narrow/wide width
// tests read from DIFFERENT sources: the artifact ids consult
// gpGame->mapHeader.version while the creature ids consult the mapVersion
// PARAMETER.  Both gates are retail's, and the asymmetry is the same one
// readGarrisonData carries.

// The two width tests also do not spell their store the same way, and the
// bytes are explicit about it: the artifact arms each write
// `Artifacts[i]` themselves (`mov [edx+4*edi],ecx` in one arm,
// `mov [eax+4*edi],edx` in the other), while the creature arms join first and
// store once (`mov [ebx-0x1c],eax`) so the loop's induction pointer anchors on
// numTroops rather than armies - the exact induction-base lesson that took
// readGarrisonData from 97.53 to 99.96.  Both width tests read UNCHECKED; only
// the troop count that follows is checked.

// The record ends with eight discarded bytes whose short count is folded into
// the return by `cmp eax,8 / sbb eax,eax`.

// EACH OF THE THREE LISTS CARRIES AN EXPLICIT ZERO TEST that loadBlackBox does
// NOT, and it is worth 38 points.  A plain `resize(count)` scores 54.6528 and
// leaves the CFG one branch short; retail spells the empty case separately -
// `if (count == 0) list.clear(); else { list.resize(count); <read loop> }` -
// which is 93.0057.  This is a real source difference, not a codegen artifact:
// retail's own loadBlackBox (0x500430) reaches the same resize with NO zero
// test, and our plain `resize(count)` reproduces THAT shape exactly, so the
// two readers genuinely disagree.  The restructuring also repaired the
// prologue's esi/edi binding for free - with the extra statements in place
// `infile` is again the first-created call-crossing pseudo and lands in ESI,
// and the frame is retail's `sub esp,0x1c` with the dead parameter slot
// reclaimed as the byte read buffer.

// Residual (93.0057%): TWO STL inline decisions, mirrored, and nothing else -
// branch counts agree 55/55 and every read, every field and every list is
// retail's.  Retail CALLS vector<SecondarySkillData>::_Destroy out of the
// first clear() and CALLS vector<int>::erase out of the artifact resize, then
// INLINES erase in the spell resize; this compile does the exact opposite at
// all three sites.  That is the /Ob2 budget spent sequentially - ours has more
// left late, retail more early - and the artifact list's `count` losing EDI to
// the inlined erase is a consequence of it, not a separate wall.  Tried and
// rejected: spelling the empty case `erase(begin(), end())` rather than
// `clear()`, which removes one nesting level from the budget division and
// costs 27 points (93.0057 -> 66.1138) - the gradient wants DEEPER nesting,
// and `clear()` is already the deepest spelling available.

// Typed-record recovery (2026-09-08): DC SecondarySkillData::type/level and
// GiveBlackBoxReward's vector<TArtifact> accesses prove the enum members now
// in game.h. Widen the signed stream bytes before copying their four-byte
// representations into the skill fields. Artifact ids use the shared bridge.
// This preserves the reads and stores but changes the template/inlining state:
// 95.3605 -> 70.4307%. In the spell resize, retail calls insert; this compile
// expands it and retains size/_Ucopy/_Ufill calls inside. Keep the proven
// element types and the 95.3605 historical peak; do not restore int artifacts
// merely to merge their vector instantiation with the spell vector.
VA(0x004ff6b0, 0x535)  // order-map: calls armyGroup::Initialize + readTreasureData 0x4fee50; callers readBlackBoxData + readEventData (DC-isomorphic), dc 0xee56c
int NewfullMap::readBlackBox(TAbstractFile* infile, BlackBoxData* thisBox,
                             int mapVersion)
{
    signed char value;
    int dwordValue;
    int count;
    int i;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisBox->m_hasCustomTreasure = value != 0;
    if (thisBox->m_hasCustomTreasure) {
        if (readTreasureData(infile, thisBox) != 0)
            return -1;
    }

    if (infile->read(&dwordValue, sizeof(dwordValue)) < sizeof(dwordValue))
        return -1;
    thisBox->m_experienceBonus = dwordValue;
    if (infile->read(&dwordValue, sizeof(dwordValue)) < sizeof(dwordValue))
        return -1;
    thisBox->m_manaBonus = dwordValue;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisBox->m_moraleBonus = value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisBox->m_luckBonus = value;

    for (i = 0; i < 7; ++i) {
        if (infile->read(&dwordValue, sizeof(dwordValue)) < sizeof(dwordValue))
            return -1;
        thisBox->m_resQty[i] = dwordValue;
    }
    for (i = 0; i < 4; ++i) {
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            return -1;
        thisBox->m_primarySkillBonus[i] = value;
    }

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    count = value;
    if (count == 0) {
        thisBox->m_secondarySkills.clear();
    } else {
        thisBox->m_secondarySkills.resize(count);
        for (i = 0; i < count; ++i) {
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            int skillType = value;
            memcpy(&thisBox->m_secondarySkills[i].m_type,
                   &skillType, sizeof(skillType));
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            int skillLevel = value;
            memcpy(&thisBox->m_secondarySkills[i].m_level,
                   &skillLevel, sizeof(skillLevel));
        }
    }

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    count = value;
    if (count == 0) {
        thisBox->m_artifacts.clear();
    } else {
        thisBox->m_artifacts.resize(count);
        for (i = 0; i < count; ++i) {
            if (g_game->m_mapHeader.m_version
                == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
                signed char narrow;
                infile->read(&narrow, sizeof(narrow));
                thisBox->m_artifacts[i] = artifactFromInt(narrow);
            } else {
                short wide;
                infile->read(&wide, sizeof(wide));
                thisBox->m_artifacts[i] = artifactFromInt(wide);
            }
        }
    }

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    count = value;
    if (count == 0) {
        // DEPTH LADDER (docs/vc6/inliner.md 6b): this third list's empty arm
        // is the LONGHAND range erase; the two above it stay `clear()`.
        // 93.0057 -> 95.3605, and a greedy second round over the other two
        // finds nothing - the rung is per-site here as everywhere.
        thisBox->m_spells.erase(thisBox->m_spells.begin(), thisBox->m_spells.end());
    } else {
        thisBox->m_spells.resize(count);
        for (i = 0; i < count; ++i) {
            if (infile->read(&value, sizeof(value)) < sizeof(value))
                return -1;
            thisBox->m_spells[i] = value;
        }
    }

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    count = value;
    thisBox->m_creatures.initialize();
    for (i = 0; i < count; ++i) {
        int creature;
        if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
            signed char narrow;
            infile->read(&narrow, sizeof(narrow));
            creature = narrow;
        } else {
            short wide;
            infile->read(&wide, sizeof(wide));
            creature = wide;
        }
        thisBox->m_creatures.m_armies[i] = creature;

        short troops;
        if (infile->read(&troops, sizeof(troops)) < sizeof(troops))
            return -1;
        thisBox->m_creatures.m_numTroops[i] = troops;
    }

    unsigned char padding[8];
    return infile->read(padding, sizeof(padding)) < sizeof(padding) ? -1 : 0;
}

// The cap here is 400, not the 4000 the treasure readers use, and the index
// lands in extraInfo's LOW TEN BITS rather than the 19..30 custom-record field
// - `xor / and 0x3ff / xor` is the read-modify-write of a 10-bit lane.

// mapVersion is carried purely to hand to readBlackBox - this body never
// inspects it, which is why the third parameter looks unused here.
VA(0x004ffbf0, 0x1F5)  // dc 0xeea60
int NewfullMap::readBlackBoxData(TAbstractFile* infile, CObject* blackboxObject,
                                 int mapVersion)
{
    int boxIndex = m_blackBoxes.size();

    blackboxObject->m_extraInfo = 0;

    BlackBoxData tempBox;
    if (readBlackBox(infile, &tempBox, mapVersion) != 0)
        return -1;

    if (boxIndex < 400) {
        m_blackBoxes.push_back(tempBox);
        blackboxObject->m_extraInfo ^= (blackboxObject->m_extraInfo ^ boxIndex)
            & 0x3ff;
    }
    return 0;
}

// DC mapcell.cpp:1729 records this ordinary public member.
// Complete expands it in Save; retained-body absence does not make it static.
// The later file interface replaces DC gzwrite through void*.
int NewfullMap::saveBlackBoxList(TAbstractFile* outfile)
{
    int count = m_blackBoxes.size();
    if (static_cast<unsigned>(outfile->write(&count, 2)) < 2)
        return -1;
    for (unsigned int i = 0; i < m_blackBoxes.size(); ++i) {
        if (saveBlackBox(outfile, &m_blackBoxes[i]) < 0)
            return -1;
    }
    return 0;
}

VA(0x004ffdf0, 0xB0)  // dc 0xf4bfc
BlackBoxData::~BlackBoxData()
{
}

// saveTreasureData is INLINED into the leading conditional rather than
// called, which is why the guardians flag gets a byte local of its own
// while every other byte write shares one.

// Three list members go out as a one-byte count followed by that many
// records; the counts are taken with the empty-vector guard, so a null
// _First writes a literal zero rather than differencing two pointers.  The
// fixed-length loops count with a signed `int` against 7 and 4, while the
// three list loops compare against size() and so come out unsigned.

VA(0x004ffea0, 0x35A)  // dc 0xeebdc
int NewfullMap::saveBlackBox(TAbstractFile* outfile, BlackBoxData* thisBox)
{
    unsigned char value = thisBox->m_hasCustomTreasure;
    if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
        return -1;

    if (thisBox->m_hasCustomTreasure) {
        if (saveTreasureData(outfile, thisBox) < 0)
            return -1;
    }

    int dwordValue = thisBox->m_experienceBonus;
    if (static_cast<unsigned>(outfile->write(&dwordValue, 4)) < 4)
        return -1;
    dwordValue = thisBox->m_manaBonus;
    if (static_cast<unsigned>(outfile->write(&dwordValue, 4)) < 4)
        return -1;

    value = thisBox->m_moraleBonus;
    if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
        return -1;
    value = thisBox->m_luckBonus;
    if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
        return -1;

    int i;
    for (i = 0; i < 7; ++i) {
        dwordValue = thisBox->m_resQty[i];
        if (static_cast<unsigned>(outfile->write(&dwordValue, 4)) < 4)
            return -1;
    }
    for (i = 0; i < 4; ++i) {
        value = thisBox->m_primarySkillBonus[i];
        if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
            return -1;
    }

    value = thisBox->m_secondarySkills.size();
    if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
        return -1;
    for (i = 0; i < thisBox->m_secondarySkills.size(); ++i) {
        value = thisBox->m_secondarySkills[i].m_type;
        if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
            return -1;
        value = thisBox->m_secondarySkills[i].m_level;
        if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
            return -1;
    }

    value = thisBox->m_artifacts.size();
    if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
        return -1;
    for (i = 0; i < thisBox->m_artifacts.size(); ++i) {
        value = thisBox->m_artifacts[i];
        if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
            return -1;
    }

    value = thisBox->m_spells.size();
    if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
        return -1;
    for (i = 0; i < thisBox->m_spells.size(); ++i) {
        value = thisBox->m_spells[i];
        if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
            return -1;
    }

    int numArmies = thisBox->m_creatures.getNumArmies();
    value = numArmies;
    if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
        return -1;
    for (i = 0; i < numArmies; ++i) {
        short creature = thisBox->m_creatures.m_armies[i];
        outfile->write(&creature, 2);
        short count = thisBox->m_creatures.m_numTroops[i];
        if (static_cast<unsigned>(outfile->write(&count, 2)) < 2)
            return -1;
    }
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

#endif  // @carcass

VA(0x00500200, 0x222)  // dc 0xef0a0
int NewfullMap::loadBlackBoxList(TAbstractFile* infile, int saveVersion)
{
    short count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_blackBoxes.resize(count);
    for (unsigned int i = 0; i < m_blackBoxes.size(); ++i) {
        if (loadBlackBox(infile, &m_blackBoxes[i], saveVersion) < 0)
            return -1;
    }
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

#endif  // @carcass

// E:\gamedcs\mapcell.cpp:1362 in the DC roster, where it is a member of
// NewfullMap. Retail inlines it at its only call site and /OPT:REF drops
// the out-of-line copy, so there is no carve row to claim - spelled a
// file-local static for the same reason loadMonsterData is.
static int loadTreasureData(TAbstractFile* infile, TreasureData* thisTreasure)
{
    game::loadString(infile, thisTreasure->m_message);

    unsigned char value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisTreasure->m_hasCustomGuardians = value != 0;
    if (thisTreasure->m_hasCustomGuardians)
        thisTreasure->m_guardians.load(infile);
    return 0;
}

// Three one-call /Ob2 budget devices for loadBlackBox. The DC roster has no
// intervening rows, so these are not claims about retail source boundaries;
// VC6 expands all three and emits no out-of-line copies. Their measured dose
// curve and the residual they leave are recorded on loadBlackBox below.
static int loadBlackBoxResources(TAbstractFile* infile,
                                 BlackBoxData* thisBox)
{
    int value;
    for (int i = 0; i < 7; ++i) {
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            return -1;
        thisBox->m_resQty[i] = value;
    }
    return 0;
}

static int loadBlackBoxDwordBonuses(TAbstractFile* infile,
                                    BlackBoxData* thisBox)
{
    int value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisBox->m_experienceBonus = value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisBox->m_manaBonus = value;
    return 0;
}

static int loadBlackBoxPrimarySkills(TAbstractFile* infile,
                                     BlackBoxData* thisBox)
{
    signed char value;
    for (int i = 0; i < 4; ++i) {
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            return -1;
        thisBox->m_primarySkillBonus[i] = value;
    }
    return 0;
}

// E:\gamedcs\mapcell.cpp:1914
// The load twin of saveBlackBox, field for field, plus the one thing Save
// has no counterpart for: a save VERSION, which picks the creature id's
// width in the army tail - one byte below version 25, two from 25 on.

// The two customization flags are NORMALIZED on the way in (`setne`), not
// copied, where every other scalar is a straight assignment. The three
// list counts arrive as single signed bytes and are widened before resize.

// The three lists do not read alike, and the asymmetry is retail's:
// secondary skills and spells come in as SIGNED bytes through checked
// reads, while each artifact is an UNCHECKED one-byte read into a dword
// local that is then masked - the same asymmetric artifact crossing
// loadMonsterList has.

// 89.8635 -> 94.4115 -> 95.0000 (2026-08-21): the residual really was the
// sequential /Ob2 budget. Retail CALLS vector<SecondarySkillData>::erase out
// of the first resize and, one level later, calls `copy` inside the artifact
// vector's resize; the original compile expanded both. Lifting the seven
// resource reads and four primary-skill reads into the one-call helpers above
// crosses the first boundary (94.4115). Lifting the two dword bonuses as the
// third, smaller dose crosses the nested copy boundary too: predict-inline's
// direct call ledgers are now exact, 9 against 9.

// Residual (95.0000%): the dword helper also changes two early failure exits.
// Retail keeps an epilogue after each read; this compile inlines the helper
// and merges its two `return -1` paths, so branch counts happen to agree
// 46/46 but base has 8 returns against retail's 10 and branch #4/#5 have the
// opposite fall-through sense. Direct dword reads restore retail's 10 exits
// but leave `copy` expanded and score 94.4115, so the higher call-exact phase
// is retained under the MAX rule. Calibrated negatives: primary helper alone
// 89.8380, resource helper alone byte-flat at 89.8635, and replacing the
// dword helper with the branch-free artifact-list helper overshoots to
// 89.7463 (10 calls vs retail 9, 45 branches vs 46). The three-helper dose is
// the measured peak; do not re-spend these boundaries without a new source
// fact.
// Typed-record recovery (2026-09-08, see readBlackBox): preserving the DC
// skill fields and artifact-vector element type gives 91.7100%, with the
// 95.0000 peak retained in HIST. The checked signed skill-byte reads and
// unchecked, masked artifact-byte read remain distinct at this boundary.
VA(0x00500430, 0x478)  // order-map: calls armyGroup::load + Initialize + loadString 0x4bb990 (loadTreasureData inlined); sole caller loadBlackBoxList (DC-isomorphic), dc 0xef158
int NewfullMap::loadBlackBox(TAbstractFile* infile, BlackBoxData* thisBox,
                             int saveVersion)
{
    signed char value;
    int count;
    int i;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisBox->m_hasCustomTreasure = value != 0;
    if (thisBox->m_hasCustomTreasure) {
        if (loadTreasureData(infile, thisBox) < 0)
            return -1;
    }

    if (loadBlackBoxDwordBonuses(infile, thisBox) < 0)
        return -1;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisBox->m_moraleBonus = value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisBox->m_luckBonus = value;

    if (loadBlackBoxResources(infile, thisBox) < 0)
        return -1;
    if (loadBlackBoxPrimarySkills(infile, thisBox) < 0)
        return -1;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    count = value;
    thisBox->m_secondarySkills.resize(count);
    for (i = 0; i < count; ++i) {
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            return -1;
        int skillType = value;
        memcpy(&thisBox->m_secondarySkills[i].m_type,
               &skillType, sizeof(skillType));
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            return -1;
        int skillLevel = value;
        memcpy(&thisBox->m_secondarySkills[i].m_level,
               &skillLevel, sizeof(skillLevel));
    }

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    count = value;
    thisBox->m_artifacts.resize(count);
    for (i = 0; i < count; ++i) {
        int artifact;
        infile->read(&artifact, sizeof(unsigned char));
        thisBox->m_artifacts[i] = artifactFromInt(artifact & 0xff);
    }

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    count = value;
    thisBox->m_spells.resize(count);
    for (i = 0; i < count; ++i) {
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            return -1;
        thisBox->m_spells[i] = value;
    }

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    count = value;
    thisBox->m_creatures.initialize();
    for (i = 0; i < count; ++i) {
        if (saveVersion < 25) {
            signed char narrow;
            infile->read(&narrow, sizeof(narrow));
            thisBox->m_creatures.m_armies[i] = narrow;
        } else {
            short wide;
            infile->read(&wide, sizeof(wide));
            thisBox->m_creatures.m_armies[i] = wide;
        }
        short troops;
        if (infile->read(&troops, sizeof(troops)) < sizeof(troops))
            return -1;
        thisBox->m_creatures.m_numTroops[i] = troops;
    }
    return 0;
}

VA(0x005008b0, 0x27A)  // dc 0xef5dc
int NewfullMap::readEventData(TAbstractFile* infile, CObject* eventObject,
                              int mapVersion)
{
    int boxIndex = m_blackBoxes.size();

    eventObject->m_extraInfo = 0;

    BlackBoxData tempBox;
    if (readBlackBox(infile, &tempBox, mapVersion) != 0)
        return -1;

    unsigned char value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    eventObject->m_extraInfo = (eventObject->m_extraInfo & 0xfffc03ff)
        | ((value & 0xff) << 10);

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    eventObject->m_extraInfo = (eventObject->m_extraInfo & 0xfffbffff)
        | ((value & 1) << 18);

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    eventObject->m_extraInfo = (eventObject->m_extraInfo & 0xfff7ffff)
        | ((value & 1) << 19);

    if (boxIndex < 400) {
        m_blackBoxes.push_back(tempBox);
        eventObject->m_extraInfo ^= (eventObject->m_extraInfo ^ boxIndex) & 0x3ff;
    }

    unsigned char padding[4];
    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:2124
DC_ONLY(0xef7c8, 0x3F0)
int NewfullMap::readSeerData(void* infile, CObject* seerObject)
{
    // @stub
}

#endif  // @carcass

// The random arms differ by lane.  A primary skill is a flat Random(0, 3),
// but a secondary skill or a spell is drawn from a CANDIDATE LIST built on
// the spot - every skill the scenario has not disabled, or every spell that
// both belongs to a school and is not disabled - and then indexed by one
// Random over the list's length.  That list is a std::vector<int> built
// with push_back, which is why this body carries an EH frame and a vector
// teardown on both arms.

VA(0x00500b30, 0x2AE)  // dc 0xefbb8
int NewfullMap::readScholarData(TAbstractFile* infile, CObject* scholarObject)
{
    ScholarInfo* scholarInfo = &scholarObject->m_scholarInfo;

    signed char value;
    int count;
    count = infile->read(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;

    unsigned char isRandom = 0;
    if (value == -1) {
        scholarInfo->m_award = random(0, 2);
        isRandom = 1;
    } else {
        scholarInfo->m_award = value;
    }

    count = infile->read(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;

    scholarInfo->m_primary = 3;
    scholarInfo->m_secondary = -1;
    scholarInfo->m_spell = -1;

    switch (scholarInfo->m_award) {
    case const_scholar_primary_skill:
        if (isRandom)
            scholarInfo->m_primary = random(0, 3);
        else
            scholarInfo->m_primary = value;
        break;

    case const_scholar_secondary_skill:
        if (isRandom) {
            std::vector<int> candidates;
            for (int skill = 0; skill < 28; ++skill) {
                if (!g_game->m_ssDisabled[skill])
                    candidates.push_back(skill);
            }
            scholarInfo->m_secondary =
                candidates[random(0, candidates.size() - 1)];
        } else {
            scholarInfo->m_secondary = value;
        }
        break;

    case const_scholar_spell:
        if (isRandom) {
            std::vector<int> candidates;
            for (int spell = 0; spell < 70; ++spell) {
                if (g_spellTraits[spell].m_schoolBits
                    && !g_game->m_spellDisabledInfo[spell])
                    candidates.push_back(spell);
            }
            scholarInfo->m_spell =
                candidates[random(0, candidates.size() - 1)];
        } else {
            scholarInfo->m_spell = value;
        }
        break;
    }

    unsigned char padding[6];
    count = infile->read(padding, sizeof(padding));
    return count < sizeof(padding) ? -1 : 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:2383
DC_ONLY(0xefe28, 0x1C2)
int NewfullMap::readShipyardData(void* infile, CObject* shipyardObject)
{
    // @stub
}

#endif  // @carcass

// The twelve adjacent squares LoadShipyards tests, in retail order. The
// body strength-reduces the indexed walk to a pointer at +4 (the first dy),
// reads dx from -4, and advances eight bytes through each pair. Retail's
// writable .data bytes at 0x67faa8..0x67fb07 prove every value.
DATA(0x0067faa8)
static int g_shipyardOffsets[12][2] = {
    { -2,  0 }, {  2,  0 }, { -2,  1 }, {  2,  1 },
    { -1,  1 }, {  1,  1 }, {  0,  1 }, { -2, -1 },
    {  2, -1 }, { -1, -1 }, {  1, -1 }, {  0, -1 }
};

// For every trigger shipyard, the first valid adjacent square in
// gShipyardOffsets order must be water, unblocked, and either non-triggering
// or a boat. Its coordinates are then copied into the ShipyardInfo overlay of
// every shipyard cell in the object's three-wide horizontal footprint.
VA(0x00500de0, 0x239)
void NewfullMap::loadShipyards()
{
    type_point newPoint;

    for (int z = 0; z < m_hasTwoLevels + 1; ++z) {
        for (int y = 0; y < g_mapHeight; ++y) {
            for (int x = 0; x < g_mapWidth; ++x) {
                NewmapCell* cell = &m_cellData[(z * m_size + y) * m_size + x];
                if (!cell->m_isTrigger || cell->m_type != SHIPYARD)
                    continue;

                newPoint.m_z = z;
                for (int count = 0; count < 12; ++count) {
                    newPoint.m_x = x + g_shipyardOffsets[count][0];
                    newPoint.m_y = y + g_shipyardOffsets[count][1];
                    if (!newPoint.isValid())
                        continue;

                    NewmapCell* boatCell = &m_cellData[
                        (newPoint.m_z * m_size + newPoint.m_y) * m_size + newPoint.m_x];
                    if (boatCell->m_groundSet == eTerrainWater
                        && !boatCell->m_isBlocked
                        && (!boatCell->m_isTrigger
                            || boatCell->m_type == BOAT)) {
                        for (int checkX = x - 1; checkX <= x + 1; ++checkX) {
                            if (checkX < 0 || checkX >= g_mapWidth)
                                continue;
                            NewmapCell* shipyardCell =
                                &m_cellData[(z * m_size + y) * m_size + checkX];
                            if (shipyardCell->m_type != SHIPYARD)
                                continue;
                            ShipyardInfo* shipyardInfo =
                                static_cast<ShipyardInfo*>(static_cast<void*>(
                                    &shipyardCell->m_extraInfo));
                            shipyardInfo->m_boatX = newPoint.m_x;
                            shipyardInfo->m_boatY = newPoint.m_y;
                        }
                        break;
                    }
                }
            }
        }
    }
}

VA(0x00501020, 0x110)  // dc 0xeffec
int NewfullMap::readMineData(TAbstractFile* infile, CObject* mineObject)
{
    mine tempMine;
    signed char owner;
    char padding[3];

    if (static_cast<unsigned>(infile->read(&owner, 1)) < 1)
        return -1;
    tempMine.m_playerOwner = owner;
    if (static_cast<unsigned>(infile->read(padding, 3)) < 3)
        return -1;

    CObjectType& objectType = m_objectTypes[mineObject->m_typeIndex];
    if (objectType.m_objectType != LIGHTHOUSE)
        tempMine.m_type = static_cast<char>(objectType.m_extra);
    else
        tempMine.m_type = 100;

    int x;
    int y;
    mineObject->findTrigger(x, y);
    tempMine.m_mapX = static_cast<unsigned char>(x);
    tempMine.m_mapY = static_cast<unsigned char>(y);
    tempMine.m_mapZ = mineObject->m_z;

    g_game->m_mines.push_back(tempMine);
    mineObject->m_extraInfo = g_game->m_mines.size() - 1;
    return 0;
}

VA(0x00501130, 0x132)  // dc 0xf013c
int NewfullMap::readAbandonedMineData(TAbstractFile* infile,
                                      CObject* mineObject)
{
    mine tempMine;
    int mineTypes;
    char padding[3];

    if (static_cast<unsigned>(infile->read(&mineTypes, 1)) < 1)
        return -1;

    int typeCount = 0;
    int type = 0;
    int availableTypes = mineTypes & 0xff;
    for (; type < 8; ++type) {
        if (availableTypes & (1 << type))
            ++typeCount;
    }

    int selected = random(1, typeCount);
    int found = 0;
    type = 0;
    availableTypes = mineTypes & 0xff;
    for (; type < 8; ++type) {
        if (availableTypes & (1 << type)) {
            ++found;
            if (found == selected)
                break;
        }
    }
    tempMine.m_type = static_cast<char>(type);

    if (static_cast<unsigned>(infile->read(padding, 3)) < 3)
        return -1;

    int x;
    int y;
    mineObject->findTrigger(x, y);
    tempMine.m_mapX = static_cast<unsigned char>(x);
    tempMine.m_mapY = static_cast<unsigned char>(y);
    tempMine.m_mapZ = mineObject->m_z;
    tempMine.m_isAbandoned = 1;

    g_game->m_mines.push_back(tempMine);
    mineObject->m_extraInfo = g_game->m_mines.size() - 1;
    return 0;
}

VA(0x00501270, 0x138)  // dc 0xf02a4
int NewfullMap::readSignData(TAbstractFile* infile, CObject* signObject)
{
    Sign tempSign;
    char padding[4];

    if (NewSMapHeader::readString(infile, tempSign.m_signText) > 0)
        tempSign.m_hasText = 1;

    g_game->m_signs.push_back(tempSign);
    signObject->m_extraInfo = g_game->m_signs.size() - 1;

    if (static_cast<unsigned>(infile->read(padding, 4)) < 4)
        return -1;
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:2579
#endif  // @carcass

// The map file's wandering-monster record.  Everything it learns is packed
// into the object's extraInfo, one masked insert per field: the troop count
// at bits 0..11, the army-size grade at 12..16, the custom-record index at
// 19..26 with its marker at 31, and the never-flees / no-growth flags above.

// The army-size grade is a five-way dispatch, and four of its arms roll:
// grade 0 is the sentinel -4, grades 1..3 draw Random(1,7), Random(1,10) and
// Random(4,10), grade 4 is a flat 10, and anything past 4 passes the stream
// byte through unchanged.  The jump table is retail's.

// Two version tests, both on gpGame->mapHeader.version rather than a
// parameter: the leading identifier dword is skipped entirely on the oldest
// map format, and the custom record's artifact is a byte there and a short
// after.

// The tail packs the object's own coordinates into a type_point and hands it
// to the game with the identifier read at the top - which is what the odd
// `mov ecx,[ebp-0x18] / mov [ebp-0x18],ecx` self-move near the entry is for:
// the dword survives the whole body to reach that call.  The 16-bit
// xor-merges at the end are type_point's bitfields (x:10, y:10, z:4), not a
// write back into the object, and the two bytes read just before are
// discarded padding.

// Residual (96.4729%, nested-inliner wall): retail inlines MonsterData's
// constructor but leaves its string::_Tidy child out of line.  This VC6
// invocation expands both.  A declaration-site inline_depth(1) is byte-flat;
// inline_depth(0) calls the entire MonsterData constructor instead (96.63%,
// but the wrong retail boundary), while pinning the block's error return
// regresses to 90.0370.  There is no pragma depth that means "inline the
// parent, call only its child" at this site, so the canonical constructor is
// retained.
// 2026-09-06, polish lane 36 (97.0513 -> 97.1225), the DC LOCAL-SCOPE SWEEP:
// the Dreamcast block names ONE `char_buffer` (T_UCHAR, sp+0x12) for the three
// flag bytes this body read into `hasCustomRecord`, `neverFlees` and
// `noGrowth`; sharing the one local is worth the 0.07 above.  Its `disposition`
// (T_RCHAR) is this body's `grade` and its `short_buffer` is `quantity`, both
// already the right signedness.  Still open in that block: the DC also names a
// single `int_buffer` where this body has `identifier`, `rawIdentifier`,
// `quantityRead` and `artifact`, and a single `ListSize` for `customIndex`.
// Hoisting one shared int for the resource and artifact reads regressed to
// 96.84 by extending its lifetime without shrinking the frame; the
// `rawIdentifier` split above is banked at +0.58 so a collapse must beat that.
VA(0x005013b0, 0x3DC)  // order-map: calls Random 0x50b230 + readString 0x4c6010 + vector<MonsterData> grow 0x506d70; called by readObject; EH-bearing, dc 0xf0390
int NewfullMap::readMonsterData(TAbstractFile* infile, CObject* monsterObject)
{
    int customIndex = m_customMonsterList.size();

    monsterObject->m_extraInfo = 0;

    int identifier;
    if (g_game->m_mapHeader.m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        identifier = 0;
    } else {
        // The else arm reads into its OWN local and assigns: retail's
        // `mov ecx,[ebp-0x18] / mov [ebp-0x18],ecx` at the join is that
        // copy after the allocator coalesced the two slots (+0.58).
        // Reading straight into `identifier` loses the pair; hoisting the
        // zero out of the if arm scores 94.95 and the whole-block rewrite
        // 94.96.
        int rawIdentifier;
        infile->read(&rawIdentifier, sizeof(rawIdentifier));
        identifier = rawIdentifier;
    }

    short quantity;
    if (infile->read(&quantity, sizeof(quantity)) < sizeof(quantity))
        return -1;
    monsterObject->m_extraInfo = (monsterObject->m_extraInfo & 0xfffff000)
        | (quantity & 0xfff);

    // DC names one `disposition` byte. Sharing it as the switch result adds
    // retail's missing block and raises 97.12 -> 97.29. A separate promoted
    // selector is optimized back to the two-byte 97.12 shape.
    signed char disposition;
    if (infile->read(&disposition, sizeof(disposition)) < sizeof(disposition))
        return -1;

    switch (disposition) {
    case MONSTER_QTY_UNRESOLVED:
        disposition = -4;
        break;
    case MONSTER_QTY_RANDOM_1_7:
        disposition = static_cast<signed char>(random(1, 7));
        break;
    case MONSTER_QTY_RANDOM_1_10:
        disposition = static_cast<signed char>(random(1, 10));
        break;
    case MONSTER_QTY_RANDOM_4_10:
        disposition = static_cast<signed char>(random(4, 10));
        break;
    case MONSTER_QTY_FIXED_10:
        disposition = 10;
        break;
    default:
        break;
    }
    monsterObject->m_extraInfo = (monsterObject->m_extraInfo & 0xfffe0fff)
        | ((disposition & 0x1f) << 12);

    unsigned char charBuffer;
    if (infile->read(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;

    if (charBuffer) {
        // The Artifact = ARTIFACT_NONE store is MonsterData's constructor.
        MonsterData tempMonster;
        NewSMapHeader::readString(infile, tempMonster.m_message);

        // MEASURED AND REJECTED: `#pragma inline_depth(0)` on this `return`.
        // predict-inline says retail CALLS basic_string::_Tidy once here and
        // we expand it, which is game::Load's return-pin shape exactly - but
        // the local whose scope this exits is BLOCK-scoped, not
        // function-scoped, and the pin costs 96.4729 -> 90.0370.
        for (int i = 0; i < 7; ++i) {
            int quantityRead;
            if (infile->read(&quantityRead, sizeof(quantityRead))
                < sizeof(quantityRead))
                return -1;
            tempMonster.m_resQty[i] = quantityRead;
        }

        int artifact;
        if (g_game->m_mapHeader.m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
            signed char narrow;
            infile->read(&narrow, sizeof(narrow));
            artifact = narrow;
        } else {
            short wide;
            infile->read(&wide, sizeof(wide));
            artifact = wide;
        }
        tempMonster.m_artifact = artifact;

        if (customIndex < 4000) {
            m_customMonsterList.push_back(tempMonster);
            monsterObject->m_extraInfo = (((customIndex & 0xff) | 0xfffff000)
                                        << 19)
                | (monsterObject->m_extraInfo & 0xf807ffff);
        }
    }

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    monsterObject->m_extraInfo = (monsterObject->m_extraInfo & 0xfffdffff)
        | ((charBuffer & 1) << 17);

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    // The mask retail computes clears bits 27..30 alongside bit 18, so this
    // write lands on more than the one flag; transcribed as the object does
    // it rather than narrowed to the single bit.
    monsterObject->m_extraInfo = (monsterObject->m_extraInfo & 0x87fbffff)
        | ((charBuffer & 1) << 18);

    unsigned char padding[2];
    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;

    type_point point;
    point.m_x = monsterObject->m_x;
    point.m_y = monsterObject->m_y;
    point.m_z = monsterObject->m_z;
    g_game->recordMonsterIdentifier(identifier, point);
    return 0;
}

// DC mapcell.cpp:2695 records this ordinary public member.
// Complete expands it in Save; retained-body absence does not make it static.
// The later file interface replaces DC gzwrite through void*.
int NewfullMap::saveMonsterList(TAbstractFile* outfile)
{
    int count = m_customMonsterList.size();
    if (static_cast<unsigned>(outfile->write(&count, 2)) < 2)
        return -1;
    for (unsigned int i = 0; i < m_customMonsterList.size(); ++i) {
        if (saveMonsterData(outfile, &m_customMonsterList[i]) < 0)
            return -1;
    }
    return 0;
}

// E:\gamedcs\mapcell.cpp:2768 - the DC roster carries this as a member of
// NewfullMap, but retail's only call site inlines it and /OPT:REF then drops
// the out-of-line copy, so no carve row exists to claim.  It is spelled as a
// file-local static for that reason: an inlined single-call static leaves no
// symbol behind, where an extern member would leave one retail has not got.

// It must stay a separate function rather than being written longhand inside
// loadMonsterList.  Longhand, the caller's front-end mass lifts the /Ob2
// budget off its 1000 floor and VC6 inlines vector<MonsterData>::erase into
// resize, where retail calls it.

// The artifact crossing is the asymmetric one.  saveMonsterData narrows the
// int to a byte, so ARTIFACT_NONE goes out as 0xff; the load reads one byte
// into a dword local and masks it, which is why the field is re-widened
// through `& 0xff` and mapped back onto ARTIFACT_NONE rather than
// sign-extended.  That final read carries no short-read gate in retail.
static int loadMonsterData(TAbstractFile* infile, MonsterData* thisMonster)
{
    game::loadString(infile, thisMonster->m_message);

    for (int i = 0; i < 7; ++i) {
        int value;
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            return -1;
        thisMonster->m_resQty[i] = value;
    }

    int artifact;
    infile->read(&artifact, sizeof(unsigned char));
    thisMonster->m_artifact = artifact & 0xff;
    if (thisMonster->m_artifact == (ARTIFACT_NONE & 0xff))
        thisMonster->m_artifact = ARTIFACT_NONE;
    return 0;
}

VA(0x00501790, 0x1E3)  // dc 0xf0788
int NewfullMap::loadMonsterList(TAbstractFile* infile)
{
    short count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_customMonsterList.resize(count);
    for (unsigned int i = 0; i < m_customMonsterList.size(); ++i) {
        if (loadMonsterData(infile, &m_customMonsterList[i]) < 0)
            return -1;
    }
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

#endif  // @carcass

VA(0x00501980, 0x6B)  // dc 0xf0824
int NewfullMap::saveMonsterData(TAbstractFile* outfile, MonsterData* thisMonster)
{
    game::saveString(outfile, &thisMonster->m_message);

    for (int i = 0; i < 7; ++i) {
        int value = thisMonster->m_resQty[i];
        if (static_cast<unsigned>(outfile->write(&value, 4)) < 4)
            return -1;
    }

    unsigned char artifact = static_cast<unsigned char>(thisMonster->m_artifact);
    return static_cast<unsigned>(outfile->write(&artifact, 1)) < 1 ? -1 : 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:2768
DC_ONLY(0xf08b8, 0x94)
int NewfullMap::loadMonsterData(void* infile, MonsterData* thisMonster)
{
    // @stub
}

#endif  // @carcass

// readTownData's two nine-byte spell masks share this /Ob2 codegen helper.
// It costs no symbol because VC6 expands it at both call sites.  readHeroData
// deliberately keeps the analogous loop in its body: that common inlining
// context lets VC6 share the two bitset<70>::_Xran exception-object homes and
// is part of its current 91.6097% result (see the residual note there).

// The `/ 8` and `% 8` are SIGNED: retail's `and ecx,0x80000007` with its
// negative fixup, and the `cdq / and edx,7 / add / sar 3` pair, are what an
// `int` counter produces and a shift-and-mask spelling would not.

// THE TWO CALLERS SPELL THE BIT STORE DIFFERENTLY, AND THE BYTES SAY SO
// (2026-08-20).  A bitset<70> bit store reaches `_Xran` one level down through
// `set(pos, val)` and two levels further through `operator[]` ->
// `reference::operator=` -> `set`, and that DEPTH is what decides whether VC6
// leaves `_Xran` as a call or expands its whole throw path inline.  Retail
// readTownData CALLS bitset<70>::_Xran (0x4d1c80) exactly ONCE - one call
// covering two mask loops, the cross-jumper having merged the two
// argument-less throw blocks - and never reaches __CxxThrowException itself.
// Retail readHeroData is the exact mirror: no _Xran call at all, and
// __CxxThrowException@8 (0x617547) TWICE - one expansion for its mask loop and
// one for the single-spell store in the Armageddon's Blade arm.  So the two
// functions use different spellings:
//   readTownData   `set(i, v)`      82.0785 -> 94.3896  (+12.31)
//   readHeroData   `(*spells)[i]=v` 83.7667; `set(i, v)` costs it 83.7528,
//                  and `spells.set(spell, 1)` on the single-spell store costs
//                  2.26 more (83.7528 -> 81.4958).
// This RETIRES the "MEASURED AND REJECTED" probe readTownData used to carry:
// that one wrote a single mask longhand to split the two range checks apart,
// which is not what retail did - retail keeps both loops the same shape and
// moves the whole throw out of line by LOWERING THE STORE'S DEPTH.
// readTownData's spelling is `set`, one level down, so `_Xran` stays a CALL.
static void unpackTownSpellMask(std::bitset<70>* spells,
                                const unsigned char* mask)
{
    for (int i = 0; i < 70; ++i)
        spells->set(i, (mask[i / 8] & (1 << (i % 8))) != 0);
}

// E:\gamedcs\mapcell.cpp:232 - TTownEvent::Read in the Dreamcast roster
// (dc 0xebc90, locals `inBuf`, `padding`, `count`).  Retail's only call site
// is readTownData, which inlines it, so it is spelled as a file-local static
// for exactly the reason loadMonsterData above is: an inlined single-call
// static leaves no symbol behind where an extern member would leave one
// retail has not got.

// Its result is DISCARDED at the call site, and that is what explains the one
// asymmetry in the bytes: the trailing four-byte read carries NO short-count
// branch while the two reads above it do.  With the return value dead, both
// arms of the last `return -1` converge on the same code and the optimizer
// folds the test away; the two earlier ones survive because failing them
// SKIPS a later read.
static int readTownEvent(TAbstractFile* infile, TTownEvent* thisEvent,
                         int mapVersion)
{
    unsigned char inBuf[6];
    char padding[4];

    thisEvent->read(infile, mapVersion);

    if (infile->read(inBuf, sizeof(inBuf)) < sizeof(inBuf))
        return -1;
    memcpy(&thisEvent->m_buildBuildings, inBuf, sizeof(inBuf));

    if (infile->read(thisEvent->m_generatorBonuses,
                     sizeof(thisEvent->m_generatorBonuses))
        < sizeof(thisEvent->m_generatorBonuses))
        return -1;

    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;
    return 0;
}

// E:\gamedcs\mapcell.cpp:2798
// The h3m town record, and the fullest statement of this pool's layout there
// is: every offset game.h's gated view models is written here, and the total
// lands on the 136-byte stride the `imul 0x78787879 / sar 6` size() inline
// divides by.

// The record index goes straight into the OBJECT rather than into a local the
// way readArtifactData's does - it is taken BEFORE the push_back that will
// create the entry, so it is the index the entry is about to get.

// THREE version tests, and they do not read the same source.  The leading
// identifier dword and the obligatory-spell mask consult
// gpGame->mapHeader.version; the creature field's WIDTH and the trailing
// alignment byte consult the mapVersion PARAMETER.  That asymmetry is
// retail's, the same one readGarrisonData and readBlackBox carry.

// The two spell masks are read into ONE nine-byte buffer, which is why the
// oldest map format still memsets it before skipping the loop that would
// read it: the second Read overwrites the whole thing regardless.  Each mask
// is unpacked bit by bit, with `/ 8` and `% 8` SIGNED - the `and
// ecx,0x80000007` negative fixup and the `cdq / and edx,7 / add / sar 3`
// pair are what prove the loop counter is `int`; shifts and masks would not
// produce those corrections.

// The tail resolves a RANDOM_TOWN's faction: the alignment byte's player slot
// if that slot is playable at all, else the town's own owner, else a roll -
// widened to nine alignments only when the scenario is expansion-era.

// 94.3896 -> 95.1600 (2026-08-21): the town-event loop counts DOWN from
// `numTownEvents`, as retail's copy-to-counter / `dec` / `jne` transcript
// proves.  The former zero-up `count < numTownEvents` loop was the remaining
// source-level loop mismatch.

// Residual (95.1600%): the call multisets agree at 24/24 and the control-flow
// census now agrees at 54 conditional branches and 12 returns.  What remains
// is placement plus C1 register/stack homing: retail keeps `infile` in ESI and
// uses EDI for the mask/event loops, while this compile keeps `infile` in EDI
// and uses ESI for those loops.  Retail also recycles incoming parameter homes
// for the identifier/alignment scratch values where this compile uses negative
// locals.

// Dreamcast-local audit (2026-08-21): restoring its declaration order and
// reusing its sole `char_buffer` for the trailing alignment byte are both
// byte-flat at 95.1600% and are retained as the stronger source model.  Using
// that same roster literally for the army-width conversion - shared
// `char_buffer`/`short_buffer`/`int_buffer` instead of the scoped signed
// byte/short/value temporaries - regresses to 94.9926%; the x86 block locals
// below are retained.
VA(0x005019f0, 0x7CC)  // order-map: calls TTimedEvent::Read 0x4fc1a0 (TTownEvent::Read inlined) + bitset<70> throw helper + vector<TTownEvent> grow 0x508250 + vector<TownExtra> grow 0x508cf0; called by readObject; EH-bearing, dc 0xf094c
int NewfullMap::readTownData(TAbstractFile* infile, CObject* townObject,
                             int mapVersion)
{
    char padding[3];
    int intBuffer;
    TownExtra tempTown;
    short shortBuffer;
    int count;
    int x;
    int numTownEvents;
    unsigned char inBuf[6];
    char charBuffer;
    unsigned char spellBuf[9];

    townObject->m_extraInfo = g_game->m_scenarioTowns.size();
    tempTown.m_townType = m_objectTypes[townObject->m_typeIndex].m_extra;

    if (g_game->m_mapHeader.m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        tempTown.m_objRef = 0;
    } else {
        infile->read(&intBuffer, sizeof(intBuffer));
        tempTown.m_objRef = intBuffer;
    }

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    tempTown.m_playerOwner = charBuffer;

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    tempTown.m_customName = charBuffer;
    if (tempTown.m_customName)
        NewSMapHeader::readString(infile, tempTown.m_name);

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    tempTown.m_customArmies = charBuffer;
    if (tempTown.m_customArmies) {
        for (x = 0; x < armyGroup::ARMY_GROUP_SLOT_COUNT; ++x) {
            int creature;
            if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
                signed char narrow;
                infile->read(&narrow, sizeof(narrow));
                creature = narrow;
            } else {
                short wide;
                infile->read(&wide, sizeof(wide));
                creature = wide;
            }
            tempTown.m_townArmy.m_armies[x] = creature;

            if (infile->read(&shortBuffer, sizeof(shortBuffer))
                < sizeof(shortBuffer))
                return -1;
            tempTown.m_townArmy.m_numTroops[x] = shortBuffer;
        }
    }

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    tempTown.m_isGrouped = charBuffer;

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    tempTown.m_customBuildings = charBuffer;

    if (tempTown.m_customBuildings) {
        if (infile->read(inBuf, sizeof(inBuf)) < sizeof(inBuf))
            return -1;
        memcpy(&tempTown.m_buildingBuiltMask, inBuf, sizeof(inBuf));
        if (infile->read(inBuf, sizeof(inBuf)) < sizeof(inBuf))
            return -1;
        memcpy(&tempTown.m_buildingDisabledMask, inBuf, sizeof(inBuf));
    } else {
        if (infile->read(&charBuffer, sizeof(charBuffer))
            < sizeof(charBuffer))
            return -1;
        tempTown.m_hasFort = charBuffer;
    }

    if (g_game->m_mapHeader.m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        memset(spellBuf, 0, sizeof(spellBuf));
    } else {
        infile->read(spellBuf, sizeof(spellBuf));
        // The single retail `call bitset<70>::_Xran` at 0x501dbb covers BOTH
        // mask loops - see unpackTownSpellMask's note.  (An older probe wrote
        // this first mask longhand to split the two range checks apart and
        // measured 82.0785 -> 79.9793; the split was the wrong reading.)
        unpackTownSpellMask(&tempTown.m_fixedSpells, spellBuf);
    }

    if (infile->read(spellBuf, sizeof(spellBuf)) < sizeof(spellBuf))
        return -1;
    unpackTownSpellMask(&tempTown.m_spells, spellBuf);

    if (infile->read(&numTownEvents, sizeof(numTownEvents))
        < sizeof(numTownEvents))
        return -1;

    for (count = numTownEvents; count > 0; --count) {
        TTownEvent thisEvent;
        readTownEvent(infile, &thisEvent, mapVersion);
        thisEvent.m_townNum = g_game->m_scenarioTowns.size();
        m_townEventList.push_back(thisEvent);
    }

    charBuffer = -1;
    if (mapVersion >= 28)
        infile->read(&charBuffer, sizeof(charBuffer));

    if (m_objectTypes[townObject->m_typeIndex].m_objectType == RANDOM_TOWN) {
        if (charBuffer != -1
            && (g_game->m_mapHeader.m_playerSlotAttributes[charBuffer].m_canBeHuman
                || g_game->m_mapHeader.m_playerSlotAttributes[charBuffer]
                       .m_canBeComputer)) {
            tempTown.m_townType = g_game->m_setup.m_alignment[charBuffer];
        } else if (tempTown.m_playerOwner != -1) {
            tempTown.m_townType = g_game->m_setup.m_alignment[tempTown.m_playerOwner];
        } else {
            // Eight town bits, widened to the ninth only for an
            // expansion-era scenario in the two game states that allow it.

            // The enumerator reads oddly here because smackmgr named this
            // domain from its OWN use of it - the two values that force a
            // video onto the bink arm - and that header says so: the names
            // are provisional until the pointee's owning TU lands. It is
            // the same global (0x69923c) and the same value 3, so the gate's
            // rule applies. Two independent readings now constrain the
            // domain: this one pairs 1 with 3, and PointToSpriteResource
            // (0x55cf50) uses the very same double indirection to INDEX the
            // 24-byte archive-set rows at 0x69e538 - so the pointee is a
            // small game/resource-context ordinal, not a video flag.
            int legalAlignments = 0xff;
            if ((*g_videoGameState == 1
                 || *g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH)
                && g_game->m_f1f698 >= 1)
                legalAlignments = 0x1ff;
            tempTown.m_townType = pickAlignment(legalAlignments, 0);
        }
    }

    g_game->m_scenarioTowns.push_back(tempTown);

    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;
    return 0;
}

// E:\gamedcs\mapcell.cpp:2951
// The map file's hero record, and the widest one this compiland reads: it
// fills a whole HeroExtra out of gpGame's 156-record setup pool at +0xa4 and
// leaves the object's extraInfo holding the hero id.

// THREE version gates, and they do not read the same source.  The leading
// quest identifier and the experience block test the mapVersion PARAMETER;
// the armies width tests the parameter again out of its stack home;
// everything from the artifacts on tests gpGame->mapHeader.version.  That is
// readBlackBox's and readTownData's asymmetry again, not a slip.

// EXACTLY ONE READ IS CHECKED - the sixteen trailing padding bytes, whose
// short count is the function's only -1.  Twenty-eight other Reads ignore
// their result, including every one whose value reaches the record.

// The hero id is resolved in three steps: the stream byte through the
// 0x4ba1c0 helper, then the per-player reservation at 0x69fb24 (CONSUMED on
// use - the slot is stored -1 again), then GetStartingHeroId as the fallback.
// Whichever answer wins is also latched into the setup screen's startingHero
// slot if that is still -1.

// The two spell formats are exclusive and THE PRIMARY SKILLS RIDE WITH THE
// SECOND: Armageddon's Blade carries one signed spell id (-2 = no record,
// -1 = clear the set) and jumps straight to the padding, while every later
// format carries a 70-bit mask in nine bytes and then the four primaries.

// The alignment reaching GetStartingHeroId is moved into its enum with
// memcpy rather than a cast, which is this tree's own idiom for the
// conversion - pick_alignment does exactly the same thing to its loop index,
// and the cast-into-an-enum floor is why.

// Historical local maximum (2026-08-20/21): a hero-name call-site pin,
// block-scoped reads, the const-reference string return, and three
// caller-shrink statics reached 91.6097%.  Restoring the source-level
// `heroObject->get_trigger()` tail then reached 92.7472% and a 0xd4 frame.
// That score is preserved as MAX/history, not as permission to keep the
// statics: Dreamcast gives the secondary-skill, army, equipped-artifact and
// backpack statements to THIS function, records one shared `int x`, and has
// no function rows for readHeroSecondarySkills/readHeroArmies.  Removing all
// three invented helpers restores that positive shape and intentionally puts
// the current checkpoint at 85.2833%.

// The next structure pass reads the raw symbol nesting, not only the compact
// roster: all thirteen DC locals appear under S_GPROC32 before the first
// S_BLOCK32. Restoring one function-scope int_buffer, short_buffer and
// char_buffer (and reusing them across the reads) moves the current checkpoint
// to 82.9570%. MAX/history remains 92.7472%. HeroID stays the Complete x86
// representation `int` until the 130-entry DC THeroID domain can be reconciled
// with Complete's 156-entry roster; the local name, width and lifetime are
// already preserved.

// Current structural frontier (2026-08-31): candidate and retail each have
// 53 branches and two returns, but enough targets differ that positional
// pairing is no longer meaningful.  The candidate frame is 0xd8 against
// retail's 0xc4.  Its emitted call stream has 14 calls against retail's 13;
// after unclaimed-name pairs are discounted, the one real census difference
// is the two nested `logic_error(const string&)` constructors that retail
// calls and this larger caller expands.  That is an over-inline consequence
// to solve through a real source/header boundary or later coherent source
// mass—not by recreating source-false caller-shrink helpers.

// Two facts remain deliberate.  Retail reaches __CxxThrowException@8 twice
// and never calls bitset<70>::_Xran, so both bit stores use `operator[]` ->
// `reference::operator=` -> `set`; spelling either as `set(pos,val)` keeps the
// throw out of line.  Retail also CALLS three-argument basic_string::assign at
// the hero-name store, so the statement-scoped inline_depth(0) below prevents
// expansion without de-inlining ReadLengthPrefixedString.  Complete's decoded
// body has no Random relocation: unlike Dreamcast, an absent custom experience
// stays zero and is passed unchanged to GetStartingHeroId.

// Historical probes, measured before the recovered inline graph changed:
//   * lifting the whole post-Restoration tail (54.6514%) or only the
//     Armageddon's-Blade spell arm (88.8320%) overshot the caller-shrink peak;
//   * `spells.reset()` reached 84.4847%; a default-constructed `noSpells`
//     reached 88.6583%; return-by-value `emptyHeroSpellSet()` and named
//     `noSpells(0)` were byte-flat;
//   * after the direct trigger helper landed, narrowing the name pragma and
//     naming `noSpells` were byte-flat; `padding` before `tempText` was also
//     byte-flat and is retained because Dreamcast records that order;
//   * widening hero_data's scope was byte-flat and was reverted, while moving
//     isRandomHero before HeroID lost 92.7472 -> 92.3945 and was reverted.
// Re-test a rejected codegen probe only after an upstream inline-graph change;
// none of these measurements is evidence against the recovered source shape.
VA(0x005021c0, 0x835)  // order-map: calls GetStartingHeroId 0x4bb400 (DC-unique callee) + FindTrigger 0x4fec30 (get_trigger inlined); called by readObject, dc 0xf0df4
int NewfullMap::readHeroData(TAbstractFile* infile, CObject* heroObject,
                             int mapVersion)
{
    // Dreamcast's raw CodeView records all thirteen source locals directly
    // under S_GPROC32, before the first S_BLOCK32: these buffers and counters
    // are function-scoped and reused, not a family of block-local scratch
    // declarations. Complete adds a few revision-only values below, but does
    // not erase that positive shared-source fact.
    char padding[16];
    unsigned char isRandomHero;
    int heroID;  // Dreamcast type: THeroID; Complete x86 stores a full int.
    int intBuffer;
    short shortBuffer;
    char owner;
    char customName;
    int count;
    int experience;
    int x;
    char charBuffer;
    char tempText[100] = { 0 };
    HeroExtra* heroData;

    // The Shadow of Death quest identifier: absent on the oldest format, and
    // it survives the whole body to reach HeroExtra::field_008.
    int identifier;
    if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        identifier = 0;
    } else {
        infile->read(&intBuffer, sizeof(intBuffer));
        identifier = intBuffer;
    }

    infile->read(&charBuffer, sizeof(charBuffer));
    owner = charBuffer;

    heroID = readHeroId(infile, mapVersion);
    isRandomHero = 0;
    if (heroID == -1)
        isRandomHero = 1;

    infile->read(&charBuffer, sizeof(charBuffer));
    customName = charBuffer != 0;
    if (customName) {
        infile->read(&intBuffer, sizeof(intBuffer));
        infile->read(tempText, intBuffer);
        tempText[intBuffer] = 0;
    }

    // Restoration of Erathia and Armageddon's Blade always carry the
    // experience dword; Shadow of Death gates it behind a flag byte.  In a
    // campaign game a value below forty is treated as no custom experience at
    // all, which is what the gbUnk69774c consult is doing on both arms.
    unsigned char customExperience;
    if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA
        || mapVersion == MAP_FORMAT_ARMAGEDDONS_BLADE) {
        infile->read(&intBuffer, sizeof(intBuffer));
        experience = intBuffer;
        if (experience != 0 && (!g_unk69774c || experience >= 40))
            customExperience = 1;
        else
            customExperience = 0;
    } else {
        char experienceFlag;
        infile->read(&experienceFlag, sizeof(experienceFlag));
        customExperience = experienceFlag != 0;
        if (customExperience) {
            infile->read(&experience, sizeof(experience));
            if (g_unk69774c && experience < 40)
                customExperience = 0;
        } else {
            experience = 0;
        }
    }

    if (heroID == -1) {
        if (g_unnamed69fb24[charBuffer] != -1) {
            heroID = g_unnamed69fb24[charBuffer];
            g_unnamed69fb24[charBuffer] = -1;
        } else {
            TTownType alignment;
            memcpy(&alignment, &g_game->m_setup.m_alignment[charBuffer],
                   sizeof(alignment));
            heroID = g_game->getStartingHeroId(alignment, charBuffer,
                                               experience);
        }
    }
    if (g_game->m_setup.m_startingHero[charBuffer] == -1)
        g_game->m_setup.m_startingHero[charBuffer] = heroID;

    heroData = &g_game->m_heroSetup[heroID];
    heroData->m_owner = owner;
    heroData->m_id = heroID;
    heroData->m_objRef = identifier;
    heroObject->m_extraInfo = heroID;

    // Dreamcast first stores `customName` into bCustomName, then copies only
    // for a non-random hero with `strncpy(Name, tempText, 12)`. Complete
    // changed this group: retail has one customName guard, stores 1, then
    // performs the runtime-NUL scan and strlen+1 copy of an inlined strcpy.
    // Our emitted 15-instruction copy sequence is instruction-identical to
    // retail, so the older bounded copy and isRandomHero guard are DC-only.
    if (customName) {
        heroData->m_hasCustomName = 1;
        strcpy(heroData->m_nameBuffer, tempText);
    }

    if (customExperience) {
        heroData->m_customExperience = 1;
        heroData->m_experience = experience;
    }

    // A random hero keeps its rolled portrait unless the campaign engine is
    // running, which is the only reader of the flag byte pair.
    infile->read(&charBuffer, sizeof(charBuffer));
    if (charBuffer) {
        infile->read(&charBuffer, sizeof(charBuffer));
        if (!isRandomHero || g_unk69774c) {
            heroData->m_customPortraitNumber = 1;
            heroData->m_portraitNumber = charBuffer;
        }
    }

    infile->read(&charBuffer, sizeof(charBuffer));
    if (charBuffer) {
        heroData->m_customSecondarySkills = 1;
        infile->read(&intBuffer, sizeof(intBuffer));
        heroData->m_numSecondarySkills = intBuffer;
        for (x = 0; x < heroData->m_numSecondarySkills; ++x) {
            infile->read(&charBuffer, sizeof(charBuffer));
            heroData->m_secondarySkill[x] = charBuffer;
            infile->read(&charBuffer, sizeof(charBuffer));
            heroData->m_secondarySkillLevel[x] = charBuffer;
        }
    }

    infile->read(&charBuffer, sizeof(charBuffer));
    if (charBuffer) {
        heroData->m_customArmies = 1;
        for (x = 0; x < armyGroup::ARMY_GROUP_SLOT_COUNT; ++x) {
            if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
                infile->read(&charBuffer, sizeof(charBuffer));
                intBuffer = charBuffer;
            } else {
                infile->read(&shortBuffer, sizeof(shortBuffer));
                intBuffer = shortBuffer;
            }
            heroData->m_armies[x] = intBuffer;

            infile->read(&shortBuffer, sizeof(shortBuffer));
            heroData->m_numTroops[x] = shortBuffer;
        }
    }

    infile->read(&charBuffer, sizeof(charBuffer));
    heroData->m_groupFormation = charBuffer != 0;

    infile->read(&charBuffer, sizeof(charBuffer));
    if (charBuffer) {
        heroData->m_customArtifacts = 1;

        // Eighteen equipped positions before Shadow of Death, nineteen after.
        count = (g_game->m_mapHeader.m_version
                 == MAP_FORMAT_SHADOW_OF_DEATH) + 18;
        for (x = 0; x < count; ++x) {
            if (g_game->m_mapHeader.m_version
                == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
                infile->read(&charBuffer, sizeof(charBuffer));
                intBuffer = charBuffer;
            } else {
                infile->read(&shortBuffer, sizeof(shortBuffer));
                intBuffer = shortBuffer;
            }
            memcpy(&heroData->m_artifacts[x].m_artifactId, &intBuffer,
                   sizeof heroData->m_artifacts[x].m_artifactId);
        }

        infile->read(&shortBuffer, sizeof(shortBuffer));
        heroData->m_numInBackpack = static_cast<unsigned char>(shortBuffer);
        for (x = 0; x < heroData->m_numInBackpack; ++x) {
            if (g_game->m_mapHeader.m_version
                == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
                infile->read(&charBuffer, sizeof(charBuffer));
                intBuffer = charBuffer;
            } else {
                infile->read(&shortBuffer, sizeof(shortBuffer));
                intBuffer = shortBuffer;
            }
            memcpy(&heroData->m_backpack[x].m_artifactId, &intBuffer,
                   sizeof heroData->m_backpack[x].m_artifactId);
        }

        // The fourth war-machine position is never serialized: every hero
        // starts with the catapult in it.
        heroData->m_artifacts[hero::EQUIPPED_SLOT_WAR_MACHINE_4].m_artifactId
            = ARTIFACT_CATAPULT;
        heroData->m_artifacts[hero::EQUIPPED_SLOT_WAR_MACHINE_4].m_extra = -1;
    }

    infile->read(&charBuffer, sizeof(charBuffer));
    heroData->m_patrolRadius = charBuffer;

    if (g_game->m_mapHeader.m_version != MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        infile->read(&charBuffer, sizeof(charBuffer));
        if (charBuffer) {
            heroData->m_customName = 1;
            // BOUND BY const REFERENCE, not copied.  ReadLengthPrefixedString
            // returns by value and VC6 does not elide the copy into a named
            // `std::string` local, so that spelling puts TWO 16-byte string
            // objects on the frame where retail has one - worth 1.67 and 16
            // frame bytes (0xf4 -> 0xe4).  C++98 extends the temporary's
            // lifetime to the reference's scope, which is exactly the block
            // the assign sits in.
            const std::string& heroName = readLengthPrefixedString(infile);
#pragma inline_depth(0)
            heroData->m_name.assign(heroName, 0, std::string::npos);
#pragma inline_depth()
        }

        infile->read(&charBuffer, sizeof(charBuffer));
        if (charBuffer != -1)
            heroData->m_sex = charBuffer;

        if (g_game->m_mapHeader.m_version == MAP_FORMAT_ARMAGEDDONS_BLADE) {
            infile->read(&charBuffer, sizeof(charBuffer));
            if (charBuffer != -2) {
                heroData->m_customSpells = 1;
                heroData->m_spells = std::bitset<70>(0);
                if (charBuffer != -1)
                    heroData->m_spells[charBuffer] = 1;
            }
        } else {
            infile->read(&charBuffer, sizeof(charBuffer));
            if (charBuffer) {
                heroData->m_customSpells = 1;
                unsigned char spellMask[9];
                infile->read(spellMask, sizeof(spellMask));
                for (int spell = 0; spell < 70; ++spell) {
                    heroData->m_spells[spell] =
                        (spellMask[spell / 8] & (1 << (spell % 8))) != 0;
                }
            }

            infile->read(&charBuffer, sizeof(charBuffer));
            if (charBuffer) {
                heroData->m_customPrimarySkills = 1;
                for (x = 0; x < 4; ++x) {
                    infile->read(&charBuffer, sizeof(charBuffer));
                    heroData->m_primarySkills[x] = charBuffer;
                }
            }
        }
    }

    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;

    // A prison hero is not owned and not in any tavern pool.
    if (m_objectTypes[heroObject->m_typeIndex].m_objectType == PRISON)
        g_game->m_heroAvailability[heroID] = 0x41;
    else
        g_game->m_heroAvailability[heroID] = owner;

    heroData->m_location = heroObject->getTrigger();
    return 0;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:3229
#endif  // @carcass

// The map file's garrison record, appended to the game's garrison pool with
// the object's extraInfo left holding its index.

VA(0x00502a00, 0x151)  // dc 0xf151c
int NewfullMap::readGarrisonData(TAbstractFile* infile, CObject* garrisonObject,
                                 int mapVersion)
{
    garrison newGarrison;

    unsigned char owner;
    if (infile->read(&owner, sizeof(owner)) < sizeof(owner))
        return -1;
    newGarrison.m_playerOwner = owner;

    unsigned char pad[3];
    if (infile->read(pad, sizeof(pad)) < sizeof(pad))
        return -1;

    for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        int creature;
        if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
            signed char narrow;
            infile->read(&narrow, sizeof(narrow));
            creature = narrow;
        } else {
            short wide;
            infile->read(&wide, sizeof(wide));
            creature = wide;
        }
        newGarrison.m_garrisonArmy.m_armies[slot] = creature;

        short count;
        if (infile->read(&count, sizeof(count)) < sizeof(count))
            return -1;
        newGarrison.m_garrisonArmy.m_numTroops[slot] = count;
    }

    if (g_game->m_mapHeader.m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        newGarrison.m_removableTroops = 1;
    } else {
        unsigned char value;
        infile->read(&value, sizeof(value));
        newGarrison.m_removableTroops = value != 0;
    }

    int triggerX;
    int triggerY;
    garrisonObject->findTrigger(triggerX, triggerY);
    newGarrison.m_mapX = static_cast<unsigned char>(triggerX);
    newGarrison.m_mapY = static_cast<unsigned char>(triggerY);
    newGarrison.m_mapZ = garrisonObject->m_z;

    g_game->m_garrisons.push_back(newGarrison);
    garrisonObject->m_extraInfo = g_game->m_garrisons.size() - 1;

    unsigned char padding[8];
    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;
    return 0;
}

VA(0x00502b60, 0x29B)
void NewfullMap::soDTransformRandomDwellings()
{
    generator newGenerator;

    for (unsigned int index = 0; index < m_randomDwellings.size(); ++index) {
        RandomDwellingData& dwelling = m_randomDwellings[index];

        int alignment;
        if (dwelling.m_castleId) {
            int townIndex = g_game->m_scenarioTowns.size();
            while (townIndex--) {
                if (g_game->m_scenarioTowns[townIndex].m_objRef
                    == dwelling.m_castleId)
                    break;
            }
            alignment = g_game->m_scenarioTowns[townIndex].m_townType;
        } else {
            alignment = pickAlignment(dwelling.m_factionMask, 0);
        }

        TCreatureType creature = g_townDwellingCreatures[
            alignment * 2 * TOWN_DWELLING_COUNT
            + random(dwelling.m_minLevel, dwelling.m_maxLevel)];

        int generatorType;
        if (creature == CREATURE_STONE_GOLEM) {
            newfullMapFn00505F20(dwelling.m_object, CREATURE_GENERATOR_4,
                                  1, -1);
            newGenerator.m_genClass = CREATURE_GENERATOR_4;
            generatorType = 1;
        } else {
            newGenerator.m_genClass = CREATURE_GENERATOR_1;
            generatorType = 79;
            do {
            } while (g_creatureGenerator1Types[generatorType] != creature
                     && generatorType--);
            if (generatorType < 0)
                continue;
            newfullMapFn00505F20(dwelling.m_object, CREATURE_GENERATOR_1,
                                  generatorType, -1);
        }

        CObjectType* objectType = dwelling.m_object->getObjectTypePtr();
        int x;
        for (x = 0; x < objectType->m_width; ++x) {
            if (!objectType->m_passableCells[47 - x]
                || objectType->m_triggerCells[47 - x])
                break;
        }
        dwelling.m_object->m_x += x;

        int triggerY;
        dwelling.m_object->findTrigger(x, triggerY);
        newGenerator.m_genType = static_cast<char>(generatorType);
        newGenerator.m_mapX = static_cast<unsigned char>(x);
        newGenerator.m_mapY = static_cast<unsigned char>(triggerY);
        newGenerator.m_mapZ = dwelling.m_object->m_z;
        newGenerator.initialize(dwelling.m_owner);

        g_game->m_generators.push_back(newGenerator);
        dwelling.m_object->m_extraInfo = g_game->m_generators.size() - 1;
    }
}

// readObject's QUEST_GUARD arm. It was factored out here because the /Ob2
// budget is per-CALLER and clamps at a floor, so a small function gets a
// small budget and the insert stayed a CALL, which the single-call-site
// static then carried back into readObject when it was inlined whole. That
// bought 42.6246 -> 45.0977 and it costs no symbol (mapcell.obj still carries
// exactly 67 functions).

// THE STARVATION IS NO LONGER WHAT KEEPS THE CALL, and this is why the helper
// looks redundant now: once the three RANDOM_DWELLING arms below stopped
// expanding their inserts, the budget they freed came straight back here and
// the arm expanded again - the documented "pinning an EARLY site enlarges
// budget/sites-remaining for the LATER ones" effect, seen from the receiving
// end. The two inserts and the size() are now SITE-PINNED instead, and the
// overload each site names is read off the retail push sequence rather than
// guessed: retail pushes TWO arguments at both inserts here, so this arm
// calls insert(iterator, const T&) and NOT the three-argument
// insert(iterator, size_type, const T&) the RANDOM_DWELLING arms call. The
// third pin is size(): retail calls vector<TQuestGuard>::size out of line
// (0x1066e0, 32 B) at `extraInfo = size() - 1`, immediately after the insert
// and with &QuestGuardList still live in esi.

// The helper is kept because it is byte-inert and still starves anything not
// explicitly pinned.

// The starvation lever did NOT transfer to its neighbours. On the
// RANDOM_DWELLING_FACTION arm it scores 33.0580 - twelve points BELOW doing
// nothing - because that arm's two siblings already come out at retail's
// exact length and the three are costed together; and the SEER arm cannot be
// written this way at all, since TSeerHut derives PRIVATELY from TQuestGuard
// and friends only NewfullMap, so a free static cannot read tempHut.quest.
static void readQuestGuardArm(NewfullMap* map, TAbstractFile* infile,
                              CObject* tempObject)
{
    TQuestGuard tempGuard;
    tempGuard.read(infile);
    {
        std::vector<TQuestGuard>::iterator guardEnd
            = map->m_questGuardList.end();
#pragma inline_depth(0)
        map->m_questGuardList.insert(guardEnd, tempGuard);
        tempObject->m_extraInfo = map->m_questGuardList.size() - 1;
#pragma inline_depth()
    }
    if (tempGuard.m_quest) {
        CMapObjectData* questData = static_cast<CMapObjectData*>(
            static_cast<void*>(tempGuard.m_quest));
        std::vector<CMapObjectData*>::iterator dataEnd
            = map->m_mapObjectData.end();
#pragma inline_depth(0)
        map->m_mapObjectData.insert(dataEnd, questData);
#pragma inline_depth()
    }
}

// E:\gamedcs\mapcell.cpp:3290
// The h3m object dispatcher.  Five stream fields land in the object itself -
// x, y, z, a FOUR-byte type index of which only the low word is kept, and
// five discarded bytes - and each of those five reads is checked and returns
// -1.  Everything after that is a switch on the object TYPE, read out of the
// type record the index selects, and every arm of it returns 1: the reads
// inside an arm that are checked at all `break` to that shared 1, they do not
// report failure.

// The switch is a jump table over 5..218 with a 214-entry byte index, and its
// arm order is retail's own source order - the reconstruction keeps it.
// Twenty of the twenty-five arms are a single call; the five inlined ones are
// HERO_PLACEHOLDER, SHIPYARD, HOLY_GRAIL, SEER, SHRINE1/2/3, QUEST_GUARD and
// the three RANDOM_DWELLING flavours, which is what the Dreamcast roster's
// missing readHolyGrail/readShrine/readShipyard rows mean here.

// MINE and LIGHTHOUSE share a tail: retail cross-jumps the mine's non-
// abandoned arm into the lighthouse's `readMineData` call rather than
// duplicating it, which is a compiler transform over two separate cases, not
// a fallthrough in the source.
// 45.0977 -> 96.9833 (2026-08-20) ON FIVE `#pragma inline_depth(0)` SITE
// PINS. MATCHING_DEBT: those narrow site pins remain necessary to reproduce
// VC6's caller-budget cutoff; they are not source evidence. The note this
// entry used to carry had the diagnosis right and the arithmetic wrong. Read
// the correction before re-deriving any of it.

// The old text said the wall was "id-218 (RANDOM_DWELLING_FACTION) at 574
// bytes against retail's 149" and that its "two siblings at 155 and 140
// already match". That last half is what misled three attempts. Retail
// CALLS the insert at ALL THREE dwelling arms - its three calls sit at
// fn+0x521, +0x5ad and +0x642 with nothing between them - while our compile
// called it at the FIRST arm only and expanded it at the other two. So the
// lever was never "make the FACTION arm behave like its siblings"; it was
// "pin the two arms the budget ran out on". Pinning FACTION alone is what
// every earlier measurement did, and it loses because arm two is still
// expanding underneath it.

// THE OVERLOAD IS READ OFF THE PUSH SEQUENCE, NOT GUESSED. `push_back`
// reaches insert(iterator, const T&), which reaches
// insert(iterator, size_type, const T&), and retail stops at a DIFFERENT
// depth per site - so an explicit spelling has to name the right one:
//   * the three RANDOM_DWELLING arms and the SEER arm's mapObjectData site
//     push THREE arguments -> insert(iterator, size_type, const T&);
//   * the SEER arm's SeerHutList site and both QUEST_GUARD sites push TWO
//     -> insert(iterator, const T&).
// Getting this wrong still scores well, because objdiff pairs the two
// overloads' relocations positionally - the three-argument spelling at the
// two-argument sites reached 95.1950 and only the extra `push 1` at each
// site was wrong - but it is a real byte difference and worth 1.7 points.

// `end()` must be HOISTED out of every pin: the pragma is statement-granular
// and would otherwise turn `end()` itself into a call retail does not have.

// One more pin, and it is the same reading applied to a non-insert callee:
// retail CALLS vector<TQuestGuard>::size (0x1066e0, 32 B) at the QUEST_GUARD
// arm's `extraInfo = size() - 1`, immediately after the insert with
// &QuestGuardList still live in esi. The SEER arm's size() is inlined on both
// sides; do not pin that one.

// Kept from the old note because they still hold:
//   * `#pragma inline_depth(2)` and `(1)` around the body are ignored under
//     /O2 /Ob2, byte-identical at 42.5589. Only 0 bites.
//   * the Dreamcast CodeView roster proves one function-scope `char_buffer`
//     and one `padding[5]`. On the current retail reconstruction, reusing the
//     entry byte for the mutually-exclusive byte reads and the five-byte
//     padding for the dwelling arms raises 96.9833 -> 97.1293. A generated
//     383-candidate lifetime tree selected that shape; another 840-candidate
//     tree over the DC-proven `int_buffer`/`count` roster was flat or worse.
//     A final five-candidate expression tree raises this to 97.4369 by
//     spelling the RANDOM_DWELLING_LVL level stores directly. This moves the
//     first store before insert setup, toward retail's order. The residual is
//     explicit in the bytes: our compile reloads the level and leaves the
//     second store late; retail loads it once and performs both stores first.
//   * the multi-site hypothesis is DISPROVED: a throwaway second growth site
//     for vector<TQuestGuard> in another function of this TU moved readObject
//     by exactly nothing. The /Ob2 budget is per CALLER.
//   * the helper-starvation lever (readQuestGuardArm above) was worth
//     42.6246 -> 45.0977 and is still in place, but it is no longer what
//     keeps that arm's insert out of line - see its own note.

// Residual (97.4369%): register/home only, with the branch sequence intact.
// Retail's frame is 0x28 and ours is 0x2c. Retail spills three of the arms'
// temporaries into the DEAD PARAMETER SLOT at [ebp+0x10] - the `mapVersion`
// argument - where our compile gives each a fresh negative local: the
// HERO_PLACEHOLDER record, the SEER arm's quest pointer and the QUEST_GUARD
// arm's quest pointer all read `[ebp+0x10]` in retail and `[ebp-N]` here.
// That is the remaining B4 knob; why-reg's model does not reach a parameter
// slot from a body spelling.
// 2026-09-06, polish lane 49 (lever B census), MEASURED NEGATIVE: the
// operand histogram shows retail splitting the byte scratch in two - the
// header x/y/z reads home at [ebp+0xb] (6 refs, 3 lea) and the switch arms
// at [ebp+0x13] (22 refs, 11 lea) - where our single `value` serves all 16
// sites from [ebp+0x8].  Spelling the second `char entry;` DOES move our
// header scratch onto retail's [ebp+0xb] exactly, but CL homes `entry` at
// [ebp-0x4] and grows the frame 0x2c -> 0x30: 97.4369 -> 97.3034, both with
// the declaration before the switch and beside `value`.  Retail reaches
// [ebp+0x13] by packing a dword, a short and the char into the ONE dead
// `mapVersion` home (frame 0x28); that packing is the same B4 knob named
// above and is not reachable from a declaration.
// 2026-09-06, polish lane 36: the family `int count` local that closed
// readResourceData and readScholarData is BYTE-FLAT here (97.4369 either
// way), and so is it on readMapLayer (95.4717) and on NewfullMap::Save
// (91.8870, where the DC's only local IS `count`).  The lever only bites
// where the Read result feeds an UNSIGNED compare whose operand VC6 would
// otherwise fold; a `< sizeof(...)` compare on a plain `char` read is
// already in retail's shape.
VA(0x00502e00, 0x832)  // order-map: dispatches to all read*Data rows (DC-isomorphic callee set) + CreateBoat 0x4bb250 (readBoatData inlined) + TQuestGuard::read (retail quest path); readHolyGrail/readShrine/readShipyard inlined, dc 0xf16c8
int NewfullMap::readObject(TAbstractFile* infile, CObject* tempObject,
                           int mapVersion)
{
    char value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    tempObject->m_x = value;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    tempObject->m_y = value;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    tempObject->m_z = value;

    int typeIndex;
    if (infile->read(&typeIndex, sizeof(typeIndex)) < sizeof(typeIndex))
        return -1;
    tempObject->m_typeIndex = static_cast<unsigned short>(typeIndex);

    char padding[5];
    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;

    switch (m_objectTypes[tempObject->m_typeIndex].m_objectType) {
    case ARTIFACT:
    case RANDOM_ARTIFACT:
    case RANDOM_ARTIFACT_1:
    case RANDOM_ARTIFACT_2:
    case RANDOM_ARTIFACT_3:
    case RANDOM_ARTIFACT_4:
        readArtifactData(infile, tempObject);
        break;

    case HERO:
    case PRISON:
    case RANDOM_HERO:
        readHeroData(infile, tempObject, mapVersion);
        break;

    case BOAT: {
        signed char boatType = static_cast<signed char>(
            m_objectTypes[tempObject->m_typeIndex].m_extra);
        int triggerX;
        int triggerY;
        tempObject->findTrigger(triggerX, triggerY);
        tempObject->m_extraInfo = g_game->createBoat(
            triggerX, triggerY, tempObject->m_z, -1, 1, boatType);
        break;
    }

    case RANDOM_TOWN:
    case TOWN:
        readTownData(infile, tempObject, mapVersion);
        break;

    case MONSTER:
    case RANDOM_MONSTER:
    case RANDOM_MONSTER_1:
    case RANDOM_MONSTER_2:
    case RANDOM_MONSTER_3:
    case RANDOM_MONSTER_4:
    case RANDOM_MONSTER_5:
    case RANDOM_MONSTER_6:
    case RANDOM_MONSTER_7:
        readMonsterData(infile, tempObject);
        break;

    case EVENT:
        readEventData(infile, tempObject, mapVersion);
        break;

    case HERO_PLACEHOLDER: {
        HeroPlaceholderData placeholder;
        placeholder.m_object = tempObject;

        infile->read(&value, sizeof(value));
        placeholder.m_owner = value;

        infile->read(&value, sizeof(value));
        placeholder.m_heroId = value;
        if (placeholder.m_heroId
                == HeroPlaceholderData::HERO_ID_BY_POWER_RATING) {
            placeholder.m_heroId = -1;
            infile->read(&value, sizeof(value));
            placeholder.m_powerRating = value;
        }

        m_heroPlaceholders.push_back(placeholder);
        break;
    }

    case SPELL_SCROLL:
        readSpellScrollData(infile, tempObject);
        break;

    case SHIPYARD: {
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            break;
        tempObject->m_shipyardInfo.m_owner = value;

        char shipyardPadding[3];
        if (infile->read(shipyardPadding, sizeof(shipyardPadding))
                < sizeof(shipyardPadding))
            break;
        tempObject->m_shipyardInfo.m_boatX = 0xff;
        tempObject->m_shipyardInfo.m_boatY = 0xff;
        break;
    }

    case RANDOM_RESOURCE:
    case RESOURCE:
        readResourceData(infile, tempObject);
        break;

    case HOLY_GRAIL: {
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            break;
        g_game->m_ultimateArtifactX = tempObject->m_x;
        g_game->m_ultimateArtifactY = tempObject->m_y;
        g_game->m_ultimateArtifactZ = tempObject->m_z;
        g_game->m_ultimateRadius = value;

        char grailPadding[3];
        infile->read(grailPadding, sizeof(grailPadding));
        break;
    }

    case BLACK_BOX:
        readBlackBoxData(infile, tempObject, mapVersion);
        break;

    case SCHOLAR:
        readScholarData(infile, tempObject);
        break;

    case SEER: {
        TSeerHut tempHut;
        tempHut.read(infile);
        {
            std::vector<TSeerHut>::iterator hutEnd = m_seerHutList.end();
#pragma inline_depth(0)
            m_seerHutList.insert(hutEnd, tempHut);
#pragma inline_depth()
        }
        tempObject->m_extraInfo = m_seerHutList.size() - 1;
        if (tempHut.m_quest)
            m_mapObjectData.push_back(static_cast<CMapObjectData*>(
                static_cast<void*>(tempHut.m_quest)));
        break;
    }

    case SHRINE1:
    case SHRINE2:
    case SHRINE3: {
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            break;
        tempObject->m_shrineInfo.m_spell = value;

        char shrinePadding[3];
        infile->read(shrinePadding, sizeof(shrinePadding));
        break;
    }

    case OCEAN_BOTTLE:
    case SIGN:
        readSignData(infile, tempObject);
        break;

    case MINE:
        if (m_objectTypes[tempObject->m_typeIndex].m_extra == NUM_RESOURCES)
            readAbandonedMineData(infile, tempObject);
        else
            readMineData(infile, tempObject);
        break;

    case LIGHTHOUSE:
        readMineData(infile, tempObject);
        break;

    case CREATURE_GENERATOR_1:
    case CREATURE_GENERATOR_4:
        readGeneratorData(infile, tempObject);
        break;

    case GARRISON:
        readGarrisonData(infile, tempObject, mapVersion);
        break;

    case RANDOM_DWELLING: {
        RandomDwellingData dwelling;

        infile->read(&value, sizeof(value));
        dwelling.m_owner = value;

        infile->read(padding, 3);

        int castleId;
        infile->read(&castleId, sizeof(castleId));
        dwelling.m_castleId = castleId;
        if (castleId == 0) {
            short factionMask;
            infile->read(&factionMask, sizeof(factionMask));
            dwelling.m_factionMask = factionMask;
        }

        infile->read(&value, sizeof(value));
        dwelling.m_minLevel = value;
        infile->read(&value, sizeof(value));
        dwelling.m_maxLevel = value;

        dwelling.m_object = tempObject;
        m_randomDwellings.push_back(dwelling);
        break;
    }

    case RANDOM_DWELLING_LVL: {
        RandomDwellingData dwelling;

        infile->read(&value, sizeof(value));
        dwelling.m_owner = value;

        infile->read(padding, 3);

        int castleId;
        infile->read(&castleId, sizeof(castleId));
        dwelling.m_castleId = castleId;
        if (castleId == 0) {
            short factionMask;
            infile->read(&factionMask, sizeof(factionMask));
            dwelling.m_factionMask = factionMask;
        }

        dwelling.m_minLevel = static_cast<unsigned char>(
            m_objectTypes[tempObject->m_typeIndex].m_extra);
        dwelling.m_maxLevel = static_cast<unsigned char>(
            m_objectTypes[tempObject->m_typeIndex].m_extra);

        dwelling.m_object = tempObject;
        {
            std::vector<RandomDwellingData>::iterator dwellingEnd
                = m_randomDwellings.end();
#pragma inline_depth(0)
            m_randomDwellings.insert(dwellingEnd, 1, dwelling);
#pragma inline_depth()
        }
        break;
    }

    case RANDOM_DWELLING_FACTION: {
        RandomDwellingData dwelling;

        infile->read(&value, sizeof(value));
        dwelling.m_owner = value;

        infile->read(padding, 3);

        dwelling.m_castleId = 0;
        dwelling.m_factionMask = static_cast<unsigned short>(
            1 << m_objectTypes[tempObject->m_typeIndex].m_extra);

        infile->read(&value, sizeof(value));
        dwelling.m_minLevel = value;
        infile->read(&value, sizeof(value));
        dwelling.m_maxLevel = value;

        dwelling.m_object = tempObject;
        {
            std::vector<RandomDwellingData>::iterator dwellingEnd
                = m_randomDwellings.end();
#pragma inline_depth(0)
            m_randomDwellings.insert(dwellingEnd, 1, dwelling);
#pragma inline_depth()
        }
        break;
    }

    case QUEST_GUARD:
        readQuestGuardArm(this, infile, tempObject);
        break;

    case WITCH_HUT:
        if (g_game->m_mapHeader.m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
            tempObject->m_extraInfo = 0xefbf;
        } else {
            int allowedSkills;
            infile->read(&allowedSkills, sizeof(allowedSkills));
            tempObject->m_extraInfo = allowedSkills;
        }
        break;

    default:
        break;
    }

    return 1;
}

VA(0x00503640, 0x8D)  // dc 0xf1b1c
int NewfullMap::saveObject(TAbstractFile* outfile, CObject& tempObject)
{
    int count;
    char value = tempObject.m_x;
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;

    value = tempObject.m_y;
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;

    value = tempObject.m_z;
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;

    unsigned short typeIndex = tempObject.m_typeIndex;
    count = outfile->write(&typeIndex, sizeof(typeIndex));
    if (count < sizeof(typeIndex))
        return -1;
    return 0;
}

VA(0x005036d0, 0xA4)  // dc 0xf1bf8
int NewfullMap::loadObject(TAbstractFile* infile, CObject* tempObject)
{
    unsigned char value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    tempObject->m_x = value;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    tempObject->m_y = value;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    tempObject->m_z = value;

    unsigned short typeIndex;
    if (infile->read(&typeIndex, sizeof(typeIndex)) < sizeof(typeIndex))
        return -1;
    tempObject->m_typeIndex = typeIndex;
    return 0;
}

// E:\gamedcs\mapcell.cpp:3514
// The h3m object-TYPE record, and the only reader in this compiland that
// draws on TWO streams at once.  The map file carries the record, but the
// PLACEMENT and SHADOW footprints come out of the object's own .msk
// resource, and retail interleaves them: msk mask -> drawCells, stream mask
// -> passableCells, msk mask -> shadowCells, stream mask -> triggerCells.
// Only the stream's two are checked; the resource reads discard their
// result, which is why the msk halves have no error path.

// The resource name is built from the image name IN PLACE, and the way it
// is built is retail's: `_strrev`, overwrite the first three characters
// with "ksm", `_strrev` back.  That replaces whatever extension the name
// carried without ever measuring it.  The 100-byte buffer and its `= ""`
// initializer are byte-proven - one explicit zero store followed by a
// 99-byte rep stos.

// When the object's own mask is missing, "default.msk" stands in, the
// player is told through a message box, and the function answers 100
// instead of 1: the tail's `neg bl / sbb ebx,ebx / and 0x63 / inc` is that
// ternary, and readMapObjects cases on it to collect the offending type
// indices.  The caption is the SAME pooled "Error!" that readMapObjects
// claims at 0x67fb08 - this is its first use in the TU, so the literal is
// written bare here and the DATA_COMPGEN claim stays at the one site.

// The type word the stream carries is not the one that survives: retail
// stores it, then RE-READS it out of the record and remaps it through the
// 16-byte adventure-object trait rows at 0x660428, taking the DWORD at +8.
// That is a second live field of that table - byte +0 and byte +1 are the
// ones findpath/advmgr/hero already read - proven here by
// `shl eax,4 / mov edx,[eax+ecx+8]`.  It is spelled as a four-byte copy
// rather than a pointer cast on purpose: 0x660428 already carries its one
// admitted extern as `unsigned char (*)[16]` and the single-view gate is
// what keeps it that way, so refining the row into a struct is a change
// that has to move findpath.cpp, hero.cpp and advmgr.cpp with it, not a
// second view bolted on from here.  VC6 expands the copy to exactly
// retail's `mov edx,[eax+ecx+8] / mov [edi+0x38],edx`.

// Four of the record's fields are read and thrown away - two 2-byte
// landscape masks into one slot, and the object-group byte before the
// overlay flag - and the record ends with sixteen discarded bytes.

// The full-width type read uses the native enum, then commits only after
// the short-read guard. This is a Complete I/O-owner reconstruction, not
// a recovered DC local: DC rows 3610..3614 reuse int_buffer (our value),
// whose filename-length and extra-field uses remain. Row 3619 positively
// reads the diagnostic type from the committed record. Direct field I/O
// is not equivalent: a short read would partially change the destination.
// Residual (99.9633%): native enum ownership changes 18 stack displacement
// bytes, with unchanged instruction order, size and relocation targets. The 8-state
// owner/query family and 9-state field-lifetime follow-up are exhausted;
// the latter's best 99.9673% requires splitting the proven generic buffer
// and adding phase scopes, so retain the simpler native local. No dummy
// lifetime operations or substitute representation casts are introduced.
// DC's CObjectType reference, separate int count, and char dummy[2] are
// restored as canonical source facts. Twelve source candidates produced four
// reproduced objects; these locals and the reference call leave the retained
// reader/caller bytes unchanged. A short or byte buffer does not explain the
// remaining int_buffer/enum-owner stack displacements.
VA(0x00503780, 0x4C0)  // order-map: calls _strrev + sprintf + PointToSpriteResource 0x55cf50 x2 + the 0x55d0d0 resource reader x4 (DC call counts match exactly); called by readMapObjects, dc 0xf1cd8
int NewfullMap::readObjectType(TAbstractFile* infile,
                               CObjectType& tempObjectType)
{
    char imageName[100] = { 0 };
    int value;
    int count;
    char byteValue;
    unsigned char packed[6];
    int i;

    count = infile->read(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    infile->read(imageName, value);
    tempObjectType.m_imageName = imageName;

    _strrev(imageName);
    imageName[0] = 'k';
    imageName[1] = 's';
    imageName[2] = 'm';
    _strrev(imageName);

    unsigned char usedDefaultMask = 0;
    LODFile* maskFile = ResourceManager::pointToSpriteResource(imageName);
    if (maskFile == 0) {
        usedDefaultMask = 1;
        maskFile = ResourceManager::pointToSpriteResource(
            DATA_COMPGEN(0x0067fb3c, readObjectTypeDefaultMask,
                         "default.msk"));
        if (maskFile == 0)
            return -1;
    }

    ResourceManager::readFromBitmapResource(maskFile, &byteValue,
                                            sizeof(byteValue));
    tempObjectType.m_width = byteValue;
    ResourceManager::readFromBitmapResource(maskFile, &byteValue,
                                            sizeof(byteValue));
    tempObjectType.m_height = byteValue;

    ResourceManager::readFromBitmapResource(maskFile, packed, sizeof(packed));
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        tempObjectType.m_drawCells.set(i,
            (packed[i / 8] & (1 << (i % 8))) != 0);
    }

    count = infile->read(packed, sizeof(packed));
    if (count < sizeof(packed))
        return -1;
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        tempObjectType.m_passableCells.set(i,
            (packed[i / 8] & (1 << (i % 8))) != 0);
    }

    ResourceManager::readFromBitmapResource(maskFile, packed, sizeof(packed));
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        tempObjectType.m_shadowCells.set(i,
            (packed[i / 8] & (1 << (i % 8))) != 0);
    }

    count = infile->read(packed, sizeof(packed));
    if (count < sizeof(packed))
        return -1;
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        tempObjectType.m_triggerCells.set(i,
            (packed[i / 8] & (1 << (i % 8))) != 0);
    }

    char dummy[2];
    count = infile->read(dummy, sizeof(dummy));
    if (count < sizeof(dummy))
        return -1;
    count = infile->read(dummy, sizeof(dummy));
    if (count < sizeof(dummy))
        return -1;

    TAdventureObjectType objectTypeRead;
    count = infile->read(&objectTypeRead, sizeof(objectTypeRead));
    if (count < sizeof(objectTypeRead))
        return -1;
    tempObjectType.m_objectType = objectTypeRead;
    if (usedDefaultMask) {
        sprintf(g_text,
                DATA_COMPGEN(0x0067fb10, readObjectTypeMissingMask,
                             "Could not load mask file for %s! - Type: %s"),
                tempObjectType.m_imageName.c_str(),
                g_adventureObjectNames[tempObjectType.m_objectType]);
        MessageBoxA(g_hwndApp, g_text, "Error!", 0);
    }

    memcpy(&tempObjectType.m_objectType,
           &g_adventureObjectTraits[tempObjectType.m_objectType][8],
           sizeof(tempObjectType.m_objectType));

    count = infile->read(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    tempObjectType.m_extra = value;

    count = infile->read(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;
    count = infile->read(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;
    tempObjectType.m_suppressDraw = byteValue != 0;

    char unused[16];
    count = infile->read(unused, sizeof(unused));
    if (count < sizeof(unused))
        return -1;
    return usedDefaultMask ? READ_OBJECT_TYPE_DEFAULT_MASK : 1;
}

// The final Write returns 1, not 0, on success - the sbb/and/inc tail is a
// `? -1 : 1` ternary, not the `? -1 : 0` every other serializer here ends on.
VA(0x00503c40, 0x2B9)  // dc 0xf22cc
int NewfullMap::saveObjectType(TAbstractFile* outfile,
                               CObjectType* tempObjectType)
{
    game::saveString(outfile, &tempObjectType->m_imageName);

    char value = tempObjectType->m_width;
    if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
        return -1;
    value = tempObjectType->m_height;
    if (static_cast<unsigned>(outfile->write(&value, 1)) < 1)
        return -1;

    unsigned char packed[6];
    int i;

    memset(packed, 0, sizeof(packed));
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        if (tempObjectType->m_drawCells.test(i))
            packed[i / 8] |= 1 << (i % 8);
    }
    if (static_cast<unsigned>(outfile->write(packed, 6)) < 6)
        return -1;

    memset(packed, 0, sizeof(packed));
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        if (tempObjectType->m_passableCells.test(i))
            packed[i / 8] |= 1 << (i % 8);
    }
    if (static_cast<unsigned>(outfile->write(packed, 6)) < 6)
        return -1;

    memset(packed, 0, sizeof(packed));
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        if (tempObjectType->m_shadowCells.test(i))
            packed[i / 8] |= 1 << (i % 8);
    }
    if (static_cast<unsigned>(outfile->write(packed, 6)) < 6)
        return -1;

    memset(packed, 0, sizeof(packed));
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        if (tempObjectType->m_triggerCells.test(i))
            packed[i / 8] |= 1 << (i % 8);
    }
    if (static_cast<unsigned>(outfile->write(packed, 6)) < 6)
        return -1;

    short typeValue = tempObjectType->m_objectType;
    if (static_cast<unsigned>(outfile->write(&typeValue, 2)) < 2)
        return -1;

    int extra = tempObjectType->m_extra;
    if (static_cast<unsigned>(outfile->write(&extra, 4)) < 4)
        return -1;

    value = tempObjectType->m_suppressDraw;
    return static_cast<unsigned>(outfile->write(&value, 1)) < 1 ? -1 : 1;
}

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:3752
#endif  // @carcass

// The trailing byte is normalized (`test al,al / setne`), not copied, so it
// is a bool crossing where width and height are plain assignments.  And
// like Save, this returns 1 on success rather than 0.
VA(0x00503f00, 0x35D)  // dc 0xf2784
int NewfullMap::loadObjectType(TAbstractFile* infile,
                               CObjectType* tempObjectType)
{
    game::loadString(infile, tempObjectType->m_imageName);

    char value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    tempObjectType->m_width = value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    tempObjectType->m_height = value;

    unsigned char packed[6];
    int i;

    if (infile->read(packed, sizeof(packed)) < sizeof(packed))
        return -1;
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        tempObjectType->m_drawCells.set(i,
            (packed[i / 8] & (1 << (i % 8))) != 0);
    }

    if (infile->read(packed, sizeof(packed)) < sizeof(packed))
        return -1;
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        tempObjectType->m_passableCells.set(i,
            (packed[i / 8] & (1 << (i % 8))) != 0);
    }

    if (infile->read(packed, sizeof(packed)) < sizeof(packed))
        return -1;
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        tempObjectType->m_shadowCells.set(i,
            (packed[i / 8] & (1 << (i % 8))) != 0);
    }

    if (infile->read(packed, sizeof(packed)) < sizeof(packed))
        return -1;
    for (i = 0; i < sizeof(packed) * 8; ++i) {
        tempObjectType->m_triggerCells.set(i,
            (packed[i / 8] & (1 << (i % 8))) != 0);
    }

    unsigned short typeValue;
    if (infile->read(&typeValue, sizeof(typeValue)) < sizeof(typeValue))
        return -1;
    union {
        unsigned long m_raw;
        TAdventureObjectType m_typed;
    } convertedType;
    convertedType.m_raw = typeValue;
    tempObjectType->m_objectType = convertedType.m_typed;

    int extra;
    if (infile->read(&extra, sizeof(extra)) < sizeof(extra))
        return -1;
    tempObjectType->m_extra = extra;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    tempObjectType->m_suppressDraw = value != 0;
    return 1;
}

// The h3m object-table reader, loadMapObjects' twin - same resize/refill/
// dispose shape for the sprite table, same re-taken subscripts - plus the
// missing-mask reporting the save path has no need of.

// readObjectType answers 100, not 1, when an object's own .msk was missing
// and default.msk stood in.  Every such TYPE index is collected here, and
// then every OBJECT that turns out to reference one is reported by name and
// position.  That is what the file-scope vector below is for, and why it is
// CLEARED on entry rather than merely constructed empty.

// It also corrects this compiland's span audit: the static ctor/dtor pair at
// 0x504260/0x504290 was recorded as belonging to a 16-byte STRING global at
// 0x699690.  It is this vector - the reads at 0x699694/0x699698 are _First
// and _Last, and the entry clear passes exactly that pair to
// vector<int>::erase.

// TWO SOURCE FACTS, both read straight out of the frame, worth 78.4070 ->
// 87.2791 together (2026-08-20).  The note this replaces had both of them
// pointed at the wrong construct, so read the bytes, not the prose.

VA_COMPGEN(0x00504290, 0x2A, STATIC_DTOR, g_invalidPlacementList)
DATA(0x00699690)
std::vector<int> g_invalidPlacementList;

// Retail-only object-type lookup rebuild.  Each of the 232 adventure-object
// classes owns a vector of type records.  Clear every record's resolved
// index first, then bind each deserialized objectTypes entry to the matching
// (class, extra, image-name) record in that per-class vector.  The backwards
// search is byte-visible: retail starts at size(), tests the pre-decrement
// value, and leaves the successful index in the decremented counter.

// This is Complete-only source.  The raw Dreamcast NB11 sequence has only
// $E482..$E485 between loadObjectType and readMapObjects, and neither caller
// contains the rebuild region.  readMapObjects owns procedure locals
// int_buffer, numObjects, count, x and i (with v only inside its object-loop
// block); loadMapObjects owns only int_buffer, count and x.  Consequently no
// Dreamcast local name or scope can honestly be transferred into this helper.
// What does transfer is negative revision evidence: do not invent a DC
// counterpart.  Retail's two calls prove the later helper boundary directly,
// and fatal caller contracts preserve it against re-inlining.

// Wall (99.88535%, 2026-08-22): all 32 blocks, all 32 edges, the 157
// instructions and their register bindings agree.  The only code delta is
// one post-RA reload schedule after the inlined string comparison: retail
// restores ESI before EAX/EDI, while this compile restores it after them.
// `why-reg --model` classifies the two unpaired slots as the B16/C3/C4
// schedule family, not a register-binding defect.  Direct/reference/pointer
// candidate spellings, && versus split conditions, compare() versus ==,
// signed/unsigned and reused/block-scoped loop counters, and every adjacent
// declaration-order mutation either preserve this transpose or get worse.
// Rechecked 2026-08-30: binding objectTypes[i] to a CObjectType& and splitting
// the short-circuit into an explicit continue are byte-flat at 99.88535%;
// declaring extra after typeIndex keeps the 421-byte size but falls to
// 95.93630%.  The coherent original spelling remains the peak.
// The other asm-report row is cosmetic: this TU's Dinkumware `_Nullstr`
// COFF symbol and the target's source-owned pooled empty-literal symbol both
// resolve to 0x63a608; no DATA_COMPGEN binding exists in mapcell.obj because
// the literal's physical owner is another compiland.
VA(0x005042c0, 0x1A5)  // retail body + two callers: readMapObjects/loadMapObjects; no DC roster row
void NewfullMap::newfullMapFn005042C0()
{
    for (int objectClass = 0; objectClass < 232; ++objectClass) {
        for (int typeIndex = 0;
             typeIndex < m_objectTypeIndex[objectClass].size(); ++typeIndex) {
            m_objectTypeIndex[objectClass][typeIndex].m_objectTypeIndex = 0xffff;
        }
    }

    for (int i = 0; i < m_objectTypes.size(); ++i) {
        int objectClass = m_objectTypes[i].m_objectType;
        int extra = m_objectTypes[i].m_extra;
        int typeIndex = m_objectTypeIndex[objectClass].size();
        while (typeIndex--) {
            CObjectType& candidate = m_objectTypeIndex[objectClass][typeIndex];
            if (candidate.m_extra == extra
                && candidate.m_imageName == m_objectTypes[i].m_imageName)
                break;
        }
        if (typeIndex >= 0)
            m_objectTypeIndex[objectClass][typeIndex].m_objectTypeIndex =
                static_cast<unsigned short>(i);
    }
}

// E:\gamedcs\mapcell.cpp:3838
VA(0x00504470, 0x5C9)  // order-map: calls readObject 0x502e00 + readObjectType 0x503780 + GetSprite 0x55c7b0 + Random x2 (CObject ctor inlined) + progress-bar helpers; $E482-$E485 pair sits just before at 0x104260/0x104290 matching DC link order; EH-bearing, dc 0xf2c20
int NewfullMap::readMapObjects(TAbstractFile* infile, int mapVersion)
{
    g_invalidPlacementList.clear();

    int count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_objectTypes.resize(count);

    int i;
    for (i = 0; i < m_objectTypes.size(); ++i) {
        int status = readObjectType(infile, m_objectTypes[i]);
        if (status < 0)
            return -1;
        if (i == m_objectTypes.size() / 2)
            incProgressBar(1);
        if (status == READ_OBJECT_TYPE_DEFAULT_MASK)
            g_invalidPlacementList.push_back(i);
    }

    newfullMapFn005042C0();
    incProgressBar(1);

    std::vector<CSprite*> oldSprites;
    oldSprites.resize(m_sprites.size());
    for (i = 0; i < m_sprites.size(); ++i)
        oldSprites[i] = m_sprites[i];

    m_sprites.resize(count);
    for (i = 0; i < m_objectTypes.size(); ++i) {
        m_sprites[i] =
            ResourceManager::getSprite(m_objectTypes[i].m_imageName.c_str());
        if (i == m_objectTypes.size() / 3)
            incProgressBar(1);
        if (i == m_objectTypes.size() / 3 * 2)
            incProgressBar(1);
    }

    for (i = 0; i < oldSprites.size(); ++i)
        oldSprites[i]->dispose();
    oldSprites.clear();

    incProgressBar(1);

    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_objects.resize(count);
    for (i = 0; i < m_objects.size(); ++i) {
        if (readObject(infile, &m_objects[i], mapVersion) < 0)
            return -1;

        for (unsigned int missing = 0; missing < g_invalidPlacementList.size();
             ++missing) {
            if (m_objects[i].m_typeIndex == g_invalidPlacementList[missing]) {
                sprintf(g_text,
                        DATA_COMPGEN(0x0067fb48, readMapObjectsInvalidObject,
                                     "Invalid Object Referenced!\n\n"
                                     "x: %d y: %d z: %d - Type: %s"),
                        m_objects[i].m_x, m_objects[i].m_y, m_objects[i].m_z,
                        g_adventureObjectNames[
                            m_objectTypes[m_objects[i].m_typeIndex].m_objectType]);
                MessageBoxA(g_hwndApp, g_text,
                            DATA_COMPGEN(0x0067fb08,
                                         readMapObjectsErrorCaption, "Error!"),
                            0);
            }
        }

        m_objects[i].m_animationOffset = static_cast<unsigned char>(random(0, 255));
    }

    incProgressBar(1);
    return 1;
}

VA(0x00504a40, 0x127)  // dc 0xf3018
int NewfullMap::saveMapObjects(TAbstractFile* outfile)
{
    int count = m_objectTypes.size();
    if (outfile->write(&count, sizeof(count)) < sizeof(count))
        return -1;

    unsigned int i;
    for (i = 0; i < m_objectTypes.size(); ++i) {
        if (saveObjectType(outfile, &m_objectTypes[i]) < 0)
            return -1;
    }

    count = m_objects.size();
    if (outfile->write(&count, sizeof(count)) < sizeof(count))
        return -1;

    for (i = 0; i < m_objects.size(); ++i) {
        if (saveObject(outfile, m_objects[i]) < 0)
            return -1;
    }
    return 1;
}

// Neither list read has the `count == 0 -> clear()` arm readTimedEventList
// needs: both go straight into resize, which is loadBlackBox's shape.

VA(0x00504b70, 0x4E9)  // dc 0xf318c
int NewfullMap::loadMapObjects(TAbstractFile* infile)
{
    int count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_objectTypes.resize(count);

    unsigned int i;
    for (i = 0; i < m_objectTypes.size(); ++i) {
        if (loadObjectType(infile, &m_objectTypes[i]) < 0)
            return -1;
    }

    incProgressBar(1);
    newfullMapFn005042C0();

    std::vector<CSprite*> oldSprites;
    oldSprites.resize(m_sprites.size());
    for (i = 0; i < m_sprites.size(); ++i)
        oldSprites[i] = m_sprites[i];

    m_sprites.resize(m_objectTypes.size());
    for (i = 0; i < m_objectTypes.size(); ++i) {
        m_sprites[i] =
            ResourceManager::getSprite(m_objectTypes[i].m_imageName.c_str());
        if (i == m_objectTypes.size() / 3)
            incProgressBar(1);
        if (i == m_objectTypes.size() / 3 * 2)
            incProgressBar(1);
    }

    for (i = 0; i < oldSprites.size(); ++i)
        oldSprites[i]->dispose();
    oldSprites.clear();

    incProgressBar(1);

    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_objects.resize(count);
    for (i = 0; i < m_objects.size(); ++i) {
        if (loadObject(infile, &m_objects[i]) < 0)
            return -1;
        m_objects[i].m_animationOffset = static_cast<unsigned char>(random(0, 255));
    }

    incProgressBar(1);
    return 1;
}

// The WIDTH index is the outer loop and the height index the inner one - the
// grid pointer advances by six per outer iteration while the inner one indexes
// single bytes, which fixes the orientation as heightMap[width][height].  Both
// loop bounds are re-read from the type record on every iteration, so neither
// is hoisted.

VA(0x00505060, 0x1CD)  // dc 0xf33fc
void NewfullMap::generateHeightMap(const CObject* object,
                                   signed char heightMap[8][6])
{
    int typeIndex = object->m_typeIndex;

    memset(heightMap, 0, 48);

    if (m_objectTypes[typeIndex].m_suppressDraw)
        return;

    for (int col = 0; col < m_objectTypes[typeIndex].m_width; ++col) {
        int depth = 1;
        for (int row = 0; row < m_objectTypes[typeIndex].m_height; ++row) {
            if (row > 0) {
                if (!m_objectTypes[typeIndex].m_passableCells[47 - row * 8 - col]
                    && m_objectTypes[typeIndex].m_passableCells[55 - row * 8 - col])
                    depth = 1;
            }
            if (col > 0) {
                if (m_objectTypes[typeIndex].m_passableCells[47 - row * 8 - col]
                    && !m_objectTypes[typeIndex].m_passableCells[48 - row * 8 - col])
                    depth = heightMap[col - 1][row];
            }
            heightMap[col][row] = static_cast<signed char>(depth);
            ++depth;
        }
    }
}

VA(0x00505230, 0x3D9)  // dc 0xf36b0
void NewfullMap::stampObject(NewmapCell* thisCell,
                             NewmapCell::TObjectCell* objectCell)
{
    std::vector<NewmapCell::TObjectCell>& objectList = thisCell->m_objects;
    std::vector<NewmapCell::TObjectCell>::iterator position
        = objectList.end();
    CObject* newObject = &m_objects[objectCell->m_objectIndex];

    signed char heightMap[8][6];
    generateHeightMap(newObject, heightMap);

    while (position != objectList.begin()) {
        NewmapCell::TObjectCell& nextCellObjInfo = *(position - 1);
        if (objectCell->m_layer > nextCellObjInfo.m_layer)
            break;

        if (objectCell->m_layer == nextCellObjInfo.m_layer) {
            unsigned char mustCover = 0;
            CObject* belowObject = &m_objects[nextCellObjInfo.m_objectIndex];

            RECT newRect;
            newRect.left = newObject->m_x - m_objectTypes[newObject->m_typeIndex].m_width + 1;
            newRect.top = newObject->m_y - m_objectTypes[newObject->m_typeIndex].m_height + 1;
            newRect.right = newObject->m_x + 1;
            newRect.bottom = newObject->m_y + 1;

            RECT belowRect;
            belowRect.left = belowObject->m_x
                - m_objectTypes[belowObject->m_typeIndex].m_width + 1;
            belowRect.top = belowObject->m_y
                - m_objectTypes[belowObject->m_typeIndex].m_height + 1;
            belowRect.right = belowObject->m_x + 1;
            belowRect.bottom = belowObject->m_y + 1;

            RECT overlap;
            unsigned char intersects = IntersectRect(&overlap, &newRect, &belowRect) != 0;
            if (overlap.left < 0)
                overlap.left = 0;
            if (overlap.top < 0)
                overlap.top = 0;
            if (overlap.right > g_mapWidth)
                overlap.right = g_mapWidth;
            if (overlap.bottom > g_mapHeight)
                overlap.bottom = g_mapHeight;

            for (int x = overlap.left; x < overlap.right; ++x) {
                int objBeingPlacedX = newObject->m_x - x;
                int objOnMapX = belowObject->m_x - x;
                for (int y = overlap.top; y < overlap.bottom; ++y) {
                    int objBeingPlacedY = newObject->m_y - y;
                    int objOnMapY = belowObject->m_y - y;
                    int objBeingPlacedHeight = heightMap[objBeingPlacedX][objBeingPlacedY];
                    NewmapCell* cell = g_game->m_worldMap.cell(
                        x, y, newObject->m_z);
                    std::vector<NewmapCell::TObjectCell>& onMapList = cell->m_objects;
                    const NewmapCell::TObjectCell* scan = onMapList.begin();
                    while (scan->m_objectIndex != nextCellObjInfo.m_objectIndex)
                        ++scan;
                    int objOnMapHeight = scan->m_layer;
                    if (objOnMapHeight > objBeingPlacedHeight) {
                        mustCover = 1;
                        break;
                    }
                }
                if (mustCover)
                    break;
            }
            if (!mustCover)
                break;
        }

        --position;
    }

    objectList.insert(position, *objectCell);
}

// The cell coordinate inside the object comes back out of TObjectCell's
// packed `offsets` byte as two SIGNED nibbles (`sar cl,4` for the row,
// `shl al,4 / sar al,4` for the column), giving the same 47 - row*8 - col bit
// index GenerateHeightMap and PlaceObject use.

VA(0x00505610, 0x3F8)  // dc 0xf3a24
void NewfullMap::calcCellExtra(NewmapCell* thisCell, unsigned char setExtraInfo)
{
    if (setExtraInfo) {
        short storedIndex = thisCell->m_objectTypeIndex;
        if (storedIndex >= 0 && storedIndex < m_objects.size())
            m_objects[storedIndex].m_extraInfo = thisCell->m_extraInfo;
    }

    thisCell->m_isBlocked = 0;
    thisCell->m_objectTypeIndex = -1;
    if (thisCell->m_groundSet == eTerrainRock)
        thisCell->m_isBlocked = 1;

    thisCell->m_typeValue = NOTHING;
    thisCell->m_isTrigger = 0;
    thisCell->m_passable = 1;
    if (thisCell->m_isBeachBorder && thisCell->m_groundSet != eTerrainWater)
        thisCell->m_typeValue = ANCHOR_POINT;

    std::vector<NewmapCell::TObjectCell>::reverse_iterator it;

    for (it = thisCell->m_objects.rbegin(); it != thisCell->m_objects.rend();
         ++it) {
        CObject* object = &m_objects[it->m_objectIndex];
        if (m_sprites[object->m_typeIndex]->getNumFrames(0) > 1)
            thisCell->m_animated = 1;
    }

    for (it = thisCell->m_objects.rbegin(); it != thisCell->m_objects.rend();
         ++it) {
        CObject* object = &m_objects[it->m_objectIndex];
        CObjectType* objectType = &m_objectTypes[object->m_typeIndex];
        signed char packed = it->m_offsets;
        int row = packed >> 4;
        int col = static_cast<signed char>(packed << 4) >> 4;
        if (objectType->m_triggerCells.test(47 - row * 8 - col)) {
            thisCell->m_objectTypeIndex = it->m_objectIndex;
            thisCell->m_typeValue = objectType->m_objectType;
            thisCell->m_isTrigger = 1;
            thisCell->m_objectIndex = static_cast<short>(objectType->m_extra);
            if (setExtraInfo)
                thisCell->m_extraInfo = object->m_extraInfo;
            return;
        }
    }

    for (it = thisCell->m_objects.rbegin(); it != thisCell->m_objects.rend();
         ++it) {
        CObject* object = &m_objects[it->m_objectIndex];
        CObjectType* objectType = &m_objectTypes[object->m_typeIndex];
        if (hasFlag(objectType->m_objectType)) {
            signed char packed = it->m_offsets;
            int row = packed >> 4;
            int col = static_cast<signed char>(packed << 4) >> 4;
            if (!objectType->m_passableCells.test(47 - row * 8 - col)) {
                thisCell->m_objectTypeIndex = it->m_objectIndex;
                thisCell->m_typeValue = objectType->m_objectType;
                thisCell->m_objectIndex = static_cast<short>(objectType->m_extra);
                if (setExtraInfo)
                    thisCell->m_extraInfo = object->m_extraInfo;
                thisCell->m_passable = 0;
                thisCell->m_isBlocked = 1;
                return;
            }
        }
    }

    for (it = thisCell->m_objects.rbegin(); it != thisCell->m_objects.rend();
         ++it) {
        CObject* object = &m_objects[it->m_objectIndex];
        CObjectType* objectType = &m_objectTypes[object->m_typeIndex];
        signed char packed = it->m_offsets;
        int row = packed >> 4;
        int col = static_cast<signed char>(packed << 4) >> 4;
        if (!objectType->m_passableCells.test(47 - row * 8 - col)) {
            thisCell->m_objectTypeIndex = it->m_objectIndex;
            thisCell->m_typeValue = objectType->m_objectType;
            thisCell->m_objectIndex = static_cast<short>(objectType->m_extra);
            if (setExtraInfo)
                thisCell->m_extraInfo = object->m_extraInfo;
            thisCell->m_passable = 0;
            thisCell->m_isBlocked = 1;
            return;
        }
    }

    for (it = thisCell->m_objects.rbegin(); it != thisCell->m_objects.rend();
         ++it) {
        CObject* object = &m_objects[it->m_objectIndex];
        CObjectType* objectType = &m_objectTypes[object->m_typeIndex];
        if (objectType->m_objectType == TERRAIN_HOLE) {
            thisCell->m_objectTypeIndex = it->m_objectIndex;
            thisCell->m_typeValue = objectType->m_objectType;
            thisCell->m_objectIndex = static_cast<short>(objectType->m_extra);
            if (setExtraInfo)
                thisCell->m_extraInfo = object->m_extraInfo;
            return;
        }
    }
}

VA(0x00505a10, 0x108)  // dc 0xf42f0
void NewfullMap::calculateCellExtra(NewmapCell* thisCell, unsigned char setExtraInfo)
{
    hero* obscuringHero = 0;
    boat* obscuringBoat = 0;
    unsigned char restoreHero = 0;
    unsigned char restoreBoat = 0;
    TAdventureObjectType visibleType = thisCell->getMapObject();

    if (visibleType == EVENT)
        return;

    if (thisCell->m_type == HERO) {
        obscuringHero = g_game->getHero(thisCell->m_extraInfo);
        restoreHero = 1;
        obscuringHero->restoreCell();
    }

    if (thisCell->m_type == BOAT) {
        obscuringBoat = g_game->getBoat(thisCell->m_extraInfo);
        restoreBoat = 1;
        obscuringBoat->restoreCell();
    }

    calcCellExtra(thisCell, setExtraInfo);

    if (restoreHero)
        obscuringHero->obscureCell();
    if (restoreBoat)
        obscuringBoat->obscureCell();
}

// Five object classes are filed by something other than this pass and are
// skipped outright: HOLY_GRAIL, HERO, RANDOM_HERO, HERO_PLACEHOLDER and BOAT.
// EVENT is skipped too but not silently - where its trigger mask covers the
// cell, the cell is cleared of IsBlocked and is_trigger, retyped EVENT, and
// given the object's extraInfo.  Everything else builds a TObjectCell and
// hands it to StampObject, then recalculates the cell's derived extra.

// The two bounds tests are not the same test twice: the x test guards the
// whole inner loop (it `continue`s the OUTER one) while the y test guards a
// single cell.  Both compare against the world extents MAP_WIDTH/MAP_HEIGHT
// that game::SetMapSize writes.

VA(0x00505b20, 0x1F2)  // dc 0xf43e8
int NewfullMap::placeObject(int objectIndex, unsigned char setExtraInfo)
{
    CObject* object = &m_objects[objectIndex];
    CObjectType* objectType = &m_objectTypes[object->m_typeIndex];

    signed char heightMap[8][6];
    generateHeightMap(object, heightMap);

    TAdventureObjectType objectClass =
        g_game->m_worldMap.m_objectTypes[object->m_typeIndex].m_objectType;

    for (int col = 0; col < objectType->m_width; ++col) {
        if (object->m_x - col < 0 || object->m_x - col >= g_mapWidth)
            continue;

        for (int row = 0; row < objectType->m_height; ++row) {
            if (object->m_y - row < 0 || object->m_y - row >= g_mapHeight)
                continue;

            NewmapCell* cell = &g_game->m_worldMap.m_cellData[
                (object->m_z * g_game->m_worldMap.m_size + (object->m_y - row))
                    * g_game->m_worldMap.m_size
                + (object->m_x - col)];

            if (objectClass == HOLY_GRAIL || objectClass == HERO
                || objectClass == RANDOM_HERO
                || objectClass == HERO_PLACEHOLDER || objectClass == BOAT)
                continue;

            if (objectClass == EVENT) {
                if (objectType->m_triggerCells.test(47 - row * 8 - col)) {
                    cell->m_isBlocked = 0;
                    cell->m_isTrigger = 0;
                    cell->m_typeValue = EVENT;
                    cell->m_extraInfo = object->m_extraInfo;
                }
                continue;
            }

            NewmapCell::TObjectCell objectCell;
            objectCell.m_objectIndex = static_cast<unsigned short>(objectIndex);
            objectCell.m_offsets = static_cast<unsigned char>((col & 0xf)
                                                            | (row << 4));
            objectCell.m_layer = heightMap[col][row];
            stampObject(cell, &objectCell);
            calculateCellExtra(cell, setExtraInfo);
        }
    }
    return 0;
}

VA(0x00505d20, 0x3F)
void NewfullMap::newfullMapFn00505D20(int heroId, int player)
{
    for (unsigned int i = 0; i < m_mapObjectData.size(); ++i)
        m_mapObjectData[i]->newMapVFn24(heroId, player);
}

VA(0x00505d60, 0x3F)
void NewfullMap::newfullMapFn00505D60(type_point point, int player)
{
    for (unsigned int i = 0; i < m_mapObjectData.size(); ++i)
        m_mapObjectData[i]->newMapVFn28(point, player);
}

VA(0x00505da0, 0xF8)
void NewfullMap::newfullMapFn00505DA0()
{
    TObjectTypeTable objectTypeTable;
    objectTypeTable.load(
        DATA_COMPGEN(0x0067fb84, objectTypeTableFilename, "objects.txt"));

    for (unsigned int i = 0; i < objectTypeTable.m_objectTypes.size(); ++i) {
        TAdventureObjectType objectType =
            objectTypeTable.m_objectTypes[i].m_objectType;
        m_objectTypeIndex[objectType].push_back(
            CObjectType(&objectTypeTable.m_objectTypes[i]));
    }
}

// Safety failure shared by the two required-definition searches below.
// Their callers immediately consume the result; throwing leaves them no null
// or invalid record to dereference. This path is absent in retail.
static void missingMapObjectDefinition()
{
    throw std::out_of_range("Map object definition not found");
}

// 0x505ea0 (game::ConvertObject's helper, declared game.h:475) reverse-scans
// objectTypeIndex[objectType] for the record whose `extra` (CObjectType+0x3c)
// equals `extra`. Retail returns &objectTypeIndex[objectType][-1] on a miss.
// The size()-1..0 reverse-scan tell
// with _First re-read per pass; same idiom as NewfullMapFn_005042C0 above.
// No direct DC counterpart: older game::InsertObject uses a flat forward
// search, not proof of an inline copy of this Complete per-class helper.
// Safety correction: convertObject and both high-score window paths require
// an existing definition and dereference the result. Throw on a miss before
// forming a pointer; returning null alone would leave those callers unsafe.
// Residual (84.0566%): the deliberate failure branch/call is absent in retail.
// Checked vector::at measured 45.2642%; the ordinary shared throw preserves
// the reverse loop and caller contract without inliner directives.
// A further 16-state paired failure-boundary family reproduces all objects:
// reverse-index/exit, postdecrement failure and predecrement failure score
// 69.4340%, 48.3962% and 43.4906%; the retained shared-throw form stays best.
VA(0x00505ea0, 0x80)  // linkorder + this@+0xdc=objectTypeIndex; reverse-find CObjectType by extra, caller game::ConvertObject, retail-only
CObjectType* NewfullMap::newfullMapFn00505EA0(int objectType, int extra)
{
    int i = m_objectTypeIndex[objectType].size();
    while (i--) {
        if (m_objectTypeIndex[objectType][i].m_extra == extra)
            break;
    }
    if (i < 0)
        missingMapObjectDefinition();
    return &m_objectTypeIndex[objectType][i];
}

// 0x505f20 (game::InsertObject's helper, declared game.h:466) reverse-scans
// objectTypeIndex[objectType] for the record matching `objectIndex` (its
// .extra) and, when terrain != -1, applicable to `terrain` (its mask_34 bit).
// If the matched record has no resolved objectTypes index yet (field_42 < 0),
// it appends a copy to objectTypes and its sprite to sprites and records the
// new index, then writes the resolved index into object->typeIndex.
// No direct DC counterpart; this is a Complete per-class helper, not a
// proven DC inline. A missing definition now throws through the same failure
// helper as 0x505ea0, before publishing an index, adding records or acquiring
// a sprite. insertObject and SoD_transformRandomDwellings require that result.
// Residual (97.7099%): intentional failure branch/call absent in retail;
// vector::at measured 63.1832%, local throw 51.1908%. Actual-body native tests
// cover reverse precedence, terrain filters, empty/missing classes, cached
// indices and no publication on failure, with three rejected negative controls.
// The paired 16-state failure-boundary family also leaves this form best:
// reverse-index/exit 71.7557%, postdecrement failure 66.5802%, predecrement
// failure 69.2824%, all independently reproduced with the canonical throw.
// MATCHING_DEBT: the two vector inserts are pinned with inline_depth(0) (the
// randomDwellings idiom) with end() and the value hoisted out so operator[]
// and end() stay inline; sprites is named as a reference so its _Last is read
// through &sprites, not folded off `this`.
// EXACT 2026-08-26 (99.9771 -> 100.0000): retail's otherwise-dead third
// frame slot comes from the non-const bitset operator[] proxy.  Spelling the
// mask test as `.test(terrain)` emits identical executable operations but
// lets VC6 compact the frame from 0xc to 0x8.  The three remaining displayed
// reloc-name rows are cosmetic cross-TU unclaimed STL/data identities and do
// not score.  A 316-candidate generated declaration/name search was flat;
// the older reference/push_back hypothesis over-inlines (21 vs 10 branches).
VA(0x00505f20, 0x157)  // linkorder + this@+0xdc=objectTypeIndex; caller game::InsertObject, retail-only
void NewfullMap::newfullMapFn00505F20(CObject* object, int objectType,
                                       int objectIndex, int terrain)
{
    int i = m_objectTypeIndex[objectType].size();
    while (i--) {
        if (m_objectTypeIndex[objectType][i].m_extra == objectIndex) {
            if (terrain == -1)
                break;
            if (m_objectTypeIndex[objectType][i].m_mask34[terrain])
                break;
        }
    }

    if (i < 0)
        missingMapObjectDefinition();

    if (static_cast<short>(m_objectTypeIndex[objectType][i].m_objectTypeIndex) < 0) {
        m_objectTypeIndex[objectType][i].m_objectTypeIndex =
            static_cast<unsigned short>(m_objectTypes.size());
        CObjectType& newType = m_objectTypeIndex[objectType][i];
        std::vector<CObjectType>::iterator typeEnd = m_objectTypes.end();
#pragma inline_depth(0)
        m_objectTypes.insert(typeEnd, 1, newType);
#pragma inline_depth()
        CSprite* sprite = ResourceManager::getSprite(
            m_objectTypeIndex[objectType][i].m_imageName.c_str());
        std::vector<CSprite*>& spriteList = m_sprites;
        std::vector<CSprite*>::iterator spriteEnd = spriteList.end();
#pragma inline_depth(0)
        spriteList.insert(spriteEnd, 1, sprite);
#pragma inline_depth()
    }

    object->m_typeIndex = m_objectTypeIndex[objectType][i].m_objectTypeIndex;
}

// The map-editor template's conversion into the runtime object-type record,
// and the first row past NewfullMapFn_00505F20 - the constructor
// NewfullMapFn_00505DA0 calls once per row of objects.txt, whose address
// advmgr_objects.h already records against this class.

// The four 48-cell masks are transposed cell by cell through the class's own
// _getBitPos(x, y) = 47 - y * 8 - x.

// Residual (62.10%): the bitset members, and only them. Retail CALLS
// bitset<48>::test at two of the four reads and expands the range check at
// the other two; our /Ob2 budget expands more of them, which is the block
// surplus and the surplus out_of_range throw path.
// The DEPTH LADDER (docs/vc6/inliner.md 6b) moved this row 39.16 -> 62.10:
// the five WRITES spelled `bits[i] = v` rather than `bits.set(i, v)`.
// Titrated, all measured at the same delink generation:
//   writes .set  + reads .test  (the old spelling)      39.1636
//   writes [i]   + reads .test  (SHIPPED)               62.0970
//   writes [i]   + reads [i]                            36.4970
//   inner-4 [i]  + mask_34 .set                         43.3576
//   inner-4 .set + mask_34 [i]                          34.6000
// so the five writes only pay TOGETHER, and flipping the reads costs 25.6.
// Retail retains two `test` calls. Repeating _getBitPos at all accessors
// falls to 31.22%; separate input/output indices are byte-flat at 58.55%.
// Explicit bool read values are 58.57%, const recommended-mask access is
// 44.32%, and at() reads add checks (37.03%). None recovers those two calls.
// Both retail grid backedges use jb; paired unsigned indices recover that
// shape at 61.08% in this header state. The DC _getBitPos helper likewise
// takes unsigned x/y (MapCell.h:565). A fixed-grid do/while is only 57.67%.
// The prior 62.35% remains banked; these controls do not settle the missing
// per-site compiler state.
// Mixed read subscripts also fail: draw/passable [] with shadow/trigger
// .test is 40.45%; the inverse is 38.82% (both 576 bytes). Neither recovers
// retail's pair of retained test calls; keep all four source reads as .test.
// A const input view with all four [] reads is 38.28% (608 bytes): the
// byte-verified trace admits every depth-two test (cost 58, budgets
// 63/67/74/88). Direct .test likewise admits all four at depth one, with
// budgets 956/814/672/465. Const access alone does not explain the frontier.

// The image name, sizes, four masks, recommended-terrain mask, type, subtype
// and underlay flag cross here. hasTrigger, triggerCell, slotCategory and
// terrainMask stay with the editor template.
VA(0x00506080, 0x1D4)  // sole caller NewfullMapFn_00505DA0 + advmgr_objects.h address, retail-only
CObjectType::CObjectType(TObjectType* source)
{
    m_imageName = source->getImageName();
    m_width = source->getWidth();
    m_height = source->getHeight();

    for (unsigned y = 0; y < 6; y++) {
        for (unsigned x = 0; x < 8; x++) {
            unsigned pos = getBitPos(x, y);
            m_drawCells[pos] = source->m_imageInfo.m_drawMask.test(pos);
            m_passableCells[pos] = source->m_passableMask.test(pos);
            m_shadowCells[pos] = source->m_imageInfo.m_shadowMask.test(pos);
            m_triggerCells[pos] = source->m_triggerMask.test(pos);
        }
    }

    for (int terrain = 0; terrain < 10; terrain++)
        m_mask34[terrain] = source->m_recommendedTerrainMask[terrain];

    m_objectType = source->m_objectType;
    m_extra = source->m_subtype;
    m_suppressDraw = source->m_isUnderlay;
}

VA_COMPGEN(0x00506260, 0x38, VECTOR_DTOR, CObjectType)
VA_COMPGEN(0x005062a0, 0x21, VECTOR_SIZE, CObjectType)
VA_COMPGEN(0x005062d0, 0x38, VECTOR_DTOR, TreasureData)
VA_COMPGEN(0x00506310, 0x38, VECTOR_DTOR, MonsterData)
VA_COMPGEN(0x00506350, 0x3B, VECTOR_DTOR, BlackBoxData)
VA_COMPGEN(0x00506390, 0x312, VECTOR_RESIZE, TSeerHut)
VA_COMPGEN(0x005066b0, 0x21, VECTOR_SIZE, TSeerHut)
VA_COMPGEN(0x005066e0, 0x20, VECTOR_SIZE, TQuestGuard)
VA_COMPGEN(0x00506700, 0x38, VECTOR_DTOR, TTimedEvent)
VA_COMPGEN(0x00506740, 0x38, VECTOR_DTOR, TTownEvent)
VA_COMPGEN(0x00506880, 0x17, BITSET_TIDY, Bitset10)
VA_COMPGEN(0x005068a0, 0xE0, VECTOR_ERASE, CObjectType)
VA_COMPGEN(0x00506980, 0x340, VECTOR_INSERT, TreasureData)
VA_COMPGEN(0x00506cc0, 0xA7, VECTOR_ERASE, TreasureData)
VA_COMPGEN(0x00506d70, 0x32C, VECTOR_INSERT, MonsterData)
VA_COMPGEN(0x005070a0, 0xA3, VECTOR_ERASE, MonsterData)
VA_COMPGEN(0x00507150, 0x32E, VECTOR_INSERT, BlackBoxData)
VA_COMPGEN(0x00507480, 0x14D, VECTOR_ERASE, BlackBoxData)
VA_COMPGEN(0x005075d0, 0x26B, VECTOR_INSERT, TSeerHut)
VA_COMPGEN(0x00507840, 0x61, VECTOR_ERASE, TSeerHut)
VA_COMPGEN(0x005078b0, 0x217, VECTOR_INSERT, TQuestGuard)
VA_COMPGEN(0x00507dd0, 0x47, VECTOR_ERASE, TQuestGuard)
VA_COMPGEN(0x00507e20, 0x328, VECTOR_INSERT, TTimedEvent)
VA_COMPGEN(0x00508150, 0xC2, VECTOR_ERASE, TTimedEvent)
VA_COMPGEN(0x00508220, 0x23, VECTOR_DESTROY, TTimedEvent)
VA_COMPGEN(0x00508250, 0x34E, VECTOR_INSERT, TTownEvent)
VA_COMPGEN(0x005085a0, 0xF3, VECTOR_ERASE, TTownEvent)
VA_COMPGEN(0x005086a0, 0x23, VECTOR_DESTROY, TTownEvent)
VA_COMPGEN(0x005086d0, 0x53, VECTOR_ERASE, HeroPlaceholderData)
VA_COMPGEN(0x00508730, 0x12E, VECTOR_ERASE, TownExtra)
VA_COMPGEN(0x00508860, 0x44, VECTOR_ERASE, generator)
VA_COMPGEN(0x005088b0, 0x58, VECTOR_UCOPY, TSeerHut)
VA_COMPGEN(0x00508910, 0x4E, VECTOR_UFILL, TSeerHut)
VA_COMPGEN(0x00508960, 0x3E, VECTOR_UCOPY, TQuestGuard)
VA_COMPGEN(0x005089a0, 0x34, VECTOR_UFILL, TQuestGuard)
// RandomDwellingData's instantiation precedes the byte-identical
// HeroPlaceholderData instantiation in mapcell.obj and owns the folded body.
VA_COMPGEN(0x005089e0, 0x30A, VECTOR_INSERT, RandomDwellingData)
VA_COMPGEN(0x005090b0, 0x30C, VECTOR_INSERT, generator)
VA_COMPGEN(0x005093c0, 0x25, STD_COPY, Int)
VA_COMPGEN(0x0054df40, 0x25, STD_COPY, const_int)
VA_COMPGEN(0x005093f0, 0x1A4, STD_COPY, TTimedEvent)
VA_COMPGEN(0x005095e0, 0x3F, STD_COPY, type_university)
VA_COMPGEN(0x00509620, 0x207, STD_COPY, type_creature_bank)
VA_COMPGEN(0x00509830, 0x168, STD_CONSTRUCT, TreasureData)
VA_COMPGEN(0x005099a0, 0x16A, STD_CONSTRUCT, MonsterData)
VA_COMPGEN(0x00509b10, 0x208, STD_CONSTRUCT, BlackBoxData)
VA_COMPGEN(0x00509d20, 0x29, STD_CONSTRUCT, TSeerHut)
VA_COMPGEN(0x00509d50, 0x0F, STD_CONSTRUCT, TQuestGuard)
VA_COMPGEN(0x00509d60, 0x188, STD_CONSTRUCT, TTimedEvent)
VA_COMPGEN(0x00509ef0, 0x1BA, STD_CONSTRUCT, TTownEvent)
VA_COMPGEN(0x0050a0b0, 0x1DD, STD_CONSTRUCT, TownExtra)
VA_COMPGEN(0x0050a290, 0x161, IMPLICIT_COPY_ASSIGN, MonsterData)
VA_COMPGEN(0x0050a400, 0x2F6, IMPLICIT_COPY_ASSIGN, BlackBoxData)
VA_COMPGEN(0x0050a700, 0x17A, IMPLICIT_COPY_ASSIGN, TTimedEvent)
VA_COMPGEN(0x0050a880, 0x1A2, IMPLICIT_COPY_ASSIGN, TTownEvent)
VA_COMPGEN(0x0050aa30, 0x1CD, IMPLICIT_COPY_ASSIGN, TownExtra)
// vector<TArtifact>::operator= is the first emitted owner of the body also
// called through the byte-identical vector<int> specialization.
VA_COMPGEN(0x0050ac00, 0x188, VECTOR_COPY_ASSIGN, TArtifact)
VA_COMPGEN(0x0050ad90, 0x13, VECTOR_CAPACITY, SecondarySkillData)
VA_COMPGEN(0x0050adb0, 0x2B, STD_COPY, SecondarySkillData)

#if 0  // @carcass -- located/reconstruction-pending bodies

// E:\gamedcs\mapcell.cpp:4404
DC_ONLY(0xf4740, 0x50)
int NewfullMap::placeObjects()
{
    // @stub
}

// E:\gamedcs\MapCell.h:333
DC_ONLY(0xf4790, 0x3C)
void TreasureData::TreasureData()
{
    // @stub
}

// E:\gamedcs\MapCell.h:364
DC_ONLY(0xf47cc, 0xF0)
void BlackBoxData::BlackBoxData()
{
    // @stub
}

// E:\gamedcs\MapCell.h:364
DC_ONLY(0xf48bc, 0x1C)
void TreasureData::~TreasureData()
{
    // @stub
}

// E:\gamedcs\MapCell.h:400
DC_ONLY(0xf48d8, 0x30)
void TTownEvent::TTownEvent()
{
    // @stub
}

// E:\gamedcs\MapCell.h:400
DC_ONLY(0xf4908, 0x20)
void TTimedEvent::TTimedEvent()
{
    // @stub
}

// E:\gamedcs\MapCell.h:400
DC_ONLY(0xf4928, 0x1C)
void TTimedEvent::~TTimedEvent()
{
    // @stub
}

// E:\gamedcs\MapCell.h:587
DC_ONLY(0xf4944, 0x60)
void CObject::CObject()
{
    // @stub
}

// NewmapCell::NewmapCell (dc 0xf49a4) moved up: claimed at VA 0x004fd650.

// E:\gamedcs\MapCell.h:735
DC_ONLY(0xf4a50, 0x28)
void MonsterData::MonsterData()
{
    // @stub
}

// CObject::get_type (dc 0xf4a78) moved to its original inline definition in
// include/advmgr_objects.h; retail emits no out-of-line copy.

// type_obscuring_object::get_obscured_object (dc 0xf4a9c) moved to its
// original inline definition in include/hero.h; retail emits no out-of-line
// copy.

// E:\gamedcs\Hero.h:167
DC_ONLY(0xf4abc, 0x32)
unsigned char type_obscuring_object::get_obscured_trigger()
{
    // @stub
}

// E:\gamedcs\game.h:377
DC_ONLY(0xf4af0, 0x48)
void TownExtra::TownExtra()
{
    // @stub
}

// E:\gamedcs\seerhut.h:108
DC_ONLY(0xf4b38, 0x2A)
void TSeerHut::TSeerHut()
{
    // @stub
}

// E:\gamedcs\mapcell.cpp:544
DC_ONLY(0xf4b64, 0x78)
void* NewmapCell::`vector deleting destructor'(unsigned __flags)
{
    // @stub
}

// NewmapCell::~NewmapCell (dc 0xf4bdc) moved up: claimed at VA 0x004fd4c0.
// BlackBoxData::~BlackBoxData (dc 0xf4bfc) moved up: claimed at VA 0x004ffdf0.

// E:\gamedcs\mapcell.cpp:2692
DC_ONLY(0xf4c50, 0x1C)
void MonsterData::~MonsterData()
{
    // @stub
}

// E:\gamedcs\mapcell.cpp:2947
DC_ONLY(0xf4c6c, 0x1C)
void TTownEvent::~TTownEvent()
{
    // @stub
}

// ..\stlport\stl_bitset.h:565
DC_ONLY(0xf4c88, 0x28)
unsigned char std::bitset<48,unsigned long>::operator[](unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf4cb0, 0x24)
void std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::vector<SecondarySkillData,std::allocator<SecondarySkillData> >(const std::allocator<SecondarySkillData>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf4cd4, 0x34)
void std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::~vector<SecondarySkillData,std::allocator<SecondarySkillData> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf4d08, 0x24)
void std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf4d2c, 0x3C)
void std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf4d68, 0x8)
void std::allocator<SecondarySkillData>::allocator<SecondarySkillData>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf4d70, 0x6)
void std::allocator<SecondarySkillData>::~allocator<SecondarySkillData>()
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf4d78, 0x3C)
void std::vector<enum TArtifact,std::allocator<enum TArtifact> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf4db4, 0x28)
void std::vector<enum SpellID,std::allocator<enum SpellID> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf4ddc, 0x3C)
void std::vector<enum SpellID,std::allocator<enum SpellID> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:186
DC_ONLY(0xf4e18, 0x50)
std::reverse_iterator<NewmapCell::TObjectCell std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::rbegin(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_vector.h:190
DC_ONLY(0xf4e68, 0x50)
std::reverse_iterator<NewmapCell::TObjectCell std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::rend(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_vector.h:204
DC_ONLY(0xf4eb8, 0x28)
const NewmapCell::TObjectCell* std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf4ee0, 0x24)
void std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >(const std::allocator<NewmapCell::TObjectCell>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf4f04, 0x34)
void std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::~vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:393
DC_ONLY(0xf4f38, 0x108)
NewmapCell::TObjectCell* std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::insert(NewmapCell::TObjectCell* __position, const NewmapCell::TObjectCell* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf5040, 0x24)
void std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf5064, 0x8)
void std::allocator<NewmapCell::TObjectCell>::allocator<NewmapCell::TObjectCell>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf506c, 0x6)
void std::allocator<NewmapCell::TObjectCell>::~allocator<NewmapCell::TObjectCell>()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf5074, 0x24)
void std::vector<CObjectType,std::allocator<CObjectType> >::vector<CObjectType,std::allocator<CObjectType> >(const std::allocator<CObjectType>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf5098, 0x34)
void std::vector<CObjectType,std::allocator<CObjectType> >::~vector<CObjectType,std::allocator<CObjectType> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf50cc, 0x48)
void std::vector<CObjectType,std::allocator<CObjectType> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5114, 0x3C)
void std::vector<CObjectType,std::allocator<CObjectType> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf5150, 0x8)
void std::allocator<CObjectType>::allocator<CObjectType>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf5158, 0x6)
void std::allocator<CObjectType>::~allocator<CObjectType>()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf5160, 0x24)
void std::vector<CObject,std::allocator<CObject> >::vector<CObject,std::allocator<CObject> >(const std::allocator<CObject>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf5184, 0x34)
void std::vector<CObject,std::allocator<CObject> >::~vector<CObject,std::allocator<CObject> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf51b8, 0x34)
void std::vector<CObject,std::allocator<CObject> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf51ec, 0x3C)
void std::vector<CObject,std::allocator<CObject> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf5228, 0x8)
void std::allocator<CObject>::allocator<CObject>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf5230, 0x6)
void std::allocator<CObject>::~allocator<CObject>()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf5238, 0x24)
void std::vector<CSprite *,std::allocator<CSprite *> >::vector<CSprite *,std::allocator<CSprite *> >(const std::allocator<CSprite* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf525c, 0x34)
void std::vector<CSprite *,std::allocator<CSprite *> >::~vector<CSprite *,std::allocator<CSprite *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf5290, 0x28)
void std::vector<CSprite *,std::allocator<CSprite *> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf52b8, 0x3C)
void std::vector<CSprite *,std::allocator<CSprite *> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf52f4, 0x8)
void std::allocator<CSprite *>::allocator<CSprite *>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf52fc, 0x6)
void std::allocator<CSprite *>::~allocator<CSprite *>()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0xf5304, 0x2C)
unsigned std::vector<TreasureData,std::allocator<TreasureData> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf5330, 0x24)
void std::vector<TreasureData,std::allocator<TreasureData> >::vector<TreasureData,std::allocator<TreasureData> >(const std::allocator<TreasureData>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf5354, 0x34)
void std::vector<TreasureData,std::allocator<TreasureData> >::~vector<TreasureData,std::allocator<TreasureData> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0xf5388, 0x54)
void std::vector<TreasureData,std::allocator<TreasureData> >::push_back(const TreasureData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf53dc, 0x48)
void std::vector<TreasureData,std::allocator<TreasureData> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5424, 0x3C)
void std::vector<TreasureData,std::allocator<TreasureData> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf5460, 0x8)
void std::allocator<TreasureData>::allocator<TreasureData>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf5468, 0x6)
void std::allocator<TreasureData>::~allocator<TreasureData>()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0xf5470, 0x2C)
unsigned std::vector<MonsterData,std::allocator<MonsterData> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf549c, 0x24)
void std::vector<MonsterData,std::allocator<MonsterData> >::vector<MonsterData,std::allocator<MonsterData> >(const std::allocator<MonsterData>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf54c0, 0x34)
void std::vector<MonsterData,std::allocator<MonsterData> >::~vector<MonsterData,std::allocator<MonsterData> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0xf54f4, 0x54)
void std::vector<MonsterData,std::allocator<MonsterData> >::push_back(const MonsterData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf5548, 0x48)
void std::vector<MonsterData,std::allocator<MonsterData> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5590, 0x3C)
void std::vector<MonsterData,std::allocator<MonsterData> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf55cc, 0x8)
void std::allocator<MonsterData>::allocator<MonsterData>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf55d4, 0x6)
void std::allocator<MonsterData>::~allocator<MonsterData>()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0xf55dc, 0x30)
unsigned std::vector<BlackBoxData,std::allocator<BlackBoxData> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf560c, 0x24)
void std::vector<BlackBoxData,std::allocator<BlackBoxData> >::vector<BlackBoxData,std::allocator<BlackBoxData> >(const std::allocator<BlackBoxData>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf5630, 0x34)
void std::vector<BlackBoxData,std::allocator<BlackBoxData> >::~vector<BlackBoxData,std::allocator<BlackBoxData> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0xf5664, 0x58)
void std::vector<BlackBoxData,std::allocator<BlackBoxData> >::push_back(const BlackBoxData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf56bc, 0x54)
void std::vector<BlackBoxData,std::allocator<BlackBoxData> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5710, 0x3C)
void std::vector<BlackBoxData,std::allocator<BlackBoxData> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf574c, 0x8)
void std::allocator<BlackBoxData>::allocator<BlackBoxData>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf5754, 0x6)
void std::allocator<BlackBoxData>::~allocator<BlackBoxData>()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf575c, 0x24)
void std::vector<TSeerHut,std::allocator<TSeerHut> >::vector<TSeerHut,std::allocator<TSeerHut> >(const std::allocator<TSeerHut>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf5780, 0x34)
void std::vector<TSeerHut,std::allocator<TSeerHut> >::~vector<TSeerHut,std::allocator<TSeerHut> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0xf57b4, 0x54)
void std::vector<TSeerHut,std::allocator<TSeerHut> >::push_back(const TSeerHut* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5808, 0x3C)
void std::vector<TSeerHut,std::allocator<TSeerHut> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf5844, 0x8)
void std::allocator<TSeerHut>::allocator<TSeerHut>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf584c, 0x6)
void std::allocator<TSeerHut>::~allocator<TSeerHut>()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf5854, 0x24)
void std::vector<TTimedEvent,std::allocator<TTimedEvent> >::vector<TTimedEvent,std::allocator<TTimedEvent> >(const std::allocator<TTimedEvent>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf5878, 0x34)
void std::vector<TTimedEvent,std::allocator<TTimedEvent> >::~vector<TTimedEvent,std::allocator<TTimedEvent> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf58ac, 0x48)
void std::vector<TTimedEvent,std::allocator<TTimedEvent> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf58f4, 0x3C)
void std::vector<TTimedEvent,std::allocator<TTimedEvent> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf5930, 0x8)
void std::allocator<TTimedEvent>::allocator<TTimedEvent>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf5938, 0x6)
void std::allocator<TTimedEvent>::~allocator<TTimedEvent>()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0xf5940, 0x24)
void std::vector<TTownEvent,std::allocator<TTownEvent> >::vector<TTownEvent,std::allocator<TTownEvent> >(const std::allocator<TTownEvent>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0xf5964, 0x34)
void std::vector<TTownEvent,std::allocator<TTownEvent> >::~vector<TTownEvent,std::allocator<TTownEvent> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0xf5998, 0x54)
void std::vector<TTownEvent,std::allocator<TTownEvent> >::push_back(const TTownEvent* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:505
DC_ONLY(0xf59ec, 0x48)
void std::vector<TTownEvent,std::allocator<TTownEvent> >::resize(unsigned __new_size)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5a34, 0x3C)
void std::vector<TTownEvent,std::allocator<TTownEvent> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0xf5a70, 0x8)
void std::allocator<TTownEvent>::allocator<TTownEvent>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0xf5a78, 0x6)
void std::allocator<TTownEvent>::~allocator<TTownEvent>()
{
    // @stub
}

// ..\stlport\stl_bitset.h:414
DC_ONLY(0xf5a80, 0x20)
void std::bitset<70,unsigned long>::bitset<70,unsigned long>()
{
    // @stub
}

// ..\stlport\stl_bitset.h:564
DC_ONLY(0xf5aa0, 0x28)
std::bitset<70,unsigned std::bitset<70,unsigned long>::operator[](__$ReturnUdt, unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_bitset.h:376
DC_ONLY(0xf5ac8, 0x6)
void std::bitset<70,unsigned long>::reference::~reference()
{
    // @stub
}

// ..\stlport\stl_bitset.h:379
DC_ONLY(0xf5ad0, 0x60)
std::bitset<70,unsigned* std::bitset<70,unsigned long>::reference::operator=(unsigned char __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0xf5b30, 0x54)
void std::vector<TownExtra,std::allocator<TownExtra> >::push_back(const TownExtra* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5b84, 0x3C)
void std::vector<TownExtra,std::allocator<TownExtra> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5bc0, 0x3C)
void std::vector<town,std::allocator<town> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0xf5bfc, 0x54)
void std::vector<Sign,std::allocator<Sign> >::push_back(const Sign* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5c50, 0x3C)
void std::vector<Sign,std::allocator<Sign> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0xf5c8c, 0x54)
void std::vector<mine,std::allocator<mine> >::push_back(const mine* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5ce0, 0x3C)
void std::vector<mine,std::allocator<mine> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0xf5d1c, 0x54)
void std::vector<generator,std::allocator<generator> >::push_back(const generator* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5d70, 0x3C)
void std::vector<generator,std::allocator<generator> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0xf5dac, 0x54)
void std::vector<garrison,std::allocator<garrison> >::push_back(const garrison* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5e00, 0x3C)
void std::vector<garrison,std::allocator<garrison> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5e3c, 0x3C)
void std::vector<boat,std::allocator<boat> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5e78, 0x3C)
void std::vector<type_university,std::allocator<type_university> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0xf5eb4, 0x3C)
void std::vector<type_creature_bank,std::allocator<type_creature_bank> >::clear()
{
    // @stub
}

// ..\stlport\stl_iterator.h:324
DC_ONLY(0xf5ef0, 0x8)
void std::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>()
{
    // @stub
}

// ..\stlport\stl_iterator.h:327
DC_ONLY(0xf5ef8, 0x28)
std::reverse_iterator<NewmapCell::TObjectCell* std::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>::operator=(const std::reverse_iterator<NewmapCell::TObjectCell* __x)
{
    // @stub
}

// ..\stlport\stl_iterator.h:333
DC_ONLY(0xf5f20, 0x20)
NewmapCell::TObjectCell* std::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>::operator->()
{
    // @stub
}

// ..\stlport\stl_iterator.h:340
DC_ONLY(0xf5f40, 0x48)
std::reverse_iterator<NewmapCell::TObjectCell std::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>::operator++(__$ReturnUdt, int __formal)
{
    // @stub
}

// ..\stlport\stl_string.h:344
DC_ONLY(0xf5f88, 0x5C)
void CObjectType::CObjectType()
{
    // @stub
}

// ..\stlport\stl_bitset.h:507
DC_ONLY(0xf5fe4, 0x40)
unsigned char std::bitset<48,unsigned long>::_Unchecked_test(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0xf6024, 0xC)
SecondarySkillData* std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf6030, 0x48)
SecondarySkillData* std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::erase(SecondarySkillData* __first, SecondarySkillData* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf6078, 0x90)
void std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::resize(unsigned __new_size, const SecondarySkillData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf6108, 0x44)
void std::_Vector_base<SecondarySkillData,std::allocator<SecondarySkillData> >::_Vector_base<SecondarySkillData,std::allocator<SecondarySkillData> >(const std::allocator<SecondarySkillData>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf614c, 0x44)
void std::_Vector_base<SecondarySkillData,std::allocator<SecondarySkillData> >::~_Vector_base<SecondarySkillData,std::allocator<SecondarySkillData> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf6190, 0x48)
SpellID* std::vector<enum SpellID,std::allocator<enum SpellID> >::erase(SpellID* __first, SpellID* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf61d8, 0x90)
void std::vector<enum SpellID,std::allocator<enum SpellID> >::resize(unsigned __new_size, const SpellID* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:180
DC_ONLY(0xf6268, 0xC)
const NewmapCell::TObjectCell* std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf6274, 0x90)
void std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::resize(unsigned __new_size, const NewmapCell::TObjectCell* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf6304, 0x44)
void std::_Vector_base<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::_Vector_base<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >(const std::allocator<NewmapCell::TObjectCell>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf6348, 0x44)
void std::_Vector_base<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::~_Vector_base<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf638c, 0x48)
CObjectType* std::vector<CObjectType,std::allocator<CObjectType> >::erase(CObjectType* __first, CObjectType* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf63d4, 0x94)
void std::vector<CObjectType,std::allocator<CObjectType> >::resize(unsigned __new_size, const CObjectType* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf6468, 0x44)
void std::_Vector_base<CObjectType,std::allocator<CObjectType> >::_Vector_base<CObjectType,std::allocator<CObjectType> >(const std::allocator<CObjectType>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf64ac, 0x54)
void std::_Vector_base<CObjectType,std::allocator<CObjectType> >::~_Vector_base<CObjectType,std::allocator<CObjectType> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0xf6500, 0xC)
CObject* std::vector<CObject,std::allocator<CObject> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf650c, 0x48)
CObject* std::vector<CObject,std::allocator<CObject> >::erase(CObject* __first, CObject* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf6554, 0x94)
void std::vector<CObject,std::allocator<CObject> >::resize(unsigned __new_size, const CObject* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf65e8, 0x44)
void std::_Vector_base<CObject,std::allocator<CObject> >::_Vector_base<CObject,std::allocator<CObject> >(const std::allocator<CObject>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf662c, 0x54)
void std::_Vector_base<CObject,std::allocator<CObject> >::~_Vector_base<CObject,std::allocator<CObject> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0xf6680, 0xC)
CSprite** std::vector<CSprite *,std::allocator<CSprite *> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf668c, 0x48)
CSprite** std::vector<CSprite *,std::allocator<CSprite *> >::erase(CSprite** __first, CSprite** __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf66d4, 0x90)
void std::vector<CSprite *,std::allocator<CSprite *> >::resize(unsigned __new_size, CSprite** __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf6764, 0x44)
void std::_Vector_base<CSprite *,std::allocator<CSprite *> >::_Vector_base<CSprite *,std::allocator<CSprite *> >(const std::allocator<CSprite* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf67a8, 0x44)
void std::_Vector_base<CSprite *,std::allocator<CSprite *> >::~_Vector_base<CSprite *,std::allocator<CSprite *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0xf67ec, 0xC)
TreasureData* std::vector<TreasureData,std::allocator<TreasureData> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf67f8, 0x48)
TreasureData* std::vector<TreasureData,std::allocator<TreasureData> >::erase(TreasureData* __first, TreasureData* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf6840, 0x94)
void std::vector<TreasureData,std::allocator<TreasureData> >::resize(unsigned __new_size, const TreasureData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf68d4, 0x44)
void std::_Vector_base<TreasureData,std::allocator<TreasureData> >::_Vector_base<TreasureData,std::allocator<TreasureData> >(const std::allocator<TreasureData>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf6918, 0x54)
void std::_Vector_base<TreasureData,std::allocator<TreasureData> >::~_Vector_base<TreasureData,std::allocator<TreasureData> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0xf696c, 0xC)
MonsterData* std::vector<MonsterData,std::allocator<MonsterData> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf6978, 0x48)
MonsterData* std::vector<MonsterData,std::allocator<MonsterData> >::erase(MonsterData* __first, MonsterData* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf69c0, 0x94)
void std::vector<MonsterData,std::allocator<MonsterData> >::resize(unsigned __new_size, const MonsterData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf6a54, 0x44)
void std::_Vector_base<MonsterData,std::allocator<MonsterData> >::_Vector_base<MonsterData,std::allocator<MonsterData> >(const std::allocator<MonsterData>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf6a98, 0x54)
void std::_Vector_base<MonsterData,std::allocator<MonsterData> >::~_Vector_base<MonsterData,std::allocator<MonsterData> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0xf6aec, 0xC)
BlackBoxData* std::vector<BlackBoxData,std::allocator<BlackBoxData> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf6af8, 0x48)
BlackBoxData* std::vector<BlackBoxData,std::allocator<BlackBoxData> >::erase(BlackBoxData* __first, BlackBoxData* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf6b40, 0x94)
void std::vector<BlackBoxData,std::allocator<BlackBoxData> >::resize(unsigned __new_size, const BlackBoxData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf6bd4, 0x44)
void std::_Vector_base<BlackBoxData,std::allocator<BlackBoxData> >::_Vector_base<BlackBoxData,std::allocator<BlackBoxData> >(const std::allocator<BlackBoxData>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf6c18, 0x54)
void std::_Vector_base<BlackBoxData,std::allocator<BlackBoxData> >::~_Vector_base<BlackBoxData,std::allocator<BlackBoxData> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0xf6c6c, 0xC)
TSeerHut* std::vector<TSeerHut,std::allocator<TSeerHut> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf6c78, 0x48)
TSeerHut* std::vector<TSeerHut,std::allocator<TSeerHut> >::erase(TSeerHut* __first, TSeerHut* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf6cc0, 0x44)
void std::_Vector_base<TSeerHut,std::allocator<TSeerHut> >::_Vector_base<TSeerHut,std::allocator<TSeerHut> >(const std::allocator<TSeerHut>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf6d04, 0x54)
void std::_Vector_base<TSeerHut,std::allocator<TSeerHut> >::~_Vector_base<TSeerHut,std::allocator<TSeerHut> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0xf6d58, 0xC)
TTimedEvent* std::vector<TTimedEvent,std::allocator<TTimedEvent> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf6d64, 0x48)
TTimedEvent* std::vector<TTimedEvent,std::allocator<TTimedEvent> >::erase(TTimedEvent* __first, TTimedEvent* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf6dac, 0x94)
void std::vector<TTimedEvent,std::allocator<TTimedEvent> >::resize(unsigned __new_size, const TTimedEvent* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf6e40, 0x44)
void std::_Vector_base<TTimedEvent,std::allocator<TTimedEvent> >::_Vector_base<TTimedEvent,std::allocator<TTimedEvent> >(const std::allocator<TTimedEvent>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf6e84, 0x54)
void std::_Vector_base<TTimedEvent,std::allocator<TTimedEvent> >::~_Vector_base<TTimedEvent,std::allocator<TTimedEvent> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0xf6ed8, 0xC)
TTownEvent* std::vector<TTownEvent,std::allocator<TTownEvent> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf6ee4, 0x48)
TTownEvent* std::vector<TTownEvent,std::allocator<TTownEvent> >::erase(TTownEvent* __first, TTownEvent* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:499
DC_ONLY(0xf6f2c, 0x94)
void std::vector<TTownEvent,std::allocator<TTownEvent> >::resize(unsigned __new_size, const TTownEvent* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0xf6fc0, 0x44)
void std::_Vector_base<TTownEvent,std::allocator<TTownEvent> >::_Vector_base<TTownEvent,std::allocator<TTownEvent> >(const std::allocator<TTownEvent>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0xf7004, 0x54)
void std::_Vector_base<TTownEvent,std::allocator<TTownEvent> >::~_Vector_base<TTownEvent,std::allocator<TTownEvent> >()
{
    // @stub
}

// ..\stlport\stl_bitset.h:107
DC_ONLY(0xf7058, 0x20)
void std::_Base_bitset<3,unsigned long>::_Base_bitset<3,unsigned long>()
{
    // @stub
}

// ..\stlport\stl_bitset.h:370
DC_ONLY(0xf7078, 0x40)
void std::bitset<70,unsigned long>::reference::reference(std::bitset<70,unsigned* __b, unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0xf70b8, 0xC)
TownExtra* std::vector<TownExtra,std::allocator<TownExtra> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf70c4, 0x48)
TownExtra* std::vector<TownExtra,std::allocator<TownExtra> >::erase(TownExtra* __first, TownExtra* __last)
{
    // @stub
}

// ..\stlport\stl_iterator.h:325
DC_ONLY(0xf710c, 0x28)
void std::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>(const std::reverse_iterator<NewmapCell::TObjectCell* __x)
{
    // @stub
}

// ..\stlport\stl_iterator.h:326
DC_ONLY(0xf7134, 0x10)
void std::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>(NewmapCell::TObjectCell* __x)
{
    // @stub
}

// ..\stlport\stl_iterator.h:329
DC_ONLY(0xf7144, 0xC)
NewmapCell::TObjectCell* std::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>::base()
{
    // @stub
}

// ..\stlport\stl_iterator.h:330
DC_ONLY(0xf7150, 0xE)
NewmapCell::TObjectCell* std::reverse_iterator<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,NewmapCell::TObjectCell &,NewmapCell::TObjectCell *,int>::operator*()
{
    // @stub
}

// ..\stlport\stl_bitset.h:120
DC_ONLY(0xf7160, 0x24)
unsigned long std::_Base_bitset<70,unsigned long>::_S_maskbit(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf7184, 0x1C)
void std::_STL_alloc_proxy<SecondarySkillData *,SecondarySkillData,std::allocator<SecondarySkillData> >::~_STL_alloc_proxy<SecondarySkillData *,SecondarySkillData,std::allocator<SecondarySkillData> >()
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf71a0, 0x1C)
void std::_STL_alloc_proxy<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::~_STL_alloc_proxy<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >()
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf71bc, 0x1C)
void std::_STL_alloc_proxy<CObjectType *,CObjectType,std::allocator<CObjectType> >::~_STL_alloc_proxy<CObjectType *,CObjectType,std::allocator<CObjectType> >()
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf71d8, 0x1C)
void std::_STL_alloc_proxy<CObject *,CObject,std::allocator<CObject> >::~_STL_alloc_proxy<CObject *,CObject,std::allocator<CObject> >()
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf71f4, 0x1C)
void std::_STL_alloc_proxy<CSprite * *,CSprite *,std::allocator<CSprite *> >::~_STL_alloc_proxy<CSprite * *,CSprite *,std::allocator<CSprite *> >()
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf7210, 0x1C)
void std::_STL_alloc_proxy<TreasureData *,TreasureData,std::allocator<TreasureData> >::~_STL_alloc_proxy<TreasureData *,TreasureData,std::allocator<TreasureData> >()
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf722c, 0x1C)
void std::_STL_alloc_proxy<MonsterData *,MonsterData,std::allocator<MonsterData> >::~_STL_alloc_proxy<MonsterData *,MonsterData,std::allocator<MonsterData> >()
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf7248, 0x1C)
void std::_STL_alloc_proxy<BlackBoxData *,BlackBoxData,std::allocator<BlackBoxData> >::~_STL_alloc_proxy<BlackBoxData *,BlackBoxData,std::allocator<BlackBoxData> >()
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf7264, 0x1C)
void std::_STL_alloc_proxy<TSeerHut *,TSeerHut,std::allocator<TSeerHut> >::~_STL_alloc_proxy<TSeerHut *,TSeerHut,std::allocator<TSeerHut> >()
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf7280, 0x1C)
void std::_STL_alloc_proxy<TTimedEvent *,TTimedEvent,std::allocator<TTimedEvent> >::~_STL_alloc_proxy<TTimedEvent *,TTimedEvent,std::allocator<TTimedEvent> >()
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0xf729c, 0x1C)
void std::_STL_alloc_proxy<TTownEvent *,TTownEvent,std::allocator<TTownEvent> >::~_STL_alloc_proxy<TTownEvent *,TTownEvent,std::allocator<TTownEvent> >()
{
    // @stub
}

// ..\stlport\stl_bitset.h:125
DC_ONLY(0xf72b8, 0x2C)
unsigned long std::_Base_bitset<2,unsigned long>::_M_getword(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf72e4, 0x28)
void std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::insert(SecondarySkillData* __pos, unsigned __n, const SecondarySkillData* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf730c, 0x1E)
void std::_STL_alloc_proxy<SecondarySkillData *,SecondarySkillData,std::allocator<SecondarySkillData> >::_STL_alloc_proxy<SecondarySkillData *,SecondarySkillData,std::allocator<SecondarySkillData> >(const std::allocator<SecondarySkillData>* __a, SecondarySkillData** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0xf732c, 0x30)
void std::_STL_alloc_proxy<SecondarySkillData *,SecondarySkillData,std::allocator<SecondarySkillData> >::deallocate(SecondarySkillData* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf735c, 0x28)
void std::vector<enum SpellID,std::allocator<enum SpellID> >::insert(SpellID* __pos, unsigned __n, const SpellID* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf7384, 0x28)
void std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::insert(NewmapCell::TObjectCell* __pos, unsigned __n, const NewmapCell::TObjectCell* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0xf73ac, 0x48)
NewmapCell::TObjectCell* std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::erase(NewmapCell::TObjectCell* __first, NewmapCell::TObjectCell* __last)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf73f4, 0x1E)
void std::_STL_alloc_proxy<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::_STL_alloc_proxy<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >(const std::allocator<NewmapCell::TObjectCell>* __a, NewmapCell::TObjectCell** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0xf7414, 0x30)
void std::_STL_alloc_proxy<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::deallocate(NewmapCell::TObjectCell* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf7444, 0x28)
void std::vector<CObjectType,std::allocator<CObjectType> >::insert(CObjectType* __pos, unsigned __n, const CObjectType* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf746c, 0x1E)
void std::_STL_alloc_proxy<CObjectType *,CObjectType,std::allocator<CObjectType> >::_STL_alloc_proxy<CObjectType *,CObjectType,std::allocator<CObjectType> >(const std::allocator<CObjectType>* __a, CObjectType** __p)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf748c, 0x28)
void std::vector<CObject,std::allocator<CObject> >::insert(CObject* __pos, unsigned __n, const CObject* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf74b4, 0x1E)
void std::_STL_alloc_proxy<CObject *,CObject,std::allocator<CObject> >::_STL_alloc_proxy<CObject *,CObject,std::allocator<CObject> >(const std::allocator<CObject>* __a, CObject** __p)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf74d4, 0x28)
void std::vector<CSprite *,std::allocator<CSprite *> >::insert(CSprite** __pos, unsigned __n, CSprite** __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf74fc, 0x1E)
void std::_STL_alloc_proxy<CSprite * *,CSprite *,std::allocator<CSprite *> >::_STL_alloc_proxy<CSprite * *,CSprite *,std::allocator<CSprite *> >(const std::allocator<CSprite* __a, CSprite*** __p)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf751c, 0x28)
void std::vector<TreasureData,std::allocator<TreasureData> >::insert(TreasureData* __pos, unsigned __n, const TreasureData* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf7544, 0x1E)
void std::_STL_alloc_proxy<TreasureData *,TreasureData,std::allocator<TreasureData> >::_STL_alloc_proxy<TreasureData *,TreasureData,std::allocator<TreasureData> >(const std::allocator<TreasureData>* __a, TreasureData** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0xf7564, 0x30)
void std::_STL_alloc_proxy<TreasureData *,TreasureData,std::allocator<TreasureData> >::deallocate(TreasureData* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf7594, 0x28)
void std::vector<MonsterData,std::allocator<MonsterData> >::insert(MonsterData* __pos, unsigned __n, const MonsterData* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf75bc, 0x1E)
void std::_STL_alloc_proxy<MonsterData *,MonsterData,std::allocator<MonsterData> >::_STL_alloc_proxy<MonsterData *,MonsterData,std::allocator<MonsterData> >(const std::allocator<MonsterData>* __a, MonsterData** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0xf75dc, 0x30)
void std::_STL_alloc_proxy<MonsterData *,MonsterData,std::allocator<MonsterData> >::deallocate(MonsterData* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf760c, 0x28)
void std::vector<BlackBoxData,std::allocator<BlackBoxData> >::insert(BlackBoxData* __pos, unsigned __n, const BlackBoxData* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf7634, 0x1E)
void std::_STL_alloc_proxy<BlackBoxData *,BlackBoxData,std::allocator<BlackBoxData> >::_STL_alloc_proxy<BlackBoxData *,BlackBoxData,std::allocator<BlackBoxData> >(const std::allocator<BlackBoxData>* __a, BlackBoxData** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0xf7654, 0x30)
void std::_STL_alloc_proxy<BlackBoxData *,BlackBoxData,std::allocator<BlackBoxData> >::deallocate(BlackBoxData* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf7684, 0x1E)
void std::_STL_alloc_proxy<TSeerHut *,TSeerHut,std::allocator<TSeerHut> >::_STL_alloc_proxy<TSeerHut *,TSeerHut,std::allocator<TSeerHut> >(const std::allocator<TSeerHut>* __a, TSeerHut** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0xf76a4, 0x30)
void std::_STL_alloc_proxy<TSeerHut *,TSeerHut,std::allocator<TSeerHut> >::deallocate(TSeerHut* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf76d4, 0x28)
void std::vector<TTimedEvent,std::allocator<TTimedEvent> >::insert(TTimedEvent* __pos, unsigned __n, const TTimedEvent* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf76fc, 0x1E)
void std::_STL_alloc_proxy<TTimedEvent *,TTimedEvent,std::allocator<TTimedEvent> >::_STL_alloc_proxy<TTimedEvent *,TTimedEvent,std::allocator<TTimedEvent> >(const std::allocator<TTimedEvent>* __a, TTimedEvent** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0xf771c, 0x30)
void std::_STL_alloc_proxy<TTimedEvent *,TTimedEvent,std::allocator<TTimedEvent> >::deallocate(TTimedEvent* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0xf774c, 0x28)
void std::vector<TTownEvent,std::allocator<TTownEvent> >::insert(TTownEvent* __pos, unsigned __n, const TTownEvent* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0xf7774, 0x1E)
void std::_STL_alloc_proxy<TTownEvent *,TTownEvent,std::allocator<TTownEvent> >::_STL_alloc_proxy<TTownEvent *,TTownEvent,std::allocator<TTownEvent> >(const std::allocator<TTownEvent>* __a, TTownEvent** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0xf7794, 0x30)
void std::_STL_alloc_proxy<TTownEvent *,TTownEvent,std::allocator<TTownEvent> >::deallocate(TTownEvent* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_bitset.h:124
DC_ONLY(0xf77c4, 0x28)
unsigned long* std::_Base_bitset<3,unsigned long>::_M_getword(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_bitset.h:164
DC_ONLY(0xf77ec, 0x2E)
void std::_Base_bitset<3,unsigned long>::_M_do_reset()
{
    // @stub
}

// ..\stlport\stl_bitset.h:117
DC_ONLY(0xf781c, 0xE)
unsigned std::_Base_bitset<70,unsigned long>::_S_whichbit(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0xf782c, 0x2C)
void std::allocator<SecondarySkillData>::deallocate(SecondarySkillData* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0xf7858, 0x28)
void std::allocator<NewmapCell::TObjectCell>::deallocate(NewmapCell::TObjectCell* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0xf7880, 0x2C)
void std::allocator<TreasureData>::deallocate(TreasureData* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0xf78ac, 0x2C)
void std::allocator<MonsterData>::deallocate(MonsterData* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0xf78d8, 0x30)
void std::allocator<BlackBoxData>::deallocate(BlackBoxData* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0xf7908, 0x2C)
void std::allocator<TSeerHut>::deallocate(TSeerHut* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0xf7934, 0x2C)
void std::allocator<TTimedEvent>::deallocate(TTimedEvent* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0xf7960, 0x2C)
void std::allocator<TTownEvent>::deallocate(TTownEvent* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_bitset.h:111
DC_ONLY(0xf798c, 0xE)
unsigned std::_Base_bitset<3,unsigned long>::_S_whichword(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf799c, 0x150)
void std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::_M_fill_insert(SecondarySkillData* __position, unsigned __n, const SecondarySkillData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf7aec, 0x138)
void std::vector<enum SpellID,std::allocator<enum SpellID> >::_M_fill_insert(SpellID* __position, unsigned __n, const SpellID* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0xf7c24, 0x11C)
void std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::_M_insert_overflow(NewmapCell::TObjectCell* __position, const NewmapCell::TObjectCell* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf7d40, 0x154)
void std::vector<NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::_M_fill_insert(NewmapCell::TObjectCell* __position, unsigned __n, const NewmapCell::TObjectCell* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf7e94, 0x18C)
void std::vector<CObjectType,std::allocator<CObjectType> >::_M_fill_insert(CObjectType* __position, unsigned __n, const CObjectType* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf8020, 0x184)
void std::vector<CObject,std::allocator<CObject> >::_M_fill_insert(CObject* __position, unsigned __n, const CObject* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf81a4, 0x138)
void std::vector<CSprite *,std::allocator<CSprite *> >::_M_fill_insert(CSprite** __position, unsigned __n, CSprite** __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0xf82dc, 0x130)
void std::vector<TreasureData,std::allocator<TreasureData> >::_M_insert_overflow(TreasureData* __position, const TreasureData* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf840c, 0x18C)
void std::vector<TreasureData,std::allocator<TreasureData> >::_M_fill_insert(TreasureData* __position, unsigned __n, const TreasureData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0xf8598, 0x130)
void std::vector<MonsterData,std::allocator<MonsterData> >::_M_insert_overflow(MonsterData* __position, const MonsterData* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf86c8, 0x18C)
void std::vector<MonsterData,std::allocator<MonsterData> >::_M_fill_insert(MonsterData* __position, unsigned __n, const MonsterData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0xf8854, 0x134)
void std::vector<BlackBoxData,std::allocator<BlackBoxData> >::_M_insert_overflow(BlackBoxData* __position, const BlackBoxData* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf8988, 0x194)
void std::vector<BlackBoxData,std::allocator<BlackBoxData> >::_M_fill_insert(BlackBoxData* __position, unsigned __n, const BlackBoxData* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0xf8b1c, 0x130)
void std::vector<TSeerHut,std::allocator<TSeerHut> >::_M_insert_overflow(TSeerHut* __position, const TSeerHut* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf8c4c, 0x18C)
void std::vector<TTimedEvent,std::allocator<TTimedEvent> >::_M_fill_insert(TTimedEvent* __position, unsigned __n, const TTimedEvent* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0xf8dd8, 0x130)
void std::vector<TTownEvent,std::allocator<TTownEvent> >::_M_insert_overflow(TTownEvent* __position, const TTownEvent* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0xf8f08, 0x18C)
void std::vector<TTownEvent,std::allocator<TTownEvent> >::_M_fill_insert(TTownEvent* __position, unsigned __n, const TTownEvent* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0xf9094, 0x130)
void std::vector<TownExtra,std::allocator<TownExtra> >::_M_insert_overflow(TownExtra* __position, const TownExtra* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_iterator.h:428
DC_ONLY(0xf91c4, 0x2C)
unsigned char std::operator!=(const std::reverse_iterator<NewmapCell::TObjectCell* __x, const std::reverse_iterator<NewmapCell::TObjectCell* __y)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0xf91f0, 0x30)
void std::destroy(SecondarySkillData* __first, SecondarySkillData* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0xf9220, 0x30)
void std::destroy(NewmapCell::TObjectCell* __first, NewmapCell::TObjectCell* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0xf9250, 0x58)
void std::construct(NewmapCell::TObjectCell* __p, const NewmapCell::TObjectCell* __value)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0xf92a8, 0x58)
NewmapCell::TObjectCell* std::copy_backward(NewmapCell::TObjectCell* __first, NewmapCell::TObjectCell* __last, NewmapCell::TObjectCell* __result)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0xf9300, 0x30)
void std::destroy(TreasureData* __first, TreasureData* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0xf9330, 0x44)
void std::construct(TreasureData* __p, const TreasureData* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0xf9374, 0x30)
void std::destroy(MonsterData* __first, MonsterData* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0xf93a4, 0x44)
void std::construct(MonsterData* __p, const MonsterData* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0xf93e8, 0x30)
void std::destroy(BlackBoxData* __first, BlackBoxData* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0xf9418, 0x48)
void std::construct(BlackBoxData* __p, const BlackBoxData* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0xf9460, 0x30)
void std::destroy(TSeerHut* __first, TSeerHut* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0xf9490, 0x78)
void std::construct(TSeerHut* __p, const TSeerHut* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0xf9508, 0x30)
void std::destroy(TTimedEvent* __first, TTimedEvent* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0xf9538, 0x30)
void std::destroy(TTownEvent* __first, TTownEvent* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0xf9568, 0x44)
void std::construct(TTownEvent* __p, const TTownEvent* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0xf95ac, 0x144)
void std::construct(TownExtra* __p, const TownExtra* __value)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf96f0, 0x58)
SecondarySkillData* std::copy(SecondarySkillData* __first, SecondarySkillData* __last, SecondarySkillData* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf9748, 0x58)
SpellID* std::copy(SpellID* __first, SpellID* __last, SpellID* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf97a0, 0x58)
CObjectType* std::copy(CObjectType* __first, CObjectType* __last, CObjectType* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf97f8, 0x58)
CObject* std::copy(CObject* __first, CObject* __last, CObject* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf9850, 0x58)
CSprite** std::copy(CSprite** __first, CSprite** __last, CSprite** __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf98a8, 0x58)
TreasureData* std::copy(TreasureData* __first, TreasureData* __last, TreasureData* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf9900, 0x58)
MonsterData* std::copy(MonsterData* __first, MonsterData* __last, MonsterData* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf9958, 0x58)
BlackBoxData* std::copy(BlackBoxData* __first, BlackBoxData* __last, BlackBoxData* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf99b0, 0x58)
TSeerHut* std::copy(TSeerHut* __first, TSeerHut* __last, TSeerHut* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf9a08, 0x58)
TTimedEvent* std::copy(TTimedEvent* __first, TTimedEvent* __last, TTimedEvent* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf9a60, 0x58)
TTownEvent* std::copy(TTownEvent* __first, TTownEvent* __last, TTownEvent* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xf9ab8, 0x58)
TownExtra* std::copy(TownExtra* __first, TownExtra* __last, TownExtra* __result)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0xf9b10, 0xA)
std::allocator<SecondarySkillData>* std::__stl_alloc_rebind(std::allocator<SecondarySkillData>* __a, const SecondarySkillData* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0xf9b1c, 0xA)
std::allocator<NewmapCell::TObjectCell>* std::__stl_alloc_rebind(std::allocator<NewmapCell::TObjectCell>* __a, const NewmapCell::TObjectCell* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0xf9b28, 0xA)
std::allocator<TreasureData>* std::__stl_alloc_rebind(std::allocator<TreasureData>* __a, const TreasureData* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0xf9b34, 0xA)
std::allocator<MonsterData>* std::__stl_alloc_rebind(std::allocator<MonsterData>* __a, const MonsterData* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0xf9b40, 0xA)
std::allocator<BlackBoxData>* std::__stl_alloc_rebind(std::allocator<BlackBoxData>* __a, const BlackBoxData* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0xf9b4c, 0xA)
std::allocator<TSeerHut>* std::__stl_alloc_rebind(std::allocator<TSeerHut>* __a, const TSeerHut* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0xf9b58, 0xA)
std::allocator<TTimedEvent>* std::__stl_alloc_rebind(std::allocator<TTimedEvent>* __a, const TTimedEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0xf9b64, 0xA)
std::allocator<TTownEvent>* std::__stl_alloc_rebind(std::allocator<TTownEvent>* __a, const TTownEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xf9b70, 0xC4)
void TreasureData::TreasureData(const TreasureData* __that)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xf9c34, 0x48)
void MonsterData::MonsterData(const MonsterData* __that)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xf9c7c, 0x1A0)
void BlackBoxData::BlackBoxData(const BlackBoxData* __that)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xf9e1c, 0x78)
void TTimedEvent::TTimedEvent(const TTimedEvent* __that)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xf9e94, 0x108)
void TTownEvent::TTownEvent(const TTownEvent* __that)
{
    // @stub
}

// ..\stlport\stl_vector.h:236
DC_ONLY(0xf9f9c, 0x80)
void std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::vector<SecondarySkillData,std::allocator<SecondarySkillData> >(const std::vector<SecondarySkillData,std::allocator<SecondarySkillData>* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:236
DC_ONLY(0xfa01c, 0x80)
void std::vector<enum SpellID,std::allocator<enum SpellID> >::vector<enum SpellID,std::allocator<enum SpellID> >(const std::vector<enum* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0xfa09c, 0x38)
NewmapCell::TObjectCell* std::_STL_alloc_proxy<NewmapCell::TObjectCell *,NewmapCell::TObjectCell,std::allocator<NewmapCell::TObjectCell> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0xfa0d4, 0x38)
TreasureData* std::_STL_alloc_proxy<TreasureData *,TreasureData,std::allocator<TreasureData> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0xfa10c, 0x38)
MonsterData* std::_STL_alloc_proxy<MonsterData *,MonsterData,std::allocator<MonsterData> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0xfa144, 0x38)
BlackBoxData* std::_STL_alloc_proxy<BlackBoxData *,BlackBoxData,std::allocator<BlackBoxData> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0xfa17c, 0x38)
TSeerHut* std::_STL_alloc_proxy<TSeerHut *,TSeerHut,std::allocator<TSeerHut> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0xfa1b4, 0x38)
TTownEvent* std::_STL_alloc_proxy<TTownEvent *,TTownEvent,std::allocator<TTownEvent> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0xfa1ec, 0x38)
TownExtra* std::_STL_alloc_proxy<TownExtra *,TownExtra,std::allocator<TownExtra> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:153
DC_ONLY(0xfa224, 0x14)
std::allocator<SecondarySkillData> std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::get_allocator(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_vector.h:94
DC_ONLY(0xfa238, 0x78)
void std::_Vector_base<SecondarySkillData,std::allocator<SecondarySkillData> >::_Vector_base<SecondarySkillData,std::allocator<SecondarySkillData> >(unsigned __n, const std::allocator<SecondarySkillData>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:153
DC_ONLY(0xfa2b0, 0x14)
std::allocator<enum std::vector<enum SpellID,std::allocator<enum SpellID> >::get_allocator(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_vector.h:94
DC_ONLY(0xfa2c4, 0x74)
void std::_Vector_base<enum SpellID,std::allocator<enum SpellID> >::_Vector_base<enum SpellID,std::allocator<enum SpellID> >(unsigned __n, const std::allocator<enum* __a)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0xfa338, 0x38)
NewmapCell::TObjectCell* std::allocator<NewmapCell::TObjectCell>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0xfa370, 0x3C)
TreasureData* std::allocator<TreasureData>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0xfa3ac, 0x3C)
MonsterData* std::allocator<MonsterData>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0xfa3e8, 0x40)
BlackBoxData* std::allocator<BlackBoxData>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0xfa428, 0x3C)
TSeerHut* std::allocator<TSeerHut>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0xfa464, 0x3C)
TTownEvent* std::allocator<TTownEvent>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0xfa4a0, 0x3C)
TownExtra* std::allocator<TownExtra>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0xfa4dc, 0x38)
SecondarySkillData* std::_STL_alloc_proxy<SecondarySkillData *,SecondarySkillData,std::allocator<SecondarySkillData> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0xfa514, 0x3C)
SecondarySkillData* std::allocator<SecondarySkillData>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0xfa550, 0x120)
void std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::_M_insert_overflow(SecondarySkillData* __position, const SecondarySkillData* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfa670, 0x3C)
SecondarySkillData* std::uninitialized_copy(SecondarySkillData* __first, SecondarySkillData* __last, SecondarySkillData* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0xfa6ac, 0x58)
SecondarySkillData* std::copy_backward(SecondarySkillData* __first, SecondarySkillData* __last, SecondarySkillData* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfa704, 0x3A)
void std::fill(SecondarySkillData* __first, SecondarySkillData* __last, const SecondarySkillData* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0xfa740, 0x3C)
SecondarySkillData* std::uninitialized_fill_n(SecondarySkillData* __first, unsigned __n, const SecondarySkillData* __x)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfa77c, 0x28)
void std::fill(SpellID* __first, SpellID* __last, const SpellID* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfa7a4, 0x3C)
NewmapCell::TObjectCell* std::uninitialized_copy(NewmapCell::TObjectCell* __first, NewmapCell::TObjectCell* __last, NewmapCell::TObjectCell* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0xfa7e0, 0x3C)
NewmapCell::TObjectCell* std::uninitialized_fill_n(NewmapCell::TObjectCell* __first, unsigned __n, const NewmapCell::TObjectCell* __x)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfa81c, 0x44)
void std::fill(NewmapCell::TObjectCell* __first, NewmapCell::TObjectCell* __last, const NewmapCell::TObjectCell* __value)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0xfa860, 0x58)
CObjectType* std::copy_backward(CObjectType* __first, CObjectType* __last, CObjectType* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfa8b8, 0x38)
void std::fill(CObjectType* __first, CObjectType* __last, const CObjectType* __value)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0xfa8f0, 0x58)
CObject* std::copy_backward(CObject* __first, CObject* __last, CObject* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfa948, 0x46)
void std::fill(CObject* __first, CObject* __last, const CObject* __value)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0xfa990, 0x58)
CSprite** std::copy_backward(CSprite** __first, CSprite** __last, CSprite** __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfa9e8, 0x28)
void std::fill(CSprite** __first, CSprite** __last, CSprite** __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfaa10, 0x3C)
TreasureData* std::uninitialized_copy(TreasureData* __first, TreasureData* __last, TreasureData* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0xfaa4c, 0x3C)
TreasureData* std::uninitialized_fill_n(TreasureData* __first, unsigned __n, const TreasureData* __x)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0xfaa88, 0x58)
TreasureData* std::copy_backward(TreasureData* __first, TreasureData* __last, TreasureData* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfaae0, 0x38)
void std::fill(TreasureData* __first, TreasureData* __last, const TreasureData* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfab18, 0x3C)
MonsterData* std::uninitialized_copy(MonsterData* __first, MonsterData* __last, MonsterData* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0xfab54, 0x3C)
MonsterData* std::uninitialized_fill_n(MonsterData* __first, unsigned __n, const MonsterData* __x)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0xfab90, 0x58)
MonsterData* std::copy_backward(MonsterData* __first, MonsterData* __last, MonsterData* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfabe8, 0x38)
void std::fill(MonsterData* __first, MonsterData* __last, const MonsterData* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfac20, 0x3C)
BlackBoxData* std::uninitialized_copy(BlackBoxData* __first, BlackBoxData* __last, BlackBoxData* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0xfac5c, 0x3C)
BlackBoxData* std::uninitialized_fill_n(BlackBoxData* __first, unsigned __n, const BlackBoxData* __x)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0xfac98, 0x58)
BlackBoxData* std::copy_backward(BlackBoxData* __first, BlackBoxData* __last, BlackBoxData* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfacf0, 0x3C)
void std::fill(BlackBoxData* __first, BlackBoxData* __last, const BlackBoxData* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfad2c, 0x3C)
TSeerHut* std::uninitialized_copy(TSeerHut* __first, TSeerHut* __last, TSeerHut* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0xfad68, 0x3C)
TSeerHut* std::uninitialized_fill_n(TSeerHut* __first, unsigned __n, const TSeerHut* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0xfada4, 0x130)
void std::vector<TTimedEvent,std::allocator<TTimedEvent> >::_M_insert_overflow(TTimedEvent* __position, const TTimedEvent* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfaed4, 0x3C)
TTimedEvent* std::uninitialized_copy(TTimedEvent* __first, TTimedEvent* __last, TTimedEvent* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0xfaf10, 0x58)
TTimedEvent* std::copy_backward(TTimedEvent* __first, TTimedEvent* __last, TTimedEvent* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfaf68, 0x38)
void std::fill(TTimedEvent* __first, TTimedEvent* __last, const TTimedEvent* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0xfafa0, 0x3C)
TTimedEvent* std::uninitialized_fill_n(TTimedEvent* __first, unsigned __n, const TTimedEvent* __x)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfafdc, 0x3C)
TTownEvent* std::uninitialized_copy(TTownEvent* __first, TTownEvent* __last, TTownEvent* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0xfb018, 0x3C)
TTownEvent* std::uninitialized_fill_n(TTownEvent* __first, unsigned __n, const TTownEvent* __x)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0xfb054, 0x58)
TTownEvent* std::copy_backward(TTownEvent* __first, TTownEvent* __last, TTownEvent* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0xfb0ac, 0x38)
void std::fill(TTownEvent* __first, TTownEvent* __last, const TTownEvent* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfb0e4, 0x3C)
TownExtra* std::uninitialized_copy(TownExtra* __first, TownExtra* __last, TownExtra* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0xfb120, 0x3C)
TownExtra* std::uninitialized_fill_n(TownExtra* __first, unsigned __n, const TownExtra* __x)
{
    // @stub
}

// ..\stlport\stl_iterator.h:405
DC_ONLY(0xfb15c, 0x30)
unsigned char std::operator==(const std::reverse_iterator<NewmapCell::TObjectCell* __x, const std::reverse_iterator<NewmapCell::TObjectCell* __y)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0xfb18c, 0x8)
SecondarySkillData* std::value_type(const SecondarySkillData* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0xfb194, 0x24)
void std::__destroy(SecondarySkillData* __first, SecondarySkillData* __last, SecondarySkillData* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0xfb1b8, 0x8)
NewmapCell::TObjectCell* std::value_type(const NewmapCell::TObjectCell* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0xfb1c0, 0x24)
void std::__destroy(NewmapCell::TObjectCell* __first, NewmapCell::TObjectCell* __last, NewmapCell::TObjectCell* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0xfb1e4, 0x5E)
NewmapCell::TObjectCell* std::__copy_backward(NewmapCell::TObjectCell* __first, NewmapCell::TObjectCell* __last, NewmapCell::TObjectCell* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0xfb244, 0x8)
TreasureData* std::value_type(const TreasureData* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0xfb24c, 0x24)
void std::__destroy(TreasureData* __first, TreasureData* __last, TreasureData* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0xfb270, 0x8)
MonsterData* std::value_type(const MonsterData* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0xfb278, 0x24)
void std::__destroy(MonsterData* __first, MonsterData* __last, MonsterData* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0xfb29c, 0x8)
BlackBoxData* std::value_type(const BlackBoxData* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0xfb2a4, 0x24)
void std::__destroy(BlackBoxData* __first, BlackBoxData* __last, BlackBoxData* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0xfb2c8, 0x8)
TSeerHut* std::value_type(const TSeerHut* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0xfb2d0, 0x24)
void std::__destroy(TSeerHut* __first, TSeerHut* __last, TSeerHut* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0xfb2f4, 0x8)
TTimedEvent* std::value_type(const TTimedEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0xfb2fc, 0x24)
void std::__destroy(TTimedEvent* __first, TTimedEvent* __last, TTimedEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0xfb320, 0x8)
TTownEvent* std::value_type(const TTownEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0xfb328, 0x24)
void std::__destroy(TTownEvent* __first, TTownEvent* __last, TTownEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb34c, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const SecondarySkillData* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb360, 0x8)
int* std::distance_type(const SecondarySkillData* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb368, 0x58)
SecondarySkillData* std::__copy(SecondarySkillData* __first, SecondarySkillData* __last, SecondarySkillData* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb3c0, 0x40)
SpellID* std::__copy(SpellID* __first, SpellID* __last, SpellID* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb400, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const CObjectType* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb414, 0x8)
int* std::distance_type(const CObjectType* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb41c, 0x60)
CObjectType* std::__copy(CObjectType* __first, CObjectType* __last, CObjectType* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb47c, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const CObject* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb490, 0x8)
int* std::distance_type(const CObject* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb498, 0x74)
CObject* std::__copy(CObject* __first, CObject* __last, CObject* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb50c, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, CSprite** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb520, 0x8)
int* std::distance_type(CSprite** __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb528, 0x40)
CSprite** std::__copy(CSprite** __first, CSprite** __last, CSprite** __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb568, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const TreasureData* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb57c, 0x8)
int* std::distance_type(const TreasureData* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb584, 0x60)
TreasureData* std::__copy(TreasureData* __first, TreasureData* __last, TreasureData* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb5e4, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const MonsterData* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb5f8, 0x8)
int* std::distance_type(const MonsterData* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb600, 0x60)
MonsterData* std::__copy(MonsterData* __first, MonsterData* __last, MonsterData* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb660, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const BlackBoxData* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb674, 0x8)
int* std::distance_type(const BlackBoxData* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb67c, 0x68)
BlackBoxData* std::__copy(BlackBoxData* __first, BlackBoxData* __last, BlackBoxData* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb6e4, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const TSeerHut* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb6f8, 0x8)
int* std::distance_type(const TSeerHut* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb700, 0x9C)
TSeerHut* std::__copy(TSeerHut* __first, TSeerHut* __last, TSeerHut* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb79c, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const TTimedEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb7b0, 0x8)
int* std::distance_type(const TTimedEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb7b8, 0x60)
TTimedEvent* std::__copy(TTimedEvent* __first, TTimedEvent* __last, TTimedEvent* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb818, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const TTownEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb82c, 0x8)
int* std::distance_type(const TTownEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb834, 0x60)
TTownEvent* std::__copy(TTownEvent* __first, TTownEvent* __last, TTownEvent* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0xfb894, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const TownExtra* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0xfb8a8, 0x8)
int* std::distance_type(const TownExtra* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfb8b0, 0x16C)
TownExtra* std::__copy(TownExtra* __first, TownExtra* __last, TownExtra* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfba1c, 0x3C)
SecondarySkillData* std::uninitialized_copy(const SecondarySkillData* __first, const SecondarySkillData* __last, SecondarySkillData* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0xfba58, 0x3C)
SpellID* std::uninitialized_copy(const SpellID* __first, const SpellID* __last, SpellID* __result)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xfba94, 0xB8)
CObjectType* CObjectType::operator=(const CObjectType* __that)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xfbb4c, 0xCC)
TreasureData* TreasureData::operator=(const TreasureData* __that)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xfbc18, 0x64)
MonsterData* MonsterData::operator=(const MonsterData* __that)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xfbc7c, 0x1B0)
BlackBoxData* BlackBoxData::operator=(const BlackBoxData* __that)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xfbe2c, 0x94)
TTimedEvent* TTimedEvent::operator=(const TTimedEvent* __that)
{
    // @stub
}

// ..\stlport\stl_alloc.h:970
DC_ONLY(0xfbec0, 0x7C)
TTownEvent* TTownEvent::operator=(const TTownEvent* __that)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0xfbf3c, 0x38)
TTimedEvent* std::_STL_alloc_proxy<TTimedEvent *,TTimedEvent,std::allocator<TTimedEvent> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0xfbf74, 0x3C)
TTimedEvent* std::allocator<TTimedEvent>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_vector.c:207
DC_ONLY(0xfbfb0, 0x16C)
std::vector<SecondarySkillData,std::allocator<SecondarySkillData>* std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::operator=(const std::vector<SecondarySkillData,std::allocator<SecondarySkillData>* __x)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0xfc11c, 0x4C)
void std::construct(SecondarySkillData* __p, const SecondarySkillData* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfc168, 0x30)
SecondarySkillData* std::__uninitialized_copy(SecondarySkillData* __first, SecondarySkillData* __last, SecondarySkillData* __result, SecondarySkillData* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0xfc198, 0x52)
SecondarySkillData* std::__copy_backward(SecondarySkillData* __first, SecondarySkillData* __last, SecondarySkillData* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0xfc1ec, 0x30)
SecondarySkillData* std::__uninitialized_fill_n(SecondarySkillData* __first, unsigned __n, const SecondarySkillData* __x, SecondarySkillData* __formal)
{
    // @stub
}

// ..\stlport\stl_vector.c:207
DC_ONLY(0xfc21c, 0x164)
std::vector<enum* std::vector<enum SpellID,std::allocator<enum SpellID> >::operator=(const std::vector<enum* __x)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfc380, 0x30)
NewmapCell::TObjectCell* std::__uninitialized_copy(NewmapCell::TObjectCell* __first, NewmapCell::TObjectCell* __last, NewmapCell::TObjectCell* __result, NewmapCell::TObjectCell* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0xfc3b0, 0x30)
NewmapCell::TObjectCell* std::__uninitialized_fill_n(NewmapCell::TObjectCell* __first, unsigned __n, const NewmapCell::TObjectCell* __x, NewmapCell::TObjectCell* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0xfc3e0, 0x60)
CObjectType* std::__copy_backward(CObjectType* __first, CObjectType* __last, CObjectType* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0xfc440, 0x70)
CObject* std::__copy_backward(CObject* __first, CObject* __last, CObject* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0xfc4b0, 0x40)
CSprite** std::__copy_backward(CSprite** __first, CSprite** __last, CSprite** __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfc4f0, 0x30)
TreasureData* std::__uninitialized_copy(TreasureData* __first, TreasureData* __last, TreasureData* __result, TreasureData* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0xfc520, 0x30)
TreasureData* std::__uninitialized_fill_n(TreasureData* __first, unsigned __n, const TreasureData* __x, TreasureData* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0xfc550, 0x60)
TreasureData* std::__copy_backward(TreasureData* __first, TreasureData* __last, TreasureData* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfc5b0, 0x30)
MonsterData* std::__uninitialized_copy(MonsterData* __first, MonsterData* __last, MonsterData* __result, MonsterData* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0xfc5e0, 0x30)
MonsterData* std::__uninitialized_fill_n(MonsterData* __first, unsigned __n, const MonsterData* __x, MonsterData* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0xfc610, 0x60)
MonsterData* std::__copy_backward(MonsterData* __first, MonsterData* __last, MonsterData* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfc670, 0x30)
BlackBoxData* std::__uninitialized_copy(BlackBoxData* __first, BlackBoxData* __last, BlackBoxData* __result, BlackBoxData* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0xfc6a0, 0x30)
BlackBoxData* std::__uninitialized_fill_n(BlackBoxData* __first, unsigned __n, const BlackBoxData* __x, BlackBoxData* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0xfc6d0, 0x68)
BlackBoxData* std::__copy_backward(BlackBoxData* __first, BlackBoxData* __last, BlackBoxData* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfc738, 0x30)
TSeerHut* std::__uninitialized_copy(TSeerHut* __first, TSeerHut* __last, TSeerHut* __result, TSeerHut* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0xfc768, 0x30)
TSeerHut* std::__uninitialized_fill_n(TSeerHut* __first, unsigned __n, const TSeerHut* __x, TSeerHut* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0xfc798, 0x44)
void std::construct(TTimedEvent* __p, const TTimedEvent* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfc7dc, 0x30)
TTimedEvent* std::__uninitialized_copy(TTimedEvent* __first, TTimedEvent* __last, TTimedEvent* __result, TTimedEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0xfc80c, 0x60)
TTimedEvent* std::__copy_backward(TTimedEvent* __first, TTimedEvent* __last, TTimedEvent* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0xfc86c, 0x30)
TTimedEvent* std::__uninitialized_fill_n(TTimedEvent* __first, unsigned __n, const TTimedEvent* __x, TTimedEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfc89c, 0x30)
TTownEvent* std::__uninitialized_copy(TTownEvent* __first, TTownEvent* __last, TTownEvent* __result, TTownEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0xfc8cc, 0x30)
TTownEvent* std::__uninitialized_fill_n(TTownEvent* __first, unsigned __n, const TTownEvent* __x, TTownEvent* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0xfc8fc, 0x60)
TTownEvent* std::__copy_backward(TTownEvent* __first, TTownEvent* __last, TTownEvent* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfc95c, 0x30)
TownExtra* std::__uninitialized_copy(TownExtra* __first, TownExtra* __last, TownExtra* __result, TownExtra* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0xfc98c, 0x30)
TownExtra* std::__uninitialized_fill_n(TownExtra* __first, unsigned __n, const TownExtra* __x, TownExtra* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0xfc9bc, 0x34)
void std::__destroy_aux(SecondarySkillData* __first, SecondarySkillData* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0xfc9f0, 0x34)
void std::__destroy_aux(NewmapCell::TObjectCell* __first, NewmapCell::TObjectCell* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0xfca24, 0x34)
void std::__destroy_aux(TreasureData* __first, TreasureData* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0xfca58, 0x34)
void std::__destroy_aux(MonsterData* __first, MonsterData* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0xfca8c, 0x38)
void std::__destroy_aux(BlackBoxData* __first, BlackBoxData* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0xfcac4, 0x34)
void std::__destroy_aux(TSeerHut* __first, TSeerHut* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0xfcaf8, 0x34)
void std::__destroy_aux(TTimedEvent* __first, TTimedEvent* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0xfcb2c, 0x34)
void std::__destroy_aux(TTownEvent* __first, TTownEvent* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfcb60, 0x30)
SecondarySkillData* std::__uninitialized_copy(const SecondarySkillData* __first, const SecondarySkillData* __last, SecondarySkillData* __result, SecondarySkillData* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0xfcb90, 0x30)
SpellID* std::__uninitialized_copy(const SpellID* __first, const SpellID* __last, SpellID* __result, SpellID* __formal)
{
    // @stub
}

// ..\stlport\stl_vector.h:199
DC_ONLY(0xfcbc0, 0x16)
unsigned std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::capacity()
{
    // @stub
}

// ..\stlport\stl_vector.h:199
DC_ONLY(0xfcbd8, 0x16)
unsigned std::vector<enum SpellID,std::allocator<enum SpellID> >::capacity()
{
    // @stub
}

// ..\stlport\stl_vector.h:514
DC_ONLY(0xfcbf0, 0x40)
std::vector<SecondarySkillData,std::allocator<SecondarySkillData> >::_M_allocate_and_copy(unsigned __n, const SecondarySkillData* __first, const SecondarySkillData* __last)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xfcc30, 0x58)
SecondarySkillData* std::copy(const SecondarySkillData* __first, const SecondarySkillData* __last, SecondarySkillData* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfcc88, 0x44)
SecondarySkillData* std::__uninitialized_copy_aux(SecondarySkillData* __first, SecondarySkillData* __last, SecondarySkillData* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0xfcccc, 0x44)
SecondarySkillData* std::__uninitialized_fill_n_aux(SecondarySkillData* __first, unsigned __n, const SecondarySkillData* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_vector.h:514
DC_ONLY(0xfcd10, 0x40)
std::vector<enum SpellID,std::allocator<enum SpellID> >::_M_allocate_and_copy(unsigned __n, const SpellID* __first, const SpellID* __last)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0xfcd50, 0x58)
SpellID* std::copy(const SpellID* __first, const SpellID* __last, SpellID* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfcda8, 0x44)
NewmapCell::TObjectCell* std::__uninitialized_copy_aux(NewmapCell::TObjectCell* __first, NewmapCell::TObjectCell* __last, NewmapCell::TObjectCell* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0xfcdec, 0x44)
NewmapCell::TObjectCell* std::__uninitialized_fill_n_aux(NewmapCell::TObjectCell* __first, unsigned __n, const NewmapCell::TObjectCell* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfce30, 0x44)
TreasureData* std::__uninitialized_copy_aux(TreasureData* __first, TreasureData* __last, TreasureData* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0xfce74, 0x44)
TreasureData* std::__uninitialized_fill_n_aux(TreasureData* __first, unsigned __n, const TreasureData* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfceb8, 0x44)
MonsterData* std::__uninitialized_copy_aux(MonsterData* __first, MonsterData* __last, MonsterData* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0xfcefc, 0x44)
MonsterData* std::__uninitialized_fill_n_aux(MonsterData* __first, unsigned __n, const MonsterData* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfcf40, 0x4C)
BlackBoxData* std::__uninitialized_copy_aux(BlackBoxData* __first, BlackBoxData* __last, BlackBoxData* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0xfcf8c, 0x48)
BlackBoxData* std::__uninitialized_fill_n_aux(BlackBoxData* __first, unsigned __n, const BlackBoxData* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfcfd4, 0x44)
TSeerHut* std::__uninitialized_copy_aux(TSeerHut* __first, TSeerHut* __last, TSeerHut* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0xfd018, 0x44)
TSeerHut* std::__uninitialized_fill_n_aux(TSeerHut* __first, unsigned __n, const TSeerHut* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfd05c, 0x44)
TTimedEvent* std::__uninitialized_copy_aux(TTimedEvent* __first, TTimedEvent* __last, TTimedEvent* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0xfd0a0, 0x44)
TTimedEvent* std::__uninitialized_fill_n_aux(TTimedEvent* __first, unsigned __n, const TTimedEvent* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfd0e4, 0x44)
TTownEvent* std::__uninitialized_copy_aux(TTownEvent* __first, TTownEvent* __last, TTownEvent* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0xfd128, 0x44)
TTownEvent* std::__uninitialized_fill_n_aux(TTownEvent* __first, unsigned __n, const TTownEvent* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfd16c, 0x44)
TownExtra* std::__uninitialized_copy_aux(TownExtra* __first, TownExtra* __last, TownExtra* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0xfd1b0, 0x44)
TownExtra* std::__uninitialized_fill_n_aux(TownExtra* __first, unsigned __n, const TownExtra* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0xfd1f4, 0x20)
void std::destroy(SecondarySkillData* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0xfd214, 0x20)
void std::destroy(TreasureData* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0xfd234, 0x20)
void std::destroy(MonsterData* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0xfd254, 0x20)
void std::destroy(BlackBoxData* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0xfd274, 0x20)
void std::destroy(TSeerHut* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0xfd294, 0x20)
void std::destroy(TTimedEvent* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0xfd2b4, 0x20)
void std::destroy(TTownEvent* __pointer)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfd2d4, 0x44)
SecondarySkillData* std::__uninitialized_copy_aux(const SecondarySkillData* __first, const SecondarySkillData* __last, SecondarySkillData* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0xfd318, 0x44)
SpellID* std::__uninitialized_copy_aux(const SpellID* __first, const SpellID* __last, SpellID* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfd35c, 0x58)
SecondarySkillData* std::__copy(const SecondarySkillData* __first, const SecondarySkillData* __last, SecondarySkillData* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0xfd3b4, 0x40)
SpellID* std::__copy(const SpellID* __first, const SpellID* __last, SpellID* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0xfd3f4, 0x8)
void std::__destroy_aux(SecondarySkillData* __pointer, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0xfd3fc, 0x20)
void std::__destroy_aux(TreasureData* __pointer, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0xfd41c, 0x20)
void std::__destroy_aux(MonsterData* __pointer, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0xfd43c, 0x20)
void std::__destroy_aux(BlackBoxData* __pointer, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0xfd45c, 0x8)
void std::__destroy_aux(TSeerHut* __pointer, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0xfd464, 0x20)
void std::__destroy_aux(TTimedEvent* __pointer, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0xfd484, 0x20)
void std::__destroy_aux(TTownEvent* __pointer, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_string.h:609
DC_ONLY(0xfd4a4, 0x38)
void* TreasureData::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// ..\stlport\stl_string.h:609
DC_ONLY(0xfd4dc, 0x38)
void* MonsterData::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// ..\stlport\stl_string.h:609
DC_ONLY(0xfd514, 0x38)
void* BlackBoxData::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// ..\stlport\stl_string.h:609
DC_ONLY(0xfd54c, 0x38)
void* TTimedEvent::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// ..\stlport\stl_string.h:609
DC_ONLY(0xfd584, 0x38)
void* TTownEvent::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x00508cf0, 0x3B9, VECTOR_INSERT, TownExtra)

VA_COMPGEN(0x00507ad0, 0x2F9, VECTOR_INSERT, TQuestGuard)

// COMDAT pairing: bitset10::set, mnemonic agreement 1.000.
VA_COMPGEN(0x00506820, 0x60, BITSET_SET, bitset10)

// COMDAT pairing: vector<CMapObjectData*>::clear, agreement 1.000 at an
// exactly equal 61-byte extent. The 4-byte element stride of its copy loop
// is the pointer element, and CMapObjectData is this header's type, so no
// other compiland can be the owner.
VA_COMPGEN(0x0048b4a0, 0x3D, VECTOR_CLEAR, CMapObjectData)

// COMDAT pairing: bitset<48>::_Tidy, agreement 1.000 at an exactly equal
// 40-byte extent.
VA_COMPGEN(0x004e66c0, 0x28, BITSET_TIDY, Bitset48)
