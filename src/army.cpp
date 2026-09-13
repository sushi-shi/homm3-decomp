// army.cpp - E:\gamedcs\army.cpp (compiland army.obj)

#include <algorithm>
#include <math.h>
#include <stdlib.h>

#include <va.h>
// army::get_clockwise / get_counter_clockwise and the two direction
// tables they index: army.cpp is their only consumer, and declaring
// them to every consumer of army.h costs command.obj's GetCommand
// 92.5714 -> 92.5357 (measured 2026-08-14, include-set class).
// The three creature ids ComputeAttackerDamageReduction's two elemental
// rules name are scoped the same way and for the same reason: declaring
// those ENUMERATORS to every consumer costs the same 0.0357 on the same
// function (measured 2026-08-14, bisected against the field slicing in
// the same change, which is innocent).
#include "creaturetype.h"
#include "army.h"
// ai.h: the narrow EAreaAttackCreature roster - LoadResources' missile
// switch needs its ARCHER/MAGOG/POWER_LICH, which deliberately live
// there rather than in armygrp.h's wide enum (see ai.h's own note).
// Include-set canaries measured after this edge was added: initialize/
// events/recruit all unmoved.
#include "ai.h"
// SSpellTraits' m_sample slice: army.cpp is its only consumer and this
// header sits inside initialize.cpp's include closure (see the field).
#include "armygrp.h"
#include "bitmap16.h"
#include "cmbtmgr.h"
#include "combatwindow.h"
#include "csprite.h"
#include "cspriteframe.h"   // CroppedY for LoadResources' image_height
#include "drawing.h"
// get_berserk_targets (0x445490) seeds the combat search and then reads
// the cost back out of cellData by hand; includes are TU-local and cost
// nothing to the include-set canaries.
#include "findpath.h"
#include "font.h"   // gpTinyFont's DrawBoundedString, for the count box
#include "game.h"   // gpGame->f_1f698, initialize's elemental-town gate
#include "hero.h"
#include "herospec.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "monframeinfo.h"   // gMonFrameInfo for LoadResources' traits copy
#include "palette.h"        // TPalette16, for DrawToBuffer's tint arms
#include "path.h"           // GetAdjacentCellIndexNoArmy for the splash loops
#include "prefs.h"
#include "resourcemanager.h"   // GetSprite for attack_wall's explosion
#include "sample.h"
#include "soundmgr.h"
#include "textresource.h"
#include "town.h"
#include "winmgr.h"
#include "includes.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// E:\gamedcs\army.cpp:52 (dc 0x436b8) - retail 0x43d250, the FIRST row of
// army.obj's span: the compiland's cinit/atexit thunk opens at 0x43ce60 and
// the nine ~95-byte rows between it and this one are the /Gy header COMDATs
// the link parked ahead of the first real body.  The constructor is fixed by
// its member-construction run alone: the two TResourceHandle<CSprite> heads
// at +0x164/+0x168, the `??_L` eh-vector-constructor-iterator over EIGHT
// FOUR-byte elements at +0x170 (armySample), the deque<int> at +0x420 and
// the four vector<army*> at +0x4f4/+0x504/+0x514/+0x524 - every one of them
// a member this header already models, in declaration order, with unwind
// states 1..7 across the run.
// Residual (95.9799%): ONE inline decision, and nothing else - every
// other instruction pairs.  Retail expands BOTH `deque<int>::iterator`
// default constructions inside the deque's own default constructor (four
// inline dword zero-stores at +0x424..+0x430); our /O2 calls
// `??0const_iterator@?$deque@HV?$allocator@H@std@@@std@@` for the first
// and expands the second, which also adds the intervening unwind-state
// store retail does not need.  It is the UNDER-inline direction (grow the
// caller), and this body has no mass to give: writing the eight-sample
// loop ahead of the icon disposals measured 64.60 and the literal 8 in
// place of MAX_SAMPLES is byte-flat.
// The array element destructor retail hands to `??_L` (0x43cb10) IS
// observable and its body is `if (resource) resource->Dispose()`, but
// TResourceHandle's destructor cannot carry it: `T` is incomplete in every
// TU that sees army.h without csprite.h/sound.h, and giving it that body
// fails the build with C2027 on CSprite and sample.  The relocation is a
// name-only difference the ratchet already ignores.
VA(0x0043d250, 0x1A8)  // anchor-global + member-construction run, dc 0x436b8
army::army()
{
    if (m_stdIcon)
        m_stdIcon->dispose();
    m_stdIcon = 0;
    if (m_missileIcon)
        m_missileIcon->dispose();
    m_missileIcon = 0;
    m_imageHeight = 0;
    m_gridIndex = 0;
    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (m_armySample[i])
            m_armySample[i]->dispose();
        m_armySample[i] = 0;
    }
    m_side = -1;
    m_slot = -1;
    m_attackLimit = 0;
    m_pathTarget = 0;
    m_yModify = 0;
    m_showTroopCount = 1;
    m_ySpecialMod = 0;
    m_xSpecialMod = 0;
    m_retaliationCount = 1;
    m_isMoving = 0;
    m_letsPretendImNotHere = 0;
}

VA(0x0043d400, 0x136)
army::~army()
{
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:77
DC_ONLY(0x437ac, 0x84)
void army::setRetaliationCount()
{
    // @stub
}

// E:\gamedcs\army.cpp:93
#endif  // @carcass

VA(0x0043d540, 0x34)  // dc 0x43830
void army::playSample(army::TSampleID id)
{
    if (!static_cast<const combatManager*>(g_combatManager)->isQuickCombat()
        && m_armySample[id]) {
        g_soundManager->memorySample(m_armySample[id]);
    }
}

VA(0x0043d580, 0x37)  // dc 0x4386c
void army::stopSample(army::TSampleID id)
{
    if (!static_cast<const combatManager*>(g_combatManager)->isQuickCombat()
        && m_armySample[id]) {
        g_soundManager->stopSample(m_armySample[id]->m_memSample.m_memSampleHandle);
    }
}

// Put the stack back to its "loaded but not in a battle" state: drop
// every sound and the sprite, wipe the standing-spell row and the
// spell queue, and reset the seven scalars that mean "nothing here".

// The four slots between stop_sample and SetLuck pop 0 / 0x18 / 0x1c / 0
// stack bytes, i.e. 0 / 6 / 7 / 0 arguments, and the six DC rows that could
// occupy them take 1 / 0 / 6 / 7 / 0 / 0 - so InitClean, initialize, Init
// and LoadResources fit in order and WaitSample (one argument) cannot.

// A TRAP THIS BODY COST AN HOUR ON, worth knowing before promoting any
// other carcass: InitClean had NO declarator in army.h at all. A
// carcass @stub compiles fine inside `#if 0`, and the moment it is
// promoted VC6 reports the missing declarator as a CASCADE of
// "undeclared identifier" on the body's OWN MEMBERS - the "InitClean is
// not a member of army" line does not appear in the full-file compile
// at all - so the message points at whichever field you added last
// instead of at the declarator. Bisect it by moving one member access
// to the TOP of the body: if it still errors there, it is the
// declarator, not the field.

// THREE THINGS THIS BODY DECIDES ABOUT THE CLASS, all recorded on the
// fields themselves: +0x420 is a std::deque (the erase-with-two-16-byte
// -iterators call), +0xfc is DC's iLastFidgetTime (the GameTime::Get
// stamp) and +0x24/+0x28 are the iMirror* pair rather than pad - they
// take -1 out of the same `or eax,-1` the other four resets share,
// which is what makes them dword fields.

// The `for` counts UP in the source and DOWN in the bytes: nothing in
// the loop reads `i` except the subscript, so C2 strength-reduces the
// walk to a pointer and turns the index into a countdown. It is
// memory-homed at [ebp-4] because the dispose call leaves no free
// register - that slot is then reused for the two dead erase
// iterators at the bottom.

// Retail calls vector<SpellID>::erase at both clear sites. Keep begin() and
// end() outside the pinned erase statement.

// Residual (92.6261%): the register-homing family. Retail fills the
// two by-value iterator temps through EDI as scratch with EAX/EDX
// holding the slot pointers; ours picks the mirror assignment, and the
// begin/end declaration-order swap is byte-inert (measured both ways,
// 92.6261 exactly). The GameTime store schedules one slot later than
// retail's and the dispose vtable call uses EDX where retail uses EAX
// - all downstream of the same homing choice, no spelling reaches it.
VA(0x0043d5c0, 0x166)  // anchor-bracket + arity, dc 0x438e8
void army::initClean()
{
    for (int i = 0; i < 8; i++) {
        if (m_armySample[i])
            m_armySample[i]->dispose();
        m_armySample[i] = 0;
    }
    m_roundsLeftBeforeVanish = -1;
    m_numSpellInfluences = 0;
    memset(m_spellInfluence, 0, sizeof(m_spellInfluence));
    {
        // clear() spelled through its own body with the erase pinned:
        // retail expands clear and CALLS deque::erase (0x448db0), and
        // our CL - the InitClean residual note below - inlines erase
        // and starves. The statement-scoped depth(0) reproduces the
        // rejection; begin()/end() build their 16-byte temps inline in
        // the two unpinned statements exactly as retail does.
        TSpellQueue::iterator queueEnd = m_spellInfluenceQueue.end();
        TSpellQueue::iterator queueBegin = m_spellInfluenceQueue.begin();
#pragma inline_depth(0)
        m_spellInfluenceQueue.erase(queueBegin, queueEnd);
#pragma inline_depth()
    }
    m_lastFidgetTime = GameTime::get();
    if (m_stdIcon)
        m_stdIcon->dispose();
    m_stdIcon = 0;
    m_imageHeight = 0;
    m_showPowEffect = 0;
    m_resetThisRound = 0;
    m_postPowSpellToCast = -1;
    m_mirrorSourceIndex = -1;
    m_mirrorDestIndex = -1;
    m_originalIndex = -1;
    m_numTroopsToShowOverride = -1;
    m_joustBonus = 0;
    m_isAreaEffectTarget = 0;
    m_auraSources.clear();
    m_auraClients.clear();
}

// E:\gamedcs\army.cpp:109
// No retail out-of-line copy survives, but the Dreamcast call graph proves
// this member boundary in range_attack. VC6 folds the inline definition into
// that caller.
inline void army::waitSample(army::TSampleID which)
{
    if (!static_cast<const combatManager*>(g_combatManager)
            ->isQuickCombat()
        && m_armySample[which]) {
        g_soundManager->waitSample(m_armySample[which]->m_memSample.m_memSampleHandle, -1);
    }
}

VA(0x0043d730, 0x17D)  // dc 0x439b0
void army::initialize(int type, long number, const hero* owner,
                      long newGroup, long newIndex, long newGridIndex)
{
    initClean();
    memcpy(&m_creatureType, &type, sizeof(m_creatureType));
    m_numTroops = number;
    m_drawPriority = 4;
    TCreatureTypeTraits* traits = &m_monInfo;
    *traits = g_creatureTypeTraits[type];
    traits->m_townType =
        (g_game->m_f1f698 == 0
         && isBaseElemental(type))
            ? -1
            : g_creatureTypeTraits[type].m_townType;
    if (owner != 0)
        owner->heroFn004E6120(type, traits);
    if (g_combatManager->m_magicTerrain
            != COMBAT_SPELL_RESTRICTION_NO_CREATURE_SPELLS
        && g_nativeTerrains[m_monInfo.m_townType]
               == g_combatManager->m_terrainType)
        m_onNativeTerrain = 1;
    else
        m_onNativeTerrain = 0;
    if (m_onNativeTerrain) {
        if (!(is(1u << 6)))
            m_monInfo.m_speed++;
        m_monInfo.m_attackSkill++;
        m_monInfo.m_defenseSkill++;
    }
    m_facing = 1 - newGroup;
    m_currFrameType = cs_wait;
    m_side = -1;
    m_combatSide = newGroup;
    m_slot = -1;
    m_bitIndex = newIndex;
    m_gridIndex = newGridIndex;
    m_numTroopsBattleResurrected = 0;
    m_currFrameIndex = 0;
    m_luckStatus = 0;
    m_topCreatureDamage = 0;
    m_showAttackFrames = 0;
    m_residualBlindness = 0;
    m_residualParalyze = 0;
    m_allUnitsKilled = 0;
    m_someUnitsDamaged = 0;
    m_showFireShield = 0;
    m_origNumTroops = m_numTroops;
    m_baseSpeed = m_monInfo.m_speed;
    m_origHitPoints = m_monInfo.m_hitPoints;
    m_poisonPenalty = 1.0f;
}

VA(0x0043d8b0, 0x135)  // dc 0x43d9c
void army::init(int armyId, int newNumTroops, const hero* owner, int side,
                int inIndex, int gridIndex, int origPos)
{
    initialize(armyId, newNumTroops, owner, side, inIndex, gridIndex);
    if (g_combatManager->validHex(m_gridIndex)) {
        hexcell* cell = &g_combatManager->m_cells[m_gridIndex];
        cell->m_armySide = static_cast<signed char>(m_combatSide);
        cell->m_armySlot = static_cast<signed char>(m_bitIndex);
        cell->m_partOfDouble = -1;
        if (is(1u << 0)) {
            hexcell* second =
                &g_combatManager->m_cells[m_gridIndex + offsetToFront(-1)];
            second->m_armySide = static_cast<signed char>(m_combatSide);
            second->m_armySlot = static_cast<signed char>(m_bitIndex);
            second->m_partOfDouble = m_facing != 0;
            cell->m_partOfDouble = m_facing == 0;
        }
        addAura();
    }
    m_originalIndex = origPos;
    m_retaliationCount = 1;
    if (m_creatureType == ARMY_CREATURE_GRIFFIN)
        m_retaliationCount = 2;
    if (m_creatureType == ARMY_CREATURE_ROYAL_GRIFFIN)
        m_retaliationCount = 5000;
    if (m_spellInfluence[SPELL_COUNTERSTRIKE])
        m_retaliationCount += m_counterstrokeBonus;
    if (is(1u << 6))
        m_retaliationCount = 0;
}

VA(0x0043d9f0, 0x525)
void army::loadResources()
{
    if (static_cast<const combatManager*>(g_combatManager)
            ->isQuickCombat())
        return;

    memcpy(m_monFrameInfo.m_missileOffset, &g_monFrameInfo[m_creatureType],
           sizeof(SMonFrameInfo));
    m_origWalkCycleTime = m_monFrameInfo.m_walkCycleTime;

    sample* s;
    if (!(is(1u << 6))) {
        sprintf(g_text, DATA_COMPGEN(0x00660a10, moveSampleFormat,
                                    "%smove.82M"),
                m_monInfo.m_samplePrefix);
        s = ResourceManager::getSample(g_text);
        if (m_armySample[WALK_SAMPLE])
            m_armySample[WALK_SAMPLE]->dispose();
        m_armySample[WALK_SAMPLE] = s;
    } else {
        if (m_armySample[WALK_SAMPLE])
            m_armySample[WALK_SAMPLE]->dispose();
        m_armySample[WALK_SAMPLE] = 0;
    }

    if (m_creatureType == CREATURE_BALLISTA)
        sprintf(g_text, DATA_COMPGEN(0x00660a04, shotSampleFormat,
                                    "%sshot.82M"),
                m_monInfo.m_samplePrefix);
    else if (is(1u << 6))
        sprintf(g_text, DATA_COMPGEN(0x006609f8, winceSampleFormat,
                                    "%swnce.82M"),
                m_monInfo.m_samplePrefix);
    else
        sprintf(g_text, DATA_COMPGEN(0x006609ec, attackSampleFormat,
                                    "%sattk.82M"),
                m_monInfo.m_samplePrefix);
    s = ResourceManager::getSample(g_text);
    if (m_armySample[ATTACK_SAMPLE])
        m_armySample[ATTACK_SAMPLE]->dispose();
    m_armySample[ATTACK_SAMPLE] = s;

    sprintf(g_text, DATA_COMPGEN(0x006609f8, winceSampleFormat,
                                "%swnce.82M"),
            m_monInfo.m_samplePrefix);
    s = ResourceManager::getSample(g_text);
    if (m_armySample[WINCE_SAMPLE])
        m_armySample[WINCE_SAMPLE]->dispose();
    m_armySample[WINCE_SAMPLE] = s;

    sprintf(g_text, DATA_COMPGEN(0x006609e0, killSampleFormat,
                                "%skill.82M"),
            m_monInfo.m_samplePrefix);
    s = ResourceManager::getSample(g_text);
    if (m_armySample[DIE_SAMPLE])
        m_armySample[DIE_SAMPLE]->dispose();
    m_armySample[DIE_SAMPLE] = s;

    if (is(1u << 6))
        sprintf(g_text, DATA_COMPGEN(0x006609f8, winceSampleFormat,
                                    "%swnce.82M"),
                m_monInfo.m_samplePrefix);
    else
        sprintf(g_text, DATA_COMPGEN(0x006609d4, defendSampleFormat,
                                    "%sdfnd.82M"),
                m_monInfo.m_samplePrefix);
    s = ResourceManager::getSample(g_text);
    if (m_armySample[DEFEND_SAMPLE])
        m_armySample[DEFEND_SAMPLE]->dispose();
    m_armySample[DEFEND_SAMPLE] = s;

    if ((is(1u << 2)) || m_creatureType == CREATURE_MASTER_GENIE
        || m_creatureType == CREATURE_OGRE_MAGE) {
        sprintf(g_text, DATA_COMPGEN(0x00660a04, shotSampleFormat,
                                    "%sshot.82M"),
                m_monInfo.m_samplePrefix);
        s = ResourceManager::getSample(g_text);
        if (m_armySample[SHOOT_SAMPLE])
            m_armySample[SHOOT_SAMPLE]->dispose();
        m_armySample[SHOOT_SAMPLE] = s;
    } else {
        if (m_armySample[SHOOT_SAMPLE])
            m_armySample[SHOOT_SAMPLE]->dispose();
        m_armySample[SHOOT_SAMPLE] = 0;
    }

    if (m_creatureType == CREATURE_VAMPIRE
        || m_creatureType == CREATURE_VAMPIRE_LORD
        || m_creatureType == CREATURE_DEVIL
        || m_creatureType == CREATURE_ARCH_DEVIL) {
        sprintf(g_text, DATA_COMPGEN(0x006609c8, ext1SampleFormat,
                                    "%sext1.82M"),
                m_monInfo.m_samplePrefix);
        s = ResourceManager::getSample(g_text);
        if (m_armySample[PRE_WALK_SAMPLE])
            m_armySample[PRE_WALK_SAMPLE]->dispose();
        m_armySample[PRE_WALK_SAMPLE] = s;
        sprintf(g_text, DATA_COMPGEN(0x006609bc, ext2SampleFormat,
                                    "%sext2.82M"),
                m_monInfo.m_samplePrefix);
        s = ResourceManager::getSample(g_text);
        if (m_armySample[POST_WALK_SAMPLE])
            m_armySample[POST_WALK_SAMPLE]->dispose();
        m_armySample[POST_WALK_SAMPLE] = s;
    } else {
        if (m_armySample[PRE_WALK_SAMPLE])
            m_armySample[PRE_WALK_SAMPLE]->dispose();
        m_armySample[PRE_WALK_SAMPLE] = 0;
        if (m_armySample[POST_WALK_SAMPLE])
            m_armySample[POST_WALK_SAMPLE]->dispose();
        m_armySample[POST_WALK_SAMPLE] = 0;
    }

    for (int i = 0; i < 8; i++) {
        if (m_armySample[i]) {
            m_armySample[i]->m_memSample.m_memVolume = 64;
            m_armySample[i]->m_memSample.m_memCindex = 3;
            m_armySample[i]->m_memSample.m_memLooping = i != 0;
        }
    }

    CSprite* icon =
        ResourceManager::getSprite(g_creatureTypeTraits[m_creatureType]
                                       .m_spriteName);
    if (m_stdIcon)
        m_stdIcon->dispose();
    m_stdIcon = icon;
    m_imageHeight = 267 - m_stdIcon->getFrame(cs_wait, 0)->getCroppedY();

    if (is(1u << 2)) {
        const char* missileName;
        switch (m_creatureType) {
        case CREATURE_ARCHER:
        case CREATURE_MARKSMAN:
            missileName = DATA_COMPGEN(0x006609b0, crossbowMissileName,
                                       "plcbowx.def");
            break;
        case CREATURE_MONK:
        case CREATURE_ZEALOT:
            missileName = DATA_COMPGEN(0x006609a4, zealotMissileName,
                                       "cprzeax.def");
            break;
        case CREATURE_WOOD_ELF:
        case CREATURE_GRAND_ELF:
        case CREATURE_SHARPSHOOTER:
            missileName = DATA_COMPGEN(0x00660998, elfMissileName,
                                       "pelfx.def");
            break;
        case CREATURE_MASTER_GREMLIN:
            missileName = DATA_COMPGEN(0x0066098c, gremlinMissileName,
                                       "cprgre.def");
            break;
        case CREATURE_HALFLING:
            missileName = DATA_COMPGEN(0x00660980, halflingMissileName,
                                       "Phalf.def");
            break;
        case CREATURE_GOG:
        case CREATURE_MAGOG:
            missileName = DATA_COMPGEN(0x00660974, gogMissileName,
                                       "cprgogx.def");
            break;
        case CREATURE_LICH:
        case CREATURE_POWER_LICH:
            missileName = DATA_COMPGEN(0x00660968, lichMissileName,
                                       "PLICH.def");
            break;
        case CREATURE_MEDUSA:
        case CREATURE_MEDUSA_QUEEN:
            missileName = DATA_COMPGEN(0x0066095c, medusaMissileName,
                                       "pmedusx.def");
            break;
        case CREATURE_ORC:
        case CREATURE_ORC_CHIEFTAIN:
            missileName = DATA_COMPGEN(0x00660950, orcMissileName,
                                       "porchx.def");
            break;
        case ARMY_CREATURE_CYCLOPS:
        case ARMY_CREATURE_CYCLOPS_KING:
            missileName = DATA_COMPGEN(0x00660944, cyclopsMissileName,
                                       "PCYCLBX.def");
            break;
        case CREATURE_LIZARDMAN:
        case CREATURE_LIZARD_WARRIOR:
            missileName = DATA_COMPGEN(0x00660938, lizardMissileName,
                                       "pplizax.def");
            break;
        case CREATURE_BALLISTA:
            missileName = DATA_COMPGEN(0x0066092c, ballistaMissileName,
                                       "SMBalx.def");
            break;
        case CREATURE_CATAPULT:
            missileName = DATA_COMPGEN(0x00660920, catapultMissileName,
                                       "SMCatx.def");
            break;
        case CREATURE_MAGE:
        case CREATURE_ARCH_MAGE:
        case CREATURE_BEHOLDER:
        case CREATURE_EVIL_EYE:
        case CREATURE_ENCHANTER:
            missileName = DATA_COMPGEN(0x00660914, mageMissileName,
                                       "pmagex.def");
            break;
        case CREATURE_ICE_ELEMENTAL:
            missileName = DATA_COMPGEN(0x00660908, iceMissileName,
                                       "PiceE.def");
            break;
        case CREATURE_TITAN:
        case CREATURE_STORM_ELEMENTAL:
            missileName = DATA_COMPGEN(0x006608fc, titanMissileName,
                                       "cprgtix.def");
            break;
        }
        CSprite* missile = ResourceManager::getSprite(missileName);
        if (m_missileIcon)
            m_missileIcon->dispose();
        m_missileIcon = missile;
    } else {
        if (m_missileIcon)
            m_missileIcon->dispose();
        m_missileIcon = 0;
    }
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:477
DC_ONLY(0x4424c, 0xCC)
void army::freeResources()
{
    // @stub
}

// E:\gamedcs\army.cpp:507
#endif  // @carcass

VA(0x0043df20, 0xDD)  // dc 0x44318
void army::setLuck(const hero* ownerHero, const armyGroup* ownerGroup,
                   const town* ownerTown, const hero* otherHero,
                   const armyGroup* otherGroup, int magicTerrain)
{
    int value = 0;
    if (magicTerrain != MAGIC_TERRAIN_CURSED_GROUND
        && (!ownerHero || !ownerHero->isWieldingArtifact(
            ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR))
        && (!otherHero || !otherHero->isWieldingArtifact(
            ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR))) {
        if (ownerGroup) {
            value = ownerGroup->getLuck(
                ownerHero, ownerTown, otherHero, otherGroup, 0, 0);
        }
        if (m_spellInfluence[51])
            value += m_luckBonus;
        if (m_spellInfluence[52])
            value -= m_luckPenalty;

        if (magicTerrain == MAGIC_TERRAIN_CLOVER_FIELD) {
            do {
                switch (m_monInfo.m_townType) {
                case TOWN_CASTLE:
                case TOWN_RAMPART:
                case TOWN_TOWER:
                case TOWN_INFERNO:
                case TOWN_NECROPOLIS:
                case TOWN_DUNGEON:
                    continue;
                case TOWN_STRONGHOLD:
                case TOWN_FORTRESS:
                case TOWN_CONFLUX:
                    value += 2;
                    break;
                }
            } while (0);
        }

        if (m_creatureType == CREATURE_HALFLING && value < 1)
            value = 1;
    }
    m_luck = value;
}

// Complete extends the DC signature with a seventh, byte-wide alignment gate:
VA(0x0043e000, 0x139)  // dc 0x443b4
void army::setMorale(const hero* ownerHero, const armyGroup* ownerGroup,
                     const town* ownerTown, const hero* otherHero,
                     const armyGroup* otherGroup, int magicTerrain,
                     unsigned char groupAlignments)
{
    int value = 0;
    if (magicTerrain != MAGIC_TERRAIN_CURSED_GROUND && !is(1u << 17)) {
        if (ownerGroup) {
            value = ownerGroup->getMorale(
                ownerHero, ownerTown, otherHero, otherGroup, 0,
                groupAlignments, 0);
        }
        if (m_spellInfluence[49])
            value += m_moraleBonus;
        if (m_spellInfluence[50])
            value -= m_moralePenalty;

        if (magicTerrain == MAGIC_TERRAIN_HOLY_GROUND) {
            do {
                switch (m_monInfo.m_townType) {
                case TOWN_CASTLE:
                case TOWN_RAMPART:
                case TOWN_TOWER:
                    ++value;
                    break;
                case TOWN_INFERNO:
                case TOWN_NECROPOLIS:
                case TOWN_DUNGEON:
                    --value;
                    break;
                case TOWN_STRONGHOLD:
                case TOWN_FORTRESS:
                case TOWN_CONFLUX:
                    continue;
                }
            } while (0);
        }
        do {
            if (magicTerrain == MAGIC_TERRAIN_EVIL_FOG) {
                switch (m_monInfo.m_townType) {
                case TOWN_CASTLE:
                case TOWN_RAMPART:
                case TOWN_TOWER:
                    --value;
                    break;
                case TOWN_INFERNO:
                case TOWN_NECROPOLIS:
                case TOWN_DUNGEON:
                    ++value;
                    break;
                case TOWN_STRONGHOLD:
                case TOWN_FORTRESS:
                case TOWN_CONFLUX:
                    continue;
                }
            }
        } while (0);

        if ((m_creatureType == CREATURE_MINOTAUR
             || m_creatureType == CREATURE_MINOTAUR_KING)
            && value < 1) {
            value = 1;
        }
        if (ownerHero
            && ownerHero->isWieldingArtifact(
                ARTIFACT_SPIRIT_OF_OPPRESSION)
            && value > 0) {
            value = 0;
        }
        if (otherHero
            && otherHero->isWieldingArtifact(
                ARTIFACT_SPIRIT_OF_OPPRESSION)
            && value > 0) {
            value = 0;
        }
    }
    m_morale = value;
}

// E:\gamedcs\army.cpp:596
// One stack onto the combat buffer: the MirrorImage slide pair joins
// the caller's point, a mid-step walk displacement (44/42 pixels per
// full walk cycle, halved on the diagonals, the vertical half
// published through giWalkingYMod), then - unless only the count box
// is wanted - the highlight color (hover 0x70, last-moved 0x60, area
// effect 0x70, read out of gSystemPalette), one of four palette tints
// under the /GX frame (the clone's rolling AdjustHSV over
// PaletteEffect, the petrify AdjustSaturation, the Stone-row Gray,
// the Bloodlust red AdjustHSV), DrawCreature, and the palette restore.
// The count box picks its side off facing and the neighbour hex's
// occupancy, clips against combatGridBitmap, colorizes 28x9 (a plain
// 0.75 hue, or the standing-spell AFFINITY hue - signed row sum over
// absolute row sum of akSpellTraits field_0, mapped to (x+1)/6), and
// prints the count in gpTinyFont. The pow overlay places powSprite by
// the effect row's flags nibble and draws it with bit 8 as the alpha.

// Retail recycles the spent x parameter twice past the numbox (the
// count ternary and the effect flags both home in [ebp+8]); count_text
// is char[12] (the frame is 0x458, and the affinity temp overlays it).

VA(0x0043e140, 0x8C0)  // anchor-global, dc 0x444a8
void army::drawToBuffer(int x, int y, int numBoxOnly)
{
    if (g_combatManager->m_battleOver != 0)
        return;
    if (static_cast<const combatManager*>(g_combatManager)
            ->isQuickCombat())
        return;

    y += m_ySpecialMod;
    x += m_xSpecialMod;
    if (m_currFrameType == cs_walk && !(is(1u << 1))) {
        long frames = m_stdIcon->getNumFrames(0);
        long stepY = m_currFrameIndex * 42 / frames;
        long stepX = m_currFrameIndex * 44 / frames;
        switch (m_walkDirection) {
        case COMBAT_DIRECTION_0:
            x += stepX / 2;
            y -= stepY;
            g_walkingYMod = -stepY;
            break;
        case COMBAT_DIRECTION_1:
            x += stepX;
            break;
        case COMBAT_DIRECTION_2:
            x += stepX / 2;
            y += stepY;
            g_walkingYMod = stepY;
            break;
        case COMBAT_DIRECTION_3:
            x -= stepX / 2;
            y += stepY;
            g_walkingYMod = stepY;
            break;
        case COMBAT_DIRECTION_4:
            x -= stepX;
            break;
        case COMBAT_DIRECTION_5:
            x -= stepX / 2;
            y -= stepY;
            g_walkingYMod = -stepY;
            break;
        case COMBAT_DIRECTION_WIDE_UPPER:
            y -= stepY;
            g_walkingYMod = -stepY;
            break;
        case COMBAT_DIRECTION_WIDE_LOWER:
            y += stepY;
            g_walkingYMod = stepY;
            break;
        }
    }

    if (numBoxOnly == 0) {
        long highlight = 0;
        if (g_combatManager->m_highlighterOn != 0
            && m_gridIndex == g_combatManager->m_highlighterIndex)
            highlight = 0x70;
        if (g_combatManager->m_lastMovedArmy == this)
            highlight = 0x60;
        if (m_isAreaEffectTarget)
            highlight = 0x70;

        TPalette16 saved;
        unsigned char restore = 0;
        if (is(1u << 29)) {
            memcpy(saved.m_data, m_stdIcon->getPalette(), 0x200);
            TPalette16 tinted(m_stdIcon->getPalette());
            tinted.adjustHSV(0, m_paletteEffect, m_paletteEffect + 1.0f,
                             m_paletteEffect + 1.0f);
            memcpy(m_stdIcon->getPalette(), tinted.m_data, 0x200);
            restore = 1;
        } else if (is(1u << 30)) {
            memcpy(saved.m_data, m_stdIcon->getPalette(), 0x200);
            TPalette16 tinted(m_stdIcon->getPalette());
            tinted.adjustSaturation(m_paletteEffect);
            memcpy(m_stdIcon->getPalette(), tinted.m_data, 0x200);
            restore = 1;
        } else if (m_spellInfluence[SPELL_STONE] > 0) {
            memcpy(saved.m_data, m_stdIcon->getPalette(), 0x200);
            TPalette16 tinted(m_stdIcon->getPalette());
            tinted.gray();
            memcpy(m_stdIcon->getPalette(), tinted.m_data, 0x200);
            restore = 1;
        } else if (is(1u << 23)) {
            memcpy(saved.m_data, m_stdIcon->getPalette(), 0x200);
            TPalette16 tinted(m_stdIcon->getPalette());
            tinted.adjustHSV(0.67f, 1.0f, 2.0f, 2.0f);
            memcpy(m_stdIcon->getPalette(), tinted.m_data, 0x200);
            restore = 1;
        }

        long drawX =
            m_facing == 0 ? x - m_stdIcon->getWidth() + 196 : x - 196;
        long drawY = y - 267;
        g_combatManager->drawCreature(
            m_stdIcon, m_currFrameType, m_currFrameIndex, drawX, drawY, 0,
            m_gridIndex, m_facing == 0, g_systemPalette->m_data[highlight]);

        if (restore)
            memcpy(m_stdIcon->getPalette(), saved.m_data, 0x200);
    }

    if (g_combatManager->m_computeExtentOnly != 0
        || (!(is(1u << 21)) && !(is(1u << 6)) && !m_isMoving
            && (m_currFrameType == cs_wait
                || m_currFrameType == cs_fidget))) {
        long step = 1;
        long xoff;
        long yoff;
        if (m_facing == 0) {
            xoff = 0x34;
            yoff = -0x1e;
        } else {
            xoff = 0x16;
            yoff = -0xf;
        }
        if (m_monInfo.m_attributes & 1) {
            xoff += 0x2c;
            step = 2;
        }
        if (m_facing == 0)
            step = -step;
        if ((g_combatManager->m_cells[m_gridIndex + step].m_armySide >= 0
             && !g_combatManager->m_cells[m_gridIndex + step]
                     .getArmy()
                     ->m_isMoving)
            || (g_combatManager->m_cells[m_gridIndex + step].m_attributes & 2)) {
            xoff -= 0x25;
            yoff = -0xf;
        } else {
            xoff += m_monFrameInfo.m_extraNumTroopsXOffset;
        }
        if (m_facing == 0)
            xoff = -xoff;
        long numboxX = x + xoff;
        long numboxY = y + yoff;
        if (g_combatManager->drawObject(
                g_combatManager->m_combatGridBitmap, numboxX, numboxY)) {
            if (m_numSpellInfluences == 0) {
                g_windowManager->m_screenBitmap->colorize(
                    numboxX + 1, numboxY + 1, 0x1c, 9, 0.75f, 0.8f);
            } else {
                long sum = 0;
                long absSum = 0;
                for (long i = 0; i < 0x51; i++) {
                    if (m_spellInfluence[i] != 0) {
                        sum += g_spellTraits[i].m_karma;
                        absSum += abs(g_spellTraits[i].m_karma);
                    }
                }
                double affinity;
                if (absSum == 0)
                    affinity = 0.0;
                else
                    affinity = sum / static_cast<double>(absSum);
                g_windowManager->m_screenBitmap->colorize(
                    numboxX + 1, numboxY + 1, 0x1c, 9,
                    static_cast<float>((affinity + 1.0) * 0.1667f),
                    0.8f);
            }
            char countText[12];
            // The count recycles the spent x parameter (retail homes
            // the ternary's result in [ebp+8]).
            x = m_numTroopsToShowOverride == -1 ? m_numTroops
                                              : m_numTroopsToShowOverride;
            sprintf(countText,
                    DATA_COMPGEN(0x00660a1c, decimalFormat, "%d"), x);
            g_tinyFont->drawBoundedString(
                countText, g_windowManager->m_screenBitmap, numboxX,
                numboxY, 0x1e, 0xf, font::WHITE, 1, -1);
        }
    }

    if (m_showPowEffect != 0 && numBoxOnly == 0) {
        if (g_combatManager->m_powFrameIndex
            < g_combatManager->m_powSprite->getNumFrames(0)) {
            // The effect flags also recycle x ([ebp+8] again, this
            // time read back unsigned for the >>8 below).
            x = g_spellEffectTraits[g_combatManager->m_powSpellEffect]
                    .m_flags;
            long ex;
            long ey;
            switch (x & 0xf) {
            case SPELL_EFFECT_PLACE_OVERHEAD:
                ex = g_combatManager->m_cells[m_gridIndex].m_refX;
                if (m_monInfo.m_attributes & 1)
                    ex += m_facing ? 22 : -22;
                ex -= g_combatManager->m_powSprite->getWidth() / 2;
                ey = g_combatManager->m_cells[m_gridIndex].m_refY
                     - g_combatManager->m_powSprite->getHeight();
                break;
            case SPELL_EFFECT_PLACE_CENTERED:
                ex = g_combatManager->m_cells[m_gridIndex].m_refX;
                if (m_monInfo.m_attributes & 1)
                    ex += m_facing ? 22 : -22;
                ex -= g_combatManager->m_powSprite->getWidth() / 2;
                ey = g_combatManager->m_cells[m_gridIndex].m_refY
                     - g_combatManager->m_powSprite->getHeight() / 2
                     - m_imageHeight / 2;
                break;
            case SPELL_EFFECT_PLACE_ABOVE:
                ex = g_combatManager->m_cells[m_gridIndex].m_refX;
                if (m_monInfo.m_attributes & 1)
                    ex += m_facing ? 22 : -22;
                ex -= g_combatManager->m_powSprite->getWidth() / 2;
                ey = g_combatManager->m_cells[m_gridIndex].m_refY
                     - g_combatManager->m_powSprite->getHeight()
                     - m_imageHeight;
                break;
            case SPELL_EFFECT_PLACE_FLANK: {
                long edge = m_stdIcon->getFrame(cs_wait, 0)->getCroppedX()
                            + m_stdIcon->getFrame(cs_wait, 0)->getCroppedWidth()
                            - 196;
                if (m_facing == 0)
                    ex = g_combatManager->m_cells[m_gridIndex].m_refX
                         - edge;
                else
                    ex = g_combatManager->m_cells[m_gridIndex].m_refX
                         + edge;
                if (m_facing == 0)
                    ex -= g_combatManager->m_powSprite->getWidth();
                ey = g_combatManager->m_cells[m_gridIndex].m_refY
                     - g_combatManager->m_powSprite->getHeight() / 2
                     - m_imageHeight / 2;
                break;
            }
            default:
                // Faithful artifact: the fallback aims BOTH coordinates
                // at the recycled x slot (retail reads [ebp+8] twice -
                // the second read is not y).
                ex = x;
                ey = x;
                break;
            }
            g_combatManager->drawSpellEffect(
                g_combatManager->m_powSprite,
                g_combatManager->m_powFrameIndex, ex, ey, m_facing == 0,
                (static_cast<unsigned long>(x) >> 8) & 1);
        }
    }
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:891
DC_ONLY(0x44d50, 0xC2)
double army::computeKarma() const
{
    // @stub
}

#endif  // @carcass

// Append `arg` to `array` unless it is already there, answering whether
// it went in.

// STATIC, and the carve is what says so: erase_item below has a retail
// out-of-line body at 0x43ea00 and this one has NO retail slot anywhere
// - the whole 0x43e140..0x43ea70 gap holds exactly that one 103-byte
// function - so the /Ob2 rule that emits an unconditional out-of-line
// copy for every EXTERN function it inlines rules external linkage out.
// Its two call sites are both inside add_aura, twice each, and every
// one of them is expanded.

// The membership test is std::find, not a hand-rolled walk: the DC
// build's army.obj carries an `army**` instantiation of it as a COMDAT
// (the roster's std::find rows at the foot of army.h), and retail's
// expansion is find's own rotated shape - the empty-range test, the
// value compare, the increment, then ONE compare of the answer against
// end().
// E:\gamedcs\army.cpp:917
DC_ONLY(0x44e14, 0xAC)
static unsigned char addItem(std::vector<army*>& array, army* arg)
{
    if (std::find(array.begin(), array.end(), arg) != array.end())
        return 0;
    array.push_back(arg);
    return 1;
}

// Drop the first occurrence of `arg` from `array`, searching BACKWARDS.

VA(0x0043ea00, 0x67)  // dc 0x44ec0
void eraseItem(std::vector<army*>& array, const army* arg)
{
    unsigned i = array.size();
    while (i-- != 0) {
        if (array[i] == arg) {
            array.erase(array.begin() + i);
            return;
        }
    }
}

// Wire this stack into the unicorn aura in both directions: every
// adjacent unicorn on its own side gains it as a client, and if this
// stack IS a unicorn every adjacent ally of its own becomes one.

// Is(1u << 21) is the death bit ProcessDeath raises (0x200000); a corpse
// neither gives nor takes an aura.
VA(0x0043ea70, 0x1DD)  // dc 0x44f0c
void army::addAura()
{
    long count;
    if (is(1u << 0))
        count = 8;
    else
        count = 6;

    while (count-- > 0) {
        long hex = getAdjacentHex(m_gridIndex, count);
        if (!g_combatManager->validHex(hex))
            continue;
        army* other = g_combatManager->m_cells[hex].getArmy();
        if (!other)
            continue;
        if ((other->m_creatureType == ARMY_CREATURE_UNICORN
             || other->m_creatureType == ARMY_CREATURE_WAR_UNICORN)
            && other->getControllingSide() == getOwningSide()
            && !(other->is(1u << 21))) {
            addItem(other->m_auraClients, this);
            addItem(m_auraSources, other);
        }
        if ((m_creatureType == ARMY_CREATURE_UNICORN
             || m_creatureType == ARMY_CREATURE_WAR_UNICORN)
            && other->getOwningSide() == getControllingSide()
            && !(other->is(1u << 21))) {
            addItem(other->m_auraSources, this);
            addItem(m_auraClients, other);
        }
    }
}

// Tear this stack out of both halves of the aura relationship: every
// stack whose aura reaches it, and every stack its own aura reaches.

// erase_item is inlined at both sites, but NOT to the same depth: the
// first site keeps `copy` and `_Destroy` as calls where the second
// gets the copy loop written out inline. That is the /Ob2 divisor
// doing its job (budget / sites-still-to-come grows as the sites are
// spent), not a source difference - both sites are the same call.
VA(0x0043ec50, 0x1B4)  // dc 0x45000
void army::removeAura()
{
    long i = m_auraSources.size();
    while (i-- > 0) {
        eraseItem(m_auraSources[i]->m_auraClients, this);
    }
    m_auraSources.clear();

    long j = m_auraClients.size();
    while (j-- > 0) {
        eraseItem(m_auraClients[j]->m_auraSources, this);
    }
    m_auraClients.clear();
}

// The same teardown for the BIND relationship, plus the one rule that
// makes it different: a stack stops being bound the moment the last
// binder lets go, so each army this one had bound gets its bind row
// cancelled once its `binders` list comes up empty.

VA(0x0043ee10, 0x1C2)  // dc 0x450bc
void army::removeBinding()
{
    long i = m_boundArmies.size();
    while (i-- > 0) {
        eraseItem(m_boundArmies[i]->m_binders, this);
        if (m_boundArmies[i]->m_binders.size() == 0)
            m_boundArmies[i]->cancelIndividualSpell(SPELL_BIND);
    }
    m_boundArmies.clear();

    long j = m_binders.size();
    while (j-- > 0) {
        eraseItem(m_binders[j]->m_boundArmies, this);
    }
    m_binders.clear();
}

// Raise or lower the area-effect latch and re-pose the stack. The
// answer is "did the latch actually change", which is why the
// no-op arm returns 0 before touching anything else.

// MarkCreatureEffect expands here for the second time in this TU (the
// first is do_multi_head_attack's) and carries the same arrow-tower
// arm, with `combatSide` on the 21-wide army stride and the 20-wide
// byte row.
VA(0x0043efe0, 0xCF)
unsigned char army::setInsideAreaEffect(unsigned char arg)
{
    if (m_isAreaEffectTarget == arg)
        return 0;
    m_isAreaEffectTarget = arg;
    g_combatManager->markCreatureEffect(m_combatSide, m_bitIndex);
    if (m_isAreaEffectTarget) {
        if (m_stdIcon->isValidSeq(cs_fidget)
            && m_currFrameType != cs_fidget) {
            m_currFrameType = cs_fidget;
            m_currFrameIndex = 0;
        } else if (m_currFrameType != cs_wait) {
            m_currFrameType = cs_wait;
            m_currFrameIndex = 0;
        }
    }
    return 1;
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:1062
// NO RETAIL BODY, AND THE BRACKET PROVES IT. The carve has exactly ONE
// row between remove_bindings (0x43ee10, 0x1C2) and Walk (0x43f0b0) -
// 0x43efe0 / 207 B - and the Dreamcast roster has TWO here, so one of
// the pair has no retail slot. 0x43efe0 is set_inside_area_effect: it
// takes ONE byte argument, compares it against +0x4f1
// (is_area_effect_target), stores it, and ends `ret 4`, which
// EndWalk() - no arguments - cannot. Retail therefore emits no
// out-of-line EndWalk anywhere, and its four statements are written at
// Walk's `if (end_walk)` instead. The shape below is byte-exact for
// this call site; whether retail's source kept the name as a header
// inline or spelled the statements out is NOT decided here.
DC_ONLY(0x45204, 0x50)
void army::endWalk()
{
    // @stub
}

#endif  // @carcass

// One hex of a walk: turn to face the step if it needs turning, publish
// the from/to pair the redraw reads, play the walk animation, and move
// the stack in the grid.

VA(0x0043f0b0, 0x206)  // dc 0x45254
void army::walk(int direction, unsigned char endWalk,
                unsigned char initialWalk)
{
    if (initialWalk)
        setupAnimation();
    if (needToTurn(direction))
        turn(initialWalk);
    int nextCell = getAdjacentCellIndex(m_gridIndex, direction);
    g_walkingFrom = m_gridIndex;
    g_walkingTo = nextCell;
    g_walkingYMod = 0;
    if (is(1u << 0)) {
        g_walkingFrom2 = m_gridIndex + offsetToFront(-1);
        g_walkingTo2 = nextCell + offsetToFront(-1);
    } else {
        g_walkingFrom2 = -1;
        g_walkingTo2 = -1;
    }
    m_walkDirection = direction;
    if (direction == COMBAT_DIRECTION_0 || direction == COMBAT_DIRECTION_5)
        m_drawPriority = 3;
    if (direction == COMBAT_DIRECTION_2 || direction == COMBAT_DIRECTION_3)
        m_drawPriority = 7;
    if (!g_combatManager->isQuickCombat()) {
        if (initialWalk) {
            playSample(PRE_WALK_SAMPLE);
            playAnimation(20, -1, 0);
            playSample(WALK_SAMPLE);
        }
        playAnimation(0, -1, 0);
    }
    g_combatManager->removeArmyFromGrid(*this);
    m_gridIndex = nextCell;
    g_combatManager->placeArmyInGrid(*this, nextCell);
    g_walkingFrom = -1;
    g_walkingFrom2 = -1;
    g_walkingTo = -1;
    g_walkingTo2 = -1;
    m_drawPriority = 4;
    if (endWalk) {
        if (!g_combatManager->isQuickCombat()) {
            playSample(POST_WALK_SAMPLE);
            if (m_armySample[WALK_SAMPLE])
                g_soundManager->stopSample(m_armySample[WALK_SAMPLE]->m_memSample.m_memSampleHandle);
            playAnimation(21, -1, 0);
            playAnimation(2, 1, 0);
        }
    }
}

// E:\gamedcs\army.cpp:1171
// One missile, from muzzle flash to impact: aim at the target stack's
// hex (the two-hex center shift), play the ranged pose the
// GetMissileStartingPosition search picks, then fly it - the Enchanter
// resolves through spells.obj's 0x59fde0 instead, the Is(1u << 11) shooters
// throw a DoBolt lightning (Arch Mage green, the eye/psychic family
// violet), and everyone else gets the pixel flight ShootBallisticMissile
// also uses: a Bitmap16Bit backing store grabbed and restored per step,
// the update rect seeded from gCombatAreaLimits, clamped to
// gCombatDrawLimits694f18, stepped dx/nframes at a
// gCombatSpeedFactors-scaled 33ms beat. The /GX frame covers `saved`.
// Residual (93.1695%): a two-slot rotation and its ripple - retail
// homes startY at -0x30 and targetY at -0x34 (missile_frame -0x58)
// where ours swaps the pair, and the GetMissileStartingPosition
// argument schedule moves with it; a declaration reorder is
// byte-inert. The EH-prologue push, the combatSpeed reloc addend and
// the bolt-table displacements are the documented masked cosmetics
// (PlayAnimation, EXACT, carries the identical combatSpeed read).
// DC type/source audit (2026-08-21): its destX, total-frame count and
// per-frame addX/addY locals are `int`, and ARROW_PERIOD is `const int`.
// Restoring those types, plus the symmetric targetY type, is byte-flat at
// 93.169495%. Conventional release VERIFY is also byte-flat both at entry
// (`armyToAttack != 0`) and immediately before the rotated argument set
// (`missileIcon != 0`). The slot swap is therefore not a missing invariant.
VA(0x0043f2c0, 0x63B)  // anchor-bracket, dc 0x453c8
void army::animateMissile(army* armyToAttack)
{
    if (static_cast<const combatManager*>(g_combatManager)
            ->isQuickCombat())
        return;

    int targetX = g_combatManager->m_cells[armyToAttack->m_gridIndex]
                      .m_refX;
    if (armyToAttack->m_monInfo.m_attributes & 1)
        targetX += armyToAttack->m_facing ? 22 : -22;
    int targetY = g_combatManager->m_cells[armyToAttack->m_gridIndex]
                      .m_refY
                  - armyToAttack->m_imageHeight / 2;
    g_combatManager->resetLimitCreature();
    g_combatManager->markCreatureEffect(m_combatSide, m_bitIndex);
    g_combatManager->computeMaxExtent();

    int startX;
    int startY;
    int missileFrame;
    getMissileStartingPosition(m_creatureType,
                               g_combatManager->m_cells[m_gridIndex].m_refX,
                               g_combatManager->m_cells[m_gridIndex].m_refY,
                               m_facing, targetX, targetY, m_missileIcon,
                               &startX, &startY, &m_currFrameType,
                               &missileFrame);
    m_currFrameIndex = 0;
    playSample(SHOOT_SAMPLE);

    int frames;
    if (m_monFrameInfo.m_attackFrames > 0)
        frames = m_monFrameInfo.m_attackFrames;
    else
        frames = m_stdIcon->getNumFrames(m_currFrameType);
    long delay = m_monFrameInfo.m_attackStartCycleTime / frames;
    for (m_currFrameIndex = 0; m_currFrameIndex < frames; m_currFrameIndex++) {
        g_combatManager->drawFrame(1, 1, 0, delay, 1, 1);
    }
    m_currFrameIndex--;

    long deltaX = targetX - startX;
    long deltaY = targetY - startY;
    int arrowtraveldist = static_cast<int>(sqrt(
        static_cast<double>(deltaY * deltaY + deltaX * deltaX)));

    if (m_creatureType == ARMY_CREATURE_ENCHANTER) {
        g_combatManager->unnamed59FDE0(startX, startY, armyToAttack);
        return;
    }
    if (is(1u << 11)) {
        GameTime::delay(static_cast<long>(
            g_combatSpeedFactors[g_unnamed698758.m_combatSpeed] * 115.0f));
        long color;
        switch (m_creatureType) {
        case CREATURE_ARCH_MAGE:
            color = BOLT_COLOR_2;
            break;
        case CREATURE_BEHOLDER:
        case CREATURE_EVIL_EYE:
        case CREATURE_PSYCHIC_ELEMENTAL:
        case CREATURE_MAGIC_ELEMENTAL:
            color = BOLT_COLOR_4;
            break;
        default:
            color = 0;
            break;
        }
        g_combatManager->doBolt(1, startX, startY, targetX, targetY, 0, 0,
                                5, 4, color, 0, 0,
                                arrowtraveldist / 15 + 15, 1, 0, 10,
                                0);
        return;
    }

    int nframes = (arrowtraveldist + 20) / 20;
    int stepX;
    int stepY;
    if (nframes > 0) {
        stepX = deltaX / nframes;
        stepY = deltaY / nframes;
    } else {
        stepX = deltaX;
        stepY = deltaY;
    }
    int width = m_missileIcon->getWidth();
    int height = m_missileIcon->getHeight();
    int x = startX - width / 2;
    int y = startY - height / 2;

    Bitmap16Bit saved(width, height);
    TDrawbridgeBounds updateArea = g_combatAreaLimits;
    const int missileperiod = static_cast<int>(
        g_combatSpeedFactors[g_unnamed698758.m_combatSpeed] * 33.0f);

    long frame = 0;
    if (nframes > 0) {
        long right = x + width - 1;
        long bottom = y + height - 1;
        for (; frame < nframes; frame++) {
            unsigned long nextFrameTime =
                GameTime::get() + missileperiod;
            if (frame != 0) {
                saved.draw(0, 0, width, height,
                           g_windowManager->m_screenBitmap->getMap(0, 0), x, y,
                           g_windowManager->m_screenBitmap->getWidth(),
                           g_windowManager->m_screenBitmap->getHeight(),
                           g_windowManager->m_screenBitmap->getPitch(), false);
                updateArea.m_minX = x;
                updateArea.m_minY = y;
                updateArea.m_maxX = right;
                updateArea.m_maxY = bottom;
                x += stepX;
                y += stepY;
                right += stepX;
                bottom += stepY;
            }
            saved.grab(g_windowManager->m_screenBitmap->getMap(0, 0), x, y,
                       g_windowManager->m_screenBitmap->getWidth(),
                       g_windowManager->m_screenBitmap->getHeight(),
                       g_windowManager->m_screenBitmap->getPitch());
            bool flipped = targetX < startX;
            m_missileIcon->draw(0, missileFrame, 0, 0, width, height,
                              g_windowManager->m_screenBitmap->getMap(0, 0), x, y,
                              g_windowManager->m_screenBitmap->getWidth(),
                              g_windowManager->m_screenBitmap->getHeight(),
                              g_windowManager->m_screenBitmap->getPitch(),
                              flipped, 1);
            if (updateArea.m_minX > x)
                updateArea.m_minX = x;
            if (updateArea.m_minY > y)
                updateArea.m_minY = y;
            if (updateArea.m_maxX < right)
                updateArea.m_maxX = right;
            if (updateArea.m_maxY < bottom)
                updateArea.m_maxY = bottom;
            if (updateArea.m_minX < g_combatDrawLimits694f18.m_minX)
                updateArea.m_minX = g_combatDrawLimits694f18.m_minX;
            if (updateArea.m_minY < g_combatDrawLimits694f18.m_minY)
                updateArea.m_minY = g_combatDrawLimits694f18.m_minY;
            if (updateArea.m_maxX > g_combatDrawLimits694f18.m_maxX)
                updateArea.m_maxX = g_combatDrawLimits694f18.m_maxX;
            if (updateArea.m_maxY > g_combatDrawLimits694f18.m_maxY)
                updateArea.m_maxY = g_combatDrawLimits694f18.m_maxY;
            g_windowManager->updateScreen(
                updateArea.m_minX, updateArea.m_minY,
                updateArea.m_maxX - updateArea.m_minX + 1,
                updateArea.m_maxY - updateArea.m_minY + 1);
            GameTime::delayTil(nextFrameTime);
        }
    }
    saved.draw(0, 0, width, height, g_windowManager->m_screenBitmap->getMap(0, 0),
               x, y, g_windowManager->m_screenBitmap->getWidth(),
               g_windowManager->m_screenBitmap->getHeight(),
               g_windowManager->m_screenBitmap->getPitch(), false);
    g_windowManager->updateScreen(x, y, width, height);
}

// E:\gamedcs\army.cpp:1356
// One landed volley. The luck preamble is do_attack's statement for
// statement (same pinned get_controlling_side, same
// TTextResource::operator[](46)
// message); then the missile flies (animate_missile, still a carcass
// callee), the ammo counts down unless the owner wields the Ammo Cart,
// and the damage routes three ways: the Magog's fireball and the two
// Liches' death cloud each play their pseudo-spell's effect sprite
// over the target hex and splash every stack on the seven hexes
// (centre + six neighbours, GetAdjacentCellIndexNoArmy), where the
// cloud only touches the undead on the OUTER six; everything else is
// one adjust_damage/Damage pair on the target alone.

// FAITHFUL ARTIFACTS, all three transcribed from the bytes - do not
// repair: the splash arms read `a->creatureType` (magog, the
// first/multiple bookkeeping) and `a->Is(1u << 4)` (lich, the undead filter)
// BEFORE any null test; and the per-hex `dmg`/`killedNow` pair is only
// assigned under `if (a)`, so an armyless hex adds the PREVIOUS
// iteration's values into the totals.

// What is genuinely left is the first shape, and the frame still says so
// (0x24 against retail's 0x20).

VA(0x0043f900, 0x7F9)  // dc-bracket forced, dc 0x458a0
void army::rangeAttack(army* armyToAttack)
{
    checkLuck();
    animateMissile(armyToAttack);
    if (!getOwner()
        || !getOwner()->isWieldingArtifact(ARTIFACT_AMMO_CART))
        m_monInfo.m_numShots--;
    if (m_creatureType == CREATURE_MAGOG) {
        long effect = g_spellTraits[SPELL_FIREBALL].m_effect;
        if (effect != -1
            && !static_cast<const combatManager*>(g_combatManager)
                    ->isQuickCombat()) {
            launchSample(g_spellTraits[SPELL_FIREBALL].m_sample, -1, 3);
            CSprite* spr =
                ResourceManager::getSprite(g_spellEffectTraits[effect]
                                               .m_name);
            long x = armyToAttack->midX() - spr->getWidth() / 2;
            long y = armyToAttack->midY() - spr->getHeight() / 2;
            g_combatManager->scrollTo(
                x + spr->getWidth() / 2,
                y + spr->getHeight() / 2, 1, 1, 1);
            for (long frame = 0; frame < spr->getNumFrames(0); frame++) {
                g_combatManager->drawFrame(0, 0, 0, 50, 1, 1);
                g_combatManager->drawSpellEffect(spr, frame, x, y, 0, 0);
                g_combatManager->updateCombatArea();
            }
            g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
            spr->dispose();
        }
        g_combatManager->clearEffects();
        long killed = 0;
        long damage = 0;
        army* first = 0;
        unsigned char multiple = 0;
        int dmg;
        int killedNow;
        for (long i = 0; i < 7; i++) {
            long hex;
            if (i == COMBAT_DIRECTION_COUNT)
                hex = m_pathTarget;
            else
                hex = getAdjacentCellIndexNoArmy(m_pathTarget, i);
            hexcell* cell = &g_combatManager->m_cells[hex];
            if (!g_combatManager->validHex(hex) || !cell->hasArmy())
                continue;
            // SLOT IS DECLARED FIRST, in both of this body's two scan
            // loops: retail's `movsx edx,byte [ecx+0x1b]` for the slot lands
            // AHEAD of the `movsx ebx,al` that reuses the already-tested
            // armySide byte.  Worth +2.08 here and +2.32 in the Lich loop
            // below.
            long slot = cell->m_armySlot;
            long side = cell->m_armySide;
            army* a = cell->getArmy();
            if (g_combatManager->m_effected[side][slot] != 0)
                continue;
            g_combatManager->m_effected[side][slot] = 1;
            if (first == 0)
                first = a;
            else if (first->m_creatureType != a->m_creatureType)
                multiple = 1;
            damageEnemy(a, &dmg, &killedNow, 1);
            damage += dmg;
            killed += killedNow;
        }
        if (damage > 0) {
            army* reported = first;
            if (multiple)
                reported = 0;
            g_combatManager->damageMessage(getName(),
                                            m_numTroops, damage, reported,
                                            killed);
            g_combatManager->powEffect(effect, 1);
        }
    } else if (m_creatureType == CREATURE_LICH
               || m_creatureType == CREATURE_POWER_LICH) {
        long effect = g_spellTraits[SPELL_DEATH_CLOUD].m_effect;
        if (effect != -1
            && !static_cast<const combatManager*>(g_combatManager)
                    ->isQuickCombat()) {
            launchSample(g_spellTraits[SPELL_DEATH_CLOUD].m_sample, -1,
                          3);
            CSprite* spr =
                ResourceManager::getSprite(g_spellEffectTraits[effect]
                                               .m_name);
            long x = armyToAttack->midX() - spr->getWidth() / 2;
            long y = armyToAttack->midY() - spr->getHeight() / 2;
            for (long frame = 0; frame < spr->getNumFrames(0); frame++) {
                g_combatManager->drawFrame(0, 0, 0, 100, 1, 1);
                g_combatManager->drawSpellEffect(spr, frame, x, y, 0, 0);
                g_combatManager->updateCombatArea();
            }
            g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
            spr->dispose();
        }
        g_combatManager->clearEffects();
        long killed = 0;
        long damage = 0;
        army* first = 0;
        unsigned char multiple = 0;
        int dmg;
        int killedNow;
        for (long i = 0; i < 7; i++) {
            long hex;
            if (i == COMBAT_DIRECTION_COUNT)
                hex = m_pathTarget;
            else
                hex = getAdjacentCellIndexNoArmy(m_pathTarget, i);
            hexcell* cell = &g_combatManager->m_cells[hex];
            if (!g_combatManager->validHex(hex) || !cell->hasArmy())
                continue;
            long slot = cell->m_armySlot;
            long side = cell->m_armySide;
            army* a = cell->getArmy();
            if (i != COMBAT_DIRECTION_COUNT && !(a->is(1u << 4)))
                continue;
            if (g_combatManager->m_effected[side][slot] != 0)
                continue;
            g_combatManager->m_effected[side][slot] = 1;
            damageEnemy(a, &dmg, &killedNow, 1);
            damage += dmg;
            killed += killedNow;
            if (first == 0)
                first = a;
            else if (first->m_creatureType != a->m_creatureType)
                multiple = 1;
        }
        if (damage > 0) {
            if (multiple)
                first = 0;
            g_combatManager->damageMessage(getName(),
                                            m_numTroops, damage, first,
                                            killed);
            g_combatManager->powEffect(-1, 1);
        }
    } else {
        int killed;
        int damage;
        damageEnemy(armyToAttack, &damage, &killed, 1);
        g_combatManager->powEffect(-1, 0);
        g_combatManager->damageMessage(getName(), m_numTroops, damage,
                                        armyToAttack, killed);
        if (static_cast<const combatManager*>(g_combatManager)
                ->isQuickCombat())
            return;
        waitSample(SHOOT_SAMPLE);
    }
}

// One stack's whole shooting turn: resolve the target it was told to
// attack, turn to face it, fire between one and three volleys, turn
// back, and drop the spells an attack cancels.

VA(0x00440160, 0x1A6)  // dc 0x45e70
void army::rangeAttack()
{
    m_yModify = 0;
    if (m_side < 0)
        return;
    if (m_slot < 0)
        return;
    army* target = &g_combatManager->m_armies[m_side][m_slot];
    if (m_creatureType == ARMY_CREATURE_ARROW_TOWER) {
        g_combatManager->keepAttack(m_slot);
        return;
    }
    long targetX = g_combatManager->m_cells[target->m_gridIndex].m_refX;
    long ourX = g_combatManager->m_cells[getSecondGridIndex()].m_refX;
    long oldFacing = m_facing;
    if (targetX > ourX && m_facing != FACING_DEFENDER) {
        setupAnimation();
        turn(1);
    }
    if (targetX < ourX && m_facing != FACING_ATTACKER) {
        setupAnimation();
        turn(1);
    }
    rangeAttack(target);
    if ((is(1u << 15)) && target->m_numTroops > 0)
        rangeAttack(target);
    if (m_creatureType == ARMY_CREATURE_BALLISTA && target->m_numTroops > 0
        && getController() && getController()->m_skillLevel[eSecSkillBattlefieldBallistics] > 1) {
        rangeAttack(target);
    }
    if (m_facing != oldFacing) {
        setupAnimation();
        turn(1);
    }
    cancelSpellType(ARMY_CANCEL_SPELLS_AFTER_ATTACK);
}

// E:\gamedcs\army.cpp:1629 / 1643. DC has both out-of-line (0x45fc0,
// 0x46008, 70 B each); retail has NEITHER, so they are `inline` here
// and every use is an expansion. One hex step around the combat ring:
// a one-hex stack walks its six neighbours with +-1 modulo 6, a
// two-hex stack has eight and they are not in ring order, so it goes
// through the index/order table pair. Only get_multi_head_directions
// expands them so far, and it expands each exactly once.
inline long army::getClockwise(long direction) const
{
    if (is(1u << 0))
        return g_wideDirectionRingOrder[
            (g_wideDirectionRingIndex[direction] + 1) % 8];
    return (direction + 1) % COMBAT_DIRECTION_COUNT;
}

inline long army::getCounterClockwise(long direction) const
{
    if (is(1u << 0))
        return g_wideDirectionRingOrder[
            (g_wideDirectionRingIndex[direction] + 7) % 8];
    return (direction + 5) % COMBAT_DIRECTION_COUNT;
}

// THE MASK IS A SKIP MASK, not a hit mask: `test attackMask, 1 << i`
// jumps to the increment when the bit is SET. The two accumulator
// pointers are read-modify-written, never initialised, so the caller
// owns them across however many sweeps a turn makes.

VA(0x00440310, 0x1EB)  // dc 0x46050
void army::doMultiHeadAttack(unsigned attackMask, int* damageAmount, int* killed,
                                long* fireDamage)
{
    army* firstTarget = 0;
    int tempDamage;
    int tempKilled;
    unsigned char mixedTypes = 0;
    for (int i = 0; i < 8; i++) {
        if (attackMask & (1 << i))
            continue;
        long hex = getAdjacentHex(m_gridIndex, i);
        if (!g_combatManager->validHex(hex))
            continue;
        army* target = g_combatManager->m_cells[hex].getArmy();
        if (!target || target->m_hitByCreature)
            continue;
        long tempFire = damageEnemy(target, &tempDamage, &tempKilled, 0);
        if (tempFire > 0) {
            target->m_showFireShield = 1;
            *fireDamage += tempFire;
        }
        *damageAmount += tempDamage;
        *killed += tempKilled;
        g_combatManager->markCreatureEffect(target->m_combatSide,
                                            target->m_bitIndex);
        target->m_hitByCreature = 1;
        if (!firstTarget || firstTarget->m_creatureType == target->m_creatureType)
            firstTarget = target;
        else
            mixedTypes = 1;
    }
    if (mixedTypes)
        firstTarget = 0;
    g_combatManager->damageMessage(getName(), m_numTroops,
                                    *damageAmount, firstTarget, *killed);
}

// The on-attack debuff roll: does THIS attacker's special land on
// `target`? The switch is over the ATTACKER's creatureType (source
// order = layout order for the jump table), each arm rolls its chance,
// runs SpellCastWorks for the real spells, and parks the landed effect
// in the target's iPostPowSpellToCast for the post-animation pow to
// apply. Returns 1 only for the three incapacitators - blind, stone,
// paralyze - which is what suppresses the retaliation.

// The dendroid arm is TWO add_item calls (the static above, inlined
// whole with its std::find and its push_back's insert COMDAT), the
// first one guarded: an already-bound target returns without
// re-raising the pending spell or the mirror link.

VA(0x00440500, 0x4B4)  // dc 0x461a0
unsigned char army::checkSpecialAttack(army* target)
{
    switch (m_creatureType) {
    case CREATURE_GHOST_DRAGON:
        if (target->is(1u << 4) && random(1, 100) <= 20
            && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_AGE,
                                               getControllingSide(),
                                               target, 1, 1))
            target->m_postPowSpellToCast = SPELL_AGE;
        return 0;
    case CREATURE_ZOMBIE:
        if (target->is(1u << 4) && random(1, 100) <= 20
            && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_DISEASE,
                                               getControllingSide(),
                                               target, 1, 1))
            target->m_postPowSpellToCast = SPELL_DISEASE;
        return 0;
    case ARMY_CREATURE_UNICORN:
    case ARMY_CREATURE_WAR_UNICORN:
        if (random(1, 100) <= 20 && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_BLIND,
                                               getControllingSide(),
                                               target, 1, 1)) {
            target->m_postPowSpellToCast = SPELL_BLIND;
            return 1;
        }
        return 0;
    case CREATURE_DENDROID_GUARD:
    case CREATURE_DENDROID_SOLDIER:
        if (!addItem(target->m_binders, this))
            return 0;
        target->m_postPowSpellToCast = SPELL_BIND;
        addItem(m_boundArmies, target);
        return 0;
    case CREATURE_BLACK_KNIGHT:
    case CREATURE_DREAD_KNIGHT:
    case CREATURE_MUMMY:
        if (random(1, 100) <= 25 && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_CURSE,
                                               getControllingSide(),
                                               target, 1, 1))
            target->m_postPowSpellToCast = SPELL_CURSE;
        return 0;
    case CREATURE_MEDUSA:
    case CREATURE_MEDUSA_QUEEN:
    case CREATURE_BASILISK:
    case CREATURE_GREATER_BASILISK:
        if (random(1, 100) <= 20 && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_STONE,
                                               getControllingSide(),
                                               target, 1, 1)) {
            target->m_postPowSpellToCast = SPELL_STONE;
            return 1;
        }
        return 0;
    case CREATURE_RUST_DRAGON:
        if (target->m_numTroops > 0 && target->m_monInfo.m_defenseSkill > 0)
            target->m_postPowSpellToCast = SPELL_ACID_BREATH_DEFENSE;
        return 0;
    case CREATURE_WYVERN_MONARCH:
        if (target->is(1u << 4) && random(1, 100) <= 30
            && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_POISON,
                                               getControllingSide(),
                                               target, 1, 1))
            target->m_postPowSpellToCast = SPELL_POISON;
        return 0;
    case CREATURE_SCORPICORE:
        if (random(1, 100) <= 20 && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_PARALYZE,
                                               getControllingSide(),
                                               target, 1, 1)) {
            target->m_postPowSpellToCast = SPELL_PARALYZE;
            return 1;
        }
        return 0;
    }
    return 0;
}

// The retaliation a Fire Shield charges the attacker, and the one body
// in this TU that repaints the WHOLE field: every stack the shield
// touched is marked, the effect is played once, and the marks are
// cleared again.

VA(0x004409c0, 0x1F9)  // dc 0x464e0
void army::doFireShield(long damageAmount)
{
    int side;
    int i;
    army* a;
    g_combatManager->resetLimitCreature();
    g_combatManager->markCreatureEffect(m_combatSide, m_bitIndex);
    for (side = 0; side < 2; side++) {
        a = &g_combatManager->m_armies[side][0];
        for (i = g_combatManager->m_numArmies[side]; i-- > 0; a++) {
            if (a->m_showFireShield) {
                g_combatManager->markCreatureEffect(side, a->m_bitIndex);
                a->m_showPowEffect = 1;
            }
        }
    }
    for (side = 0; side < 2; side++) {
        a = &g_combatManager->m_armies[side][0];
        for (i = g_combatManager->m_numArmies[side]; i-- > 0; a++)
            a->m_showFireShield = 0;
    }
    int killed = damage(damageAmount);
    SAMPLE2 sample;
    if (!g_combatManager->isQuickCombat())
        sample = loadPlaySample(g_spellTraits[SPELL_FIRE_SHIELD].m_sample);
    g_combatManager->powEffect(combatManager::eSpellEffectFireShield, 0);
    g_combatManager->damageMessage(g_spellTraits[SPELL_FIRE_SHIELD].m_name, 1,
                                    damageAmount, this, killed);
    if (!g_combatManager->isQuickCombat())
        waitEndSample(sample, -1);
}

// E:\gamedcs\army.cpp:1896
// The blow has landed; now the attacker's aftermath, one arm per
// special: the Vampire Lord drains min(damage, victim life, missing
// life) back into its own stack and resurrects a Lord per full
// hit-point block; the Mighty Gorgon death-stares up to
// (numTroops+9)/10 victims dead at 10% a head; the Thunderbird lands
// a 20% Lightning Bolt at 10 damage a bird; the Serpent/Dragon Fly
// dispels (the Dragon Fly adding advanced Weakness); and the Rust
// Dragon's 20% acid breath burns half a damage roll per head. Every
// arm reads the bytes' own quick-combat asymmetries faithfully -
// the Gorgon and Rust Dragon copy UNINITIALIZED SAMPLE2 slots on the
// quick path (guarded again before WaitEndSample), and the Rust arm
// constructs a std::string it never uses (its dtor constant-folds
// away over the known-null _Ptr; only the EH state betrays it).

// Restoring that source initially moves the byte score through the expected
// banked dip, but improves retail structure from 122/136 to 131/136 blocks,
// missing blocks from 14 to 5, and branches from 67/71 to 69/71. The residual
// is now a four-byte frame difference plus nested GetName/GetArmyName inline
// selection. Dreamcast's later Rust-message rows are not carried into
// Complete: retail's relocation/call multiset directly rejects them.
VA(0x00440bc0, 0xA41)  // anchor-global, dc 0x46658
void army::doPostAttack(army* target, int attackDamage, int killedCount,
                          int totalLife)
{
    switch (m_creatureType) {
    case CREATURE_VAMPIRE_LORD:
        if (target->is(1u << 4)) {
            long deadVampires = 0;
            long missingLife =
                m_monInfo.m_hitPoints * (m_origNumTroops - m_numTroops) + m_topCreatureDamage;
            long damageRecovered = min(attackDamage, totalLife);
            damageRecovered = min(damageRecovered, missingLife);
            m_topCreatureDamage -= damageRecovered;
            if (m_topCreatureDamage < 0) {
                deadVampires =
                    (m_monInfo.m_hitPoints - m_topCreatureDamage - 1) / m_monInfo.m_hitPoints;
                m_topCreatureDamage += m_monInfo.m_hitPoints * deadVampires;
                m_numTroops += deadVampires;
            }
            if (damageRecovered > 0) {
                std::string text;
                const char* targetName =
                    target->getName(target->m_numTroops + killedCount);
                if (m_numTroops - deadVampires == 1)
                    text = formatString((*g_generalText)[362],
                                         getName(m_numTroops - deadVampires),
                                         damageRecovered, targetName);
                else
                    text = formatString((*g_generalText)[363],
                                         getName(m_numTroops - deadVampires),
                                         damageRecovered, targetName);
                if (deadVampires > 0) {
                    if (deadVampires == 1)
                        text += (*g_generalText)[364];
                    else
                        text += formatString((*g_generalText)[365],
                                              deadVampires);
                }
                if (!static_cast<const combatManager*>(g_combatManager)
                         ->isQuickCombat()) {
                    g_combatManager->m_combatWindow->combatMessage(
                        text.c_str(), 1, 0);
                    SAMPLE2 sample = loadPlaySample(
                        DATA_COMPGEN(0x00660a40, drainLifeSampleName,
                                     "DrainLif.wav"));
                    g_combatManager->spellEffect(74, this, 100, 0);
                    waitEndSample(sample, -1);
                }
            }
        }
        break;

    case CREATURE_MIGHTY_GORGON: {
        int stares = 0;
        if (target->is(1u << 4)) {
            for (long i = 0; i < m_numTroops; i++) {
                if (random(1, 100) <= 10)
                    stares++;
            }
            long dead = min(stares, target->m_numTroops);
            dead = min(dead, (m_numTroops + 9) / 10);
            if (dead > 0) {
                long damage =
                    target->m_monInfo.m_hitPoints * dead - target->m_topCreatureDamage;
                std::string text;
                if (dead == 1)
                    text = formatString((*g_generalText)[119],
                                         target->getName(dead),
                                         getName());
                else
                    text = formatString((*g_generalText)[120],
                                         dead,
                                         target->getName(dead),
                                         getName());
                SAMPLE2 sample;
                if (!static_cast<const combatManager*>(g_combatManager)
                         ->isQuickCombat()) {
                    g_combatManager->m_combatWindow->combatMessage(
                        text.c_str(), 1, 0);
                    sample = loadPlaySample(
                        g_spellTraits[SPELL_DEATH_STARE].m_sample);
                    g_combatManager->spellEffect(80, target, 100, 0);
                }
                target->damage(damage);
                g_combatManager->powEffect(-1, 1);
                if (!static_cast<const combatManager*>(g_combatManager)
                         ->isQuickCombat())
                    waitEndSample(sample, -1);
            }
        }
        break;
    }

    case CREATURE_THUNDERBIRD:
        if (target->m_numTroops > 0 && random(1, 100) <= 20) {
            if (g_combatManager->spellCastWorks(SPELL_LIGHTNING_BOLT,
                                                getControllingSide(),
                                                target, 1, 1)) {
                long damage = g_combatManager->modifySpellDamage(
                    m_numTroops * 10, SPELL_LIGHTNING_BOLT, 0, 0, target,
                    0);
                if (damage > 0) {
                    std::string text;
                    SAMPLE2 sample;
                    if (!static_cast<const combatManager*>(
                             g_combatManager)
                             ->isQuickCombat()) {
                        text = formatString(
                            (*g_generalText)[368],
                            target->getName());
                        g_combatManager->m_combatWindow->combatMessage(
                            text.c_str(), 1, 0);
                        sample = loadPlaySample(
                            DATA_COMPGEN(0x00660a30,
                                         lightningBoltSampleName,
                                         "LightBlt.wav"));
                        g_combatManager->spellEffect(1, target, 10, 0);
                    }
                    long killed = target->damage(damage);
                    g_combatManager->damageMessage(
                        g_spellTraits[SPELL_LIGHTNING_BOLT].m_name, 1,
                        damage, target, killed);
                    target->m_showPowEffect = 1;
                    g_combatManager->powEffect(49, 1);
                    if (!static_cast<const combatManager*>(
                             g_combatManager)
                             ->isQuickCombat())
                        waitEndSample(sample, -1);
                }
            }
        }
        break;

    case CREATURE_SERPENT_FLY:
    case CREATURE_DRAGON_FLY:
        if (target->m_numTroops > 0) {
            if (g_combatManager->spellCastWorks(SPELL_DISPEL_HELPFUL,
                                                getControllingSide(),
                                                target, 1, 1))
                g_combatManager->castSpell(SPELL_DISPEL_HELPFUL,
                                           target->m_gridIndex, 1, -1, 0,
                                           3);
            if (m_creatureType == CREATURE_DRAGON_FLY
                && target->m_spellInfluence[SPELL_WEAKNESS] == 0) {
                if (g_combatManager->spellCastWorks(
                        SPELL_WEAKNESS, getControllingSide(), target,
                        1, 1))
                    g_combatManager->castSpell(SPELL_WEAKNESS,
                                               target->m_gridIndex, 1, -1,
                                               2, 3);
            }
        }
        break;

    case CREATURE_RUST_DRAGON:
        if (target->m_numTroops > 0 && random(1, 100) <= 20) {
            long damage = random(m_monInfo.m_damageLowBound, m_monInfo.m_damageHighBound) * m_numTroops / 2;
            if (damage > 0) {
                std::string text;
                SAMPLE2 sample;
                if (!static_cast<const combatManager*>(g_combatManager)
                         ->isQuickCombat())
                    sample = loadPlaySample(
                        g_spellTraits[SPELL_ACID_BREATH_DEFENSE]
                            .m_sample);
                long killed = target->damage(damage);
                g_combatManager->damageMessage(
                    g_spellTraits[SPELL_ACID_BREATH_DEFENSE].m_name, 1,
                    damage, target, killed);
                if (!static_cast<const combatManager*>(g_combatManager)
                         ->isQuickCombat()) {
                    g_combatManager->spellEffect(81, target, 100, 0);
                    waitEndSample(sample, -1);
                }
                g_combatManager->powEffect(-1, 1);
            }
        }
        break;
    }
}

// E:\gamedcs\army.cpp:2044
// One swing, fully resolved: mark who gets hit (the hydra's whole
// adjacency mask - widened by the Cerberus's two ring neighbours - or
// the struck stack plus the dragon's-breath cell behind it), roll the
// luck bonus, deal and apply the damage, pick the attack frame row
// from the direction, run the on-attack special, print the damage
// line and hand the kill accounting to do_post_attack. Returns 1 when
// the blow incapacitated the defender (the special's own answer, or a
// fresh full blind).

// GetName expands at the two damage_message sites and stays a call at
// the sprintf; get_controlling_side is a CALL here (the one this TU
// keeps out of line); MarkCreatureEffect expands three times.

// SPELLING LEDGER (0 -> 88.11 -> 93.99 -> 95.52 -> 98.59 -> 99.9616 -> 99.9962):
// the two over-inlines take statement-scoped depth(0) pins with the call
// hoisted to a local (striking_side, creature_name); the berserk criteria is
// an IF/ELSE around two GetAttackMask calls, not a ternary. Naming the two
// ComputeBaseDamage results and the adjacent hex fixes VC6's nested-argument
// evaluation order. The DC prototype and retail's destination slots prove
// do_multi_head_attack's fourth output is fire_shield_damage, not total_life.
// A nested shield-charge scope gives retail's dead [ebp+8] parameter home;
// spelling the null arm explicitly gives its fall-through and zero register.
// Named side/index arguments select retail's address association in both
// MarkCreatureEffect expansions.

VA(0x00441610, 0x6A0)  // corroborates, dc 0x46bec
unsigned char army::doAttack(army* armyToAttack, int direction)
{
    unsigned attackMask;
    army* behind = 0;
    g_combatManager->resetHitByCreature();
    if (is(1u << 19)) {
        if (m_spellInfluence[59])
            attackMask = getAttackMask(m_gridIndex, 2, -1);
        else
            attackMask = getAttackMask(m_gridIndex, 1, -1);
        if (m_creatureType == CREATURE_CERBERUS) {
            unsigned mask = 0xff;
            mask &= ~(1u << direction);
            mask &= ~(1u << getCounterClockwise(direction));
            mask &= ~(1u << getClockwise(direction));
            attackMask |= mask;
        }
    } else {
        armyToAttack->m_hitByCreature = 1;
        if (is(1u << 3)) {
            int adjacentHex = getAdjacentHex(m_gridIndex, direction);
            long behindHex = getAdjacentCellIndex(adjacentHex, direction);
            if (g_combatManager->validHex(behindHex)) {
                behind = g_combatManager->m_cells[behindHex].getArmy();
                if (behind) {
                    if (behind->m_hitByCreature)
                        behind = 0;
                    else
                        behind->m_hitByCreature = 1;
                }
            }
        }
    }
    g_combatManager->resetLimitCreature();
    g_combatManager->markCreatureEffect(getOwningSide(), m_bitIndex);
    checkLuck();
    int damage = 0;
    int killed = 0;
    int nextDamage = 0;
    int nextKilled = 0;
    long fireDamage = 0;
    long totalLife = 0;
    if (is(1u << 19)) {
        doMultiHeadAttack(attackMask, &damage, &killed,
                             &fireDamage);
    } else {
        g_combatManager->markCreatureEffect(armyToAttack->getOwningSide(),
                                            armyToAttack->m_bitIndex);
        if (behind)
            g_combatManager->markCreatureEffect(behind->getOwningSide(),
                                                behind->m_bitIndex);
        totalLife = armyToAttack->getTotalHitPoints(0);
        fireDamage = damageEnemy(armyToAttack, &damage, &killed, 0);
        if (fireDamage > 0)
            armyToAttack->m_showFireShield = 1;
        if (behind)
            damageEnemy(behind, &nextDamage, &nextKilled, 0);
    }
    g_combatManager->computeMaxExtent();
    m_showAttackFrames = 1;
    if (m_creatureType != CREATURE_HYDRA && m_creatureType != CREATURE_CHAOS_HYDRA
        && !static_cast<const combatManager*>(g_combatManager)
                ->isQuickCombat()) {
        if (direction == COMBAT_DIRECTION_WIDE_UPPER
            || direction == COMBAT_DIRECTION_5
            || direction == COMBAT_DIRECTION_0) {
            if (behind && m_stdIcon->isValidSeq(cs_special_ur))
                m_showAttackFrameType = cs_special_ur;
            else if (m_creatureType == ARMY_CREATURE_BALLISTA)
                m_showAttackFrameType = cs_range_ur;
            else
                m_showAttackFrameType = cs_attack_ur;
        } else if (direction == COMBAT_DIRECTION_1
                   || direction == COMBAT_DIRECTION_4) {
            if (behind && m_stdIcon->isValidSeq(cs_special_r)) {
                m_showAttackFrameType = cs_special_r;
            } else if (m_creatureType == ARMY_CREATURE_BALLISTA) {
                m_showAttackFrameType = cs_range_r;
            } else {
                m_showAttackFrameType = cs_attack_r;
            }
        } else {
            if (behind && m_stdIcon->isValidSeq(cs_special_dr))
                m_showAttackFrameType = cs_special_dr;
            else if (m_creatureType == ARMY_CREATURE_BALLISTA)
                m_showAttackFrameType = cs_range_dr;
            else
                m_showAttackFrameType = cs_attack_dr;
        }
    } else {
        m_showAttackFrameType = cs_attack_r;
    }
    unsigned char special = checkSpecialAttack(armyToAttack);
    g_combatManager->powEffect(-1, 0);
    if (!(is(1u << 19))) {
        if (behind && behind->m_creatureType != armyToAttack->m_creatureType)
            g_combatManager->damageMessage(
                getName(), m_numTroops,
                damage + nextDamage, 0, killed + nextKilled);
        else
            g_combatManager->damageMessage(
                getName(), m_numTroops, damage, armyToAttack, killed);
    }
    doPostAttack(armyToAttack, damage, killed, totalLife);
    if (fireDamage > 0)
        doFireShield(fireDamage);
    if (armyToAttack->m_residualBlindness && armyToAttack->m_blindFactor == 0.0)
        return 1;
    else
        return special;
}

// The whole melee exchange in one direction: turn the defender to face
// the blow, land it, let the defender retaliate, take a second swing if
// this stack has one, and put both facings back.

VA(0x00441cb0, 0x2BE)  // dc 0x46fb0
void army::doAttack(int direction)
{
    m_drawPriority = 6;
    int hex = getAdjacentHex(m_gridIndex, direction);
    if (!g_combatManager->validHex(hex))
        return;
    army* armyToAttack = g_combatManager->m_cells[hex].getArmy();
    if (!armyToAttack)
        return;
    int savedArmyToAttackFacing = armyToAttack->m_facing;
    long counterDirection = armyToAttack->getAttackDirection(this);
    if (armyToAttack->needToTurn(counterDirection)) {
        int savedSide = g_combatManager->m_actingSide;
        int savedSlot = g_combatManager->m_actingSlot;
        g_combatManager->m_actingSide = armyToAttack->m_combatSide;
        g_combatManager->m_actingSlot = armyToAttack->m_bitIndex;
        armyToAttack->setupAnimation();
        armyToAttack->turn(1);
        g_combatManager->m_actingSide = savedSide;
        g_combatManager->m_actingSlot = savedSlot;
    }
    if (armyToAttack->m_spellInfluence[62])
        armyToAttack->m_residualBlindness = 1;
    if (armyToAttack->m_spellInfluence[74])
        armyToAttack->m_residualParalyze = 1;
    unsigned char killed = doAttack(armyToAttack, direction);
    m_joustBonus = 0;
    if (armyToAttack->m_numTroops > 0
        && armyToAttack->canRetaliate(*this) && !killed) {
        GameTime::delay(static_cast<int>(
            g_combatSpeedFactors[g_unnamed698758.m_combatSpeed] * 150.0f));
        g_combatManager->m_currentSide = 1 - g_combatManager->m_currentSide;
        armyToAttack->doAttack(this, counterDirection);
        g_combatManager->m_currentSide = 1 - g_combatManager->m_currentSide;
        armyToAttack->m_retaliationCount--;
    }
    armyToAttack->m_residualBlindness = 0;
    armyToAttack->m_residualParalyze = 0;
    if ((is(1u << 15)) && armyToAttack->m_numTroops > 0 && !(is(1u << 2))
        && !isIncapacitated() && m_numTroops > 0) {
        GameTime::delay(static_cast<int>(
            g_combatSpeedFactors[g_unnamed698758.m_combatSpeed] * 150.0f));
        doAttack(armyToAttack, direction);
    }
    if (!(armyToAttack->is(1u << 21))) {
        if (savedArmyToAttackFacing != armyToAttack->m_facing) {
            int savedSide = g_combatManager->m_actingSide;
            int savedSlot = g_combatManager->m_actingSlot;
            g_combatManager->m_actingSide = armyToAttack->m_combatSide;
            g_combatManager->m_actingSlot = armyToAttack->m_bitIndex;
            armyToAttack->setupAnimation();
            armyToAttack->turn(1);
            g_combatManager->m_actingSide = savedSide;
            g_combatManager->m_actingSlot = savedSlot;
        }
    }
    cancelSpellType(ARMY_CANCEL_SPELLS_AFTER_ATTACK);
    m_side = -1;
}

// The arrow-tower guard in front of the combat manager's landmine /
// fire-wall worker (0x46a570). Two things worth recording.

VA(0x00441f70, 0x26)  // dc 0x47270
unsigned char army::checkObstacleAttacks(unsigned char isWalking)
{
    if (m_creatureType == ARMY_CREATURE_ARROW_TOWER)
        return 0;
    return g_combatManager->checkObstacleAttacks(this, isWalking);
}

// E:\gamedcs\army.cpp:2386
// The whole walk: path to the destination, tear the stack out of the
// aura and bind graphs, then step the path cell by cell - lowering the
// drawbridge when the next cell asks for it, stopping dead (result 0)
// on a moat-slowed cell or a hidden trap it reveals, charging up the
// Champion's joustBonus with the step count - and re-wire auras and
// facing at the end.

// WHAT IS INLINE AND WHAT IS NOT, read off the bytes: GetSpeed expands
// at both of its sites (the slowRounds float re-time with its floor at
// 1), remove_aura expands whole (both teardown loops with erase_item
// CALLS kept), remove_binding stays a call, play_sample(POST_WALK)
// expands with its own IsQuickCombat re-test under the outer one, and
// the WALK_SAMPLE stop is written straight through gpSoundManager -
// stop_sample's inline would re-test IsQuickCombat a third time and
// retail has exactly two. Walk's direction argument re-reads the path
// cell's bitfield rather than the `direction` local the two adjacency
// calls share - do not cache what retail reloads.

// `stop` doubles as the loop bound: a moat or trap RAISES it to the
// current index so the walk ends on this step. The explicit-else
// spelling and the `(stop = ...) < 0` condition-assignment measure
// IDENTICALLY (89.7721) - both give retail's shared zero-store block -
// and the `long stop = 0;` pre-initialized form is 1.0 WORSE
// (88.7607); the init store must not exist ahead of the branch.

// Residual (89.7721%): the register-mirror family. Retail homes
// gpSearchArray in EBX and the counts in EDI for the whole body; our
// C2 picks the mirror image at the first definition and every
// downstream pairing follows, plus the `stop` slot takes its zero from
// an immediate store where ours routes a zeroed register. Same B1
// handle-state class as attack_hex's direction-search note. Calls,
// call order, and every block pair off (24/24 calls after the
// remove_aura longhand below).
// 89.8063 -> 93.3162 (2026-08-21): the two blocked-hex else-arms must
// write `succeeded = 0; stop = i;` in THAT order while the moat arms
// write `stop = i; succeeded = 0;` - the asymmetry is what stops our
// CL cross-jumping the four arm tails into one shared block, which
// retail keeps duplicated per arm (why-branch's D8 jne->je pair, and
// the whole 343-vs-351 instruction gap). Residual (93.32): the walk
// region's ebx/edi roles and the stop/conversion-temp slots are
// permuted - retail homes `stop` at [ebp-0x4] and reloads it per
// iteration where we keep it in EBX; why-reg's model reads the first
// ESI/EBX/EDI definitions as agreeing on both sides, so the flip is
// mid-function creation order past the model's reach. Global-load
// census agrees 23=23, so it is not a cache-vs-reload spelling.
VA(0x00441fa0, 0x461)  // anchor-global, dc 0x472f4
unsigned char army::walkTo(int destIndex, unsigned char restoreFacing)
{
    m_side = m_slot = -1;
    if (!findPath(destIndex, getSpeed(), 0, 0))
        return 0;
    long originalFacing = m_facing;
    // remove_aura()'s body, spelled through so the two erase_item
    // sites can carry the pins retail's own expansion decisions need:
    // both stay CALLS here (our CL otherwise inlines one), the first
    // size() and clear() expand, the second size() and clear() stay
    // out of line.
    long sourceCount = m_auraSources.size();
    while (sourceCount-- > 0) {
        army* source = m_auraSources[sourceCount];
#pragma inline_depth(0)
        eraseItem(source->m_auraClients, this);
#pragma inline_depth()
    }
    {
        // Retail reads _First and _Last through the VECTOR'S OWN ADDRESS -
        // `mov eax,[ebx+8] / mov ecx,[ebx+4]`, the same EBX it then hands the
        // erase as `this` - where `aura_sources.begin()` makes VC6 fold the
        // member offset into each load off `this` (`[esi+0x52c]` and
        // `[esi+0x528]`) and form EBX only for the call. Naming the vector as
        // a reference is what puts the base in a register first:
        // 89.7721 -> 89.8063. NARROW, and measured: naming BOTH vectors at
        // the top of the remove_aura block instead puts the row back at
        // exactly 89.7721, so this is per-site, not a style to spread.
        std::vector<army*>& sources = m_auraSources;
        army** first = sources.begin();
        army** end = sources.end();
#pragma inline_depth(0)
        sources.erase(first, end);
#pragma inline_depth()
    }
#pragma inline_depth(0)
    long clientCount = m_auraClients.size();
#pragma inline_depth()
    while (clientCount-- > 0) {
        army* client = m_auraClients[clientCount];
#pragma inline_depth(0)
        eraseItem(client->m_auraSources, this);
#pragma inline_depth()
    }
#pragma inline_depth(0)
    m_auraClients.clear();
#pragma inline_depth()
    removeBinding();
    unsigned char succeeded = 1;
    long stop;
    if (!g_combatManager->m_creaturePlacement) {
        stop = g_searchArray->getPathSteps() - getSpeed();
        if (stop < 0)
            stop = 0;
    } else {
        stop = 0;
    }
    long last = g_searchArray->getPathSteps() - 1;
    unsigned char atRest = 1;
    m_isMoving = 1;
    m_joustBonus = last - stop + 1;
    for (long i = last; i >= stop; i--) {
        long direction = g_searchArray->getStep(i);
        long nextHex = getAdjacentCellIndex(m_gridIndex, direction);
        if (g_combatManager->shouldLowerDoor(this, nextHex)) {
            if (!atRest) {
                if (!static_cast<const combatManager*>(g_combatManager)
                         ->isQuickCombat()) {
                    playSample(POST_WALK_SAMPLE);
                    if (m_armySample[WALK_SAMPLE])
                        g_soundManager->stopSample(
                            m_armySample[WALK_SAMPLE]->m_memSample.m_memSampleHandle);
                    playAnimation(cs_postwalk, -1, 0);
                    playAnimation(cs_wait, 1, 0);
                }
                m_currFrameType = cs_wait;
                m_currFrameIndex = 0;
                g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
            }
            g_combatManager->lowerDoor();
            g_combatManager->m_drawbridgeBounds = g_combatAreaLimits;
            atRest = 1;
        }
        if (g_searchArray->isMoat(static_cast<short>(nextHex))) {
            stop = i;
            succeeded = 0;
        } else if (g_combatManager->m_cells[nextHex].m_attributes & 4) {
            g_combatManager->m_obstacles
                [g_combatManager->m_cells[nextHex].m_obstacleIndex]
                .m_isVisible = 1;
            succeeded = 0;
            stop = i;
        }
        if (m_monInfo.m_attributes & 1) {
            long secondHex = getAdjacentCellIndex(m_gridIndex, direction)
                              + (m_facing ? 1 : -1);
            if (g_searchArray
                    ->isMoat(static_cast<short>(secondHex))) {
                stop = i;
                succeeded = 0;
            } else if (g_combatManager->m_cells[secondHex].m_attributes & 4) {
                g_combatManager->m_obstacles
                    [g_combatManager->m_cells[secondHex].m_obstacleIndex]
                    .m_isVisible = 1;
                succeeded = 0;
                stop = i;
            }
        }
        walk(g_searchArray->getStep(i), i == stop, atRest);
        atRest = 0;
        if (m_creatureType != ARMY_CREATURE_ARROW_TOWER)
            g_combatManager->checkObstacleAttacks(this, i != stop);
        if (m_numTroops <= 0) {
            succeeded = 0;
            break;
        }
    }
    if (m_numTroops > 0) {
        if (m_facing != originalFacing && restoreFacing)
            turn(1);
        addAura();
        m_currFrameType = cs_wait;
        m_currFrameIndex = 0;
    }
    m_isMoving = 0;
    g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
    g_combatManager->testRaiseDoor();
    return succeeded;
}

// E:\gamedcs\army.cpp:2528
inline void army::checkLuck()
{
    m_luckStatus = 0;
    if (getController() && m_luck > 0) {
        if (sRandom(1, 24) <= min(m_luck, 3)) {
            m_luckStatus = 1;
            if (!static_cast<const combatManager*>(g_combatManager)
                     ->isQuickCombat()) {
                launchSample(DATA_COMPGEN(0x00660a20, goodLuckSampleName,
                                           "goodluck.82m"),
                              -1, 3);
                sprintf(g_text, (*g_generalText)[46], getName());
                g_combatManager->m_combatWindow->combatMessage(g_text, 1, 0);
                g_combatManager->spellEffect(
                    combatManager::eSpellEffectFortune, this, 100, 0);
            }
        }
    }
}

VA(0x00442410, 0x13B)  // dc 0x47690
long army::getAdjustedAttack(const army* enemy,
                               unsigned char rangedAttack) const
{
    long attack = m_monInfo.m_attackSkill;
    if (rangedAttack) {
        if (m_spellInfluence[44])
            attack += m_precisionAmount;
    } else {
        if (m_spellInfluence[43])
            attack += m_bloodlustAmount;
    }
    if (m_spellInfluence[55] && enemy) {
        if (((enemy->is(1u << 7)) && m_slayerLevel >= 0)
            || ((enemy->is(1u << 8)) && m_slayerLevel >= 2)
            || ((enemy->is(1u << 9)) && m_slayerLevel >= 3)) {
            attack += 8;
            if (g_combatManager->m_heroes[getControllingSide()]) {
                hero* castingHero =
                    g_combatManager->m_heroes[getControllingSide()];
                attack += castingHero->getHeroSpellBonus(55, m_monInfo.m_level, 8);
            }
        }
    }
    if (m_spellInfluence[56])
        return static_cast<long>(
            getAdjustedDefense(enemy, 0) * m_frenzyFactor + attack);
    return attack;
}

VA(0x00442550, 0x35)  // dc 0x477b8
long army::getAttackModifier(const army* enemy,
                               unsigned char rangedAttack) const
{
    return getAdjustedAttack(enemy, rangedAttack)
        - g_creatureTypeTraits[m_creatureType].m_attackSkill;
}

VA(0x00442590, 0xC2)  // dc 0x477e8
long army::getAdjustedDefense(const army* enemy,
                                unsigned char frenzyIncluded) const
{
    if (frenzyIncluded && m_spellInfluence[56])
        return 0;
    long defense = m_monInfo.m_defenseSkill;
    if (enemy) {
        if (enemy->m_creatureType == ARMY_CREATURE_BEHEMOTH)
            defense = static_cast<long>(defense - defense * 0.4f);
        else if (enemy->m_creatureType == ARMY_CREATURE_ANCIENT_BEHEMOTH)
            defense = static_cast<long>(defense - defense * 0.8f);
    }
    if (g_combatManager->m_moatOn) {
        int secondHex;
        if (m_monInfo.m_attributes & 1)
            secondHex = m_gridIndex + (m_facing ? 1 : -1);
        else
            secondHex = -1;
        if (g_combatManager->isInMoat(m_gridIndex, 0)
            || g_combatManager->isInMoat(secondHex, 0))
            defense -= 3;
    }
    return defense;
}

VA(0x00442660, 0x29)  // dc 0x478c8
long army::getDefenseModifier() const
{
    return getAdjustedDefense(0, 1)
        - g_creatureTypeTraits[m_creatureType].m_defenseSkill;
}

// E:\gamedcs\army.cpp:2680
// Dreamcast's eight-byte body returns 1.0, but the shared helper boundary is
// still visible in get_unit_combat_value. Complete expands the later body
// below at that call site; no retail out-of-line copy survives.
inline double army::getDefenseDamageModifier(
    unsigned char rangedAttack) const
{
    double factor = 1.0;
    if (rangedAttack) {
        if (m_spellInfluence[28])
            factor = m_airShieldFactor;
    } else {
        if (m_spellInfluence[27])
            factor = m_shieldFactor;
    }
    if (m_spellInfluence[70])
        factor = factor * 0.5;
    hero* controller = getController();
    if (controller)
        factor = controller->getDefenseFactor() * factor;
    return factor;
}

// The controller/owner pair, and the resolution of a naming inversion
// that two lanes had recorded in opposite directions. The two bodies
// differ by exactly the hypnotize flip:

//   0x442690 (57 B)  heroes[hypnotizeFlag ? 1 - combatSide : combatSide]
//   0x4426d0 (20 B)  heroes[combatSide]

VA(0x00442690, 0x39)  // dc 0x47904
hero* army::getController() const
{
    return g_combatManager->m_heroes[getControllingSide()];
}

VA(0x004426d0, 0x14)  // dc 0x47924
hero* army::getOwner() const
{
    return g_combatManager->m_heroes[getOwningSide()];
}

// E:\gamedcs\army.cpp:2708, dc 0x47944
unsigned char isNaturalEnemy(TCreatureType attacker, TCreatureType defender)
{
    switch (attacker) {
    case CREATURE_ANGEL:
    case CREATURE_ARCHANGEL:
        return defender == CREATURE_DEVIL
               || defender == CREATURE_ARCH_DEVIL;
    case CREATURE_DEVIL:
    case CREATURE_ARCH_DEVIL:
        return defender == CREATURE_ANGEL
               || defender == CREATURE_ARCHANGEL;
    case CREATURE_EFREETI:
    case CREATURE_EFREET_SULTAN:
        return defender == CREATURE_GENIE
               || defender == CREATURE_MASTER_GENIE;
    case CREATURE_GENIE:
    case CREATURE_MASTER_GENIE:
        return defender == CREATURE_EFREETI
               || defender == CREATURE_EFREET_SULTAN;
    case CREATURE_BLACK_DRAGON:
        return defender == CREATURE_TITAN;
    case CREATURE_TITAN:
        return defender == army::ARMY_CREATURE_BLACK_DRAGON;
    }
    return 0;
}

VA(0x004426f0, 0x8B)  // dc 0x47a14
double army::getAverageDamage() const
{
    if (m_spellInfluence[41])
        return m_blessAmount + m_monInfo.m_damageHighBound;
    if (m_spellInfluence[42])
        return cppMax(m_monInfo.m_damageLowBound - m_curseAmount, 1);
    return (m_monInfo.m_damageHighBound + m_monInfo.m_damageLowBound) / 2.0;
}

VA(0x00442780, 0x100)  // dc 0x47af8
long army::getAverageDamage(const army* enemy, unsigned char rangedAttack, long amount, unsigned char limitDamage, long distance) const
{
    double average = getAverageDamage();
    long damage = adjustDamage(const_cast<army*>(enemy),
                                static_cast<long>(amount * average),
                                rangedAttack, 1, distance, 0);
    long totalLife;
    if (enemy->is(1u << 23))
        totalLife = 1;
    else
        totalLife = enemy->m_monInfo.m_hitPoints * enemy->m_numTroops
                     - enemy->m_topCreatureDamage;
    if (damage < 1)
        damage = 1;
    if (rangedAttack && (is(1u << 15)))
        damage += damage;
    if (limitDamage && damage > totalLife)
        damage = totalLife;
    return damage;
}

VA(0x00442880, 0x68)  // dc 0x47bcc
unsigned char army::isEnemy(const army* arg) const
{
    if (!arg)
        return 0;
    if (this == arg)
        return 0;
    if (!m_spellInfluence[59] && !arg->m_spellInfluence[59])
        return getControllingSide() != arg->getOwningSide();
    return 1;
}

//   spelling                          can_shoot  AI_target_time  berserk
//   `inline`, every site expands         (none)      100.0000    92.5170
//   no keyword anywhere                  92.0000      27.8667     0.0000
//   `inline` on the army.h declarator    (none)      100.0000    92.5170
//   `inline` + ONE rejected site         92.0000      100.0000    92.5170

// UNTIL 0x447a80 IS RECONSTRUCTED the rejected site is supplied by a
// SCAFFOLD: `#pragma inline_depth(0)` around get_total_combat_value
// below, which is un-carcassed for exactly this purpose and stays
// UNCLAIMED because the pragma makes its own body the 49.64 call-form
// where retail expands. The scaffold buys the 92.0000 row here and
// costs nothing in the ledger. RETIRE IT when 0x447a80 lands: drop
// the pragma, and get_total_combat_value's own 81.97 becomes
// claimable in the same change.

VA(0x004428f0, 0xF6)  // dc 0x47c04
inline unsigned char army::canShoot(const army* excluded) const
{
    if (m_creatureType == ARMY_CREATURE_BALLISTA
        || m_creatureType == ARMY_CREATURE_ARROW_TOWER)
        return 1;
    if (!(is(1u << 2)) || m_monInfo.m_numShots <= 0)
        return 0;
    hero* controller = getController();
    int canShoot = 1;
    if (!controller
        || !controller->isWieldingArtifact(ARTIFACT_BOW_OF_THE_SHARPSHOOTER)) {
        if (enemyIsAdjacent(excluded))
            canShoot = 0;
    }
    return canShoot
           && (m_spellInfluence[61] == 0 || m_forgetfulnessLevel < 2);
}

VA(0x004429f0, 0x5C)  // dc 0x47c74
unsigned char army::enemyIsAdjacent(const army* excluded) const
{
    if (g_combatManager->enemyIsAdjacent(this, m_gridIndex, excluded))
        return 1;
    if (is(1u))
        return g_combatManager->enemyIsAdjacent(
            this, getSecondGridIndex(), excluded);
    return 0;
}

// The Artillery multipliers get_unit_combat_value's ballista arm
// indexes by the controller's skill level, at .rdata 0x63b7f0 -
// {1.0, 1.5, 1.5, 2.0} read from the hash-verified image. Name is a
// source-facing invention; no roster row reaches the table.
DATA(0x0063b7f0)
static const double g_artilleryFactors[4] = { 1.0, 1.5, 1.5, 2.0 };

// E:\gamedcs\army.cpp:2820
// Dreamcast recovers the source boundaries hidden by Complete's inliner:
// attack/defense modifiers, defense-damage adjustment, can_shoot(0), three
// independent controller reads, get_average_damage, and two
// get_total_hit_points(0) calls. Retail corroborates those boundaries and
// also proves Complete's expanded shield/controller damage factor. Keep the
// side-mass scan in its recovered pointer/count form; the indexed spelling
// preserves the semantics but loses retail's loop lowering.

// SOURCE-SHAPE LEDGER (84.9968 -> 96.5871): restoring the Dreamcast helper
// vocabulary and its pointer loop takes the CFG from 14/71 to 69/71 exact
// blocks, with all 38 branches and both returns exact. The two residual
// size-only blocks are codegen: EAX/ECX scheduling while the two modifier
// helpers inline, and retail materializing the 0.1 result in an existing
// stack slot while this compiler selects the equivalent literal pool.
VA(0x00442a50, 0x410)  // anchor-global, dc 0x47cf4
double army::getUnitCombatValue(long lowestAttack, long lowestDefense,
                                   unsigned char ranged,
                                   const army* excluded) const
{
    long attackModifier = getAttackModifier(0, ranged);
    long attackDiff = attackModifier - lowestAttack;
    long defenseModifier = getDefenseModifier();
    long defenseDiff = defenseModifier - lowestDefense;
    double defense =
        (defenseDiff * 0.05 + 1.0)
        * getDefenseDamageModifier(ranged);
    if (ranged && !canShoot(0))
        ranged = 0;
    double attack = attackDiff * 0.05 + 1.0;
    if (!ranged && (is(1u << 2)))
        attack = attack * 0.5;
    if (m_creatureType == ARMY_CREATURE_BALLISTA) {
        if (getController()) {
            if (getController()
                    ->m_skillLevel[eSecSkillBattlefieldBallistics] > 1)
                attack = attack + attack;
            attack =
                attack
                * g_artilleryFactors[getController()->getSecondarySkill(
                    eSecSkillBattlefieldBallistics)];
        }
    }
    if (m_spellInfluence[41] || m_spellInfluence[42]) {
        long damageRange = m_monInfo.m_damageLowBound + m_monInfo.m_damageHighBound;
        double baseAverage = damageRange / 2.0;
        double averageDamage = getAverageDamage();
        attack = averageDamage / baseAverage * attack;
    }
    if (ranged && (is(1u << 15)))
        attack = attack + attack;
    double result = sqrt(attack * defense)
                    * g_creatureTypeTraits[m_creatureType].m_baseFightValue;
    if (m_monInfo.m_attributes & 0x400040) {
        long total = getTotalHitPoints(0);
        long sum = 0;
        army* group = g_combatManager->m_armies[m_combatSide];
        for (long i = 0; i < g_combatManager->m_numArmies[m_combatSide];
             i++, group++) {
            if (!(group->m_monInfo.m_attributes & 0x1d0)) {
                sum += group->getTotalHitPoints(0);
            }
        }
        if (total == 0) {
            result = 0.1;
            return result;
        }
        result = sum * result / (total + sum);
    }
    return result;
}

// The two tails are the same product seen two ways: a stack whose
// creature bit 23 is set is priced by raw count, everyone else by the
// EFFECTIVE count `(hitPoints * numTroops - topCreatureDamage) /
// hitPoints`, which folds the wounded top creature back in as a
// fraction. Both quotients divide the per-unit value, so the division
// is written last in both arms.

// WHY THE PRAGMA, AND WHY THIS BODY IS NOT CLAIMED. VC6 emits
// can_shoot's out-of-line copy only when the TU holds a call it
// declines to expand; retail's army.obj holds two, inside
// spell_is_valid_on_target (0x447a80), a body this TU does not have.
// `#pragma inline_depth(0)` here supplies one in its place, which is
// what banks can_shoot at 92.0000 - but it also compiles THIS body in
// its 49.64 call-form instead of the 81.97 expanded form retail has,
// so it must not be claimed while the pragma stands. Everything from
// the get_unit_combat_value call to the end - both floating-point
// tails, the argument-slot homing, the /Op fild round trips - is
// byte-identical in BOTH forms, so the body itself is right and only
// the inliner stands between it and exact. Full matrix in can_shoot's
// note above. RETIRING 0x447a80's reconstruction retires the pragma
// and makes 0x442e60 claimable in the same change.

VA(0x00442e60, 0x169)  // dc 0x48168
long army::getTotalCombatValue(long lowestAttack, long lowestDefense) const
{
    if (m_numTroops <= 0)
        return 0;
    unsigned char ranged = canShoot(0);
    double value = getUnitCombatValue(lowestAttack, lowestDefense,
                                         ranged, 0);
    if (is(1u << 23))
        return static_cast<long>(m_numTroops * value / 5.0);
    return static_cast<long>((m_monInfo.m_hitPoints * m_numTroops - m_topCreatureDamage)
                             * value / m_monInfo.m_hitPoints);
}

VA(0x00442fd0, 0xA9)  // dc 0x482f0
long army::getLossCombatValue(long lowestAttack, long lowestDefense,
                                 unsigned char ranged, long damage,
                                 unsigned char killsOnly) const
{
    double value = getUnitCombatValue(lowestAttack, lowestDefense,
                                         ranged, 0);
    if (is(1u << 23))
        return static_cast<long>(m_numTroops * value / 5.0);
    if (killsOnly)
        value = 1000.0;
    long lost = 0;
    if (damage % m_monInfo.m_hitPoints + m_topCreatureDamage >= m_monInfo.m_hitPoints)
        lost = m_topCreatureDamage;
    return static_cast<long>((lost + damage) * value / m_monInfo.m_hitPoints);
}

VA(0x00443080, 0x50)  // dc 0x48454
long army::getTotalHitPoints(unsigned char simulated) const
{
    long total;
    if (is(1u << 23))
        total = 1;
    else
        total = m_monInfo.m_hitPoints * m_numTroops - m_topCreatureDamage;
    if (simulated)
        total = max(total - m_aiExpectedDamage, 0);
    return total;
}

VA(0x004430d0, 0x56)  // dc 0x484a0
void army::setAIExpectedDamage(long arg)
{
    m_aiExpectedDamage = cppMin(arg, getTotalHitPoints(0));
}

VA(0x00443130, 0x25)
float army::getFireShieldStrength() const
{
    if (m_spellInfluence[29])
        return m_fireShieldStrength;
    if (m_creatureType == CREATURE_EFREET_SULTAN)
        return DATA_COMPGEN(0x0063b8b4, innateFireShieldStrength, 0.2f);
    return DATA_COMPGEN(0x0063ac64, zeroFireShieldStrength, 0.0f);
}

VA(0x00443160, 0x1BF)  // dc 0x48524
int army::computeBaseDamage(unsigned char simulateOnly) const
{
    int num;
    if (m_spellInfluence[61] > 0 && (is(1u << 2)))
        num = cppMax(m_numTroops / 2, 1);
    else
        num = m_numTroops;

    int low;
    int high;
    if (m_creatureType == ARMY_CREATURE_BALLISTA) {
        const hero* shooter = getController();
        low = (shooter->getPrimarySkill(0) + 1) * m_monInfo.m_damageLowBound;
        high = (shooter->getPrimarySkill(0) + 1) * m_monInfo.m_damageHighBound;
    } else {
        low = m_monInfo.m_damageLowBound;
        high = m_monInfo.m_damageHighBound;
    }

    int damage;
    if (m_spellInfluence[41]) {
        damage = (high + m_blessAmount) * num;
    } else if (m_spellInfluence[42]) {
        damage = cppMax(low - m_curseAmount, 1) * num;
    } else if (simulateOnly) {
        damage = (low + high) * num / 2;
    } else {
        int total = 0;
        if (num > 10) {
            for (int i = 0; i < 10; i++)
                total += random(low, high);
            damage = total * num / 10;
        } else {
            for (int i = 0; i < num; i++)
                total += random(low, high);
            damage = total;
        }
    }
    return damage;
}

// Complete separates the numeric damage bonus from the combat message and sound
// so estimated-damage calculations can reuse it. Dreamcast combines these paths.

VA(0x00443320, 0x514)
int army::computeAttackerBonus(int baseDamage, unsigned char isShooting,
                                 army* defender, unsigned char announce,
                                 long distance) const
{
    int bonus = 0;
    if ((m_creatureType == CREATURE_CAVALIER
         || m_creatureType == CREATURE_CHAMPION)
        && defender->m_creatureType != 0 && defender->m_creatureType != 1) {
        bonus = static_cast<long>(static_cast<double>(baseDamage)
                                  * distance * 0.05);
    }
    if (m_luckStatus > 0)
        bonus += baseDamage;

    long attack = getAdjustedAttack(defender, isShooting);
    long defense = defender->getAdjustedDefense(this, 1);
    if (attack >= defense) {
        double factor = (attack - defense) * 0.05;
        if (factor > 3.0)
            factor = 3.0;
        bonus = static_cast<long>(baseDamage * factor + bonus);
    }

    if (defender != 0) {
        switch (m_creatureType) {
        case CREATURE_EARTH_ELEMENTAL:
        case CREATURE_MAGMA_ELEMENTAL:
            if (defender->m_creatureType == CREATURE_AIR_ELEMENTAL
                || defender->m_creatureType == CREATURE_STORM_ELEMENTAL)
                bonus += baseDamage;
            break;
        case CREATURE_AIR_ELEMENTAL:
        case CREATURE_STORM_ELEMENTAL:
            if (defender->m_creatureType == CREATURE_EARTH_ELEMENTAL
                || defender->m_creatureType == CREATURE_MAGMA_ELEMENTAL)
                bonus += baseDamage;
            break;
        case CREATURE_WATER_ELEMENTAL:
        case CREATURE_ICE_ELEMENTAL:
            if (defender->m_creatureType == CREATURE_FIRE_ELEMENTAL
                || defender->m_creatureType == CREATURE_ENERGY_ELEMENTAL)
                bonus += baseDamage;
            break;
        case CREATURE_FIRE_ELEMENTAL:
        case CREATURE_ENERGY_ELEMENTAL:
            if (defender->m_creatureType == CREATURE_WATER_ELEMENTAL
                || defender->m_creatureType == CREATURE_ICE_ELEMENTAL)
                bonus += baseDamage;
            break;
        }

        unsigned char hates =
            isNaturalEnemy(m_creatureType, defender->m_creatureType);
        if (hates) {
            bonus += baseDamage / 2;
            if (announce) {
                if (!static_cast<const combatManager*>(g_combatManager)
                         ->isQuickCombat()) {
                    std::string text;
                    if (m_numTroops == 1)
                        text = formatString(
                            g_generalText->getText(369),
                            ::getArmyName(m_creatureType, m_numTroops),
                            ::getArmyName(defender->m_creatureType,
                                    defender->m_numTroops));
                    else
                        text = formatString(
                            g_generalText->getText(370),
                            ::getArmyName(m_creatureType, m_numTroops),
                            ::getArmyName(defender->m_creatureType,
                                    defender->m_numTroops));
                    g_combatManager->m_combatWindow->combatMessage(
                        text.c_str(), 1, 0);
                }
            }
        }
    }

    long total = bonus;
    hero* controller =
        g_combatManager->m_heroes[getControllingSide()];
    if (controller != 0) {
        if (isShooting)
            total = static_cast<long>(
                controller->getArcheryFactor()
                    * static_cast<float>(baseDamage)
                + static_cast<float>(bonus));
        else
            total = static_cast<long>(
                controller->getOffenseFactor()
                    * static_cast<float>(baseDamage)
                + static_cast<float>(bonus));
        if (m_spellInfluence[SPELL_BLESS])
            total += controller->getHeroSpellBonus(SPELL_BLESS,
                                                   m_monInfo.m_level,
                                                   baseDamage);
    }
    return total;
}

// The Artillery double-damage chance per mastery, at .rdata 0x63b810 -
// {0, 50, 75, 100} read from the hash-verified image. Name is a
// source-facing invention like kArtilleryFactors below; no roster row
// reaches the table.
DATA(0x0063b810)
static const int g_artilleryDoubleChances[4] = { 0, 50, 75, 100 };

VA(0x00443840, 0x344)  // dc 0x4868c
int army::computeAttackerDamageBonuses(int baseDamage,
                                       unsigned char isShooting,
                                       army* defender,
                                       unsigned char simulateOnly,
                                       long distance) const
{
    int result = computeAttackerBonus(baseDamage, isShooting, defender,
                                        simulateOnly == 0, distance);
    switch (m_creatureType) {
    case ARMY_CREATURE_BALLISTA: {
        hero* controlling =
            g_combatManager->m_heroes[getControllingSide()];
        long mastery =
            controlling->m_skillLevel[eSecSkillBattlefieldBallistics];
        if (!simulateOnly
            && random(1, 100) <= g_artilleryDoubleChances[mastery]) {
            result += baseDamage;
            if (!static_cast<const combatManager*>(g_combatManager)
                     ->isQuickCombat()) {
                std::string text;
                const char* creatureName;
                creatureName = getName();
                text = formatString(g_generalText->getText(366),
                                     creatureName);
                g_combatManager->m_combatWindow->combatMessage(
                    text.c_str(), 1, 0);
            }
        }
        break;
    }

    case CREATURE_DREAD_KNIGHT:
        if (!simulateOnly && random(1, 100) <= 20) {
            result += baseDamage;
            if (!static_cast<const combatManager*>(g_combatManager)
                     ->isQuickCombat()) {
                std::string text;
                if (m_numTroops == 1)
                    text = formatString(g_generalText->getText(366),
                                         getName());
                else
                    text = formatString(g_generalText->getText(367),
                                         getName());
                g_combatManager->m_combatWindow->combatMessage(
                    text.c_str(), 1, 0);
                SAMPLE2 sample = loadPlaySample(
                    DATA_COMPGEN(0x00660a50, deathBlowSampleName,
                                 "Deathblo.wav"));
                g_combatManager->spellEffect(
                    combatManager::eSpellEffectDeathBlow, defender, 100,
                    0);
                waitEndSample(sample, -1);
            }
        }
        break;
    }
    return result;
}

// E:\gamedcs\army.cpp:3230
inline int army::computeDefenderDamageBonuses(int baseDamage) const
{
    return 0;
}

VA(0x00443b90, 0x1FE)  // dc 0x48c60
double army::computeAttackerDamageReduction(const army* defender,
                                            unsigned char isShooting) const
{
    double reduction = 1.0;
    long attack = getAdjustedAttack(defender, isShooting);
    long defense = defender->getAdjustedDefense(this, 1);
    if (defense > attack) {
        double factor = 1.0 - (defense - attack) * 0.025;
        if (factor < 0.3)
            factor = 0.3;
        reduction = factor;
    }
    if (m_creatureType == ARMY_CREATURE_PSYCHIC_ELEMENTAL
            && (defender->is(1u << 10)))
        reduction *= 0.5;
    if (m_creatureType == ARMY_CREATURE_MAGIC_ELEMENTAL
            && (defender->m_creatureType == ARMY_CREATURE_MAGIC_ELEMENTAL
                || defender->m_creatureType == ARMY_CREATURE_BLACK_DRAGON))
        reduction *= 0.5;
    if (isShooting) {
        int hex = m_gridIndex;
        if (m_monInfo.m_attributes & 1)
            hex += m_facing ? 1 : -1;
        if (g_combatManager->shotIsThroughWall(this, hex, defender->m_gridIndex))
            reduction *= 0.5;
        if (g_combatManager->shotIsNotOptimal(this, defender))
            reduction *= 0.5;
    }
    if ((is(1u << 2)) && !isShooting && !(is(1u << 12)))
        reduction *= 0.5;
    if (m_residualBlindness && m_residualParalyze) {
        double penalty = cppMin<double>(
            m_blindFactor, g_spellTraits[SPELL_BLIND].m_masteryBonus[2] / 100.0);
        reduction = penalty * reduction;
    } else if (m_residualBlindness)
        reduction = m_blindFactor * reduction;
    else if (m_residualParalyze)
        reduction = g_spellTraits[SPELL_BLIND].m_masteryBonus[2] / 100.0
                    * reduction;
    return reduction;
}

VA(0x00443d90, 0x9C)  // dc 0x48fc4
double army::computeDefenderDamageReduction(unsigned char isShooting) const
{
    double reduction = 1.0;
    if (isShooting) {
        if (m_spellInfluence[28])
            reduction = m_airShieldFactor;
    } else {
        if (m_spellInfluence[27])
            reduction = m_shieldFactor;
    }
    if (m_spellInfluence[70])
        reduction *= 0.5;
    hero* castingHero = g_combatManager->m_heroes[getControllingSide()];
    if (castingHero)
        reduction *= castingHero->getDefenseFactor();
    return reduction;
}

// RETAIL-ONLY: UNNAMED in the DC dump, the HD name map and IDA alike, and
// its two callers are both in ai_tactical - get_attack_skill_value
// (0x437800) and get_defense_skill_value (0x438910), each asking what a
// hypothetical stack of ours would do to a target. Four stack arguments
// (ret 0x10); it runs compute_attacker_bonus, then
// ComputeAttackerDamageReduction, then the defender's own shield /
// petrify / hero-defense-factor chain, and floors the answer at 1. It is
// NOT DamageEnemy: that row takes (army*, int*, int*, bool) on the DC
// roster and calls ComputeBaseDamage / adjust_damage / Damage, none of
// which appear here. Name declared in army.h as a bootstrap invention.

// THE DEFENDER'S WHOLE CHAIN IS ComputeDefenderDamageReduction INLINED,
// which is what identifies the tail: the +0x208/+0x4bc and
// +0x204/+0x4b8 shield pair, the +0x2b0 petrify halving and the
// heroes[get_controlling_side()] defense factor arrive here in exactly
// that body's order, including its inlined `1 - combatSide` hypnotize
// flip. Retail keeps an out-of-line copy at 0x443d90 and expands it
// here - /Ob2 on a same-TU body defined above the call site.

// The attacker's two reductions are NOT symmetric, and the byte order
// says why: ComputeAttackerDamageReduction is CALLED and multiplies
// `reduction * amount` (the double is the left operand, the int is
// filed into a temp and multiplied in), while the defender's inlined
// factor multiplies `amount * factor` the other way round. Both
// orientations are transcribed as retail has them.

// `amount` is assigned back into its own parameter slot [ebp+0xc]
// three times over, so the source updates the parameter rather than
// introducing locals.
// A 17-state original-damage capture family (two emitted objects) also
// does not improve 98.5227%: int/long captures, const qualifiers, block
// lifetimes and return-then-add all leave the add-register choice unresolved.
VA(0x00443e30, 0x101)  // anchor-callee (ai_tactical's two skill-value
                       // functions) + arity ret 0x10, retail-only slot
long army::getEstimatedDamage(const army* target, long amount,
                                unsigned char ranged, long distance) const
{
    if (!target)
        return 0;
    // Residual (98.5227%): ONE instruction pair. Retail accumulates
    // INTO the call's return register (`add eax, ecx` /
    // `mov [amount], eax`); our C2 loads `amount` into ECX and
    // accumulates the other way (`add ecx, eax` / `mov [amount], ecx`).
    // Everything else in the body is byte-identical and the remaining
    // diff rows are reloc names on unclaimed rows. VC6 CANONICALISES
    // this add, so the written operand order does not reach it - tried
    // and rejected: `amount += compute_attacker_bonus(...)` (identical
    // bytes) and a named `long bonus` local, which is strictly worse
    // (it sinks the add past the ComputeAttackerDamageReduction call
    // and spills the bonus to a stack slot first).
    amount = computeAttackerBonus(amount, ranged, const_cast<army*>(target),
                                    0, distance)
             + amount;
    amount = static_cast<long>(
        computeAttackerDamageReduction(target, ranged) * amount);
    amount = static_cast<long>(
        amount * target->computeDefenderDamageReduction(ranged));
    if (amount <= 0)
        amount = 1;
    return amount;
}

VA(0x00443f40, 0x14F)  // dc 0x490f4
long army::adjustDamage(army* enemy, long baseDamage, unsigned char isShot,
                         unsigned char simulated, long distance,
                         long* fireDamage) const
{
    if (fireDamage)
        *fireDamage = 0;
    if (!enemy)
        return 0;
    int totalDamage = baseDamage
        + computeAttackerDamageBonuses(
            baseDamage, isShot, enemy, simulated, distance)
        + enemy->computeDefenderDamageBonuses(baseDamage);
    if (fireDamage) {
        *fireDamage = g_combatManager->computeFireShieldDamage(
            totalDamage, this, enemy, enemy->getTotalHitPoints(0));
    }
    totalDamage = static_cast<long>(
        totalDamage * computeAttackerDamageReduction(enemy, isShot));
    totalDamage = static_cast<long>(
        totalDamage * enemy->computeDefenderDamageReduction(isShot));
    if (totalDamage <= 0)
        totalDamage = 1;
    return totalDamage;
}

// E:\gamedcs\army.cpp:3406
inline long army::damageEnemy(army* enemy, int* damageOut, int* killed,
                              unsigned char isShot)
{
    long fireDamage = 0;
    if (!enemy)
        return 0;
    long damage = computeBaseDamage(0);
    *damageOut = adjustDamage(enemy, damage, isShot, 0, m_joustBonus,
                             &fireDamage);
    *killed = enemy->damage(*damageOut);
    m_luckStatus = 0;
    return fireDamage;
}

VA(0x00444090, 0x8F)  // dc 0x492c8
int army::damage(int damage)
{
    int total = damage + m_topCreatureDamage;
    int killed = total / m_monInfo.m_hitPoints;
    m_topCreatureDamage = total % m_monInfo.m_hitPoints;
    if (is(1u << 23)) {
        killed = m_numTroops;
        m_topCreatureDamage = 0;
    }
    m_someUnitsDamaged = 1;
    if (killed > 0)
        m_numTroopsToShowOverride = m_numTroops;
    if (killed > m_numTroops)
        killed = m_numTroops;
    m_numTroops -= killed;
    if (m_numTroops <= 0)
        m_allUnitsKilled = 1;
    cancelIndividualSpell(62);
    cancelIndividualSpell(70);
    cancelIndividualSpell(74);
    return killed;
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:3481
// The carve has ONE row between Damage (0x444090, 0x8F, ending 0x44411f)
// and CancelSpellType (0x4444d0) - 0x444120 / 0x3A6 - and the DC roster
// has TWO here, so one of the pair has no retail body. It is this one:
// 44 bytes of SH4 for a `Strength()` accessor is the in-class-inline
// shape, and 0x444120 ends `ret 4` which a no-argument member cannot.
DC_ONLY(0x4935c, 0x44)
unsigned long army::strength()
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\army.cpp:3492
// LOCATED 2026-08-14, and ResetRound's own tail is the proof: it ends
// `push 1 / mov ecx,esi / call 0x444120`, a thiscall with exactly ONE
// stack argument, and the body at 0x444120 ends `ret 4` to match - which
// is the DC roster's two-parameter row (this + bFadeElementals) and
// refutes the only other candidate in the gap outright. The body
// corroborates it end to end: it walks the two bound-army vectors at
// +0x514/+0x524 and calls remove_binding (0x43ee10), cancels all 0x51
// spell rows through CancelIndividualSpell (0x444510), raises the death
// bit 0x200000 in creatureId, rolls Random(1,100) twice for the two
// morale side-effects, files the corpse into the hexcell's dead list,
// and finally clears the CLONE link both ways - reaching the clone
// through +0x28 and re-entering itself, which is the tail recursion C2
// turned into the loop back to 0x44413c. DC ProcessDeath's own callee
// set (remove_binding, remove_aura, CancelAllSpells, LeavesNoBody,
// hexcell::HasArmy, Random) is the same body one build removed.
// The carve size 0x3A6 against the DC row's 0x2F4 is 1.24x, inside the
// SH4 -> x86 band.

// STRUCTURE FULLY DERIVED 2026-08-15; RECONSTRUCTED 2026-08-20 once all
// five header prerequisites had landed from other lanes' work (the
// mirror pair, hexcell's third dead row, cmbtmgr's field_53de/13438
// band, Random). No new header surface was needed by this promotion.

// THE DC LINE MAP (dc 0x493a0, forward read), retail agreeing at every
// point checked:
//   3493  if (Is(1u << 21)) return;          retail's entry test, and the
//         tail recursion at the bottom re-enters PAST it (0x44413c).
//   3494  remove_aura();     INLINED by retail (both vector walks and
//         both erase_item calls are open-coded here); the body already
//         in this file expands to exactly those bytes.
//   3497  remove_binding();  a real call to 0x43ee10.
//   3501  if (!hypnotizeFlag) {
//   3503    if (Random(1, 100) < 60)
//   3504      gpCombatManager->field_53dc[get_owning_side()] = 1;
//   3505    else if (Random(1, 100) < 80)
//   3506      gpCombatManager->field_53de[1 - get_owning_side()] = 1;
//         (+0x53de is inside cmbtmgr.h's pad_53de today; retail's
//         `sub ecx,side / [ecx+0x53df]` is the 1-side index.)
//   3512  CancelAllSpells();  INLINED - the 0x51-iteration walk of
//         spellInfluence with CancelIndividualSpell(i) on each positive
//         row. DC_ONLY 0x499ac has NO retail slot (the carve leaves no
//         gap between CancelIndividualSpell 0x444510 and
//         SetSpellInfluence 0x4448f0), the EndWalk situation again.
//   3516  creatureId |= 0x200000;   3517  bAllUnitsKilled = 0;
//   3521  if (combatManager::ValidHex(gridIndex)) {   -- and retail's
//         two rejects jump to the FUNCTION epilogue, so this block runs
//         to the end of the body, mirror links included.
//   3522    hexcell* pCell = &gpCombatManager->cells[gridIndex];
//   3529    if (Is(1u << 0)) {          (the two-hex marker)
//   3531      iGI2 = gridIndex + OffsetToFront(-1);
//   3532      pCell2 = &gpCombatManager->cells[iGI2];   }
//         BOTH ARE FUNCTION-SCOPE LOCALS ASSIGNED ONLY IN THAT ARM:
//         retail's one-hex path reads [ebp-8] and [ebp-0xc] back
//         UNINITIALISED. Same faithful-artifact class as AttackWall's
//         defaultless switch - do not initialise them.
//   3535    if (LeavesNoBody()) {     -- an Army.h inline with no
//           retail copy, and it is a MASK not two Is() calls: retail
//           emits one `test dword, 0x10400000`.
//   3543      gpCombatManager->field_13438[combatSide][bitIndex] = 1;
//   3544      gpCombatManager->field_13460 = 1;
//   3558    } else {
//   3562      if (pCell->field_1c < 14
//                  && (!(Is(1u << 0)) || pCell2->field_1c < 14)) {
//   3566        if (pCell->HasArmy()) {        (armySide >= 0, inlined)
//   3568..3571   the three dead-list rows plus the count bump - and the
//                THIRD array is retail-only surface: hexcell's
//                pad_3c[0xe] is deadArmySide/deadArmySlot's parallel
//                fourteen-entry row for field_1a (`[count + cell +
//                0x3c]`).
//   3574..3582   the identical block for pCell2.
//   3592..3603   gpCombatManager->field_132cc = 0, the two cell
//                identities set back to -1, and the same pair on the
//                second hex behind another Is(1u << 0).
//   3607..3619   the CLONE LINK, and the DC member table names both
//                halves: army@36 iMirrorSourceIndex is retail +0x24 and
//                army@40 iMirrorDestIndex is +0x28 (this band is
//                unshifted - groupToAttack 16/+0x10 anchors it).
//                `if (iMirrorSourceIndex != -1 &&
//                 armies[combatSide][iMirrorSourceIndex].iMirrorDestIndex
//                 == bitIndex) that.iMirrorDestIndex = -1;` then
//                `if (iMirrorDestIndex != -1) { clone->numTroops = 0;
//                 clone->ProcessDeath(bFadeElementals); }`.
//   `bFadeElementals` IS NEVER READ - retail's body has no [ebp+8]
//   access at all. It is a dead parameter, transcribed faithfully, and
//   that is also why C2 could turn the recursion into the loop.

VA(0x00444120, 0x3A6)  // dc 0x493a0
void army::processDeath(int fadeElementals)
{
    if (is(1u << 21))
        return;

    removeAura();
    removeBinding();

    if (!m_spellInfluence[60]) {
        if (random(1, 100) < 60)
            g_combatManager->m_playDoh[getOwningSide()] = 1;
        else if (random(1, 100) < 80)
            g_combatManager->m_playYeah[1 - getOwningSide()] = 1;
    }

    // DC 0x49420 calls the canonical member (army.cpp:3512). Complete
    // expands its positive-duration walk here; keep the source call.
    cancelAllSpells();

    m_monInfo.m_attributes |= 0x200000;
    m_allUnitsKilled = 0;

    if (g_combatManager->validHex(m_gridIndex)) {
        hexcell* cell = &g_combatManager->m_cells[m_gridIndex];
        unsigned char twoHex = is(1u << 0);
        hexcell* cell2;
        int gi2;
        if (twoHex) {
            gi2 = m_gridIndex + offsetToFront(-1);
            cell2 = &g_combatManager->m_cells[gi2];
        }
        if (leavesNoBody()) {
            g_combatManager->m_creatureIsDead[m_combatSide][m_bitIndex] = 1;
            g_combatManager->m_someCreaturesVanish = 1;
        } else {
            if (cell->m_bodiesInHex < 14
                && (!twoHex || cell2->m_bodiesInHex < 14)) {
                if (g_combatManager->m_cells[m_gridIndex].hasArmy()) {
                    cell->m_deadArmySide[cell->m_bodiesInHex] =
                        g_combatManager->m_cells[m_gridIndex].m_armySide;
                    cell->m_deadArmySlot[cell->m_bodiesInHex] =
                        g_combatManager->m_cells[m_gridIndex].m_armySlot;
                    cell->m_deadPartOfDouble[cell->m_bodiesInHex] =
                        g_combatManager->m_cells[m_gridIndex].m_partOfDouble;
                    cell->m_bodiesInHex++;
                }
                if (is(1u << 0)) {
                    if (g_combatManager->m_cells[gi2].hasArmy()) {
                        cell2->m_deadArmySide[cell2->m_bodiesInHex] =
                            g_combatManager->m_cells[gi2].m_armySide;
                        cell2->m_deadArmySlot[cell2->m_bodiesInHex] =
                            g_combatManager->m_cells[gi2].m_armySlot;
                        cell2->m_deadPartOfDouble[cell2->m_bodiesInHex] =
                            g_combatManager->m_cells[gi2].m_partOfDouble;
                        cell2->m_bodiesInHex++;
                    }
                }
            }
            g_combatManager->m_highlighterOn = 0;
            cell->m_armySide = -1;
            cell->m_armySlot = -1;
            if (is(1u << 0)) {
                cell2->m_armySide = -1;
                cell2->m_armySlot = -1;
            }
        }
        if (m_mirrorSourceIndex != -1) {
            army* mirror =
                &g_combatManager->m_armies[m_combatSide][m_mirrorSourceIndex];
            if (mirror->m_mirrorDestIndex == m_bitIndex)
                mirror->m_mirrorDestIndex = -1;
        }
        if (m_mirrorDestIndex != -1) {
            army* clone =
                &g_combatManager->m_armies[m_combatSide][m_mirrorDestIndex];
            clone->m_numTroops = 0;
            clone->processDeath(fadeElementals);
        }
    }
}

VA(0x004444d0, 0x3B)  // decorated identity + CancelIndividualSpell edges
void army::cancelSpellType(int spellType)
{
    switch (spellType) {
    case ARMY_CANCEL_SPELLS_AFTER_ATTACK:
        cancelIndividualSpell(59);
        break;
    case ARMY_CANCEL_SPELLS_AFTER_DAMAGE:
        cancelIndividualSpell(62);
        cancelIndividualSpell(70);
        cancelIndividualSpell(74);
        break;
    }
}

// E:\gamedcs\army.cpp:3660
inline void army::adjustHitpoints()
{
    if (m_spellInfluence[SPELL_AGE])
        m_monInfo.m_hitPoints = static_cast<int>(
            m_origHitPoints * m_poisonPenalty * 0.5f + 0.95f);
    else
        m_monInfo.m_hitPoints = static_cast<int>(
            m_origHitPoints * m_poisonPenalty + 0.95f);
    m_topCreatureDamage = min(m_topCreatureDamage, m_monInfo.m_hitPoints - 1);
}

// Take one standing spell off this stack: clear its round row, undo
// whatever the spell had folded into the stack's own words, and pull
// the spell's entry back out of the cast-order queue. Disrupting Ray
// is the one spell that never comes off.

// The switch arms are in Dreamcast source order (jump-table switch): BIND's
// binder teardown, HYPNOTIZE's remove_aura/add_aura helper pair, then the
// seven stat restores. BIND's vector::clear, the aura helper boundary, and
// AGE's adjust_hitpoints boundary are all positive CodeView facts; none may
// be substituted with their lower-level implementation to protect a local score.

// The AGE arm re-reads spellInfluence[SPELL_AGE] AFTER the entry code
// zeroed it, so its 0.5f halving arm is dead at runtime - the
// uninitialized-`level` class of retail quirk: transcribe, do not fix.
// Its rounding constant is +0.95f (read from the image at 0x63b8c8),
// not the usual +0.5f.

// The tail is the deque surgery InitClean's note promised: std::find
// over SpellInfluenceQueue expanded inline (the 0x1000 node-hop is
// Dinkumware's _DEQUESIZ for a 4-byte element), then
// `erase(it)` = `erase(it, it + 1)` with the two 16-byte iterators
// built on the stack - the second and last call site of the
// deque::erase COMDAT at 0x448db0.

VA(0x00444510, 0x3DB)  // anchor-global, dc 0x49748
void army::cancelIndividualSpell(int spell)
{
    if (m_spellInfluence[spell] <= 0)
        return;
    if (spell == SPELL_DISRUPTING_RAY)
        return;
    m_numSpellInfluences--;
    m_spellInfluence[spell] = 0;
    switch (spell) {
    case SPELL_BIND: {
        unsigned i = m_binders.size();
        while (i-- != 0) {
            eraseItem(m_binders[i]->m_boundArmies, this);
        }
        m_binders.clear();
        break;
    }
    case SPELL_HYPNOTIZE:
        removeAura();
        addAura();
        break;
    case SPELL_STONE_SKIN:
        m_monInfo.m_defenseSkill -= m_toughskinBonus;
        break;
    case SPELL_WEAKNESS:
        m_monInfo.m_attackSkill += m_weaknessPenalty;
        break;
    case SPELL_PRAYER:
        m_monInfo.m_attackSkill -= m_prayerBonus;
        m_monInfo.m_defenseSkill -= m_prayerBonus;
        if (!(is(1u << 6)))
            m_monInfo.m_speed -= m_prayerBonus;
        break;
    case SPELL_HASTE:
        if (!(is(1u << 6))) {
            m_monInfo.m_speed -= m_tailwindBonus;
            m_monFrameInfo.m_walkCycleTime = m_origWalkCycleTime;
        }
        break;
    case SPELL_SLOW:
        m_monFrameInfo.m_walkCycleTime = m_origWalkCycleTime;
        break;
    case SPELL_AGE:
        adjustHitpoints();
        break;
    case SPELL_DISEASE:
        m_monInfo.m_attackSkill += m_diseaseAttackPenalty;
        m_monInfo.m_defenseSkill += m_diseaseDefensePenalty;
        break;
    }
    TSpellQueue::iterator it = std::find(m_spellInfluenceQueue.begin(),
                                         m_spellInfluenceQueue.end(), spell);
    if (it != m_spellInfluenceQueue.end()) {
        TSpellQueue::iterator next = it;
#pragma inline_depth(0)
        next += 1;
        m_spellInfluenceQueue.erase(it, next);
#pragma inline_depth()
    }
}

// Cancel spells with positive durations. Complete has 81 spell entries;
// Dreamcast has 80.
// E:\gamedcs\army.cpp:3802, dc 0x499ac
void army::cancelAllSpells()
{
    for (int i = 0; i < 81; i++) {
        if (m_spellInfluence[i] > 0)
            cancelIndividualSpell(i);
    }
}

// Rejected 99.5510% local maximum (2026-08-31): file-local duration and
// Poison helpers, plus duplicated HYPNOTIZE/AGE bodies, manipulated VC6's
// /Ob2 budget but had no Dreamcast definition or line-table boundary. The
// source contracts now forbid restoring that compiler-state scaffold. Its
// useful codegen finding remains: retail's final deque expansion keeps a
// nested copy/iterator update out of line while natural VC6 expands it.

// Rejected local-maximum scaffold. Dreamcast army.cpp:4055 proves that
// SetSpellInfluence calls the named remove_aura boundary here, followed by
// add_aura on line 4056. Keep the exhausted spelling only as negative
// evidence; it may not replace the proven helper again.
#if 0
static void drop_aura_links(army* self)
{
    long i = self->m_auraSources.size();
    while (i-- > 0) {
        std::vector<army*>& clients = self->m_auraSources[i]->m_auraClients;
        unsigned n = clients.size();
        while (n-- != 0) {
            if (clients[n] == self) {
                army** pos = clients.begin() + n;
                clients.erase(pos);
                break;
            }
        }
    }
    {
        std::vector<army*>& links = self->m_auraSources;
        army** first = links.begin();
        army** last = links.end();
        links.erase(first, last);
    }
    long j = self->m_auraClients.size();
    while (j-- > 0) {
        std::vector<army*>& sources = self->m_auraClients[j]->m_auraSources;
        unsigned n = sources.size();
        while (n-- != 0) {
            if (sources[n] == self) {
                army** pos = sources.begin() + n;
                sources.erase(pos);
                break;
            }
        }
    }
    {
        std::vector<army*>& links = self->m_auraClients;
        army** first = links.begin();
        army** last = links.end();
        links.erase(first, last);
    }
}
#endif

// Put one spell ON this stack: pick how many rounds it stands (255 for
// the three that never time out on their own, "until your next turn"
// for Frenzy, the caster's power for everything else), raise-or-return
// if the spell is already standing, otherwise record it, fold its
// per-mastery traits amount into the stack's own words, and append it
// to the cast-order queue. The exact inverse of CancelIndividualSpell
// above, arm for arm.

// Dreamcast lines 3822-3843 recover the duration switch in this caller;
// 4054-4057 recover CancelIndividualSpell -> remove_aura -> add_aura;
// 4067-4068 are two separate Disease mins; 4075 is max followed by the
// adjust_hitpoints boundary; and 4091-4092 are adjust_hitpoints followed by
// one further min. Restoring those statement groups changes the retail CFG
// from 31/135 to 121/135 exact blocks and restores its exact 54-branch
// sequence. The byte score is 96.98% with the older 99.5510% peak banked.

// Complete retail lowers Poison's 0.5 floor through double temporaries, so
// the direct max statement retains that x86-proved type conversion even
// though the SH4/STLport build uses a float max. Binding the result to a
// named value reproduces the local copy but perturbs VC6's whole-function
// inliner state and drops the structure to 110 exact blocks; the direct
// expression is therefore the coherent form. The remaining missing block is
// inside deque::push_back's nested iterator update. A statement-scoped
// inline_depth(1) is byte-flat and is not retained.
VA(0x004448f0, 0xB99)  // anchor-global, dc 0x499e8
void army::setSpellInfluence(int spell, int power, int mastery,
                             const hero* castingHero)
{
    long rounds;
    switch (spell) {
    case SPELL_DISRUPTING_RAY:
    case SPELL_BERSERK:
    case SPELL_BIND:
        rounds = 255;
        break;
    case SPELL_FRENZY:
        if (this == g_combatManager->getCurrentArmy())
            rounds = 1;
        else
            rounds = 2;
        break;
    default:
        rounds = power;
        break;
    }
    if (m_spellInfluence[spell] > 0) {
        if (rounds > m_spellInfluence[spell])
            m_spellInfluence[spell] = rounds;
        if (mastery > m_spellLevel[spell])
            m_spellLevel[spell] = mastery;
        return;
    }
    m_numSpellInfluences++;
    m_spellInfluence[spell] = rounds;
    m_spellLevel[spell] = mastery;
    long amount = g_spellTraits[spell].m_masteryBonus[mastery];
    switch (spell) {
    case SPELL_SHIELD:
        m_shieldFactor = amount / 100.0;
        break;
    case SPELL_AIR_SHIELD:
        m_airShieldFactor = amount / 100.0;
        break;
    case SPELL_FIRE_SHIELD:
        m_fireShieldStrength = amount / 100.0;
        break;
    case SPELL_PROTECTION_FROM_AIR:
        m_protectionFromAirFactor = amount / 100.0;
        break;
    case SPELL_PROTECTION_FROM_FIRE:
        m_protectionFromFireFactor = amount / 100.0;
        break;
    case SPELL_PROTECTION_FROM_WATER:
        m_protectionFromWaterFactor = amount / 100.0;
        break;
    case SPELL_PROTECTION_FROM_EARTH:
        m_protectionFromEarthFactor = amount / 100.0;
        break;
    case SPELL_ANTI_MAGIC: {
        m_antiMagicSpellLevel = amount;
        for (int j = 0; j < 81; j++) {
            if (g_spellTraits[j].m_level < m_antiMagicSpellLevel
                && g_spellTraits[j].m_karma < 0
                && !(g_spellTraits[j].m_flags & 8))
                cancelIndividualSpell(j);
        }
        break;
    }
    case SPELL_BLESS:
        cancelIndividualSpell(SPELL_CURSE);
        m_blessAmount = amount;
        break;
    case SPELL_CURSE:
        cancelIndividualSpell(SPELL_BLESS);
        m_curseAmount = amount;
        break;
    case SPELL_BLOODLUST:
        m_bloodlustAmount = amount;
        if (castingHero)
            m_bloodlustAmount += castingHero->getHeroSpellBonus(
                spell, m_monInfo.m_level, amount);
        break;
    case SPELL_PRECISION:
        m_precisionAmount = amount;
        if (castingHero)
            m_precisionAmount += castingHero->getHeroSpellBonus(
                spell, m_monInfo.m_level, amount);
        break;
    case SPELL_WEAKNESS:
        m_weaknessPenalty = amount;
        if (castingHero)
            m_weaknessPenalty += castingHero->getHeroSpellBonus(
                spell, m_monInfo.m_level, amount);
        if (m_monInfo.m_attackSkill < m_weaknessPenalty)
            m_weaknessPenalty = m_monInfo.m_attackSkill;
        m_monInfo.m_attackSkill = m_monInfo.m_attackSkill - m_weaknessPenalty;
        break;
    case SPELL_STONE_SKIN:
        m_toughskinBonus = amount;
        if (castingHero)
            m_toughskinBonus += castingHero->getHeroSpellBonus(
                spell, m_monInfo.m_level, amount);
        m_monInfo.m_defenseSkill = m_monInfo.m_defenseSkill + m_toughskinBonus;
        break;
    case SPELL_PRAYER:
        m_prayerBonus = amount;
        if (castingHero)
            m_prayerBonus += castingHero->getHeroSpellBonus(
                spell, m_monInfo.m_level, amount);
        m_monInfo.m_attackSkill = m_monInfo.m_attackSkill + m_prayerBonus;
        m_monInfo.m_defenseSkill = m_monInfo.m_defenseSkill + m_prayerBonus;
        if (!(is(1u << 6)))
            m_monInfo.m_speed = m_monInfo.m_speed + m_prayerBonus;
        break;
    case SPELL_MIRTH:
        m_moraleBonus = amount;
        break;
    case SPELL_SORROW:
        m_moralePenalty = amount;
        break;
    case SPELL_FORTUNE:
        m_luckBonus = amount;
        if (castingHero)
            m_luckBonus += castingHero->getHeroSpellBonus(
                spell, m_monInfo.m_level, amount);
        break;
    case SPELL_MISFORTUNE:
        m_luckPenalty = amount;
        break;
    case SPELL_HASTE:
        if (!(is(1u << 6))) {
            cancelIndividualSpell(SPELL_SLOW);
            m_tailwindBonus = amount;
            if (castingHero)
                m_tailwindBonus += castingHero->getHeroSpellBonus(
                    spell, m_monInfo.m_level, amount);
            m_monInfo.m_speed = m_monInfo.m_speed + m_tailwindBonus;
            m_monFrameInfo.m_walkCycleTime =
                static_cast<long>(m_origWalkCycleTime * 0.65);
        }
        break;
    case SPELL_SLOW:
        if (!(is(1u << 6))) {
            cancelIndividualSpell(SPELL_HASTE);
            m_slowFactor = amount / 100.0;
            m_monFrameInfo.m_walkCycleTime =
                static_cast<long>(m_origWalkCycleTime * 1.5);
        }
        break;
    case SPELL_SLAYER:
        m_slayerLevel = mastery;
        break;
    case SPELL_FRENZY:
        m_frenzyFactor = amount / 100.0;
        break;
    case SPELL_COUNTERSTRIKE:
        m_counterstrokeBonus = amount;
        m_retaliationCount = m_retaliationCount + amount;
        break;
    case SPELL_BERSERK:
        cancelIndividualSpell(SPELL_HYPNOTIZE);
        break;
    case SPELL_HYPNOTIZE:
        cancelIndividualSpell(SPELL_BERSERK);
        removeAura();
        addAura();
        break;
    case SPELL_BLIND:
        m_blindFactor = amount / 100.0;
        break;
    case SPELL_DISEASE:
        m_diseaseDefensePenalty = min(2, m_monInfo.m_defenseSkill);
        m_diseaseAttackPenalty = min(2, m_monInfo.m_attackSkill);
        m_monInfo.m_attackSkill = m_monInfo.m_attackSkill - m_diseaseAttackPenalty;
        m_monInfo.m_defenseSkill = m_monInfo.m_defenseSkill - m_diseaseDefensePenalty;
        break;
    case SPELL_POISON: {
        m_poisonPenalty = static_cast<float>(
            cppMax(static_cast<double>(m_poisonPenalty - 0.1f), 0.5));
        adjustHitpoints();
        break;
    }
    case SPELL_AGE:
        adjustHitpoints();
        m_topCreatureDamage = min(m_topCreatureDamage, m_monInfo.m_hitPoints - 1);
        break;
    case SPELL_MAGIC_MIRROR:
        m_backlashChance = amount;
        break;
    case SPELL_FORGETFULNESS:
        m_forgetfulnessLevel = mastery;
        break;
    }
    m_spellInfluenceQueue.push_back(spell);
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:4129
DC_ONLY(0x4a2e8, 0x5E)
void army::decrementSpellRounds()
{
    // @stub
}

// E:\gamedcs\army.cpp:4249
#endif  // @carcass

// The legal victims of a berserked stack: every living stack on either
// side except this one and the arrow towers, kept only while it ties or
// beats the closest distance seen so far.

VA(0x00445490, 0x23B)  // anchor-global, dc 0x4a348
void army::getBerserkTargets(std::vector<army*>& armies) const
{
    unsigned char canShootTarget;
    army* other;
    if (canShoot(0)) {
        canShootTarget = 1;
    } else {
        canShootTarget = 0;
        g_searchArray->seedCombatPosition(this, -1, 127, 0, -1);
    }
    long best = 0;
    for (int side = 0; side < 2; side++) {
        other = g_combatManager->m_armies[side];
        long count = g_combatManager->m_numArmies[side];
        for (; count-- > 0; other++) {
            if (other->is(1u << 21))
                continue;
            if (other == this)
                continue;
            if (other->m_creatureType == ARMY_CREATURE_ARROW_TOWER)
                continue;
            long value;
            if (canShootTarget) {
                value = combatManager::getDistance(m_gridIndex, other->m_gridIndex);
            } else {
                if (!g_combatManager->m_cells[other->m_gridIndex].m_validMove)
                    continue;
                value = g_searchArray->getHex(other->m_gridIndex)->m_cost;
            }
            if (armies.size() > 0 && value > best)
                continue;
            if (value < best)
                armies.clear();
            armies.push_back(other);
            best = value;
        }
    }
}

// A berserked stack picks one of its legal victims at random and does
// whatever it can to it - shoot if it may shoot, swing otherwise - and
// if the list came back empty it simply defends.

VA(0x004456d0, 0x164)  // dc 0x4a480
void army::goBerserk()
{
    std::vector<army*> berserkTargets;
    getBerserkTargets(berserkTargets);
    int count = berserkTargets.size();
    if (count == 0) {
        g_combatManager->m_nextAction = 12;
        return;
    }
    army* target = berserkTargets[random(0, count - 1)];
    if (canShoot(0)) {
        g_combatManager->m_nextAction = 7;
        g_combatManager->m_nextActionGridIndex = target->m_gridIndex;
        if (target->getOwningSide() == getOwningSide())
            g_combatManager->m_playDoh[getOwningSide()] = 1;
        return;
    }
    g_combatManager->berserkAttack(this, target);
}

VA(0x00445840, 0x66)  // dc 0x4a598
long army::getAttackDirection(long ourHex, const army* enemy,
                                long enemyHex) const
{
    long secondHex = enemyHex;
    if (enemy->m_monInfo.m_attributes & 1)
        secondHex = enemyHex + (enemy->m_facing ? 1 : -1);
    for (long direction = 0; direction < 8; direction++) {
        if (direction < COMBAT_DIRECTION_COUNT || (m_monInfo.m_attributes & 1)) {
            long hex = getAdjacentHex(ourHex, direction);
            if (hex == enemyHex || hex == secondHex)
                return direction;
        }
    }
    return -1;
}

VA(0x004458b0, 0x9D)  // dc 0x4a610
inline long army::getAttackDirection(long ourHex, const army* enemy) const
{
    long best = -1;
    long direction = 0;
    for (;;) {
        if (direction < COMBAT_DIRECTION_COUNT || (m_monInfo.m_attributes & 1)) {
            long hex = getAdjacentHex(ourHex, direction);
            if (hex >= 0 && hex < COMBAT_GRID_CELLS
                && enemy == g_combatManager->m_cells[hex].getArmy()) {
                if (direction >= COMBAT_DIRECTION_COUNT)
                    return direction;
                if ((m_facing == 0) == (direction >= 3))
                    return direction;
                if (best == -1)
                    best = direction;
            }
        }
        direction++;
        if (direction >= 8)
            return best;
    }
}

// One stack, one hex, whichever way it travels: a flyer flies, a devil
// teleports, everything else walks. The grid identity is INVALIDATED
// first (side and slot both -1 out of one register, the signed-char CSE
// this class uses everywhere), then the destination is bounds-checked
// and CanFit'ed, and only then does the manager clear its
// last-moved-stack slot, drop the highlighter and mark the mover's own
// hexes.

VA(0x00445950, 0x107)  // dc 0x4a6a4
unsigned char army::simpleMove(int hex, unsigned char restoreFacing)
{
    m_side = -1;
    m_slot = -1;
    if (!g_combatManager->validHex(hex))
        return 0;
    if (!canFit(hex, 0, 0))
        return 0;
    g_combatManager->m_lastMovedArmy = 0;
    g_combatManager->turnOffHighlighter(1);
    g_combatManager->markMovingArmy(this);
    unsigned char moved;
    if (is(1u << 1)) {
        m_pathTarget = hex;
        moved = validFlight(hex, 0);
        if (moved) {
            flyTo(m_pathTarget, restoreFacing);
            moved = 1;
        }
    } else if (m_creatureType == CREATURE_DEVIL
               || m_creatureType == CREATURE_ARCH_DEVIL) {
        m_pathTarget = hex;
        moved = validFlight(hex, 0);
        if (moved) {
            teleportTo(m_pathTarget, restoreFacing);
            moved = 1;
        }
    } else {
        moved = walkTo(hex, restoreFacing);
    }
    g_combatManager->m_lastMovedArmy = this;
    return moved;
}

// simple_move's sibling: one stack attacking one hex. The grid identity
// is invalidated first out of the same single register, the target is
// resolved and rejected if it is missing or is this stack itself, and
// then the stack either shoots or walks its facing around to a
// direction that reaches the target.

VA(0x00445a60, 0x26D)  // dc 0x4a7ac
unsigned char army::attackHex(int hex, unsigned char restoreFacing)
{
    m_side = -1;
    m_slot = -1;
    if (!g_combatManager->validHex(hex))
        return 0;
    hexcell* cell = &g_combatManager->m_cells[hex];
    army* target = cell->getArmy();
    if (!target || target == this)
        return 0;
    g_combatManager->m_lastMovedArmy = 0;
    g_combatManager->turnOffHighlighter(1);
    m_side = cell->m_armySide;
    m_slot = cell->m_armySlot;
    m_pathTarget = hex;
    if (canShoot(0)) {
        rangeAttack();
    } else {
        int direction = getAttackDirection(target);
        if (direction >= 0) {
            unsigned char turned;
            if (needToTurn(direction)) {
                setupAnimation();
                turn(1);
                turned = restoreFacing;
            } else {
                turned = 0;
            }
            doAttack(direction);
            if (!(is(1u << 21)) && turned) {
                setupAnimation();
                turn(1);
            }
        }
    }
    g_combatManager->m_lastMovedArmy = this;
    g_combatManager->checkRebirth();
    return 1;
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:4427
// The old link-order join placed move_to at 0x445cd0; decorated-symbol and
// call-edge evidence instead prove its retail body at 0x445d10 below.
unsigned char army::moveTo(int hex, unsigned char restore_facing)
{
    // @stub
}

#endif  // @carcass

// Every segment, in order, at 0x63b820: the eight-entry run AttackWall
// (0x445d30) re-aims over when the segment it was firing at falls, read
// the same POINTER-RUN way as the four-entry table below - the loop
// limit is the one-past address, which is why AttackWall's inlined
// chooser steps `add edi,4 / cmp edi,0x63b840` and lands exactly on the
// next table's base. NAME FROM THE DREAMCAST LINE TABLE: the static's
// own S_LDATA32 is `all_targets` (dc 0x4aa0c hands it to
// choose_wall_target with the count 8).
DATA(0x0063b820)
static const TWallTargetId g_allTargets[WALL_TARGET_COUNT] = {
    WALL_TARGET_0, WALL_TARGET_1, WALL_TARGET_2, WALL_TARGET_3,
    WALL_TARGET_4, WALL_TARGET_5, WALL_TARGET_6, WALL_TARGET_7
};

// The four segments a missed catapult shot may be re-aimed at, at
// 0x63b840. Read as a POINTER RUN, which is why the loop's limit is the
// one-past address 0x63b850 in its own right: attack_wall (0x445ec0)
// steps `add edi,4 / cmp edi,0x63b850`, and one more army body at
// 0x447530 reads the same pair. NAME FROM THE DREAMCAST LINE TABLE
// (dc 0x4aaac, `walls` with the count 4), replacing the bootstrap
// invention `gRetargetableWalls` this table carried.
DATA(0x0063b840)
static const TWallTargetId g_walls[4] = {
    WALL_TARGET_1, WALL_TARGET_2, WALL_TARGET_5, WALL_TARGET_4
};

// E:\gamedcs\army.cpp:4436
// STATIC, and the retail link order is what says so: this body has no
// retail slot of its own (attack_wall 0x445ec0 runs straight into
// AttackWall's neighbour) while VC6 emits an out-of-line copy of every
// EXTERN function it inlines. Its ONE call site is attack_wall, where
// /Ob2 expands it with both the array and the count constant-folded -
// which is what turns the `targets + count` limit into the bare
// 0x63b850 the retail instruction carries.
DC_ONLY(0x4a8c8, 0xB2)
static TWallTargetId chooseWallTarget(TWallTargetId wall,
                                        const TWallTargetId* targets,
                                        long count)
{
    TWallTargetId chosen = wall;
    long best = 50;
    for (long i = 0; i < count; i++) {
        if (targets[i] == wall)
            continue;
        if (!g_combatManager->validWallTarget(targets[i]))
            continue;
        long distance = abs(targets[i] - wall);
        if (distance > best)
            continue;
        if (distance == best && random(1, 100) > 50)
            continue;
        best = distance;
        chosen = targets[i];
    }
    return chosen;
}

VA(0x00445d10, 0x14)
unsigned char army::moveTo(int hex, unsigned char restoreFacing)
{
    return simpleMove(hex, restoreFacing);
}

// One catapult (or cyclops) bombardment: aim once at the segment the
// grid index falls on, then fire the ballistics row's `shots` at it,
// re-aiming at the nearest other STANDING segment whenever the one
// being fired at comes down.

VA(0x00445d30, 0x189)  // dc 0x4a97c
void army::attackWall(int targetGridIndex)
{
    g_combatManager->m_lastMovedArmy = 0;
    g_combatManager->turnOffHighlighter(1);
    TWallTargetId wall;
    {
        // GetTargetWallIndex returns int in Complete and an enum in Dreamcast.
        wall = TWallTargetId(getTargetWallIndex(targetGridIndex));
    }
    hero* controller = getController();
    long level;
    switch (m_creatureType) {
    case CREATURE_CATAPULT:
        level = controller->m_skillLevel[eSecSkillSiegeBallistics];
        break;

    case ARMY_CREATURE_CYCLOPS:
        level = 1;
        break;

    case ARMY_CREATURE_CYCLOPS_KING:
        level = 2;
        break;
    }
    const type_ballistics_traits& ballistics = g_constBallisticsTraits[level];
    long shots = ballistics.m_shots;
    for (long i = 0; i < shots; i++) {
        attackWall(wall, ballistics);
        if (i + 1 < shots && !g_combatManager->validWallTarget(wall)) {
            wall = chooseWallTarget(wall, g_allTargets, WALL_TARGET_COUNT);
            if (!g_combatManager->validWallTarget(wall))
                break;
        }
    }
    g_combatManager->m_lastMovedArmy = this;
}

// One catapult shot: how much of the aimed segment it takes down, and
// whether it lands on that segment at all.

// The jump table at 0x445fac maps all eight ids: 0 and 6 (the two
// tower rows) take the row's +0x1 chance, 7 (the keep) takes +0x0,
// 3 takes +0x2, and 1/2/4/5 plus everything out of range take +0x3.
VA(0x00445ec0, 0x10C)  // dc 0x4aa3c
void army::attackWall(TWallTargetId wall,
                       const type_ballistics_traits& ballistics)
{
    long levels;
    int roll = random(1, 100);
    for (levels = 0; levels < 2; levels++) {
        roll -= ballistics.m_levelChance[levels];
        if (roll <= 0)
            break;
    }
    long chance;
    switch (wall) {
    case WALL_TARGET_0:
    case WALL_TARGET_6:
        chance = ballistics.m_chanceToHitTower;
        break;

    case WALL_TARGET_7:
        chance = ballistics.m_chanceToHitMainBuilding;
        break;

    case WALL_TARGET_3:
        chance = ballistics.m_chanceToHitDrawbridge;
        break;

    default:
        chance = ballistics.m_chanceToHitWall;
        break;
    }
    if (random(1, 100) > chance) {
        wall = chooseWallTarget(wall, g_walls, 4);
    }
    attackWall(wall, levels);
}

// E:\gamedcs\army.cpp:4577
// The landed shot: aim from the catapult's launch point at the
// segment's own impact point (wallTargets' +0x4/+0x6 pair, sliced by
// this body), pick the ranged pose from the same +-25-degree atan the
// cast path uses, play the attack cycle, fly the missile, and run the
// explosion sprite over the wall - applying the damage on frame FIVE
// of that explosion, not before. A quick combat skips all of it and
// just applies the damage.

// The angle machinery is cast_spell's, statement for statement
// (missileRadiansToDegrees, the +-25 float pool pair, the
// `(dy > 0) ? -90 : 90` vertical arm); the first estimate reads the
// straight pair iMissileOffset[2]/[3], the aimed shot re-reads
// [2*pose]/[2*pose+1] with the chosen pose.

// SPELLINGS THE BYTES FORCED, each measured:
//   - dy is declared AFTER the abs-of-dx branch (81.42 -> 86.67):
//     startY must stay live across the jns or C2 folds it away.
//   - both name ternaries are spelled `levelsDestroyed == 0 ?
//     <miss form> : <hit form>` - the miss operand loads first and the
//     je jumps the hit overwrite, retail's exact arm order.
//   - the bounds stores and the clamp chain each go through their own
//     block-scoped TDrawbridgeBounds& (87.47 -> 89.70): longhand
//     spellings reload gpCombatManager after every aliasing store,
//     where retail materialises each group's base exactly once.

//   - the explosion bounds are computed BEFORE any of them is stored,
//     and `bottom` is computed before `right` (89.6998 -> 91.4600).
//     Retail's sequence is halfWidth / x / halfHeight / y / Height -
//     halfHeight + targetY - 1 / Width - halfWidth + targetX - 1 and only
//     then the four stores; with the last two written inline in the store
//     statements VC6 interleaves compute and store and homes nothing.
//     A note here recorded "precomputing right/bottom as named locals
//     (89.47)" as rejected - it is the right edit in the wrong ORDER;
//     `right` before `bottom` is what loses.

// Residual (91.46%): the register-homing family. Retail's frame is
// 0x34 with x/y/halfWidth homed in fresh bottom slots (-0x34/-0x30/
// -0x14) and targetX carried to the explosion block in EBX; ours is now
// 0x24 and still reloads three of them. Branch sequences AGREE (25/25).
// DC type/source audit (2026-08-21): its `destX`, `destY` and `numFrames`
// locals are `const int`, while `startY` is plain `int`. Restoring those
// types and spelling numFrames as the required conditional initializer are
// byte-flat at 91.460045%. Conventional release VERIFY is also byte-flat,
// both for the wall-id domain at entry and `explosion != 0` immediately
// before the bounds expressions. Neither source class creates retail's four
// extra frame slots; the residual remains allocator state.
VA(0x00445fd0, 0x526)  // anchor-callee, dc 0x4aacc
void army::attackWall(TWallTargetId wall, long levelsDestroyed)
{
    if (static_cast<const combatManager*>(g_combatManager)
            ->isQuickCombat()) {
        g_combatManager->damageWall(wall, levelsDestroyed);
        return;
    }

    const int targetX = combatManager::s_wallTargets[wall].m_hitX;
    const int targetY = combatManager::s_wallTargets[wall].m_hitY;

    long startX;
    if (m_facing == 1)
        startX = g_combatManager->m_cells[m_gridIndex].m_refX
                 + m_monFrameInfo.m_missileOffset[2];
    else
        startX = g_combatManager->m_cells[m_gridIndex].m_refX
                 - m_monFrameInfo.m_missileOffset[2];
    int startY = g_combatManager->m_cells[m_gridIndex].m_refY
                 + m_monFrameInfo.m_missileOffset[3];

    // dy is declared AFTER the abs: startY has to stay live across the
    // branch, which is what keeps retail from folding it into the
    // subtraction (the jns/jge polarity at +0xbb is the same artifact).
    long dx = targetX - startX;
    if (dx < 0)
        dx = -dx;
    long dy = targetY - startY;
    float angle;
    if (dx == 0) {
        angle = (dy > 0) ? -90 : 90;
    } else {
        angle = static_cast<float>(
            atan(static_cast<double>(dy) / dx)
            * DATA_COMPGEN(0x0063b8f0, missileRadiansToDegrees,
                           57.2957763671875));
    }
    long pose;
    if (angle > 25.0f) {
        m_currFrameType = cs_range_ur;
        pose = 0;
    } else if (angle > -25.0f) {
        m_currFrameType = cs_range_r;
        pose = 1;
    } else {
        m_currFrameType = cs_range_dr;
        pose = 2;
    }
    if (m_facing == 1)
        startX = g_combatManager->m_cells[m_gridIndex].m_refX
                 + m_monFrameInfo.m_missileOffset[2 * pose];
    else
        startX = g_combatManager->m_cells[m_gridIndex].m_refX
                 - m_monFrameInfo.m_missileOffset[2 * pose];
    startY = g_combatManager->m_cells[m_gridIndex].m_refY
             + m_monFrameInfo.m_missileOffset[2 * pose + 1];

    sample* wallSample = ResourceManager::getSample(
        levelsDestroyed == 0 ? DATA_COMPGEN(0x00660a84, wallMissSampleName,
                                            "WallMiss.82m")
                             : DATA_COMPGEN(0x00660a78, wallHitSampleName,
                                            "WallHit.82m"));
    g_combatManager->resetLimitCreature();
    g_combatManager->markCreatureEffect(m_combatSide, m_bitIndex);
    g_combatManager->computeMaxExtent();
    ds_memsample* shootMemSample =
        g_soundManager->memorySample(m_armySample[SHOOT_SAMPLE]);

    const int frames = m_monFrameInfo.m_attackFrames <= 0
                           ? m_stdIcon->getNumFrames(m_currFrameType)
                           : m_monFrameInfo.m_attackFrames;
    long delay = m_monFrameInfo.m_attackStartCycleTime / frames;
    for (m_currFrameIndex = 0; m_currFrameIndex < frames; m_currFrameIndex++) {
        g_combatManager->drawFrame(1, 1, 0, delay, 1, 1);
    }

    m_currFrameType = cs_wait;
    m_currFrameIndex = 0;
    g_combatManager->shootBallisticMissile(startX, startY, targetX, targetY,
                                           m_missileIcon);
    ds_memsample* wallMemSample = g_soundManager->memorySample(wallSample);

    CSprite* explosion = ResourceManager::getSprite(
        levelsDestroyed == 0
            ? DATA_COMPGEN(0x00660a6c, rockSpriteName, "CSGRCK.DEF")
            : DATA_COMPGEN(0x00660a60, explosionSpriteName, "SGEXPL.DEF"));
    long halfWidth = explosion->getWidth() / 2;
    long x = targetX - halfWidth;
    long halfHeight = explosion->getHeight() / 2;
    long y = targetY - halfHeight;
    long bottom = explosion->getHeight() - halfHeight + targetY - 1;
    long right = explosion->getWidth() - halfWidth + targetX - 1;
    {
        TDrawbridgeBounds& bounds = g_combatManager->m_drawbridgeBounds;
        bounds.m_minX = x;
        bounds.m_minY = y;
        bounds.m_maxX = right;
        bounds.m_maxY = bottom;
    }
    {
        TDrawbridgeBounds& bounds = g_combatManager->m_drawbridgeBounds;
        if (bounds.m_minX < g_combatDrawLimits694f18.m_minX)
            bounds.m_minX = g_combatDrawLimits694f18.m_minX;
        if (bounds.m_minY < g_combatDrawLimits694f18.m_minY)
            bounds.m_minY = g_combatDrawLimits694f18.m_minY;
        if (bounds.m_maxX > g_combatDrawLimits694f18.m_maxX)
            bounds.m_maxX = g_combatDrawLimits694f18.m_maxX;
        if (bounds.m_maxY > g_combatDrawLimits694f18.m_maxY)
            bounds.m_maxY = g_combatDrawLimits694f18.m_maxY;
    }

    for (long frame = 0; frame < explosion->getNumFrames(0); frame++) {
        if (frame == combatManager::WALL_EXPLOSION_HIT_FRAME
            && levelsDestroyed != 0)
            g_combatManager->damageWall(wall, levelsDestroyed);
        g_combatManager->drawFrame(0, 0, 1, 100, 0, 1);
        explosion->draw(0, frame, 0, 0,
                        g_combatManager->m_drawbridgeBounds.m_maxX
                            - g_combatManager->m_drawbridgeBounds.m_minX
                            + 1,
                        g_combatManager->m_drawbridgeBounds.m_maxY
                            - g_combatManager->m_drawbridgeBounds.m_minY
                            + 1,
                        g_windowManager->m_screenBitmap->getMap(0, 0),
                        targetX - explosion->getWidth() / 2,
                        targetY - explosion->getHeight() / 2,
                        g_windowManager->m_screenBitmap->getWidth(),
                        g_windowManager->m_screenBitmap->getHeight(),
                        g_windowManager->m_screenBitmap->getPitch(), 0, 1);
        g_windowManager->updateScreen(
            g_combatManager->m_drawbridgeBounds.m_minX,
            g_combatManager->m_drawbridgeBounds.m_minY,
            g_combatManager->m_drawbridgeBounds.m_maxX
                - g_combatManager->m_drawbridgeBounds.m_minX + 1,
            g_combatManager->m_drawbridgeBounds.m_maxY
                - g_combatManager->m_drawbridgeBounds.m_minY + 1);
    }
    explosion->dispose();
    g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
    g_soundManager->waitSample(shootMemSample, -1);
    g_soundManager->waitSample(wallMemSample, -1);
    if (wallSample)
        wallSample->dispose();
    cancelSpellType(ARMY_CANCEL_SPELLS_AFTER_ATTACK);
}

VA(0x00446500, 0x126)
void army::cure(int level, int spellPower, const hero* castingHero)
{
    m_poisonPenalty = 1.0f;
    if (m_spellInfluence[SPELL_AGE])
        m_monInfo.m_hitPoints = static_cast<int>(
            static_cast<float>(m_origHitPoints) * 0.5f + 0.95f);
    else
        m_monInfo.m_hitPoints = static_cast<int>(
            static_cast<float>(m_origHitPoints) + 0.95f);
    m_topCreatureDamage = cppMin(m_topCreatureDamage, m_monInfo.m_hitPoints - 1);
    cancelIndividualSpell(SPELL_CURSE);
    cancelIndividualSpell(SPELL_WEAKNESS);
    cancelIndividualSpell(SPELL_SORROW);
    cancelIndividualSpell(SPELL_MISFORTUNE);
    cancelIndividualSpell(SPELL_SLOW);
    cancelIndividualSpell(SPELL_BERSERK);
    cancelIndividualSpell(SPELL_HYPNOTIZE);
    cancelIndividualSpell(SPELL_FORGETFULNESS);
    cancelIndividualSpell(SPELL_BLIND);
    cancelIndividualSpell(SPELL_STONE);
    cancelIndividualSpell(SPELL_POISON);
    cancelIndividualSpell(SPELL_DISEASE);
    cancelIndividualSpell(SPELL_PARALYZE);
    cancelIndividualSpell(SPELL_AGE);
    int healed = g_spellTraits[SPELL_CURE].m_masteryBonus[level]
                 + g_spellTraits[SPELL_CURE].m_powerFactor * spellPower;
    if (castingHero)
        healed += castingHero->getHeroSpellBonus(SPELL_CURE, m_monInfo.m_level,
                                                  healed);
    m_topCreatureDamage -= healed;
    if (m_topCreatureDamage < 0)
        m_topCreatureDamage = 0;
}

VA(0x00446630, 0x2E)  // dc 0x4b14c
int army::midY() const
{
    return g_combatManager->m_cells[m_gridIndex].m_refY - m_imageHeight / 2;
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:4779
DC_ONLY(0x4b170, 0x20)
int army::topY() const
{
    // @stub
}

// E:\gamedcs\army.cpp:4784
DC_ONLY(0x4b190, 0x16)
int army::bottomY() const
{
    // @stub
}

#endif  // @carcass

VA(0x00446660, 0x35)  // dc 0x4b1a8
int army::midX() const
{
    int x = g_combatManager->m_cells[m_gridIndex].m_refX;
    if (m_monInfo.m_attributes & 1)
        x += m_facing ? 22 : -22;
    return x;
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:4800
DC_ONLY(0x4b20c, 0x6C)
int army::rightX() const
{
    // @stub
}

// E:\gamedcs\army.cpp:4812
DC_ONLY(0x4b278, 0x6C)
int army::leftX() const
{
    // @stub
}

// E:\gamedcs\army.cpp:4825
DC_ONLY(0x4b2e4, 0x6E)
int army::frontX() const
{
    // @stub
}

// E:\gamedcs\army.cpp:4839
#endif  // @carcass

VA(0x004466a0, 0x1E)  // dc 0x4b354
int army::getSecondGridIndex() const
{
    if (!is(1u))
        return m_gridIndex;
    return m_gridIndex + offsetToFront(-1);
}

VA(0x004466c0, 0x5A)  // dc 0x4b398
unsigned char army::isAdjacent(int hex) const
{
    if (g_combatManager->isAdjacent(m_gridIndex, hex))
        return 1;
    if (m_monInfo.m_attributes & 1) {
        int secondHex = m_gridIndex + (m_facing ? 1 : -1);
        return g_combatManager->isAdjacent(secondHex, hex);
    }
    return 0;
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:4864
DC_ONLY(0x4b3e8, 0x3E)
unsigned char army::isAdjacent(const army& other_army) const
{
    // @stub
}

// E:\gamedcs\army.cpp:4881
DC_ONLY(0x4b428, 0x2A)
int army::otherArmyAdjacent(int OAgroup, int OAindex)
{
    // @stub
}

// E:\gamedcs\army.cpp:4891
#endif  // @carcass

// Residual (91.7125%): control flow is exact. Candidate CL hoists the literal
// 1 into EBX, costing a push/pop and replacing retail's immediate tests,
// stores, and final animation argument with BL. Literal facing values and
// hexcell::field_1a signedness are byte-flat; the remaining difference is the
// register-homing family, not a reason to flatten either Is boundary.
VA(0x00446720, 0x107)  // anchor-global, dc 0x4b454
void army::turn(unsigned char animateTurn)
{
    if (m_facing == FACING_ATTACKER) {
        if (animateTurn)
            playAnimation(9, -1, 0);
        m_facing = FACING_DEFENDER;
        if (is(1u)) {
            m_gridIndex--;
            g_combatManager->m_cells[m_gridIndex].m_partOfDouble = 0;
            g_combatManager->m_cells[m_gridIndex + 1].m_partOfDouble = 1;
        }
        if (animateTurn) {
            playAnimation(8, -1, 0);
            playAnimation(2, 1, 0);
        }
    } else {
        if (animateTurn)
            playAnimation(7, -1, 0);
        m_facing = FACING_ATTACKER;
        if (is(1u)) {
            m_gridIndex++;
            g_combatManager->m_cells[m_gridIndex].m_partOfDouble = 1;
            g_combatManager->m_cells[m_gridIndex - 1].m_partOfDouble = 0;
        }
        if (animateTurn) {
            playAnimation(10, -1, 0);
            playAnimation(2, 1, 0);
        }
    }
}

// Capture the battlefield without this stack on it, once, before an
// animation starts: widen the draw limits to the whole combat area,
// draw this stack's cell into the buffer, take one full frame with
// LetsPretendImNotHere raised, and blit the 800x600 result into the
// manager's backup bitmap. Quick combat skips the lot.
VA(0x00446830, 0x103)  // dc 0x4b558
void army::setupAnimation()
{
    if (g_combatManager->isQuickCombat())
        return;
    hexcell* cell = &g_combatManager->m_cells[m_gridIndex];
    g_combatManager->m_drawbridgeBounds = g_combatAreaLimits;
    g_combatManager->m_saveBiggestExtent = 1;
    g_combatManager->m_computeExtentOnly = 1;
    drawToBuffer(cell->m_refX, cell->m_refY, 0);
    g_combatManager->m_computeExtentOnly = 0;
    g_combatManager->m_saveBiggestExtent = 0;
    m_letsPretendImNotHere = 1;
    g_combatManager->drawFrame(0, 0, 0, 0, 1, 0);
    m_letsPretendImNotHere = 0;
    g_windowManager->m_screenBitmap->draw(
        0, 0, 800, 600, g_combatManager->m_saveScreenPostGrid->getMap(0, 0), 0, 0,
        g_combatManager->m_saveScreenPostGrid->getWidth(),
        g_combatManager->m_saveScreenPostGrid->getHeight(),
        g_combatManager->m_saveScreenPostGrid->getPitch(), false);
    g_combatManager->m_backgroundDrawn = 0;
}

// Run one animation sequence to the screen, frame by frame, pacing
// itself off the adventure timer. Every frame first repaints the clean
// background SetupAnimation captured over the extent the LAST frame
// dirtied, then redraws this stack into the buffer with the extent
// widened by half a hex either side, unions the two rectangles, takes a
// full frame and blits exactly that union. Quick combat skips the lot.
VA(0x00446940, 0x2FE)  // dc 0x4b624
void army::playAnimation(int sequence, int nframes, int startFrame)
{
    if (g_combatManager->isQuickCombat())
        return;
    if (nframes < 0)
        nframes = m_stdIcon->getNumFrames(sequence) - startFrame;
    m_currFrameType = sequence;
    int frameDelay;
    if (sequence == 0)
        frameDelay = static_cast<int>(
            static_cast<float>(m_monFrameInfo.m_walkCycleTime)
            * g_combatSpeedFactors[g_unnamed698758.m_combatSpeed]
            / static_cast<float>(m_stdIcon->getNumFrames(0)));
    else
        frameDelay = static_cast<int>(
            g_combatSpeedFactors[g_unnamed698758.m_combatSpeed] * 100.0f);

    TDrawbridgeBounds bounds = g_combatManager->m_drawbridgeBounds;
    for (m_currFrameIndex = startFrame;
         m_currFrameIndex < nframes + startFrame; m_currFrameIndex++) {
        TDrawbridgeBounds frame = bounds;
        g_combatManager->m_saveScreenPostGrid->draw(
            frame.m_minX, frame.m_minY,
            frame.m_maxX - frame.m_minX + 1,
            frame.m_maxY - frame.m_minY + 1,
            g_windowManager->m_screenBitmap->getMap(0, 0),
            frame.m_minX, frame.m_minY,
            g_windowManager->m_screenBitmap->getWidth(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getPitch(), false);

        g_combatManager->m_drawbridgeBounds = g_combatAreaLimits;
        g_combatManager->m_saveBiggestExtent = 1;
        g_combatManager->m_computeExtentOnly = 1;
        drawToBuffer(g_combatManager->m_cells[m_gridIndex].m_refX,
                     g_combatManager->m_cells[m_gridIndex].m_refY, 0);
        g_combatManager->m_computeExtentOnly = 0;
        g_combatManager->m_saveBiggestExtent = 0;

        g_combatManager->m_drawbridgeBounds.m_minX -= 17;
        g_combatManager->m_drawbridgeBounds.m_maxX += 17;
        bounds = g_combatManager->m_drawbridgeBounds;
        if (frame.m_minX > bounds.m_minX)
            frame.m_minX = bounds.m_minX;
        if (frame.m_minY > bounds.m_minY)
            frame.m_minY = bounds.m_minY;
        if (frame.m_maxX < bounds.m_maxX)
            frame.m_maxX = bounds.m_maxX;
        if (frame.m_maxY < bounds.m_maxY)
            frame.m_maxY = bounds.m_maxY;
        g_combatManager->m_drawbridgeBounds = frame;

        g_combatManager->m_limitToExtent = 1;
        g_combatManager->drawFrame(0, 0, 0, 0, 0, 0);
        g_combatManager->m_limitToExtent = 0;
        GameTime::delayTil(g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT]);
        g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT]
            = GameTime::nextFrameTime(
                g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT], frameDelay);
        g_windowManager->updateScreen(
            frame.m_minX, frame.m_minY,
            frame.m_maxX - frame.m_minX + 1,
            frame.m_maxY - frame.m_minY + 1);
    }

    if (nframes > 0)
        m_currFrameIndex--;
}

// "Can this stack stand on destIndex?" - the placement predicate
// searchArray::mark_teleport (0x4b2ff0) already calls with (hex, 0, 0).
// Three copies of the same on-grid test: never a margin column, never a
// blocked hex, and the cell either empty or already MINE. A one-hex
// stack is done after the first; a two-hex stack must also fit its
// trailing hex, and when that fails and shifting is allowed it retries
// one hex the OTHER way and reports the shifted anchor through
// iNewDestIndex.

VA(0x00446c40, 0x1E1)  // dc 0x4b8c4
int army::canFit(int destIndex, int allowShifting, int* newDestIndex) const
{
    if (newDestIndex)
        *newDestIndex = destIndex;

    if (destIndex < 0 || destIndex >= COMBAT_GRID_CELLS)
        return 0;
    if (destIndex % COMBAT_GRID_ROW_STRIDE == 0
            || destIndex % COMBAT_GRID_ROW_STRIDE == COMBAT_GRID_LAST_COLUMN)
        return 0;

    hexcell* cell = &g_combatManager->m_cells[destIndex];
    if (g_combatManager->hexIsBlocked(destIndex))
        return 0;
    if (cell->m_armySide >= 0) {
        if (cell->m_armySide != m_combatSide)
            return 0;
        if (cell->m_armySlot != m_bitIndex)
            return 0;
    }

    if (!(m_monInfo.m_attributes & 1))
        return 1;

    int otherIndex = getAdjacentCellIndex(destIndex, m_facing ? 1 : 4);
    if (otherIndex < 0 || otherIndex >= COMBAT_GRID_CELLS)
        return 0;
    if (otherIndex % COMBAT_GRID_ROW_STRIDE == 0
            || otherIndex % COMBAT_GRID_ROW_STRIDE == COMBAT_GRID_LAST_COLUMN)
        return 0;
    hexcell* otherCell = &g_combatManager->m_cells[otherIndex];
    if (!g_combatManager->hexIsBlocked(otherIndex)) {
        if (otherCell->m_armySide < 0
                || (otherCell->m_armySide == m_combatSide
                    && otherCell->m_armySlot == m_bitIndex))
            return 1;
    }

    if (allowShifting) {
        int shiftedIndex = getAdjacentCellIndex(destIndex, m_facing ? 4 : 1);
        if (shiftedIndex < 0 || shiftedIndex >= COMBAT_GRID_CELLS)
            return 0;
        if (shiftedIndex % COMBAT_GRID_ROW_STRIDE == 0
                || shiftedIndex % COMBAT_GRID_ROW_STRIDE
                       == COMBAT_GRID_LAST_COLUMN)
            return 0;
        hexcell* shiftedCell = &g_combatManager->m_cells[shiftedIndex];
        if (g_combatManager->hexIsBlocked(shiftedIndex))
            return 0;
        if (shiftedCell->m_armySide >= 0) {
            if (shiftedCell->m_armySide != g_combatManager->m_actingSide)
                return 0;
            if (shiftedCell->m_armySlot != g_combatManager->m_actingSlot)
                return 0;
        }
        if (newDestIndex)
            *newDestIndex = shiftedIndex;
        return 1;
    }
    return 0;
}

VA(0x00446e30, 0x2E1)  // dc 0x4ba88
void army::newTurn()
{
    if (m_resetThisRound != 0)
        return;
    m_resetThisRound = 1;
    if (is(1u << 27)) {
        m_monInfo.m_defenseSkill -= m_defendBonus;
        m_monInfo.m_attributes &= ~0x08000000;
    }
    if (g_combatManager->m_creaturePlacement != 0)
        return;
    if (m_topCreatureDamage > 0) {
        if (m_creatureType == CREATURE_WIGHT
            || m_creatureType == ARMY_CREATURE_WRAITH
            || m_creatureType == CREATURE_TROLL
            || ((g_creatureTypeTraits[m_creatureType].m_attributes
                 & g_ctaAlive)
                && g_combatManager->m_heroes[m_combatSide] != 0
                && g_combatManager->m_heroes[m_combatSide]
                       ->isWieldingArtifact(ARTIFACT_ELIXIR_OF_LIFE))) {
            long heal = m_topCreatureDamage;
            long amount = heal > 50 ? 50 : heal;
            m_topCreatureDamage = heal - amount;
            if (!static_cast<const combatManager*>(g_combatManager)
                     ->isQuickCombat()) {
                SAMPLE2 sample = loadPlaySample(
                    DATA_COMPGEN(0x00660a94, regenerSampleName,
                                 "Regener.wav"));
                std::string text;
                if (m_numTroops == 1)
                    text = formatString(
                        g_generalText->getText(371),
                        getName());
                else
                    text = formatString(
                        g_generalText->getText(372),
                        getName());
                g_combatManager->m_combatWindow->combatMessage(
                    text.c_str(), 1, 0);
                g_combatManager->spellEffect(
                    combatManager::eSpellEffectRegeneration, this, 100,
                    0);
                waitEndSample(sample, -1);
            }
        }
    }
    if (m_spellInfluence[SPELL_FRENZY] != 0) {
        if (m_spellInfluence[SPELL_FRENZY] > 1)
            m_spellInfluence[SPELL_FRENZY]--;
        else
            cancelIndividualSpell(SPELL_FRENZY);
    }
}

// Everything a stack does between rounds: the round-scoped creature
// bits cleared, the retaliation allowance rebuilt, every standing spell
// row decremented, and - if poison is on it - one more slice off its
// maximum hit points with the sample, the pow effect and the message
// that go with it. A summoned stack counts down here too, and the round
// it reaches zero it is sent to ProcessDeath.

VA(0x00447120, 0x20A)  // dc 0x4bc84
void army::resetRound()
{
    if (m_numTroops <= 0)
        return;

    m_monInfo.m_attributes &= 0xf8ffffff;
    m_resetThisRound = 0;
    m_retaliationCount = 1;
    if (m_creatureType == ARMY_CREATURE_GRIFFIN)
        m_retaliationCount = 2;
    if (m_creatureType == ARMY_CREATURE_ROYAL_GRIFFIN)
        m_retaliationCount = 5000;
    if (m_spellInfluence[SPELL_COUNTERSTRIKE])
        m_retaliationCount += m_counterstrokeBonus;
    if (is(1u << 6))
        m_retaliationCount = 0;

    if (g_combatManager->m_creaturePlacement)
        return;

    for (int spell = 0; spell < 81; spell++) {
        if (m_spellInfluence[spell] > 0 && spell != SPELL_FRENZY) {
            if (m_spellInfluence[spell] == 1)
                cancelIndividualSpell(spell);
            else
                m_spellInfluence[spell]--;
        }
    }

    if (m_roundsLeftBeforeVanish > 0)
        m_roundsLeftBeforeVanish--;

    if (m_spellInfluence[SPELL_POISON] > 0) {
        int oldHitPoints = m_monInfo.m_hitPoints;
        double factor = cppMax<double>(m_poisonPenalty - 0.1f, 0.5);
        m_poisonPenalty = static_cast<float>(factor);
        if (m_spellInfluence[SPELL_AGE])
            m_monInfo.m_hitPoints = static_cast<int>(
                m_origHitPoints * m_poisonPenalty * 0.5f + 0.95f);
        else
            m_monInfo.m_hitPoints = static_cast<int>(
                m_origHitPoints * m_poisonPenalty + 0.95f);
        m_topCreatureDamage = cppMin(m_topCreatureDamage, m_monInfo.m_hitPoints - 1);
        if (oldHitPoints - m_monInfo.m_hitPoints > 0) {
            m_showPowEffect = 1;
            m_someUnitsDamaged = 1;
            SAMPLE2 sample;
            if (!g_combatManager->isQuickCombat()) {
                sample = loadPlaySample(
                    DATA_COMPGEN(0x00660aa0, poisonWaveName, "Poison.wav"));
                g_combatManager->showSpellMessage(1, SPELL_POISON, this);
            }
            g_combatManager->powEffect(SPELL_SUMMON_EARTH_ELEMENTAL, 1);
            if (!g_combatManager->isQuickCombat())
                waitEndSample(sample, -1);
        }
    }

    if (m_roundsLeftBeforeVanish == 0)
        processDeath(1);
}

VA(0x00447330, 0x9C)  // dc 0x4bd80
long army::getResurrectionSize(const army* target) const
{
    if (m_creatureType == CREATURE_ARCHANGEL) {
        int missing = target->m_origNumTroops - target->m_numTroops;
        int raised = m_numTroops * 100 / target->m_monInfo.m_hitPoints;
        return cppMin(raised, missing);
    }
    int totalLife = target->m_monInfo.m_hitPoints * target->m_origNumTroops;
    int raised = cppMin(totalLife, m_numTroops * 50)
        / g_creatureTypeTraits[ARMY_CREATURE_DEMON].m_hitPoints;
    return cppMin(raised, target->m_origNumTroops);
}

VA(0x004473d0, 0x13D)  // dc 0x4be64
bool army::canCastResurrect(long hex) const
{
    if ((m_creatureType != CREATURE_ARCHANGEL
         && m_creatureType != ARMY_CREATURE_PIT_LORD)
        || m_monInfo.m_hasSpell <= 0)
        return 0;
    long side = getControllingSide();
    if (!g_combatManager->canCastSpells(side, 0))
        return 0;
    army* target;
    if (m_creatureType == ARMY_CREATURE_PIT_LORD)
        target = g_combatManager->findDemonicResurrectionTarget(side, hex);
    else
        target = g_combatManager->findResurrectionTarget(side, hex, 1);
    if (!target)
        return 0;
    return getResurrectionSize(target) > 0;
}

// The Faerie Dragon's lottery: (spell, weight) pairs, -1-terminated,
// read from the hash-verified image at 0x63b850 (.rdata, so const).
// The eight direct-damage spells at their proven ids.
DATA(0x0063b850)
static const int g_faerieDragonSpells[] = {
    SPELL_ICE_BOLT,        22,
    SPELL_LIGHTNING_BOLT,  22,
    SPELL_FIREBALL,        21,
    SPELL_MAGIC_ARROW,     10,
    SPELL_FROST_RING,      10,
    SPELL_CHAIN_LIGHTNING, 5,
    SPELL_METEOR_SHOWER,   5,
    SPELL_INFERNO,         5,
    -1,
};

VA(0x00447510, 0x1A8)
void army::faerieDragonSpell()
{
    long total = 0;
    const int* p = g_faerieDragonSpells;
    if (m_monInfo.m_hasSpell == 0)
        return;
    while (*p >= 0) {
        total += *++p;
        ++p;
    }
    long roll = rand() % total;
    for (p = g_faerieDragonSpells; *p >= 0; p += 2) {
        roll -= p[1];
        if (roll < 0) {
            m_faerieDragonSpell = *p;
            break;
        }
    }

    if (static_cast<const combatManager*>(g_combatManager)
            ->isQuickCombat())
        return;
    if (g_combatManager->m_sideIsAi[getControllingSide()]) {
        const char* fmt =
            m_numTroops == 1
                ? DATA_COMPGEN(0x00660ad0, faerieReadiesFormat,
                               "The %s readies %s (press F to cast)")
                : DATA_COMPGEN(0x00660aac, faerieReadyFormat,
                               "The %s ready %s (press F to cast)");
        g_combatManager->m_combatWindow->combatMessage(
            formatString(fmt, ::getArmyName(m_creatureType, m_numTroops),
                          g_spellTraits[m_faerieDragonSpell].m_name)
                .c_str(),
            1, 0);
    }
}

VA(0x004476c0, 0x3BA)  // dc 0x4beec
unsigned char army::canCastSpell(long hex) const
{
    if (m_monInfo.m_hasSpell == 0)
        return 0;
    if (!g_combatManager->canCastSpells(getControllingSide(), 0))
        return 0;
    if (hex < 0 || hex >= COMBAT_GRID_CELLS)
        return 0;
    army* target = g_combatManager->m_cells[hex].getArmy();
    switch (m_creatureType) {
    case CREATURE_ARCHANGEL:
    case ARMY_CREATURE_PIT_LORD:
        return canCastResurrect(hex);
    case CREATURE_MASTER_GENIE:
        return target && getValidCaliphSpells(target) > 0;
    case CREATURE_FAERIE_DRAGON:
        return target
               && g_combatManager->validSpellTargetArmy(m_faerieDragonSpell,
                                                  getControllingSide(),
                                                  target, 1, 1);
    case CREATURE_STORM_ELEMENTAL:
        return target
               && g_combatManager->validSpellTargetArmy(SPELL_PROTECTION_FROM_AIR,
                                                  getControllingSide(),
                                                  target, 1, 1);
    case CREATURE_ICE_ELEMENTAL:
        return target
               && g_combatManager->validSpellTargetArmy(
                SPELL_PROTECTION_FROM_WATER, getControllingSide(), target,
                1, 1);
    case CREATURE_ENERGY_ELEMENTAL:
        return target
               && g_combatManager->validSpellTargetArmy(SPELL_PROTECTION_FROM_FIRE,
                                                  getControllingSide(),
                                                  target, 1, 1);
    case CREATURE_MAGMA_ELEMENTAL:
        return target
               && g_combatManager->validSpellTargetArmy(
                SPELL_PROTECTION_FROM_EARTH, getControllingSide(), target,
                1, 1);
    case CREATURE_OGRE_MAGE:
        return target
               && g_combatManager->validSpellTargetArmy(SPELL_BLOODLUST,
                                                  getControllingSide(),
                                                  target, 1, 1);
    }
    return 0;
}

// RETAIL-ONLY: the shared spell-validity worker army.h describes -
// is_valid_caliph_spell (0x447eb0) TAIL-JUMPS to it, can_cast_spell above
// calls it, and Unnamed447fe0 calls it twice. Two fastcall register
// arguments and a bare `ret`, so it is a free function, which rules out
// the three one-argument group_has_* statics the DC roster has left in
// this bracket. Name is army.h's, a bootstrap invention.

// Would the Genie's roll actually CHANGE anything for the target it
// landed on? A spell already standing on the stack is never re-cast,
// ValidSpellTargetArmy answers the immunity/first-target half, and the
// switch is the per-spell "would it matter" test: protections and the
// mirror only matter while the enemy hero can cast (wields the
// spellbook slot), Cure needs damage to heal, the two melee-side
// screens need a live shooter (Air Shield) or a live melee attacker
// (Shield / Fire Shield) among the enemy stacks, Precision needs the
// target itself shooting and Bloodlust the opposite.

// THIS BODY IS WHY can_shoot HAS AN OUT-OF-LINE COPY AT ALL (the note
// on 0x4428f0): the two loop sites below are the sites VC6 declines,
// while the Precision/Bloodlust tail sites expand - Bloodlust's only
// partially, leaving the OffsetToFront COMDAT call and the
// army::enemy_is_adjacent wrapper call retail shows.

// WHAT EACH SPELLING MEASURED (0.00 -> 41.65 -> 85.84 -> 87.41 ->
// 91.07): the switch arms are laid out in SOURCE order exactly as
// written; the two loop arms carry statement-scoped
// `#pragma inline_depth(0)` pins on their can_shoot calls because our
// CL otherwise expands both (retail declines them - the pins are what
// force can_shoot's out-of-line COMDAT exactly as retail's own
// rejected sites do); the loop idiom is `i = n; while (i-- > 0)`
// (the `for (i=n-1; i>=0; i--)` spelling emits dec/js and loses the
// count-copy retail has); `1 - side` must be a NAMED LOCAL `group` in
// all three loop arms or the front end folds numArmies/armies into
// two different displacement constants where retail indexes both off
// one register; Prayer needs the uchar cast `(uchar)~Is(1u << 26)` for
// retail's `not al` (bare `~Is(1u << 26)` emits `not eax`); the
// protections arm must RETURN the `&&` expression (the int
// materialization `mov eax,1`/`xor eax,eax` retail has - the
// early-return spelling emits byte `xor al,al` and a private
// epilogue); Precision is `return can_shoot(0);` EXPANDED (writing
// the same tests longhand duplicates five zero-exits retail shares,
// 91.07 -> 87.41-class); Bloodlust longhand-with-flag beats
// `return !can_shoot(0);` (87.02) because our expansion of the
// negated call diverges harder than the flag form's exits.

VA(0x00447a80, 0x429)  // anchor-callee (four call sites, one of them the
                       // tail-jump from 0x447eb0), retail-only slot
unsigned char spellIsValidOnTarget(int spell, const army* target)
{
    if (target->m_spellInfluence[spell])
        return 0;
    long side = g_combatManager->m_currentSide;
    if (!g_combatManager->validSpellTargetArmy(spell, side, target, 1, 1))
        return 0;
    switch (spell) {
    case SPELL_PROTECTION_FROM_AIR:
    case SPELL_PROTECTION_FROM_FIRE:
    case SPELL_PROTECTION_FROM_WATER:
    case SPELL_PROTECTION_FROM_EARTH:
    case SPELL_ANTI_MAGIC:
    case SPELL_MAGIC_MIRROR: {
        hero* enemyHero = g_combatManager->m_heroes[1 - side];
        return enemyHero
               && enemyHero->isWieldingArtifact(ARTIFACT_SPELLBOOK);
    }
    case SPELL_CURE:
        return target->m_topCreatureDamage > 0;
    case SPELL_PRAYER:
        return static_cast<unsigned char>(~target->is(1u << 26)) & 1;
    case SPELL_SLAYER: {
        long group = 1 - side;
        long i = g_combatManager->m_numArmies[group];
        while (i-- > 0) {
            if (g_combatManager->m_armies[group][i].is(1u << 7))
                return 1;
        }
        return 0;
    }
    case SPELL_SHIELD:
    case SPELL_FIRE_SHIELD: {
        long group = 1 - side;
        long i = g_combatManager->m_numArmies[group];
        while (i-- > 0) {
            army* enemy = &g_combatManager->m_armies[group][i];
            if (!enemy->m_spellInfluence[62] && !enemy->m_spellInfluence[70]
                && !enemy->m_spellInfluence[74] && !(enemy->is(1u << 21))
                && enemy->m_creatureType != CREATURE_FIRST_AID_TENT
                && enemy->m_creatureType != CREATURE_AMMO_CART) {
#pragma inline_depth(0)
                if (!enemy->canShoot(0))
                    return 1;
#pragma inline_depth()
            }
        }
        return 0;
    }
    case SPELL_AIR_SHIELD: {
        long group = 1 - side;
        long i = g_combatManager->m_numArmies[group];
        while (i-- > 0) {
            army* enemy = &g_combatManager->m_armies[group][i];
            if (!enemy->m_spellInfluence[62] && !enemy->m_spellInfluence[70]
                && !enemy->m_spellInfluence[74] && !(enemy->is(1u << 21))
                && enemy->m_creatureType != CREATURE_FIRST_AID_TENT
                && enemy->m_creatureType != CREATURE_AMMO_CART) {
#pragma inline_depth(0)
                if (enemy->canShoot(0))
                    return 1;
#pragma inline_depth()
            }
        }
        return 0;
    }
    case SPELL_PRECISION:
        return target->canShoot(0);
    case SPELL_BLOODLUST: {
        unsigned char shoots;
        if (target->m_creatureType == army::ARMY_CREATURE_BALLISTA
            || target->m_creatureType == army::ARMY_CREATURE_ARROW_TOWER) {
            shoots = 1;
        } else if (!(target->is(1u << 2)) || target->m_monInfo.m_numShots <= 0) {
            shoots = 0;
        } else {
            hero* controller = target->getController();
            shoots = 1;
            if (!controller
                || !controller->isWieldingArtifact(
                       ARTIFACT_BOW_OF_THE_SHARPSHOOTER)) {
                if (target->enemyIsAdjacent(0))
                    shoots = 0;
            }
            if (shoots && target->m_spellInfluence[61]
                && target->m_forgetfulnessLevel >= 2)
                shoots = 0;
        }
        return !shoots;
    }
    }
    return 1;
}

#if 0  // @carcass

// E:\gamedcs\army.cpp:5396
DC_ONLY(0x4c004, 0x80)
void army::castResurrect(long hex)
{
    // @stub
}

// E:\gamedcs\army.cpp:5419
DC_ONLY(0x4c084, 0x6C)
void army::castDemonicResurrect(long hex)
{
    // @stub
}

// E:\gamedcs\army.cpp:5433
DC_ONLY(0x4c0f0, 0x64)
unsigned char group_has_melee(long group)
{
    // @stub
}

// E:\gamedcs\army.cpp:5451
DC_ONLY(0x4c154, 0x64)
unsigned char group_has_shooters(long group)
{
    // @stub
}

// E:\gamedcs\army.cpp:5469
DC_ONLY(0x4c1b8, 0x56)
unsigned char group_has_dragons(long group)
{
    // @stub
}

// E:\gamedcs\army.cpp:5488
unsigned char isValidCaliphSpell(SpellID spell, const army* target)
{
    // @stub
}

// E:\gamedcs\army.cpp:5546
// The DC row army::get_valid_caliph_spells has NO retail slot in this
// run: 0x447eb0 is is_valid_caliph_spell (claimed below, see army.h for
// the refutation) and 0x447a80 is the retail-only shared worker.
#endif  // @carcass

VA(0x00447eb0, 0x21)  // dc 0x4c210
unsigned char isValidCaliphSpell(int spell, const army* target)
{
    if (!(g_spellTraits[spell].m_flags & 0x800))
        return 0;
    return spellIsValidOnTarget(spell, target);
}

// E:\gamedcs\army.cpp:5546
// Complete's x86 optimizer expands this source helper into both callers. The
// named boundary remains part of the recovered source even though retail has
// no separate emitted slot for it.
inline long army::getValidCaliphSpells(const army* target) const
{
    long count = 0;
    for (SpellID spell = 10; spell < 70; spell++) {
        if (isValidCaliphSpell(spell, target))
            count++;
    }
    return count;
}

VA(0x00447ee0, 0xF8)  // dc 0x4c3ac
void army::castCaliphSpell(long hex)
{
    if (hex < 0 || hex >= COMBAT_GRID_CELLS)
        return;
    army* target = g_combatManager->m_cells[hex].getArmy();
    if (!target)
        return;
    SpellID spell;
    long count = getValidCaliphSpells(target);
    if (count == 0)
        return;
    long pick = random(1, count);
    for (spell = 10; spell < 70; spell++) {
        if (isValidCaliphSpell(spell, target)) {
            if (--pick == 0) {
                // mastery 2 is ADVANCED on ai_tactical.h's
                // TSkillMastery ladder, spelled as a literal because
                // that header is not in this TU's closure.
                g_combatManager->castSpell(spell, hex, 1, -1, 2, 6);
                return;
            }
        }
    }
}

// The Enchanter's per-turn mass-cast roster: (spell, weight) pairs,
// -1-terminated, read from the hash-verified image at 0x6608b8 (.data,
// so the source array was NOT const). Every id lands on an enumerator
// the tree already proves; the weights are the cast's lottery tickets.
DATA(0x006608b8)
static int g_enchanterSpells[] = {
    SPELL_HASTE,      15,
    SPELL_AIR_SHIELD, 10,
    SPELL_SLOW,       10,
    SPELL_STONE_SKIN, 15,
    SPELL_BLOODLUST,  5,
    SPELL_BLESS,      15,
    SPELL_CURE,       10,
    SPELL_WEAKNESS,   4,
    -1,
};

// The Enchanter's turn: weigh every table spell that is still valid on
// SOME stack of its target side (negative akSpellTraits field_0 means
// a curse, so the enemy side is scanned), lottery one out by weight,
// pose every able Enchanter on the controlling side for the cast, and
// CastSpell it as a monster cast at expert mastery.

VA(0x00447fe0, 0x27E)
unsigned char army::unnamed447fe0()
{
    if (!g_combatManager->canCastSpells(g_combatManager->m_currentSide,
                                          0))
        return 0;

    long total = 0;
    const int* p = g_enchanterSpells;
    long spell;
    while (*p >= 0) {
        spell = *p++;
        long side = g_combatManager->m_currentSide;
        if (g_spellTraits[spell].m_karma < 0)
            side = 1 - side;
        army* targets = g_combatManager->m_armies[side];
        long n = g_combatManager->m_numArmies[side];
        while (n-- != 0) {
            if (spellIsValidOnTarget(spell, targets)) {
                total += *p;
                break;
            }
        }
        p++;
    }
    if (total == 0)
        return 0;

    long roll = rand() % total;
    for (p = g_enchanterSpells; *p >= 0; p += 2) {
        spell = *p;
        long side = g_combatManager->m_currentSide;
        if (g_spellTraits[spell].m_karma < 0)
            side = 1 - side;
        army* targets = g_combatManager->m_armies[side];
        long n = g_combatManager->m_numArmies[side];
        while (n-- != 0) {
            if (spellIsValidOnTarget(spell, targets)) {
                roll -= p[1];
                break;
            }
        }
        if (roll < 0)
            break;
    }

    if (!static_cast<const combatManager*>(g_combatManager)
             ->isQuickCombat()) {
        long side = getControllingSide();
        army* stack = g_combatManager->m_armies[side];
        for (long i = 0; i < g_combatManager->m_numArmies[side]; i++, stack++) {
            if (stack->m_creatureType == ARMY_CREATURE_ENCHANTER
                && stack->m_spellInfluence[SPELL_BLIND] == 0
                && stack->m_spellInfluence[SPELL_STONE] == 0
                && stack->m_spellInfluence[SPELL_PARALYZE] == 0
                && !(stack->is(1u << 21))) {
                stack->m_showAttackFrames = 1;
                stack->m_showAttackFrameType = cs_range_r;
            }
        }
        playSample(SHOOT_SAMPLE);
        g_combatManager->powEffect(-1, 1);
    }
    g_combatManager->castSpell(spell, -1, 1, -1, 3, 3);
    return 1;
}

VA(0x00448260, 0x582)  // dc 0x4c468
void army::castSpell(long hex)
{
    long originalFacing = m_facing;
    if (!static_cast<const combatManager*>(g_combatManager)
             ->isQuickCombat()) {
        long targetX = g_combatManager->m_cells[hex].m_refX;
        long targetY = g_combatManager->m_cells[hex].m_refY;
        long myX = midX();
        long myY = midY();
        unsigned char shouldTurn = 0;
        setupAnimation();
        if (targetX > myX && m_facing == 0)
            shouldTurn = 1;
        if ((targetX < myX && m_facing == 1) || shouldTurn)
            turn(1);
        g_combatManager->resetLimitCreature();
        g_combatManager->markCreatureEffect(m_combatSide, m_bitIndex);
        g_combatManager->computeMaxExtent();
        long dx = targetX - myX;
        long dy = targetY - myY;
        if (dx < 0)
            dx = -dx;
        float angle;
        if (dx == 0) {
            angle = (dy > 0) ? -90 : 90;
        } else {
            angle = static_cast<float>(
                atan(static_cast<double>(dy) / dx)
                * DATA_COMPGEN(0x0063b8f0, missileRadiansToDegrees,
                               57.2957763671875));
        }
        if (angle > 25.0f)
            m_currFrameType = cs_special_ur;
        else if (angle > -25.0f)
            m_currFrameType = cs_special_r;
        else
            m_currFrameType = cs_special_dr;
        long frames = m_stdIcon->getNumFrames(m_currFrameType);
        if (frames == 0) {
            if (angle > 25.0f)
                m_currFrameType = cs_attack_ur;
            else if (angle > -25.0f)
                m_currFrameType = cs_attack_r;
            else
                m_currFrameType = cs_attack_dr;
            frames = m_stdIcon->getNumFrames(m_currFrameType);
        }
        playSample(SHOOT_SAMPLE);
        long delay = m_monFrameInfo.m_attackStartCycleTime / frames;
        for (m_currFrameIndex = 0; m_currFrameIndex < frames;
             m_currFrameIndex++) {
            g_combatManager->drawFrame(1, 1, 0, delay, 1, 1);
        }
        m_currFrameIndex = frames - 1;
    }
    m_monInfo.m_hasSpell--;
    switch (m_creatureType) {
    case CREATURE_ARCHANGEL: {
        army* target = g_combatManager->findResurrectionTarget(
            getControllingSide(), hex, 1);
        if (target) {
            SAMPLE2 sample;
            if (!static_cast<const combatManager*>(g_combatManager)
                     ->isQuickCombat())
                sample = loadPlaySample(DATA_COMPGEN(
                    0x00660af4, resurrectSampleName, "Resurect.wav"));
            g_combatManager->resurrect(target, m_numTroops * 100, 0);
            if (!static_cast<const combatManager*>(g_combatManager)
                     ->isQuickCombat())
                waitEndSample(sample, -1);
        }
        break;
    }
    case ARMY_CREATURE_PIT_LORD: {
        army* target = g_combatManager->findDemonicResurrectionTarget(
            getControllingSide(), hex);
        if (target)
            g_combatManager->demonicResurrection(this, target);
        break;
    }
    case CREATURE_MASTER_GENIE:
        castCaliphSpell(hex);
        break;
    case CREATURE_FAERIE_DRAGON:
        if (hex >= 0 && hex < COMBAT_GRID_CELLS)
            g_combatManager->castSpell(m_faerieDragonSpell, hex, 1, -1, 2,
                                       m_numTroops * 5);
        break;
    case CREATURE_STORM_ELEMENTAL:
        g_combatManager->castSpell(SPELL_PROTECTION_FROM_AIR, hex, 1, -1,
                                   2, 6);
        break;
    case CREATURE_ICE_ELEMENTAL:
        g_combatManager->castSpell(SPELL_PROTECTION_FROM_WATER, hex, 1, -1,
                                   2, 6);
        break;
    case CREATURE_ENERGY_ELEMENTAL:
        g_combatManager->castSpell(SPELL_PROTECTION_FROM_FIRE, hex, 1, -1,
                                   2, 6);
        break;
    case CREATURE_MAGMA_ELEMENTAL:
        g_combatManager->castSpell(SPELL_PROTECTION_FROM_EARTH, hex, 1, -1,
                                   2, 6);
        break;
    case CREATURE_OGRE_MAGE:
        g_combatManager->castSpell(SPELL_BLOODLUST, hex, 1, -1, 2, 6);
        break;
    }
    if (!static_cast<const combatManager*>(g_combatManager)
             ->isQuickCombat()
        && m_armySample[SHOOT_SAMPLE])
        g_soundManager->waitSample(m_armySample[SHOOT_SAMPLE]->m_memSample.m_memSampleHandle, -1);
    g_combatManager->m_drawbridgeBounds = g_combatAreaLimits;
    if (originalFacing != m_facing
        && !static_cast<const combatManager*>(g_combatManager)
                ->isQuickCombat())
        turn(1);
}

VA(0x004487f0, 0x43)
int army::getMirrorEffect() const
{
    int effect = 0;
    if (m_spellInfluence[36] > 0)
        effect = m_backlashChance;
    if (m_creatureType == CREATURE_FAERIE_DRAGON) {
        const SSpellTraits* mirrorTraits =
            &g_spellTraits[SPELL_MAGIC_MIRROR];
        int current = effect;
        int innate = mirrorTraits->m_masteryBonus[0];
        const int& selected = current < innate ? innate : current;
        return selected;
    }
    return effect;
}

VA(0x00448840, 0x26F)
void army::considerAttack(const army* enemy, long value, long attackDistance)
{
    long turns;
    if (!getSpeed()) {
        turns = 1;
    } else {
        turns = (attackDistance + getSpeed() - 1) / getSpeed();
        if (turns < 1)
            turns = 1;
    }
    if (turns == 1)
        m_aiPossibleTargets |= 1 << enemy->m_bitIndex;
    if (m_aiTarget) {
        long current = getAITargetTime(getSpeed());
        if (turns > current)
            return;
        if (turns == current && value <= m_aiTargetValue)
            return;
    }
    m_aiTarget = const_cast<army*>(enemy);
    m_aiTargetValue = value;
    m_aiTargetTime = attackDistance;
}

VA(0x00448ab0, 0x114)  // dc 0x4c82c
long army::getMultiHeadDirections(long ourHex, const army* enemy,
                                     long enemyHex) const
{
    long mask = 0xff;
    if (!(m_monInfo.m_attributes & 1))
        mask = 0x3f;
    if (m_creatureType == ARMY_CREATURE_CERBERUS) {
        long direction = getAttackDirection(ourHex, enemy, enemyHex);

        mask = 1 << direction;
        mask |= 1 << getCounterClockwise(direction);
        mask |= 1 << getClockwise(direction);
    }
    return mask;
}

VA(0x00448bd0, 0xF6)  // dc 0x4c898
long army::getAITargetTime(long speed) const
{
    if (canShoot(0))
        return 1;
    if (!speed)
        return 1;
    long turns = (m_aiTargetTime + speed - 1) / speed;
    if (turns < 1)
        return 1;
    return turns;
}

VA_COMPGEN(0x00448d30, 0x36, VECTOR_ERASE, army)

VA(0x00448cd0, 0x4B)  // dc 0x4c918
int army::getSpeed() const
{
    int speed = m_monInfo.m_speed;
    if (m_spellInfluence[54]) {
        if (is(1u << 6))
            return 0;
        speed = static_cast<long>(speed * m_slowFactor);
        if (speed <= 0)
            speed = 1;
    }
    return speed;
}

#if 0  // @carcass

// E:\gamedcs\struct.h:315
DC_ONLY(0x4c968, 0x2C)
void SLimitData::include(const SLimitData* include_limits)
{
    // @stub
}

// E:\gamedcs\struct.h:438
DC_ONLY(0x4c994, 0x2C)
unsigned long GameTime::nextFrameTime(unsigned long this_frame, long interval)
{
    // @stub
}

// E:\gamedcs\includes.h:117
DC_ONLY(0x4c9c0, 0x2C)
double min(double a, double b)
{
    // @stub
}

// E:\gamedcs\Army.h:760
DC_ONLY(0x4c9ec, 0x20)
bool army::needToTurn(int direction) const
{
    // @stub
}

// E:\gamedcs\Army.h:810
DC_ONLY(0x4ca0c, 0x20)
const char* army::getName() const
{
    // @stub
}

// E:\gamedcs\Army.h:815
DC_ONLY(0x4ca2c, 0x18)
const char* army::getName(int count) const
{
    // @stub
}

// E:\gamedcs\Army.h:869
DC_ONLY(0x4ca44, 0x1C)
long army::getAttackDirection(const army* enemy) const
{
    // @stub
}

// E:\gamedcs\Army.h:875
DC_ONLY(0x4ca60, 0x1C)
bool army::leavesNoBody() const
{
    // @stub
}

// E:\gamedcs\Bitmap16.h:156
DC_ONLY(0x4ca7c, 0x10)
const unsigned short* Bitmap16Bit::getMap(int x, int y)
{
    // @stub
}

// E:\gamedcs\Bitmap16.h:162
DC_ONLY(0x4ca8c, 0x90)
void Bitmap16Bit::draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char alpha)
{
    // @stub
}

// E:\gamedcs\Bitmap16.h:168
DC_ONLY(0x4cb1c, 0x74)
void Bitmap16Bit::grab(const Bitmap16Bit* src, int sx, int sy)
{
    // @stub
}

// E:\gamedcs\CSpriteFrame.h:87
DC_ONLY(0x4cb90, 0x4)
int CSpriteFrame::getCroppedWidth()
{
    // @stub
}

// E:\gamedcs\CSpriteFrame.h:89
DC_ONLY(0x4cb94, 0x4)
int CSpriteFrame::getCroppedX()
{
    // @stub
}

// E:\gamedcs\CSpriteFrame.h:90
DC_ONLY(0x4cb98, 0x4)
int CSpriteFrame::getCroppedY()
{
    // @stub
}

// E:\gamedcs\CSprite.h:148
DC_ONLY(0x4cb9c, 0x44)
int CSprite::getCroppedX(int seq, int frame)
{
    // @stub
}

// E:\gamedcs\CSprite.h:149
DC_ONLY(0x4cbe0, 0x44)
int CSprite::getCroppedY(int seq, int frame)
{
    // @stub
}

// E:\gamedcs\CSprite.h:150
DC_ONLY(0x4cc24, 0x44)
int CSprite::getCroppedWidth(int seq, int frame)
{
    // @stub
}

// E:\gamedcs\HexCell.h:90
DC_ONLY(0x4cc68, 0xA)
unsigned char hexcell::hasArmy()
{
    // @stub
}

// E:\gamedcs\CmbtMgr.h:1555
DC_ONLY(0x4cc74, 0x18)
void combatManager::markCreatureEffect(int group, int index)
{
    // @stub
}

// E:\gamedcs\TownMgr.h:745
DC_ONLY(0x4cc8c, 0x10)
TTerrainType townManager::getNativeTerrain(int type)
{
    // @stub
}

// ..\stlport\stl_deque.h:568
DC_ONLY(0x4cc9c, 0x20)
std::_Deque_iterator<enum std::deque<enum SpellID,std::allocator<enum SpellID>,0>::begin(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_deque.h:569
DC_ONLY(0x4ccbc, 0x20)
std::_Deque_iterator<enum std::deque<enum SpellID,std::allocator<enum SpellID>,0>::end(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_deque.h:615
DC_ONLY(0x4ccdc, 0x1C)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::deque<enum SpellID,std::allocator<enum SpellID>,0>(const std::allocator<enum* __a)
{
    // @stub
}

// ..\stlport\stl_deque.h:765
DC_ONLY(0x4ccf8, 0x3C)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::push_back(const SpellID* __t)
{
    // @stub
}

// ..\stlport\stl_deque.h:896
DC_ONLY(0x4cd34, 0x104)
std::_Deque_iterator<enum std::deque<enum SpellID,std::allocator<enum SpellID>,0>::erase(__$ReturnUdt, std::_Deque_iterator<enum __pos)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x4ce38, 0x4)
void std::allocator<enum SpellID>::allocator<enum SpellID>()
{
    // @stub
}

// ..\stlport\stl_vector.h:480
DC_ONLY(0x4ce3c, 0x4C)
army** std::vector<army *,std::allocator<army *> >::erase(army** __position)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x4ce88, 0x38)
void std::vector<army *,std::allocator<army *> >::clear()
{
    // @stub
}

// ..\stlport\stl_deque.h:806
DC_ONLY(0x4cec0, 0x34)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::pop_back()
{
    // @stub
}

// ..\stlport\stl_deque.h:816
DC_ONLY(0x4cef4, 0x38)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::pop_front()
{
    // @stub
}

// ..\stlport\stl_deque.h:301
DC_ONLY(0x4cf2c, 0x3C)
std::_Deque_iterator<enum std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator+(__$ReturnUdt, int __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x4cf68, 0x3C)
army** std::vector<army *,std::allocator<army *> >::erase(army** __first, army** __last)
{
    // @stub
}

// ..\stlport\stl_deque.h:299
DC_ONLY(0x4cfa4, 0x1C)
std::_Deque_iterator<enum* std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator+=(int __n)
{
    // @stub
}

// ..\stlport\stl_deque.h:218
DC_ONLY(0x4cfc0, 0x84)
void std::_Deque_iterator_base<enum SpellID,std::_Buf_size_traits<enum SpellID,0> >::_M_advance(int __n)
{
    // @stub
}

// E:\gamedcs\DC_precompiledheaders.h:41
DC_ONLY(0x4d044, 0x38)
const double& cppMin(const double& _X, const double& _Y)
{
    // @stub
}

// ..\stlport\stl_deque.c:318
DC_ONLY(0x4d07c, 0x84)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::clear()
{
    // @stub
}

// ..\stlport\stl_deque.c:364
DC_ONLY(0x4d100, 0x54)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::_M_push_back_aux(const SpellID* __t)
{
    // @stub
}

// ..\stlport\stl_deque.c:430
DC_ONLY(0x4d154, 0x40)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::_M_pop_back_aux()
{
    // @stub
}

// ..\stlport\stl_deque.c:444
DC_ONLY(0x4d194, 0x40)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::_M_pop_front_aux()
{
    // @stub
}

// ..\stlport\stl_algo.h:102
DC_ONLY(0x4d1d4, 0x1C)
army** std::find(army** __first, army** __last, army** __val)
{
    // @stub
}

// ..\stlport\stl_algo.h:102
DC_ONLY(0x4d1f0, 0x6C)
std::_Deque_iterator<enum std::find(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, const SpellID* __val)
{
    // @stub
}

// ..\stlport\stl_algobase.h:137
DC_ONLY(0x4d25c, 0xE)
const float* std::max(const float* __a, const float* __b)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0x4d26c, 0x98)
std::_Deque_iterator<enum std::copy_backward(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, std::_Deque_iterator<enum __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x4d304, 0x98)
std::_Deque_iterator<enum std::copy(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, std::_Deque_iterator<enum __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x4d39c, 0x50)
army** std::copy(army** __first, army** __last, army** __result)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x4d3ec, 0x30)
void std::destroy(SpellID* __first, SpellID* __last)
{
    // @stub
}

// ..\stlport\stl_deque.h:1145
DC_ONLY(0x4d41c, 0x30)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::_M_reserve_map_at_back(unsigned __nodes_to_add)
{
    // @stub
}

// ..\stlport\stl_deque.c:731
DC_ONLY(0x4d44c, 0x110)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::_M_reallocate_map(unsigned __nodes_to_add, unsigned char __add_at_front)
{
    // @stub
}

// ..\stlport\stl_algo.h:66
DC_ONLY(0x4d55c, 0x16)
army** std::find(army** __first, army** __last, army** __val, std::input_iterator_tag __formal)
{
    // @stub
}

// ..\stlport\stl_algo.h:66
DC_ONLY(0x4d574, 0x60)
std::_Deque_iterator<enum std::find(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, const SpellID* __val, std::input_iterator_tag __formal)
{
    // @stub
}

// ..\stlport\stl_deque.h:419
DC_ONLY(0x4d5d4, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const std::_Deque_iterator<enum* __formal)
{
    // @stub
}

// ..\stlport\stl_deque.h:425
DC_ONLY(0x4d5e0, 0x4)
int* std::distance_type(const std::_Deque_iterator<enum* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0x4d5e4, 0x78)
std::_Deque_iterator<enum std::__copy_backward(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, std::_Deque_iterator<enum __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x4d65c, 0x78)
std::_Deque_iterator<enum std::__copy(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, std::_Deque_iterator<enum __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x4d6d4, 0x1E)
army** std::__copy(army** __first, army** __last, army** __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x4d6f4, 0x4)
SpellID* std::value_type(const SpellID* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x4d6f8, 0x1C)
void std::__destroy(SpellID* __first, SpellID* __last, SpellID* __formal)
{
    // @stub
}

// ..\stlport\stl_deque.h:292
DC_ONLY(0x4d714, 0x1C)
std::_Deque_iterator<enum* std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator--()
{
    // @stub
}

// ..\stlport\stl_deque.h:208
DC_ONLY(0x4d730, 0x30)
void std::_Deque_iterator_base<enum SpellID,std::_Buf_size_traits<enum SpellID,0> >::_M_decrement()
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x4d760, 0x50)
SpellID** std::copy(SpellID** __first, SpellID** __last, SpellID** __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0x4d7b0, 0x50)
SpellID** std::copy_backward(SpellID** __first, SpellID** __last, SpellID** __result)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x4d800, 0x30)
void std::__destroy_aux(SpellID* __first, SpellID* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x4d830, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, SpellID** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x4d83c, 0x4)
int* std::distance_type(SpellID** __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x4d840, 0x1E)
SpellID** std::__copy(SpellID** __first, SpellID** __last, SpellID** __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0x4d860, 0x22)
SpellID** std::__copy_backward(SpellID** __first, SpellID** __last, SpellID** __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x004490b0, 0x73, DEQUE_FREEFRONT, int)
VA_COMPGEN(0x00449130, 0x8E, DEQUE_FREEBACK, int)

// COMDAT pairing: deque<int>::erase(first, last), agreement 0.996.
VA_COMPGEN(0x00448db0, 0x2FE, DEQUE_ERASE, int)

// COMDAT pairing: vector<army*>::clear, agreement 0.954.
VA_COMPGEN(0x00448d70, 0x3D, VECTOR_CLEAR, army)

// COMDAT pairing: deque<int>::iterator::operator++ and ::operator--, both
// 51 bytes and byte-identical apart from the step they add - 0x449230 adds
// +4 and 0x449270 adds -4 (materialized as 0xfffffffc), which is the whole
// difference between the two bodies on our side as well.
VA_COMPGEN(0x00449230, 0x33, DEQUE_ITERATOR_INC, int)
VA_COMPGEN(0x00449270, 0x33, DEQUE_ITERATOR_DEC, int)
