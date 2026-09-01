// seerhut.h - canonical retail quest-guard and seer-hut layouts.
#ifndef HOMM3_SEERHUT_H
#define HOMM3_SEERHUT_H

#include <string>
#include <vector>
#include <va.h>

class type_quest;
class TAdventureMapWindow;
class hero;
class NewmapCell;
struct type_point;

// Complete's seer-hut name table replaced Dreamcast's const-char pointer
// array with Dinkumware strings; TSeerHut::GetName keeps the shared header
// accessor boundary over the revised storage.
DATA(0x0069fab8) extern std::vector<std::string>* gpSeerHutNames;

#pragma pack(push, 1)

// Retail's constructor family and NewfullMap vector walks prove the packed
// five-byte base: a quest pointer followed by the visited-player mask.
class TAbstractFile;

class TQuestGuard {
public:
    type_quest* quest;
    unsigned char visitedPlayers;

    // NOT inline for this view: readObject (0x502e00) CALLS the constructor
    // at 0x572b50 on its quest-guard local instead of expanding the two
    // stores, so the declaration retail's mapcell.cpp saw was this one and
    // the definition lived in seerhut.cpp. The inline body stays for the
    // views whose own call sites do expand it.
    TQuestGuard();

    // The quest-guard adventure event. The exact HD structural twin fixes
    // the name and argument list; retail independently proves the four
    // stack arguments, the quest virtual slots, and the EraseAndFizzle tail.
    void DoEvent(hero* current_hero, bool human_player,
                 NewmapCell* eventCell, type_point point);

    // The h3m reader, reached from readObject's QUEST_GUARD arm with the
    // stream as its one argument. DECLARED, not defined: the body is an
    // unclaimed carve row outside this compiland.
    void read(TAbstractFile* infile);

    // TWO TEXT BUILDERS, NOT ONE (2026-08-20). 0x572e40 and 0x573040 are
    // 511 B each and byte-identical apart from ONE relocation - the
    // separator literal they append, "\n\n" (0x6603b0) against " "
    // (0x660330). advManager::QuickInfo calls the FIRST and
    // advManager::SetRolloverText the second; the carve's own caller-derived
    // names say so (`game_137c0_sub00_172e40` against
    // `game_b150_sub07_173040`, 0x137c0 being QuickInfo and 0xb150
    // SetRolloverText) and both bodies confirm it by their separator. The
    // TSeerHut pair below splits the same way, crosswise.
    std::string QuestGuardFn_00572E40(int player);
    std::string QuestGuardFn_00573040(int player);
    // 0x572d60, 224 B, carved and unclaimed. NULLARY where the pair above
    // takes a player: TQuestLogWindow::UpdateQuestLocator pushes only the
    // hidden return buffer and calls it on the QuestGuardList element,
    // then strcpy's its c_str() into gText. Provisional name.
    std::string QuestGuardFn_00572D60();
    // Reached from NewfullMap::Save, which calls it on every QuestGuardList
    // element with the stream as its one argument. DECLARED, not defined:
    // the body is an unclaimed carve row outside this compiland.
    int save(TAbstractFile* outfile);
    // Complete retains the Dreamcast TSeerHut predicate on the new shared
    // quest-guard base.  DoQuestLog proves that its final two tests are the
    // visited-player bit followed by a fresh quest-pointer read.
    unsigned char QuestActiveforPlayer(
        const unsigned char playerNum) const;
#ifdef HOMM3_QUEST_GUARD_LOAD_DECLS
    int load(TAbstractFile* infile, int saveVersion);
#endif

protected:
    // TSeerHut initializes the shared bytes in its own body; retail's store
    // order proves that its reward constructor runs before those assignments.
    explicit TQuestGuard(int) {}
};
SIZE(TQuestGuard, 0x5);

// Dreamcast names this reward domain on TSeerData. Retail's ten-way helper
// dispatch and the three adjacent users preserve the same 0..10 values even
// though the x86 build split the reward into its own 12-byte record.
enum TSeerRewardType {
    eRewardNone = 0,
    eRewardExperience = 1,
    eRewardMana = 2,
    eRewardMorale = 3,
    eRewardLuck = 4,
    eRewardResource = 5,
    eRewardPrimarySkill = 6,
    eRewardSecondarySkill = 7,
    eRewardArtifact = 8,
    eRewardSpell = 9,
    eRewardCreature = 10
};

// Bytes +5..+0x10 of TSeerHut. The constructor initializes the common type
// word; the remaining eight bytes are the selected reward's payload.
struct TSeerReward {
    // DC TPrimarySkill values. Kept nested because the canonical global
    // secondary-skill header is intentionally outside game.h's wide include
    // closure; these are exactly the four case labels this record needs.
    enum TPrimarySkillType {
        ePriSkillAttack = 0,
        ePriSkillDefense = 1,
        ePriSkillPower = 2,
        ePriSkillKnowledge = 3
    };

    int rewardType;
    union {
        char payload[8];
        int dwords[2];
        struct {
            signed int bonus : 8;
        } signedLow;
        struct {
            int first;
            signed int bonus : 8;
        } signedHigh;
        struct {
            int skillType;
            int bonus;
        } secondarySkill;
        struct {
            int resourceType;
            int quantity;
        } resource;
        struct {
            int skillType;
            signed int bonus : 8;
        } primarySkill;
        struct {
            int creatureType;
            signed int count : 16;
            signed int : 16;
        } creature;
    } value;

    TSeerReward() : rewardType(0) {}
    int getValue(const hero* currentHero);
    void giveReward(hero* currentHero, bool humanPlayer);
    int GetRewardExtra(const hero* thisHero);
};
SIZE(TSeerReward, 0xc);

// Retail indexes NewfullMap::SeerHutList with a 0x13 stride and the constructor
// writes every named state byte below. This is the one class layout used by
// seerhut, mapcell, and advmgr; there is no TU-private vector projection.
class TSeerHut : private TQuestGuard {
    // readObject's SEER arm tests the base's `quest` pointer on the local it
    // just deserialized, before deciding whether to register it in the
    // +0xb0 pool. Friendship rather than a public base: only the map reader
    // reaches across, and everything else here still goes through TSeerHut's
    // own surface.
    friend class NewfullMap;
    friend class TAdventureMapWindow;

    // Dreamcast preserves this private source boundary. Complete replaces
    // the VMU-era text lookup inside it, but retail expands the revised body
    // into DoSeerEvent's no-quest arm.
    void DoEmptyDialog();
    // Dreamcast's next private helper owns the completion dialog and reward
    // application. Complete revises both models, while retaining the source
    // boundary inside DoSeerEvent's human arm.
    inline void DoCompletionDialog(hero* current_hero, bool human_player);
    // Dreamcast proves this nested no-local switch helper as the first call
    // made by DoCompletionDialog. Complete retains the boundary while
    // shifting the primary-skill icon domain by one.
    inline int GetRewardType();

public:
    TSeerReward reward;
    signed char NameIndex;
    unsigned char field_12;

    TSeerHut();
    // E:\gamedcs\seerhut.h:121, dc 0x20244. Retail corroborates the signed
    // NameIndex load, 16-byte vector stride and inlined c_str() fallback.
    const char* GetName() const
    {
        return (*gpSeerHutNames)[NameIndex].c_str();
    }
    // Dreamcast supplies the surviving public name/signature; retail's
    // Complete-era body replaces the monolith with the virtual quest family.
    void DoSeerEvent(hero* current_hero, bool human_player);
    // The AI appraisal of an unvisited or active hut. Retail fixes the
    // hero ABI and all quest/reward calls; the HD twin supplies the name.
    int getValue(hero* currentHero);

    // The SeerHutList twin of TQuestGuard::read, reached the same way from
    // readObject's SEER arm. Declared separately because the TQuestGuard
    // base is private here.
    int read(TAbstractFile* infile);
    // 0x574a90, `ret 8` - the savegame reader, called by NewfullMap::Load on
    // every element of the list it has just resized.
    int load(TAbstractFile* infile, int saveVersion);

    // The TQuestGuard pair's twin, and it splits CROSSWISE: 0x5741b0 and
    // 0x5743e0 are 556 B each and differ only in the separator relocation,
    // with 0x5741b0 taking " " (SetRolloverText) and 0x5743e0 taking "\n\n"
    // (QuickInfo). Same carve-name evidence: `game_b150_sub08_1741b0`
    // against `game_137c0_sub01_1743e0`.
    std::string SeerHutFn_005741B0(int player);
    std::string SeerHutFn_005743E0(int player);
    // The SeerHutList twin of TQuestGuard::QuestGuardFn_00572D60, reached
    // from the other arm of the quest log's list split. The exact HD
    // structural twin supplies the later method name after retail fixes the
    // receiver and nullary string-return ABI.
    std::string getSeerLogText();
    // 0x573fd0, the SeerHutList twin of TQuestGuard::save and reached the
    // same way from NewfullMap::Save. Declared separately because the
    // TQuestGuard base is private here.
    int save(TAbstractFile* outfile);
    // Dreamcast names this source boundary on TSeerHut.  Complete's quest
    // log applies the same predicate to both of its quest pools.
    unsigned char QuestActiveforPlayer(
        const unsigned char playerNum) const;
};
SIZE(TSeerHut, 0x13);

#pragma pack(pop)

#endif  // HOMM3_SEERHUT_H
