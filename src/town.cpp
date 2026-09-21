#include "va.h"

#include <algorithm>
#include <stdlib.h>
#include <string.h>

#include "town.h"

#include "advmgr.h"
#include "creaturetype.h"
#include "cursor.h"
#include "events.h"
#include "exec.h"
#include "game.h"
#include "herospec.h"
#include "kb.h"
#include "mapcell.h"
#include "misc.h"
#include "philai.h"
#include "resourcemanager.h"
#include "terrain.h"
#include "textresource.h"
#include "townmgr.h"

// Narrow town.obj-only globals reached by town::View. Their owning
// compilands remain outside the admitted surface.
DATA(0x0067f570) extern int g_unnamed67f570;
DATA(0x006aa5f0) extern int g_unnamed6aa5f0;
DATA(0x00699548) extern int g_unnamed699548;
DATA(0x0069778c) extern int g_unnamed69778c;
DATA(0x006994fc) extern townManager* g_townManager;
DATA(0x00699500) extern executive* g_executive;

// DC public ?included_buildings@town@@2PAY0CM@_JA; retail .bss
// 0x6a8bb8, nine 0x160-stride rows to 0x6a9818. Ownership: the DC
// data segment brackets it with town's other statics
// (NeutralBuildingCosts 0003:35990 / DwellingCosts 0003:36e54), and
// the retail .bss walk places it between smackmgr's tail (0x69fe5c)
// and widget's 0x6aac68 - the band this TU's text (0x5bde80) links in.
// Filled by initialize.cpp's create_included_masks; read by the
// can_build/can_ever_build family (0x5c0d20..0x5c0f20).
DATA(0x006a8bb8)
__int64 town::s_includedBuildings[TOWN_TYPE_COUNT][TOWN_BUILDING_SLOTS];

// The saved-game town name became a length-prefixed string at save
// version 25; before it, thirteen fixed bytes.
const int g_saveVersionTownNameString = 25;
const int g_townNameFixedLength = 13;

// E:\gamedcs\town.cpp:458, promoted from the carcass. Complete widens the
// Dreamcast signature with the save version - retail is `ret 8` where save
// below is `ret 4` - because only the reader has to cope with the old
// fixed-width name. Its two callers are game::Load's town pool and
// CCombatInitMsg::read, which deserializes the by-value town it carries.

// Every offset the body touches is already in town.h: the ten leading
// bytes, the garrison at +0xe0, the two hero ids through game.h's
// LoadHeroId, the name into kb's shared gText buffer, then population,
// generatorBonus, mageGuildSpellCounts, the three building masks, the
// mage-guild spell grid, a 70-BYTE buffer unpacked one BIT at a time into
// the bitset<70>, and a packed byte that splits three ways.
// Residual (98.17%): branches and the whole call multiset agree. Retail
// reads the five position/dock bytes through a SECOND char local, homed
// at [ebp-1], while the other twelve byte reads share charBuffer in the
// dead `infile` parameter home at [ebp+0xb] - recovering that local moves
// the spilled `this` to retail's [ebp-8]. What is left is one frame slot:
// retail also homes the name length in the dead `saveVersion` parameter
// home at [ebp+0xc] (it loads the dword and masks 0xffff), so its frame
// stays 0x54 where ours takes a fourth slot at [ebp-0x10] and 0x58.
// A 40-member source family over the local block, the length read and the
// name assignment ceilings at this same 98.1694 with 14 distinct objects, so
// the slot is not reachable from declaration order, scope or width: byte-flat
// are nameLength at function top (int or unsigned short), spellBuf first,
// posBuffer first, and an undeclared assignment; worse are an unsigned short
// nameLength read (98.10), reading into `saveVersion` itself (97.60) and
// hoisting `m_name = g_text` out of the two arms (88.68).

VA(0x005bcd60, 0x586)  // carcass promotion, dc 0x165628; anchor-callee armyGroup::load + LoadHeroId; callers game::Load and CCombatInitMsg::read
int town::load(TAbstractFile* infile, int saveVersion)
{
    char charBuffer;
    char posBuffer;
    unsigned char spellBuf[70];

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    m_id = charBuffer;
    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    m_owner = charBuffer;
    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    m_builtThisTurn = charBuffer;
    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    m_threateningHeroes = charBuffer;
    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    m_type = charBuffer;
    if (infile->read(&posBuffer, sizeof(posBuffer)) < sizeof(posBuffer))
        return -1;
    m_mapX = posBuffer;
    if (infile->read(&posBuffer, sizeof(posBuffer)) < sizeof(posBuffer))
        return -1;
    m_mapY = posBuffer;
    if (infile->read(&posBuffer, sizeof(posBuffer)) < sizeof(posBuffer))
        return -1;
    m_mapZ = posBuffer;
    if (infile->read(&posBuffer, sizeof(posBuffer)) < sizeof(posBuffer))
        return -1;
    m_dockSite = posBuffer;
    if (infile->read(&posBuffer, sizeof(posBuffer)) < sizeof(posBuffer))
        return -1;
    m_dockSiteY = posBuffer;

    if (m_garrison.load(infile) < 0)
        return -1;

    m_garrisonHeroId = loadHeroId(infile, saveVersion);
    m_visitingHeroId = loadHeroId(infile, saveVersion);

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    m_mageLevel = charBuffer;
    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    m_isGrouped = charBuffer;

    if (saveVersion >= g_saveVersionTownNameString) {
        int nameLength = 0;
        infile->read(&nameLength, sizeof(unsigned short));
        infile->read(g_text, nameLength & 0xffff);
        g_text[nameLength & 0xffff] = 0;
        m_name = g_text;
    } else {
        infile->read(g_text, g_townNameFixedLength);
        m_name = g_text;
    }

    if (infile->read(m_population, sizeof(m_population)) < sizeof(m_population))
        return -1;
    infile->read(m_generatorBonus, sizeof(m_generatorBonus));
    if (infile->read(m_mageGuildSpellCounts, sizeof(m_mageGuildSpellCounts))
        < sizeof(m_mageGuildSpellCounts))
        return -1;
    if (infile->read(&m_built, sizeof(m_built)) < sizeof(m_built))
        return -1;
    if (infile->read(&m_active, sizeof(m_active)) < sizeof(m_active))
        return -1;
    if (infile->read(&m_available, sizeof(m_available)) < sizeof(m_available))
        return -1;
    if (infile->read(m_mageGuildSpells, sizeof(m_mageGuildSpells))
        < sizeof(m_mageGuildSpells))
        return -1;

    if (infile->read(spellBuf, sizeof(spellBuf)) < sizeof(spellBuf))
        return -1;
    for (int spell = 0; spell < 70; ++spell) {
        m_spells.set(spell,
                   (spellBuf[spell / 8] & (1 << (spell % 8))) != 0);
    }

    if (infile->read(&charBuffer, sizeof(charBuffer)) < sizeof(charBuffer))
        return -1;
    m_manaVortexFull = charBuffer & 1;
    m_pondAmount = (charBuffer >> 1) & 7;
    m_pondResource = charBuffer >> 4;

    if (infile->read(&m_summoningType, sizeof(m_summoningType))
        < sizeof(m_summoningType))
        return -1;
    if (infile->read(&m_summoningPopulation, sizeof(m_summoningPopulation))
        < sizeof(m_summoningPopulation))
        return -1;
    return 0;
}

// E:\gamedcs\town.cpp:616, promoted from the carcass and the mirror of
// load above. It always writes the modern length-prefixed name, it packs
// field_38/field_34/field_33 back into one byte, and it re-packs the
// bitset<70> into 70 bytes of which only the first nine carry bits.
// Residual (99.8370%): the frame, 0x48 against retail's 0x50, and it is one
// recycled-home decision. Retail puts the BYTE `char_buffer` in the first
// parameter's padding byte [ebp+0xb] and gives the name-length dword its own
// slot at [ebp-8]; our CL puts the DWORD in the parameter home [ebp+8] and
// gives the byte its own slot at [ebp-1], which costs the 8 bytes and shifts
// spellBuf from [ebp-0x50] to [ebp-0x48]. Every other instruction agrees
// (59 = 59 blocks, all exact). MEASURED AND REJECTED 2026-09-06, all
// byte-flat at 99.8370: declaring nameLength FIRST in the local block,
// hoisting the spell loop counter to function scope, and block-scoping
// char_buffer around its whole run. VC6 hands the recycled home to the
// widest local that fits, and no declaration form observed here changes it.
VA(0x005bd2f0, 0x402)  // carcass promotion, dc 0x165988; anchor-callee armyGroup::save; caller game::Save's town pool
int town::save(TAbstractFile* outfile)
{
    char charBuffer;
    char posBuffer;
    unsigned char spellBuf[70];

    charBuffer = m_id;
    if (outfile->write(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;
    charBuffer = m_owner;
    if (outfile->write(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;
    charBuffer = m_builtThisTurn;
    if (outfile->write(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;
    charBuffer = m_threateningHeroes;
    if (outfile->write(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;
    charBuffer = m_type;
    if (outfile->write(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;
    posBuffer = m_mapX;
    if (outfile->write(&posBuffer, sizeof(posBuffer))
        < sizeof(posBuffer))
        return -1;
    posBuffer = m_mapY;
    if (outfile->write(&posBuffer, sizeof(posBuffer))
        < sizeof(posBuffer))
        return -1;
    posBuffer = m_mapZ;
    if (outfile->write(&posBuffer, sizeof(posBuffer))
        < sizeof(posBuffer))
        return -1;
    posBuffer = m_dockSite;
    if (outfile->write(&posBuffer, sizeof(posBuffer))
        < sizeof(posBuffer))
        return -1;
    posBuffer = m_dockSiteY;
    if (outfile->write(&posBuffer, sizeof(posBuffer))
        < sizeof(posBuffer))
        return -1;

    if (m_garrison.save(outfile) < 0)
        return -1;

    charBuffer = m_garrisonHeroId;
    if (outfile->write(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;
    charBuffer = m_visitingHeroId;
    if (outfile->write(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;
    charBuffer = m_mageLevel;
    if (outfile->write(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;
    charBuffer = m_isGrouped;
    if (outfile->write(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;

    unsigned short nameLength = m_name.size();
    outfile->write(&nameLength, sizeof(nameLength));
    outfile->write(m_name.c_str(), m_name.size());

    if (outfile->write(m_population, sizeof(m_population)) < sizeof(m_population))
        return -1;
    outfile->write(m_generatorBonus, sizeof(m_generatorBonus));
    if (outfile->write(m_mageGuildSpellCounts, sizeof(m_mageGuildSpellCounts))
        < sizeof(m_mageGuildSpellCounts))
        return -1;
    if (outfile->write(&m_built, sizeof(m_built)) < sizeof(m_built))
        return -1;
    if (outfile->write(&m_active, sizeof(m_active)) < sizeof(m_active))
        return -1;
    if (outfile->write(&m_available, sizeof(m_available)) < sizeof(m_available))
        return -1;
    if (outfile->write(m_mageGuildSpells, sizeof(m_mageGuildSpells))
        < sizeof(m_mageGuildSpells))
        return -1;

    memset(spellBuf, 0, sizeof(spellBuf));
    for (int spell = 0; spell < 70; ++spell) {
        if (m_spells.test(spell))
            spellBuf[spell / 8] |= 1 << (spell % 8);
    }
    if (outfile->write(spellBuf, sizeof(spellBuf)) < sizeof(spellBuf))
        return -1;

    charBuffer = static_cast<char>(
        (((m_pondResource & 7) << 3 | (m_pondAmount & 7)) << 1) | m_manaVortexFull);
    if (outfile->write(&charBuffer, sizeof(charBuffer))
        < sizeof(charBuffer))
        return -1;

    if (outfile->write(&m_summoningType, sizeof(m_summoningType))
        < sizeof(m_summoningType))
        return -1;
    if (outfile->write(&m_summoningPopulation, sizeof(m_summoningPopulation))
        < sizeof(m_summoningPopulation))
        return -1;
    return 0;
}

VA(0x005bd700, 0x47)  // dc 0x165d5c
int town::getPortraitFrame(bool isSmall) const
{
    int frame;
    if (m_active & g_bitNumber[CASTLE_FORT_ID])
        frame = m_type * 2;
    else
        frame = m_type * 2 + 18;

    if (m_builtThisTurn)
        frame++;
    if (isSmall)
        frame += 2;
    return frame;
}

VA(0x005bd750, 0x188)  // dc 0x165da4
void town::setSummoningGenerator()
{
    std::vector<int> generators;
    generator thisGenerator;
    int i;

    for (i = 0; i < g_game->m_generators.size(); ++i) {
        if (g_game->m_generators[i].getOwner() == m_owner)
            generators.push_back(i);
    }

    if (generators.size() == 0) {
        m_summoningType = CREATURE_NONE;
        return;
    }

    thisGenerator = g_game->m_generators[
        generators[random(0, generators.size() - 1)]];

    int creatureCount = 0;
    for (i = 0; i < 4; ++i) {
        if (thisGenerator.m_type[i] != CREATURE_NONE)
            ++creatureCount;
    }

    if (creatureCount) {
        int selectedCreature = rand() % creatureCount;
        for (i = 0; i < 4; ++i) {
            if (thisGenerator.m_type[i] != CREATURE_NONE
                && selectedCreature-- == 0)
                break;
        }
        m_summoningType = thisGenerator.m_type[i];
        m_summoningPopulation =
            g_creatureTypeTraits[m_summoningType].m_growthRate;
    }
}

VA(0x005bd8e0, 0x551)  // dc 0x165ea0
void town::applySpecialBuildingEffect(hero* townHero)
{
    if (m_type == TOWN_DUNGEON && m_manaVortexFull
        && hasBuilding(EXTRA_0_ID, 0)) {
        int maxMana = townHero->getMaxMana() * 2;
        if (townHero->m_mana < maxMana) {
            if (g_game->isLocalHuman(m_owner))
                normalDialog(g_generalText->getText(580), // Mana Vortex
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            townHero->m_mana = static_cast<short>(maxMana);
            m_manaVortexFull = 0;
        }
    }

    if (m_type == TOWN_CASTLE && hasBuilding(EXTRA_0_ID, 0)
        && !(townHero->m_flags & 2)) {
        townHero->m_flags |= 2;
        townHero->m_maxMovePoints += g_stablesMovementBonus;
        townHero->m_movePoints += g_stablesMovementBonus;
        if (g_game->isLocalHuman(townHero->m_owner))
            normalDialog(g_generalText->getText(581), // Stables
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }

    if (m_type == TOWN_TOWER && hasBuilding(EXTRA_2_ID, 0)
        && !townHero->m_townSpecialGrantedMask.test(m_id)) {
        townHero->m_townSpecialGrantedMask[m_id] = 1;
        townHero->adjustPrimarySkill(3, 1);
        if (g_game->isLocalHuman(townHero->m_owner))
            normalDialog(
                g_generalText->getText(582), // Wall of Knowledge
                1, -1, -1, 0x22, 1, -1, 0, -1, 0, -1, 0);
    }

    if (m_type == TOWN_INFERNO && hasBuilding(EXTRA_2_ID, 0)
        && !townHero->m_townSpecialGrantedMask[m_id]) {
        townHero->m_townSpecialGrantedMask[m_id] = 1;
        townHero->adjustPrimarySkill(2, 1);
        if (g_game->isLocalHuman(townHero->m_owner))
            normalDialog(g_generalText->getText(583), // Order of Fire
                         1, -1, -1, 0x21, 1, -1, 0, -1, 0, -1, 0);
    }

    if (m_type == TOWN_DUNGEON && hasBuilding(EXTRA_2_ID, 0)
        && !townHero->m_townSpecialGrantedMask[m_id]) {
        int experience = static_cast<int>(
            townHero->getExperienceBonusFactor() * 1000.0f);
        if (g_game->isLocalHuman(townHero->m_owner))
            normalDialog(
                g_generalText->getText(584), // Battle Scholar Academy
                1, -1, -1, 0x11, experience, -1, 0, -1, 0, -1, 0);
        townHero->m_townSpecialGrantedMask[m_id] = 1;
        townHero->giveExperience(experience, 1, 1);
    }

    if (m_type == TOWN_STRONGHOLD && hasBuilding(EXTRA_2_ID, 0)
        && !townHero->m_townSpecialGrantedMask[m_id]) {
        if (g_game->isLocalHuman(townHero->m_owner))
            normalDialog(
                g_generalText->getText(585), // Hall of Valhalla
                1, -1, -1, 0x1f, 1, -1, 0, -1, 0, -1, 0);
        townHero->m_townSpecialGrantedMask[m_id] = 1;
        townHero->adjustPrimarySkill(0, 1);
    }

    if (m_type == TOWN_FORTRESS && hasBuilding(SPECIAL_BUILDING_ID, 0)
        && !townHero->m_townSpecialGrantedMask[m_id]) {
        if (g_game->isLocalHuman(townHero->m_owner))
            normalDialog(
                g_generalText->getText(586), // Cage of Warlords
                1, -1, -1, 0x20, 1, -1, 0, -1, 0, -1, 0);
        townHero->m_townSpecialGrantedMask[m_id] = 1;
        townHero->adjustPrimarySkill(1, 1);
    }
}

VA(0x005bde80, 0xD4)  // dc 0x166408
town::town()
{
    m_type = 0;
    m_pondAmount = 0;
    m_pondResource = -1;
    m_id = 0;
    m_mapX = 0;
    m_mapY = 0;
    m_mapZ = 0;
    m_visitingHeroId = -1;
    m_built = g_bitNumber[HALL_VILLAGE_ID];
    m_active = m_built;
    m_mageLevel = 0;
    m_owner = -1;
    m_garrisonHeroId = -1;
    int slot;
    MEMSET(m_garrison.m_armies, -1, sizeof(m_garrison.m_armies), slot);
    m_summoningType = CREATURE_NONE;
    m_builtThisTurn = 0;
    m_manaVortexFull = 1;
}

VA(0x005bdf60, 0x76)  // dc 0x1664b0
void town::initializeHordes()
{
    int creatureBase = 0;
    for (short townType = 0; townType < TOWN_TYPE_COUNT; townType++) {
        for (short entry = 0; entry < 4; entry += 2) {
            type_horde_effect* effect = &s_constHordeEffects[townType][entry];
            TCreatureType creature = effect->m_creature;
            type_horde_effect* upgrade = effect + 1;
            short slot;
            for (slot = 0; slot <= TOWN_DWELLING_COUNT; slot++) {
                if (creature == g_townDwellingCreatures[creatureBase + slot])
                    break;
            }
            if (slot <= TOWN_DWELLING_COUNT) {
                effect->m_dwelling = slot;
                slot += TOWN_DWELLING_COUNT;
                upgrade->m_creature = g_townDwellingCreatures[creatureBase + slot];
                upgrade->m_dwelling = slot;
                const short* bonus = &effect->m_bonus;
                upgrade->m_bonus = *bonus;
            }
        }
        creatureBase += 2 * TOWN_DWELLING_COUNT;
    }
}

VA(0x005bdfe0, 0x4E)  // dc 0x16654c
int town::hasGarrison()
{
    if (m_visitingHeroId < 0) {
        armyGroup* group;
        if (m_garrisonHeroId < 0)
            group = &m_garrison;
        else
            group = &g_game->getHero(m_garrisonHeroId)->m_army;
        if (!group->hasCreatures())
            return 0;
    }
    return 1;
}

VA(0x005be030, 0x1D3)  // dc 0x1665a0
void town::giveSpells(hero* forceHero) const
{
    if (!forceHero && m_visitingHeroId == -1 && m_garrisonHeroId == -1)
        return;

    int heroIndex = 0;
    while (heroIndex < (2 - (forceHero != 0))) {
        hero* currentHero;
        if (forceHero) {
            currentHero = forceHero;
        } else {
            int heroId;
            if (heroIndex == 0)
                heroId = m_visitingHeroId;
            else
                heroId = m_garrisonHeroId;
            currentHero = g_game->getHero(heroId);
        }

        if (currentHero) {
            if (currentHero->isWieldingArtifact(ARTIFACT_SPELLBOOK)) {
                if (hasBuilding(MAGE_GUILD_ID, 1)) {
                    if (m_type == TOWN_CONFLUX
                        && (m_active & g_bitNumber[HOLY_GRAIL_ID])) {
                        for (int spell = 0; spell < hero::NUM_SPELLS; ++spell) {
                            if (!m_spells.test(spell)
                                && g_spellTraits[spell].m_level
                                    < currentHero->m_skillLevel[
                                        eSecSkillWisdom] + 3
                                && spell != SPELL_TITANS_LIGHTNING_BOLT)
                                currentHero->addSpell(spell);
                        }
                    } else {
                        for (int level = 0;
                             level < currentHero->m_skillLevel[
                                         eSecSkillWisdom] + 2
                                 && level <= m_mageLevel;
                             ++level) {
                            for (int slot = 0;
                                 slot < m_mageGuildSpellCounts[level]; ++slot)
                                currentHero->addSpell(
                                    m_mageGuildSpells[level][slot]);
                        }
                    }
                }
            }
        }
        ++heroIndex;
    }
}

VA(0x005be210, 0xC0)  // dc 0x166688
void town::view(int alreadyFaded)
{
    int threshold = g_unnamed67f570 + 0x514;
    g_unnamed6aa5f0 = 1;
    if (threshold > 0xb54)
        g_unnamed699548 = 2;
    else if (threshold > 0x320)
        g_unnamed699548 = 1;

    g_townManager->setTown(this);
    g_executive->callManager(g_townManager);

    town* viewedTown = g_townManager->m_townToView;
    int heroId = viewedTown->m_visitingHeroId;
    if (heroId != -1) {
        hero* visitingHero = g_game->getHero(heroId);
        if (visitingHero->m_owner == g_unnamed69778c) {
            g_advManager->setHeroContext(visitingHero->m_id, 0, 0, 1);
            g_unnamed699548 = 0;
            g_unnamed6aa5f0 = 0;
            return;
        }
    }
    g_unnamed699548 = 0;
    g_unnamed6aa5f0 = 0;
}

VA(0x005be2d0, 0xB3)  // dc 0x166720
void town::deallocate()
{
    playerData* player = &g_game->m_players[m_owner];
    int slot = -1;
    for (int i = 0; i < player->m_numTowns; i++) {
        if (player->m_townIds[i] == m_id)
            slot = i;
    }
    for (int j = slot; j < player->m_numTowns - 1; j++)
        player->m_townIds[j] = player->m_townIds[j + 1];
    player->m_townIds[player->m_numTowns - 1] = -1;
    if (player->m_currTownId == m_id)
        player->m_currTownId = -1;
    player->m_numTowns--;
    g_advManager->m_advWindow->updateTownLocators(0, 1, 1);
    g_game->m_towns[m_id].m_owner = -1;
    m_owner = -1;
}

VA(0x005be390, 0xB7)  // dc 0x16682c
void town::removeGarrisonHero()
{
    if (m_garrisonHeroId < 0)
        return;
    hero* garrisonHero = g_game->getHero(m_garrisonHeroId);
    int player = m_owner;
    m_visitingHeroId = m_garrisonHeroId;
    m_garrisonHeroId = -1;
    hero* placedHero = g_game->getHero(garrisonHero->m_id);
    type_point point;
    point.m_x = m_mapX;
    point.m_y = m_mapY;
    point.m_z = m_mapZ;
    placedHero->placeInMap(player, point, 0);
}

// E:\gamedcs\town.cpp:1111
// Exchanges the two resident heroes, removes the departing visitor from the
// acting player's roster and map cell, broadcasts the hide change, then
// places the departing garrison hero on the town tile. Retail clears the two
// adventure-view latches only when the hidden hero was both current and
// locally owned. Dreamcast splits the exchange over lines 1115-1117; the
// precise source spelling is unknown because no temporary survives CodeView.
// Retail requires `std::swap`'s reference boundary: a natural temporary plus
// two assignments makes VC6 retain the already-loaded ids and falls to
// 88.92481%. The coherent base-first CMCHideHero constructor leaves only its
// caller-specific zero/id store schedule unmatched (97.77444%); reversing the
// shared constructor closes this caller but contradicts netmsg.h:717-718 and
// breaks exact playerData::add_garrison_hero, so that old 100% remains history.
VA(0x005be450, 0x1AC)  // anchor-global, dc 0x166864
void town::swapHeroes()
{
    town* currentTown = this;
    hero* garrisonHero = g_game->getHero(currentTown->m_garrisonHeroId);
    hero* visitingHero = g_game->getHero(currentTown->m_visitingHeroId);

    std::swap(currentTown->m_garrisonHeroId, currentTown->m_visitingHeroId);

    int rosterIndex = g_currentPlayer->findHero(visitingHero->m_id);
    g_game->recordHideHero(visitingHero, visitingHero->m_owner, 0);
    visitingHero->restoreCell();

    CMCHideHero hideHero(visitingHero->m_id);
    sendMapChange(&hideHero);

    for (int i = rosterIndex; i < g_currentPlayer->m_numHeroes - 1; ++i)
        g_currentPlayer->m_heroes[i] = g_currentPlayer->m_heroes[i + 1];
    --g_currentPlayer->m_numHeroes;
    g_currentPlayer->m_heroes[g_currentPlayer->m_numHeroes] = -1;

    if (g_currentPlayer->m_currHeroId == visitingHero->m_id) {
        g_currentPlayer->m_currHeroId = -1;
        if (g_netLocalGamePos == visitingHero->m_owner) {
            g_advManager->m_drawCursor = 0;
            g_advManager->m_curHeroMobile = 0;
        }
    }

    int player = currentTown->m_owner;
    hero* placedHero = g_game->getHero(garrisonHero->m_id);
    type_point point;
    point.m_x = currentTown->m_mapX;
    point.m_y = currentTown->m_mapY;
    point.m_z = currentTown->m_mapZ;
    placedHero->placeInMap(player, point, 0);
}

VA(0x005be600, 0x32A)  // dc 0x166950
void town::initializeSpells(const TownExtra* townSetup)
{
    std::bitset<70> prohibited;
    for (int spell = 0; spell < hero::NUM_SPELLS; ++spell) {
        // Complete builds this mask one bit at a time; retail 0x5be668
        // retains Dinkumware's set(position,bool), without a game adapter.
        prohibited.set(spell,
            m_spells[spell] || g_game->m_spellDisabledInfo[spell]);
    }

    for (int level = 1; level <= 5; ++level) {
        for (int slot = 0;
             slot < g_mageGuildBaseSpellCounts[level - 1] + 1; ++slot) {
            int totalWeight = 0;
            int spell;
            for (spell = 0; spell < hero::NUM_SPELLS; ++spell) {
                if (!prohibited[spell]
                    && g_spellTraits[spell].m_level == level) {
                    totalWeight +=
                        g_spellTraits[spell].m_townProbability[m_type];
                    if (townSetup->m_fixedSpells[spell]) {
                        m_mageGuildSpells[level - 1][slot] = spell;
                        prohibited.set(spell);
                        break;
                    }
                }
            }

            if (spell < hero::NUM_SPELLS)
                continue;
            if (totalWeight == 0) {
                m_mageGuildSpells[level - 1][slot] = -1;
                continue;
            }
            int roll = random(1, totalWeight);
            for (spell = 0; spell < hero::NUM_SPELLS; ++spell) {
                if (!prohibited[spell]
                    && g_spellTraits[spell].m_level == level) {
                    roll -= g_spellTraits[spell].m_townProbability[m_type];
                    if (roll <= 0)
                        break;
                }
            }
            m_mageGuildSpells[level - 1][slot] = spell;
            prohibited.set(spell);
        }
    }

    int guildLevel = 5;
    while (guildLevel > 0
           && !hasBuilding(guildLevel - 1, false))
        --guildLevel;
    m_mageLevel = static_cast<signed char>(guildLevel);
    setSpellsAvailable();
}

// E:\gamedcs\town.cpp:1206
void town::setSpellsAvailable()
{
    memset(m_mageGuildSpellCounts, 0, sizeof(m_mageGuildSpellCounts));
    for (int level = 1; level <= m_mageLevel; level++) {
        int count = g_mageGuildBaseSpellCounts[level - 1];
        if (m_type == TOWN_TOWER && hasBuilding(EXTRA_1_ID, 1))
            count++;
        while (count > 0
               && m_mageGuildSpells[level - 1][count - 1] == -1)
            count--;
        m_mageGuildSpellCounts[level - 1] = count;
    }
}

// E:\gamedcs\town.cpp:1226
VA(0x005be930, 0x330)  // body (built-mask OR) + order-map, dc 0x166c08
type_building_id town::createBuilding(type_building_id building)
{
    m_built |= g_bitNumber[building];
    m_built &= ~s_includedBuildings[m_type][building];

    for (int slot = 0; slot < TOWN_HORDE_SLOTS; slot++) {
        short dw = s_constHordeEffects[m_type][slot].m_dwelling;
        if (building == g_hordeBuildings[slot]) {
            m_built &= ~g_bitNumber[DWELLING_0_ID + dw];
            if (dw < TOWN_DWELLING_COUNT
                && (m_built & g_bitNumber[DWELLING_0_UPG_ID + dw])) {
                m_built &= ~g_bitNumber[building];
                m_built &= ~g_bitNumber[DWELLING_0_UPG_ID + dw];
                building = g_hordeBuildings[slot + 1];
                m_built |= g_bitNumber[building];
            }
        }
        if (m_built & g_bitNumber[g_hordeBuildings[slot]]) {
            if (building == DWELLING_0_UPG_ID + dw) {
                m_built &= ~g_bitNumber[building];
                m_built &= ~g_bitNumber[g_hordeBuildings[slot]];
                building = g_hordeBuildings[slot + 1];
                m_built |= g_bitNumber[building];
            }
        }
    }

    if (building >= DWELLING_0_ID && building <= DWELLING_6_ID) {
        short slot = building - DWELLING_0_ID;
        m_population[slot] = g_creatureTypeTraits[g_townDwellingCreatures[
            m_type * (2 * TOWN_DWELLING_COUNT) + slot]].m_growthRate;
    }
    if (building >= DWELLING_0_UPG_ID && building <= DWELLING_6_UPG_ID) {
        short slot = building - DWELLING_0_UPG_ID;
        short upgraded = building - DWELLING_0_ID;
        m_population[upgraded] = m_population[slot];
        m_population[slot] = 0;
    }

    if (m_type == TOWN_RAMPART || m_type == TOWN_NECROPOLIS
        || m_type == TOWN_CONFLUX) {
        switch (building) {
        case HALL_VILLAGE_ID:
            createBuilding(EXTRA_2_ID);
            break;
        case HALL_TOWN_ID:
            createBuilding(EXTRA_3_ID);
            break;
        case HALL_CITY_ID:
            createBuilding(EXTRA_4_ID);
            break;
        case HALL_CAPITOL_ID:
            createBuilding(EXTRA_5_ID);
            break;
        }
    }
    return building;
}

VA(0x005bec60, 0x173)  // dc 0x166ed8
void town::destroyExtraCapitol()
{
    if (isCapitol() && m_owner >= 0) {
        playerData* player = &g_game->m_players[m_owner];
        int townCount = player->m_numTowns;

        for (int slot = 0; slot < townCount; ++slot) {
            const char& townId = player->m_townIds[slot];
            if (townId != m_id) {
                town* otherTown = g_game->getTown(townId);
                if (otherTown->isCapitol()) {
                    m_built &= ~g_bitNumber[HALL_CAPITOL_ID];
                    m_built |= g_bitNumber[HALL_CITY_ID];
                    m_active &= ~g_bitNumber[HALL_CAPITOL_ID];

                    NewmapCell* cell =
                        g_game->m_worldMap.cell(m_mapX, m_mapY, m_mapZ);
                    g_game->convertObject(cell);
                    break;
                }
            }
        }
    }
}

// The kb.h prototype; town.cpp must not widen its include closure for
// one free-function call (the include-set class), so the declaration is
// repeated here the way the import-thunk precedent repeats declarations
// per-TU.
void checkEndGame(int forceWin);

// E:\gamedcs\town.cpp:1340
VA(0x005bede0, 0x427)  // anchor-global, dc 0x166fc8
type_building_id town::buildBuilding(int buildingId,
                                     unsigned char setBuiltFlag,
                                     unsigned char applySpecialEffect)
{
    type_building_id built;
    unsigned char hadFort = isCastle();
    unsigned char hadCapitol = isCapitol();
    // The parameter is int - DC-attested (`...QAA?AW4type_building_id@@
    // HEE@Z`) and required by the townmgr call sites - while
    // create_building's domain is the enum; the conversion is the
    // boundary between retail's own two spellings of the id.
    built = createBuilding(type_building_id(buildingId));

    if (setBuiltFlag && buildingId != DOCK_WITH_BOAT_ID) {
        if (g_game->m_setup.m_difficulty < 2) {
            int team = m_owner;
            if (team >= 0)
                team = g_game->m_mapHeader.m_teamInfo[team];
            if (!g_game->isHumanAlly(team))
                m_builtThisTurn = 2;
            else
                m_builtThisTurn = 1;
        } else {
            m_builtThisTurn = 1;
        }
    }

    updateFullBuildingMask();

    if (buildingId <= MAGE_GUILD5_ID) {
        m_mageLevel = buildingId + 1;
        setSpellsAvailable();
    }
    if (m_type == TOWN_TOWER && buildingId == EXTRA_1_ID) {
        setSpellsAvailable();
    }
    giveSpells(0);

    if ((isCapitol() && !hadCapitol)
        || (isCastle() && !hadFort))
    {
        g_game->convertObject(g_game->m_worldMap.cell(m_mapX, m_mapY, m_mapZ));
    }
    if (g_game->m_mapHeader.m_victoryCondition.checkForUpgradedTown())
        checkEndGame(0);

    if (m_type == TOWN_TOWER) {
        if (buildingId == EXTRA_0_ID) {
            g_game->setVisibility(m_mapX, m_mapY, m_mapZ, m_owner, 20, 0);
        } else if (buildingId == HOLY_GRAIL_ID) {
            g_game->setVisibility(g_mapWidth / 2, g_mapHeight / 2, 0, m_owner,
                                  g_mapWidth, 0);
            if (g_game->m_worldMap.getNumLevels() > 1)
                g_game->setVisibility(g_mapWidth / 2, g_mapHeight / 2, 1,
                                      m_owner, g_mapWidth, 0);
        }
    }

    if (m_garrisonHeroId != -1 && applySpecialEffect)
        applySpecialBuildingEffect(g_game->getHero(m_garrisonHeroId));
    if (m_visitingHeroId != -1 && applySpecialEffect)
        applySpecialBuildingEffect(g_game->getHero(m_visitingHeroId));
    return built;
}

VA(0x005bf210, 0x1A5)  // dc 0x1671cc
void town::updateShipyard()
{
    if (m_active & g_bitNumber[DOCK_ID]) {
        type_point point;
        point.m_x = m_dockSite;
        point.m_y = m_dockSiteY;
        point.m_z = m_mapZ;

        NewmapCell* cell = g_game->m_worldMap.cell(point.m_x, point.m_y, point.m_z);
        if (cell->m_isTrigger && (cell->m_type == BOAT || cell->m_type == HERO)) {
            if (!(m_active & g_bitNumber[DOCK_WITH_BOAT_ID])) {
                createBuilding(DOCK_WITH_BOAT_ID);
                return;
            }
        } else if (m_built & g_bitNumber[DOCK_WITH_BOAT_ID]) {
            m_built &= ~g_bitNumber[DOCK_WITH_BOAT_ID];
            m_active = m_built;
            for (int building = 0; building < TOWN_BUILDING_SLOTS; building++) {
                if (m_built & g_bitNumber[building])
                    m_active |= s_includedBuildings[m_type][building];
            }
        }
    }
}

VA(0x005bf3c0, 0x11E)  // dc 0x167274
unsigned char town::buyBuilding(type_building_id building)
{
    if (m_owner < 0)
        return 0;
    if (!canBuild(building))
        return 0;
    int* costs = getBuildCostArray(building);
    playerData* player = &g_game->m_players[m_owner];
    if (!player->isHuman()) {
        unnamed526d20(m_owner, costs, 1);
        if (m_builtThisTurn)
            return 0;
    }
    for (int resource = 0; resource < NUM_RESOURCES; resource++) {
        if (player->m_resources[resource] < costs[resource])
            return 0;
    }
    for (int paid = 0; paid < NUM_RESOURCES; paid++)
        player->m_resources[paid] -= costs[paid];
    buildBuilding(building, 1, 1);
    return 1;
}

VA(0x005bf4e0, 0xC)  // dc 0x167378
unsigned char town::canBuildDock() const
{
    return m_dockSite != TOWN_DOCK_SITE_NONE;
}

VA(0x005bf4f0, 0x7A)  // dc 0x167388
void town::calcNumLevelArchers(int* numArchers, int* archerLevel)
{
    *archerLevel = 10;
    int level = 4;
    for (int building = 0; building < MAX_BUILDING_TYPE; building++) {
        if ((m_available & g_bitNumber[building]) && (m_built & g_bitNumber[building]))
            level++;
    }
    *numArchers = level;
}

VA(0x005bf570, 0x86)  // dc 0x1673dc
long town::getCastleGrowthBonus(TCreatureType creature) const
{
    if (m_built & g_bitNumber[CASTLE_CASTLE_ID])
        return g_creatureTypeTraits[creature].m_growthRate;
    if (m_built & g_bitNumber[CASTLE_CITADEL_ID])
        return g_creatureTypeTraits[creature].m_growthRate / 2;
    return 0;
}

VA(0x005bf600, 0xC6)  // dc 0x167458
short town::getGoldIncome(unsigned char includeSilo) const
{
    short income = 500;
    if (m_built & g_bitNumber[HALL_TOWN_ID])
        income = 1000;
    if (m_built & g_bitNumber[HALL_CITY_ID])
        income = 2000;
    if (m_built & g_bitNumber[HALL_CAPITOL_ID])
        income = 4000;
    if (includeSilo && (m_built & g_bitNumber[MARKETPLACE_SILO_ID]))
        income += getSiloIncome()[GOLD];
    if (m_active & g_bitNumber[HOLY_GRAIL_ID])
        income += 5000;
    return income;
}

VA(0x005bf6d0, 0x97)  // dc 0x1674d4
int town::getHorde(long dwelling) const
{
    if (!(m_active & g_bitNumber[DWELLING_0_ID + dwelling]))
        return MAX_BUILDING_TYPE;
    for (int slot = 0; slot < TOWN_HORDE_SLOTS; slot++) {
        if (m_active & g_bitNumber[g_hordeBuildings[slot]]) {
            if (s_constHordeEffects[m_type][slot].m_dwelling == dwelling)
                return g_hordeBuildings[slot];
        }
    }
    return MAX_BUILDING_TYPE;
}

VA(0x005bf770, 0x9E)  // dc 0x167544
long town::getHordeBonus(long dwelling) const
{
    if (!(m_active & g_bitNumber[DWELLING_0_ID + dwelling]))
        return 0;
    long bonus = 0;
    for (int slot = 0; slot < TOWN_HORDE_SLOTS; slot++) {
        if (m_active & g_bitNumber[g_hordeBuildings[slot]]) {
            if (s_constHordeEffects[m_type][slot].m_dwelling == dwelling)
                bonus = s_constHordeEffects[m_type][slot].m_bonus;
        }
    }
    return bonus;
}

VA(0x005bf810, 0xE2)
long town::getAssembledLegionBonus(long dwelling)
{
    long bonus = 0;
    if (m_owner >= 0 && g_game->m_players[m_owner].hasGivenArtifact(0x85)) {
        long growth = g_creatureTypeTraits[g_townDwellingCreatures[
            m_type * (2 * TOWN_DWELLING_COUNT) + dwelling]].m_growthRate;
        if (m_built & g_bitNumber[CASTLE_CASTLE_ID])
            bonus = growth;
        else if (m_built & g_bitNumber[CASTLE_CITADEL_ID])
            bonus = growth / 2;
        else
            bonus = 0;
        bonus += growth;
        bonus /= 2;
    }
    return bonus;
}

VA(0x005bf900, 0x258)  // dc 0x1675d4
long town::getLegionBonus(long dwelling) const
{
    game* currentGame = g_game;
    int tier = dwelling % TOWN_DWELLING_COUNT + 1;
    hero* garrisonHero = 0;
    hero* visitingHero = 0;
    int bonus = 0;

    if (m_garrisonHeroId >= 0)
        garrisonHero = currentGame->getHero(m_garrisonHeroId);

    if (m_visitingHeroId >= 0) {
        visitingHero = currentGame->getHero(m_visitingHeroId);
    } else {
        // Constructor form, not default-then-assign: it merges the y|z
        // bitfield unit into one clear-then-or (99.8469 -> 100.0000).
        type_point point(m_mapX, m_mapY, m_mapZ);

        NewmapCell* cell = currentGame->m_worldMap.cell(point.m_x, point.m_y, point.m_z);
        if (cell->m_type == HERO)
            visitingHero = currentGame->getHero(cell->m_extraInfo);
    }

    switch (tier) {
    case TOWN_DWELLING_TIER_2:
        if (visitingHero)
            bonus = 5 * visitingHero->isWieldingArtifact(0x76);
        if (garrisonHero)
            bonus += 5 * garrisonHero->isWieldingArtifact(0x76);
        break;
    case TOWN_DWELLING_TIER_3:
        if (visitingHero)
            bonus = 4 * visitingHero->isWieldingArtifact(0x77);
        if (garrisonHero)
            bonus += 4 * garrisonHero->isWieldingArtifact(0x77);
        break;
    case TOWN_DWELLING_TIER_4:
        if (visitingHero)
            bonus = 3 * visitingHero->isWieldingArtifact(0x78);
        if (garrisonHero)
            bonus += 3 * garrisonHero->isWieldingArtifact(0x78);
        break;
    case TOWN_DWELLING_TIER_5:
        if (visitingHero)
            bonus = 2 * visitingHero->isWieldingArtifact(0x79);
        if (garrisonHero)
            bonus += 2 * garrisonHero->isWieldingArtifact(0x79);
        break;
    case TOWN_DWELLING_TIER_6:
        if (visitingHero)
            bonus = visitingHero->isWieldingArtifact(0x7a);
        if (garrisonHero)
            bonus += garrisonHero->isWieldingArtifact(0x7a);
        break;
    }
    return bonus;
}

VA(0x005bfb60, 0x266)  // dc 0x167748
short town::getGrowthRate(short dwelling) const
{
    long dwellingIndex = dwelling;
    if (!(m_active & g_bitNumber[DWELLING_0_ID + dwellingIndex]))
        return 0;
    if (dwelling < TOWN_DWELLING_COUNT
        && (m_active & g_bitNumber[DWELLING_0_UPG_ID + dwellingIndex]))
        return 0;

    TCreatureType creature = g_townDwellingCreatures[
        m_type * TOWN_DWELLING_SLOTS + dwellingIndex];
    short growth = g_creatureTypeTraits[creature].m_growthRate;
    growth += getCastleGrowthBonus(creature);

    if (m_owner >= 0) {
        long legionBonus = 0;
        if (g_game->m_players[m_owner].hasGivenArtifact(0x85)) {
            TCreatureType legionCreature = g_townDwellingCreatures[
                m_type * TOWN_DWELLING_SLOTS + dwellingIndex];
            long legionGrowth =
                g_creatureTypeTraits[legionCreature].m_growthRate;
            long castleBonus;
#pragma inline_depth(0)
            if (m_built & g_bitNumber[CASTLE_CASTLE_ID])
                castleBonus = legionGrowth;
            else if (hasBuilding(CASTLE_CITADEL_ID, 0))
                castleBonus =
                    g_creatureTypeTraits[legionCreature].m_growthRate / 2;
            else
                castleBonus = 0;
#pragma inline_depth()
            legionBonus = (legionGrowth + castleBonus) / 2;
        }
        growth += legionBonus;
        growth += getLegionBonus(dwellingIndex);
    }

    for (short slot = 0; slot < TOWN_HORDE_SLOTS; slot++) {
        if ((m_active & g_bitNumber[g_hordeBuildings[slot]])
            && s_constHordeEffects[m_type][slot].m_dwelling == dwelling) {
            growth += s_constHordeEffects[m_type][slot].m_bonus;
            break;
        }
    }

    growth += m_generatorBonus[dwellingIndex];
    if (m_active & g_bitNumber[HOLY_GRAIL_ID])
        growth += growth / 2;
    return growth;
}

VA(0x005bfdd0, 0x74)  // dc 0x167848
void town::increasePopulation(TCreatureType bonusCreature,
                               TCreatureType alternateBonus, long bonusAmount)
{
    for (short dwelling = 0; dwelling < TOWN_DWELLING_SLOTS; dwelling++) {
        short growth = getGrowthRate(dwelling);
        if (growth > 0) {
            TCreatureType creature =
                g_townDwellingCreatures[TOWN_DWELLING_SLOTS * m_type + dwelling];
            if (creature == bonusCreature || creature == alternateBonus)
                growth += bonusAmount;
            if (m_owner == -1)
                growth = growth / 2;
            m_population[dwelling] += growth;
        }
    }
}

VA(0x005bfe50, 0x55)  // dc 0x167900
void town::changeGeneratorBonus(TCreatureType creature, long change)
{
    int slot;
    for (slot = 0; slot < TOWN_DWELLING_SLOTS; slot++) {
        if (g_townDwellingCreatures[TOWN_DWELLING_SLOTS * m_type + slot]
            == creature)
            break;
    }
    if (slot == TOWN_DWELLING_SLOTS)
        return;
    m_generatorBonus[slot] += change;
    if (slot < TOWN_DWELLING_COUNT)
        m_generatorBonus[slot + TOWN_DWELLING_COUNT] += change;
}

// The kb.h and castle.h prototypes, repeated file-locally for the
// reason CheckEndGame above is.
void extendedDialog(const char* text,
                     std::vector<type_dialog_resource>& resources,
                     long x, long y, long timeout);
const char* getBuildingName(int townType, int buildingId);

void showBuildingRewards(const town* thisTown,
                           std::vector<type_dialog_resource>* rewards);
void showCreatureRewards(const town* thisTown,
                           std::vector<type_dialog_resource>* rewards);

// The reward dialog flushes in batches of eight rows (the extended
// dialog's row capacity); named per the kStartLevelCampaign precedent.
static const int g_rewardDialogBatch = 8;

// E:\gamedcs\town.cpp:1793
// Still open: branch topology #12 lands one block off (the D3
// jump-threading class - why-branch's catalog found no applicable
// lever). Restoring the two source-proven HasBuilding calls is byte-flat
// after inlining. A shared-tail `bool has_reward` keeps 20 branches but
// falls to 78.6294, so it is not the route to retail's cross-jump.
// `why-reg --model --il-order` agrees on every first register definition;
// the divergence starts after the B1 minimum slice. Also rejected: an
// `unsigned short growth` used only by the zero test (byte-flat - VC6
// folds it back into the memory compare), the eligible mask hoisted into
// a local (85.7), and resource-store reordering (neutral).
// Translates the event's 41-bit editor building mask through this
// faction's gEventBuildingIds row, masks away what is already active,
// illegal for the faction, or dock-impossible, builds the survivors
// (flushing a dialog every eight), then applies the seven generator
// bonuses to whichever dwelling tier is active, upgraded first.
// 2026-09-06, polish lane 38, the DC LOCAL-SCOPE SWEEP, also a NEGATIVE: the
// Dreamcast block names TWO T_QUAD masks (`exclude_mask` = this body's `mask`,
// `reward_mask` = `grantable`) and no third, so `eventBuildings` reads as a
// cache of `thisEvent->BuildBuildings` that retail reloads. Re-reading the
// member in the translation loop instead costs 90.8986 -> 89.1573; the cached
// __int64 stands.
VA(0x005bfeb0, 0x369)  // anchor-callgraph + arity (ret 4), dc 0x167c3c
void town::giveEventReward(const TTownEvent* thisEvent)
{
    __int64 mask = m_active;
    __int64 grantable = 0;
    __int64 eventBuildings = thisEvent->m_buildBuildings;
    int i;
    for (i = 0; i < MAX_BUILDING_TYPE; i++) {
        if (eventBuildings & g_bitNumber[i])
            grantable |= g_bitNumber[g_eventBuildingIds[m_type][i]];
    }
    for (i = 0; i < MAX_BUILDING_TYPE; i++) {
        if (g_townEligibleBuildMask[m_type] & g_bitNumber[i]) {
            if (grantable & g_bitNumber[i])
                mask |= s_includedBuildings[m_type][i];
        } else {
            mask |= g_bitNumber[i];
        }
    }
    if (m_dockSite == TOWN_DOCK_SITE_NONE)
        mask |= g_bitNumber[DOCK_ID];
    grantable &= ~mask;

    std::vector<type_dialog_resource> rewards;
    type_dialog_resource reward;
    for (i = 0; i < MAX_BUILDING_TYPE; i++) {
        if (grantable & g_bitNumber[i]) {
            buildBuilding(i, 0, 1);
            reward.m_resource = m_type + 0x16;
            reward.m_qualifier = i;
            rewards.push_back(reward);
            if (rewards.size() == g_rewardDialogBatch)
                showBuildingRewards(this, &rewards);
        }
    }
    if (rewards.size() > 0)
        showBuildingRewards(this, &rewards);

    for (i = 0; i < TOWN_DWELLING_COUNT; i++) {
        if (thisEvent->m_generatorBonuses[i] != 0) {
            if (m_active & g_bitNumber[DWELLING_0_UPG_ID + i]) {
                reward.m_resource = 0x15;
                m_population[i + TOWN_DWELLING_COUNT] +=
                    thisEvent->m_generatorBonuses[i];
                reward.m_qualifier = (thisEvent->m_generatorBonuses[i] << 16)
                    | static_cast<unsigned short>(
                          g_townDwellingCreatures[
                              m_type * (2 * TOWN_DWELLING_COUNT)
                              + i + TOWN_DWELLING_COUNT]);
                rewards.push_back(reward);
            } else if (m_active & g_bitNumber[DWELLING_0_ID + i]) {
                reward.m_resource = 0x15;
                m_population[i] += thisEvent->m_generatorBonuses[i];
                reward.m_qualifier = (thisEvent->m_generatorBonuses[i] << 16)
                    | static_cast<unsigned short>(
                          g_townDwellingCreatures[
                              m_type * (2 * TOWN_DWELLING_COUNT) + i]);
                rewards.push_back(reward);
            }
            if (rewards.size() == g_rewardDialogBatch)
                showCreatureRewards(this, &rewards);
        }
    }
    if (rewards.size() > 0)
        showCreatureRewards(this, &rewards);
}

VA(0x005c0220, 0x1DA)  // dc 0x167958
void showBuildingRewards(const town* thisTown,
                           std::vector<type_dialog_resource>* rewards)
{
    std::string text;
    for (int i = 0; i < rewards->size(); i++) {
        if (i > 0) {
            if (i == rewards->size() - 1)
                text += g_generalText->getText(GENERAL_TEXT_LIST_AND);
            else
                text += ", ";
        }
        text += getBuildingName(thisTown->m_type, (*rewards)[i].m_qualifier);
    }
    text = formatString(g_generalText->getText(GENERAL_TEXT_EVENT_BUILDINGS),
                         thisTown->m_name.c_str(), text.c_str());
    if (g_currentPlayer->isLocalHuman()
        && g_netLocalGamePos == thisTown->m_owner)
        extendedDialog(text.c_str(), *rewards, -1, -1, 0);
    rewards->clear();
}

// E:\gamedcs\town.cpp:1760
// Residual (99.0596%): the same DC-attested clear() repair restores the
// inlined format_string destructor and recovers the honest pre-carrier peak.
// The 218-instruction streams and all 20 branch targets agree; only the
// adjacent hidden-return-slot `lea` and count `push` around format_string
// remain transposed. The DC-attested GetArmyName call is byte-flat and kept;
// swapping count/creature declarations was flat, while a scoped named
// temporary fell to 97.3624%, so this is the closest source-compatible form.
// The creature twin emits "<count> <name>" per reward with the same separators,
// selects singular/plural by count, and drives the outer format by the first
// reward's count.
VA(0x005c0400, 0x26F)  // anchor-caller (give_event_reward), dc 0x167a8c
void showCreatureRewards(const town* thisTown,
                           std::vector<type_dialog_resource>* rewards)
{
    std::string text;
    for (int i = 0; i < rewards->size(); i++) {
        long count = (*rewards)[i].m_qualifier >> 16;
        int creature = static_cast<unsigned short>((*rewards)[i].m_qualifier);
        if (i > 0) {
            if (i == rewards->size() - 1)
                text += g_generalText->getText(GENERAL_TEXT_LIST_AND);
            else
                text += ", ";
        }
        text += formatString("%d ", count);
        text += getArmyName(creature, count);
    }
    long firstCount = (*rewards)[0].m_qualifier >> 16;
    text = formatString(g_generalText->getText(GENERAL_TEXT_EVENT_CREATURES),
                         firstCount, text.c_str(), thisTown->m_name.c_str());
    if (g_currentPlayer->isLocalHuman()
        && g_netLocalGamePos == thisTown->m_owner)
        extendedDialog(text.c_str(), *rewards, -1, -1, 0);
    rewards->clear();
}

// Forward declarations for the two bodies that follow their callers in
// link order (check_shipyard_square's definition sits after
// initialize_buildings at 0x5c0c90).
void initializeBuildings(town* currentTown, const TownExtra* townSetup);
unsigned char checkShipyardSquare(town* currentTown, long x, long y);

// Original: initialize_army; town.cpp:2017, dc 0x168330.
// Complete expands the ordinary initializer into town::initialize. Its custom
// army path additionally resolves the map format's negative random-tier IDs;
// DC's older body copied those creature IDs directly. Both use getArmy's
// canonical garrison/hero selection at every read and write.
static void initializeArmy(town* currentTown, const TownExtra* townSetup)
{
    if (townSetup->m_customArmies) {
        for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; slot++) {
            currentTown->getArmy().m_numTroops[slot] =
                townSetup->m_townArmy.m_numTroops[slot];
            if (currentTown->getArmy().m_numTroops[slot] > 0) {
                int troop = townSetup->m_townArmy.m_armies[slot];
                if (troop <= -2) {
                    int tier = (-2 - troop) / 2;
                    if (troop & 1)
                        tier += TOWN_DWELLING_COUNT;
                    troop = g_townDwellingCreatures[
                        currentTown->m_type * (2 * TOWN_DWELLING_COUNT) + tier];
                }
                currentTown->getArmy().m_armies[slot] = troop;
            } else {
                currentTown->getArmy().m_armies[slot] = -1;
            }
        }
    } else {
        for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; slot++) {
            currentTown->getArmy().m_armies[slot] = -1;
            currentTown->getArmy().m_numTroops[slot] = 0;
        }
        if (currentTown->m_owner < 0) {
            for (int tier = 0; tier < 4; tier++) {
                if (random(1, 100) <= g_townInitArmyChance[tier]) {
                    int creature = g_townDwellingCreatures[
                        currentTown->m_type * (2 * TOWN_DWELLING_COUNT) + tier];
                    currentTown->getArmy().add(creature,
                        random(g_townInitArmyLow[tier], g_townInitArmyHigh[tier]),
                        -1);
                }
            }
        }
    }
}

VA(0x005c0670, 0x24E)  // dc 0x16842c
void town::initialize(const TownExtra* townSetup)
{
    m_type = townSetup->m_townType;
    m_owner = -1;
    memset(m_generatorBonus, 0, sizeof(m_generatorBonus));
    g_game->claimTown(m_id, townSetup->m_playerOwner, 0, 0);
    initializeArmy(this, townSetup);
    initializeBuildings(this, townSetup);
    updateFullBuildingMask();
    m_spells = townSetup->m_spells;
    initializeSpells(townSetup);
}

VA(0x005c08c0, 0x3CE)  // dc 0x167ff4
void initializeBuildings(town* currentTown, const TownExtra* townSetup)
{
    int i;
    memset(currentTown->m_population, 0, sizeof(currentTown->m_population));
    currentTown->m_built = 0;
    currentTown->updateFullBuildingMask();
    currentTown->createBuilding(HALL_VILLAGE_ID);

    __int64 unavailable = 0;
    if (g_game->m_mapHeader.m_victoryCondition.m_type
            == VICTORY_CONDITION_BUILD_GRAIL
        && !g_game->m_mapHeader.m_victoryCondition.isGrailTarget(currentTown))
        unavailable = g_bitNumber[HOLY_GRAIL_ID];

    currentTown->m_dockSite = -1;
    currentTown->m_dockSiteY = -1;
    if (!(g_townEligibleBuildMask[currentTown->m_type] & g_bitNumber[DOCK_ID])
        || (!checkShipyardSquare(currentTown, currentTown->m_mapX - 1,
                                   currentTown->m_mapY + 2)
            && !checkShipyardSquare(currentTown, currentTown->m_mapX + 1,
                                      currentTown->m_mapY + 2)))
        unavailable |= g_bitNumber[DOCK_ID];

    if (townSetup->m_customBuildings) {
        for (i = 0; i < MAX_BUILDING_TYPE; i++) {
            if (townSetup->m_buildingDisabledMask & g_bitNumber[i])
                unavailable |= g_bitNumber[
                    g_eventBuildingIds[currentTown->m_type][i]];
        }
        if (unavailable & g_bitNumber[HORDE_ID])
            unavailable |= g_bitNumber[HORDE_UPG_ID];
        if (unavailable & g_bitNumber[HORDE_2_ID])
            unavailable |= g_bitNumber[HORDE_2_UPG_ID];
        currentTown->setLegalBuildings(unavailable);

        __int64 toBuild = 0;
        for (i = 0; i < MAX_BUILDING_TYPE; i++) {
            if (townSetup->m_buildingBuiltMask & g_bitNumber[i])
                toBuild |= town::s_includedBuildings[currentTown->m_type][
                        g_eventBuildingIds[currentTown->m_type][i]]
                    | g_bitNumber[g_eventBuildingIds[currentTown->m_type][i]];
        }
        for (i = 0; i < MAX_BUILDING_TYPE; i++) {
            if ((toBuild & g_bitNumber[i])
                && !(currentTown->m_available & g_bitNumber[i]))
                toBuild &= ~g_bitNumber[i];
        }
        for (i = 0; i < MAX_BUILDING_TYPE; i++) {
            if (toBuild & g_bitNumber[i])
                currentTown->createBuilding(type_building_id(i));
        }
        return;
    }

    currentTown->setLegalBuildings(unavailable);
    if (townSetup->m_hasFort)
        currentTown->createBuilding(CASTLE_FORT_ID);
    if (currentTown->m_owner >= 0) {
        currentTown->createBuilding(TAVERN_ID);
        if (currentTown->m_built & g_bitNumber[CASTLE_FORT_ID]) {
            currentTown->createBuilding(DWELLING_0_ID);
            if (random(1, 100) <= 30)
                currentTown->createBuilding(DWELLING_1_ID);
        }
    } else {
        currentTown->createBuilding(DWELLING_0_ID);
        currentTown->createBuilding(DWELLING_1_ID);
    }
}

// Retail 0x5c0ce7 reads the whole +0xc flags word before testing bits 8
// and 12 at 0x5c0ceb/0x5c0cf3. The canonical NewmapCell union now exposes
// that word as m_cellFlags; the old cell_flags_word cast wrapper predates
// its restoration and has no independent CodeView helper boundary.

VA(0x005c0c90, 0x89)  // dc 0x167f68
unsigned char checkShipyardSquare(town* currentTown, long x, long y)
{
    if (x >= 0) {
        if (x < g_mapWidth) {
            if (y >= 0) {
                if (y < g_mapHeight) {

                    NewmapCell* cell = g_game->m_worldMap.cell(x, y, currentTown->m_mapZ);
                    if (cell->m_groundSet == eTerrainWater) {
                        unsigned short flags = cell->m_cellFlags;
                        if (!(flags & 0x100)) {
                            if (!(flags & 0x1000) || cell->m_type == BOAT) {
                                currentTown->m_dockSite =
                                    static_cast<unsigned char>(x);
                                currentTown->m_dockSiteY =
                                    static_cast<unsigned char>(y);
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

// E:\gamedcs\town.cpp:2084
void town::updateFullBuildingMask()
{
    m_active = m_built;
    for (int i = 0; i < MAX_BUILDING_TYPE; i++) {
        if (hasBuilding(i, 0))
            m_active |= s_includedBuildings[m_type][i];
    }
}

// E:\gamedcs\town.cpp:2097
// DC names the short parameter building_id and the 64-bit local
// building_mask. Its source rows retain TownAlreadyBuiltOn,
// is_legal_building, CanBuildDock and get_building_mask; Complete expands
// those same source calls and matches this body exactly.
VA(0x005c0d20, 0x13D)  // anchor-global, dc 0x168504
unsigned char town::canBuild(short buildingId) const
{
    if (g_game->townAlreadyBuiltOn(m_id))
        return 0;
    if (!isLegalBuilding(type_building_id(buildingId)))
        return 0;
    if (buildingId == DOCK_ID)
        return canBuildDock();
    if (buildingId == HALL_CAPITOL_ID)
        return !g_game->m_players[m_owner].hasCapitol();

    char townType = m_type;
    __int64 requirements = g_hierarchyMask[townType][buildingId];
    __int64 buildingMask = getBuildingMask();
    if (g_game->m_isTutorial && buildingId == DWELLING_2_ID
        && townType == TOWN_CASTLE)
        requirements &= ~g_bitNumber[BLACKSMITH_ID];
    if (!(buildingMask & g_bitNumber[buildingId])
        && (buildingMask & requirements) == requirements)
        return 1;
    return 0;
}

VA(0x005c0e60, 0xC0)  // dc 0x16865c
// Complete reads the full dword parameter and its exact symbol encodes int;
// Dreamcast's older interface records short building_id.
unsigned char town::canEverBuild(int buildingId) const
{
    if (g_bitNumber[buildingId] & m_available) {
        if (buildingId == DOCK_ID)
            return m_dockSite != TOWN_DOCK_SITE_NONE;
        if (!(buildingId == HALL_CAPITOL_ID
              && g_game->m_players[m_owner].hasCapitol())) {
            __int64 requirements = g_hierarchyMask[m_type][buildingId];
            if (((m_active | m_available) & requirements) == requirements)
                return 1;
        }
    }
    return 0;
}

VA(0x005c0f20, 0x156)  // dc 0x168714
__int64 town::getBuildableMask() const
{
    __int64 activeMask = getBuildingMask();
    __int64 mask = 0;
    char townType = m_type;
    const char& castleGriffinException = g_game->m_isTutorial;
    for (int building = 0; building < MAX_BUILDING_TYPE; building++) {
        __int64 requirements = g_hierarchyMask[townType][building];
        if (castleGriffinException && building == DWELLING_2_ID
            && townType == TOWN_CASTLE)
            requirements &= ~g_bitNumber[BLACKSMITH_ID];
        if (!(activeMask & g_bitNumber[building])
            && (activeMask & requirements) == requirements)
            mask |= g_bitNumber[building];
    }
    mask &= m_available;
    if (m_dockSite == TOWN_DOCK_SITE_NONE)
        mask &= ~g_bitNumber[DOCK_ID];
    if (g_game->m_players[m_owner].hasCapitol())
        mask &= ~g_bitNumber[HALL_CAPITOL_ID];
    return mask;
}

VA(0x005c1080, 0x64)  // dc 0x1688a0
int* town::getBuildCostArray(type_building_id building) const
{
    if (building < SPECIAL_BUILDING_ID)
        return s_neutralBuildingCosts[building];
    if (building < DWELLING_0_ID)
        return s_specialBuildingCosts[m_type][building - SPECIAL_BUILDING_ID];
    return s_dwellingCosts[m_type][building - DWELLING_0_ID];
}

VA(0x005c10f0, 0x8A)  // dc 0x1688f0
void town::getBuildCost(type_building_id building, int* resources) const
{
    memcpy(resources, getBuildCostArray(building),
           NUM_RESOURCES * sizeof(int));
}

VA(0x005c1180, 0xA9)  // dc 0x168910
short town::getBuildCost(type_building_id building, EGameResource* types,
                         int* amounts) const
{
    short count = 0;
    const int* costs = getBuildCostArray(building);
    memset(amounts, 0, NUM_RESOURCES * sizeof(int));
    for (short resource = 0; resource < NUM_RESOURCES; resource++) {
        if (costs[resource] > 0) {
            types[count] = EGameResource(resource);
            amounts[count++] = costs[resource];
        }
    }
    return count;
}

VA(0x005c1230, 0x46)  // dc 0x168970
type_horde_effect* town::getHordeEffect(type_building_id building) const
{
    short slot;
    for (slot = 0; slot < TOWN_HORDE_SLOTS; slot++) {
        if (g_hordeBuildings[slot] == building)
            break;
    }
    if (slot >= TOWN_HORDE_SLOTS)
        return 0;
    return &s_constHordeEffects[m_type][slot];
}

VA(0x005c1280, 0x15)  // dc 0x1689dc
int* town::getSiloIncome() const
{
    return g_siloIncome[m_type];
}

VA(0x005c12a0, 0x39)  // dc 0x1689ec
unsigned char town::isLegalBuilding(type_building_id building) const
{
    return (g_bitNumber[building] & m_available) != 0;
}

// Original: town::set_legal_buildings; town.cpp:2291, dc 0x168a10.
void town::setLegalBuildings(__int64 disabledBuildings)
{
    m_available = g_townEligibleBuildMask[m_type] & ~disabledBuildings;
}

// Original: town::is_disabled; town.cpp:2300, dc 0x168a50.
unsigned char town::isDisabled(type_building_id building) const
{
    if (isLegalBuilding(building))
        return 0;
    return (g_townEligibleBuildMask[m_type] & g_bitNumber[building]) != 0;
}

VA(0x005c12e0, 0xC9)  // dc 0x168a98
void town::hire(hero* newHero, long playerId)
{
    playerData* player = &g_game->m_players[playerId];
    int recruitSlot;
    int heroId = newHero->m_id;
    for (recruitSlot = 0; recruitSlot < 2; recruitSlot++) {
        if (player->m_recruits[recruitSlot] == heroId)
            break;
    }

    player->m_resources[GOLD] -= g_heroGoldCost;
    hero* hiredHero = g_game->getHero(heroId);
    type_point point;
    point.m_x = m_mapX;
    point.m_y = m_mapY;
    point.m_z = m_mapZ;
    hiredHero->placeInMap(playerId, point, 1);
    giveSpells(0);
    g_game->finishTownHire(playerId, recruitSlot);
}

VA(0x005c13b0, 0x83)  // dc 0x168b54
void town::placeInMap(int heroId, long playerId, unsigned char resetFlags)
{
    hero* newHero = g_game->getHero(heroId);
    type_point point;
    point.m_x = m_mapX;
    point.m_y = m_mapY;
    point.m_z = m_mapZ;
    newHero->placeInMap(playerId, point, resetFlags);
}

VA(0x005c1440, 0xC)  // dc 0x168ba0
TTerrainType town::getNativeTerrain() const
{
    return townManager::getNativeTerrain(m_type);
}

VA(0x005c1450, 0xC)  // dc 0x168bb8
const char* town::getTypeName() const
{
    return townManager::getTownTypeName(m_type);
}

// Original: town::get_army; town.cpp:2375, dc 0x168bd0.
// This ordinary non-const twin returns the same selected army address as the
// const overload. Complete callers of both interfaces share 0x5c1460.
armyGroup& town::getArmy()
{
    if (m_garrisonHeroId < 0)
        return m_garrison;
    return g_game->getHero(m_garrisonHeroId)->m_army;
}

VA(0x005c1460, 0x38)  // dc 0x168bf8
const armyGroup& town::getArmy() const
{
    if (m_garrisonHeroId < 0)
        return m_garrison;
    return g_game->getHero(m_garrisonHeroId)->m_army;
}

VA(0x005c14a0, 0x19)  // dc 0x168c20
int town::upgradedDwellingID(int id)
{
    if (id == HORDE_ID)
        return HORDE_UPG_ID;
    if (id == HORDE_2_ID)
        return HORDE_2_UPG_ID;
    return id + TOWN_DWELLING_COUNT;
}

// E:\gamedcs\town.cpp:2476, static and inlined at all three of its call
// sites by /Ob2 - the Dreamcast keeps it out of line and names its two
// parameters. `resource` is bound by REFERENCE, which is why retail
// re-reads the vector's _First between every one of the seven columns
// instead of holding it in a register.
static void initializeBuildingCosts(int* costs,
                                    const std::vector<char*>& resource)
{
    costs[0] = atoi(resource[0]);
    costs[1] = atoi(resource[1]);
    costs[2] = atoi(resource[2]);
    costs[3] = atoi(resource[3]);
    costs[4] = atoi(resource[4]);
    costs[5] = atoi(resource[5]);
    costs[6] = atoi(resource[6]);
}

// The three build-cost tables town.h declares, defined here because this
// is the compiland that fills them - which settles the "not yet located in
// retail" caveat that header carried. Their .bss extents chain exactly
// (0x6a80f8 + 17*7*4 = 0x6a82dc, + 9*9*7*4 = 0x6a8bb8 = included_buildings,
// + 9*44*8 = 0x6a9818, + 9*14*7*4 = 0x6aa5e0).
DATA(0x006a80f8)
int town::s_neutralBuildingCosts[SPECIAL_BUILDING_ID][NUM_RESOURCES];
DATA(0x006a82dc)
int town::s_specialBuildingCosts[9][9][NUM_RESOURCES];
DATA(0x006a9818)
int town::s_dwellingCosts[9][14][NUM_RESOURCES];

VA(0x005c14c0, 0x1F6)
unsigned char town::initializeBuildingCostsTables()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00688fb4, townBuildingSpreadsheetName, "building.txt"));
    if (!sheet)
        return 0;

    int row = 2;
    for (int type = 0; type < TOWN_TYPE_COUNT; ++type) {
        row += 2;
        for (int special = 0; special < 9; ++special) {
            initializeBuildingCosts(s_specialBuildingCosts[type][special],
                                    sheet->getRow(row));
            ++row;
        }
    }

    row += 3;
    for (int neutral = 0; neutral < SPECIAL_BUILDING_ID; ++neutral) {
        initializeBuildingCosts(s_neutralBuildingCosts[neutral], sheet->getRow(row));
        ++row;
    }

    row += 2;
    for (int dwellingType = 0; dwellingType < TOWN_TYPE_COUNT; ++dwellingType) {
        row += 2;
        for (int dwelling = 0; dwelling < 14; ++dwelling) {
            initializeBuildingCosts(s_dwellingCosts[dwellingType][dwelling],
                                    sheet->getRow(row));
            ++row;
        }
    }

    sheet->dispose();
    return 1;
}

// COMDAT pairing: bitset<48>::set(pos, bool), agreement 1.000 at an exactly
// equal 96-byte extent.
VA_COMPGEN(0x00506780, 0x60, BITSET_SET, Bitset48)
