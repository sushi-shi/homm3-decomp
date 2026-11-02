#include "va.h"
#include "includes.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "army.h"

#include "ai.h"
#include "armygrp.h"
#include "bitmap16.h"
#include "cmbtmgr.h"
#include "combatwindow.h"
#include "creaturetype.h"
#include "csprite.h"
#include "cspriteframe.h"
#include "drawing.h"
#include "findpath.h"
#include "font.h"
#include "game.h"
#include "hero.h"
#include "herospec.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "monframeinfo.h"
#include "palette.h"
#include "path.h"
#include "prefs.h"
#include "resourcemanager.h"
#include "sample.h"
#include "soundmgr.h"
#include "terrain.h"
#include "textresource.h"
#include "town.h"
#include "townmgr.h"
#include "winmgr.h"

// Retail table initializers, in the layouts used by their named consumers.
DATA(0x00660878) long g_wideDirectionRingIndex[COMBAT_DIRECTION_WIDE_LOWER + 1] = { 0, 1, 2, 4, 5, 6, 7, 3 };
DATA(0x00660898) long g_wideDirectionRingOrder[COMBAT_DIRECTION_WIDE_LOWER + 1] = { 0, 1, 2, 7, 3, 4, 5, 6 };

// Retail scalar state; startup initial values come from the pinned image.
DATA(0x00660868) int g_walkingFrom = -1;
DATA(0x0066086c) int g_walkingFrom2 = -1;
DATA(0x00660870) int g_walkingTo = -1;
DATA(0x00660874) int g_walkingTo2 = -1;
DATA(0x00693858) int g_walkingYMod;

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// E:\gamedcs\army.cpp:52
VA(0x0043d250, 0x1A8)
DC_ADDRESS(0x0436b8, 0xf4)
MAC_ADDRESS(0x048b38, 0x1ac)  // anchor-global + member-construction run
army::army()
{
    m_stdIcon = 0;
    m_missileIcon = 0;
    m_imageHeight = 0;
    m_gridIndex = 0;
    for (int i = 0; i < MAX_SAMPLES; i++) {
        m_armySample[i] = 0;
    }
    groupToAttack = -1;
    indexToAttack = -1;
    attackLimit = 0;
    targetCellIndex = 0;
    m_yModify = 0;
    m_showTroopCount = 1;
    m_ySpecialMod = 0;
    m_xSpecialMod = 0;
    m_retaliationCount = 1;
    m_isMoving = 0;
    m_letsPretendImNotHere = 0;
}

VA(0x0043d400, 0x136)
MAC_ADDRESS(0x048cf0, 0x140)
army::~army()
{
}

// Original: army::set_retaliation_count; army.cpp:77
// Inlined in Init and ResetRound. DC line 80 increments the Griffin
// allowance.
DC_ADDRESS(0x0437ac, 0x84)
MAC_ADDRESS(0x048e5c, 0x68)
// Mac 0x48e90 reads COUNTERSTRIKE at +0x280; DC set_retaliation_count makes
// no get_spell_time call (see army.h getSpellTime), so the row is read directly.
void army::setRetaliationCount()
{
    m_retaliationCount = 1;
    if (m_creatureType == ARMY_CREATURE_GRIFFIN)
        m_retaliationCount++;
    if (m_creatureType == ARMY_CREATURE_ROYAL_GRIFFIN)
        m_retaliationCount = 5000;
    if (m_spellInfluence[SPELL_COUNTERSTRIKE])
        m_retaliationCount += m_counterstrokeBonus;
    if (is(creatureSiegeWeapon))
        m_retaliationCount = 0;
}

VA(0x0043d540, 0x34)
DC_ADDRESS(0x043830, 0x3a)
MAC_ADDRESS(0x048ec4, 0x68)
void army::playSample(army::TSampleID id)
{
    if (!g_combatManager->isQuickCombat()
        && m_armySample[id]) {
        g_soundManager->memorySample(m_armySample[id]);
    }
}

VA(0x0043d580, 0x37)
DC_ADDRESS(0x04386c, 0x3c)
MAC_ADDRESS(0x048f2c, 0x6c)
void army::stopSample(army::TSampleID id)
{
    if (!g_combatManager->isQuickCombat()
        && m_armySample[id]) {
        g_soundManager->stopSample(m_armySample[id]->memSample.memHSample);
    }
}

// Put the stack back to its "loaded but not in a battle" state: drop
// every sound and the sprite, wipe the standing-spell row and the
// spell queue, and reset the seven scalars that mean "nothing here".

VA(0x0043d5c0, 0x166)
DC_ADDRESS(0x0438e8, 0xc8)
MAC_ADDRESS(0x049008, 0x100)  // anchor-bracket + arity
void army::initClean()
{
    for (int i = 0; i < MAX_SAMPLES; i++) {
        m_armySample[i] = 0;
    }
    m_roundsLeftBeforeVanish = -1;
    m_numSpellInfluences = 0;
    memset(m_spellInfluence, 0, sizeof(m_spellInfluence));
    m_spellInfluenceQueue.clear();
    m_lastFidgetTime = GameTime::get();
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
// Inlined in range_attack; Mac retains the body.
DC_ADDRESS(0x0438a8, 0x3e)
MAC_ADDRESS(0x048f98, 0x70)
inline void army::waitSample(army::TSampleID which)
{
    if (!g_combatManager->isQuickCombat()
        && m_armySample[which]) {
        g_soundManager->waitSample(m_armySample[which]->memSample.memHSample, -1);
    }
}

VA(0x0043d730, 0x17D)
DC_ADDRESS(0x0439b0, 0x3ea)
MAC_ADDRESS(0x049108, 0x1d8)
void army::initialize(TCreatureType type, long number, const hero* owner,
                      long newGroup, long newIndex, long newGridIndex)
{
    initClean();
    m_creatureType = type;
    m_numTroops = number;
    m_drawPriority = 4;
    TCreatureTypeTraits* traits = &m_monInfo;
    *traits = akCreatureTypeTraits[type];
    traits->townType = g_game->getAlignment(type);
    if (owner != 0)
        owner->applyCreatureStatBonuses(type, traits);
    if (g_combatManager->m_magicTerrain
            != COMBAT_SPELL_RESTRICTION_NO_CREATURE_SPELLS
        && townManager::getNativeTerrain(m_monInfo.townType)
               == g_combatManager->m_terrainType)
        m_onNativeTerrain = 1;
    else
        m_onNativeTerrain = 0;
    if (m_onNativeTerrain) {
        if (!is(creatureSiegeWeapon))
            m_monInfo.speed++;
        m_monInfo.attackSkill++;
        m_monInfo.defenseSkill++;
    }
    m_facing = 1 - newGroup;
    m_currFrameType = cs_wait;
    groupToAttack = -1;
    group = newGroup;
    indexToAttack = -1;
    index = newIndex;
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
    m_baseSpeed = m_monInfo.speed;
    m_origHitPoints = m_monInfo.hitPoints;
    m_poisonPenalty = 1.0f;
}

VA(0x0043d8b0, 0x135)
DC_ADDRESS(0x043d9c, 0xe4)
// DC records hexcell& back_cell. Both side stores read group
// directly: DC Init has no get_owning_side call.
// Complete/Mac addAura precedes retaliation setup, unlike the older DC order.
MAC_ADDRESS(0x0492e0, 0x13c)
void army::init(int armyId, int newNumTroops, const hero* owner, int side,
                int inIndex, int gridIndex, int origPos)
{
    initialize(TCreatureType(armyId), newNumTroops, owner, side, inIndex,
               gridIndex);
    if (g_combatManager->validHex(m_gridIndex)) {
        hexcell* cell = &g_combatManager->m_cells[m_gridIndex];
        cell->m_armySide = static_cast<signed char>(group);
        cell->m_armySlot = static_cast<signed char>(index);
        cell->m_partOfDouble = -1;
        if (is(creatureDoubleWide)) {
            hexcell& backCell =
                g_combatManager->m_cells[m_gridIndex + offsetToFront(-1)];
            backCell.m_armySide = static_cast<signed char>(group);
            backCell.m_armySlot = static_cast<signed char>(index);
            backCell.m_partOfDouble = m_facing != 0;
            cell->m_partOfDouble = m_facing == 0;
        }
        addAura();
    }
    m_originalIndex = origPos;
    setRetaliationCount();
}

VA(0x0043d9f0, 0x525)
DC_ADDRESS(0x043e80, 0x3ca)
MAC_ADDRESS(0x04941c, 0x774)
void army::loadResources()
{
    if (g_combatManager->isQuickCombat())
        return;

    memcpy(m_monFrameInfo.m_missileOffset, &g_monFrameInfo[m_creatureType],
           sizeof(SMonFrameInfo));
    m_origWalkCycleTime = m_monFrameInfo.m_walkCycleTime;

    sample* s;
    if (!is(creatureSiegeWeapon)) {
        sprintf(g_text, DATA_COMPGEN(0x00660a10, moveSampleFormat,
                                    "%smove.82M"),
                m_monInfo.cSamplePrefix);
        s = ResourceManager::GetSample(g_text);
        m_armySample[WALK_SAMPLE] = s;
    } else {
        m_armySample[WALK_SAMPLE] = 0;
    }

    if (m_creatureType == CREATURE_BALLISTA)
        sprintf(g_text, DATA_COMPGEN(0x00660a04, shotSampleFormat,
                                    "%sshot.82M"),
                m_monInfo.cSamplePrefix);
    else if (is(creatureSiegeWeapon))
        sprintf(g_text, DATA_COMPGEN(0x006609f8, winceSampleFormat,
                                    "%swnce.82M"),
                m_monInfo.cSamplePrefix);
    else
        sprintf(g_text, DATA_COMPGEN(0x006609ec, attackSampleFormat,
                                    "%sattk.82M"),
                m_monInfo.cSamplePrefix);
    s = ResourceManager::GetSample(g_text);
    m_armySample[ATTACK_SAMPLE] = s;

    sprintf(g_text, DATA_COMPGEN(0x006609f8, winceSampleFormat,
                                "%swnce.82M"),
            m_monInfo.cSamplePrefix);
    s = ResourceManager::GetSample(g_text);
    m_armySample[WINCE_SAMPLE] = s;

    sprintf(g_text, DATA_COMPGEN(0x006609e0, killSampleFormat,
                                "%skill.82M"),
            m_monInfo.cSamplePrefix);
    s = ResourceManager::GetSample(g_text);
    m_armySample[DIE_SAMPLE] = s;

    if (is(creatureSiegeWeapon))
        sprintf(g_text, DATA_COMPGEN(0x006609f8, winceSampleFormat,
                                    "%swnce.82M"),
                m_monInfo.cSamplePrefix);
    else
        sprintf(g_text, DATA_COMPGEN(0x006609d4, defendSampleFormat,
                                    "%sdfnd.82M"),
                m_monInfo.cSamplePrefix);
    s = ResourceManager::GetSample(g_text);
    m_armySample[DEFEND_SAMPLE] = s;

    if (is(creatureShootingArmy) || m_creatureType == CREATURE_MASTER_GENIE
        || m_creatureType == CREATURE_OGRE_MAGE) {
        sprintf(g_text, DATA_COMPGEN(0x00660a04, shotSampleFormat,
                                    "%sshot.82M"),
                m_monInfo.cSamplePrefix);
        s = ResourceManager::GetSample(g_text);
        m_armySample[SHOOT_SAMPLE] = s;
    } else {
        m_armySample[SHOOT_SAMPLE] = 0;
    }

    if (m_creatureType == CREATURE_VAMPIRE
        || m_creatureType == CREATURE_VAMPIRE_LORD
        || m_creatureType == CREATURE_DEVIL
        || m_creatureType == CREATURE_ARCH_DEVIL) {
        sprintf(g_text, DATA_COMPGEN(0x006609c8, ext1SampleFormat,
                                    "%sext1.82M"),
                m_monInfo.cSamplePrefix);
        s = ResourceManager::GetSample(g_text);
        m_armySample[PRE_WALK_SAMPLE] = s;
        sprintf(g_text, DATA_COMPGEN(0x006609bc, ext2SampleFormat,
                                    "%sext2.82M"),
                m_monInfo.cSamplePrefix);
        s = ResourceManager::GetSample(g_text);
        m_armySample[POST_WALK_SAMPLE] = s;
    } else {
        m_armySample[PRE_WALK_SAMPLE] = 0;
        m_armySample[POST_WALK_SAMPLE] = 0;
    }

    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (m_armySample[i]) {
            m_armySample[i]->memSample.memVolume = 64;
            m_armySample[i]->memSample.memCindex = 3;
            m_armySample[i]->memSample.memLooping = i != 0;
        }
    }

    CSprite* icon =
        ResourceManager::GetSprite(akCreatureTypeTraits[m_creatureType]
                                       .m_sprite_name);
    m_stdIcon = icon;
    m_imageHeight = 267 - m_stdIcon->GetFrame(cs_wait, 0)->GetCroppedY();

    if (is(creatureShootingArmy)) {
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
        CSprite* missile = ResourceManager::GetSprite(missileName);
        m_missileIcon = missile;
    } else {
        m_missileIcon = 0;
    }
}

VA(0x0043df20, 0xDD)
DC_ADDRESS(0x044318, 0x9c)
MAC_ADDRESS(0x049b90, 0x150)
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
            if (classifyTownAlignment(m_monInfo.townType)
                == moralAlignmentNeutral)
                value += 2;
        }

        if (m_creatureType == CREATURE_HALFLING && value < 1)
            value = 1;
    }
    m_luck = value;
}

// Complete extends the DC signature with a seventh, byte-wide alignment gate:
VA(0x0043e000, 0x139)
DC_ADDRESS(0x0443b4, 0xf4)
MAC_ADDRESS(0x049ce0, 0x1f4)
void army::setMorale(const hero* ownerHero, const armyGroup* ownerGroup,
                     const town* ownerTown, const hero* otherHero,
                     const armyGroup* otherGroup, int magicTerrain,
                     unsigned char groupAlignments)
{
    int value = 0;
    if (magicTerrain != MAGIC_TERRAIN_CURSED_GROUND && !is(creatureNoMorale)) {
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
            int alignment = classifyTownAlignment(m_monInfo.townType);
            if (alignment == moralAlignmentGood)
                ++value;
            else if (alignment == moralAlignmentEvil)
                --value;
        } else if (magicTerrain == MAGIC_TERRAIN_EVIL_FOG) {
            int alignment = classifyTownAlignment(m_monInfo.townType);
            if (alignment == moralAlignmentGood)
                --value;
            else if (alignment == moralAlignmentEvil)
                ++value;
        }

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
VA(0x0043e140, 0x8C0)
DC_ADDRESS(0x0444a8, 0x8a6)
MAC_ADDRESS(0x049ed4, 0x810)  // anchor-global
void army::drawToBuffer(int x, int y, int numBoxOnly)
{
    // Placement modes outside 0..3 leave the coordinates unassigned, as in
    // DC 0x44d0c and Mac 0x4a570.
    int powX;
    int powY;
    if (g_combatManager->m_battleOver != 0)
        return;
    if (g_combatManager->isQuickCombat())
        return;

    y += m_ySpecialMod;
    x += m_xSpecialMod;
    if (m_currFrameType == cs_walk && !is(creatureFlyingArmy)) {
        int frames = m_stdIcon->GetNumFrames(0);
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
        int highlight = 0;
        if (g_combatManager->m_highlighterOn != 0
            && m_gridIndex == g_combatManager->m_highlighterIndex)
            highlight = 0x70;
        if (g_combatManager->m_lastMovedArmy == this)
            highlight = 0x60;
        if (m_isAreaEffectTarget)
            highlight = 0x70;

        TPalette16 saved;
        unsigned char restore = 0;
        if (is(creatureRedColoring)) {
            memcpy(saved.Palette, m_stdIcon->GetPalette(), 0x200);
            TPalette16 tinted(m_stdIcon->GetPalette());
            tinted.AdjustHSV(0, m_paletteEffect, m_paletteEffect + 1.0f,
                             m_paletteEffect + 1.0f);
            memcpy(m_stdIcon->GetPalette(), tinted.Palette, 0x200);
            restore = 1;
        } else if (is(creatureGreyColoring)) {
            memcpy(saved.Palette, m_stdIcon->GetPalette(), 0x200);
            TPalette16 tinted(m_stdIcon->GetPalette());
            tinted.AdjustSaturation(m_paletteEffect);
            memcpy(m_stdIcon->GetPalette(), tinted.Palette, 0x200);
            restore = 1;
        } else if (m_spellInfluence[SPELL_STONE] > 0) {
            memcpy(saved.Palette, m_stdIcon->GetPalette(), 0x200);
            TPalette16 tinted(m_stdIcon->GetPalette());
            tinted.Gray();
            memcpy(m_stdIcon->GetPalette(), tinted.Palette, 0x200);
            restore = 1;
        } else if (is(creatureClone)) {
            memcpy(saved.Palette, m_stdIcon->GetPalette(), 0x200);
            TPalette16 tinted(m_stdIcon->GetPalette());
            tinted.AdjustHSV(0.67f, 1.0f, 2.0f, 2.0f);
            memcpy(m_stdIcon->GetPalette(), tinted.Palette, 0x200);
            restore = 1;
        }

        int drawX =
            m_facing == 0 ? x - m_stdIcon->GetWidth() + 196 : x - 196;
        long drawY = y - 267;
        g_combatManager->drawCreature(
            m_stdIcon, m_currFrameType, m_currFrameIndex, drawX, drawY, 0,
            m_gridIndex, m_facing == 0, g_systemPalette->Palette[highlight]);

        if (restore)
            memcpy(m_stdIcon->GetPalette(), saved.Palette, 0x200);
    }

    if (g_combatManager->m_computeExtentOnly != 0
        || (!is(creatureImmobilized) && !is(creatureSiegeWeapon) && !m_isMoving
            && (m_currFrameType == cs_wait
                || m_currFrameType == cs_fidget))) {
        int step = 1;
        long xoff;
        int yoff;
        if (m_facing == 0) {
            xoff = 0x34;
            yoff = -0x1e;
        } else {
            xoff = 0x16;
            yoff = -0xf;
        }
        if (is(creatureDoubleWide)) {
            xoff += 0x2c;
            ++step;
        }
        long neighborIndex = m_gridIndex + (m_facing == 0 ? -step : step);
        if ((g_combatManager->m_cells[neighborIndex].hasArmy()
             && !g_combatManager->m_cells[neighborIndex].getArmy()->m_isMoving)
            || (g_combatManager->m_cells[neighborIndex].m_attributes & hexcell::blocked)) {
            xoff -= 0x25;
            yoff = -0xf;
        } else {
            xoff += m_monFrameInfo.m_extraNumTroopsXOffset;
        }
        long numboxX = x + (m_facing == 0 ? -xoff : xoff);
        long numboxY = y + yoff;
        if (g_combatManager->drawObject(
                g_combatManager->NumberWindow, numboxX, numboxY)) {
            if (m_numSpellInfluences == 0) {
                g_windowManager->m_screenBitmap->Colorize(
                    numboxX + 1, numboxY + 1, 0x1c, 9, 0.75f, 0.8f);
            } else {
                g_windowManager->m_screenBitmap->Colorize(
                    numboxX + 1, numboxY + 1, 0x1c, 9,
                    static_cast<float>((computeKarma() + 1.0) * 0.1667f),
                    0.8f);
            }
            char countText[10];
            // The count reuses the spent x parameter.
            x = m_numTroopsToShowOverride == -1 ? m_numTroops
                                              : m_numTroopsToShowOverride;
            sprintf(countText,
                    DATA_COMPGEN(0x00660a1c, decimalFormat, "%d"), x);
            g_tinyFont->DrawBoundedString(
                countText, g_windowManager->m_screenBitmap, numboxX,
                numboxY, 0x1e, 0xf, font::WHITE, 1, -1);
        }
    }

    if (m_showPowEffect != 0 && numBoxOnly == 0) {
        if (g_combatManager->m_powFrameIndex
            < g_combatManager->m_powSprite->GetNumFrames(0)) {
            // The effect flags reuse x too.
            x = g_spellEffectTraits[g_combatManager->m_powSpellEffect]
                    .m_flags;
            switch (x & 0xf) {
            case SPELL_EFFECT_PLACE_OVERHEAD:
                powX = midX() - g_combatManager->m_powSprite->GetWidth() / 2;
                powY = bottomY() - g_combatManager->m_powSprite->GetHeight();
                break;
            case SPELL_EFFECT_PLACE_CENTERED:
                powX = midX() - g_combatManager->m_powSprite->GetWidth() / 2;
                powY = midY() - g_combatManager->m_powSprite->GetHeight() / 2;
                break;
            case SPELL_EFFECT_PLACE_ABOVE:
                powX = midX() - g_combatManager->m_powSprite->GetWidth() / 2;
                powY = topY() - g_combatManager->m_powSprite->GetHeight();
                break;
            case SPELL_EFFECT_PLACE_FLANK:
                powX = frontX();
                if (m_facing == 0)
                    powX -= g_combatManager->m_powSprite->GetWidth();
                powY = midY() - g_combatManager->m_powSprite->GetHeight() / 2;
                break;
            }
            g_combatManager->drawSpellEffect(
                g_combatManager->m_powSprite,
                g_combatManager->m_powFrameIndex, powX, powY, m_facing == 0,
                (static_cast<unsigned long>(x) >> 8) & 1);
        }
    }
}

// Original: army::ComputeKarma; army.cpp:891
// Inlined in DrawToBuffer; Complete's spell table grew from DC's 80 entries
// to 81.
DC_ADDRESS(0x044d50, 0xc2)
MAC_ADDRESS(0x04a6e4, 0xcc)
// Mac 0x4a720 reads row i before the karma table; DC ComputeKarma makes no
// get_spell_time call (see army.h getSpellTime).
double army::computeKarma() const
{
    if (m_numSpellInfluences == 0)
        return 0.0;
    long sum = 0;
    long absSum = 0;
    for (long i = 0; i < kNumSpellsAndCreatureEffects; i++) {
        if (m_spellInfluence[i] != 0) {
            sum += akSpellTraits[i].m_karma;
            absSum += abs(akSpellTraits[i].m_karma);
        }
    }
    if (absSum == 0)
        return 0.0;
    return sum / static_cast<double>(absSum);
}

// Append `arg` to `array` unless it is already there, answering whether
// it went in.

// STATIC: it has no retail out-of-line copy; both call sites in add_aura
// inline it.

// The membership test is std::find (DC army.obj carries its `army**`
// instantiation).
// E:\gamedcs\army.cpp:917
DC_ADDRESS(0x044e14, 0xac)
MAC_ADDRESS(0x04a7b0, 0xdc)
static unsigned char addItem(std::vector<army*>& array, army* arg)
{
    if (std::find(array.begin(), array.end(), arg) != array.end())
        return 0;
    array.push_back(arg);
    return 1;
}

// Drop the first occurrence of `arg` from `array`, searching BACKWARDS.
VA(0x0043ea00, 0x67)
DC_ADDRESS(0x044ec0, 0x4a)
MAC_ADDRESS(0x04a88c, 0x78)
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
VA(0x0043ea70, 0x1DD)
DC_ADDRESS(0x044f0c, 0xf2)
MAC_ADDRESS(0x04a904, 0x180)
void army::addAura()
{
    long count;
    if (is(creatureDoubleWide))
        count = 8;
    else
        count = 6;

    while (count-- > 0) {
        long hex = getAdjacentHex(count);
        if (!g_combatManager->validHex(hex))
            continue;
        army* other = g_combatManager->m_cells[hex].getArmy();
        if (!other)
            continue;
        if ((other->m_creatureType == ARMY_CREATURE_UNICORN
             || other->m_creatureType == ARMY_CREATURE_WAR_UNICORN)
            && other->getControllingSide() == getOwningSide()
            && !other->is(creatureImmobilized)) {
            addItem(other->m_auraClients, this);
            addItem(m_auraSources, other);
        }
        if ((m_creatureType == ARMY_CREATURE_UNICORN
             || m_creatureType == ARMY_CREATURE_WAR_UNICORN)
            && other->getOwningSide() == getControllingSide()
            && !other->is(creatureImmobilized)) {
            addItem(other->m_auraSources, this);
            addItem(m_auraClients, other);
        }
    }
}

// Tear this stack out of both halves of the aura relationship: every
// stack whose aura reaches it, and every stack its own aura reaches.
VA(0x0043ec50, 0x1B4)
DC_ADDRESS(0x045000, 0xbc)
MAC_ADDRESS(0x04aa84, 0xa8)
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
VA(0x0043ee10, 0x1C2)
DC_ADDRESS(0x0450bc, 0xa6)
MAC_ADDRESS(0x04ab2c, 0xc4)
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
VA(0x0043efe0, 0xCF)
DC_ADDRESS(0x045164, 0xa0)
MAC_ADDRESS(0x04abf0, 0x118)  // DC reads the side directly; Mac 0x4ac24's load is the same field read.
unsigned char army::setInsideAreaEffect(unsigned char arg)
{
    if (m_isAreaEffectTarget == arg)
        return 0;
    m_isAreaEffectTarget = arg;
    g_combatManager->markCreatureEffect(group, index);
    if (m_isAreaEffectTarget) {
        if (m_stdIcon->IsValidSeq(cs_fidget)
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

// E:\gamedcs\army.cpp:1062
// DC EndWalk, called at Walk:1163 and WalkTo:2439.
// Inlined; no standalone retail body.
DC_ADDRESS(0x045204, 0x50)
MAC_ADDRESS(0x04ad08, 0x8c)
void army::endWalk()
{
    if (!g_combatManager->isQuickCombat()) {
        playSample(POST_WALK_SAMPLE);
        if (m_armySample[WALK_SAMPLE])
            g_soundManager->stopSample(
                m_armySample[WALK_SAMPLE]->memSample.memHSample);
        playAnimation(cs_postwalk, -1, 0);
        playAnimation(cs_wait, 1, 0);
    }
}

// One hex of a walk: turn to face the step if it needs turning, publish
// the from/to pair the redraw reads, play the walk animation, and move
// the stack in the grid.
VA(0x0043f0b0, 0x206)
DC_ADDRESS(0x045254, 0x172)
MAC_ADDRESS(0x04ad94, 0x20c)
void army::walk(int direction, bool endWalk, bool initialWalk)
{
    if (initialWalk)
        setupAnimation();
    if (needToTurn(direction))
        turn(initialWalk);
    int nextCell = getAdjacentCellIndex(m_gridIndex, direction);
    g_walkingFrom = m_gridIndex;
    g_walkingTo = nextCell;
    g_walkingYMod = 0;
    if (is(creatureDoubleWide)) {
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
    if (endWalk)
        this->endWalk();
}

// E:\gamedcs\army.cpp:1171. Windows and Mac use (distance + 20)/40 flight
// frames.
VA(0x0043f2c0, 0x63B)
DC_ADDRESS(0x0453c8, 0x4d8)
MAC_ADDRESS(0x04afa0, 0x68c)  // MAC_ABSTRACTION_FROM(tokens1:1b8626a2ce74,31.8986): Windows recovery of the SLimitData/s_combatAreaLimits identities and DC's else-if missile chain; CW reallocates the restructured body (masked 255 -> 241 of 419).
void army::animateMissile(army* armyToAttack)
{
    if (g_combatManager->isQuickCombat())
        return;

    int targetX = armyToAttack->midX();
    int targetY = armyToAttack->midY();
    g_combatManager->resetLimitCreature();
    g_combatManager->markCreatureEffect(group, index);
    g_combatManager->computeMaxExtent();

    int startX;
    int startY;
    int missileFrame;
    combatManager::getMissileStartingPosition(m_creatureType,
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
        frames = m_stdIcon->GetNumFrames(m_currFrameType);
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
        g_combatManager->animateMagicArrow(startX, startY, armyToAttack);
    } else if (is(creatureShootsRay)) {
        GameTime::delay(static_cast<long>(
            combatManager::s_combatSpeedMod[g_config.m_combatSpeed] * 115.0f));
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
    } else {
        int width = m_missileIcon->GetWidth();
        int height = m_missileIcon->GetHeight();

        int nframes = (arrowtraveldist + 20) / 40;
        int stepX;
        int stepY;
        if (nframes > 0) {
            stepX = deltaX / nframes;
            stepY = deltaY / nframes;
        } else {
            stepX = deltaX;
            stepY = deltaY;
        }
        int x = startX - width / 2;
        int y = startY - height / 2;

        Bitmap16Bit saved(width, height);
        SLimitData updateArea = heroWindowManager::s_nullLimits;
        const int missileperiod = static_cast<int>(
            combatManager::s_combatSpeedMod[g_config.m_combatSpeed] * 33.0f);

        long frame = 0;
        if (nframes > 0) {
            for (; frame < nframes; frame++) {
                unsigned long nextFrameTime =
                    GameTime::get() + missileperiod;
                if (frame != 0) {
                    saved.Draw(0, 0, width, height,
                               g_windowManager->m_screenBitmap, x, y, false);
                    updateArea = SLimitData(x, y, x + width - 1, y + height - 1);
                    x += stepX;
                    y += stepY;
                }
                saved.Grab(g_windowManager->m_screenBitmap, x, y);
                m_missileIcon->Draw(0, missileFrame, 0, 0, width, height,
                                  g_windowManager->m_screenBitmap, x, y,
                                  targetX < startX, 1);
                // DC army.cpp:1326-1327: Include, then Clip.
                updateArea.include(SLimitData(x, y, x + width - 1, y + height - 1));
                updateArea.clip(combatManager::s_combatAreaLimits);
                // DC army.cpp:1335/1336 retains the extent calls.
                if (!g_combatManager->scrollTo(updateArea, true, true, true))
                    g_combatManager->updateCombatArea(updateArea);
                GameTime::delayTil(nextFrameTime);
            }
        }
        saved.Draw(0, 0, width, height, g_windowManager->m_screenBitmap, x, y, false);
        g_combatManager->updateCombatArea(x, y, width, height);  // DC army.cpp:1348
    }
}

// E:\gamedcs\army.cpp:1356
// One landed volley. The luck preamble is do_attack's statement for
// statement (same pinned get_controlling_side, same
// TTextResource::operator[](46)
// message); then the missile flies (animate_missile), the ammo counts
// down unless the owner wields the Ammo Cart,
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
VA(0x0043f900, 0x7F9)
DC_ADDRESS(0x0458a0, 0x5d0)
MAC_ADDRESS(0x04b62c, 0x6e4)  // dc-bracket forced
void army::rangeAttack(army* armyToAttack)
{
    checkLuck();
    animateMissile(armyToAttack);
    if (!getOwner()
        || !getOwner()->isWieldingArtifact(ARTIFACT_AMMO_CART))
        m_monInfo.numShots--;
    if (m_creatureType == CREATURE_MAGOG) {
        long effect = akSpellTraits[SPELL_FIREBALL].m_effect;
        if (effect != -1
            && !g_combatManager->isQuickCombat()) {
            launchSample(akSpellTraits[SPELL_FIREBALL].m_sample, -1, 3);
            CSprite* spr =
                ResourceManager::GetSprite(g_spellEffectTraits[effect]
                                               .m_name);
            long x = armyToAttack->midX() - spr->GetWidth() / 2;
            long y = armyToAttack->midY() - spr->GetHeight() / 2;
            g_combatManager->scrollTo(
                x + spr->GetWidth() / 2,
                y + spr->GetHeight() / 2, 1, 1, 1);
            for (long frame = 0; frame < spr->GetNumFrames(0); frame++) {
                g_combatManager->drawFrame(0, 0, 0, 50, 1, 1);
                g_combatManager->drawSpellEffect(spr, frame, x, y, 0, 0);
                g_combatManager->updateCombatArea();
            }
            g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
            ResourceManager::Dispose(spr);
        }
        g_combatManager->clearEffects();
        int killed = 0;
        long damage = 0;
        army* first = 0;
        unsigned char multiple = 0;
        int dmg;
        int killedNow;
        for (long i = 0; i < COMBAT_DIRECTION_COUNT + 1; i++) {
            long hex;
            if (i == COMBAT_DIRECTION_COUNT)
                hex = targetCellIndex;
            else
                hex = getAdjacentCellIndexNoArmy(targetCellIndex, i);
            hexcell* cell = &g_combatManager->m_cells[hex];
            if (!g_combatManager->validHex(hex) || !cell->hasArmy())
                continue;
            const int slot = cell->m_armySlot;
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
            if (multiple)
                first = 0;
            g_combatManager->damageMessage(getName(),
                                            m_numTroops, damage, first,
                                            killed);
            g_combatManager->powEffect(TSpellEffectID(effect), 1);
        }
    } else if (m_creatureType == CREATURE_LICH
               || m_creatureType == CREATURE_POWER_LICH) {
        long effect = akSpellTraits[SPELL_DEATH_CLOUD].m_effect;
        if (effect != -1
            && !g_combatManager->isQuickCombat()) {
            launchSample(akSpellTraits[SPELL_DEATH_CLOUD].m_sample, -1,
                          3);
            CSprite* spr =
                ResourceManager::GetSprite(g_spellEffectTraits[effect]
                                               .m_name);
            long x = armyToAttack->midX() - spr->GetWidth() / 2;
            long y = armyToAttack->midY() - spr->GetHeight() / 2;
            for (long frame = 0; frame < spr->GetNumFrames(0); frame++) {
                g_combatManager->drawFrame(0, 0, 0, 100, 1, 1);
                g_combatManager->drawSpellEffect(spr, frame, x, y, 0, 0);
                g_combatManager->updateCombatArea();
            }
            g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
            ResourceManager::Dispose(spr);
        }
        g_combatManager->clearEffects();
        int killed = 0;
        int damage = 0;
        army* first = 0;
        unsigned char multiple = 0;
        int dmg;
        int killedNow;
        for (long i = 0; i < COMBAT_DIRECTION_COUNT + 1; i++) {
            long hex;
            if (i == COMBAT_DIRECTION_COUNT)
                hex = targetCellIndex;
            else
                hex = getAdjacentCellIndexNoArmy(targetCellIndex, i);
            hexcell* cell = &g_combatManager->m_cells[hex];
            if (!g_combatManager->validHex(hex) || !cell->hasArmy())
                continue;
            const int slot = cell->m_armySlot;
            const int side = cell->m_armySide;
            army* a = cell->getArmy();
            if (i != COMBAT_DIRECTION_COUNT && !a->is(creatureAlive))
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
            g_combatManager->powEffect(eSpellEffectNone, 1);
        }
    } else {
        int killed;
        int damage;
        damageEnemy(armyToAttack, &damage, &killed, 1);
        g_combatManager->powEffect(eSpellEffectNone, 0);
        g_combatManager->damageMessage(getName(), m_numTroops, damage,
                                        armyToAttack, killed);
        if (g_combatManager->isQuickCombat())
            return;
        waitSample(SHOOT_SAMPLE);
    }
}

// One stack's whole shooting turn: resolve the target it was told to
// attack, turn to face it, fire between one and three volleys, turn
// back, and drop the spells an attack cancels.
VA(0x00440160, 0x1A6)
DC_ADDRESS(0x045e70, 0x150)
MAC_ADDRESS(0x04bd10, 0x1a0)
void army::rangeAttack()
{
    m_yModify = 0;
    if (groupToAttack < 0)
        return;
    if (indexToAttack < 0)
        return;
    army* target = &g_combatManager->m_armies[groupToAttack][indexToAttack];
    if (m_creatureType == ARMY_CREATURE_ARROW_TOWER) {
        g_combatManager->keepAttack(indexToAttack);
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
    if (is(creatureTwoAttacks) && target->m_numTroops > 0)
        rangeAttack(target);
    if (m_creatureType == ARMY_CREATURE_BALLISTA && target->m_numTroops > 0
        && getController()
        && getController()->m_skillLevel[eSecSkillBattlefieldBallistics]
               > 1) {
        rangeAttack(target);
    }
    if (m_facing != oldFacing) {
        setupAnimation();
        turn(1);
    }
    cancelSpellType(ARMY_CANCEL_SPELLS_AFTER_ATTACK);
}

// E:\gamedcs\army.cpp:1629 / 1643; inlined at every Windows use. One hex
// step around the combat ring: a one-hex stack walks its six neighbours
// with +-1 modulo 6, a two-hex stack has eight and they are not in ring
// order, so it goes through the index/order table pair.
DC_ADDRESS(0x045fc0, 0x46)
MAC_ADDRESS(0x04beb0, 0x60)
long army::getClockwise(long direction) const
{
    if (is(creatureDoubleWide))
        return g_wideDirectionRingOrder[
            (g_wideDirectionRingIndex[direction] + 1) % 8];
    return (direction + 1) % COMBAT_DIRECTION_COUNT;
}

DC_ADDRESS(0x046008, 0x46)
MAC_ADDRESS(0x04bf10, 0x60)
long army::getCounterClockwise(long direction) const
{
    if (is(creatureDoubleWide))
        return g_wideDirectionRingOrder[
            (g_wideDirectionRingIndex[direction] + 7) % 8];
    return (direction + 5) % COMBAT_DIRECTION_COUNT;
}

// THE MASK IS A SKIP MASK, not a hit mask: `test attackMask, 1 << i`
// jumps to the increment when the bit is SET. The two accumulator
// pointers are read-modify-written, never initialised, so the caller
// owns them across however many sweeps a turn makes.
VA(0x00440310, 0x1EB)
DC_ADDRESS(0x046050, 0x150)
MAC_ADDRESS(0x04bf70, 0x1fc)
void army::doMultiHeadAttack(unsigned attackMask, int* damageAmount, int* killed,
                                long* fireDamage)
{
    army* firstTarget = 0;
    int tempDamage;
    int tempKilled;
    unsigned char mixedTypes = 0;
    for (int i = 0; i < COMBAT_DIRECTION_WIDE_LOWER + 1; i++) {
        if (attackMask & (1 << i))
            continue;
        long hex = getAdjacentHex(i);
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
        g_combatManager->markCreatureEffect(target->group,
                                            target->index);
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

// Original public ?check_special_attack@army@@QAA_NPAV1@@Z proves bool.
// The dendroid arm is two add_item calls, the first one guarded: an
// already-bound target returns without re-raising the pending spell or the
// mirror link.
VA(0x00440500, 0x4B4)
DC_ADDRESS(0x0461a0, 0x33e)
MAC_ADDRESS(0x04c16c, 0x440)
bool army::checkSpecialAttack(army* target)
{
    switch (m_creatureType) {
    case CREATURE_GHOST_DRAGON:
        if (target->is(creatureAlive) && sRandom(1, 100) <= 20
            && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_AGE,
                                               getControllingSide(),
                                               target, 1, 1))
            target->m_postPowSpellToCast = SPELL_AGE;
        return 0;
    case CREATURE_ZOMBIE:
        if (target->is(creatureAlive) && sRandom(1, 100) <= 20
            && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_DISEASE,
                                               getControllingSide(),
                                               target, 1, 1))
            target->m_postPowSpellToCast = SPELL_DISEASE;
        return 0;
    case ARMY_CREATURE_UNICORN:
    case ARMY_CREATURE_WAR_UNICORN:
        if (sRandom(1, 100) <= 20 && target->m_numTroops > 0
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
        if (sRandom(1, 100) <= 25 && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_CURSE,
                                               getControllingSide(),
                                               target, 1, 1))
            target->m_postPowSpellToCast = SPELL_CURSE;
        return 0;
    case CREATURE_MEDUSA:
    case CREATURE_MEDUSA_QUEEN:
    case CREATURE_BASILISK:
    case CREATURE_GREATER_BASILISK:
        if (sRandom(1, 100) <= 20 && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_STONE,
                                               getControllingSide(),
                                               target, 1, 1)) {
            target->m_postPowSpellToCast = SPELL_STONE;
            return 1;
        }
        return 0;
    case CREATURE_RUST_DRAGON:
        if (target->m_numTroops > 0 && target->m_monInfo.defenseSkill > 0)
            target->m_postPowSpellToCast = SPELL_ACID_BREATH_DEFENSE;
        return 0;
    case CREATURE_WYVERN_MONARCH:
        if (target->is(creatureAlive) && sRandom(1, 100) <= 30
            && target->m_numTroops > 0
            && g_combatManager->spellCastWorks(SPELL_POISON,
                                               getControllingSide(),
                                               target, 1, 1))
            target->m_postPowSpellToCast = SPELL_POISON;
        return 0;
    case CREATURE_SCORPICORE:
        if (sRandom(1, 100) <= 20 && target->m_numTroops > 0
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
VA(0x004409c0, 0x1F9)
DC_ADDRESS(0x0464e0, 0x178)
MAC_ADDRESS(0x04c5ac, 0x258)
void army::doFireShield(long damageAmount)
{
    long side;
    int i;
    army* a;
    g_combatManager->resetLimitCreature();
    // Direct side read: DC DoFireShield has no get_owning_side call.
    g_combatManager->markCreatureEffect(group, index);
    for (side = 0; side < 2; side++) {
        a = &g_combatManager->m_armies[side][0];
        for (i = g_combatManager->m_numArmies[side]; i-- > 0; a++) {
            if (a->m_showFireShield) {
                g_combatManager->markCreatureEffect(side, a->index);
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
        sample = loadPlaySample(akSpellTraits[SPELL_FIRE_SHIELD].m_sample);
    g_combatManager->powEffect(eSpellEffectFireShield, 0);
    g_combatManager->damageMessage(akSpellTraits[SPELL_FIRE_SHIELD].m_name, 1,
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
VA(0x00440bc0, 0xA41)
DC_ADDRESS(0x046658, 0x592)
MAC_ADDRESS(0x04c804, 0xa84)  // anchor-global
void army::doPostAttack(army* target, int attackDamage, int killedCount,
                          int totalLife)
{
    switch (m_creatureType) {
    case CREATURE_VAMPIRE_LORD:
        if (target->is(creatureAlive)) {
            long deadVampires = 0;
            long missingTroops = m_origNumTroops - m_numTroops;
            long damageRecovered = min(attackDamage, totalLife);
            damageRecovered = min(damageRecovered,
                m_monInfo.hitPoints * missingTroops + m_topCreatureDamage);
            m_topCreatureDamage -= damageRecovered;
            if (m_topCreatureDamage < 0) {
                deadVampires =
                    (m_monInfo.hitPoints - m_topCreatureDamage - 1) / m_monInfo.hitPoints;
                m_topCreatureDamage += m_monInfo.hitPoints * deadVampires;
                m_numTroops += deadVampires;
            }
            if (damageRecovered > 0) {
                std::string text;
                const char* targetName =
                    target->getName(target->m_numTroops + killedCount);
                if (m_numTroops - deadVampires == 1)
                    text = formatString(g_generalText->GetText(GENERAL_TEXT_LIFE_DRAIN_ONE_FORMAT),
                                         getName(m_numTroops - deadVampires),
                                         damageRecovered, targetName);
                else
                    text = formatString(g_generalText->GetText(GENERAL_TEXT_LIFE_DRAIN_MANY_FORMAT),
                                         getName(m_numTroops - deadVampires),
                                         damageRecovered, targetName);
                if (deadVampires > 0) {
                    if (deadVampires == 1)
                        text += g_generalText->GetText(GENERAL_TEXT_LIFE_DRAIN_RAISE_ONE_SUFFIX);
                    else
                        text += formatString(g_generalText->GetText(GENERAL_TEXT_LIFE_DRAIN_RAISE_MANY_SUFFIX_FORMAT),
                                              deadVampires);
                }
                if (!g_combatManager->isQuickCombat()) {
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
        if (target->is(creatureAlive)) {
            for (long i = 0; i < m_numTroops; i++) {
                if (random(1, 100) <= 10)
                    stares++;
            }
            long dead = min(stares, target->m_numTroops);
            dead = min(dead, (m_numTroops + 9) / 10);
            if (dead > 0) {
                long damage =
                    target->m_monInfo.hitPoints * dead - target->m_topCreatureDamage;
                std::string text;
                if (dead == 1)
                    text = formatString(g_generalText->GetText(GENERAL_TEXT_DEATH_STARE_ONE_FORMAT),
                                         target->getName(dead),
                                         getName());
                else
                    text = formatString(g_generalText->GetText(GENERAL_TEXT_DEATH_STARE_MANY_FORMAT),
                                         dead,
                                         target->getName(dead),
                                         getName());
                SAMPLE2 sample;
                if (!g_combatManager->isQuickCombat()) {
                    g_combatManager->m_combatWindow->combatMessage(
                        text.c_str(), 1, 0);
                    sample = loadPlaySample(
                        akSpellTraits[SPELL_DEATH_STARE].m_sample);
                    g_combatManager->spellEffect(80, target, 100, 0);
                }
                target->damage(damage);
                g_combatManager->powEffect(eSpellEffectNone, 1);
                if (!g_combatManager->isQuickCombat())
                    waitEndSample(sample, -1);
            }
        }
        break;
    }

    case CREATURE_THUNDERBIRD:
        if (target->m_numTroops > 0 && sRandom(1, 100) <= 20) {
            if (g_combatManager->spellCastWorks(SPELL_LIGHTNING_BOLT,
                                                getControllingSide(),
                                                target, 1, 1)) {
                long damage = m_numTroops * 10;
                damage = g_combatManager->modifySpellDamage(
                    damage, SPELL_LIGHTNING_BOLT, 0, 0, target, 0);
                if (damage > 0) {
                    std::string text;
                    SAMPLE2 sample;
                    if (!g_combatManager->isQuickCombat()) {
                        text = formatString(
                            g_generalText->GetText(GENERAL_TEXT_LIGHTNING_STRIKE_FORMAT),
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
                        akSpellTraits[SPELL_LIGHTNING_BOLT].m_name, 1,
                        damage, target, killed);
                    target->m_showPowEffect = 1;
                    g_combatManager->powEffect(eSpellEffectLightningDust, 1);
                    if (!g_combatManager->isQuickCombat())
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
                                           target->m_gridIndex, 1, -1, eMasteryNone,
                                           3);
            if (m_creatureType == CREATURE_DRAGON_FLY
                && target->m_spellInfluence[SPELL_WEAKNESS] == 0) {
                if (g_combatManager->spellCastWorks(
                        SPELL_WEAKNESS, getControllingSide(), target,
                        1, 1))
                    g_combatManager->castSpell(SPELL_WEAKNESS,
                                               target->m_gridIndex, 1, -1,
                                               eMasteryAdvanced, 3);
            }
        }
        break;

    case CREATURE_RUST_DRAGON:
        if (target->m_numTroops > 0 && sRandom(1, 100) <= 20) {
            long damage = sRandom(m_monInfo.damageLowBound, m_monInfo.damageHighBound) * m_numTroops / 2;
            if (damage > 0) {
                std::string text;
                SAMPLE2 sample;
                if (!g_combatManager->isQuickCombat())
                    sample = loadPlaySample(
                        akSpellTraits[SPELL_ACID_BREATH_DEFENSE]
                            .m_sample);
                long killed = target->damage(damage);
                g_combatManager->damageMessage(
                    akSpellTraits[SPELL_ACID_BREATH_DEFENSE].m_name, 1,
                    damage, target, killed);
                if (!g_combatManager->isQuickCombat()) {
                    g_combatManager->spellEffect(81, target, 100, 0);
                    waitEndSample(sample, -1);
                }
                g_combatManager->powEffect(eSpellEffectNone, 1);
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

// Dreamcast records the three MarkCreatureEffect source calls; the
// canonical helper is retained at each site and expands in retail. Mac
// 0x4d3e8 expands the same owning-side accessor for this stack as for
// the target and the stack behind it; keep that accessor at all three sites.

// Original public ?do_attack@army@@AAA_NPAV1@H@Z proves this overload is
// private and returns bool. The direction overload is the public entry;
// all calls to the single-swing helper remain inside army. Mac forwards
// CheckSpecialAttack's byte result, replacing it with true for fresh blind.
// Mac 0x4d3e8, 0x4d488 and 0x4d4e4 load the same full-width side
// returned by getOwningSide at each marking site. Missing retained DC calls
// do not disprove those expansions. Restoring the three canonical calls is
// byte-flat at 99.9232%; the residual is two effect-address decompositions
// and the independent EDI/EBX reload order after the hydra call.
VA(0x00441610, 0x6A0)
DC_ADDRESS(0x046bec, 0x3c2)
MAC_ADDRESS(0x04d288, 0x638)  // corroborates
bool army::doAttack(army* armyToAttack, int direction)
{
    unsigned attackMask;
    army* behind = 0;
    g_combatManager->resetHitByCreature();
    if (is(creatureMultiHeaded)) {
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
        if (is(creatureHasExtendedAttack)) {
            int adjacentHex = getAdjacentHex(direction);
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
    g_combatManager->markCreatureEffect(getOwningSide(), index);
    checkLuck();
    int damage = 0;
    int killed = 0;
    int nextDamage = 0;
    int nextKilled = 0;
    long fireDamage = 0;
    long totalLife = 0;
    if (is(creatureMultiHeaded)) {
        doMultiHeadAttack(attackMask, &damage, &killed,
                             &fireDamage);
    } else {
        g_combatManager->markCreatureEffect(armyToAttack->getOwningSide(),
                                            armyToAttack->index);
        if (behind)
            g_combatManager->markCreatureEffect(behind->getOwningSide(),
                                                behind->index);
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
        && !g_combatManager->isQuickCombat()) {
        if (direction == COMBAT_DIRECTION_WIDE_UPPER
            || direction == COMBAT_DIRECTION_5
            || direction == COMBAT_DIRECTION_0) {
            if (behind && m_stdIcon->IsValidSeq(cs_special_ur))
                m_showAttackFrameType = cs_special_ur;
            else if (m_creatureType == ARMY_CREATURE_BALLISTA)
                m_showAttackFrameType = cs_range_ur;
            else
                m_showAttackFrameType = cs_attack_ur;
        } else if (direction == COMBAT_DIRECTION_1
                   || direction == COMBAT_DIRECTION_4) {
            if (behind && m_stdIcon->IsValidSeq(cs_special_r)) {
                m_showAttackFrameType = cs_special_r;
            } else if (m_creatureType == ARMY_CREATURE_BALLISTA) {
                m_showAttackFrameType = cs_range_r;
            } else {
                m_showAttackFrameType = cs_attack_r;
            }
        } else {
            if (behind && m_stdIcon->IsValidSeq(cs_special_dr))
                m_showAttackFrameType = cs_special_dr;
            else if (m_creatureType == ARMY_CREATURE_BALLISTA)
                m_showAttackFrameType = cs_range_dr;
            else
                m_showAttackFrameType = cs_attack_dr;
        }
    } else {
        m_showAttackFrameType = cs_attack_r;
    }
    bool special = checkSpecialAttack(armyToAttack);
    g_combatManager->powEffect(eSpellEffectNone, 0);
    if (!is(creatureMultiHeaded)) {
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
VA(0x00441cb0, 0x2BE)
DC_ADDRESS(0x046fb0, 0x2c0)
MAC_ADDRESS(0x04d8c0, 0x36c)
void army::doAttack(int direction)
{
    m_drawPriority = 6;
    int hex = getAdjacentHex(direction);
    if (!g_combatManager->validHex(hex))
        return;
    army* armyToAttack = g_combatManager->m_cells[hex].getArmy();
    if (!armyToAttack)
        return;
    int savedArmyToAttackFacing = armyToAttack->m_facing;
    int counterDirection = armyToAttack->getAttackDirection(this);
    if (armyToAttack->needToTurn(counterDirection)) {
        int savedSide = g_combatManager->m_actingSide;
        int savedSlot = g_combatManager->m_actingSlot;
        g_combatManager->m_actingSide = armyToAttack->group;
        g_combatManager->m_actingSlot = armyToAttack->index;
        armyToAttack->setupAnimation();
        armyToAttack->turn(1);
        g_combatManager->m_actingSide = savedSide;
        g_combatManager->m_actingSlot = savedSlot;
    }
    if (armyToAttack->m_spellInfluence[62])
        armyToAttack->m_residualBlindness = 1;
    if (armyToAttack->m_spellInfluence[74])
        armyToAttack->m_residualParalyze = 1;
    bool killed = doAttack(armyToAttack, direction);
    m_joustBonus = 0;
    if (armyToAttack->m_numTroops > 0
        && armyToAttack->canRetaliate(*this) && !killed) {
        GameTime::delay(static_cast<int>(
            combatManager::s_combatSpeedMod[g_config.m_combatSpeed] * 150.0f));
        g_combatManager->m_currentSide = 1 - g_combatManager->m_currentSide;
        armyToAttack->doAttack(this, counterDirection);
        g_combatManager->m_currentSide = 1 - g_combatManager->m_currentSide;
        armyToAttack->m_retaliationCount--;
    }
    armyToAttack->m_residualBlindness = 0;
    armyToAttack->m_residualParalyze = 0;
    if (is(creatureTwoAttacks) && armyToAttack->m_numTroops > 0 && !is(creatureShootingArmy)
        && !isIncapacitated() && m_numTroops > 0) {
        GameTime::delay(static_cast<int>(
            combatManager::s_combatSpeedMod[g_config.m_combatSpeed] * 150.0f));
        doAttack(armyToAttack, direction);
    }
    if (!armyToAttack->is(creatureImmobilized)) {
        if (savedArmyToAttackFacing != armyToAttack->m_facing) {
            int savedSide = g_combatManager->m_actingSide;
            int savedSlot = g_combatManager->m_actingSlot;
            g_combatManager->m_actingSide = armyToAttack->group;
            g_combatManager->m_actingSlot = armyToAttack->index;
            armyToAttack->setupAnimation();
            armyToAttack->turn(1);
            g_combatManager->m_actingSide = savedSide;
            g_combatManager->m_actingSlot = savedSlot;
        }
    }
    cancelSpellType(ARMY_CANCEL_SPELLS_AFTER_ATTACK);
    groupToAttack = -1;
}

// The arrow-tower guard in front of the combat manager's landmine /
// fire-wall worker (0x46a570). Two things worth recording.
VA(0x00441f70, 0x26)
DC_ADDRESS(0x047270, 0x84)
MAC_ADDRESS(0x04dc2c, 0x44)
// Original ?check_obstacle_attacks@army@@QAA_N_N@Z proves both
// boolean domains; Complete delegates the older inline operation.
bool army::checkObstacleAttacks(bool isWalking)
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

// DC names/types: save_facing (int), direction and both next_cell locals
// (const int). Stopping happens before revealing the obstacle (DC 2459
// before GetObstacle:2461, likewise 2481/2483).
// cancelSpellType(AFTER_MOVE) currently cancels nothing; Mac retains the
// call (0:0x4dfd0).
VA(0x00441fa0, 0x461)
DC_ADDRESS(0x0472f4, 0x2f8)
MAC_ADDRESS(0x04dc70, 0x3a4)  // anchor-global
bool army::walkTo(int destIndex, bool restoreFacing)
{
    groupToAttack = indexToAttack = -1;
    if (!findPath(destIndex, getSpeed(), 0, 0))
        return 0;
    int saveFacing = m_facing;
    removeAura();
    removeBinding();
    bool succeeded = true;
    long stop;
    if (!g_combatManager->m_creaturePlacement) {
        stop = g_searchArray->getPathSteps() - getSpeed();
        if (stop < 0)
            stop = 0;
    } else {
        stop = 0;
    }
    long last = g_searchArray->getPathSteps() - 1;
    bool initialWalk = true;
    m_isMoving = 1;
    m_joustBonus = last - stop + 1;
    for (long i = last; i >= stop; i--) {
        const int direction = g_searchArray->getStep(i);
        const int nextCell = getAdjacentCellIndex(m_gridIndex, direction);
        if (g_combatManager->shouldLowerDoor(this, nextCell)) {
            if (!initialWalk) {
                endWalk();
                m_currFrameType = cs_wait;
                m_currFrameIndex = 0;
                g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
            }
            g_combatManager->lowerDoor();
            g_combatManager->m_extent = heroWindowManager::s_nullLimits;
            initialWalk = 1;
        }
        if (g_searchArray->isMoat(static_cast<short>(nextCell))) {
            stop = i;
            succeeded = 0;
        } else if (g_combatManager->m_cells[nextCell].m_attributes & hexcell::quicksand) {
            stop = i;
            succeeded = 0;
            g_combatManager->getObstacle(
                g_combatManager->m_cells[nextCell].m_obstacleIndex)
                .m_isVisible = 1;
        }
        if (is(creatureDoubleWide)) {
            const int nextCell = getAdjacentCellIndex(m_gridIndex, direction)
                                 + offsetToFront(-1);
            if (g_searchArray
                    ->isMoat(static_cast<short>(nextCell))) {
                stop = i;
                succeeded = 0;
            } else if (g_combatManager->m_cells[nextCell].m_attributes & hexcell::quicksand) {
                stop = i;
                succeeded = 0;
                g_combatManager->getObstacle(
                    g_combatManager->m_cells[nextCell].m_obstacleIndex)
                    .m_isVisible = 1;
            }
        }
        walk(g_searchArray->getStep(i), i == stop, initialWalk);
        initialWalk = 0;
        checkObstacleAttacks(i != stop);
        if (m_numTroops <= 0) {
            succeeded = 0;
            break;
        }
    }
    if (m_numTroops > 0) {
        if (m_facing != saveFacing && restoreFacing)
            turn(1);
        addAura();
        m_currFrameType = cs_wait;
        m_currFrameIndex = 0;
    }
    m_isMoving = 0;
    cancelSpellType(ARMY_CANCEL_SPELLS_AFTER_MOVE);
    g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
    g_combatManager->testRaiseDoor();
    return succeeded;
}

// E:\gamedcs\army.cpp:2528
DC_ADDRESS(0x0475ec, 0xa4)
MAC_ADDRESS(0x04e014, 0x160)
inline void army::checkLuck()
{
    m_luckStatus = 0;
    if (getController() && m_luck > 0) {
        // Mac 0x4e084 calls sRandom; Windows folds its body with random.
        if (sRandom(1, 24) <= min(m_luck, 3)) {
            m_luckStatus = 1;
            if (!g_combatManager->isQuickCombat()) {
                launchSample(DATA_COMPGEN(0x00660a20, goodLuckSampleName,
                                           "goodluck.82m"),
                              -1, 3);
                sprintf(g_text, g_generalText->GetText(GENERAL_TEXT_GOOD_LUCK_FORMAT), getName());
                g_combatManager->m_combatWindow->combatMessage(g_text, 1, 0);
                g_combatManager->spellEffect(
                    eSpellEffectFortune, this, 100, 0);
            }
        }
    }
}

VA(0x00442410, 0x13B)
DC_ADDRESS(0x047690, 0x128)
MAC_ADDRESS(0x04e174, 0x160)
// Original DC ?get_adjusted_attack@army@@QBAJPBV1@_N@Z proves bool
// rangedAttack, matching the predicate parameter used by the modifier leaf.
long army::getAdjustedAttack(const army* enemy,
                               bool rangedAttack) const
{
    long attack = m_monInfo.attackSkill;
    if (rangedAttack) {
        if (m_spellInfluence[44])
            attack += m_precisionAmount;
    } else {
        if (m_spellInfluence[43])
            attack += m_bloodlustAmount;
    }
    if (m_spellInfluence[55] && enemy) {
        if ((enemy->is(creatureKing1) && m_slayerLevel >= 0)
            || (enemy->is(creatureKing2) && m_slayerLevel >= 2)
            || (enemy->is(creatureKing3) && m_slayerLevel >= 3)) {
            attack += 8;
            if (getController()) {
                hero* castingHero = getController();
                attack += castingHero->getHeroSpellBonus(55, m_monInfo.level, 8);
            }
        }
    }
    if (m_spellInfluence[56])
        return static_cast<long>(
            getAdjustedDefense(enemy, 0) * m_frenzyFactor + attack);
    return attack;
}

// DC2624/2626 separates the adjusted call from the base-skill subtraction.
VA(0x00442550, 0x35)
DC_ADDRESS(0x0477b8, 0x30)
MAC_ADDRESS(0x04e2d4, 0x48)
// Original DC ?get_attack_modifier@army@@QBAJPBV1@_N@Z proves the same
// bool contract as getAdjustedAttack; preserve the call and named result.
long army::getAttackModifier(const army* enemy,
                               bool rangedAttack) const
{
    long adjusted = getAdjustedAttack(enemy, rangedAttack);
    return adjusted - akCreatureTypeTraits[m_creatureType].attackSkill;
}

VA(0x00442590, 0xC2)
DC_ADDRESS(0x0477e8, 0xde)
MAC_ADDRESS(0x04e31c, 0x17c)
// Original public ?get_adjusted_defense@army@@QBAJPBV1@_N@Z proves the
// Frenzy flag bool.
long army::getAdjustedDefense(const army* enemy,
                                bool frenzyIncluded) const
{
    if (frenzyIncluded && m_spellInfluence[56])
        return 0;
    long defense = m_monInfo.defenseSkill;
    if (enemy) {
        if (enemy->m_creatureType == ARMY_CREATURE_BEHEMOTH)
            defense = static_cast<long>(defense - defense * 0.4f);
        else if (enemy->m_creatureType == ARMY_CREATURE_ANCIENT_BEHEMOTH)
            defense = static_cast<long>(defense - defense * 0.8f);
    }
    if (g_combatManager->m_moatOn) {
        int secondHex;
        if (is(creatureDoubleWide))
            secondHex = m_gridIndex + offsetToFront(-1);
        else
            secondHex = -1;
        if (g_combatManager->isInMoat(m_gridIndex, 0)
            || g_combatManager->isInMoat(secondHex, 0))
            defense -= 3;
    }
    return defense;
}

VA(0x00442660, 0x29)
DC_ADDRESS(0x0478c8, 0x32)
MAC_ADDRESS(0x04e498, 0x50)
long army::getDefenseModifier() const
{
    long adjusted = getAdjustedDefense(0, 1);
    return adjusted - akCreatureTypeTraits[m_creatureType].defenseSkill;
}

// E:\gamedcs\army.cpp:2680: the recovered base modifier returns 1.0.
// Complete's full damage-reduction operation starts from the same baseline.
DC_ADDRESS(0x0478fc, 0x8)
inline double army::getDefenseDamageModifier(
    unsigned char rangedAttack) const
{
    return 1.0;
}

// The controller/owner pair; they differ by exactly the hypnotize flip.
VA(0x00442690, 0x39)
DC_ADDRESS(0x047904, 0x1e)
MAC_ADDRESS(0x04e4e8, 0x34)
hero* army::getController() const
{
    return g_combatManager->m_heroes[getControllingSide()];
}

VA(0x004426d0, 0x14)
DC_ADDRESS(0x047924, 0x1e)
MAC_ADDRESS(0x04e51c, 0x1c)
hero* army::getOwner() const
{
    return g_combatManager->m_heroes[getOwningSide()];
}

// E:\gamedcs\army.cpp:2708
DC_ADDRESS(0x047944, 0xd0)
MAC_ADDRESS(0x04e538, 0xe4)
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

VA(0x004426f0, 0x8B)
DC_ADDRESS(0x047a14, 0xe4)
MAC_ADDRESS(0x04e61c, 0xd8)
double army::getAverageDamage() const
{
    if (m_spellInfluence[41])
        return m_blessAmount + m_monInfo.damageHighBound;
    if (m_spellInfluence[42])
        return cppMax(m_monInfo.damageLowBound - m_curseAmount, 1);
    return (m_monInfo.damageHighBound + m_monInfo.damageLowBound) / 2.0;
}

VA(0x00442780, 0x100)
DC_ADDRESS(0x047af8, 0xd2)
MAC_ADDRESS(0x04e6f4, 0xdc)
// Original DC public ?get_average_damage@army@@QBAJABV1@_NJ1J@Z proves
// const army& and both bool flags.
long army::getAverageDamage(const army& enemy, bool rangedAttack, long amount, bool limitDamage, long distance) const
{
    double average = getAverageDamage();
    long damage = adjustDamage(const_cast<army*>(&enemy),
                                static_cast<long>(amount * average),
                                rangedAttack, 1, distance, 0);
    long totalLife = enemy.getTotalHitPoints(0);
    if (damage < 1)
        damage = 1;
    if (rangedAttack && is(creatureTwoAttacks))
        damage += damage;
    if (limitDamage && damage > totalLife)
        damage = totalLife;
    return damage;
}

// Original ?is_enemy@army@@QBA_NPBV1@@Z proves the bool result.
VA(0x00442880, 0x68)
DC_ADDRESS(0x047bcc, 0x38)
MAC_ADDRESS(0x04e7d0, 0x70)
bool army::isEnemy(const army* arg) const
{
    if (!arg)
        return 0;
    if (this == arg)
        return 0;
    if (!m_spellInfluence[59] && !arg->m_spellInfluence[59])
        return getControllingSide() != arg->getOwningSide();
    return 1;
}

// DC public ?can_shoot@army@@QBA_NPBV1@@Z proves a native bool result. DC
// line 2797 is a combined predicate; Complete adds the controller artifact
// exemption before adjacency.
VA(0x004428f0, 0xF6)
DC_ADDRESS(0x047c04, 0x6e)
MAC_ADDRESS(0x04e840, 0x118)
bool army::canShoot(const army* excluded) const
{
    if (m_creatureType == ARMY_CREATURE_BALLISTA
        || m_creatureType == ARMY_CREATURE_ARROW_TOWER)
        return 1;
    if (!is(creatureShootingArmy) || m_monInfo.numShots <= 0)
        return 0;
    hero* controller = getController();
    return ((controller
             && controller->isWieldingArtifact(ARTIFACT_BOW_OF_THE_SHARPSHOOTER))
            || !enemyIsAdjacent(excluded))
           && (m_spellInfluence[61] == 0 || m_forgetfulnessLevel < 2);
}

// Original ?enemy_is_adjacent@army@@QBA_NPBV1@@Z proves the bool result.
VA(0x004429f0, 0x5C)
DC_ADDRESS(0x047c74, 0x80)
MAC_ADDRESS(0x04e958, 0x94)
bool army::enemyIsAdjacent(const army* excluded) const
{
    if (g_combatManager->enemyIsAdjacent(this, m_gridIndex, excluded))
        return 1;
    if (is(creatureDoubleWide))
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
// Dreamcast recovers the source boundaries: attack/defense modifiers,
// defense-damage adjustment, can_shoot(0), three independent controller
// reads, get_average_damage, and two get_total_hit_points(0) calls.

// The side-mass exclusion mask is 0x600040: siege, immobilized and summoned
// armies.
VA(0x00442a50, 0x410)
DC_ADDRESS(0x047cf4, 0x472)
MAC_ADDRESS(0x04e9ec, 0x30c)  // anchor-global
// Original DC public ?get_unit_combat_value@army@@QBANJJ_NPBV1@@Z.
double army::getUnitCombatValue(long lowestAttack, long lowestDefense,
                                   bool ranged,
                                   const army* excluded) const
{
    long attackDiff = getAttackModifier(0, ranged) - lowestAttack;
    long defenseDiff = getDefenseModifier() - lowestDefense;
    double defense = computeDefenderDamageReduction(ranged);
    defense = (defenseDiff * 0.05 + 1.0) * defense;
    if (ranged && !canShoot(0))
        ranged = 0;
    double attack = attackDiff * 0.05 + 1.0;
    if (!ranged && is(creatureShootingArmy))
        attack = attack * 0.5;
    if (m_creatureType == ARMY_CREATURE_BALLISTA) {
        if (getController()) {
            if (getController()
                    ->getSecondarySkill(eSecSkillBattlefieldBallistics) > 1)
                attack = attack + attack;
            attack =
                attack
                * g_artilleryFactors[getController()->getSecondarySkill(
                    eSecSkillBattlefieldBallistics)];
        }
    }
    if (m_spellInfluence[41] || m_spellInfluence[42]) {
        long damageRange = m_monInfo.damageLowBound + m_monInfo.damageHighBound;
        double baseAverage = damageRange / 2.0;
        double averageDamage = getAverageDamage();
        attack = averageDamage / baseAverage * attack;
    }
    if (ranged && is(creatureTwoAttacks))
        attack = attack + attack;
    double result = sqrt(attack * defense)
                    * akCreatureTypeTraits[m_creatureType].baseFightValue;
    if (is(creatureSiegeWeapon | creatureSummoned)) {
        long total = getTotalHitPoints(0);
        long sum = 0;
        army* stack = g_combatManager->m_armies[group];
        for (long i = 0; i < g_combatManager->m_numArmies[group];
             i++, stack++) {
            if (!stack->is(creatureSiegeWeapon | creatureImmobilized | creatureSummoned)) {
                sum += stack->getTotalHitPoints(0);
            }
        }
        if (total == 0)
            result = 0.1;
        else
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
VA(0x00442e60, 0x169)
DC_ADDRESS(0x048168, 0x186)
MAC_ADDRESS(0x04ecf8, 0x12c)
long army::getTotalCombatValue(long lowestAttack, long lowestDefense) const
{
    if (m_numTroops <= 0)
        return 0;
    bool ranged = canShoot(0);
    double value = getUnitCombatValue(lowestAttack, lowestDefense,
                                         ranged, 0);
    if (is(creatureClone))
        return static_cast<long>(m_numTroops * value / 5.0);
    return static_cast<long>(getTotalHitPoints(0)
                             * value / m_monInfo.hitPoints);
}

VA(0x00442fd0, 0xA9)
DC_ADDRESS(0x0482f0, 0x164)
MAC_ADDRESS(0x04ee24, 0x114)
// Original DC public ?get_loss_combat_value@army@@QBAJJJ_NJ0@Z proves
// both flags bool, with 0 backreferencing _N.
long army::getLossCombatValue(long lowestAttack, long lowestDefense,
                                 bool ranged, long damage,
                                 bool killsOnly) const
{
    double value = getUnitCombatValue(lowestAttack, lowestDefense,
                                         ranged, 0);
    if (is(creatureClone))
        return static_cast<long>(m_numTroops * value / 5.0);
    if (killsOnly)
        value = 1000.0;
    long lost = 0;
    if (damage % m_monInfo.hitPoints + m_topCreatureDamage >= m_monInfo.hitPoints)
        lost = m_topCreatureDamage;
    return static_cast<long>((lost + damage) * value / m_monInfo.hitPoints);
}

VA(0x00443080, 0x50)
DC_ADDRESS(0x048454, 0x4c)
MAC_ADDRESS(0x04ef38, 0x68)
// Original DC public ?get_total_hit_points@army@@QBAJ_N@Z.
long army::getTotalHitPoints(bool simulated) const
{
    long total;
    if (is(creatureClone))
        total = 1;
    else
        total = m_monInfo.hitPoints * m_numTroops - m_topCreatureDamage;
    if (simulated)
        total = max(total - AI_expected_damage, 0);
    return total;
}

VA(0x004430d0, 0x56)
DC_ADDRESS(0x0484a0, 0x2e)
MAC_ADDRESS(0x04efa0, 0x5c)
void army::setAIExpectedDamage(long arg)
{
    AI_expected_damage = min(arg, getTotalHitPoints(0));
}

VA(0x00443130, 0x25)
DC_ADDRESS(0x0484d0, 0x54)
MAC_ADDRESS(0x04effc, 0x30)
float army::getFireShieldStrength() const
{
    if (m_spellInfluence[29])
        return m_fireShieldStrength;
    if (m_creatureType == CREATURE_EFREET_SULTAN)
        return DATA_COMPGEN(0x0063b8b4, innateFireShieldStrength, 0.2f);
    return DATA_COMPGEN(0x0063ac64, zeroFireShieldStrength, 0.0f);
}

VA(0x00443160, 0x1BF)
DC_ADDRESS(0x048524, 0x168)
MAC_ADDRESS(0x04f02c, 0x204)
int army::computeBaseDamage(unsigned char simulateOnly) const
{
    int num;
    if (m_spellInfluence[61] > 0 && is(creatureShootingArmy))
        num = cppMax(m_numTroops / 2, 1);
    else
        num = m_numTroops;

    int low;
    int high;
    if (m_creatureType == ARMY_CREATURE_BALLISTA) {
        const hero* shooter = getController();
        low = (shooter->getPrimarySkill(0) + 1) * m_monInfo.damageLowBound;
        high = (shooter->getPrimarySkill(0) + 1) * m_monInfo.damageHighBound;
    } else {
        low = m_monInfo.damageLowBound;
        high = m_monInfo.damageHighBound;
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
                total += sRandom(low, high);
            damage = total * num / 10;
        } else {
            for (int i = 0; i < num; i++)
                total += sRandom(low, high);
            damage = total;
        }
    }
    return damage;
}

// Complete separates the numeric damage bonus from the combat message and sound
// so estimated-damage calculations can reuse it. Dreamcast combines these paths.
VA(0x00443320, 0x514)
MAC_ADDRESS(0x04f230, 0x530)
int army::computeAttackerBonus(int baseDamage, bool isShooting,
                                 army* defender, bool announce,
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
                if (!g_combatManager->isQuickCombat()) {
                    std::string text;
                    if (m_numTroops == 1)
                        text = formatString(
                            g_generalText->GetText(GENERAL_TEXT_HATRED_DAMAGE_ONE_FORMAT),
                            getName(),
                            defender->getName());
                    else
                        text = formatString(
                            g_generalText->GetText(GENERAL_TEXT_HATRED_DAMAGE_MANY_FORMAT),
                            getName(),
                            defender->getName());
                    g_combatManager->m_combatWindow->combatMessage(
                        text.c_str(), 1, 0);
                }
            }
        }
    }

    long total = bonus;
    hero* controller = getController();
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
                                                   m_monInfo.level,
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

VA(0x00443840, 0x344)
DC_ADDRESS(0x04868c, 0x582)
MAC_ADDRESS(0x04f760, 0x358)
// Original DC public ?ComputeAttackerDamageBonuses@army@@QBAHH_NPAV1@0J@Z
// proves both flags bool.
int army::computeAttackerDamageBonuses(int baseDamage,
                                       bool isShooting,
                                       army* defender,
                                       bool simulateOnly,
                                       long distance) const
{
    int result = computeAttackerBonus(baseDamage, isShooting, defender,
                                        simulateOnly == 0, distance);
    switch (m_creatureType) {
    case ARMY_CREATURE_BALLISTA: {
        hero* controlling = getController();
        long mastery =
            controlling->getSecondarySkill(eSecSkillBattlefieldBallistics);
        if (!simulateOnly
            && sRandom(1, 100) <= g_artilleryDoubleChances[mastery]) {
            result += baseDamage;
            if (!g_combatManager->isQuickCombat()) {
                std::string text;
                const char* creatureName;
                creatureName = getName();
                text = formatString((*g_generalText)[GENERAL_TEXT_DOUBLE_DAMAGE_ONE_FORMAT],
                                     creatureName);
                g_combatManager->m_combatWindow->combatMessage(
                    text.c_str(), 1, 0);
            }
        }
        break;
    }

    case CREATURE_DREAD_KNIGHT:
        if (!simulateOnly && sRandom(1, 100) <= 20) {
            result += baseDamage;
            if (!g_combatManager->isQuickCombat()) {
                std::string text;
                if (m_numTroops == 1)
                    text = formatString((*g_generalText)[GENERAL_TEXT_DOUBLE_DAMAGE_ONE_FORMAT],
                                         getName());
                else
                    text = formatString((*g_generalText)[GENERAL_TEXT_DOUBLE_DAMAGE_MANY_FORMAT],
                                         getName());
                g_combatManager->m_combatWindow->combatMessage(
                    text.c_str(), 1, 0);
                SAMPLE2 sample = loadPlaySample(
                    DATA_COMPGEN(0x00660a50, deathBlowSampleName,
                                 "Deathblo.wav"));
                g_combatManager->spellEffect(
                    eSpellEffectDeathBlow, defender, 100,
                    0);
                waitEndSample(sample, -1);
            }
        }
        break;
    }
    return result;
}

// E:\gamedcs\army.cpp:3230
DC_ADDRESS(0x048c10, 0x50)
MAC_ADDRESS(0x04fab8, 0x8)
inline int army::computeDefenderDamageBonuses(int baseDamage) const
{
    return 0;
}

VA(0x00443b90, 0x1FE)
DC_ADDRESS(0x048c60, 0x364)
MAC_ADDRESS(0x04fac0, 0x244)
// Original DC public ?ComputeAttackerDamageReduction@army@@QBANPBV1@_N@Z
// proves the shooting parameter bool independently of its lowered debug type.
double army::computeAttackerDamageReduction(const army* defender,
                                            bool isShooting) const
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
            && defender->is(creatureImmuneToMindSpells))
        reduction *= 0.5;
    if (m_creatureType == ARMY_CREATURE_MAGIC_ELEMENTAL
            && (defender->m_creatureType == ARMY_CREATURE_MAGIC_ELEMENTAL
                || defender->m_creatureType == ARMY_CREATURE_BLACK_DRAGON))
        reduction *= 0.5;
    if (isShooting) {
        int hex = m_gridIndex;
        if (is(creatureDoubleWide))
            hex = getSecondGridIndex();
        if (g_combatManager->shotIsThroughWall(this, hex, defender->m_gridIndex))
            reduction *= 0.5;
        if (g_combatManager->shotIsNotOptimal(this, defender))
            reduction *= 0.5;
    }
    if (is(creatureShootingArmy) && !isShooting && !is(creatureNoMeleePenalty))
        reduction *= 0.5;
    if (m_residualBlindness && m_residualParalyze) {
        double penalty = min(
            m_blindFactor, akSpellTraits[SPELL_BLIND].m_mastery_bonus[2] / 100.0);
        reduction = penalty * reduction;
    } else if (m_residualBlindness)
        reduction = m_blindFactor * reduction;
    else if (m_residualParalyze)
        reduction = akSpellTraits[SPELL_BLIND].m_mastery_bonus[2] / 100.0
                    * reduction;
    return reduction;
}

VA(0x00443d90, 0x9C)
DC_ADDRESS(0x048fc4, 0x12e)
MAC_ADDRESS(0x04fd04, 0x88)
// Original DC public ?ComputeDefenderDamageReduction@army@@QBAN_N@Z.
double army::computeDefenderDamageReduction(bool isShooting) const
{
    double reduction = getDefenseDamageModifier(isShooting);
    if (isShooting) {
        if (m_spellInfluence[28])
            reduction *= m_airShieldFactor;
    } else {
        if (m_spellInfluence[27])
            reduction *= m_shieldFactor;
    }
    if (m_spellInfluence[70])
        reduction *= 0.5;
    hero* castingHero = getController();
    if (castingHero)
        reduction *= castingHero->getDefenseFactor();
    return reduction;
}

// Complete-only damage estimate used by the two ai_tactical skill-value
// helpers. Its original name is unknown; the DC DamageEnemy interface and
// helper sequence describe a different operation.
VA(0x00443e30, 0x101)
MAC_ADDRESS(0x04fd8c, 0xf4)  // anchor-callee (ai_tactical's two skill-value; MAC_ABSTRACTION_FROM(tokens1:c55baa363583,55.5556): DC-backed bool ranged interface and accumulated damage local; masked shape rises from 40 to 48 of 61 retail instructions.
                       // functions) + arity ret 0x10, retail-only slot
long army::getEstimatedDamage(const army* target, long amount,
                                bool ranged, long distance) const
{
    if (!target)
        return 0;
    long damage = amount + computeAttackerBonus(
        amount, ranged, const_cast<army*>(target), 0, distance);
    damage += target->computeDefenderDamageBonuses(amount);
    damage = static_cast<long>(
        computeAttackerDamageReduction(target, ranged) * damage);
    damage = static_cast<long>(
        damage * target->computeDefenderDamageReduction(ranged));
    if (damage <= 0)
        damage = 1;
    return damage;
}

VA(0x00443f40, 0x14F)
DC_ADDRESS(0x0490f4, 0x16c)
MAC_ADDRESS(0x04fe80, 0x134)
// Original DC public ?adjust_damage@army@@QBAJPAV1@J_N1JPAJ@Z:
// both flags bool (1 backreferences the earlier _N parameter).
long army::adjustDamage(army* enemy, long baseDamage, bool isShot,
                         bool simulated, long distance,
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
// Original DC public ?DamageEnemy@army@@QAAJPAV1@PAH1_N@Z proves bool.
DC_ADDRESS(0x049260, 0x68)
MAC_ADDRESS(0x04ffb4, 0x98)
inline long army::damageEnemy(army* enemy, int* damageOut, int* killed,
                              bool isShot)
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

VA(0x00444090, 0x8F)
DC_ADDRESS(0x0492c8, 0x92)
MAC_ADDRESS(0x05004c, 0xac)
int army::damage(int damage)
{
    int total = damage + m_topCreatureDamage;
    int killed = total / m_monInfo.hitPoints;
    m_topCreatureDamage = total % m_monInfo.hitPoints;
    if (is(creatureClone)) {
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
    cancelSpellType(ARMY_CANCEL_SPELLS_AFTER_DAMAGE);
    return killed;
}

// Original: army::Strength; army.cpp:3481
// DC3482 multiplies the current troop count by baseFightValue (traits+46);
// Complete widens that traits field to int at +64.
DC_ADDRESS(0x04935c, 0x44)
unsigned long army::strength()
{
    return m_numTroops * akCreatureTypeTraits[m_creatureType].baseFightValue;
}

// E:\gamedcs\army.cpp:3492

// pCell2 and iGI2 are function-scope locals assigned only in the two-hex
// arm; the one-hex path reads them back uninitialised, as retail does - do
// not initialise them.
// `fadeElementals` is never read: a dead parameter, transcribed faithfully.
VA(0x00444120, 0x3A6)
DC_ADDRESS(0x0493a0, 0x2f4)
MAC_ADDRESS(0x0500f8, 0x338)
void army::processDeath(int fadeElementals)
{
    if (is(creatureImmobilized))
        return;

    removeAura();
    removeBinding();

    if (!m_spellInfluence[60]) {
        if (random(1, 100) < 60)
            g_combatManager->m_playDoh[getOwningSide()] = 1;
        else if (random(1, 100) < 80)
            g_combatManager->m_playYeah[1 - getOwningSide()] = 1;
    }

    // DC 0x49420 calls the canonical member (army.cpp:3512).
    cancelAllSpells();

    m_monInfo.attributes |= creatureImmobilized;
    m_allUnitsKilled = 0;

    if (g_combatManager->validHex(m_gridIndex)) {
        hexcell* cell = &g_combatManager->m_cells[m_gridIndex];
        unsigned char twoHex = is(creatureDoubleWide);
        hexcell* cell2;
        int gi2;
        if (twoHex) {
            gi2 = m_gridIndex + offsetToFront(-1);
            cell2 = &g_combatManager->m_cells[gi2];
        }
        if (leavesNoBody()) {
            g_combatManager->bCreatureVanish[getOwningSide()][index] = 1;
            g_combatManager->bSomeCreaturesVanish = 1;
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
                if (is(creatureDoubleWide)) {
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
            if (is(creatureDoubleWide)) {
                cell2->m_armySide = -1;
                cell2->m_armySlot = -1;
            }
        }
        if (m_mirrorSourceIndex != -1) {
            army* mirror =
                &g_combatManager->m_armies[getOwningSide()][m_mirrorSourceIndex];
            if (mirror->m_mirrorDestIndex == index)
                mirror->m_mirrorDestIndex = -1;
        }
        if (m_mirrorDestIndex != -1) {
            army* clone =
                &g_combatManager->m_armies[getOwningSide()][m_mirrorDestIndex];
            clone->m_numTroops = 0;
            clone->processDeath(fadeElementals);
        }
    }
}

VA(0x004444d0, 0x3B)
DC_ADDRESS(0x049694, 0x48)
MAC_ADDRESS(0x050430, 0x7c)  // decorated identity + CancelIndividualSpell edges
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

// Original: army::adjust_hitpoints; army.cpp:3660
// Inlined in ResetRound and the AGE/POISON spell paths.
DC_ADDRESS(0x0496dc, 0x6a)
MAC_ADDRESS(0x0504ac, 0xd4)
// Mac 0x504ac reads AGE at +0x2c4, as a direct row read and an inline
// getSpellTime(SPELL_AGE) both would; DC adjust_hitpoints makes no
// get_spell_time call (see army.h getSpellTime). Retail agrees: expanded in
// ResetRound, the AGE load is scheduled above the poison-factor store
// (+0x4a4). VC6 hoists it across that store only for the constant-offset
// field read; the inline getter's substituted index keeps the load behind
// the store.
void army::adjustHitpoints()
{
    if (m_spellInfluence[SPELL_AGE])
        m_monInfo.hitPoints = static_cast<int>(
            m_origHitPoints * m_poisonPenalty * 0.5f + 0.95f);
    else
        m_monInfo.hitPoints = static_cast<int>(
            m_origHitPoints * m_poisonPenalty + 0.95f);
    m_topCreatureDamage = min(m_topCreatureDamage, m_monInfo.hitPoints - 1);
}

// Take one standing spell off this stack: clear its round row, undo
// whatever the spell had folded into the stack's own words, and pull
// the spell's entry back out of the cast-order queue. Disrupting Ray
// is the one spell that never comes off.

// The switch arms are in Dreamcast source order: BIND's binder teardown,
// HYPNOTIZE's remove_aura/add_aura helper pair, then the seven stat
// restores.

// The AGE arm re-reads spellInfluence[SPELL_AGE] AFTER the entry code
// zeroed it, so its 0.5f halving arm is dead at runtime - the
// uninitialized-`level` class of retail quirk: transcribe, do not fix.
// Its rounding constant is +0.95f (read from the image at 0x63b8c8),
// not the usual +0.5f.

// The tail pulls the spell out of SpellInfluenceQueue with std::find and the
// single-iterator erase (DC 3790..3792 names the iterator si).
VA(0x00444510, 0x3DB)
DC_ADDRESS(0x049748, 0x262)
MAC_ADDRESS(0x050580, 0x3b4)  // anchor-global
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
        m_monInfo.defenseSkill -= m_toughskinBonus;
        break;
    case SPELL_WEAKNESS:
        m_monInfo.attackSkill += m_weaknessPenalty;
        break;
    case SPELL_PRAYER:
        m_monInfo.attackSkill -= m_prayerBonus;
        m_monInfo.defenseSkill -= m_prayerBonus;
        if (!is(creatureSiegeWeapon))
            m_monInfo.speed -= m_prayerBonus;
        break;
    case SPELL_HASTE:
        if (!is(creatureSiegeWeapon)) {
            m_monInfo.speed -= m_tailwindBonus;
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
        m_monInfo.attackSkill += m_diseaseAttackPenalty;
        m_monInfo.defenseSkill += m_diseaseDefensePenalty;
        break;
    }
    TSpellQueue::iterator si = std::find(m_spellInfluenceQueue.begin(),
                                        m_spellInfluenceQueue.end(), spell);
    if (si != m_spellInfluenceQueue.end())
        m_spellInfluenceQueue.erase(si);
}

// Cancel spells with positive durations. Complete has 81 spell entries;
// Dreamcast has 80.
// E:\gamedcs\army.cpp:3802
DC_ADDRESS(0x0499ac, 0x3a)
MAC_ADDRESS(0x050c24, 0x68)
// Mac 0x50c48 reads row i; DC makes no get_spell_time call (see army.h).
void army::cancelAllSpells()
{
    for (int i = 0; i < kNumSpellsAndCreatureEffects; i++) {
        if (m_spellInfluence[i] > 0)
            cancelIndividualSpell(i);
    }
}


// Put one spell ON this stack: pick how many rounds it stands (255 for
// the three that never time out on their own, "until your next turn"
// for Frenzy, the caster's power for everything else), raise-or-return
// if the spell is already standing, otherwise record it, fold its
// per-mastery traits amount into the stack's own words, and append it
// to the cast-order queue. The exact inverse of CancelIndividualSpell
// above, arm for arm.

// Poison's double max result is named before its float conversion.
VA(0x004448f0, 0xB99)
DC_ADDRESS(0x0499e8, 0x900)
MAC_ADDRESS(0x050c8c, 0x7a8)  // anchor-global
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
    long amount = akSpellTraits[spell].m_mastery_bonus[mastery];
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
        for (int j = 0; j < kNumSpellsAndCreatureEffects; j++) {
            if (akSpellTraits[j].m_level < m_antiMagicSpellLevel
                && akSpellTraits[j].m_karma < 0
                && !(akSpellTraits[j].m_flags & 8))
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
                spell, m_monInfo.level, amount);
        break;
    case SPELL_PRECISION:
        m_precisionAmount = amount;
        if (castingHero)
            m_precisionAmount += castingHero->getHeroSpellBonus(
                spell, m_monInfo.level, amount);
        break;
    case SPELL_WEAKNESS:
        m_weaknessPenalty = amount;
        if (castingHero)
            m_weaknessPenalty += castingHero->getHeroSpellBonus(
                spell, m_monInfo.level, amount);
        if (m_monInfo.attackSkill < m_weaknessPenalty)
            m_weaknessPenalty = m_monInfo.attackSkill;
        m_monInfo.attackSkill = m_monInfo.attackSkill - m_weaknessPenalty;
        break;
    case SPELL_STONE_SKIN:
        m_toughskinBonus = amount;
        if (castingHero)
            m_toughskinBonus += castingHero->getHeroSpellBonus(
                spell, m_monInfo.level, amount);
        m_monInfo.defenseSkill = m_monInfo.defenseSkill + m_toughskinBonus;
        break;
    case SPELL_PRAYER:
        m_prayerBonus = amount;
        if (castingHero)
            m_prayerBonus += castingHero->getHeroSpellBonus(
                spell, m_monInfo.level, amount);
        m_monInfo.attackSkill = m_monInfo.attackSkill + m_prayerBonus;
        m_monInfo.defenseSkill = m_monInfo.defenseSkill + m_prayerBonus;
        if (!is(creatureSiegeWeapon))
            m_monInfo.speed = m_monInfo.speed + m_prayerBonus;
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
                spell, m_monInfo.level, amount);
        break;
    case SPELL_MISFORTUNE:
        m_luckPenalty = amount;
        break;
    case SPELL_HASTE:
        if (!is(creatureSiegeWeapon)) {
            cancelIndividualSpell(SPELL_SLOW);
            m_tailwindBonus = amount;
            if (castingHero)
                m_tailwindBonus += castingHero->getHeroSpellBonus(
                    spell, m_monInfo.level, amount);
            m_monInfo.speed = m_monInfo.speed + m_tailwindBonus;
            m_monFrameInfo.m_walkCycleTime =
                static_cast<long>(m_origWalkCycleTime * 0.65);
        }
        break;
    case SPELL_SLOW:
        if (!is(creatureSiegeWeapon)) {
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
        m_diseaseDefensePenalty = min(2, m_monInfo.defenseSkill);
        m_diseaseAttackPenalty = min(2, m_monInfo.attackSkill);
        m_monInfo.attackSkill = m_monInfo.attackSkill - m_diseaseAttackPenalty;
        m_monInfo.defenseSkill = m_monInfo.defenseSkill - m_diseaseDefensePenalty;
        break;
    case SPELL_POISON: {
        double poisonValue = cppMax(static_cast<double>(m_poisonPenalty - 0.1f), 0.5);
        m_poisonPenalty = static_cast<float>(poisonValue);
        adjustHitpoints();
        break;
    }
    case SPELL_AGE:
        adjustHitpoints();
        m_topCreatureDamage = min(m_topCreatureDamage, m_monInfo.hitPoints - 1);
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

// Original: army::DecrementSpellRounds; army.cpp:4129
// Inlined in ResetRound. Complete has 81 spell entries; the older DC loop
// at line 4133 ends at 80.
DC_ADDRESS(0x04a2e8, 0x5e)
MAC_ADDRESS(0x051434, 0x98)
// Mac 0x51458 reads the row once for both tests; DC DecrementSpellRounds
// makes no get_spell_time call (see army.h getSpellTime).
void army::decrementSpellRounds()
{
    for (int spell = 0; spell < kNumSpellsAndCreatureEffects; spell++) {
        if (m_spellInfluence[spell] > 0 && spell != SPELL_FRENZY) {
            if (m_spellInfluence[spell] == 1)
                cancelIndividualSpell(spell);
            else
                m_spellInfluence[spell]--;
        }
    }

    if (m_roundsLeftBeforeVanish > 0)
        m_roundsLeftBeforeVanish--;
}

// The legal victims of a berserked stack: every living stack on either
// side except this one and the arrow towers, kept only while it ties or
// beats the closest distance seen so far.
// Original: army::get_berserk_targets; army.cpp:4158. Dreamcast names the
// two retained locals `closest` and `target`.
VA(0x00445490, 0x23B)
DC_ADDRESS(0x04a348, 0x138)
MAC_ADDRESS(0x0514cc, 0x1d0)  // anchor-global
void army::getBerserkTargets(std::vector<army*>& armies) const
{
    long closest = 0;
    unsigned char canShootTarget;
    army* target;
    if (canShoot(0)) {
        canShootTarget = 1;
    } else {
        canShootTarget = 0;
        g_searchArray->seedCombatPosition(this, -1, 127, 0, -1);
    }
    for (int side = 0; side < 2; side++) {
        target = g_combatManager->m_armies[side];
        long count = g_combatManager->m_numArmies[side];
        for (; count-- > 0; target++) {
            if (target->is(creatureImmobilized))
                continue;
            if (target == this)
                continue;
            if (target->m_creatureType == ARMY_CREATURE_ARROW_TOWER)
                continue;
            long value;
            if (canShootTarget) {
                value = combatManager::getDistance(m_gridIndex, target->m_gridIndex);
            } else {
                if (!g_combatManager->m_cells[target->m_gridIndex].m_validMove)
                    continue;
                value = g_searchArray->getHex(target->m_gridIndex)->m_cost;
            }
            if (armies.size() > 0 && value > closest)
                continue;
            if (value < closest)
                armies.clear();
            armies.push_back(target);
            closest = value;
        }
    }
}

// A berserked stack picks one of its legal victims at random and does
// whatever it can to it - shoot if it may shoot, swing otherwise - and
// if the list came back empty it simply defends.
VA(0x004456d0, 0x164)
DC_ADDRESS(0x04a480, 0x118)
MAC_ADDRESS(0x05169c, 0x118)
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

VA(0x00445840, 0x66)
DC_ADDRESS(0x04a598, 0x78)
MAC_ADDRESS(0x0517b4, 0xac)
long army::getAttackDirection(long ourHex, const army* enemy,
                                long enemyHex) const
{
    long secondHex = enemyHex;
    if (enemy->is(creatureDoubleWide))
        secondHex = enemyHex + enemy->offsetToFront(-1);
    for (long direction = 0; direction < COMBAT_DIRECTION_WIDE_LOWER + 1; direction++) {
        if (direction < COMBAT_DIRECTION_COUNT || is(creatureDoubleWide)) {
            long hex = getAdjacentHex(ourHex, direction);
            if (hex == enemyHex || hex == secondHex)
                return direction;
        }
    }
    return -1;
}

VA(0x004458b0, 0x9D)
DC_ADDRESS(0x04a610, 0x94)
MAC_ADDRESS(0x051860, 0x108)
inline long army::getAttackDirection(long ourHex, const army* enemy) const
{
    long best = -1;
    long direction = 0;
    for (;;) {
        if (direction < COMBAT_DIRECTION_COUNT || is(creatureDoubleWide)) {
            long hex = getAdjacentHex(ourHex, direction);
            if (combatManager::validHex(hex)
                && enemy == g_combatManager->m_cells[hex].getArmy()) {
                if (!needToTurn(direction))
                    return direction;
                if (best == -1)
                    best = direction;
            }
        }
        direction++;
        if (direction >= COMBAT_DIRECTION_WIDE_LOWER + 1)
            return best;
    }
}

// One stack, one hex, whichever way it travels: a flyer flies, a devil
// teleports, everything else walks. The grid identity is INVALIDATED
// first, then the destination is bounds-checked
// and CanFit'ed, and only then does the manager clear its
// last-moved-stack slot, drop the highlighter and mark the mover's own
// hexes.
VA(0x00445950, 0x107)
DC_ADDRESS(0x04a6a4, 0x108)
MAC_ADDRESS(0x051968, 0x16c)
bool army::simpleMove(int hex, bool restoreFacing)
{
    groupToAttack = -1;
    indexToAttack = -1;
    if (!g_combatManager->validHex(hex))
        return 0;
    if (!canFit(hex, 0, 0))
        return 0;
    g_combatManager->m_lastMovedArmy = 0;
    g_combatManager->turnOffHighlighter(1);
    g_combatManager->beginObstacleTraversal(this);
    bool moved;
    if (is(creatureFlyingArmy)) {
        targetCellIndex = hex;
        moved = validFlight(hex, 0);
        if (moved) {
            flyTo(targetCellIndex, restoreFacing);
            moved = 1;
        }
    } else if (m_creatureType == CREATURE_DEVIL
               || m_creatureType == CREATURE_ARCH_DEVIL) {
        targetCellIndex = hex;
        moved = validFlight(hex, 0);
        if (moved) {
            teleportTo(targetCellIndex, restoreFacing);
            moved = 1;
        }
    } else {
        moved = walkTo(hex, restoreFacing);
    }
    g_combatManager->m_lastMovedArmy = this;
    return moved;
}

// simple_move's sibling: one stack attacking one hex. The grid identity
// is invalidated first, the target is
// resolved and rejected if it is missing or is this stack itself, and
// then the stack either shoots or walks its facing around to a
// direction that reaches the target.
VA(0x00445a60, 0x26D)
DC_ADDRESS(0x04a7ac, 0x10a)
MAC_ADDRESS(0x051ad4, 0x1b4)
bool army::attackHex(int hex, bool restoreFacing)
{
    groupToAttack = -1;
    indexToAttack = -1;
    if (!g_combatManager->validHex(hex))
        return 0;
    hexcell* cell = &g_combatManager->m_cells[hex];
    army* target = cell->getArmy();
    if (!target || target == this)
        return 0;
    g_combatManager->m_lastMovedArmy = 0;
    g_combatManager->turnOffHighlighter(1);
    groupToAttack = cell->m_armySide;
    indexToAttack = cell->m_armySlot;
    targetCellIndex = hex;
    if (canShoot(0)) {
        rangeAttack();
    } else {
        int direction = getAttackDirection(target);
        if (direction >= 0) {
            bool turned;
            if (needToTurn(direction)) {
                setupAnimation();
                turn(1);
                turned = restoreFacing;
            } else {
                turned = 0;
            }
            doAttack(direction);
            if (!is(creatureImmobilized) && turned) {
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
bool army::moveTo(int hex, bool restore_facing)
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
// STATIC: no retail out-of-line copy; inlined at its one call site,
// attack_wall.
DC_ADDRESS(0x04a8c8, 0xb2)
MAC_ADDRESS(0x051ca8, 0xac)
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
        if (distance == best && sRandom(1, 100) > 50)
            continue;
        best = distance;
        chosen = targets[i];
    }
    return chosen;
}

VA(0x00445d10, 0x14)
DC_ADDRESS(0x04a8b8, 0x10)
MAC_ADDRESS(0x051c88, 0x20)
bool army::moveTo(int hex, bool restoreFacing)
{
    return simpleMove(hex, restoreFacing);
}

// One catapult (or cyclops) bombardment: aim once at the segment the
// grid index falls on, then fire the ballistics row's `shots` at it,
// re-aiming at the nearest other STANDING segment whenever the one
// being fired at comes down.
VA(0x00445d30, 0x189)
DC_ADDRESS(0x04a97c, 0xc0)
MAC_ADDRESS(0x051d54, 0x13c)
void army::attackWall(int targetGridIndex)
{
    g_combatManager->m_lastMovedArmy = 0;
    g_combatManager->turnOffHighlighter(1);
    TWallTargetId wall;
    {
        // GetTargetWallIndex returns int in Complete and an enum in Dreamcast.
        wall = TWallTargetId(combatManager::getTargetWallIndex(targetGridIndex));
    }
    hero* controller = getController();
    long level;
    switch (m_creatureType) {
    case CREATURE_CATAPULT:
        level = controller->getSecondarySkill(eSecSkillSiegeBallistics);
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
VA(0x00445ec0, 0x10C)
DC_ADDRESS(0x04aa3c, 0x90)
MAC_ADDRESS(0x051e90, 0x114)
void army::attackWall(TWallTargetId wall,
                       const type_ballistics_traits& ballistics)
{
    long levels;
    int roll = sRandom(1, 100);
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
    if (sRandom(1, 100) > chance) {
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

// DC names destX/destY/numFrames as const int and startY as int;
// sample2b, shoot_sample and hit_sample name the resource/sample lifetimes.
// DC army.cpp:4731 guards the sample; 4732 calls ResourceManager::Dispose.
VA(0x00445fd0, 0x526)
DC_ADDRESS(0x04aacc, 0x5a4)
MAC_ADDRESS(0x051fa4, 0x64c)  // anchor-callee
void army::attackWall(TWallTargetId wall, long levelsDestroyed)
{
    if (g_combatManager->isQuickCombat()) {
        g_combatManager->damageWall(wall, levelsDestroyed);
        return;
    }

    const int destX = combatManager::s_wallTargets[wall].m_hitX;
    const int destY = combatManager::s_wallTargets[wall].m_hitY;

    long startX;
    if (m_facing == 1)
        startX = g_combatManager->m_cells[m_gridIndex].m_refX
                 + m_monFrameInfo.m_missileOffset[2];
    else
        startX = g_combatManager->m_cells[m_gridIndex].m_refX
                 - m_monFrameInfo.m_missileOffset[2];
    int startY = g_combatManager->m_cells[m_gridIndex].m_refY
                 + m_monFrameInfo.m_missileOffset[3];

    long dx = destX - startX;
    if (dx < 0)
        dx = -dx;
    long dy = destY - startY;
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

    sample* sample2b = ResourceManager::GetSample(
        levelsDestroyed == 0 ? DATA_COMPGEN(0x00660a84, wallMissSampleName,
                                            "WallMiss.82m")
                             : DATA_COMPGEN(0x00660a78, wallHitSampleName,
                                            "WallHit.82m"));
    g_combatManager->resetLimitCreature();
    g_combatManager->markCreatureEffect(group, index);
    g_combatManager->computeMaxExtent();
    ds_memsample* shootSample =
        g_soundManager->memorySample(m_armySample[SHOOT_SAMPLE]);

    const int numFrames = m_monFrameInfo.m_attackFrames <= 0
                           ? m_stdIcon->GetNumFrames(m_currFrameType)
                           : m_monFrameInfo.m_attackFrames;
    long delay = m_monFrameInfo.m_attackStartCycleTime / numFrames;
    for (m_currFrameIndex = 0; m_currFrameIndex < numFrames; m_currFrameIndex++) {
        g_combatManager->drawFrame(1, 1, 0, delay, 1, 1);
    }

    m_currFrameType = cs_wait;
    m_currFrameIndex = 0;
    g_combatManager->shootBallisticMissile(startX, startY, destX, destY,
                                           m_missileIcon);
    ds_memsample* hitSample = g_soundManager->memorySample(sample2b);

    CSprite* explosion = ResourceManager::GetSprite(
        levelsDestroyed == 0
            ? DATA_COMPGEN(0x00660a6c, rockSpriteName, "CSGRCK.DEF")
            : DATA_COMPGEN(0x00660a60, explosionSpriteName, "SGEXPL.DEF"));
    g_combatManager->m_extent = SLimitData(
        destX - explosion->GetWidth() / 2,
        destY - explosion->GetHeight() / 2,
        destX - explosion->GetWidth() / 2 + explosion->GetWidth() - 1,
        destY - explosion->GetHeight() / 2 + explosion->GetHeight() - 1);
    g_combatManager->m_extent.clip(combatManager::s_combatAreaLimits);

    for (long frame = 0; frame < explosion->GetNumFrames(0); frame++) {
        if (frame == combatManager::WALL_EXPLOSION_HIT_FRAME
            && levelsDestroyed != 0)
            g_combatManager->damageWall(wall, levelsDestroyed);
        g_combatManager->drawFrame(0, 0, 1, 100, 0, 1);
        explosion->Draw(0, frame, 0, 0,
                        g_combatManager->m_extent.width(),
                        g_combatManager->m_extent.height(),
                        g_windowManager->m_screenBitmap,
                        destX - explosion->GetWidth() / 2,
                        destY - explosion->GetHeight() / 2, 0, 1);
        // DC army.cpp:4719/4720: the fixed-viewport scroll and area update.
        if (!g_combatManager->scrollTo(g_combatManager->m_extent,
                                       true, true, true))
            g_combatManager->updateCombatArea(
                g_combatManager->m_extent);
    }
    ResourceManager::Dispose(explosion);
    g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
    g_soundManager->waitSample(shootSample, -1);
    g_soundManager->waitSample(hitSample, -1);
    if (sample2b)
        ResourceManager::Dispose(sample2b);
    cancelSpellType(ARMY_CANCEL_SPELLS_AFTER_ATTACK);
}

VA(0x00446500, 0x126)
DC_ADDRESS(0x04b070, 0xda)
MAC_ADDRESS(0x0525f0, 0x164)
void army::cure(int level, int spellPower, const hero* castingHero)
{
    m_poisonPenalty = 1.0f;
    adjustHitpoints();
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
    int healed = akSpellTraits[SPELL_CURE].m_mastery_bonus[level]
                 + akSpellTraits[SPELL_CURE].m_power_factor * spellPower;
    if (castingHero)
        healed += castingHero->getHeroSpellBonus(SPELL_CURE, m_monInfo.level,
                                                  healed);
    m_topCreatureDamage -= healed;
    if (m_topCreatureDamage < 0)
        m_topCreatureDamage = 0;
}

VA(0x00446630, 0x2E)
DC_ADDRESS(0x04b14c, 0x24)
MAC_ADDRESS(0x052754, 0x2c)
int army::midY() const
{
    return g_combatManager->m_cells[m_gridIndex].m_refY - m_imageHeight / 2;
}

// Original: army::TopY; army.cpp:4779
DC_ADDRESS(0x04b170, 0x20)
MAC_ADDRESS(0x052780, 0x24)
int army::topY() const
{
    return g_combatManager->m_cells[m_gridIndex].m_refY - m_imageHeight;
}

// Original: army::BottomY; army.cpp:4784
DC_ADDRESS(0x04b190, 0x16)
MAC_ADDRESS(0x0527a4, 0x1c)
int army::bottomY() const
{
    return g_combatManager->m_cells[m_gridIndex].m_refY;
}

VA(0x00446660, 0x35)
DC_ADDRESS(0x04b1a8, 0x64)
MAC_ADDRESS(0x0527c0, 0x48)
int army::midX() const
{
    int x = g_combatManager->m_cells[m_gridIndex].m_refX;
    if (is(creatureDoubleWide))
        x += m_facing ? 22 : -22;
    return x;
}

// Original: army::RightX; army.cpp:4800
DC_ADDRESS(0x04b20c, 0x6c)
int army::rightX() const
{
    int offset;
    if (m_facing == 0)
        offset = 196 - m_stdIcon->GetCroppedX(cs_wait, 0);
    else
        offset = m_stdIcon->GetCroppedX(cs_wait, 0)
                 + m_stdIcon->GetCroppedWidth(cs_wait, 0) - 196;
    return g_combatManager->m_cells[m_gridIndex].m_refX + offset;
}

// Original: army::LeftX; army.cpp:4812
DC_ADDRESS(0x04b278, 0x6c)
int army::leftX() const
{
    int offset;
    if (m_facing == 0)
        offset = 196 - m_stdIcon->GetCroppedX(cs_wait, 0)
                 - m_stdIcon->GetCroppedWidth(cs_wait, 0);
    else
        offset = m_stdIcon->GetCroppedX(cs_wait, 0) - 196;
    return g_combatManager->m_cells[m_gridIndex].m_refX + offset;
}

// Original: army::FrontX; army.cpp:4825
// Inlined in DrawToBuffer.
DC_ADDRESS(0x04b2e4, 0x6e)
MAC_ADDRESS(0x052808, 0x6c)
int army::frontX() const
{
    int offset = m_stdIcon->GetCroppedX(cs_wait, 0)
                 + m_stdIcon->GetCroppedWidth(cs_wait, 0) - 196;
    if (m_facing == 0)
        return g_combatManager->m_cells[m_gridIndex].m_refX - offset;
    else
        return g_combatManager->m_cells[m_gridIndex].m_refX + offset;
}

VA(0x004466a0, 0x1E)
DC_ADDRESS(0x04b354, 0x44)
MAC_ADDRESS(0x052874, 0x38)
int army::getSecondGridIndex() const
{
    if (!is(creatureDoubleWide))
        return m_gridIndex;
    return m_gridIndex + offsetToFront(-1);
}

// Original is_adjacent publics QBA_NH and QBA_NABV1 prove bool results.
VA(0x004466c0, 0x5A)
DC_ADDRESS(0x04b398, 0x4e)
MAC_ADDRESS(0x0528ac, 0x8c)
bool army::isAdjacent(int hex) const
{
    if (g_combatManager->isAdjacent(m_gridIndex, hex))
        return 1;
    if (is(creatureDoubleWide)) {
        int secondHex = getSecondGridIndex();
        return g_combatManager->isAdjacent(secondHex, hex);
    }
    return 0;
}

// Original: army::is_adjacent; army.cpp:4864
DC_ADDRESS(0x04b3e8, 0x3e)
bool army::isAdjacent(const army& otherArmy) const
{
    if (isAdjacent(otherArmy.m_gridIndex))
        return 1;
    if (otherArmy.is(creatureDoubleWide))
        return isAdjacent(otherArmy.getSecondGridIndex());
    return 0;
}

// Original: army::OtherArmyAdjacent; army.cpp:4881
DC_ADDRESS(0x04b428, 0x2a)
int army::otherArmyAdjacent(int group, int index)
{
    return isAdjacent(g_combatManager->m_armies[group][index]);
}

// Original Turn@@QAAX_N proves the animation flag is bool.
VA(0x00446720, 0x107)
DC_ADDRESS(0x04b454, 0x104)
MAC_ADDRESS(0x052938, 0x174)  // anchor-global
void army::turn(bool animateTurn)
{
    if (m_facing == FACING_ATTACKER) {
        if (animateTurn)
            playAnimation(9, -1, 0);
        m_facing = FACING_DEFENDER;
        if (is(creatureDoubleWide)) {
            m_gridIndex--;
            g_combatManager->m_cells[m_gridIndex].m_partOfDouble = 0;
            g_combatManager->m_cells[m_gridIndex + 1].m_partOfDouble = 1;
        }
        if (animateTurn)
            playAnimation(8, -1, 0);
    } else {
        if (animateTurn)
            playAnimation(7, -1, 0);
        m_facing = FACING_ATTACKER;
        if (is(creatureDoubleWide)) {
            m_gridIndex++;
            g_combatManager->m_cells[m_gridIndex].m_partOfDouble = 1;
            g_combatManager->m_cells[m_gridIndex - 1].m_partOfDouble = 0;
        }
        if (animateTurn)
            playAnimation(10, -1, 0);
    }
    if (animateTurn)
        playAnimation(2, 1, 0);
}

// Capture the battlefield without this stack on it, once, before an
// animation starts: widen the draw limits to the whole combat area,
// draw this stack's cell into the buffer, take one full frame with
// LetsPretendImNotHere raised, and blit the 800x600 result into the
// manager's backup bitmap. Quick combat skips the lot.
VA(0x00446830, 0x103)
DC_ADDRESS(0x04b558, 0xca)
MAC_ADDRESS(0x052aac, 0x15c)
void army::setupAnimation()
{
    if (g_combatManager->isQuickCombat())
        return;
    hexcell* cell = &g_combatManager->m_cells[m_gridIndex];
    g_combatManager->m_extent = heroWindowManager::s_nullLimits;
    g_combatManager->m_saveBiggestExtent = 1;
    g_combatManager->m_computeExtentOnly = 1;
    drawToBuffer(cell->m_refX, cell->m_refY, 0);
    g_combatManager->m_computeExtentOnly = 0;
    g_combatManager->m_saveBiggestExtent = 0;
    m_letsPretendImNotHere = 1;
    g_combatManager->drawFrame(0, 0, 0, 0, 1, 0);
    m_letsPretendImNotHere = 0;
    g_windowManager->m_screenBitmap->Draw(
        0, 0, 800, 600, g_combatManager->m_saveScreenPostGrid, 0, 0, false);
    g_combatManager->m_backgroundDrawn = 0;
}

// Run one animation sequence to the screen, frame by frame, pacing
// itself off the adventure timer. Every frame first repaints the clean
// background SetupAnimation captured over the extent the LAST frame
// dirtied, then redraws this stack into the buffer with the extent
// widened by half a hex either side, unions the two rectangles, takes a
// full frame and blits exactly that union. Quick combat skips the lot.
VA(0x00446940, 0x2FE)
DC_ADDRESS(0x04b624, 0x2a0)
MAC_ADDRESS(0x052c08, 0x420)
void army::playAnimation(int sequence, int nframes, int startFrame)
{
    if (g_combatManager->isQuickCombat())
        return;
    if (nframes < 0)
        nframes = m_stdIcon->GetNumFrames(sequence) - startFrame;
    m_currFrameType = sequence;
    int frameDelay;
    if (sequence == 0)
        frameDelay = static_cast<int>(
            static_cast<float>(m_monFrameInfo.m_walkCycleTime)
            * combatManager::s_combatSpeedMod[g_config.m_combatSpeed]
            / static_cast<float>(m_stdIcon->GetNumFrames(0)));
    else
        frameDelay = static_cast<int>(
            combatManager::s_combatSpeedMod[g_config.m_combatSpeed] * 100.0f);

    SLimitData bounds = g_combatManager->m_extent;
    for (m_currFrameIndex = startFrame;
         m_currFrameIndex < nframes + startFrame; m_currFrameIndex++) {
        SLimitData frame = bounds;
        g_combatManager->m_saveScreenPostGrid->Draw(
            frame.m_minX, frame.m_minY,
            frame.width(),
            frame.height(),
            g_windowManager->m_screenBitmap,
            frame.m_minX, frame.m_minY, false);

        g_combatManager->m_extent = heroWindowManager::s_nullLimits;
        g_combatManager->m_saveBiggestExtent = 1;
        g_combatManager->m_computeExtentOnly = 1;
        drawToBuffer(g_combatManager->m_cells[m_gridIndex].m_refX,
                     g_combatManager->m_cells[m_gridIndex].m_refY, 0);
        g_combatManager->m_computeExtentOnly = 0;
        g_combatManager->m_saveBiggestExtent = 0;

        g_combatManager->m_extent.m_minX -= 17;
        g_combatManager->m_extent.m_maxX += 17;
        bounds = g_combatManager->m_extent;
        frame.include(bounds);
        g_combatManager->m_extent = frame;

        g_combatManager->m_limitToExtent = 1;
        g_combatManager->drawFrame(0, 0, 0, 0, 0, 0);
        g_combatManager->m_limitToExtent = 0;
        GameTime::delayTil(g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT]);
        g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT]
            = GameTime::nextFrameTime(
                g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT], frameDelay);
        g_windowManager->updateScreen(
            frame.m_minX, frame.m_minY,
            frame.width(),
            frame.height());
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
VA(0x00446c40, 0x1E1)
DC_ADDRESS(0x04b8c4, 0x1c4)
MAC_ADDRESS(0x053028, 0x374)  // DC and Mac 0x5310c/0x5322c load the owning side directly.
int army::canFit(int destIndex, int allowShifting, int* newDestIndex) const
{
    if (newDestIndex)
        *newDestIndex = destIndex;

    if (!combatManager::validHex(destIndex))
        return 0;
    if (combatManager::inInvisibleColumn(destIndex))
        return 0;

    const hexcell* cell = &g_combatManager->m_cells[destIndex];
    if (g_combatManager->hexIsBlocked(destIndex))
        return 0;
    if (cell->hasArmy()) {
        if (cell->m_armySide != group)
            return 0;
        if (cell->m_armySlot != index)
            return 0;
    }

    if (!is(creatureDoubleWide))
        return 1;

    int otherIndex = getAdjacentCellIndex(destIndex, m_facing ? 1 : 4);
    if (!combatManager::validHex(otherIndex))
        return 0;
    if (combatManager::inInvisibleColumn(otherIndex))
        return 0;
    hexcell* otherCell = &g_combatManager->m_cells[otherIndex];
    if (!g_combatManager->hexIsBlocked(otherIndex)) {
        if (!otherCell->hasArmy()
                || (otherCell->m_armySide == group
                    && otherCell->m_armySlot == index))
            return 1;
    }

    if (allowShifting) {
        int shiftedIndex = getAdjacentCellIndex(destIndex, m_facing ? 4 : 1);
        if (!combatManager::validHex(shiftedIndex))
            return 0;
        if (combatManager::inInvisibleColumn(shiftedIndex))
            return 0;
        hexcell* shiftedCell = &g_combatManager->m_cells[shiftedIndex];
        if (g_combatManager->hexIsBlocked(shiftedIndex))
            return 0;
        if (shiftedCell->hasArmy()) {
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

VA(0x00446e30, 0x2E1)
DC_ADDRESS(0x04ba88, 0x1fc)
MAC_ADDRESS(0x05339c, 0x300)
void army::newTurn()
{
    if (m_resetThisRound != 0)
        return;
    m_resetThisRound = 1;
    if (is(creatureDefending)) {
        m_monInfo.defenseSkill -= m_defendBonus;
        m_monInfo.attributes &= ~creatureDefending;
    }
    if (g_combatManager->m_creaturePlacement != 0)
        return;
    if (m_topCreatureDamage > 0) {
        // Not getOwner(): DC NewTurn has no get_owner call.
        if (m_creatureType == CREATURE_WIGHT
            || m_creatureType == ARMY_CREATURE_WRAITH
            || m_creatureType == CREATURE_TROLL
            || ((akCreatureTypeTraits[m_creatureType].attributes
                 & g_ctaAlive)
                && g_combatManager->m_heroes[group] != 0
                && g_combatManager->m_heroes[group]
                       ->isWieldingArtifact(ARTIFACT_ELIXIR_OF_LIFE))) {
            long heal = m_topCreatureDamage;
            long amount = heal > 50 ? 50 : heal;
            m_topCreatureDamage = heal - amount;
            if (!g_combatManager->isQuickCombat()) {
                SAMPLE2 sample = loadPlaySample(
                    DATA_COMPGEN(0x00660a94, regenerSampleName,
                                 "Regener.wav"));
                std::string text;
                if (m_numTroops == 1)
                    text = formatString(
                        (*g_generalText)[GENERAL_TEXT_REGENERATION_ONE_FORMAT],
                        getName());
                else
                    text = formatString(
                        (*g_generalText)[GENERAL_TEXT_REGENERATION_MANY_FORMAT],
                        getName());
                g_combatManager->m_combatWindow->combatMessage(
                    text.c_str(), 1, 0);
                g_combatManager->spellEffect(
                    eSpellEffectRegeneration, this, 100,
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
VA(0x00447120, 0x20A)
DC_ADDRESS(0x04bc84, 0xfa)
MAC_ADDRESS(0x05369c, 0x15c)
void army::resetRound()
{
    if (m_numTroops <= 0)
        return;

    m_monInfo.attributes &= ~(creatureMorale | creatureWaiting | creatureDone);
    m_resetThisRound = 0;
    setRetaliationCount();

    if (g_combatManager->m_creaturePlacement)
        return;

    decrementSpellRounds();

    if (m_spellInfluence[SPELL_POISON] > 0) {
        int oldHitPoints = m_monInfo.hitPoints;
        double factor = cppMax<double>(m_poisonPenalty - 0.1f, 0.5);
        m_poisonPenalty = static_cast<float>(factor);
        adjustHitpoints();
        if (oldHitPoints - m_monInfo.hitPoints > 0) {
            m_showPowEffect = 1;
            m_someUnitsDamaged = 1;
            SAMPLE2 sample;
            if (!g_combatManager->isQuickCombat()) {
                sample = loadPlaySample(
                    DATA_COMPGEN(0x00660aa0, poisonWaveName, "Poison.wav"));
                g_combatManager->showSpellMessage(1, SPELL_POISON, this);
            }
            g_combatManager->powEffect(eSpellEffectPoison, 1);
            if (!g_combatManager->isQuickCombat())
                waitEndSample(sample, -1);
        }
    }

    if (m_roundsLeftBeforeVanish == 0)
        processDeath(1);
}

VA(0x00447330, 0x9C)
DC_ADDRESS(0x04bd80, 0xe4)
MAC_ADDRESS(0x0537f8, 0x100)
long army::getResurrectionSize(const army* target) const
{
    if (m_creatureType == CREATURE_ARCHANGEL) {
        int missing = target->m_origNumTroops - target->m_numTroops;
        int raised = m_numTroops * 100 / target->m_monInfo.hitPoints;
        return min(raised, missing);
    }
    int totalLife = target->m_monInfo.hitPoints * target->m_origNumTroops;
    int raised = min(totalLife, m_numTroops * 50)
        / akCreatureTypeTraits[ARMY_CREATURE_DEMON].hitPoints;
    return min(raised, target->m_origNumTroops);
}

VA(0x004473d0, 0x13D)
DC_ADDRESS(0x04be64, 0x88)
MAC_ADDRESS(0x0538f8, 0x144)
bool army::canCastResurrect(long hex) const
{
    if (!canCastResurrect())
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

// Uses DC's guarded-return getControllingSide (see army.h).
VA(0x00447510, 0x1A8)
MAC_ADDRESS(0x053a3c, 0x18c)
void army::chooseFaerieDragonSpell()
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

    if (g_combatManager->isQuickCombat())
        return;
    if (g_combatManager->IsHuman[getControllingSide()]) {
        const char* fmt =
            m_numTroops == 1
                ? DATA_COMPGEN(0x00660ad0, faerieReadiesFormat,
                               "The %s readies %s (press F to cast)")
                : DATA_COMPGEN(0x00660aac, faerieReadyFormat,
                               "The %s ready %s (press F to cast)");
        g_combatManager->m_combatWindow->combatMessage(
            formatString(fmt, getName(),
                          akSpellTraits[m_faerieDragonSpell].m_name)
                .c_str(),
            1, 0);
    }
}

// DC public ?can_cast_spell@army@@QBA_NJ@Z proves native bool.
VA(0x004476c0, 0x3BA)
DC_ADDRESS(0x04beec, 0x116)
MAC_ADDRESS(0x053bc8, 0x338)
bool army::canCastSpell(long hex) const
{
    if (m_monInfo.m_hasSpell == 0)
        return 0;
    if (!g_combatManager->canCastSpells(getControllingSide(), 0))
        return 0;
    if (!combatManager::validHex(hex))
        return 0;
    army* target = g_combatManager->m_cells[hex].getArmy();
    switch (m_creatureType) {
    case CREATURE_ARCHANGEL:
    case ARMY_CREATURE_PIT_LORD:
        return canCastResurrect(hex);
    case CREATURE_MASTER_GENIE:
        return target && getValidCaliphSpells(target) > 0;
    case CREATURE_FAERIE_DRAGON:
        return target != 0
            && g_combatManager->validSpellTargetArmy(
                m_faerieDragonSpell, getControllingSide(), target, 1, 1);
    case CREATURE_STORM_ELEMENTAL:
        return target != 0
            && g_combatManager->validSpellTargetArmy(
                SPELL_PROTECTION_FROM_AIR, getControllingSide(), target, 1, 1);
    case CREATURE_ICE_ELEMENTAL:
        return target != 0
            && g_combatManager->validSpellTargetArmy(
                SPELL_PROTECTION_FROM_WATER, getControllingSide(), target, 1, 1);
    case CREATURE_ENERGY_ELEMENTAL:
        return target != 0
            && g_combatManager->validSpellTargetArmy(
                SPELL_PROTECTION_FROM_FIRE, getControllingSide(), target, 1, 1);
    case CREATURE_MAGMA_ELEMENTAL:
        return target != 0
            && g_combatManager->validSpellTargetArmy(
                SPELL_PROTECTION_FROM_EARTH, getControllingSide(), target, 1, 1);
    case CREATURE_OGRE_MAGE:
        return target != 0
            && g_combatManager->validSpellTargetArmy(
                SPELL_BLOODLUST, getControllingSide(), target, 1, 1);
    }
    return 0;
}

// Original: army::cast_resurrect; army.cpp:5396
// Inlined in CastSpell, as is the demonic variant below.
DC_ADDRESS(0x04c004, 0x80)
MAC_ADDRESS(0x053f00, 0xc4)
void army::castResurrect(long hex)
{
    army* target = g_combatManager->findResurrectionTarget(
        getControllingSide(), hex, 1);
    if (!target)
        return;
    SAMPLE2 sound;
    if (!g_combatManager->isQuickCombat())
        sound = loadPlaySample(DATA_COMPGEN(
            0x00660af4, resurrectSampleName, "Resurect.wav"));
    g_combatManager->resurrect(target, m_numTroops * 100, 0);
    if (!g_combatManager->isQuickCombat())
        waitEndSample(sound, -1);
}

// Original: army::cast_demonic_resurrect; army.cpp:5419
DC_ADDRESS(0x04c084, 0x6c)
MAC_ADDRESS(0x053fc4, 0x70)
void army::castDemonicResurrect(long hex)
{
    army* target = g_combatManager->findDemonicResurrectionTarget(
        getControllingSide(), hex);
    if (!target)
        return;
    g_combatManager->demonicResurrection(this, target);
}

// Original: group_has_melee; army.cpp:5433
// DC5437/5455/5473 all count down with i-- > 0. Inlined in the
// spell-validity worker.
DC_ADDRESS(0x04c0f0, 0x64)
MAC_ADDRESS(0x054034, 0x134)
static bool groupHasMelee(long group)
{
    long i = g_combatManager->m_numArmies[group];
    while (i-- > 0) {
        army* enemy = &g_combatManager->m_armies[group][i];
        if (!enemy->cannotAttack() && !enemy->canShoot(0))
            return 1;
    }
    return 0;
}

// Original: group_has_shooters; army.cpp:5451
DC_ADDRESS(0x04c154, 0x64)
MAC_ADDRESS(0x054168, 0x134)
static bool groupHasShooters(long group)
{
    long i = g_combatManager->m_numArmies[group];
    while (i-- > 0) {
        army* enemy = &g_combatManager->m_armies[group][i];
        if (!enemy->cannotAttack() && enemy->canShoot(0))
            return 1;
    }
    return 0;
}

// Original: group_has_dragons; army.cpp:5469
DC_ADDRESS(0x04c1b8, 0x56)
MAC_ADDRESS(0x05429c, 0x54)
static bool groupHasDragons(long group)
{
    long i = g_combatManager->m_numArmies[group];
    while (i-- > 0) {
        army* enemy = &g_combatManager->m_armies[group][i];
        if (enemy->is(creatureKing1))
            return 1;
    }
    return 0;
}

// Complete's shared spell-validity worker is a desktop addition: the
// older DC is_valid_caliph_spell contains these per-spell tests itself.
// Both reuse the three ordinary group helpers above.
VA(0x00447a80, 0x429)
MAC_ADDRESS(0x0542f0, 0x17c)  // anchor-callee (four call sites, one of them the
                       // tail-jump from 0x447eb0), retail-only slot
bool isCreatureSpellUsefulOnTarget(int spell, const army* target)
{
    if (target->getSpellTime(spell))
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
        return !target->is(creatureDone);
    case SPELL_SLAYER:
        return groupHasDragons(1 - side);
    case SPELL_SHIELD:
    case SPELL_FIRE_SHIELD:
        return groupHasMelee(1 - side);
    case SPELL_AIR_SHIELD:
        return groupHasShooters(1 - side);
    case SPELL_PRECISION:
        return target->canShoot(0);
    case SPELL_BLOODLUST:
        return !target->canShoot(0);
    }
    return 1;
}

// Mac retains this helper; the Enchanter calls it twice. Its name is
// inferred.
MAC_ADDRESS(0x05446c, 0xac)
static bool enchanterSpellHasTarget(int spell)
{
    long side = g_combatManager->m_currentSide;
    if (akSpellTraits[spell].m_karma < 0)
        side = 1 - side;
    army* targets = g_combatManager->m_armies[side];
    long n = g_combatManager->m_numArmies[side];
    while (n-- != 0) {
        if (isCreatureSpellUsefulOnTarget(spell, targets))
            return 1;
    }
    return 0;
}

// DC public ?is_valid_caliph_spell@@YA_NW4SpellID@@PBVarmy@@@Z proves bool.
VA(0x00447eb0, 0x21)
DC_ADDRESS(0x04c210, 0x164)
MAC_ADDRESS(0x054518, 0x44)
bool isValidCaliphSpell(SpellID spell, const army* target)
{
    if (!(akSpellTraits[spell].m_flags & 0x800))
        return 0;
    return isCreatureSpellUsefulOnTarget(spell, target);
}

// E:\gamedcs\army.cpp:5546
// Inlined in both callers; no standalone retail body.
DC_ADDRESS(0x04c374, 0x38)
MAC_ADDRESS(0x05455c, 0x68)
long army::getValidCaliphSpells(const army* target) const
{
    long count = 0;
    for (SpellID spell = 10; spell < hero::NUM_SPELLS; spell++) {
        if (isValidCaliphSpell(spell, target))
            count++;
    }
    return count;
}

VA(0x00447ee0, 0xF8)
DC_ADDRESS(0x04c3ac, 0xba)
MAC_ADDRESS(0x0545c4, 0xd8)
void army::castCaliphSpell(long hex)
{
    if (!combatManager::validHex(hex))
        return;
    army* target = g_combatManager->m_cells[hex].getArmy();
    if (!target)
        return;
    SpellID spell;
    long count = getValidCaliphSpells(target);
    if (count == 0)
        return;
    long pick = random(1, count);
    for (spell = 10; spell < hero::NUM_SPELLS; spell++) {
        if (isValidCaliphSpell(spell, target)) {
            if (--pick == 0) {
                // This creature cast uses advanced mastery (2).
                g_combatManager->castSpell(spell, hex, 1, -1, eMasteryAdvanced, 6);
                return;
            }
        }
    }
}

// Mac retains this ordinary method after castCaliphSpell; Windows inlines
// it in castSpell. The name is inferred.
MAC_ADDRESS(0x05469c, 0x64)
void army::castFaerieDragonSpell(long hex)
{
    if (combatManager::validHex(hex))
        g_combatManager->castSpell(m_faerieDragonSpell, hex, 1, -1, eMasteryAdvanced,
                                   m_numTroops * 5);
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
MAC_ADDRESS(0x054700, 0x1f4)
unsigned char army::castEnchanterSpell()
{
    if (!g_combatManager->canCastSpells(g_combatManager->m_currentSide,
                                          0))
        return 0;

    long total = 0;
    const int* p = g_enchanterSpells;
    long spell;
    while (*p >= 0) {
        spell = *p++;
        if (enchanterSpellHasTarget(spell))
            total += *p;
        p++;
    }
    if (total == 0)
        return 0;

    long roll = rand() % total;
    for (p = g_enchanterSpells; *p >= 0; p += 2) {
        spell = *p;
        if (enchanterSpellHasTarget(spell))
            roll -= p[1];
        if (roll < 0)
            break;
    }

    if (!g_combatManager->isQuickCombat()) {
        long side = getControllingSide();
        army* stack = g_combatManager->m_armies[side];
        for (long i = 0; i < g_combatManager->m_numArmies[side]; i++, stack++) {
            if (stack->m_creatureType == ARMY_CREATURE_ENCHANTER
                && !stack->isIncapacitated()
                && !stack->is(creatureImmobilized)) {
                stack->m_showAttackFrames = 1;
                stack->m_showAttackFrameType = cs_range_r;
            }
        }
        playSample(SHOOT_SAMPLE);
        g_combatManager->powEffect(eSpellEffectNone, 1);
    }
    g_combatManager->castSpell(spell, -1, 1, -1, eMasteryExpert, 3);
    return 1;
}

VA(0x00448260, 0x582)
DC_ADDRESS(0x04c468, 0x30e)
MAC_ADDRESS(0x0548f4, 0x4f0)
void army::castSpell(long hex)
{
    long originalFacing = m_facing;
    if (!g_combatManager->isQuickCombat()) {
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
        // Direct side read: DC CastSpell has no get_owning_side call.
        g_combatManager->markCreatureEffect(group, index);
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
        long frames = m_stdIcon->GetNumFrames(m_currFrameType);
        if (frames == 0) {
            if (angle > 25.0f)
                m_currFrameType = cs_attack_ur;
            else if (angle > -25.0f)
                m_currFrameType = cs_attack_r;
            else
                m_currFrameType = cs_attack_dr;
            frames = m_stdIcon->GetNumFrames(m_currFrameType);
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
    case CREATURE_ARCHANGEL:
        castResurrect(hex);
        break;
    case ARMY_CREATURE_PIT_LORD:
        castDemonicResurrect(hex);
        break;
    case CREATURE_MASTER_GENIE:
        castCaliphSpell(hex);
        break;
    case CREATURE_FAERIE_DRAGON:
        castFaerieDragonSpell(hex);
        break;
    case CREATURE_STORM_ELEMENTAL:
        g_combatManager->castSpell(SPELL_PROTECTION_FROM_AIR, hex, 1, -1,
                                   eMasteryAdvanced, 6);
        break;
    case CREATURE_ICE_ELEMENTAL:
        g_combatManager->castSpell(SPELL_PROTECTION_FROM_WATER, hex, 1, -1,
                                   eMasteryAdvanced, 6);
        break;
    case CREATURE_ENERGY_ELEMENTAL:
        g_combatManager->castSpell(SPELL_PROTECTION_FROM_FIRE, hex, 1, -1,
                                   eMasteryAdvanced, 6);
        break;
    case CREATURE_MAGMA_ELEMENTAL:
        g_combatManager->castSpell(SPELL_PROTECTION_FROM_EARTH, hex, 1, -1,
                                   eMasteryAdvanced, 6);
        break;
    case CREATURE_OGRE_MAGE:
        g_combatManager->castSpell(SPELL_BLOODLUST, hex, 1, -1, eMasteryAdvanced, 6);
        break;
    }
    waitSample(SHOOT_SAMPLE);
    g_combatManager->m_extent = heroWindowManager::s_nullLimits;
    if (originalFacing != m_facing
        && !g_combatManager->isQuickCombat())
        turn(1);
}

VA(0x004487f0, 0x43)
MAC_ADDRESS(0x054de4, 0x5c)
int army::getMagicMirrorChance() const
{
    int effect = 0;
    if (getSpellTime(36) > 0)
        effect = m_backlashChance;
    if (m_creatureType == CREATURE_FAERIE_DRAGON) {
        const TSpellTraits* mirrorTraits =
            &akSpellTraits[SPELL_MAGIC_MIRROR];
        int current = effect;
        int innate = mirrorTraits->m_mastery_bonus[0];
        const int& selected = current < innate ? innate : current;
        return selected;
    }
    return effect;
}

VA(0x00448840, 0x26F)
DC_ADDRESS(0x04c778, 0xb4)
MAC_ADDRESS(0x054e40, 0xd8)
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
        AI_possible_targets |= 1 << enemy->index;
    if (AI_target) {
        long current = getAITargetTime();
        if (turns > current)
            return;
        if (turns == current && value <= AI_target_value)
            return;
    }
    AI_target = const_cast<army*>(enemy);
    AI_target_value = value;
    AI_target_distance = attackDistance;
}

VA(0x00448ab0, 0x114)
DC_ADDRESS(0x04c82c, 0x6a)
MAC_ADDRESS(0x054f18, 0xa0)
long army::getMultiHeadDirections(long ourHex, const army* enemy,
                                     long enemyHex) const
{
    long mask = 0xff;
    if (!is(creatureDoubleWide))
        mask = 0x3f;
    if (m_creatureType == ARMY_CREATURE_CERBERUS) {
        long direction = getAttackDirection(ourHex, enemy, enemyHex);

        mask = 1 << direction;
        mask |= 1 << getCounterClockwise(direction);
        mask |= 1 << getClockwise(direction);
    }
    return mask;
}

VA(0x00448bd0, 0xF6)
DC_ADDRESS(0x04c898, 0x80)
MAC_ADDRESS(0x054fb8, 0x70)
long army::getAITargetTime(long speed) const
{
    if (canShoot(0))
        return 1;
    if (!speed)
        return 1;
    long turns = (AI_target_distance + speed - 1) / speed;
    if (turns < 1)
        return 1;
    return turns;
}

VA_COMPGEN(0x00448d30, 0x36, VECTOR_ERASE, army)

VA(0x00448cd0, 0x4B)
DC_ADDRESS(0x04c918, 0x50)
MAC_ADDRESS(0x055028, 0x6c)
int army::getSpeed() const
{
    int speed = m_monInfo.speed;
    if (m_spellInfluence[54]) {
        if (is(creatureSiegeWeapon))
            return 0;
        speed = static_cast<long>(speed * m_slowFactor);
        if (speed <= 0)
            speed = 1;
    }
    return speed;
}

// army.obj's retained TResourceHandle<sample> default-constructor COMDAT.
VA_COMPGEN(0x00448d20, 0x9, CLASS_CTOR, TResourceHandle)

VA_COMPGEN(0x004490b0, 0x73, DEQUE_FREEFRONT, int)

VA_COMPGEN(0x00449130, 0x8E, DEQUE_FREEBACK, int)

// COMDAT pairing: deque<int>::erase(first, last).
VA_COMPGEN(0x00448db0, 0x2FE, DEQUE_ERASE, int)

// COMDAT pairing: vector<army*>::clear.
VA_COMPGEN(0x00448d70, 0x3D, VECTOR_CLEAR, army)

// COMDAT pairing: deque<int>::iterator::operator++ (0x449230, +4) and
// ::operator-- (0x449270, -4).
VA_COMPGEN(0x00449230, 0x33, DEQUE_ITERATOR_INC, int)

VA_COMPGEN(0x00449270, 0x33, DEQUE_ITERATOR_DEC, int)
