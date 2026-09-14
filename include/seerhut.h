// seerhut.h - canonical retail quest-guard and seer-hut layouts.
#ifndef HOMM3_SEERHUT_H
#define HOMM3_SEERHUT_H

#include <string>
#include <vector>
#include <va.h>
#include "quest.h"

// E:\gamedcs\seerhut.cpp:50, dc 0x12cd28
unsigned char initializeSeerHutText();

class AdventureMapWindow;
class Hero;
class NewmapCell;
struct type_point;

// Complete's seer-hut name table replaced Dreamcast's const-char pointer
// array with Dinkumware strings; TSeerHut::GetName keeps the shared header
// accessor boundary over the revised storage.
DATA(0x0069fab8) extern std::vector<std::string>* g_seerHutNamesPointer;

#pragma pack(push, 1)

// Retail's constructor family and NewfullMap vector walks prove the packed
// five-byte guard record: a quest pointer followed by the visited-player mask.
class AbstractFile;

class QuestGuard {
public:
    Quest* m_quest;
    unsigned char m_visitedPlayers;
    // readObject (0x502e00) retains constructor 0x572b50 on its quest-guard
    // local. This Complete-only class has no DC inline declaration; keep
    // one ordinary constructor definition in seerhut.cpp.
    QuestGuard();
    void doEvent(Hero* currentHero, bool humanPlayer,
                 NewmapCell* eventCell, type_point point);
    void read(AbstractFile* infile);
    std::string questGuardFn00572E40(int player);
    std::string questGuardFn00573040(int player);
    std::string questGuardFn00572D60();
    int save(AbstractFile* outfile);
    // Complete retains the Dreamcast TSeerHut predicate on the new shared
    // quest-guard base.  DoQuestLog proves that its final two tests are the
    // visited-player bit followed by a fresh quest-pointer read.
    unsigned char questActiveforPlayer(
        const unsigned char playerNum) const
    {
        return m_quest
            && m_quest->questTexts()[Quest::QUEST_TEXT_LOG].length()
            && (m_visitedPlayers & (1 << playerNum))
            && m_quest;
    }
    int load(AbstractFile* infile, int saveVersion);
};
SIZE(QuestGuard, 0x5);

// Dreamcast names this reward domain on TSeerData. Retail's ten-way helper
// dispatch and the three adjacent users preserve the same 0..10 values even
// though the x86 build split the reward into its own 12-byte record.
// Before normalization (type): TSeerRewardType.
enum SeerRewardType {
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
// Before normalization (type): TSeerReward.
struct SeerReward {
public:
    // DC TPrimarySkill values. Kept nested because the canonical global
    // secondary-skill header is intentionally outside game.h's wide include
    // closure; these are exactly the four case labels this record needs.
// Before normalization (type): TSeerReward::TPrimarySkillType.
    enum PrimarySkillType {
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
    SeerReward() : m_rewardType(0) {}
    int getValue(const Hero* currentHero);
    void giveReward(Hero* currentHero, bool humanPlayer);
    int getRewardExtra(const Hero* thisHero);
};
SIZE(SeerReward, 0xc);

// Before normalization (type): TSeerData.
struct SeerData {
    Quest* m_quest;              // Prior role: quest.
    unsigned char m_visitedPlayers;  // Prior role: visitedPlayers.
    SeerReward m_reward;            // Prior role: reward.
};
SIZE(SeerData, 0x11);

// Retail indexes NewfullMap::SeerHutList with a 0x13 stride. Preserve the
// Dreamcast-proven private TSeerData base with the revised Complete payload.
class SeerHut : private SeerData {
public:
    // readObject's SEER arm tests the private base's quest pointer before
    // registering the deserialized record in the +0xb0 pool. These consumers
    // retain friendship while ordinary access stays on TSeerHut's surface.
    friend class NewfullMap;
    friend class AdventureMapWindow;

private:
    // Dreamcast preserves this private source boundary. Complete replaces
    // the VMU-era text lookup inside it, but retail expands the revised body
    // into DoSeerEvent's no-quest arm.
    void doEmptyDialog();
    // Dreamcast's next private helper owns the completion dialog and reward
    // application. Complete revises both models, while retaining the source
    // boundary inside DoSeerEvent's human arm.
    inline void doCompletionDialog(Hero* currentHero, bool humanPlayer);
    // Dreamcast proves this nested no-local switch helper as the first call
    // made by DoCompletionDialog. Complete retains the boundary while
    // shifting the primary-skill icon domain by one.
    inline int getRewardType();
    signed char m_nameIndex;

public:
    // DC save 0x12d8c0 writes the old object in member order. Retail load
    // 0x574a90's version<28 arm reads artifact + reward, then preserves
    // this second legacy byte at +0x12 (store 0x574b01). The surrounding
    // bytes retain QuestCompleted/playerInfo/Type/NameIndex ordering.
    // Complete retains this byte through save/load even
    // though completion behavior now belongs to the quest object.
    unsigned char m_completedByPlayer;
    // E:\gamedcs\SeerHut.h:108, dc 0xf4b38
    VA(0x00573580, 0x13)
    SeerHut()
    {
        m_quest = 0;
        m_visitedPlayers = 0;
        m_nameIndex = 0;
        m_completedByPlayer = 0;
    }
    // Dreamcast supplies the surviving public name/signature; retail's
    // Complete-era body replaces the monolith with the virtual quest family.
    void doSeerEvent(Hero* currentHero, bool humanPlayer);
    int getValue(Hero* currentHero);
    void read(AbstractFile* infile);

private:
    void load(AbstractFile* infile, int saveVersion);

public:
    std::string seerHutFn005741B0(int player) const;
    std::string seerHutFn005743E0(int player) const;
    std::string getSeerLogText();
    // Dreamcast names QuestActiveforPlayer as a const byte-returning TSeerHut
    // helper.  Its old body tested playerGivenQuest and then !QuestCompleted.
    // Complete's virtual quest model replaces the latter byte with a live quest
    // and a non-empty quest-log line, but retail keeps the same final visited-bit
    // and fresh quest-pointer tests.  Keep both pool-specific spellings: retail
    // forms a named quest_text_row pointer for SeerHutList, while the exact
    // UpdateQuestLogButton sibling proves quest_texts()[LOG] for guards.
    // E:\gamedcs\SeerHut.h:112, dc 0x3250
    unsigned char questActiveforPlayer(
        const unsigned char playerNum) const
    {
        Quest* thisQuest = m_quest;
        if (!thisQuest)
            return 0;

        const std::string* questTexts = thisQuest->questTextRow()
            + Quest::QUEST_TEXT_COLUMNS * thisQuest->questType();
        return questTexts[Quest::QUEST_TEXT_LOG].length()
            && (m_visitedPlayers & (1 << playerNum))
            && m_quest;
    }
    // E:\gamedcs\seerhut.h:121, dc 0x20244. Retail corroborates the signed
    // NameIndex load, 16-byte vector stride and inlined c_str() fallback.
    const char* getName() const
    {
        return (*g_seerHutNamesPointer)[m_nameIndex].c_str();
    }

private:
    // 0x573fd0, the SeerHutList twin of TQuestGuard::save and reached the
    // same way from NewfullMap::Save. Declared separately because the
    // seer and guard records each own their serialization interface.
    int save(AbstractFile* outfile);
};
SIZE(SeerHut, 0x13);

#pragma pack(pop)

#endif  // HOMM3_SEERHUT_H
