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
DATA(0x0069fab8) extern std::vector<std::string>* g_seerHutNamesPointer;

#pragma pack(push, 1)

// Retail's constructor family and NewfullMap vector walks prove the packed
// five-byte base: a quest pointer followed by the visited-player mask.
class TAbstractFile;

class TQuestGuard {
public:
    type_quest* m_quest;
    unsigned char m_visitedPlayers;

    TQuestGuard();

    void doEvent(hero* currentHero, bool humanPlayer,
                 NewmapCell* eventCell, type_point point);

    void read(TAbstractFile* infile);

    std::string questGuardFn00572E40(int player);
    std::string questGuardFn00573040(int player);
    std::string questGuardFn00572D60();
    int save(TAbstractFile* outfile);
    // Complete retains the Dreamcast TSeerHut predicate on the new shared
    // quest-guard base.  DoQuestLog proves that its final two tests are the
    // visited-player bit followed by a fresh quest-pointer read.
    unsigned char questActiveforPlayer(
        const unsigned char playerNum) const;
    int load(TAbstractFile* infile, int saveVersion);

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

    int m_rewardType;
    union {
        char m_payload[8];
        int m_dwords[2];
        struct {
            signed int m_bonus : 8;
        } m_signedLow;
        struct {
            int m_first;
            signed int m_bonus : 8;
        } m_signedHigh;
        struct {
            int m_skillType;
            int m_bonus;
        } m_secondarySkill;
        struct {
            int m_resourceType;
            int m_quantity;
        } m_resource;
        struct {
            int m_skillType;
            signed int m_bonus : 8;
        } m_primarySkill;
        struct {
            int m_creatureType;
            signed int m_count : 16;
            signed int : 16;
        } m_creature;
    } m_value;

    TSeerReward() : m_rewardType(0) {}
    int getValue(const hero* currentHero);
    void giveReward(hero* currentHero, bool humanPlayer);
    int getRewardExtra(const hero* thisHero);
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
    void doEmptyDialog();
    // Dreamcast's next private helper owns the completion dialog and reward
    // application. Complete revises both models, while retaining the source
    // boundary inside DoSeerEvent's human arm.
    inline void doCompletionDialog(hero* currentHero, bool humanPlayer);
    // Dreamcast proves this nested no-local switch helper as the first call
    // made by DoCompletionDialog. Complete retains the boundary while
    // shifting the primary-skill icon domain by one.
    inline int getRewardType();

public:
    TSeerReward m_reward;

private:
    signed char m_nameIndex;

public:
    // Original: CompletedByPlayer (Dreamcast TSeerHut +0x11).
    // DC save 0x12d8c0 writes the old object in member order. Retail load
    // 0x574a90's version<28 arm reads artifact + reward, then preserves
    // this second legacy byte at +0x12 (store 0x574b01). The surrounding
    // bytes retain QuestCompleted/playerInfo/Type/NameIndex ordering.
    // Previously field_12; Complete retains it through save/load even
    // though completion behavior now belongs to the quest object.
    unsigned char m_completedByPlayer;

    TSeerHut();
    // E:\gamedcs\seerhut.h:121, dc 0x20244. Retail corroborates the signed
    // NameIndex load, 16-byte vector stride and inlined c_str() fallback.
    const char* getName() const
    {
        return (*g_seerHutNamesPointer)[m_nameIndex].c_str();
    }
    // Dreamcast supplies the surviving public name/signature; retail's
    // Complete-era body replaces the monolith with the virtual quest family.
    void doSeerEvent(hero* currentHero, bool humanPlayer);
    int getValue(hero* currentHero);

    // The SeerHutList twin of TQuestGuard::read, reached the same way from
    // readObject's SEER arm. Declared separately because the TQuestGuard
    // base is private here. VOID, corrected 2026-09-05 when the body came
    // in: retail's 0x574610 sets no return register at any exit, exactly as
    // TQuestGuard::read does, and its one caller discards the result.
    void read(TAbstractFile* infile);

private:
    void load(TAbstractFile* infile, int saveVersion);

public:
    std::string seerHutFn005741B0(int player) const;
    std::string seerHutFn005743E0(int player) const;
    std::string getSeerLogText();
    // Dreamcast names this source boundary on TSeerHut.  Complete's quest
    // log applies the same predicate to both of its quest pools.
    unsigned char questActiveforPlayer(
        const unsigned char playerNum) const;

private:
    // 0x573fd0, the SeerHutList twin of TQuestGuard::save and reached the
    // same way from NewfullMap::Save. Declared separately because the
    // TQuestGuard base is private here.
    int save(TAbstractFile* outfile);
};
SIZE(TSeerHut, 0x13);

#pragma pack(pop)

#endif  // HOMM3_SEERHUT_H
