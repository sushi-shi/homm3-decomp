// quest.h - retail's seer-hut quest hierarchy (compiland seerhut.obj).
// HAND-OWNED. Class layouts are NOT fabricated from method symbols;
// prototypes stay comments until a retail layout is proven.
//
// Retail replaced the Dreamcast port's TSeerHut::Do*Dialog / GetRewardType
// monolith with a virtual `type_quest` family: ten classes, each with its own
// 13-slot vtable at 0x64174c, 0x641788, 0x6417c4, 0x641800, 0x64183c,
// 0x641878, 0x6418b4, 0x6418f0, 0x64192c and 0x641968. Only the two slots
// this file needs are declared; the rest of the roster, and every byte of the
// base object below +0x40, are still unattested.
//
// This is deliberately NOT in seerhut.h: that header reaches game.h,
// advmgr.h and mapcell.h and therefore most of the tree, and adding ten
// class definitions to that include closure is exactly the perturbation the
// include-set residual class warns about. seerhut.cpp is the only consumer.
#ifndef HOMM3_QUEST_H
#define HOMM3_QUEST_H

#include <va.h>

class hero;

// Every derived class reads its own payload at +0x40, so the base closes at
// 0x40; nothing between the vptr and there is attested.
class type_quest {
public:
    unsigned char pad_4[0x3c];

    // Slot 2 of every quest vtable: does this hero satisfy the quest?
    virtual int is_satisfied(const hero* current_hero);
    // Slot 8: the quest-type discriminator. Retail's four leaf bodies return
    // a bare constant, and the constants line up with the classes the vtable
    // symbols name - 3 defeat-hero, 4 monster, 8 be-hero, 9 belong-to-player.
    virtual int quest_type();
};

class type_experience_quest : public type_quest {
public:
    int required_level;  // +0x40

    virtual int is_satisfied(const hero* current_hero);
};

class type_defeat_hero_quest : public type_quest {
public:
    virtual int quest_type();
};

class type_monster_quest : public type_quest {
public:
    virtual int quest_type();
};

class type_be_hero_quest : public type_quest {
public:
    int required_hero;  // +0x40

    virtual int is_satisfied(const hero* current_hero);
    virtual int quest_type();
};

class type_belong_to_player_quest : public type_quest {
public:
    int required_owner;  // +0x40

    virtual int is_satisfied(const hero* current_hero);
    virtual int quest_type();
};

#endif  // HOMM3_QUEST_H
