
#include "text.h"
#include "va.h"
#include "objnames.h"

#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include "platform.h"

#include "mapcell.h"

#include "advmgr.h"
#include "advmgr_objects.h"
#include "csprite.h"
#include "game.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "newgame.h"
#include "resourcemanager.h"
#include "smackmgr.h"

VA(0x004fbf90, 0x61)  // dc 0xeb6a4
void ExtraInfoUnion::setCellVisited(short player)
{
    if (player < 0 || player >= 8)
        return;

    // DC mapcell.cpp:49 calls the canonical Game.h GetTeam member.
    int team = g_game->getTeam(player);

    for (int i = 0; i < 8; ++i) {
        if (g_game->m_mapHeader.m_teamInfo[i] == team)
            m_cellVisitedInfo.m_visited |= 1 << i;
    }
}

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
// DC locals prove int count, string throwAway, and char padding[16]. At
// mapcell.cpp:89/91 both readString results are stored in count's sp+0x14
// slot, as are the later gzread results. The read/guard pairs and failure
// scopes are separate at mapcell.cpp:93..120.
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
    count = NewSMapHeader::readString(infile, throwAway);
    count = NewSMapHeader::readString(infile, m_message);

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
    if (game::saveString(outfile, m_message) < 0)
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

// E:\gamedcs\mapcell.cpp:232, dc 0xebc90
int TTownEvent::read(TAbstractFile* infile, int mapVersion)
{
    unsigned char inBuf[6];
    char padding[4];

    TTimedEvent::read(infile, mapVersion);

    if (infile->read(inBuf, sizeof(inBuf)) < sizeof(inBuf))
        return -1;
    memcpy(&m_buildBuildings, inBuf, sizeof(inBuf));

    if (infile->read(m_generatorBonuses,
                     sizeof(m_generatorBonuses))
        < sizeof(m_generatorBonuses))
        return -1;

    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;
    return 0;
}

VA(0x004fc770, 0xFA)  // dc 0xebd24
int NewfullMap::saveTownEventList(TAbstractFile* outfile)
{
    int count = m_townEventList.size();
    if (static_cast<unsigned>(outfile->write(&count, sizeof(count)))
        < sizeof(count))
        return -1;

    for (unsigned int i = 0; i < m_townEventList.size(); ++i) {
        if (m_townEventList[i].save(outfile) < 0)
            return -1;
    }
    return 0;
}

// E:\gamedcs\mapcell.cpp:283, dc 0xebdbc.
// Retail saveTownEventList calls the base at 0x4fc80b and expands the
// derived writes at 0x4fc81a/0x4fc82c/0x4fc83e. CodeView calls the
// base at dc 0xebdca and deliberately ignores its result.
int TTownEvent::save(TAbstractFile* outfile)
{
    TTimedEvent::save(outfile);
    if (static_cast<unsigned>(outfile->write(&m_townNum, 1)) < 1)
        return -1;
    if (static_cast<unsigned>(outfile->write(&m_buildBuildings, 8)) < 8)
        return -1;
    if (static_cast<unsigned>(outfile->write(m_generatorBonuses, 14)) < 14)
        return -1;
    return 0;
}

VA(0x004fc870, 0x1E4)  // dc 0xebe3c
int NewfullMap::loadTownEventList(TAbstractFile* infile, int saveVersion)
{
    int count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_townEventList.resize(count);
    for (unsigned int i = 0; i < m_townEventList.size(); ++i) {
        if (m_townEventList[i].load(infile, saveVersion) < 0)
            return -1;
    }
    return 0;
}

// E:\gamedcs\mapcell.cpp:329, dc 0xebeec.
// Complete adds saveVersion for the canonical base loader. The list
// caller pushes it at 0x4fc9d7 before the base call at 0x4fc9e6,
// then expands the derived reads at 0x4fc9f5/0x4fca0b/0x4fca21.
// CodeView calls the base at dc 0xebefa and ignores its result.
int TTownEvent::load(TAbstractFile* infile, int saveVersion)
{
    TTimedEvent::load(infile, saveVersion);
    if (static_cast<unsigned>(infile->read(&m_townNum, 1)) < 1)
        return -1;
    if (static_cast<unsigned>(infile->read(&m_buildBuildings, 8)) < 8)
        return -1;
    if (static_cast<unsigned>(infile->read(m_generatorBonuses, 14)) < 14)
        return -1;
    return 0;
}

// E:\gamedcs\mapcell.cpp:354
// Dreamcast retains an out-of-line copy. Mac places the retained body between
// TTownEvent::load and getTriggerCell; both Mac callers are in mapcell.
// Retail's corresponding source-order slot is twelve bytes of NOP padding,
// while isDiggable and getSpecialTerrain expand this lookup. Removing inline
// leaves both callers exact but emits an unused VC6 COMDAT; the original
// keyword remains unproven.
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

VA(0x004fcbd0, 0x5C)  // dc 0xec098
TAdventureObjectType NewmapCell::getMapObject() const
{
    if (m_type == HERO) {
        const hero* currentHero = g_game->getHero(m_extraInfo);
        return currentHero->getObscuredObject();
    }
    if (m_type == BOAT) {
        const boat* currentBoat = g_game->getBoat(m_extraInfo);
        return currentBoat->getObscuredObject();
    }
    return m_type;
}

VA(0x004fcc30, 0x4D)  // dc 0xec114
unsigned long NewmapCell::getMapExtraInfo() const
{
    if (m_type == HERO)
        return g_game->getHero(m_extraInfo)->getObscuredExtraInfo();
    if (m_type == BOAT)
        return g_game->getBoat(m_extraInfo)->getObscuredExtraInfo();
    return m_extraInfo;
}

VA(0x004fcc80, 0x65)  // dc 0xec1c8
unsigned char NewmapCell::cellIsTrigger() const
{
    if (m_type == HERO) {
        hero* obscurer = g_game->getHero(m_extraInfo);
        return obscurer->getObscuredTrigger();
    }
    if (m_type == BOAT) {
        boat* obscurer = g_game->getBoat(m_extraInfo);
        return obscurer->getObscuredTrigger();
    }
    return m_isTrigger;
}

VA(0x004fccf0, 0xD0)  // dc 0xec254
unsigned char NewmapCell::isDiggable() const
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
const unsigned char NewmapCell::hasTriggerableEvent() const
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

VA(0x004fce20, 0x116)  // dc 0xec3b4
TAdventureObjectType NewmapCell::getSpecialTerrain() const
{
    if (m_type == HERO && (m_cellFlags & 0x1000)) {
        const hero* ourHero = g_game->getHero(m_extraInfo);
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

// Complete-only classifier; CodeView declares no corresponding member.
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

// Original: NewfullMap::~NewfullMap, mapcell.cpp:530, dc 0xec6a4.
// DC line 532 calls Close before member destruction. Complete additionally
// disposes the sprite resources here. Recovering the ordinary member call
// and Close's shared map-object cleanup makes this body and init both 100%,
// with no inline-depth control. A cell-only Close gives 97.66% here (init
// stays 100%): mapObjectData.clear still expands instead of remaining a call.
// The old pasted body with its clear pin was 95.8833; a whole-body free
// closeMap helper was not the original boundary and gave 87.7157.
VA(0x004fd1e0, 0x271)  // anchor-global, dc 0xec6a4
NewfullMap::~NewfullMap()
{
    close();

    unsigned int i;

    for (i = 0; i < m_sprites.size(); ++i)
        m_sprites[i]->dispose();
    m_sprites.clear();
}

// Original: NewfullMap::Close, mapcell.cpp:537, dc 0xec724.
// DC lines 539/541/542 prove the guard, array deletion and null store.
// Complete extends this with map-object deletion/clear: both destructor and
// init have that same sequence, and restoring it here matches both callers.
// Sprite disposal remains destructor-only; putting it here would change init.
void NewfullMap::close()
{
    if (m_cellData) {
        delete[] m_cellData;
        m_cellData = 0;
    }

    for (unsigned int i = 0; i < m_mapObjectData.size(); ++i)
        delete m_mapObjectData[i];
    m_mapObjectData.clear();
}

VA_COMPGEN(0x004fd460, 0x58, VECTOR_DELETING_DTOR, NewmapCell)

// CodeView dc 0xf4bdc: CV_fldattr_t.compgenx marks this destructor
// as implicit. Its retained retail body performs only base/member teardown.
VA_COMPGEN(0x004fd4c0, 0x26, IMPLICIT_DTOR, NewmapCell)

VA(0x004fd4f0, 0x160)  // dc 0xec80c
void NewfullMap::init(int size, unsigned char twoLayers)
{
    m_size = size;
    m_hasTwoLevels = twoLayers;

    close();


    int cellCount = (twoLayers != 0) + 1;
    cellCount *= m_size;
    cellCount *= m_size;
    m_cellData = new NewmapCell[cellCount];

    if (g_mapExtra)
        delete[] g_mapExtra;
    g_mapExtra = new unsigned short[cellCount];
    memset(g_mapExtra, 0, cellCount * sizeof(*g_mapExtra));
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

    placeObjects();

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
void NewfullMap::loadQuestGuardList(
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

// DC TSeerHut::LoadSeerList (0x12d854, seerhut.cpp:503..522) owns the
// count/read/resize/row-load operation. Complete moves the pool into this map,
// makes row load void, and registers quests in m_mapObjectData. A map-owned
// member is the inferred replacement interface; its original placement is
// unknown. DC uses one vector subscript per row. Keeping that named row
// reference makes VC6 expand this helper in load while retaining the nested
// TSeerHut constructor, as retail does.
int NewfullMap::loadSeerList(TAbstractFile* infile, int saveVersion)
{
    short seerCount;
    if (infile->read(&seerCount, sizeof(seerCount)) < sizeof(seerCount))
        return -1;

    m_seerHutList.resize(seerCount);
    int spriteNum;
    for (spriteNum = 0; spriteNum < m_seerHutList.size(); ++spriteNum) {
        TSeerHut& seerHut = m_seerHutList[spriteNum];
        seerHut.load(infile, saveVersion);
        if (seerHut.m_quest)
            m_mapObjectData.push_back(static_cast<CMapObjectData*>(
                static_cast<void*>(seerHut.m_quest)));
    }
    return 0;
}

// The Classic Mac save driver retains this map-owned helper at 0x11f5cc.
// Complete VC6 expands its call inside NewfullMap::save.
int NewfullMap::saveSeerList(TAbstractFile* outfile)
{
    short count = static_cast<short>(m_seerHutList.size());
    if (static_cast<unsigned>(outfile->write(&count, 2)) < 2)
        return -1;
    for (unsigned int i = 0; i < m_seerHutList.size(); ++i)
        m_seerHutList[i].save(outfile);
    return 0;
}

// Mac retains this sibling at 0x11f798; Complete VC6 expands it in save.
void NewfullMap::saveQuestGuardList(TAbstractFile* outfile)
{
    short count = static_cast<short>(m_questGuardList.size());
    outfile->write(&count, 2);
    for (unsigned int i = 0; i < m_questGuardList.size(); ++i)
        m_questGuardList[i].save(outfile);
}

// E:\gamedcs\mapcell.cpp:679, dc 0xecb94
// Dreamcast's int count is the reusable result of the layer/list loaders:
// dc 0xecbbc..0xecbbe and 0xecd3e..0xecd42 store their returns before
// the negative-result tests. It is distinct from Complete's signed-short
// seerCount read from the save stream; preserving both does not widen I/O.
VA(0x004fdbc0, 0x371)  // order-map: calls loadTimedEventList 0xfc500, loadTownEventList 0xfc870, Init 0xfd4f0, loadMapLayer 0xfe920 x2, loadBlackBoxList/loadMonsterList/loadMapObjects, dc 0xecb94
int NewfullMap::load(TAbstractFile* infile, int size, unsigned char twoLayers,
                     int saveVersion)
{
    int count;

    init(size, twoLayers);

    count = loadMapLayer(infile, size, 0, saveVersion);
    if (count < 0)
        return -1;
    if (twoLayers) {
        count = loadMapLayer(infile, size, 1, saveVersion);
        if (count < 0)
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

    count = loadMapObjects(infile);
    if (count < 0)
        return -1;
    count = loadBlackBoxList(infile, saveVersion);
    if (count < 0)
        return -1;
    count = loadTreasureList(infile);
    if (count < 0)
        return -1;
    count = loadMonsterList(infile);
    if (count < 0)
        return -1;

    count = loadSeerList(infile, saveVersion);
    if (count < 0)
        return -1;

    if (saveVersion >= 25)
        loadQuestGuardList(infile, saveVersion);

    count = loadTimedEventList(infile, saveVersion);
    if (count < 0)
        return -1;
    count = loadTownEventList(infile, saveVersion);
    if (count < 0)
        return -1;

    incProgressBar(1);
    return 0;
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

VA(0x004fdf40, 0x2D1)  // order-map: calls saveTimedEventList 0xfc390, saveTownEventList 0xfc770, saveMapLayer 0xfe490 x2, saveMapObjects 0x104a40, TQuestGuard::save, dc 0xecdf8
int NewfullMap::save(TAbstractFile* outfile, int size, unsigned char twoLayers)
{
    int count;
    count = saveMapLayer(outfile, size, 0);
    if (count < 0)
        return -1;
    if (twoLayers) {
        count = saveMapLayer(outfile, size, 1);
        if (count < 0)
            return -1;
    }
    count = saveMapObjects(outfile);
    if (count < 0)
        return -1;

    count = saveBlackBoxList(outfile);
    if (count < 0)
        return -1;
    count = saveTreasureList(outfile);
    if (count < 0)
        return -1;
    count = saveMonsterList(outfile);
    if (count < 0)
        return -1;

    count = saveSeerList(outfile);
    if (count < 0)
        return -1;
    saveQuestGuardList(outfile);

    count = saveTimedEventList(outfile);
    if (count < 0)
        return -1;
    count = saveTownEventList(outfile);
    if (count < 0)
        return -1;
    return 0;
}

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
    NewmapCell* thisCell = cell(0, 0, layer);

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

VA(0x004fe490, 0x22A)  // dc 0xed384
int NewfullMap::saveMapLayer(TAbstractFile* outfile, int size, int layer)
{
    NewmapCell* thisCell = cell(0, 0, layer);

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
    NewmapCell* thisCell = cell(0, 0, layer);

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

// DC proves the ordinary helper, boatType/x/y locals and call order.
// Retail readObject's BOAT arm expands this body and discards status.
int NewfullMap::readBoatData(TAbstractFile* infile, CObject* boatObject)
{
    signed char boatType = static_cast<signed char>(
        m_objectTypes[boatObject->m_typeIndex].m_extra);
    int x;
    int y;
    boatObject->findTrigger(x, y);
    boatObject->m_extraInfo = g_game->createBoat(
        x, y, boatObject->m_z, -1, 1, boatType);
    return 0;
}

VA(0x004fec10, 0x1D)  // dc 0xeda1c
CObjectType* CObject::getObjectTypePtr() const
{
    return &g_game->m_worldMap.m_objectTypes[m_typeIndex];
}

// E:\gamedcs\mapcell.cpp:1119. Dreamcast retains this source helper as an
// out-of-line SH4 body; Mac places it between getObjectTypePtr and findTrigger.
// Its two Mac callers are in mapcell. Complete expands the admitted uses;
// removing inline keeps getTriggerCell exact but emits an unused VC6 COMDAT,
// so the original keyword remains unproven.
inline type_point CObject::getTrigger() const
{
    int resultX;
    int resultY;
    findTrigger(resultX, resultY);
    return type_point(resultX, resultY, m_z);
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

            if (objType->m_triggerCells[
                    CObjectType::getBitPos(horiz, vert)]) {
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

// readObject discards the -1/0 status, so retail
// eliminates the final padding-read comparison from its inline expansion.
int NewfullMap::readHolyGrailData(TAbstractFile* infile, CObject* grailObject)
{
    char charBuffer;
    int count;
    count = infile->read(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;
    g_game->m_ultimateArtifactX = grailObject->m_x;
    g_game->m_ultimateArtifactY = grailObject->m_y;
    g_game->m_ultimateArtifactZ = grailObject->m_z;
    g_game->m_ultimateRadius = charBuffer;

    char padding[3];
    count = infile->read(padding, sizeof(padding));
    if (count < sizeof(padding))
        return -1;
    return 0;
}

// The discarded final status leaves only
// the second virtual read in readObject's retail expansion.
int NewfullMap::readShrineData(TAbstractFile* infile, CObject* shrineObject)
{
    char charBuffer;
    int count;
    count = infile->read(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;
    shrineObject->m_shrineInfo.m_spell = charBuffer;

    char padding[3];
    count = infile->read(padding, sizeof(padding));
    if (count < sizeof(padding))
        return -1;
    return 0;
}

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
    game::saveString(outfile, thisTreasure->m_message);

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
        if (loadTreasureData(infile, treasure) < 0)
            return -1;
    }
    return 0;
}

// E:\gamedcs\mapcell.cpp:1362, dc 0xee12c.
// CodeView proves this NewfullMap member. Complete expands the retained
// callers; emission does not turn the source member into a file-static helper.
// The record is a reference in the CodeView formal argument list.
int NewfullMap::loadTreasureData(TAbstractFile* infile, TreasureData& thisTreasure)
{
    game::loadString(infile, thisTreasure.m_message);

    unsigned char value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisTreasure.m_hasCustomGuardians = value != 0;
    if (thisTreasure.m_hasCustomGuardians)
        thisTreasure.m_guardians.load(infile);
    return 0;
}

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

// Mac retains these adjacent source helpers at 0:0x1217a0 and 0:0x12180c.
// The first is called by readBlackBox, readTownData and readHeroData; the
// second is called by loadBlackBox. Each reads a signed creature ID using the
// byte width of its older file format and the short width of later formats.
// Complete VC6 expands the calls in all four readers. The original names are
// unavailable in the older Dreamcast build.
static int readMapCreatureId(TAbstractFile* infile, int mapVersion)
{
    if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        signed char narrow;
        infile->read(&narrow, sizeof(narrow));
        return narrow;
    }
    short wide;
    infile->read(&wide, sizeof(wide));
    return wide;
}

static int readSavedCreatureId(TAbstractFile* infile, int saveVersion)
{
    if (saveVersion < 25) {
        signed char narrow;
        infile->read(&narrow, sizeof(narrow));
        return narrow;
    }
    short wide;
    infile->read(&wide, sizeof(wide));
    return wide;
}

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
                thisBox->m_artifacts[i] = TArtifact(narrow);
            } else {
                short wide;
                infile->read(&wide, sizeof(wide));
                thisBox->m_artifacts[i] = TArtifact(wide);
            }
        }
    }

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    count = value;
    if (count == 0) {
        // Dreamcast mapcell.cpp:1655 calls vector<SpellID>::clear here,
        // just as the two preceding empty-list arms do.
        thisBox->m_spells.clear();
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
        thisBox->m_creatures.m_armies[i] =
            readMapCreatureId(infile, mapVersion);

        short troops;
        if (infile->read(&troops, sizeof(troops)) < sizeof(troops))
            return -1;
        thisBox->m_creatures.m_numTroops[i] = troops;
    }

    char padding[8];
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

// CodeView dc 0xf4bfc: CV_fldattr_t.compgenx marks this destructor
// as implicit. Its retained retail body performs only base/member teardown.
VA_COMPGEN(0x004ffdf0, 0xB0, IMPLICIT_DTOR, BlackBoxData)

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
        if (loadTreasureData(infile, *thisBox) < 0)
            return -1;
    }

    {
        int intValue;
        if (infile->read(&intValue, sizeof(intValue)) < sizeof(intValue))
            return -1;
        thisBox->m_experienceBonus = intValue;
        if (infile->read(&intValue, sizeof(intValue)) < sizeof(intValue))
            return -1;
        thisBox->m_manaBonus = intValue;
    }

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisBox->m_moraleBonus = value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    thisBox->m_luckBonus = value;

    {
        int resourceValue;
        for (int resourceIndex = 0; resourceIndex < 7; ++resourceIndex) {
            if (infile->read(&resourceValue, sizeof(resourceValue))
                < sizeof(resourceValue))
                return -1;
            thisBox->m_resQty[resourceIndex] = resourceValue;
        }
    }
    {
        signed char skillValue;
        for (int skill = 0; skill < 4; ++skill) {
            if (infile->read(&skillValue, sizeof(skillValue)) < sizeof(skillValue))
                return -1;
            thisBox->m_primarySkillBonus[skill] = skillValue;
        }
    }

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
        thisBox->m_artifacts[i] = TArtifact(artifact & 0xff);
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
        thisBox->m_creatures.m_armies[i] =
            readSavedCreatureId(infile, saveVersion);
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

// Original: NewfullMap::readSeerData; mapcell.cpp:2124, dc 0xef7c8.
// Complete delegates quest/reward deserialization to TSeerHut::read instead of
// filling the DC fixed quest fields here. This wrapper still owns the temporary,
// pool insertion and map-object index, and now registers the polymorphic quest.
int NewfullMap::readSeerData(TAbstractFile* infile, CObject* seerObject)
{
    TSeerHut seerData;
    seerData.read(infile);
    m_seerHutList.push_back(seerData);
    seerObject->m_extraInfo = m_seerHutList.size() - 1;
    if (seerData.m_quest)
        m_mapObjectData.push_back(static_cast<CMapObjectData*>(
            static_cast<void*>(seerData.m_quest)));
    return 0;
}

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

    // Dreamcast mapcell.cpp:2333 calls ExtraInfoUnion::GetScholarAward.
    switch (scholarObject->getScholarAward()) {
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

// Complete defers the later DC
// trigger/terrain scan to loadShipyards; its readObject arm only initializes
// the two boat coordinates after the reads, then discards the status.
int NewfullMap::readShipyardData(TAbstractFile* infile, CObject* shipyardObject)
{
    char charBuffer;
    int count;
    count = infile->read(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;
    shipyardObject->m_shipyardInfo.m_owner = charBuffer;

    char padding[3];
    count = infile->read(padding, sizeof(padding));
    if (count < sizeof(padding))
        return -1;
    shipyardObject->m_shipyardInfo.m_boatX = 0xff;
    shipyardObject->m_shipyardInfo.m_boatY = 0xff;
    return 0;
}

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

    for (int z = 0; z < getNumLevels(); ++z) {
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
// Both Complete builds write MonsterInfo fields and clear bits 27..30
// after dontGrow. Mac code0+0x123e64..0x123e6c proves the latter store;
// the Windows mask 0x87fbffff combines it with the dontGrow assignment.
// Shared typed-field probes preserve Windows 97.3333% and its call structure.
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
#if defined(HOMM3_TARGET_MAC)
        // Mac map data is little endian; the retail PowerPC load uses lwbrx.
        identifier = __lwbrx(&rawIdentifier, 0);
#else
        identifier = rawIdentifier;
#endif
    }

    short quantity;
    if (infile->read(&quantity, sizeof(quantity)) < sizeof(quantity))
        return -1;
#if defined(HOMM3_TARGET_MAC)
    quantity = __lhbrx(&quantity, 0);
#endif
    monsterObject->m_monsterInfo.m_qty = quantity;

    // DC records unsigned char_buffer at line 2598, then the signed
    // disposition result separately across the switch arms at 2606..2630.
    unsigned char charBuffer;
    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;

    // DC and Mac leave disposition untouched for an out-of-range input.
    // The former Windows-only default assignment was a codegen workaround:
    // removing it raises Windows from 97.3333% to 98.4017%.
    char disposition;
    switch (static_cast<signed char>(charBuffer)) {
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
    monsterObject->m_monsterInfo.m_disposition = disposition;

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
#if defined(HOMM3_TARGET_MAC)
            quantityRead = __lwbrx(&quantityRead, 0);
#endif
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
#if defined(HOMM3_TARGET_MAC)
            artifact = static_cast<short>(__lhbrx(&wide, 0));
#else
            artifact = wide;
#endif
        }
        tempMonster.m_artifact = artifact;

        if (customIndex < 4000) {
            m_customMonsterList.push_back(tempMonster);
            monsterObject->m_monsterInfo.m_custom = 1;
            monsterObject->m_monsterInfo.m_index = customIndex;
        }
    }

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    monsterObject->m_monsterInfo.m_neverFlee = charBuffer & 1;

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    // The mask retail computes clears bits 27..30 alongside bit 18, so this
    // write lands on more than the one flag; transcribed as the object does
    // it rather than narrowed to the single bit.
    monsterObject->m_monsterInfo.m_dontGrow = charBuffer & 1;
    monsterObject->m_monsterInfo.m_unused27 = 0;

    char padding[2];
    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;

    type_point point(monsterObject->m_x, monsterObject->m_y,
                     monsterObject->m_z);
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

VA(0x00501790, 0x1E3)  // dc 0xf0788
int NewfullMap::loadMonsterList(TAbstractFile* infile)
{
    short count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_customMonsterList.resize(count);
    for (unsigned int i = 0; i < m_customMonsterList.size(); ++i) {
        if (loadMonsterData(infile, m_customMonsterList[i]) < 0)
            return -1;
    }
    return 0;
}

VA(0x00501980, 0x6B)  // dc 0xf0824
int NewfullMap::saveMonsterData(TAbstractFile* outfile, MonsterData* thisMonster)
{
    game::saveString(outfile, thisMonster->m_message);

    for (int i = 0; i < 7; ++i) {
        int value = thisMonster->m_resQty[i];
        if (static_cast<unsigned>(outfile->write(&value, 4)) < 4)
            return -1;
    }

    unsigned char artifact = static_cast<unsigned char>(thisMonster->m_artifact);
    return static_cast<unsigned>(outfile->write(&artifact, 1)) < 1 ? -1 : 0;
}

// E:\gamedcs\mapcell.cpp:2768, dc 0xf08b8.
// CodeView proves this NewfullMap member. Complete expands the retained
// callers; emission does not turn the source member into a file-static helper.
// The record is a reference in the CodeView formal argument list.
int NewfullMap::loadMonsterData(TAbstractFile* infile, MonsterData& thisMonster)
{
    game::loadString(infile, thisMonster.m_message);

    for (int i = 0; i < 7; ++i) {
        int value;
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            return -1;
        thisMonster.m_resQty[i] = value;
    }

    int artifact;
    infile->read(&artifact, sizeof(unsigned char));
    thisMonster.m_artifact = artifact & 0xff;
    if (thisMonster.m_artifact == (ARTIFACT_NONE & 0xff))
        thisMonster.m_artifact = ARTIFACT_NONE;
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
// DC's second spell-mask loop uses bitset::operator[] and reference assignment.
// Spelling that source form here lowered current Complete x86 79.82% to 79.21%
// and added an exception path absent from retail; keep the retail set call.
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
            tempTown.m_townArmy.m_armies[x] =
                readMapCreatureId(infile, mapVersion);

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
        for (int spell = 0; spell < 70; ++spell)
            tempTown.m_fixedSpells.set(
                spell, (spellBuf[spell / 8] & (1 << (spell % 8))) != 0);
    }

    if (infile->read(spellBuf, sizeof(spellBuf)) < sizeof(spellBuf))
        return -1;
    for (int spell = 0; spell < 70; ++spell)
        tempTown.m_spells.set(
            spell, (spellBuf[spell / 8] & (1 << (spell % 8))) != 0);

    if (infile->read(&numTownEvents, sizeof(numTownEvents))
        < sizeof(numTownEvents))
        return -1;

    for (count = numTownEvents; count > 0; --count) {
        TTownEvent thisEvent;
        thisEvent.read(infile, mapVersion);
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
                && g_game->m_gameVersion >= 1)
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

// The next structure pass reads the raw symbol nesting, not only the compact
// roster: all thirteen DC locals appear under S_GPROC32 before the first
// S_BLOCK32. Restoring one function-scope int_buffer, short_buffer and
// char_buffer (and reusing them across the reads) moves the current checkpoint
// to 82.9570%. MAX/history remains 92.7472%. HeroID stays the Complete x86
// representation `int` until the 130-entry DC THeroID domain can be reconciled
// with Complete's 156-entry roster; the local name, width and lifetime are
// already preserved.

// Two facts remain deliberate.  Retail reaches __CxxThrowException@8 twice
// and never calls bitset<70>::_Xran, so both bit stores use `operator[]` ->
// `reference::operator=` -> `set`; spelling either as `set(pos,val)` keeps the
// throw out of line.  Retail also CALLS three-argument basic_string::assign at
// the hero-name store, so the statement-scoped inline_depth(0) below prevents
// expansion without de-inlining ReadLengthPrefixedString.  Complete's decoded
// body has no Random relocation: unlike Dreamcast, an absent custom experience
// stays zero and is passed unchanged to GetStartingHeroId.
// With the preserved Owner dataflow restored, MAX is 82.6833 (HIST 92.7472).
// Unpinned `m_name = heroName` gives 75.34 and expands to 110 CFG blocks
// against retail's 94; direct assignment from the returned string also
// over-expands (75.71 before the Owner correction). These are not recovered
// boundaries, so the assignment pin remains diagnostic debt.

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
    // all, which is what the g_inCampaign consult is doing on both arms.
    unsigned char customExperience;
    if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA
        || mapVersion == MAP_FORMAT_ARMAGEDDONS_BLADE) {
        infile->read(&intBuffer, sizeof(intBuffer));
        experience = intBuffer;
        if (experience != 0 && (!g_inCampaign || experience >= 40))
            customExperience = 1;
        else
            customExperience = 0;
    } else {
        char experienceFlag;
        infile->read(&experienceFlag, sizeof(experienceFlag));
        customExperience = experienceFlag != 0;
        if (customExperience) {
            infile->read(&experience, sizeof(experience));
            if (g_inCampaign && experience < 40)
                customExperience = 0;
        } else {
            experience = 0;
        }
    }

    // Owner must survive the scratch-byte reads above. DC lines 3035/3039
    // use the preserved Owner (sp+0x13); retail likewise uses the original
    // owner byte for these player-table lookups. charBuffer now contains
    // the custom-name flag and is not a player index.
    if (heroID == -1) {
        if (g_startingHeroOverrides[owner] != -1) {
            heroID = g_startingHeroOverrides[owner];
            g_startingHeroOverrides[owner] = -1;
        } else {
            TTownType alignment;
            memcpy(&alignment, &g_game->m_setup.m_alignment[owner],
                   sizeof(alignment));
            heroID = g_game->getStartingHeroId(alignment, owner,
                                               experience);
        }
    }
    if (g_game->m_setup.m_startingHero[owner] == -1)
        g_game->m_setup.m_startingHero[owner] = heroID;

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
        if (!isRandomHero || g_inCampaign) {
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
            heroData->m_armies[x] =
                readMapCreatureId(infile, mapVersion);

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

    count = infile->read(padding, sizeof(padding));
    if (count < sizeof(padding))
        return -1;

    // A prison hero is not owned and not in any tavern pool.
    if (m_objectTypes[heroObject->m_typeIndex].m_objectType == PRISON)
        g_game->m_heroAvailability[heroID] = 0x41;
    else
        g_game->m_heroAvailability[heroID] = owner;

    heroData->m_location = heroObject->getTrigger();
    return 0;
}

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
        newGarrison.m_garrisonArmy.m_armies[slot] =
            readMapCreatureId(infile, mapVersion);

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
            setObjectType(dwelling.m_object, CREATURE_GENERATOR_4,
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
            setObjectType(dwelling.m_object, CREATURE_GENERATOR_1,
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

// Mac retains these six map-object readers in readObject's call sequence.
// Complete's Windows dispatcher expands their corresponding bodies.
void NewfullMap::readQuestGuardData(TAbstractFile* infile, CObject* tempObject)
{
    TQuestGuard tempGuard;
    tempGuard.read(infile);
    {
        m_questGuardList.push_back(tempGuard);
        tempObject->m_extraInfo = m_questGuardList.size() - 1;
    }
    if (tempGuard.m_quest) {
        CMapObjectData* questData = static_cast<CMapObjectData*>(
            static_cast<void*>(tempGuard.m_quest));
        m_mapObjectData.push_back(questData);
    }
}

static void readWitchHutData(TAbstractFile* infile, CObject* tempObject)
{
    if (g_game->m_mapHeader.m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        tempObject->m_extraInfo = 0xefbf;
    } else {
        int allowedSkills;
        infile->read(&allowedSkills, sizeof(allowedSkills));
        tempObject->m_extraInfo = allowedSkills;
    }
}

void NewfullMap::readRandomDwellingData(TAbstractFile* infile,
                                         CObject* object)
{
    RandomDwellingData dwelling;

    char value;
    infile->read(&value, sizeof(value));
    dwelling.m_owner = value;

    char padding[3];
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

    dwelling.m_object = object;
    m_randomDwellings.push_back(dwelling);
}

void NewfullMap::readRandomDwellingLevelData(TAbstractFile* infile,
                                              CObject* object)
{
    RandomDwellingData dwelling;

    char value;
    infile->read(&value, sizeof(value));
    dwelling.m_owner = value;

    char padding[3];
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
        m_objectTypes[object->m_typeIndex].m_extra);
    dwelling.m_maxLevel = static_cast<unsigned char>(
        m_objectTypes[object->m_typeIndex].m_extra);

    dwelling.m_object = object;
    m_randomDwellings.push_back(dwelling);
}

void NewfullMap::readRandomDwellingFactionData(TAbstractFile* infile,
                                                CObject* object)
{
    RandomDwellingData dwelling;

    char value;
    infile->read(&value, sizeof(value));
    dwelling.m_owner = value;

    char padding[3];
    infile->read(padding, 3);

    dwelling.m_castleId = 0;
    dwelling.m_factionMask = static_cast<unsigned short>(
        1 << m_objectTypes[object->m_typeIndex].m_extra);

    infile->read(&value, sizeof(value));
    dwelling.m_minLevel = value;
    infile->read(&value, sizeof(value));
    dwelling.m_maxLevel = value;

    dwelling.m_object = object;
    m_randomDwellings.push_back(dwelling);
}

void NewfullMap::readHeroPlaceholderData(TAbstractFile* infile, CObject* tempObject)
{
    char value;
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
// Dreamcast calls the ordinary readBoatData/readShipyardData/
// readHolyGrailData/readShrineData members. Their definitions stay at their
// original mapcell.cpp positions, and Complete expands those calls here.
// No standalone retail slot is needed to preserve those source boundaries.
// Mac retains the placeholder, quest, witch-hut, and random-dwelling
// readers; Complete expands their calls in this dispatcher.
// Restoring all four helpers raises 56.6382 -> 60.0594 in the 76-TU control.
// The native-vector frontier in QUEST_GUARD remains the large residual;
// no invented arm wrapper or additional inline-depth pin is introduced.

// MINE and LIGHTHOUSE share a compiler-generated tail. Retail calls the three
// dwelling inserts, both quest-guard inserts, and the quest-guard size query;
// natural push_back reproduces all three dwelling count-insert calls without
// pins. Quest-guard insertion still over-expands, so this is not an exact body.
// Unpinning only RANDOM_DWELLING_LVL measured 61.98%; using push_back also
// for the faction and quest-data appends measures 61.50% (baseline 61.38%).
// All 29 virtual read calls agree; DC's gzread calls are the older file API.
// Complete additionally passes mapVersion and includes the new object arms.

// Older pinned-model register evidence (97.4369%, not the current residual):
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
// Follow-up after ordinary-reader restoration: keep DC's function-scope
// count and the five separate header-read/result-test statements. The
// 36-state QUEST_GUARD lifetime family finds public push_back at 60.9729%
// versus direct two-argument insert at 60.0594%; no sibling score moves.
// Both guard/data insertion workers still expand where retail calls the
// two-argument bodies, so this does not close that native-vector frontier.
// Current residual (59.40%): readSeerData expands with its owned temporary,
// read and pool insertion. VC6 also expands the temporary TSeerHut constructor
// that retail retains; other map-reader call/expansion differences remain.
VA(0x00502e00, 0x832)  // order-map: dispatches to all read*Data rows (DC-isomorphic callee set) + CreateBoat 0x4bb250 (readBoatData inlined) + TQuestGuard::read (retail quest path); readHolyGrail/readShrine/readShipyard inlined, dc 0xf16c8
int NewfullMap::readObject(TAbstractFile* infile, CObject* tempObject,
                           int mapVersion)
{
    int count;
    char value;
    count = infile->read(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    tempObject->m_x = value;

    count = infile->read(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    tempObject->m_y = value;

    count = infile->read(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    tempObject->m_z = value;

    int typeIndex;
    count = infile->read(&typeIndex, sizeof(typeIndex));
    if (count < sizeof(typeIndex))
        return -1;
    tempObject->m_typeIndex = static_cast<unsigned short>(typeIndex);

    char padding[5];
    count = infile->read(padding, sizeof(padding));
    if (count < sizeof(padding))
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

    case BOAT:
        readBoatData(infile, tempObject);
        break;

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

    case HERO_PLACEHOLDER:
        readHeroPlaceholderData(infile, tempObject);
        break;

    case SPELL_SCROLL:
        readSpellScrollData(infile, tempObject);
        break;

    case SHIPYARD:
        readShipyardData(infile, tempObject);
        break;

    case RANDOM_RESOURCE:
    case RESOURCE:
        readResourceData(infile, tempObject);
        break;

    case HOLY_GRAIL:
        readHolyGrailData(infile, tempObject);
        break;

    case BLACK_BOX:
        readBlackBoxData(infile, tempObject, mapVersion);
        break;

    case SCHOLAR:
        readScholarData(infile, tempObject);
        break;

    case SEER:
        readSeerData(infile, tempObject);
        break;

    case SHRINE1:
    case SHRINE2:
    case SHRINE3:
        readShrineData(infile, tempObject);
        break;

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
        readRandomDwellingData(infile, tempObject);
        break;
    }

    case RANDOM_DWELLING_LVL: {
        readRandomDwellingLevelData(infile, tempObject);
        break;
    }

    case RANDOM_DWELLING_FACTION: {
        readRandomDwellingFactionData(infile, tempObject);
        break;
    }

    case QUEST_GUARD:
        readQuestGuardData(infile, tempObject);
        break;

    case WITCH_HUT:
        readWitchHutData(infile, tempObject);
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

    // The object type is read through the int staging value and copied into
    // the typed member, exactly as the trait fixup below copies into it: a
    // separate TAdventureObjectType local takes its own frame slot and pushes
    // every later displacement by four (99.9633 against retail's 0x8c frame).
    count = infile->read(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    memcpy(&tempObjectType.m_objectType, &value,
           sizeof(tempObjectType.m_objectType));
    if (usedDefaultMask) {
        sprintf(g_text,
                DATA_COMPGEN(0x0067fb10, readObjectTypeMissingMask,
                             "Could not load mask file for %s! - Type: %s"),
                tempObjectType.m_imageName.c_str(),
                g_quickViewText[tempObjectType.m_objectType]);
        MessageBoxA(g_hwndApp, g_text, "Error!", 0);
    }

    memcpy(&tempObjectType.m_objectType,
           &g_adventureObjectTraits[tempObjectType.m_objectType].m_nameRow,
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
    game::saveString(outfile, tempObjectType->m_imageName);

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
    tempObjectType->m_objectType = TAdventureObjectType(typeValue);

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
// Mac 0:0x1270c0..0x127278 is the same later helper: it clears 232 per-class
// lists, scans 0x38-byte types backwards by extra and image name, and has the
// same two reader calls. Declaration order, equality operand order and
// signedness of a narrowed index are shared source decisions; differing
// instruction choices do not establish a platform fork. No DC procedure
// is claimed for this Complete addition.
VA(0x005042c0, 0x1A5)  // retail body + two callers: readMapObjects/loadMapObjects; no DC roster row
void NewfullMap::rebuildObjectTypeIndex()
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
// DC's long i (sp+0x18) belongs to the preceding hero-reset loop, absent in
// Complete. These three loading loops use int x (sp+0x1c); push_back at
// dc 0xf2d48 passes that slot directly to vector<int>::push_back(const int&).
// A long counter instead creates a conversion temporary and changes VC6's
// aliasing/induction decisions despite having the same x86 width.
// Both count reads use int_buffer (sp+0x34), then copy to numObjects (sp+0x3c).
// count (sp+0x30) owns read lengths and object-reader status; int v (sp+0x2c)
// scans invalid placements. This ownership and the two braced read guards
// reproduce retail, including its distinct empty/nonempty vector cleanups.
VA(0x00504470, 0x5C9)  // order-map: calls readObject 0x502e00 + readObjectType 0x503780 + GetSprite 0x55c7b0 + Random x2 (CObject ctor inlined) + progress-bar helpers; $E482-$E485 pair sits just before at 0x104260/0x104290 matching DC link order; EH-bearing, dc 0xf2c20
int NewfullMap::readMapObjects(TAbstractFile* infile, int mapVersion)
{
    g_invalidPlacementList.clear();

    int intBuffer;
    int numObjects;
    int count;
    int x;
    count = infile->read(&intBuffer, sizeof(intBuffer));
    if (count < sizeof(intBuffer)) {
        return -1;
    }

    numObjects = intBuffer;
    m_objectTypes.resize(numObjects);
    for (x = 0; x < m_objectTypes.size(); ++x) {
        count = readObjectType(infile, m_objectTypes[x]);
        if (count < 0)
            return -1;
        if (x == m_objectTypes.size() / 2)
            incProgressBar(1);
        if (count == READ_OBJECT_TYPE_DEFAULT_MASK)
            g_invalidPlacementList.push_back(x);
    }

    rebuildObjectTypeIndex();
    incProgressBar(1);

    std::vector<CSprite*> oldSprites;
    oldSprites.resize(m_sprites.size());
    for (x = 0; x < m_sprites.size(); ++x)
        oldSprites[x] = m_sprites[x];

    m_sprites.resize(numObjects);
    for (x = 0; x < m_objectTypes.size(); ++x) {
        m_sprites[x] =
            ResourceManager::getSprite(m_objectTypes[x].m_imageName.c_str());
        if (x == m_objectTypes.size() / 3)
            incProgressBar(1);
        if (x == m_objectTypes.size() / 3 * 2)
            incProgressBar(1);
    }

    for (x = 0; x < oldSprites.size(); ++x)
        oldSprites[x]->dispose();
    oldSprites.clear();

    incProgressBar(1);

    count = infile->read(&intBuffer, sizeof(intBuffer));
    if (count < sizeof(intBuffer)) {
        return -1;
    }

    numObjects = intBuffer;
    m_objects.resize(numObjects);
    for (x = 0; x < m_objects.size(); ++x) {
        count = readObject(infile, &m_objects[x], mapVersion);
        if (count < 0)
            return -1;

        for (int v = 0; v < g_invalidPlacementList.size();
             ++v) {
            if (m_objects[x].m_typeIndex == g_invalidPlacementList[v]) {
                sprintf(g_text,
                        DATA_COMPGEN(0x0067fb48, readMapObjectsInvalidObject,
                                     "Invalid Object Referenced!\n\n"
                                     "x: %d y: %d z: %d - Type: %s"),
                        m_objects[x].m_x, m_objects[x].m_y, m_objects[x].m_z,
                        g_quickViewText[
                            m_objectTypes[m_objects[x].m_typeIndex].m_objectType]);
                MessageBoxA(g_hwndApp, g_text,
                            DATA_COMPGEN(0x0067fb08,
                                         readMapObjectsErrorCaption, "Error!"),
                            0);
            }
        }

        m_objects[x].m_animationOffset = static_cast<unsigned char>(random(0, 255));
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
// DC mapcell.cpp:3974 names the shared loop counter `int x`; restoring that
// signed local closes Complete's final error-cleanup block (61/61 CFG blocks).

VA(0x00504b70, 0x4E9)  // dc 0xf318c
int NewfullMap::loadMapObjects(TAbstractFile* infile)
{
    int count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_objectTypes.resize(count);

    int x;
    for (x = 0; x < m_objectTypes.size(); ++x) {
        if (loadObjectType(infile, &m_objectTypes[x]) < 0)
            return -1;
    }

    incProgressBar(1);
    rebuildObjectTypeIndex();

    std::vector<CSprite*> oldSprites;
    oldSprites.resize(m_sprites.size());
    for (x = 0; x < m_sprites.size(); ++x)
        oldSprites[x] = m_sprites[x];

    m_sprites.resize(m_objectTypes.size());
    for (x = 0; x < m_objectTypes.size(); ++x) {
        m_sprites[x] =
            ResourceManager::getSprite(m_objectTypes[x].m_imageName.c_str());
        if (x == m_objectTypes.size() / 3)
            incProgressBar(1);
        if (x == m_objectTypes.size() / 3 * 2)
            incProgressBar(1);
    }

    for (x = 0; x < oldSprites.size(); ++x)
        oldSprites[x]->dispose();
    oldSprites.clear();

    incProgressBar(1);

    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_objects.resize(count);
    for (x = 0; x < m_objects.size(); ++x) {
        if (loadObject(infile, &m_objects[x]) < 0)
            return -1;
        m_objects[x].m_animationOffset = static_cast<unsigned char>(random(0, 255));
    }

    incProgressBar(1);
    return 1;
}

// The WIDTH index is the outer loop and the height index the inner one - the
// grid pointer advances by six per outer iteration while the inner one indexes
// single bytes, which fixes the orientation as heightMap[width][height].  Both
// loop bounds are re-read from the type record on every iteration, so neither
// is hoisted.

// DC names four CObjectType::_getBitPos calls; Mac retains the same helper at
// 0:0x127c38. Complete expands them without changing this exact retail body.
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
                if (!m_objectTypes[typeIndex].m_passableCells[CObjectType::getBitPos(col, row)]
                    && m_objectTypes[typeIndex].m_passableCells[CObjectType::getBitPos(col, row - 1)])
                    depth = 1;
            }
            if (col > 0) {
                if (m_objectTypes[typeIndex].m_passableCells[CObjectType::getBitPos(col, row)]
                    && !m_objectTypes[typeIndex].m_passableCells[CObjectType::getBitPos(col - 1, row)])
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
        if (objectType->m_triggerCells.test(
                CObjectType::getBitPos(col, row))) {
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
            if (!objectType->m_passableCells.test(
                    CObjectType::getBitPos(col, row))) {
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
        if (!objectType->m_passableCells.test(
                CObjectType::getBitPos(col, row))) {
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

    // Mac retains getObjectTypePtr here at 0:0x128560; VC6 expands it.
    TAdventureObjectType objectClass = object->getObjectTypePtr()->m_objectType;

    for (int col = 0; col < objectType->m_width; ++col) {
        if (object->m_x - col < 0 || object->m_x - col >= g_mapWidth)
            continue;

        for (int row = 0; row < objectType->m_height; ++row) {
            if (object->m_y - row < 0 || object->m_y - row >= g_mapHeight)
                continue;

            NewmapCell* cell = g_game->m_worldMap.cell(
                object->m_x - col, object->m_y - row, object->m_z);

            if (objectClass == HOLY_GRAIL || objectClass == HERO
                || objectClass == RANDOM_HERO
                || objectClass == HERO_PLACEHOLDER || objectClass == BOAT)
                continue;

            if (objectClass == EVENT) {
                if (objectType->m_triggerCells.test(
                        CObjectType::getBitPos(col, row))) {
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
void NewfullMap::notifyHeroDefeated(int heroId, int player)
{
    for (unsigned int i = 0; i < m_mapObjectData.size(); ++i)
        m_mapObjectData[i]->notifyHeroDefeated(heroId, player);
}

VA(0x00505d60, 0x3F)
void NewfullMap::notifyMonsterDefeated(type_point point, int player)
{
    for (unsigned int i = 0; i < m_mapObjectData.size(); ++i)
        m_mapObjectData[i]->notifyMonsterDefeated(point, player);
}

VA(0x00505da0, 0xF8)
void NewfullMap::loadObjectTypeTemplates()
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

VA(0x00505ea0, 0x80)  // linkorder + this@+0xdc=objectTypeIndex; reverse-find CObjectType by extra, caller game::ConvertObject, retail-only
CObjectType* NewfullMap::findObjectType(int objectType, int extra)
{
    int i = m_objectTypeIndex[objectType].size();
    while (i--) {
        if (m_objectTypeIndex[objectType][i].m_extra == extra)
            break;
    }
    return &m_objectTypeIndex[objectType][i];
}

// 0x505f20 (game::InsertObject's helper, declared game.h:466) reverse-scans
// objectTypeIndex[objectType] for the record matching `objectIndex` (its
// .extra) and, when terrain != -1, applicable to `terrain` (its mask_34 bit).
// If the matched record has no resolved objectTypes index yet (field_42 < 0),
// it appends a copy to objectTypes and its sprite to sprites and records the
// new index, then writes the resolved index into object->typeIndex.
// No direct DC counterpart; Complete retains both vector count-insert calls.
// Ordinary push_back calls reproduce them at 100% without inline-depth pins:
// named/direct record and sprite arguments all reproduce the same exact body.
// The sprite insert's widget* retail name is a folded pointer-vector alias.
// The non-const bitset operator[] proxy supplies retail's third frame slot;
// .test(terrain) instead compacts the frame from 0xc to 0x8. There is no
// missing-record failure branch in retail; the caller must supply a match.
VA(0x00505f20, 0x157)  // linkorder + this@+0xdc=objectTypeIndex; caller game::InsertObject, retail-only
void NewfullMap::setObjectType(CObject* object, int objectType,
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

    if (static_cast<short>(m_objectTypeIndex[objectType][i].m_objectTypeIndex) < 0) {
        m_objectTypeIndex[objectType][i].m_objectTypeIndex =
            static_cast<unsigned short>(m_objectTypes.size());
        m_objectTypes.push_back(m_objectTypeIndex[objectType][i]);
        m_sprites.push_back(ResourceManager::getSprite(
            m_objectTypeIndex[objectType][i].m_imageName.c_str()));
    }

    object->m_typeIndex = m_objectTypeIndex[objectType][i].m_objectTypeIndex;
}

// The map-editor template's conversion into the runtime object-type record,
// and the first row past NewfullMapFn_00505F20 - the constructor
// NewfullMapFn_00505DA0 calls once per row of objects.txt, whose address
// advmgr_objects.h already records against this class.

// The four 48-cell masks are transposed cell by cell through the class's own
// _getBitPos(x, y) = 47 - y * 8 - x.

// The image name, sizes, four masks, recommended-terrain mask, type, subtype
// and underlay flag cross here. hasTrigger, triggerCell, slotCategory and
// terrainMask stay with the editor template.
// Complete-only conversion constructor: retail 0x506080 constructs the
// string and five masks, then copies the editor template's runtime fields.
// DC CObjectType fieldlist 0x309c (class 0x309b) declares only the generated
// default/copy constructors (attributes 0x103), with no TObjectType* overload.
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
// The mutable/const-source int-copy helpers at 0x5093c0/0x54df40 now expand
// in mapcell. Both canonical <algorithm> specializations still emit in rmg,
// where their enrollments live. The mutable form is also called for folded pointer
// arrays; BlackBoxData's implicit assignment calls the separate const form.
VA_COMPGEN(0x005093f0, 0x1A4, STD_COPY, TTimedEvent)
VA_COMPGEN(0x005095a0, 0x33, STD_COPY, garrison)
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

// Original: NewfullMap::placeObjects; mapcell.cpp:4404, dc 0xf4740.
// Read's placement pass owns this ordinary helper; the int loop variable is
// compared with vector::size() as recorded in the DC unsigned comparison.
int NewfullMap::placeObjects()
{
    for (int x = 0; x < m_objects.size(); ++x)
        placeObject(x, 1);
    return 0;
}


VA_COMPGEN(0x00508cf0, 0x3B9, VECTOR_INSERT, TownExtra)

// Retail insert(position, count, value) is a separate overload from 0x5078b0.
VA_COMPGEN(0x00507ad0, 0x2F9, VECTOR_INSERT_COUNT, TQuestGuard)

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
