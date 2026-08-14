// quest.h - retail's seer-hut quest hierarchy (compiland seerhut.obj).
// HAND-OWNED. Class layouts are NOT fabricated from method symbols;
// prototypes stay comments until a retail layout is proven.
//
// Retail replaced the Dreamcast port's TSeerHut::Do*Dialog / GetRewardType
// monolith with a virtual `type_quest` family: ten classes, each with its own
// 15-slot vtable at 0x64174c, 0x641788, 0x6417c4, 0x641800, 0x64183c,
// 0x641878, 0x6418b4, 0x6418f0, 0x64192c and 0x641968. Only the slots this
// file needs are declared; the rest of the roster, and every byte of the base
// object below +0x40, are still unattested.
//
// The class-to-quest-type mapping is now byte-proven end to end: the ten
// vtables sit in address order and their slot-8 bodies return 1..9 in that
// same order (0x49a680=1, 0x49a9b0=2, 0x56e3d0=3, 0x56ebc0=4, 0x49acf0=5,
// 0x49af30=6, 0x49b180=7, 0x5721f0=8, 0x572810=9), which is exactly the h3m
// quest-type enumeration - level, primary skills, defeat hero, defeat
// monster, artifacts, creatures, resources, be hero, belong to player.
//
// This is deliberately NOT in seerhut.h: that header reaches game.h,
// advmgr.h and mapcell.h and therefore most of the tree, and adding ten
// class definitions to that include closure is exactly the perturbation the
// include-set residual class warns about. seerhut.cpp is the only consumer.
#ifndef HOMM3_QUEST_H
#define HOMM3_QUEST_H

#include <string>
#include <vector>
#include <va.h>
// TAbstractFile, the two-slot save/load stream every quest deserializer
// virtual-calls (slot 1 = Read). This adds NOTHING to seerhut.cpp's include
// closure: hero.h, which seerhut.cpp already includes ahead of this header,
// includes armygrp.h itself.
#include "armygrp.h"

class hero;

// The packed map position the defeat-monster quest carries at +0x44 and
// compares field by field in slot 10. The three widths are read straight off
// that body: it xors the low word against the argument's and masks 0x3ff,
// then xors the high word and masks 0x3ff and 0x3c00 out of the SAME xor.
struct TQuestPosition {
    unsigned short x : 10;
    unsigned short : 6;
    unsigned short y : 10;
    unsigned short z : 4;
    unsigned short : 2;
};

// Every derived class reads its own payload at +0x40, so the base closes at
// 0x40; nothing between the vptr and there is attested.
class type_quest {
public:
    unsigned char pad_4[0x3c];

    // Slot 1: the AI's valuation of the quest for one player. The base body
    // at 0x4ec560 is a bare `xor eax,eax / ret 4`, so the default is 0.
    virtual int GetAIValue(int player);
    // Slot 2 of every quest vtable: does this hero satisfy the quest? It
    // returns a BYTE - the defeat-hero body ends `xor al,al` on its guard
    // path and the monster body `mov al,dl`. The hero is NOT const: the
    // artifact and resource leaves call hero::HasArtifact, which is not.
    virtual unsigned char is_satisfied(hero* current_hero);
    // Slot 3: take the quest's price off the hero. The base body at
    // 0x485d80 is a bare `ret 4`; the artifact leaf removes the artifacts
    // and the resource leaf debits the player's treasury. Provisional name.
    virtual void TakePayment(hero* current_hero);
    // Slot 8: the quest-type discriminator. Retail's leaf bodies return a
    // bare constant; see the enumeration proof in the file header.
    virtual int quest_type();
    // Slot 9 / slot 10: the "this target was defeated" notifications. Both
    // default to the shared `ret 8` stub at 0x5bc7e0, so both take two dword
    // arguments and return nothing. The NAMES are provisional inventions -
    // only the argument shape and the two overriding bodies are attested.
    virtual std::string GetRequirementText();
    virtual void NotifyHeroDefeated(int hero_id, int player);
    // Slot 10's monster override (0x56ed40) is decoded but NOT claimed - see
    // the residual note in seerhut.cpp.
    virtual void NotifyMonsterDefeated(TQuestPosition where, int player);
    // Slot 11 / slot 12: the two deserializers. Both are loads - every body
    // in the family calls TAbstractFile slot 1 and then stores what came back
    // into the object - and they differ in format, not direction: slot 11
    // takes a savegame version and reads the wider savegame encodings (a
    // short hero id, a short level), slot 12 takes no version and reads the
    // h3m encodings (a byte hero id, a dword level). The base bodies agree:
    // 0x56cd00 reads a visited flag and two extra fields that 0x56ce50 does
    // not. `Load` / `LoadFromMap` are provisional names for that split.
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
};

class type_experience_quest : public type_quest {
public:
    int required_level;  // +0x40

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual std::string GetRequirementText();
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
};

// Quest type 2. The payload is four bytes read as one block by both
// deserializers - the four primary skills, in the h3m order. They are
// SIGNED: slot 2 widens each one with `movsx`, exactly as hero::stats is
// read everywhere else.
class type_skill_quest : public type_quest {
public:
    signed char required_skills[4];  // +0x40

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual std::string GetRequirementText();
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
};

class type_defeat_hero_quest : public type_quest {
public:
    int map_hero;         // +0x40, the h3m identity slot 12 fills
    int defeated_hero;    // +0x44
    int satisfied_mask;   // +0x48, one bit per player

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual int quest_type();
    virtual void NotifyHeroDefeated(int hero_id, int player);
    virtual void Load(TAbstractFile* file, int version);
};

class type_monster_quest : public type_quest {
public:
    int map_monster;             // +0x40, the h3m identity slot 12 fills
    TQuestPosition position;     // +0x44
    int monster_id;       // +0x48
    int defeated_by;      // +0x4c, -1 until some player kills it

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual int quest_type();
    virtual void Load(TAbstractFile* file, int version);
};

// Quest type 5: the artifacts the hero must hand over. The vector at +0x40
// is a real VC6 Dinkumware std::vector - the `_First == 0 ? 0 : _Last -
// _First` guard its own size() carries is visible in every body that walks
// it, and _First/_Last sit at +0x44/+0x48, which puts the 16-byte container
// exactly at the +0x40 payload slot every other leaf uses.
class type_artifact_quest : public type_quest {
public:
    std::vector<TArtifact> artifacts;  // +0x40

    virtual int GetAIValue(int player);
    virtual unsigned char is_satisfied(hero* current_hero);
    virtual void TakePayment(hero* current_hero);
};

// Quest type 6: parallel creature-type and creature-count vectors.
class type_creature_quest : public type_quest {
public:
    std::vector<int> counts;             // +0x40
    std::vector<TCreatureType> types;    // +0x50

    virtual int GetAIValue(int player);
    virtual unsigned char is_satisfied(hero* current_hero);
};

// Quest type 7: seven resource amounts, read as one 0x1c-byte block.
class type_resource_quest : public type_quest {
public:
    int resources[7];  // +0x40

    virtual int GetAIValue(int player);
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
};

class type_be_hero_quest : public type_quest {
public:
    int required_hero;  // +0x40

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual int quest_type();
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
};

class type_belong_to_player_quest : public type_quest {
public:
    int required_owner;  // +0x40

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual int quest_type();
    virtual std::string GetRequirementText();
    virtual void Load(TAbstractFile* file, int version);
};

#endif  // HOMM3_QUEST_H
