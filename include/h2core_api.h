/*
 * h2core.dll - HoMM2 combat rules and AI for the HoMM2-vs-HoMM3 mashup.
 *
 * Plain C, 4-byte aligned, __cdecl: shared verbatim between the HoMM2 build
 * (clang/MinGW, i686) and the HoMM3 host (MSVC 6). The host owns the battle;
 * every call passes a complete snapshot, which the DLL mirrors into HoMM2's
 * own combatManager before running HoMM2 code on it.
 */
#ifndef H2CORE_API_H
#define H2CORE_API_H

#ifdef __cplusplus
extern "C" {
#endif

#define H2CORE_API_VERSION 6

#define H2CORE_SIDES 2
#define H2CORE_STACKS 21
#define H2CORE_HEXES 187
#define H2CORE_SPELL_INFLUENCES 15
#define H2CORE_SECONDARY_SKILLS 14
#define H2CORE_SPELLS 65

#if defined(_MSC_VER)
#define H2CORE_CALL __cdecl
#else
#define H2CORE_CALL __attribute__((cdecl))
#endif

#ifdef H2CORE_BUILD
#define H2CORE_EXPORT __declspec(dllexport)
#else
#define H2CORE_EXPORT
#endif

/* HoMM2 creature record, as the host registers it. */
typedef struct H2CoreCreature {
    char name[32];
    char pluralName[32];
    int cost;
    int fightValue;
    int growth;
    int hitPoints;
    int speed;        /* HoMM2 hexes per turn on HoMM2's 13x9 field */
    int attack;
    int defense;
    int damageMin;
    int damageMax;
    int shots;
    int attributes;   /* HoMM2 MONSTER_FLAGS_* */
} H2CoreCreature;

/* HoMM2 MONSTER_FLAGS_* bits the host maps. */
#define H2CORE_FLAG_WIDE 0x1
#define H2CORE_FLAG_FLYING 0x2
#define H2CORE_FLAG_SHOOTER 0x4
#define H2CORE_FLAG_UNDEAD 0x400
/* Host creature immune to every spell (HoMM3 Black Dragon); not a HoMM2 flag. */
#define H2CORE_FLAG_SPELL_IMMUNE 0x10000

typedef struct H2CoreStack {
    int alive;
    int h2Type;          /* HoMM2 creature id; -1 for a host creature */
    int hostType;        /* host creature id (keys the foreign rows) */
    int quantity;
    int hitPointsLost;   /* damage already taken by the top creature */
    int hex;             /* tail hex; a wide stack also covers hex+-1 */
    int facingRight;
    int attributes;      /* H2CORE_FLAG_* */
    int shots;
    int speed;           /* hexes per turn on the 17x11 field */
    int attack;
    int defense;
    int damageMin;
    int damageMax;
    int hitPoints;
    int fightValue;      /* HoMM2 scale; used for host creatures */
    int retaliated;
    int acted;
    int morale;
    int luck;
    unsigned char spellInfluence[H2CORE_SPELL_INFLUENCES]; /* rounds left */
    unsigned char pad[1];
} H2CoreStack;

typedef struct H2CoreHero {
    int present;
    int attack;
    int defense;
    int power;
    int knowledge;
    int spellPoints;
    int castThisRound;
    unsigned char secondarySkills[H2CORE_SECONDARY_SKILLS];
    unsigned char spells[H2CORE_SPELLS + 1];
} H2CoreHero;

typedef struct H2CoreBattle {
    int h2HeroSide;      /* side led by the hero from h2core_make_hero, or -1 */
    int round;
    int currentSide;
    int currentIndex;
    unsigned char blockedHex[H2CORE_HEXES + 1];
    H2CoreHero heroes[H2CORE_SIDES];
    H2CoreStack stacks[H2CORE_SIDES][H2CORE_STACKS];
} H2CoreBattle;

/* A HoMM2 hero, as HoMM2's own rules build and level it. */
typedef struct H2CoreHeroInfo {
    char name[16];
    int level;
    int experience;
    int attack;
    int defense;
    int power;
    int knowledge;
    int spellPoints;
    unsigned char secondarySkills[H2CORE_SECONDARY_SKILLS]; /* 0 none .. 3 expert */
    unsigned char spells[H2CORE_SPELLS + 1];                /* 1 = in the spellbook */
} H2CoreHeroInfo;

/* HoMM2 secondary skill ids. */
#define H2CORE_SKILL_ARCHERY 1
#define H2CORE_SKILL_LEADERSHIP 6
#define H2CORE_SKILL_WISDOM 7
#define H2CORE_SKILL_LUCK 9

/* The state of every stack after a HoMM2 spell resolved. */
typedef struct H2CoreStackAfter {
    int alive;
    int quantity;
    int hitPointsLost;
    int hex;
    int facingRight;
    unsigned char spellInfluence[H2CORE_SPELL_INFLUENCES];
    unsigned char pad[1];
} H2CoreStackAfter;

#define H2CORE_CAST_MESSAGES 4

typedef struct H2CoreCastResult {
    char spellName[32];
    int spellPointsLeft;
    int messageCount;
    char messages[H2CORE_CAST_MESSAGES][128];  /* HoMM2's own combat-log lines */
    H2CoreStackAfter stacks[H2CORE_SIDES][H2CORE_STACKS];
} H2CoreCastResult;

enum H2CoreActionKind {
    H2CORE_ACT_SKIP = 0,
    H2CORE_ACT_MOVE = 1,
    H2CORE_ACT_ATTACK = 2,
    H2CORE_ACT_SHOOT = 3,
    H2CORE_ACT_WAIT = 4,
    H2CORE_ACT_CAST = 5
};

typedef struct H2CoreAction {
    int kind;
    int standHex;        /* tail hex after moving (MOVE/ATTACK) */
    int targetSide;      /* ATTACK/SHOOT target stack */
    int targetIndex;
    int spell;           /* CAST: HoMM2 spell id */
    int spellHex;
} H2CoreAction;

/* Exported functions. All return 0 on failure. */
H2CORE_EXPORT int H2CORE_CALL h2core_version(void);
H2CORE_EXPORT int H2CORE_CALL h2core_init(unsigned int seed);
H2CORE_EXPORT int H2CORE_CALL h2core_creature(int h2Type, H2CoreCreature* out);
H2CORE_EXPORT int H2CORE_CALL h2core_choose_action(const H2CoreBattle* battle,
                                                   H2CoreAction* out);
/* Damage the attacker deals under HoMM2's rules (army::DamageEnemy).
 * luck: +1 good luck fired, -1 bad luck fired, 0 neither. */
H2CORE_EXPORT int H2CORE_CALL h2core_damage(const H2CoreBattle* battle,
                                            int attackerSide, int attackerIndex,
                                            int targetSide, int targetIndex,
                                            int ranged, int luck);

/* Builds the HoMM2 hero heroId (0 = Lord Kilburn, a Knight) at the given
 * level: HoMM2's starting record, HoMM2's level-ups (AI skill choices), a
 * spellbook with every spell a fully built mage guild of the hero's town
 * offers within the hero's Wisdom. The DLL keeps this hero as the one
 * leading H2CoreBattle.h2HeroSide. */
H2CORE_EXPORT int H2CORE_CALL h2core_make_hero(int heroId, int level, unsigned int seed,
                                               H2CoreHeroInfo* out);
/* Replaces the spellbook of the hero from h2core_make_hero (1 = known). */
H2CORE_EXPORT int H2CORE_CALL h2core_set_hero_spells(const unsigned char* spells);
/* Sets one primary skill (0 attack, 1 defense, 2 spell power, 3 knowledge)
 * of the hero from h2core_make_hero. */
H2CORE_EXPORT int H2CORE_CALL h2core_set_hero_primary(int skill, int value);
/* Casts a HoMM2 spell for the side to act by HoMM2's rules
 * (combatManager::CastSpell) and reports the resulting battle. */
H2CORE_EXPORT int H2CORE_CALL h2core_cast(const H2CoreBattle* battle, int spell,
                                          int targetHex, int secondHex,
                                          H2CoreCastResult* out);
/* A creature's spell ability (a Mummy's curse) on the stack at targetHex,
 * cast by the side to act: HoMM2's SpellCastWorks roll, then
 * combatManager::CastSpell as a creature cast. Returns 2 when the target
 * resisted (nothing changes), 1 when it took effect. */
H2CORE_EXPORT int H2CORE_CALL h2core_creature_cast(const H2CoreBattle* battle, int spell,
                                                   int targetHex, H2CoreCastResult* out);
/* Morale and luck a stack of this side gets under HoMM2's rules. */
H2CORE_EXPORT int H2CORE_CALL h2core_morale_luck(const H2CoreBattle* battle, int side,
                                                 int* morale, int* luck);

typedef int (H2CORE_CALL* H2CoreVersionFn)(void);
typedef int (H2CORE_CALL* H2CoreInitFn)(unsigned int);
typedef int (H2CORE_CALL* H2CoreCreatureFn)(int, H2CoreCreature*);
typedef int (H2CORE_CALL* H2CoreChooseActionFn)(const H2CoreBattle*, H2CoreAction*);
typedef int (H2CORE_CALL* H2CoreDamageFn)(const H2CoreBattle*, int, int, int, int,
                                          int, int);
typedef int (H2CORE_CALL* H2CoreMakeHeroFn)(int, int, unsigned int, H2CoreHeroInfo*);
typedef int (H2CORE_CALL* H2CoreSetHeroSpellsFn)(const unsigned char*);
typedef int (H2CORE_CALL* H2CoreSetHeroPrimaryFn)(int, int);
typedef int (H2CORE_CALL* H2CoreCastFn)(const H2CoreBattle*, int, int, int, H2CoreCastResult*);
typedef int (H2CORE_CALL* H2CoreCreatureCastFn)(const H2CoreBattle*, int, int, H2CoreCastResult*);
typedef int (H2CORE_CALL* H2CoreMoraleLuckFn)(const H2CoreBattle*, int, int*, int*);

#ifdef __cplusplus
}
#endif

#endif
