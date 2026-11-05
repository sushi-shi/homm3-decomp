#ifndef MASHUP_H
#define MASHUP_H

// HoMM2-vs-HoMM3 battle mashup (not part of the retail game).
//
// `Heroes3.exe /M` skips the menus, loads a map for terrain and hero
// records, and runs one visible battle: a HoMM3 Castle hero with 100 weeks
// of fully built Castle growth against a HoMM2 Knight hero with 100 weeks
// of fully built Knight growth. Each side is driven by its own game's AI.

class army;
class combatManager;

// HoMM2 creatures occupy host creature ids 150..160 (HoMM2 ids 0..10).
enum {
    MASHUP_H2_CREATURE_FIRST = 150,
    MASHUP_H2_CREATURE_COUNT = 66
};

enum EMashupSide {
    MASHUP_SIDE_H3 = 0,  // attacker, left
    MASHUP_SIDE_H2 = 1   // defender, right
};

extern bool g_mashupMode;

// `/MA`: close the results screen by itself, for unattended (recorded) runs.
extern bool g_mashupAutoClose;

// Called from oldmain once the executive and managers are up.
void mashupMain();

// Called by advManager::main; starts the battle once. Returns true when
// a battle ran, after which the adventure loop should terminate.
bool mashupRunPendingBattle();

class hero;

// True for the HoMM3 hero record that stands in for the HoMM2 hero.
bool mashupIsH2Hero(const hero* h);

// True when the given combat side is played by the HoMM2 rules and AI.
bool mashupIsH2Side(int side);

void mashupLog(const char* format, ...);

bool mashupIsH2Creature(int type);

// Asks HoMM2's combat AI what the current (HoMM2-controlled) stack does and
// stores the decision as the combat manager's next action.
void mashupChooseH2Action(combatManager* cm);

// Damage a HoMM2 attacker deals, by HoMM2's formula; a HoMM3 target then
// applies its own reductions.
int mashupH2Damage(const army* attacker, const army* target, bool isShot);

// HoMM2 creature abilities the HoMM3 engine has no rule for, by HoMM2's
// rules (mashup.cpp). The HoMM2 Black Dragon shrugs off every spell; the
// Griffin retaliates without limit; each shooter has its HoMM2 missile.
bool mashupIsH2SpellImmune(int creatureType);
bool mashupH2UnlimitedRetaliation(int creatureType);
const char* mashupH2MissileName(int creatureType);
// The Lich's cloud: the target and every stack around it (army::rangeAttack).
bool mashupIsH2Lich(int creatureType);
void mashupH2LichCloud(army* shooter, army* target);
// After a HoMM2 creature's melee strike (army::doPostAttack): the Vampire
// Lord's drain and the Mummies' curse.
void mashupH2PostAttack(army* attacker, army* target, int damage, int killed);
// Which HoMM2 combat hero sprite leads this matchup (offset from
// COMBAT_HERO_SPRITE_H2).
int mashupH2CombatHeroSpriteOffset();

// Presentation set piece: queues the HoMM3 hero's scripted cast, if due
// (called from combatManager::doSpellAI). Returns true when queued.
bool mashupScriptedH3Cast(combatManager* cm);

// Presentation rule: the HoMM3 hero casts every spell once before any
// repeats (called from combatManager::castSpell).
void mashupNoteHeroCast(hero* h, int spell);

// HoMM2's luck roll (good and bad) and its presentation, for a HoMM2 stack.
void mashupCheckH2Luck(army* stack);

// Gives the HoMM2 stacks the morale and luck HoMM2 rules grant them.
void mashupUpdateMoraleLuck(combatManager* cm);

#endif
