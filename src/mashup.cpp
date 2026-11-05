// HoMM2-vs-HoMM3 battle mashup driver (not part of the retail game).
#include "va.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mashup.h"
#include "h2core_api.h"

#include "advmgr.h"
#include "army.h"
#include "creaturetype.h"
#include "monframeinfo.h"
#include "platform.h"
#include "resourcemanager.h"
#include "armygrp.h"
#include "cmbtmgr.h"
#include "combatwindow.h"
#include "exec.h"
#include "game.h"
#include "hero.h"
#include "kb.h"
#include "mousemgr.h"
#include "winmgr.h"
#include "soundmgr.h"
#include "csprite.h"
#include "misc.h"
#include "mashup_h2fx.h"

bool g_mashupMode = 0;
bool g_mashupAutoClose = 0;

// Battle log next to the executable (mashup.log), flushed per line so it
// survives a crash.
void mashupLog(const char* format, ...)
{
    static FILE* s_log = 0;
    if (!s_log) {
        // MASHUP_LOG names the log when two runs share the directory.
        const char* name = getenv("MASHUP_LOG");
        s_log = fopen(name && *name ? name : "mashup.log", "w");
    }
    if (!s_log)
        return;
    // Milliseconds since the first line, to line the log up with a recording.
    static DWORD s_start = GetTickCount();
    fprintf(s_log, "[%7lu] ", GetTickCount() - s_start);
    va_list args;
    va_start(args, format);
    vfprintf(s_log, format, args);
    va_end(args);
    fputc('\n', s_log);
    fflush(s_log);
}

namespace {

// Logs the faulting address and the frame-pointer call chain (the game is
// built with /Oy-); map them with mashup/tools/addr2sym.py.
LONG WINAPI logCrash(EXCEPTION_POINTERS* info)
{
    CONTEXT* context = info->ContextRecord;
    mashupLog("crash: code %08lx at %08lx", info->ExceptionRecord->ExceptionCode,
              context->Eip);
    unsigned long* frame = (unsigned long*)context->Ebp;
    for (int depth = 0; depth < 24 && frame; ++depth) {
        if (IsBadReadPtr(frame, 8))
            break;
        mashupLog("  called from %08lx", frame[1]);
        frame = (unsigned long*)frame[0];
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

const char MASHUP_MAP_DIR[] = "maps";
const char MASHUP_MAP_FILE[] = "Arrogance.h3m";

// Number of weeks of town growth each side has saved up.
const int MASHUP_WEEKS = 100;

// Christian's record is borrowed as the shell for every HoMM2 hero.
const int MASHUP_H2_HERO = 6;

const int MASHUP_H3_PLAYER = 0;
const int MASHUP_H2_PLAYER = 1;

struct MashupStack {
    TCreatureType m_type;
    int m_weeklyGrowth;
};

// Fully built HoMM3 Castle: base growth, the Castle building's +100%, and
// the Griffin Bastion's +3 griffins (town.cpp getCastleGrowthBonus /
// getHordeBonus). Every dwelling is upgraded.
const MashupStack H3_CASTLE_ARMY[armyGroup::ARMY_GROUP_SLOT_COUNT] = {
    { TCreatureType(1), 14 * 2 },       // Halberdier
    { TCreatureType(3), 9 * 2 },        // Marksman
    { TCreatureType(5), 7 * 2 + 3 },    // Royal Griffin
    { TCreatureType(7), 4 * 2 },        // Crusader
    { TCreatureType(9), 3 * 2 },        // Zealot
    { TCreatureType(11), 2 * 2 },       // Champion
    { TCreatureType(13), 1 * 2 },       // Archangel
};

// Fully built HoMM2 Knight castle, all dwellings upgraded: base growth plus
// the Well's +2 (HoMM2 game::PerWeek; the Castle adds no growth). HoMM2
// heroes lead five stacks, so the Farm-boosted Peasants stay home. The
// growth numbers are read from HoMM2's own monster table at startup.
const int H2_WELL_BONUS = 2;
const int H2_ARMY_SIZE = 5;
const int H2_KNIGHT_ARMY_TYPES[H2_ARMY_SIZE] = {
    2,   // Ranger
    4,   // Veteran Pikeman
    6,   // Master Swordsman
    8,   // Champion
    10,  // Crusader
};

// Fully built HoMM3 Necropolis: the Castle's +100% and the Unearthed
// Graves' +6 skeletons; every dwelling upgraded.
const MashupStack H3_NECROPOLIS_ARMY[armyGroup::ARMY_GROUP_SLOT_COUNT] = {
    { TCreatureType(57), 12 * 2 + 6 },  // Skeleton Warrior
    { TCreatureType(59), 8 * 2 },       // Zombie
    { TCreatureType(61), 7 * 2 },       // Wraith
    { TCreatureType(63), 4 * 2 },       // Vampire Lord
    { TCreatureType(65), 3 * 2 },       // Power Lich
    { TCreatureType(67), 2 * 2 },       // Dread Knight
    { TCreatureType(69), 1 * 2 },       // Ghost Dragon
};

// Fully built HoMM3 Dungeon: the Castle's +100% and the Mushroom Rings'
// +7 troglodytes; every dwelling upgraded.
const MashupStack H3_DUNGEON_ARMY[armyGroup::ARMY_GROUP_SLOT_COUNT] = {
    { TCreatureType(71), 14 * 2 + 7 },  // Infernal Troglodyte
    { TCreatureType(73), 8 * 2 },       // Harpy Hag
    { TCreatureType(75), 7 * 2 },       // Evil Eye
    { TCreatureType(77), 4 * 2 },       // Medusa Queen
    { TCreatureType(79), 3 * 2 },       // Minotaur King
    { TCreatureType(81), 2 * 2 },       // Scorpicore
    { TCreatureType(83), 1 * 2 },       // Black Dragon
};

// Fully built HoMM2 Necromancer and Warlock castles, tier 1 left home as
// with the Knight.
const int H2_NECROMANCER_ARMY_TYPES[H2_ARMY_SIZE] = {
    49,  // Mutant Zombie
    51,  // Royal Mummy
    53,  // Vampire Lord
    55,  // Power Lich
    56,  // Bone Dragon
};
const int H2_WARLOCK_ARMY_TYPES[H2_ARMY_SIZE] = {
    30,  // Gargoyle
    31,  // Griffin
    33,  // Minotaur King
    34,  // Hydra
    37,  // Black Dragon
};

// Who fights whom (MASHUP_MATCHUP in the environment: castle, the default,
// necropolis or dungeon). Each pairs a HoMM3 town with its HoMM2
// counterpart under heroes of the same name where both games have one.
struct MashupMatchup {
    const char* m_name;
    int m_h3Hero;                  // HoMM3 hero record
    int m_h3Town;                  // whose mage guild fills his spellbook
    const MashupStack* m_h3Army;
    int m_h2HeroId;                // h2core_make_hero
    const int* m_h2ArmyTypes;
    int m_h2CombatHeroSprite;      // offset from COMBAT_HERO_SPRITE_H2
    const char* m_h2LargePortrait;
    const char* m_h2SmallPortrait;
    bool m_armageddonSetPiece;     // both heroes open round 3 with Armageddon
};

const MashupMatchup MASHUP_MATCHUPS[] = {
    // Lord Haart vs Lord Kilburn
    { "castle", 4, TOWN_CASTLE, H3_CASTLE_ARMY, 0, H2_KNIGHT_ARMY_TYPES, 0,
      "HPLH2KIL.pcx", "HPSH2KIL.pcx", 1 },
    // Sandro vs Sandro
    { "necropolis", 90, TOWN_NECROPOLIS, H3_NECROPOLIS_ARMY, 52, H2_NECROMANCER_ARMY_TYPES, 1,
      "HPLH2SAN.pcx", "HPSH2SAN.pcx", 0 },
    // Alamar vs Alamar
    { "dungeon", 104, TOWN_DUNGEON, H3_DUNGEON_ARMY, 28, H2_WARLOCK_ARMY_TYPES, 2,
      "HPLH2ALA.pcx", "HPSH2ALA.pcx", 0 },
};

const MashupMatchup& mashupMatchup()
{
    static const MashupMatchup* s_matchup = 0;
    if (!s_matchup) {
        const char* name = getenv("MASHUP_MATCHUP");
        s_matchup = &MASHUP_MATCHUPS[0];
        for (int i = 0; i < int(sizeof(MASHUP_MATCHUPS) / sizeof(MASHUP_MATCHUPS[0])); ++i) {
            if (name && !_stricmp(name, MASHUP_MATCHUPS[i].m_name))
                s_matchup = &MASHUP_MATCHUPS[i];
        }
        mashupLog("mashup: matchup %s", s_matchup->m_name);
    }
    return *s_matchup;
}

// The user's handicap: HoMM2 has no second upgrade tier and fewer stacks,
// so the HoMM2 army fields a multiple of its 100-week growth
// (MASHUP_H2_ARMY in the environment, default 3).
int h2ArmyMultiplier()
{
    const char* text = getenv("MASHUP_H2_ARMY");
    int multiplier = text && *text ? atoi(text) : 3;
    return multiplier > 0 ? multiplier : 1;
}

void fillArmy(armyGroup& army, const MashupStack* stacks, int count)
{
    for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        army.m_armyTypes[slot] = TCreatureType(-1);
        army.m_numTroops[slot] = 0;
    }
    for (int i = 0; i < count; ++i) {
        army.m_armyTypes[i] = stacks[i].m_type;
        army.m_numTroops[i] = stacks[i].m_weeklyGrowth * MASHUP_WEEKS;
    }
}

bool loadMashupMap()
{
    strcpy(g_game->m_setup.m_path, MASHUP_MAP_DIR);
    strcpy(g_game->m_setup.m_filename, MASHUP_MAP_FILE);
    g_game->resetGame(0, 0, 0);

    playerData& human = g_game->m_players[MASHUP_H3_PLAYER];
    human.m_isLocal = 1;
    human.m_isHuman = 1;
    strcpy(human.m_name, "HoMM3");
    for (int i = 0; i < 8; ++i) {
        g_newMapStartingBonus[i] = 3;
        g_game->m_setup.m_startingBonus[i] = 3;
        g_game->m_setup.m_startingHero[i] = -1;
    }
    g_localGamePos = MASHUP_H3_PLAYER;
    g_game->setupFirstPlayer();
    return g_game->newMap(g_game->m_setup.m_path, g_game->m_setup.m_filename,
                          0, -1) != 0;
}

// Any land cell will do: the battlefield background follows its terrain.
type_point battleLocation()
{
    playerData& human = g_game->m_players[MASHUP_H3_PLAYER];
    for (int i = 0; i < human.m_numHeroes; ++i) {
        if (human.m_heroes[i] >= 0)
            return g_game->getHero(human.m_heroes[i])->getLocation();
    }
    type_point point;
    point.m_x = 10;
    point.m_y = 10;
    point.m_z = 0;
    return point;
}

hero* prepareHero(int id, int owner, const MashupStack* stacks, int count)
{
    hero* h = g_game->getHero(id);
    h->m_owner = owner;
    // No artifacts and no war machines (HoMM2 has none); the spellbook
    // (slot 17) stays.
    for (int slot = 0; slot < 19; ++slot) {
        if (slot != 17)
            h->removeArtifact(slot);
    }
    fillArmy(h->m_army, stacks, count);
    return h;
}


// ---------------------------------------------------------------------------
// h2core.dll: HoMM2's own combat code, mirrored onto this battle.

struct H2Core {
    HMODULE m_module;
    H2CoreVersionFn m_version;
    H2CoreInitFn m_init;
    H2CoreCreatureFn m_creature;
    H2CoreChooseActionFn m_chooseAction;
    H2CoreDamageFn m_damage;
    H2CoreMakeHeroFn m_makeHero;
    H2CoreCastFn m_cast;
    H2CoreMoraleLuckFn m_moraleLuck;
    H2CoreSetHeroSpellsFn m_setHeroSpells;
    H2CoreSetHeroPrimaryFn m_setHeroPrimary;
    H2CoreCreatureCastFn m_creatureCast;
};

H2Core g_h2core;

// HoMM3 AI values per HoMM2 fight value point (see registerCreatures).
double g_aiValuePerH2FightValue = 1.0;

// Creature attack and defense run on different scales in the two games
// (HoMM3's top units sit near 30, HoMM2's near 15). A HoMM3 point is worth
// this many HoMM2 points, measured over both games' whole rosters; each
// game's formula sees the other game's creatures converted to its scale.
double g_h3AttackPerH2 = 1.0;
double g_h3DefensePerH2 = 1.0;

// HoMM2 creatures' own attack and defense (HoMM2 ids).
int g_h2NativeAttack[MASHUP_H2_CREATURE_COUNT];
int g_h2NativeDefense[MASHUP_H2_CREATURE_COUNT];

// MASHUP_SEED in the environment fixes every random choice that is not
// timing-dependent, so two runs (one watched, one recorded) fight the same
// battle; without it each run differs.
unsigned int mashupSeed()
{
    static unsigned int s_seed = 0;
    if (!s_seed) {
        const char* text = getenv("MASHUP_SEED");
        s_seed = text && *text ? unsigned(strtoul(text, 0, 10)) : GetTickCount();
        if (!s_seed)
            s_seed = 1;
        mashupLog("mashup: seed %u", s_seed);
    }
    return s_seed;
}

bool loadH2Core()
{
    g_h2core.m_module = LoadLibraryA("h2core.dll");
    if (!g_h2core.m_module)
        return 0;
    g_h2core.m_version = (H2CoreVersionFn)GetProcAddress(g_h2core.m_module, "h2core_version");
    g_h2core.m_init = (H2CoreInitFn)GetProcAddress(g_h2core.m_module, "h2core_init");
    g_h2core.m_creature = (H2CoreCreatureFn)GetProcAddress(g_h2core.m_module, "h2core_creature");
    g_h2core.m_chooseAction =
        (H2CoreChooseActionFn)GetProcAddress(g_h2core.m_module, "h2core_choose_action");
    g_h2core.m_damage = (H2CoreDamageFn)GetProcAddress(g_h2core.m_module, "h2core_damage");
    g_h2core.m_makeHero = (H2CoreMakeHeroFn)GetProcAddress(g_h2core.m_module, "h2core_make_hero");
    g_h2core.m_cast = (H2CoreCastFn)GetProcAddress(g_h2core.m_module, "h2core_cast");
    g_h2core.m_moraleLuck =
        (H2CoreMoraleLuckFn)GetProcAddress(g_h2core.m_module, "h2core_morale_luck");
    g_h2core.m_setHeroSpells =
        (H2CoreSetHeroSpellsFn)GetProcAddress(g_h2core.m_module, "h2core_set_hero_spells");
    g_h2core.m_setHeroPrimary =
        (H2CoreSetHeroPrimaryFn)GetProcAddress(g_h2core.m_module, "h2core_set_hero_primary");
    g_h2core.m_creatureCast =
        (H2CoreCreatureCastFn)GetProcAddress(g_h2core.m_module, "h2core_creature_cast");
    if (!g_h2core.m_version || !g_h2core.m_init || !g_h2core.m_creature
        || !g_h2core.m_chooseAction || !g_h2core.m_damage || !g_h2core.m_makeHero
        || !g_h2core.m_cast || !g_h2core.m_moraleLuck || !g_h2core.m_setHeroSpells
        || !g_h2core.m_setHeroPrimary || !g_h2core.m_creatureCast)
        return 0;
    if (g_h2core.m_version() != H2CORE_API_VERSION)
        return 0;
    return g_h2core.m_init(mashupSeed()) != 0;
}

// HoMM2 speeds count hexes on its 13-column field (11 playable columns);
// HoMM3's has 15. Scale so a creature crosses the same share of the field.
int scaleH2Speed(int speed)
{
    return (speed * 15 + 5) / 11;
}

// The HoMM3 creature standing in for a HoMM2 one when borrowing animation
// timings and missiles.
const TCreatureType H2_ANIMATION_DONOR[MASHUP_H2_CREATURE_COUNT] = {
    TCreatureType(139),   // Peasant          <- Peasant
    TCreatureType(2),     // Archer           <- Archer
    TCreatureType(3),     // Ranger           <- Marksman
    TCreatureType(0),     // Pikeman          <- Pikeman
    TCreatureType(1),     // Veteran Pikeman  <- Halberdier
    TCreatureType(6),     // Swordsman        <- Swordsman
    TCreatureType(7),     // Master Swordsman <- Crusader
    TCreatureType(10),    // Cavalry          <- Cavalier
    TCreatureType(11),    // Champion         <- Champion
    TCreatureType(7),     // Paladin          <- Crusader
    TCreatureType(7),     // Crusader         <- Crusader
    TCreatureType(84),    // Goblin           <- Goblin
    TCreatureType(88),    // Orc              <- Orc
    TCreatureType(89),    // Orc Chief        <- Orc Chieftain
    TCreatureType(86),    // Wolf             <- Wolf Rider
    TCreatureType(90),    // Ogre             <- Ogre
    TCreatureType(91),    // Ogre Lord        <- Ogre Mage
    TCreatureType(144),   // Troll            <- Troll
    TCreatureType(144),   // War Troll        <- Troll
    TCreatureType(94),    // Cyclops          <- Cyclops
    TCreatureType(119),   // Sprite           <- Sprite
    TCreatureType(16),    // Dwarf            <- Dwarf
    TCreatureType(17),    // Battle Dwarf     <- Battle Dwarf
    TCreatureType(18),    // Elf              <- Wood Elf
    TCreatureType(19),    // Grand Elf        <- Grand Elf
    TCreatureType(34),    // Druid            <- Mage
    TCreatureType(35),    // Greater Druid    <- Arch Mage
    TCreatureType(24),    // Unicorn          <- Unicorn
    TCreatureType(131),   // Phoenix          <- Phoenix
    TCreatureType(14),    // Centaur          <- Centaur
    TCreatureType(30),    // Gargoyle         <- Stone Gargoyle
    TCreatureType(4),     // Griffin          <- Griffin
    TCreatureType(78),    // Minotaur         <- Minotaur
    TCreatureType(79),    // Minotaur King    <- Minotaur King
    TCreatureType(110),   // Hydra            <- Hydra
    TCreatureType(26),    // Green Dragon     <- Green Dragon
    TCreatureType(82),    // Red Dragon       <- Red Dragon
    TCreatureType(83),    // Black Dragon     <- Black Dragon
    TCreatureType(138),   // Halfling         <- Halfling
    TCreatureType(140),   // Boar             <- Boar
    TCreatureType(33),    // Iron Golem       <- Iron Golem
    TCreatureType(33),    // Steel Golem      <- Iron Golem
    TCreatureType(92),    // Roc              <- Roc
    TCreatureType(34),    // Mage             <- Mage
    TCreatureType(35),    // Archmage         <- Arch Mage
    TCreatureType(40),    // Giant            <- Giant
    TCreatureType(41),    // Titan            <- Titan
    TCreatureType(56),    // Skeleton         <- Skeleton
    TCreatureType(58),    // Zombie           <- Walking Dead
    TCreatureType(59),    // Mutant Zombie    <- Zombie
    TCreatureType(141),   // Mummy            <- Mummy
    TCreatureType(141),   // Royal Mummy      <- Mummy
    TCreatureType(62),    // Vampire          <- Vampire
    TCreatureType(63),    // Vampire Lord     <- Vampire Lord
    TCreatureType(64),    // Lich             <- Lich
    TCreatureType(65),    // Power Lich       <- Power Lich
    TCreatureType(68),    // Bone Dragon      <- Bone Dragon
    TCreatureType(143),   // Rogue            <- Rogue
    TCreatureType(142),   // Nomad            <- Nomad
    TCreatureType(61),    // Ghost            <- Wraith
    TCreatureType(36),    // Genie            <- Genie
    TCreatureType(76),    // Medusa           <- Medusa
    TCreatureType(113),   // Earth Elemental  <- Earth Elemental
    TCreatureType(112),   // Air Elemental    <- Air Elemental
    TCreatureType(114),   // Fire Elemental   <- Fire Elemental
    TCreatureType(115),   // Water Elemental  <- Water Elemental
};

// HoMM2 creature ids the mashup treats specially (HoMM2 KB_TYPES.h).
enum {
    H2_CREATURE_GRIFFIN = 31,
    H2_CREATURE_BLACK_DRAGON = 37,
    H2_CREATURE_MUMMY = 50,
    H2_CREATURE_ROYAL_MUMMY = 51,
    H2_CREATURE_VAMPIRE_LORD = 53,
    H2_CREATURE_LICH = 54,
    H2_CREATURE_POWER_LICH = 55
};
const int H2_MONSTER_FLAG_BREATH = 0x8;  // HoMM2 MONSTER_FLAGS_BREATH_ATTACK

// HoMM2 abilities the HoMM3 engine already performs, by HoMM2's own lists
// (army::DoAttack / SpecialAttack): the Ranger and the Elves shoot twice;
// the Wolf, Paladin and Crusader strike twice; the Hydra strikes every
// adjacent enemy and none strikes back; nobody strikes back at Sprites,
// Vampires or Rogues; dragons breathe through two hexes; the undead have no
// morale and no mind. The rest (the Griffin's endless retaliation, the
// Lich's cloud, the Vampire Lord's drain, the Mummies' curse, the Black
// Dragon's spell immunity) are mashup hooks below.
unsigned int h2HostAttributes(int h2Type, int h2Flags)
{
    unsigned int attributes = 0;
    if (h2Flags & H2CORE_FLAG_UNDEAD)
        attributes |= creatureUndead | creatureNoMorale | creatureImmuneToMindSpells;
    else
        attributes |= creatureAlive;
    if (h2Flags & H2CORE_FLAG_WIDE)
        attributes |= creatureDoubleWide;
    if (h2Flags & H2CORE_FLAG_FLYING)
        attributes |= creatureFlyingArmy;
    if (h2Flags & H2CORE_FLAG_SHOOTER)
        attributes |= creatureShootingArmy;
    if (h2Flags & H2_MONSTER_FLAG_BREATH)
        attributes |= creatureHasExtendedAttack;
    switch (h2Type) {
    case 2:   // Ranger
    case 9:   // Paladin
    case 10:  // Crusader
    case 14:  // Wolf
    case 23:  // Elf
    case 24:  // Grand Elf
        attributes |= creatureTwoAttacks;
        break;
    case 34:  // Hydra
        attributes |= creatureMultiHeaded | creatureFreeAttack;
        break;
    case 20:  // Sprite
    case 52:  // Vampire
    case 53:  // Vampire Lord
    case 57:  // Rogue
        attributes |= creatureFreeAttack;
        break;
    }
    if (h2Type == 35 || h2Type == 36 || h2Type == 37 || h2Type == 56)
        attributes |= creatureDragon;  // HoMM2 IS_DRAGON_CREATURE
    return attributes;
}

// HoMM2 keeps creature names in lower case ("veteran pikeman").
char* titleCase(const char* name)
{
    char* result = strdup(name);
    bool wordStart = 1;
    for (char* c = result; *c; ++c) {
        if (wordStart && *c >= 'a' && *c <= 'z')
            *c = char(*c - 'a' + 'A');
        wordStart = *c == ' ';
    }
    return result;
}

void registerCreatures()
{
    // Scale HoMM2 fight values to HoMM3 AI values over the matching Castle
    // and Knight units, so each AI weighs foreign stacks on its own scale.
    static const int pairs[][2] = {
        { 0, 3 }, { 1, 4 }, { 2, 1 }, { 3, 2 },
        { 6, 5 }, { 7, 6 }, { 10, 7 }, { 11, 8 },
    };
    double ratio = 0;
    int pairCount = sizeof(pairs) / sizeof(pairs[0]);
    for (int p = 0; p < pairCount; ++p) {
        H2CoreCreature h2;
        g_h2core.m_creature(pairs[p][1], &h2);
        ratio += double(g_creatureTypeTraits[pairs[p][0]].m_aiValue) / h2.fightValue;
    }
    g_aiValuePerH2FightValue = ratio / pairCount;

    double h3Attack = 0, h3Defense = 0, h2Attack = 0, h2Defense = 0;
    int h3Count = 0, h2Count = 0;
    for (int type = 0; type < RETAIL_CREATURE_TYPE_COUNT; ++type) {
        const TCreatureTypeTraits& traits = g_creatureTypeTraits[type];
        if (traits.m_hitPoints <= 0 || (traits.m_attributes & g_ctaSiegeWeapon))
            continue;
        h3Attack += traits.m_attackSkill;
        h3Defense += traits.m_defenseSkill;
        ++h3Count;
    }
    for (int rosterType = 0; ; ++rosterType) {
        H2CoreCreature h2;
        if (!g_h2core.m_creature(rosterType, &h2))
            break;
        h2Attack += h2.attack;
        h2Defense += h2.defense;
        ++h2Count;
    }
    g_h3AttackPerH2 = (h3Attack / h3Count) / (h2Attack / h2Count);
    g_h3DefensePerH2 = (h3Defense / h3Count) / (h2Defense / h2Count);
    mashupLog("mashup: stat scale over %d HoMM3 / %d HoMM2 creatures: attack x%.2f, defense x%.2f",
              h3Count, h2Count, g_h3AttackPerH2, g_h3DefensePerH2);

    for (int h2Type = 0; h2Type < MASHUP_H2_CREATURE_COUNT; ++h2Type) {
        H2CoreCreature h2;
        if (!g_h2core.m_creature(h2Type, &h2))
            continue;
        int id = MASHUP_H2_CREATURE_FIRST + h2Type;
        TCreatureTypeTraits& traits = mutableCreatureTypeTraits(id);
        traits.m_name = titleCase(h2.name);
        traits.m_pluralName = titleCase(h2.pluralName);
        traits.m_specialAbility = "HoMM2 rules; attack and defense on HoMM3's scale";
        for (int r = 0; r < 7; ++r)
            traits.m_cost[r] = 0;
        traits.m_cost[6] = h2.cost;
        traits.m_aiValue = int(h2.fightValue * g_aiValuePerH2FightValue + 0.5);
        traits.m_baseFightValue = traits.m_aiValue;
        traits.m_growthRate = h2.growth;
        traits.m_hordeGrowthRate = 0;
        traits.m_hitPoints = h2.hitPoints;
        traits.m_speed = scaleH2Speed(h2.speed);
        // Registered on HoMM3's scale: HoMM3's formula and AI see a HoMM2
        // creature as strong, relative to HoMM3, as it is in HoMM2.
        g_h2NativeAttack[h2Type] = h2.attack;
        g_h2NativeDefense[h2Type] = h2.defense;
        traits.m_attackSkill = int(h2.attack * g_h3AttackPerH2 + 0.5);
        traits.m_defenseSkill = int(h2.defense * g_h3DefensePerH2 + 0.5);
        traits.m_damageLowBound = h2.damageMin;
        traits.m_damageHighBound = h2.damageMax;
        traits.m_numShots = h2.shots;
        traits.m_hasSpell = 0;
        traits.m_attributes = h2HostAttributes(h2Type, h2.attributes);
        mutableMonFrameInfo(id) = g_monFrameInfo[H2_ANIMATION_DONOR[h2Type]];
    }
}

// HoMM2 spell influences (army::m_spellInfluence slots) and the HoMM3
// status effect that carries each one on this battlefield. HoMM2's Shield
// halves ranged damage, which is HoMM3's Air Shield.
const int H2_INFLUENCE_TO_H3_SPELL[H2CORE_SPELL_INFLUENCES] = {
    SPELL_HASTE, SPELL_SLOW, SPELL_BLIND, SPELL_BLESS, SPELL_CURSE,
    SPELL_BERSERK, SPELL_PARALYZE, SPELL_HYPNOTIZE, SPELL_SLAYER,
    SPELL_BLOODLUST, SPELL_AIR_SHIELD, SPELL_STONE, SPELL_ANTI_MAGIC,
    SPELL_STONE_SKIN, SPELL_STONE_SKIN,
};


// How HoMM2 shows each of its combat spells (HoMM2 spell ids), converted by
// mashup/h2assets/build_h2_assets.py: the effect (an index into
// g_spellEffectTraits, -1 for none), the missile sprite and the sound.
enum EH2SpellVisualKind {
    H2_VISUAL_STACK = 0,   // the effect plays on every affected stack
    H2_VISUAL_AREA = 1,    // once, on the target hex
    H2_VISUAL_BOLT = 2,    // a missile or bolt to the target, then the effect
    H2_VISUAL_SCREEN = 3   // the whole field; the effect on affected stacks
};

struct H2SpellVisual {
    int m_kind;
    int m_effect;
    const char* m_missile;
    const char* m_sound;
};

const H2SpellVisual H2_SPELL_VISUALS[H2CORE_SPELLS] = {
    { 1, 83, 0, "H2SFIREBALL" },  // 0 Fireball
    { 1, 84, 0, "H2SFIREBALL" },  // 1 Fireblast
    { 2, 85, 0, "H2SLIGHTBLT" },  // 2 Lightning Bolt
    { 2, 85, 0, "H2SCHAINLTE" },  // 3 Chain Lightning
    { 0, -1, 0, "H2STELPTOUT" },  // 4 Teleport
    { 0, 86, 0, "H2SCURE" },  // 5 Cure
    { 0, 86, 0, "H2SMASSCURE" },  // 6 Mass Cure
    { 0, 87, 0, "H2SRESURECT" },  // 7 Resurrect
    { 0, 87, 0, "H2SRESURTRU" },  // 8 True Resurrect
    { 0, 88, 0, "H2SHASTE" },  // 9 Haste
    { 0, 88, 0, "H2SMASSHAST" },  // 10 Mass Haste
    { 0, 89, 0, "H2SSLOW" },  // 11 Slow
    { 0, 89, 0, "H2SMASSSLOW" },  // 12 Mass Slow
    { 0, 90, 0, "H2SBLIND" },  // 13 Blind
    { 0, 91, 0, "H2SBLESS" },  // 14 Bless
    { 0, 91, 0, "H2SMASSBLES" },  // 15 Mass Bless
    { 0, 92, 0, "H2SSTONSKIN" },  // 16 Stoneskin
    { 0, 93, 0, "H2SSTELSKIN" },  // 17 Steelskin
    { 0, 94, 0, "H2SCURSE" },  // 18 Curse
    { 0, 94, 0, "H2SMASSCURS" },  // 19 Mass Curse
    { 3, 95, 0, "H2SMASSCURS" },  // 20 Holy Word
    { 3, 95, 0, "H2SMASSCURS" },  // 21 Holy Shout
    { 0, 96, 0, "H2SANTIMAGK" },  // 22 Anti-Magic
    { 0, 97, 0, "H2SDIPMAGK" },  // 23 Dispel Magic
    { 0, 97, 0, "H2SDIPMAGK" },  // 24 Mass Dispel
    { 2, -1, "H2SE39.DEF", "H2SMAGCAROW" },  // 25 Magic Arrow
    { 0, 98, 0, "H2SBERZERK" },  // 26 Berserker
    { 3, -1, 0, "H2SARMGEDN" },  // 27 Armageddon
    { 3, 99, 0, "H2SSTORM" },  // 28 Elemental Storm
    { 1, 100, 0, "H2SMETEOR1" },  // 29 Meteor Shower
    { 0, 101, 0, "H2SPARALIZE" },  // 30 Paralyze
    { 0, 102, 0, "H2SHYPNOTIZ" },  // 31 Hypnotize
    { 2, 103, "H2SE37.DEF", "H2SCOLDRAY" },  // 32 Cold Ray
    { 1, 104, 0, "H2SCOLDRING" },  // 33 Cold Ring
    { 2, -1, "H2SE38.DEF", "H2SDISRUPTR" },  // 34 Disrupting Ray
    { 3, 105, 0, "H2SMNRDEATH" },  // 35 Death Ripple
    { 3, 105, 0, "H2SMNRDEATH" },  // 36 Death Wave
    { 0, 106, 0, "H2SDRGNSLAY" },  // 37 Dragon Slayer
    { 0, -1, 0, "H2SBLOODLUS" },  // 38 Blood Lust
    { 0, 87, 0, "H2SRESURECT" },  // 39 Animate Dead
    { 0, -1, 0, "H2SMIRRORIM" },  // 40 Mirror Image
    { 0, 107, 0, "H2SSHIELD" },  // 41 Shield
    { 0, 107, 0, "H2SSHIELD" },  // 42 Mass Shield
    { 0, -1, 0, "H2SSUMNELM" },  // 43 Summon Earth Elemental
    { 0, -1, 0, "H2SSUMNELM" },  // 44 Summon Air Elemental
    { 0, -1, 0, "H2SSUMNELM" },  // 45 Summon Fire Elemental
    { 0, -1, 0, "H2SSUMNELM" },  // 46 Summon Water Elemental
    { 1, 108, 0, "H2SERTHQUAK" },  // 47 Earthquake
};

// HoMM2's missile directions (degrees, frame order), shared by its missiles.
const float H2_MISSILE_ANGLES[] = { 90.0f, 68.5f, 45.0f, 20.8f, 0.0f, -20.8f, -45.0f, -68.5f, -90.0f };

// HoMM2 SpellType ids drawn in code.
enum {
    H2_SPELL_LIGHTNING_BOLT = 2,
    H2_SPELL_CHAIN_LIGHTNING = 3,
    H2_SPELL_ARMAGEDDON = 27
};

int toH2Influence(int spell)
{
    switch (spell) {
    case SPELL_SLAYER: return 8;
    case SPELL_AIR_SHIELD: return 10;
    case SPELL_HASTE: return 0;
    case SPELL_SLOW: return 1;
    case SPELL_BLIND: return 2;
    case SPELL_BLESS: return 3;
    case SPELL_CURSE: return 4;
    case SPELL_BERSERK: return 5;
    case SPELL_PARALYZE: return 6;
    case SPELL_HYPNOTIZE: return 7;
    case SPELL_BLOODLUST: return 9;
    case SPELL_SHIELD: return 10;
    case SPELL_STONE: return 11;
    case SPELL_ANTI_MAGIC: return 12;
    case SPELL_STONE_SKIN: return 13;
    }
    return -1;
}

void snapshotHero(const hero* h, H2CoreHero& out)
{
    memset(&out, 0, sizeof(out));
    if (!h)
        return;
    out.present = 1;
    out.attack = h->getPrimarySkill(0);
    out.defense = h->getPrimarySkill(1);
    out.power = h->getPrimarySkill(2);
    out.knowledge = h->getPrimarySkill(3);
    out.spellPoints = h->m_mana;
}

// Attack or defense as HoMM2's army::Init sees it: the creature's own
// number on HoMM2's scale (converted for a HoMM3 creature) plus the
// commanding hero's primary skill. HoMM3 keeps the hero's part out of the
// stack (it adds it when computing damage), so it is added here.
int h2ScaleSkill(const army& a, int value, int heroBonus, int nativeValue, double h3PerH2)
{
    if (mashupIsH2Creature(a.m_creatureType)) {
        // Keep HoMM3's own adjustments (native terrain, spells) on top of
        // HoMM2's base number.
        int registered = int(nativeValue * h3PerH2 + 0.5);
        return nativeValue + int((value - registered) / h3PerH2) + heroBonus;
    }
    return int(value / h3PerH2 + 0.5) + heroBonus;
}

void snapshotStack(const army& a, const hero* commander, H2CoreStack& out)
{
    memset(&out, 0, sizeof(out));
    out.alive = a.isActive() ? 1 : 0;
    if (!out.alive)
        return;
    int type = a.m_creatureType;
    out.h2Type = mashupIsH2Creature(type) ? type - MASHUP_H2_CREATURE_FIRST : -1;
    out.hostType = type;
    out.quantity = a.m_numTroops;
    out.hitPointsLost = a.m_topCreatureDamage;
    out.hex = a.m_gridIndex;
    out.facingRight = a.m_facing ? 1 : 0;
    unsigned int attributes = a.m_monInfo.m_attributes;
    if (attributes & 0x1)
        out.attributes |= H2CORE_FLAG_WIDE;
    if (attributes & 0x2)
        out.attributes |= H2CORE_FLAG_FLYING;
    if (attributes & 0x4)
        out.attributes |= H2CORE_FLAG_SHOOTER;
    if (attributes & 0x40000)
        out.attributes |= H2CORE_FLAG_UNDEAD;
    // HoMM3's Black Dragon shrugs off every spell, HoMM2's too.
    if (type == CREATURE_BLACK_DRAGON)
        out.attributes |= H2CORE_FLAG_SPELL_IMMUNE;
    out.shots = a.m_monInfo.m_numShots;
    out.speed = a.getSpeed();
    int h2Type = out.h2Type >= 0 ? out.h2Type : 0;
    out.attack = h2ScaleSkill(a, a.m_monInfo.m_attackSkill,
                              commander ? commander->getPrimarySkill(0) : 0,
                              g_h2NativeAttack[h2Type], g_h3AttackPerH2);
    out.defense = h2ScaleSkill(a, a.m_monInfo.m_defenseSkill,
                               commander ? commander->getPrimarySkill(1) : 0,
                               g_h2NativeDefense[h2Type], g_h3DefensePerH2);
    out.damageMin = a.m_monInfo.m_damageLowBound;
    out.damageMax = a.m_monInfo.m_damageHighBound;
    out.hitPoints = a.m_monInfo.m_hitPoints;
    out.fightValue = int(g_creatureTypeTraits[type].m_aiValue / g_aiValuePerH2FightValue + 0.5);
    out.retaliated = a.m_retaliationCount <= 0 ? 1 : 0;
    out.acted = (attributes & creatureDone) ? 1 : 0;
    out.morale = a.getMorale(1);
    out.luck = a.getLuck(1);
    for (int spell = 0; spell < hero::NUM_SPELLS; ++spell) {
        int influence = toH2Influence(spell);
        if (influence >= 0 && a.m_spellInfluence[spell] > 0)
            out.spellInfluence[influence] = (unsigned char)min(a.m_spellInfluence[spell], 255);
    }
}

void snapshotBattle(const combatManager* cm, H2CoreBattle& out)
{
    memset(&out, 0, sizeof(out));
    out.h2HeroSide = MASHUP_SIDE_H2;
    out.round = cm->m_turnNumber;
    out.currentSide = cm->m_actingSide;
    out.currentIndex = cm->m_actingSlot;
    for (int hex = 0; hex < H2CORE_HEXES; ++hex)
        out.blockedHex[hex] = (cm->m_cells[hex].m_attributes
                               & (hexcell::obstacleOrigin | hexcell::blocked)) ? 1 : 0;
    for (int side = 0; side < H2CORE_SIDES; ++side) {
        snapshotHero(cm->m_heroes[side], out.heroes[side]);
        out.heroes[side].castThisRound = cm->m_spellsCast[side];
        for (int slot = 0; slot < H2CORE_STACKS; ++slot)
            snapshotStack(cm->m_armies[side][slot], cm->m_heroes[side], out.stacks[side][slot]);
    }
}

long totalHitPoints(int quantity, int hitPoints, int lost)
{
    return quantity > 0 ? long(quantity - 1) * hitPoints + (hitPoints - lost) : 0;
}

int lowestHeroExperience(int level)
{
    int low = 0;
    int high = 100000000;
    while (low < high) {
        int middle = low + (high - low) / 2;
        if (hero::getLevel(middle) >= level)
            high = middle;
        else
            low = middle + 1;
    }
    return low;
}

const int MASHUP_HERO_LEVEL = 20;

// The user's settings: both heroes cast with this spell power and
// knowledge (MASHUP_SPELL_POWER, default 50, and MASHUP_KNOWLEDGE, default
// 30, in the environment; 0 keeps the leveled value).
int mashupSetting(const char* name, int fallback)
{
    const char* text = getenv(name);
    return text && *text ? atoi(text) : fallback;
}

int mashupSpellPower()
{
    return mashupSetting("MASHUP_SPELL_POWER", 50);
}

int mashupKnowledge()
{
    return mashupSetting("MASHUP_KNOWLEDGE", 30);
}

// Every stack on both sides has at least this morale and luck
// (MASHUP_MIN_MORALE / MASHUP_MIN_LUCK, default 2).
int mashupMinMorale()
{
    return mashupSetting("MASHUP_MIN_MORALE", 2);
}

int mashupMinLuck()
{
    return mashupSetting("MASHUP_MIN_LUCK", 2);
}

// The presentation rule: a hero casts each spell of his book once before
// casting any of them again, so the battle shows his whole repertoire.
// HoMM3 spell ids for Lord Haart, HoMM2 ones for Lord Kilburn.
unsigned char g_h3Repertoire[hero::NUM_SPELLS];

// The presentation's set piece: each hero casts Armageddon at his first
// chance from the third round on (round index 2), never before. Until then
// it is out of both books; afterwards it joins the rotation.
const int MASHUP_ARMAGEDDON_ROUND = 2;
const int H2_SPELL_ARMAGEDDON_ID = 27;
const int H2_SPELL_CURSE_ID = 18;
bool g_armageddonCast[2];
unsigned char g_h2Repertoire[H2CORE_SPELLS + 1];
unsigned char g_h2SpellsLeft[H2CORE_SPELLS + 1];

// The user's rule for this run: both heroes carry attacking spells only -
// the spells that deal damage, in each game's own numbering.
const bool MASHUP_ATTACK_SPELLS_ONLY = 1;

bool isH3AttackSpell(int spell)
{
    switch (spell) {
    case SPELL_MAGIC_ARROW: case SPELL_ICE_BOLT: case SPELL_LIGHTNING_BOLT:
    case SPELL_IMPLOSION: case SPELL_CHAIN_LIGHTNING: case SPELL_FROST_RING:
    case SPELL_FIREBALL: case SPELL_INFERNO: case SPELL_METEOR_SHOWER:
    case SPELL_DEATH_RIPPLE: case SPELL_DESTROY_UNDEAD: case SPELL_ARMAGEDDON:
        return 1;
    }
    return 0;
}

// HoMM2 SpellType ids.
bool isH2AttackSpell(int spell)
{
    switch (spell) {
    case 0:   // Fireball
    case 1:   // Fireblast
    case 2:   // Lightning Bolt
    case 3:   // Chain Lightning
    case 20:  // Holy Word
    case 21:  // Holy Shout
    case 25:  // Magic Arrow
    case 27:  // Armageddon
    case 28:  // Elemental Storm
    case 29:  // Meteor Shower
    case 32:  // Cold Ray
    case 33:  // Cold Ring
    case 35:  // Death Ripple
    case 36:  // Death Wave
        return 1;
    }
    return 0;
}
const int SPELL_BOOK_ARTIFACT = 0;
const int CASTLE_MAGE_GUILD_LEVELS = 4;

}  // namespace

// Mashup access to private game state (friend of army, combatManager, hero).
struct MashupAccess {
    // Lord Haart at MASHUP_HERO_LEVEL by HoMM3's own level-ups (the AI's
    // skill choices), with a spellbook holding every spell a fully built
    // Castle mage guild offers within his Wisdom.
    static void setupH3Hero(hero* h)
    {
        h->m_formation &= ~1;  // HoMM3 deploys spread out
        h->m_experience = lowestHeroExperience(MASHUP_HERO_LEVEL);
        int savedInSetup = g_inSetup;
        g_inSetup = 1;
        h->checkLevel();
        g_inSetup = savedInSetup;

        type_artifact book;
        book.m_artifactId = TArtifact(SPELL_BOOK_ARTIFACT);
        book.m_extra = -1;
        if (h->m_equipped[17].m_artifactId == ARTIFACT_NONE)
            h->giveArtifact(book, 0, 0);
        int maxLevel = 2 + h->getSecondarySkill(eSecSkillWisdom);
        if (maxLevel > CASTLE_MAGE_GUILD_LEVELS)
            maxLevel = CASTLE_MAGE_GUILD_LEVELS;
        for (int spell = 0; spell < hero::NUM_SPELLS; ++spell) {
            h->m_inSpellbook[spell] = 0;
            h->m_availableSpells[spell] = 0;
            const SSpellTraits& traits = g_spellTraits[spell];
            if (MASHUP_ATTACK_SPELLS_ONLY && !isH3AttackSpell(spell))
                continue;
            if (spell == SPELL_ARMAGEDDON) {
                // The Castle set piece saves it for the third round; in the
                // other matchups it is in the book, guild or no guild.
                if (!mashupMatchup().m_armageddonSetPiece) {
                    h->addSpell(spell);
                    g_h3Repertoire[spell] = 1;
                    mashupLog("mashup: %s knows %s", h->m_name, traits.m_name);
                }
                continue;
            }
            if (traits.m_townProbability[mashupMatchup().m_h3Town] > 0
                && traits.m_level <= maxLevel) {
                h->addSpell(spell);
                g_h3Repertoire[spell] = 1;
                mashupLog("mashup: %s knows %s", h->m_name, traits.m_name);
            }
        }
        if (mashupSpellPower() > 0)
            h->m_stats[2] = (signed char)mashupSpellPower();
        if (mashupKnowledge() > 0)
            h->m_stats[3] = (signed char)mashupKnowledge();
        h->m_mana = short(h->getMaxMana());
    }

    // The HoMM2 hero as HoMM2 builds him; the HoMM3 record only carries his
    // name, level, primary skills and spell points. HoMM3 secondary skills
    // are cleared so no HoMM3 bonus touches the HoMM2 army.
    static void setupH2Shell(hero* h, const H2CoreHeroInfo& info)
    {
        // Lord Kilburn's HoMM2 portraits (h2mash.lod) on the borrowed record.
        g_heroTraitsStorage[h->m_portrait].m_largePortraitName = mashupMatchup().m_h2LargePortrait;
        g_heroTraitsStorage[h->m_portrait].m_smallPortraitName = mashupMatchup().m_h2SmallPortrait;
        strncpy(h->m_name, info.name, sizeof(h->m_name) - 1);
        h->m_name[sizeof(h->m_name) - 1] = 0;
        h->m_experience = info.experience;
        h->m_level = short(info.level);
        h->m_stats[0] = (signed char)info.attack;
        h->m_stats[1] = (signed char)info.defense;
        h->m_stats[2] = (signed char)info.power;
        h->m_stats[3] = (signed char)info.knowledge;
        for (int skill = 0; skill < 28; ++skill)
            h->m_skillLevel[skill] = 0;
        h->m_skillCount = 0;
        // The HoMM2 army deploys grouped near the centre (HoMM3 tight formation).
        h->m_formation |= 1;
        h->m_mana = short(info.spellPoints);
    }

    // Applies a HoMM2 spell's outcome (from h2core_cast) to the battle and
    // shows it with the closest HoMM3 animation.
    // byCreature: a creature's spell ability (the Mummies' curse), with no
    // hero gestures, spell points or once-a-round flag.
    static void applyH2Cast(combatManager* cm, int h2Spell, int targetHex,
                            const H2CoreBattle& before, const H2CoreCastResult& result,
                            bool byCreature = 0)
    {
        int side = before.currentSide;
        hero* caster = cm->m_heroes[side];
        char spellName[sizeof(result.spellName)];
        strcpy(spellName, result.spellName);
        for (int end = int(strlen(spellName)) - 1; end >= 0 && spellName[end] == ' '; --end)
            spellName[end] = 0;
        char text[160];
        if (byCreature) {
            mashupLog("h2 creature cast: %s on hex %d", spellName, targetHex);
        } else {
            sprintf(text, "%s casts %s.", caster ? caster->m_name : "The hero", spellName);
            mashupLog("h2 cast: %s", text);
            if (!cm->isQuickCombat() && cm->m_combatWindow)
                cm->m_combatWindow->combatMessage(text, 1, 0);
        }

        // The HoMM2 hero raises his arm (his own cast frames) before the
        // spell lands and lowers it afterwards, as HoMM3 heroes do.
        const TCombatHeroSprite& heroSprite = combatHeroSprite(caster);
        int castFrames = 0;
        if (!byCreature && caster && !cm->isQuickCombat() && cm->m_creatureSprites[side]) {
            castFrames = cm->m_creatureSprites[side]->getNumFrames(4);
            cm->m_cmbtHeroFrameType[side] = 4;
            for (int frame = 0; frame < heroSprite.m_castFrame && frame < castFrames; ++frame) {
                cm->m_cmbtHeroFrameIndex[side] = frame;
                cm->drawFrame(1, 0, 0, 100, 1, 1);
            }
        }

        // As HoMM3's castSpell does: the field may still be limited to
        // redrawing the acting stack, which would leave every other stack's
        // count stale until some later full redraw.
        cm->resetLimitCreature();
        cm->clearEffects();
        memset(s_hitPointsLost, 0, sizeof(s_hitPointsLost));
        bool anyEffect = 0;
        bool anyDamage = 0;
        for (int s = 0; s < H2CORE_SIDES; ++s) {
            for (int i = 0; i < H2CORE_STACKS; ++i) {
                const H2CoreStack& b = before.stacks[s][i];
                const H2CoreStackAfter& a = result.stacks[s][i];
                if (!b.alive)
                    continue;
                army& stack = cm->m_armies[s][i];
                bool changed = 0;
                long hpBefore = totalHitPoints(b.quantity, b.hitPoints, b.hitPointsLost);
                long hpAfter = a.alive ? totalHitPoints(a.quantity, b.hitPoints, a.hitPointsLost) : 0;
                if (hpAfter < hpBefore) {
                    s_hitPointsLost[s][i] = hpBefore - hpAfter;
                    int killed = stack.damage(int(hpBefore - hpAfter));
                    mashupLog("  %d/%d loses %ld hp (%d killed)", s, i, hpBefore - hpAfter, killed);
                    anyDamage = 1;
                    changed = 1;
                } else if (hpAfter > hpBefore) {
                    stack.m_numTroops = a.quantity;
                    stack.m_topCreatureDamage = a.hitPointsLost;
                    mashupLog("  %d/%d restored to %d", s, i, a.quantity);
                    changed = 1;
                }
                for (int k = 0; k < H2CORE_SPELL_INFLUENCES; ++k) {
                    if (a.spellInfluence[k] == b.spellInfluence[k])
                        continue;
                    int h3Spell = H2_INFLUENCE_TO_H3_SPELL[k];
                    if (a.spellInfluence[k])
                        stack.setSpellInfluence(h3Spell, a.spellInfluence[k], 1, caster);
                    else
                        stack.cancelIndividualSpell(h3Spell);
                    mashupLog("  %d/%d influence %d: %d -> %d", s, i, k,
                              b.spellInfluence[k], a.spellInfluence[k]);
                    changed = 1;
                }
                if (a.alive && a.hex != b.hex)
                    mashupLog("  %d/%d moved %d -> %d (not shown)", s, i, b.hex, a.hex);
                if (changed && i < 20) {
                    cm->m_effected[s][i] = 1;
                    anyEffect = 1;
                }
            }
        }
        if (!cm->isQuickCombat())
            showH2Spell(cm, h2Spell, targetHex, caster, anyEffect, anyDamage);
        // HoMM2's own report of what the spell did ("The lightning bolt
        // does 1250 damage..."), as HoMM2's CastSpell wrote it.
        for (int line = 0; line < result.messageCount; ++line) {
            mashupLog("  h2 says: %s", result.messages[line]);
            if (!cm->isQuickCombat() && cm->m_combatWindow)
                cm->m_combatWindow->combatMessage(result.messages[line], 1, 0);
        }
        for (int frame = heroSprite.m_castFrame; frame < castFrames; ++frame) {
            cm->m_cmbtHeroFrameIndex[side] = frame;
            cm->drawFrame(1, 0, 0, 100, 1, 1);
        }
        if (castFrames) {
            cm->m_cmbtHeroFrameType[side] = 0;
            cm->m_cmbtHeroFrameIndex[side] = 0;
        }
        // Redraw every stack the spell changed, with its new count.
        for (int markSide = 0; markSide < 2; ++markSide)
            for (int markIndex = 0; markIndex < 20; ++markIndex)
                if (cm->m_effected[markSide][markIndex])
                    cm->markCreatureEffect(markSide, markIndex);
        cm->computeMaxExtent();
        cm->drawFrame(1, 0, 0, 0, 1, 0);
        cm->checkRebirth();
        if (!byCreature) {
            cm->m_spellsCast[side] = 1;
            if (caster)
                caster->m_mana = short(result.spellPointsLeft);
        }
    }

    // HoMM2's own sound, missile and effect for the spell (converted from
    // HEROES2.AGG); the battle state has already changed.
    static void showH2Spell(combatManager* cm, int h2Spell, int targetHex, hero* caster,
                            bool anyEffect, bool anyDamage)
    {
        const H2SpellVisual& visual = H2_SPELL_VISUALS[h2Spell];
        if (visual.m_sound) {
            char sample[32];
            sprintf(sample, "%s.wav", visual.m_sound);
            launchSample(sample, -1, 3);
        }
        army* target = targetHex >= 0 && targetHex < 187 ? cm->m_cells[targetHex].getArmy() : 0;
        int castX = 0;
        int castY = 0;
        if (caster) {
            const TCombatHeroSprite& heroSprite = combatHeroSprite(caster);
            castX = cm->m_currentSide == 0
                ? heroSprite.m_castX - 43
                : cm->m_creatureSprites[1]->getWidth() - heroSprite.m_castX + 693;
            castY = heroSprite.m_castY - 19;
        }
        // HoMM2 draws these in code rather than from art (mashup_h2fx.cpp).
        if (h2Spell == H2_SPELL_LIGHTNING_BOLT && target && caster)
            mashupH2Bolt(cm, castX, castY, target->midX(), target->midY(),
                         MASHUP_H2_LIGHTNING_BOLT);
        if (h2Spell == H2_SPELL_CHAIN_LIGHTNING && caster)
            showH2ChainLightning(cm, castX, castY);
        if (h2Spell == H2_SPELL_ARMAGEDDON)
            mashupH2Armageddon(cm);
        if (visual.m_missile && target && caster) {
            CSprite* missile = ResourceManager::getSprite(visual.m_missile);
            if (missile) {
                cm->shootMissile(castX, castY, target->midX(), target->midY(),
                                 H2_MISSILE_ANGLES, missile);
                ResourceManager::dispose(missile);
            }
        }
        if (visual.m_kind == H2_VISUAL_AREA && visual.m_effect >= 0 && targetHex >= 0) {
            cm->spellEffect(visual.m_effect, targetHex, 100, 0);
            for (int s = 0; s < 2; ++s)
                for (int i = 0; i < 20; ++i)
                    if (cm->m_effected[s][i])
                        cm->m_armies[s][i].m_showPowEffect = 1;
            cm->powEffect(eSpellEffectNone, 1);
        } else if (anyDamage) {
            // As HoMM3's own damage spells do: powEffect plays the effect
            // over the marked stacks, their wince or death, and releases the
            // troop counts army::damage held at their old value. A spell
            // with no HoMM2 effect (Armageddon, Magic Arrow) marks nothing:
            // army::draw would show the last loaded effect sprite instead.
            if (visual.m_effect >= 0)
                for (int s = 0; s < 2; ++s)
                    for (int i = 0; i < 20; ++i)
                        if (cm->m_effected[s][i])
                            cm->m_armies[s][i].m_showPowEffect = 1;
            cm->powEffect(TSpellEffectID(visual.m_effect >= 0 ? visual.m_effect : eSpellEffectNone), 1);
        } else if (anyEffect) {
            cm->showMassSpell(cm->m_effected,
                              visual.m_effect >= 0 ? visual.m_effect : eSpellEffectNone, 0);
        }
    }

    // HoMM2's army::CheckLuck: good luck on 1d24 <= luck, bad luck on
    // 1d24 < -luck (HoMM3 has no bad luck), with HoMM2's effect and sound.
    static void checkH2Luck(army* stack)
    {
        stack->m_luckStatus = 0;
        if (!stack->getController())
            return;
        int luck = stack->m_luck;
        if (luck > 0 && sRandom(1, 24) <= luck)
            stack->m_luckStatus = 1;
        if (luck < 0 && sRandom(1, 24) < -luck)
            stack->m_luckStatus = -1;
        if (!stack->m_luckStatus || g_combatManager->isQuickCombat())
            return;
        bool good = stack->m_luckStatus > 0;
        mashupLog("h2 luck: %s %s", stack->getName(), good ? "good" : "bad");
        launchSample(good ? "H2BGOODLUCK.wav" : "H2BBADLUCK.wav", -1, 3);
        sprintf(g_text, good ? "Good luck shines on the %s." : "Bad luck descends on the %s.",
                stack->getName());
        g_combatManager->m_combatWindow->combatMessage(g_text, 1, 0);
        g_combatManager->spellEffect(good ? MASHUP_EFFECT_H2_GOOD_LUCK : MASHUP_EFFECT_H2_BAD_LUCK,
                                     stack, 100, 0);
    }

    // HoMM2's chain jumps from the caster to the hardest-hit stack and on
    // down: each jump halves the damage, so damage orders the links.
    static void showH2ChainLightning(combatManager* cm, int castX, int castY)
    {
        bool linked[2][21];
        memset(linked, 0, sizeof(linked));
        int fromX = castX;
        int fromY = castY;
        for (int link = 0; link < 4; ++link) {
            int bestSide = -1;
            int bestIndex = -1;
            for (int s = 0; s < 2; ++s)
                for (int i = 0; i < 21; ++i)
                    if (!linked[s][i] && s_hitPointsLost[s][i] > 0
                        && (bestSide < 0 || s_hitPointsLost[s][i] > s_hitPointsLost[bestSide][bestIndex])) {
                        bestSide = s;
                        bestIndex = i;
                    }
            if (bestSide < 0)
                break;
            linked[bestSide][bestIndex] = 1;
            army& stack = cm->m_armies[bestSide][bestIndex];
            mashupH2Bolt(cm, fromX, fromY, stack.midX(), stack.midY(), link);
            fromX = stack.midX();
            fromY = stack.midY();
        }
    }

    // Hit points each stack lost to the HoMM2 spell being shown.
    static long s_hitPointsLost[2][21];

    // Takes a cast HoMM3 spell out of the book until the whole repertoire
    // has been cast once.
    static void noteH3Cast(hero* h, int spell)
    {
        if (spell < 0 || spell >= hero::NUM_SPELLS || !g_h3Repertoire[spell])
            return;
        h->m_inSpellbook[spell] = 0;
        h->m_availableSpells[spell] = 0;
        for (int other = 0; other < hero::NUM_SPELLS; ++other)
            if (g_h3Repertoire[other] && h->m_inSpellbook[other])
                return;
        mashupLog("mashup: %s has cast every spell; his book is whole again", h->m_name);
        for (int restored = 0; restored < hero::NUM_SPELLS; ++restored)
            if (g_h3Repertoire[restored])
                h->addSpell(restored);
    }

    // Lord Haart's Armageddon: queued as his hero cast, as his spell AI
    // would queue it (action 1, whole field, no target).
    static bool scriptH3Armageddon(combatManager* cm)
    {
        int side = cm->m_currentSide;
        hero* h = cm->m_heroes[side];
        if (!mashupMatchup().m_armageddonSetPiece
            || !h || g_armageddonCast[side] || cm->m_spellsCast[side]
            || cm->m_turnNumber < MASHUP_ARMAGEDDON_ROUND)
            return 0;
        g_armageddonCast[side] = 1;
        h->addSpell(SPELL_ARMAGEDDON);
        g_h3Repertoire[SPELL_ARMAGEDDON] = 1;
        mashupLog("mashup: round %d, %s casts the set-piece Armageddon",
                  cm->m_turnNumber + 1, h->m_name);
        cm->m_nextAction = 1;
        cm->m_nextActionExtra = SPELL_ARMAGEDDON;
        cm->m_nextActionGridIndex = -1;
        cm->m_nextActionGridIndex2 = -1;
        return 1;
    }

    // HoMM2's Lich cloud (army::SpecialAttack): the target and every stack
    // around it, friend or foe, each hit once by the shot's HoMM2 damage,
    // under HoMM2's cloud and explosion sound.
    static void h2LichCloud(army* shooter, army* target)
    {
        combatManager* cm = g_combatManager;
        if (!cm->isQuickCombat()) {
            char sample[32];
            sprintf(sample, "%sEXPL.wav", shooter->m_monInfo.m_samplePrefix);
            launchSample(sample, -1, 3);
            cm->spellEffect(MASHUP_EFFECT_H2_LICH_CLOUD, target, 100, 0);
        }
        cm->clearEffects();
        int killed = 0;
        long damage = 0;
        army* first = 0;
        bool multiple = 0;
        for (long direction = 0; direction <= COMBAT_DIRECTION_COUNT; ++direction) {
            long hex = direction == COMBAT_DIRECTION_COUNT
                ? target->m_gridIndex
                : shooter->getAdjacentCellIndexNoArmy(target->m_gridIndex, direction);
            if (!cm->validHex(hex) || !cm->m_cells[hex].hasArmy())
                continue;
            hexcell* cell = &cm->m_cells[hex];
            const int slot = cell->m_armySlot;
            const int side = cell->m_armySide;
            if (cm->m_effected[side][slot])
                continue;
            cm->m_effected[side][slot] = 1;
            army* stack = cell->getArmy();
            if (!first)
                first = stack;
            else if (first->m_creatureType != stack->m_creatureType)
                multiple = 1;
            int dealt = 0;
            int killedNow = 0;
            shooter->damageEnemy(stack, &dealt, &killedNow, 1);
            damage += dealt;
            killed += killedNow;
        }
        if (damage > 0) {
            cm->damageMessage(shooter->getName(), shooter->m_numTroops, damage,
                              multiple ? 0 : first, killed);
            cm->powEffect(eSpellEffectNone, 1);
        }
    }

    // After a HoMM2 creature's strike (army::DoAttack's tail).
    static void h2PostAttack(army* attacker, army* target, int damage, int killed)
    {
        combatManager* cm = g_combatManager;
        int h2Type = attacker->m_creatureType - MASHUP_H2_CREATURE_FIRST;
        if (h2Type == H2_CREATURE_VAMPIRE_LORD) {
            // The hit points of the creatures killed heal the Vampire Lords,
            // then raise fallen ones, up to the stack's starting size.
            long benefit = long(killed) * target->m_monInfo.m_hitPoints;
            if (benefit <= 0)
                return;
            int before = attacker->m_numTroops;
            if (attacker->m_topCreatureDamage > benefit) {
                attacker->m_topCreatureDamage -= int(benefit);
            } else {
                benefit -= attacker->m_topCreatureDamage;
                attacker->m_topCreatureDamage = 0;
                long revived = benefit / attacker->m_monInfo.m_hitPoints;
                if (attacker->m_origNumTroops - attacker->m_numTroops > revived)
                    attacker->m_numTroops += int(revived);
                else
                    attacker->m_numTroops = attacker->m_origNumTroops;
            }
            mashupLog("h2 drain: %d %s -> %d", before, attacker->getName(),
                      attacker->m_numTroops);
            return;
        }
        if (h2Type == H2_CREATURE_MUMMY || h2Type == H2_CREATURE_ROYAL_MUMMY) {
            // ARMY_ATTACK_EFFECT_CHANCE / ARMY_ROYAL_MUMMY_EFFECT_CHANCE.
            int chance = h2Type == H2_CREATURE_ROYAL_MUMMY ? 30 : 20;
            if (target->m_numTroops <= 0 || sRandom(1, 100) >= chance)
                return;
            static H2CoreBattle battle;
            static H2CoreCastResult result;
            snapshotBattle(cm, battle);
            battle.currentSide = attacker->getOwningSide();
            battle.currentIndex = int(attacker - &cm->m_armies[battle.currentSide][0]);
            int outcome = g_h2core.m_creatureCast(&battle, H2_SPELL_CURSE_ID,
                                                  target->m_gridIndex, &result);
            mashupLog("h2 curse: %s -> %s: %s", attacker->getName(), target->getName(),
                      outcome == 1 ? "cursed" : "resisted");
            if (outcome == 1)
                applyH2Cast(cm, H2_SPELL_CURSE_ID, target->m_gridIndex, battle, result, 1);
        }
    }

    static void setMoraleLuck(army& stack, int morale, int luck)
    {
        stack.m_morale = morale;
        stack.m_luck = luck;
    }
};

long MashupAccess::s_hitPointsLost[2][21];

bool mashupIsH2Creature(int type)
{
    return type >= MASHUP_H2_CREATURE_FIRST
        && type < MASHUP_H2_CREATURE_FIRST + MASHUP_H2_CREATURE_COUNT;
}

void noteH2Cast(int spell);

void mashupChooseH2Action(combatManager* cm)
{
    static H2CoreBattle battle;
    static H2CoreCastResult castResult;
    H2CoreAction action;
    for (;;) {
        snapshotBattle(cm, battle);
        int side = battle.currentSide;
        if (mashupMatchup().m_armageddonSetPiece
            && !g_armageddonCast[side] && !cm->m_spellsCast[side]
            && cm->m_turnNumber >= MASHUP_ARMAGEDDON_ROUND && cm->m_heroes[side]) {
            // Lord Kilburn's set-piece Armageddon, by HoMM2's rules.
            g_armageddonCast[side] = 1;
            mashupLog("mashup: round %d, the HoMM2 hero casts the set-piece Armageddon",
                      cm->m_turnNumber + 1);
            if (g_h2core.m_cast(&battle, H2_SPELL_ARMAGEDDON_ID, -1, -1, &castResult)) {
                MashupAccess::applyH2Cast(cm, H2_SPELL_ARMAGEDDON_ID, -1, battle, castResult);
                g_h2Repertoire[H2_SPELL_ARMAGEDDON_ID] = 1;
                noteH2Cast(H2_SPELL_ARMAGEDDON_ID);
            }
            continue;
        }
        mashupLog("h2 turn: side %d slot %d", battle.currentSide, battle.currentIndex);
        if (!g_h2core.m_chooseAction(&battle, &action)) {
            cm->m_nextAction = AI_ORDER_NONE;
            return;
        }
        if (action.kind != H2CORE_ACT_CAST)
            break;
        // The hero casts first, as in HoMM2; then the stack still acts.
        mashupLog("  -> cast spell %d at %d", action.spell, action.spellHex);
        if (cm->m_spellsCast[battle.currentSide]
            || !g_h2core.m_cast(&battle, action.spell, action.spellHex, -1, &castResult)) {
            cm->m_spellsCast[battle.currentSide] = 1;
            continue;
        }
        MashupAccess::applyH2Cast(cm, action.spell, action.spellHex, battle, castResult);
        noteH2Cast(action.spell);
    }
    mashupLog("  -> kind %d stand %d target %d/%d", action.kind, action.standHex,
              action.targetSide, action.targetIndex);
    army* target = 0;
    if (action.targetSide >= 0 && action.targetIndex >= 0)
        target = &cm->m_armies[action.targetSide][action.targetIndex];
    cm->m_nextActionExtra = 0;
    cm->m_nextActionGridIndex = -1;
    cm->m_nextActionGridIndex2 = -1;
    switch (action.kind) {
    case H2CORE_ACT_MOVE:
        cm->m_nextAction = 2;
        cm->m_nextActionGridIndex = action.standHex;
        break;
    case H2CORE_ACT_ATTACK:
        cm->m_nextAction = AI_ORDER_MOVE_AND_ATTACK;
        cm->m_nextActionExtra = action.standHex;
        cm->m_nextActionGridIndex = target ? target->m_gridIndex : -1;
        break;
    case H2CORE_ACT_SHOOT:
        cm->m_nextAction = AI_ORDER_SHOOT;
        cm->m_nextActionGridIndex = target ? target->m_gridIndex : -1;
        break;
    case H2CORE_ACT_WAIT:
        cm->m_nextAction = 8;
        break;
    default:
        cm->m_nextAction = AI_ORDER_NONE;
        break;
    }
}

int mashupH2Damage(const army* attacker, const army* target, bool isShot)
{
    static H2CoreBattle battle;
    snapshotBattle(g_combatManager, battle);
    int luck = attacker->m_luckStatus > 0 ? 1 : attacker->m_luckStatus < 0 ? -1 : 0;
    int damage = g_h2core.m_damage(&battle, attacker->m_combatSide, attacker->m_bitIndex,
                                   target->m_combatSide, target->m_bitIndex,
                                   isShot ? 1 : 0, luck);
    const H2CoreStack& a = battle.stacks[attacker->m_combatSide][attacker->m_bitIndex];
    const H2CoreStack& d = battle.stacks[target->m_combatSide][target->m_bitIndex];
    mashupLog("h2 damage: %d %s (A%d, %d-%d) -> %s (D%d) shot %d luck %d = %d",
              a.quantity, g_creatureTypeTraits[attacker->m_creatureType].m_pluralName,
              a.attack, a.damageMin, a.damageMax,
              g_creatureTypeTraits[target->m_creatureType].m_pluralName, d.defense,
              isShot, luck, damage);
    // The defender's own game decides how much of a blow it shrugs off.
    if (!mashupIsH2Creature(target->m_creatureType))
        damage = int(damage * target->computeDefenderDamageReduction(isShot));
    return damage > 0 ? damage : 1;
}

bool mashupIsH2SpellImmune(int creatureType)
{
    return mashupIsH2Creature(creatureType)
        && creatureType - MASHUP_H2_CREATURE_FIRST == H2_CREATURE_BLACK_DRAGON;
}

bool mashupH2UnlimitedRetaliation(int creatureType)
{
    return mashupIsH2Creature(creatureType)
        && creatureType - MASHUP_H2_CREATURE_FIRST == H2_CREATURE_GRIFFIN;
}

bool mashupIsH2Lich(int creatureType)
{
    int h2Type = creatureType - MASHUP_H2_CREATURE_FIRST;
    return mashupIsH2Creature(creatureType)
        && (h2Type == H2_CREATURE_LICH || h2Type == H2_CREATURE_POWER_LICH);
}

// HoMM2's missile for each shooter (army::LoadResources), from h2mash.lod.
const char* mashupH2MissileName(int creatureType)
{
    switch (creatureType - MASHUP_H2_CREATURE_FIRST) {
    case 1:   // Archer
    case 2:   // Ranger
        return "h2carrow.def";
    case 12:  // Orc
    case 13:  // Orc Chief
        return "h2morc.def";
    case 17:  // Troll
    case 18:  // War Troll
        return "h2mtroll.def";
    case 25:  // Druid
    case 26:  // Greater Druid
        return "h2mdruid.def";
    case 38:  // Halfling
        return "h2mhalf.def";
    case 45:  // Giant
    case 46:  // Titan
        return "h2mtitan.def";
    case H2_CREATURE_LICH:
    case H2_CREATURE_POWER_LICH:
        return "h2mlich.def";
    }
    return "h2melf.def";
}

void mashupH2LichCloud(army* shooter, army* target)
{
    MashupAccess::h2LichCloud(shooter, target);
}

void mashupH2PostAttack(army* attacker, army* target, int damage, int killed)
{
    MashupAccess::h2PostAttack(attacker, target, damage, killed);
}

int mashupH2CombatHeroSpriteOffset()
{
    return mashupMatchup().m_h2CombatHeroSprite;
}

bool mashupScriptedH3Cast(combatManager* cm)
{
    return g_mashupMode && MashupAccess::scriptH3Armageddon(cm);
}

void mashupNoteHeroCast(hero* h, int spell)
{
    if (g_mashupMode && h && !mashupIsH2Hero(h))
        MashupAccess::noteH3Cast(h, spell);
}

// The same rotation for the HoMM2 hero's book, held by h2core.
void noteH2Cast(int spell)
{
    if (spell < 0 || spell >= H2CORE_SPELLS)
        return;
    g_h2SpellsLeft[spell] = 0;
    for (int other = 0; other < H2CORE_SPELLS; ++other)
        if (g_h2SpellsLeft[other])
            goto update;
    mashupLog("mashup: the HoMM2 hero has cast every spell; his book is whole again");
    memcpy(g_h2SpellsLeft, g_h2Repertoire, sizeof(g_h2SpellsLeft));
update:
    g_h2core.m_setHeroSpells(g_h2SpellsLeft);
}

void mashupCheckH2Luck(army* stack)
{
    MashupAccess::checkH2Luck(stack);
}

void mashupUpdateMoraleLuck(combatManager* cm)
{
    if (!g_mashupMode)
        return;
    static H2CoreBattle battle;
    snapshotBattle(cm, battle);
    int morale = 0;
    int luck = 0;
    if (!g_h2core.m_moraleLuck(&battle, MASHUP_SIDE_H2, &morale, &luck))
        return;
    // MASHUP_H2_LUCK overrides HoMM2's luck, to exercise the luck effects.
    const char* forcedLuck = getenv("MASHUP_H2_LUCK");
    if (forcedLuck && *forcedLuck)
        luck = atoi(forcedLuck);
    for (int slot = 0; slot < H2CORE_STACKS; ++slot) {
        army& stack = cm->m_armies[MASHUP_SIDE_H2][slot];
        if (stack.isActive() && mashupIsH2Creature(stack.m_creatureType))
            MashupAccess::setMoraleLuck(stack, morale, luck);
    }
    // The user's floor: every stack on both sides at least this morale/luck.
    for (int side = 0; side < 2; ++side) {
        for (int slot = 0; slot < H2CORE_STACKS; ++slot) {
            army& stack = cm->m_armies[side][slot];
            if (!stack.isActive())
                continue;
            int floorMorale = stack.getMorale(0) < mashupMinMorale() ? mashupMinMorale()
                                                                     : stack.getMorale(0);
            int floorLuck = stack.getLuck(0) < mashupMinLuck() ? mashupMinLuck()
                                                               : stack.getLuck(0);
            MashupAccess::setMoraleLuck(stack, floorMorale, floorLuck);
        }
    }
}

bool mashupIsH2Hero(const hero* h)
{
    return g_mashupMode && h && h == g_game->getHero(MASHUP_H2_HERO);
}

bool mashupIsH2Side(int side)
{
    return g_mashupMode && side == MASHUP_SIDE_H2;
}

// Runs from advManager::main on the first adventure-map tick, so the battle
// starts exactly like an adventure-map attack.
bool mashupRunPendingBattle()
{
    static bool s_battleDone = 0;
    if (!g_mashupMode || s_battleDone)
        return 0;
    s_battleDone = 1;

    const MashupMatchup& matchup = mashupMatchup();
    for (int stack = 0; stack < armyGroup::ARMY_GROUP_SLOT_COUNT; ++stack) {
        const TCreatureTypeTraits& traits = g_creatureTypeTraits[matchup.m_h3Army[stack].m_type];
        mashupLog("mashup: HoMM3 %s: %d a week (table growth %d, horde %d)",
                  traits.m_pluralName, matchup.m_h3Army[stack].m_weeklyGrowth,
                  traits.m_growthRate, traits.m_hordeGrowthRate);
    }
    hero* h3Hero = prepareHero(matchup.m_h3Hero, MASHUP_H3_PLAYER,
                               matchup.m_h3Army, armyGroup::ARMY_GROUP_SLOT_COUNT);
    MashupStack h2Army[H2_ARMY_SIZE];
    for (int i = 0; i < H2_ARMY_SIZE; ++i) {
        int id = MASHUP_H2_CREATURE_FIRST + matchup.m_h2ArmyTypes[i];
        h2Army[i].m_type = TCreatureType(id);
        h2Army[i].m_weeklyGrowth = h2ArmyMultiplier()
            * (g_creatureTypeTraits[id].m_growthRate + H2_WELL_BONUS);
        mashupLog("mashup: HoMM2 %s: %d a week", g_creatureTypeTraits[id].m_pluralName,
                  h2Army[i].m_weeklyGrowth);
    }
    hero* h2Hero = prepareHero(MASHUP_H2_HERO, MASHUP_H2_PLAYER, h2Army, H2_ARMY_SIZE);

    MashupAccess::setupH3Hero(h3Hero);
    H2CoreHeroInfo h2Info;
    if (!g_h2core.m_makeHero(matchup.m_h2HeroId, MASHUP_HERO_LEVEL, mashupSeed(), &h2Info))
        shutDown("Mashup: h2core could not build the HoMM2 hero");
    if (MASHUP_ATTACK_SPELLS_ONLY) {
        for (int spell = 0; spell < H2CORE_SPELLS; ++spell) {
            if (!isH2AttackSpell(spell))
                h2Info.spells[spell] = 0;
            else if (h2Info.spells[spell])
                mashupLog("mashup: %s knows HoMM2 spell %d", h2Info.name, spell);
        }
        g_h2core.m_setHeroSpells(h2Info.spells);
    }
    // The Castle set piece saves Armageddon for the third round; in the other
    // matchups it is in the book, guild or no guild.
    h2Info.spells[H2_SPELL_ARMAGEDDON_ID] = matchup.m_armageddonSetPiece ? 0 : 1;
    g_h2core.m_setHeroSpells(h2Info.spells);
    memcpy(g_h2Repertoire, h2Info.spells, sizeof(g_h2Repertoire));
    memcpy(g_h2SpellsLeft, h2Info.spells, sizeof(g_h2SpellsLeft));
    if (mashupSpellPower() > 0) {
        h2Info.power = mashupSpellPower();
        g_h2core.m_setHeroPrimary(2, h2Info.power);
    }
    if (mashupKnowledge() > 0) {
        // HoMM2 spell points are knowledge x 10 (HERO_NORMAL_SPELL_POINTS).
        h2Info.knowledge = mashupKnowledge();
        h2Info.spellPoints = h2Info.knowledge * 10;
        g_h2core.m_setHeroPrimary(3, h2Info.knowledge);
    }
    MashupAccess::setupH2Shell(h2Hero, h2Info);
    mashupLog("mashup: %s, level %d, A%d D%d P%d K%d, %d spell points",
              h3Hero->m_name, h3Hero->m_level, h3Hero->getPrimarySkill(0),
              h3Hero->getPrimarySkill(1), h3Hero->getPrimarySkill(2),
              h3Hero->getPrimarySkill(3), h3Hero->m_mana);
    mashupLog("mashup: %s, level %d, A%d D%d P%d K%d, %d spell points",
              h2Info.name, h2Info.level, h2Info.attack, h2Info.defense, h2Info.power,
              h2Info.knowledge, h2Info.spellPoints);

    mashupLog("mashup: battle starts");
    int winner = g_advManager->doCombat(battleLocation(), h3Hero, &h3Hero->m_army,
                                        MASHUP_H2_PLAYER, 0, h2Hero, &h2Hero->m_army,
                                        int(mashupSeed() % 1000) + 1, false, false);
    mashupLog("mashup: battle over after %d rounds, winner: %s (%s)",
              g_combatManager->m_turnNumber + 1, winner == 0 ? "HoMM3" : "HoMM2",
              winner == 0 ? h3Hero->m_name : h2Info.name);
    hero* survivors = winner == 0 ? h3Hero : h2Hero;
    for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        int type = survivors->m_army.m_armyTypes[slot];
        if (type >= 0 && survivors->m_army.m_numTroops[slot] > 0)
            mashupLog("  survivors: %d %s", survivors->m_army.m_numTroops[slot],
                      g_creatureTypeTraits[type].m_pluralName);
    }
    return 1;
}

void mashupMain()
{
    SetUnhandledExceptionFilter(logCrash);
    mashupLog("mashup: loading h2core.dll");
    if (!loadH2Core())
        shutDown("Mashup: could not load h2core.dll (HoMM2 rules)");
    mashupLog("mashup: registering HoMM2 creatures");
    registerCreatures();
    mashupLog("mashup: opening h2mash.lod");
    if (!ResourceManager::addMashupArchives())
        shutDown("Mashup: could not open Data\\h2mash.lod / h2mash.snd (HoMM2 art and sound)");
    mashupLog("mashup: loading map");
    if (!loadMashupMap())
        shutDown("Mashup: could not load maps\\Arrogance.h3m");

    computeAdvNetControl();
    g_windowManager->m_colorCyclingOn = 1;
    if (g_executive->addManager(g_advManager, -1))
        shutDown("Mashup: could not start the adventure manager");
    g_executive->mainLoop();
    g_executive->removeManager(g_advManager);
}
