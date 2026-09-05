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

// The two adventure-object names the TQuestGuard / TSeerHut text builders
// read directly. Both are elements of advmgr.h's gAdventureObjectNames -
// 0x6a79ec + 4*215 == 0x6a7d48 for QUEST_GUARD and 0x6a79ec + 4*83 ==
// 0x6a7b38 for SEER - and retail loads each with a bare
// `mov reg, dword ptr [abs]`, which a constant subscript and a scalar
// extern spell identically. They are declared HERE, in the header only
// seerhut.cpp includes, rather than in seerhut.h (which game.h pulls into
// most of the tree) or by including advmgr.h; the two existing declarators
// of exactly this shape - gTreeOfKnowledgeName at index 102 and
// gWitchHutName at index 113 - are the precedent.
DATA(0x006a7d48) extern const char* gQuestGuardName;
DATA(0x006a7b38) extern const char* gSeerName;

// The family's TWO text tables, .data pointer cells at 0x68320c and
// 0x683210. Every quest body that reads either one picks between them on
// type_quest::field_04 and on nothing else, and reaches a string through
// the SAME two strides: 832 bytes per row (field_04's partner +0x38,
// scaled by 13 and then by 64) and 16 bytes per column, taking the text
// from the column's +4 behind the same `_Ptr ? _Ptr : _Nullstr` guard
// every other string read in this compiland carries. That is Dinkumware
// basic_string's own c_str(), so the columns ARE std::strings; there are
// five per quest type (`5 * quest_type()` scaled by 16 is exactly
// retail's `lea eax,[eax+eax*4] / shl eax,4`), and a row is 832/16 == 52
// of them - ten quest types' worth plus two no body here reaches.
// Nothing in the admitted surface writes either cell, so both are
// declared rather than claimed.
DATA(0x0068320c) extern std::string (*gQuestTextA)[52];
DATA(0x00683210) extern std::string (*gQuestTextB)[52];

// The packed map position the defeat-monster quest carries at +0x44 and
// compares field by field in slot 10. Retail reads the same signed 10/10/4
// lanes as the shared type_point, and NH3API independently types both the
// member and slot-10 argument as type_point. Keep the old local spelling as
// an alias so the evidence correction does not obscure the quest discussion.
typedef type_point TQuestPosition;

// Retail's factory at 0x573240 switches on exactly these nine values. The
// class mapping is independently fixed by the slot-8 constants in the nine
// derived vtables (see the file header).
enum EQuestType {
    QUEST_EXPERIENCE = 1,
    QUEST_PRIMARY_SKILLS = 2,
    QUEST_DEFEAT_HERO = 3,
    QUEST_DEFEAT_MONSTER = 4,
    QUEST_ARTIFACTS = 5,
    QUEST_CREATURES = 6,
    QUEST_RESOURCES = 7,
    QUEST_BE_HERO = 8,
    QUEST_BELONG_TO_PLAYER = 9
};

// Every derived class reads its own payload at +0x40, so the base closes at
// 0x40; nothing between the vptr and there is attested.
class type_quest {
public:
    // SLICED 2026-08-21 out of the family's slot-13 serializer, which
    // writes every one of these in this order and is the only body in
    // the image that touches all of them: a byte at +0x04, a byte
    // NARROWED from the dword at +0x38, the dword at +0x3c, and then
    // three std::strings as (int length, length bytes) pairs. The three
    // 16-byte containers close on +0x38 exactly, which is what fixes the
    // whole run.
    //
    // +0x04 is a two-valued selector: slot 7 and slot 14 pick between
    // the two text tables at 0x68320c and 0x683210 on it and on nothing
    // else. +0x38 is the ROW of whichever table that picks, scaled by
    // 832. TSeerHut::getValue now proves +0x3c is the quest deadline, but
    // it keeps an ordinal name until a retail-era source identity fixes the
    // original spelling.
    unsigned char field_04;
    char pad_05[3];
    // PROVISIONAL names, on two pieces of evidence: the order slot 13
    // writes them is the h3m quest record's own
    // firstVisitText / nextVisitText / completedText order, and slot 14
    // back-fills each one from its own column of the text table when it
    // is empty. Nothing in this file proves WHICH dialog reads which,
    // so the roles are read off that order and not off a body.
    std::string proposalText;    // +0x08
    std::string progressText;    // +0x18
    std::string completionText;  // +0x28
    int field_38;
    int field_3c;

    type_quest(unsigned char flags);

    // THE VTABLE IS NOW MODELLED AT ITS REAL WIDTH (2026-08-20). The ten
    // tables each hold FIFTEEN slots, and every declaration below sits at
    // the index its own comment names, so a virtual call through this class
    // emits retail's `call dword ptr [edx + 4*slot]`. The previous model
    // declared only the nine attested methods back to back, which put every
    // one of them at the wrong offset; that was invisible while nothing in
    // the tree dispatched on a `type_quest*`, and stopped being invisible
    // when the four TQuestGuard / TSeerHut text builders turned out to call
    // slot 7. The layout is read straight off the ten tables:
    //
    //   0x64174c 0056cbe0 004ec560 00617d9a 00485d80 00617d9a 00617d9a
    //            00617d9a 00617d9a 00617d9a 005bc7e0 005bc7e0 0056cd00
    //            0056ce50 0056cf70 00617d9a
    //
    // 0x617d9a is `__purecall` (`push 0x19 / call __amsg_exit`), so every
    // slot holding it is PURE in the base; the four slots that are not
    // (1, 3, 9, 10) are the four whose base bodies the file header already
    // priced. Slots 4, 5, 13 and 14 are overridden by all ten leaves and
    // nothing in this tree reaches them yet, so they are placeholders whose
    // only job is to hold the offsets - do NOT invent semantics for them.

    // Slot 0: the destructor. Retail's slot-0 bodies are scalar deleting
    // dtors - 0x571530 is `call <base dtor> / test [ebp+8],1 / call
    // operator delete / mov eax,esi`, the standard `??_G` shape - so the
    // source declared `virtual ~type_quest()`.
    virtual ~type_quest();
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
    // Retail-only 0x56ccb0 (seerhut.obj, 68 B, the row after ~type_quest):
    // `field_3c >= 0 && field_3c < today`, today being the game date's
    // (month * 4 + week - 5) * 7 + day. searchArray::enter_trigger asks it
    // before pricing a quest guard. Provisional name - no DC row carries
    // it; the deadline comparison seerhut.cpp spells inline is its body.
    unsigned char has_expired() const;
    // Slots 4 and 5, IDENTIFIED 2026-08-21: the family's two dialog
    // entry points. Every leaf body is the same shape - take a string
    // off one of the two base getters below, hand it to NormalDialog
    // with iMBType 1 and whatever picture that quest type shows - and
    // the only difference between the two slots is which getter and
    // whether the caller passes a hero. Slot 4 takes one; the skill
    // leaf 0x56dad0 is what types it, reading `[arg + 0x476]`, which is
    // hero::stats. NAMES provisional: no roster row has this arity.
    // ONE OF THESE TWO NAME PAIRS IS INVERTED (found 2026-09-05, bytes,
    // recorded rather than acted on). The two base text getters the leaves
    // take their string off are 0x56d240 and 0x56d310, and the xref graph
    // pairs them one-for-one with slot 4 and slot 5: every DoProposalDialog
    // calls 0x56d240, which reads +0x18, and every DoProgressDialog calls
    // 0x56d310, which reads +0x08. So the slot-4 dialog shows the SECOND
    // string of the h3m triple and slot 5 the FIRST - which contradicts
    // either the proposalText/progressText roles above (assigned off slot
    // 13's write order) or these two slot names. Both name sets are
    // provisional and nothing in the image settles which one to flip;
    // renaming either would touch forty reconstructed leaf bodies for no
    // byte, so the observation is banked here for the lane that closes the
    // two getters.
    virtual void DoProposalDialog(hero* current_hero);
    virtual void DoProgressDialog();
    // Slot 6. Pure in the base and overridden everywhere.
    virtual std::string GetRequirementText();
    // Slot 7: the second string-returning virtual, and the one the four
    // TQuestGuard / TSeerHut quick-info and rollover builders call on
    // `TQuestGuard::quest` (0x572e40, 0x573040, 0x5741b0 and 0x5743e0 all
    // reach it as `mov edx,[esi] / call dword ptr [edx+0x1c]` with a hidden
    // std::string return buffer and no argument). The belong-to-player leaf
    // at 0x572670 is 413 B against its 32-byte slot-6 sibling, and it
    // indexes the player-name table at 0x6a7df8 with the quest's own
    // `required_owner`, so slot 7 is the long, player-facing description
    // where slot 6 is the short requirement line. The NAME is provisional.
    virtual std::string GetQuestDescription();
    // Slot 8: the quest-type discriminator. Retail's leaf bodies return a
    // bare constant; see the enumeration proof in the file header.
    virtual int quest_type();
    // Slot 9 / slot 10: the "this target was defeated" notifications. Both
    // default to the shared `ret 8` stub at 0x5bc7e0, so both take two dword
    // arguments and return nothing. The NAMES are provisional inventions -
    // only the argument shape and the two overriding bodies are attested.
    virtual void NotifyHeroDefeated(int hero_id, int player);
    // Slot 10's monster override is reconstructed at 0x56ed40; see its
    // compiler-generation residual note in seerhut.cpp.
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
    // Slot 13, IDENTIFIED 2026-08-21: the SERIALIZER, and the mirror of
    // the two loads above. Every leaf writes its own payload and then
    // the same base run through TAbstractFile slot 2 (Write): the byte
    // at +0x04, the dword at +0x38 narrowed to a byte, the dword at
    // +0x3c, and the three strings as length-prefixed blocks. The base
    // run is INLINED into all eight leaves - retail emits no call - so
    // it is spelled longhand in each of them here, which is what the
    // bytes have.
    virtual void Save(TAbstractFile* file);
    // Slot 14, IDENTIFIED 2026-08-21: every leaf back-fills the three
    // strings above from columns 0/1/2 of its own text group, and only
    // where the string is still empty. NAME provisional.
    virtual void SetDefaultText();

    // The one way into the two text tables above, and the shape of every
    // read: the column is the only thing that varies between the two
    // slots that use it. Written as a member rather than repeated
    // because retail expands it at every one of its eighteen sites.
    enum {
        QUEST_TEXT_PROPOSAL = 0,
        QUEST_TEXT_PROGRESS = 1,
        QUEST_TEXT_COMPLETION = 2,
        QUEST_TEXT_DESCRIPTION = 3,
        // Column 4, the QUEST-LOG line: TQuestGuard 0x572d60 and TSeerHut
        // 0x574070 are its only readers, and they are the two builders the
        // quest log's own window calls.
        QUEST_TEXT_LOG = 4,
        QUEST_TEXT_COLUMNS = 5,
        // One row-wide line follows the ten five-column quest groups.
        // TQuestGuard::DoEvent is its byte-proven reader when a deadline
        // has passed; unlike the grouped lines, it does not call quest_type.
        QUEST_TEXT_EXPIRED = 50,
        // Column 51, the last of the row's 52 strings and the deadline
        // suffix's format line: get_time_limit_text() is its only reader
        // and takes it at `[row + 0x334]`, i.e. 51 * sizeof(std::string)
        // plus the _Ptr member, with no quest_type() in the address.
        QUEST_TEXT_TIME_LIMIT = 51
    };
    // The five-column group this quest type owns, computed ONCE: slot 14
    // fills three of the columns off one row and retail keeps the group
    // base in a register across all three.
    const std::string* quest_texts()
    {
        const std::string* row =
            field_04 ? gQuestTextA[field_38] : gQuestTextB[field_38];
        return row + QUEST_TEXT_COLUMNS * quest_type();
    }
    // The row-selecting half of quest_texts(), emitted out of line at
    // 0x52e6b0.  Large callers use this body while the smaller slot-14
    // functions inline the same field_04/field_38 calculation.
    const std::string* quest_text_row();
    const std::string& quest_text(int column)
    {
        // The ternary is on the whole INDEXED ROW, not on the table
        // pointer: retail duplicates the `field_38 * 13 << 6` product
        // into both arms and adds the base inside each one, which is
        // what this spelling produces and what hoisting the pointer out
        // does not.
        const std::string* row =
            field_04 ? gQuestTextA[field_38] : gQuestTextB[field_38];
        return row[QUEST_TEXT_COLUMNS * quest_type() + column];
    }

    // Retail 0x56d240 and 0x56d310, the two string getters the dialog
    // pair above calls - both thiscall with a hidden return buffer and
    // no argument, both BELOW this compiland's first claimed row, and
    // neither reconstructed. Declared so the ten dialog bodies can name
    // them. Same provisional standing as the slots that call them.
    std::string GetProposalDialogText();
    std::string GetProgressDialogText();
    // The exact HD structural twin maps this accessor to retail 0x45bad0;
    // its body copies the base's +0x28 completionText member.
    std::string get_completion_text();
    // The exact HD structural twin maps this deadline suffix builder to
    // retail 0x56d040. The two dated dialog getters and the complex skill /
    // creature dialogs are its four callers.
    std::string get_time_limit_text();
};

class type_experience_quest : public type_quest {
public:
    int required_level;  // +0x40

    type_experience_quest(unsigned char flags);

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual std::string GetRequirementText();
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
    virtual void DoProposalDialog(hero* current_hero);
    virtual void DoProgressDialog();
    virtual void Save(TAbstractFile* file);
    virtual std::string GetQuestDescription();
    virtual void SetDefaultText();
};

// Quest type 2. The payload is four bytes read as one block by both
// deserializers - the four primary skills, in the h3m order. They are
// SIGNED: slot 2 widens each one with `movsx`, exactly as hero::stats is
// read everywhere else.
class type_skill_quest : public type_quest {
public:
    signed char required_skills[4];  // +0x40

    type_skill_quest(unsigned char flags);

    // seerhut.obj's shared primary-skill list builder, called with this in
    // ecx and the four-byte payload as its explicit stack argument.
    std::string skill_requirement_text(
        const signed char (&skills)[4]);

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual void DoProposalDialog(hero* current_hero);
    virtual void DoProgressDialog();
    virtual std::string GetRequirementText();
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
    virtual void Save(TAbstractFile* file);
    virtual std::string GetQuestDescription();
    virtual void SetDefaultText();
};

class type_defeat_hero_quest : public type_quest {
public:
    int map_hero;         // +0x40, the h3m identity slot 12 fills
    int defeated_hero;    // +0x44
    int satisfied_mask;   // +0x48, one bit per player

    type_defeat_hero_quest(unsigned char flags);

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual int quest_type();
    virtual void NotifyHeroDefeated(int hero_id, int player);
    virtual void Load(TAbstractFile* file, int version);
    virtual void DoProposalDialog(hero* current_hero);
    virtual void DoProgressDialog();
    virtual void Save(TAbstractFile* file);
    virtual std::string GetQuestDescription();
    virtual std::string GetRequirementText();
    virtual void SetDefaultText();
};

class type_monster_quest : public type_quest {
public:
    int map_monster;             // +0x40, the h3m identity slot 12 fills
    TQuestPosition position;     // +0x44
    int monster_id;       // +0x48
    int defeated_by;      // +0x4c, -1 until some player kills it

    type_monster_quest(unsigned char flags);

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual int quest_type();
    virtual void Load(TAbstractFile* file, int version);
    virtual void DoProposalDialog(hero* current_hero);
    virtual void DoProgressDialog();
    virtual void NotifyMonsterDefeated(TQuestPosition where, int player);
    virtual void Save(TAbstractFile* file);
    virtual std::string GetQuestDescription();
    virtual std::string GetRequirementText();
    virtual void SetDefaultText();
};

// Quest type 5: the artifacts the hero must hand over. The vector at +0x40
// is a real VC6 Dinkumware std::vector - the `_First == 0 ? 0 : _Last -
// _First` guard its own size() carries is visible in every body that walks
// it, and _First/_Last sit at +0x44/+0x48, which puts the 16-byte container
// exactly at the +0x40 payload slot every other leaf uses.
class type_artifact_quest : public type_quest {
public:
    std::vector<TArtifact> artifacts;  // +0x40

    type_artifact_quest(unsigned char flags);

    virtual int GetAIValue(int player);
    virtual unsigned char is_satisfied(hero* current_hero);
    virtual void TakePayment(hero* current_hero);
    // Retail-only 0x56ccb0 (seerhut.obj, 68 B, the row after ~type_quest):
    // `field_3c >= 0 && field_3c < today`, today being the game date's
    // (month * 4 + week - 5) * 7 + day. searchArray::enter_trigger asks it
    // before pricing a quest guard. Provisional name - no DC row carries
    // it; the deadline comparison seerhut.cpp spells inline is its body.
    unsigned char has_expired() const;
    virtual void DoProposalDialog(hero* current_hero);
    virtual void DoProgressDialog();
    virtual std::string GetRequirementText();
    virtual void Save(TAbstractFile* file);
    virtual std::string GetQuestDescription();
    virtual void SetDefaultText();
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
};

// Quest type 6: parallel creature-type and creature-count vectors.
class type_creature_quest : public type_quest {
public:
    std::vector<int> counts;             // +0x40
    std::vector<TCreatureType> types;    // +0x50

    type_creature_quest(unsigned char flags);

    virtual int GetAIValue(int player);
    virtual unsigned char is_satisfied(hero* current_hero);
    virtual void TakePayment(hero* current_hero);
    // Retail-only 0x56ccb0 (seerhut.obj, 68 B, the row after ~type_quest):
    // `field_3c >= 0 && field_3c < today`, today being the game date's
    // (month * 4 + week - 5) * 7 + day. searchArray::enter_trigger asks it
    // before pricing a quest guard. Provisional name - no DC row carries
    // it; the deadline comparison seerhut.cpp spells inline is its body.
    unsigned char has_expired() const;
    virtual void DoProposalDialog(hero* current_hero);
    virtual void DoProgressDialog();
    virtual std::string GetRequirementText();
    virtual void Save(TAbstractFile* file);
    virtual std::string GetQuestDescription();
    virtual void SetDefaultText();
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
};

// Quest type 7: seven resource amounts, read as one 0x1c-byte block.
class type_resource_quest : public type_quest {
public:
    int resources[7];  // +0x40

    type_resource_quest(unsigned char flags);

    virtual int GetAIValue(int player);
    virtual unsigned char is_satisfied(hero* current_hero);
    virtual void TakePayment(hero* current_hero);
    // Retail-only 0x56ccb0 (seerhut.obj, 68 B, the row after ~type_quest):
    // `field_3c >= 0 && field_3c < today`, today being the game date's
    // (month * 4 + week - 5) * 7 + day. searchArray::enter_trigger asks it
    // before pricing a quest guard. Provisional name - no DC row carries
    // it; the deadline comparison seerhut.cpp spells inline is its body.
    unsigned char has_expired() const;
    virtual void DoProposalDialog(hero* current_hero);
    virtual void DoProgressDialog();
    virtual std::string GetRequirementText();
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
    virtual void Save(TAbstractFile* file);
    virtual std::string GetQuestDescription();
    virtual void SetDefaultText();
};

class type_be_hero_quest : public type_quest {
public:
    int required_hero;  // +0x40

    type_be_hero_quest(unsigned char flags);

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual int quest_type();
    virtual void Load(TAbstractFile* file, int version);
    virtual void LoadFromMap(TAbstractFile* file);
    virtual void DoProposalDialog(hero* current_hero);
    virtual void DoProgressDialog();
    virtual std::string GetQuestDescription();
    virtual std::string GetRequirementText();
    virtual void SetDefaultText();
};

class type_belong_to_player_quest : public type_quest {
public:
    int required_owner;  // +0x40

    type_belong_to_player_quest(unsigned char flags);

    virtual unsigned char is_satisfied(hero* current_hero);
    virtual int quest_type();
    virtual std::string GetRequirementText();
    virtual std::string GetQuestDescription();
    virtual void Load(TAbstractFile* file, int version);
    virtual void DoProposalDialog(hero* current_hero);
    virtual void DoProgressDialog();
    virtual void Save(TAbstractFile* file);
    virtual void SetDefaultText();
};

#endif  // HOMM3_QUEST_H
