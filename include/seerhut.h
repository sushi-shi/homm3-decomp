// seerhut.h - canonical retail quest-guard and seer-hut layouts.
#ifndef HOMM3_SEERHUT_H
#define HOMM3_SEERHUT_H

#include <string>
#include <va.h>

class type_quest;

#pragma pack(push, 1)

// Retail's constructor family and NewfullMap vector walks prove the packed
// five-byte base: a quest pointer followed by the visited-player mask.
class TQuestGuard {
public:
    type_quest* quest;
    unsigned char visitedPlayers;

    TQuestGuard() : quest(0), visitedPlayers(0) {}

    std::string QuestGuardFn_00573040(int player);
    // 0x572d60, 224 B, carved and unclaimed. NULLARY where the pair above
    // takes a player: TQuestLogWindow::UpdateQuestLocator pushes only the
    // hidden return buffer and calls it on the QuestGuardList element,
    // then strcpy's its c_str() into gText. Provisional name.
    std::string QuestGuardFn_00572D60();

protected:
    // TSeerHut initializes the shared bytes in its own body; retail's store
    // order proves that its reward constructor runs before those assignments.
    explicit TQuestGuard(int) {}
};
SIZE(TQuestGuard, 0x5);

// Bytes +5..+0x10 of TSeerHut. The constructor initializes the common type
// word; the remaining eight bytes are the selected reward's payload.
struct TSeerReward {
    int rewardType;
    union {
        char payload[8];
        int dwords[2];
    } value;

    TSeerReward() : rewardType(0) {}
};
SIZE(TSeerReward, 0xc);

// Retail indexes NewfullMap::SeerHutList with a 0x13 stride and the constructor
// writes every named state byte below. This is the one class layout used by
// seerhut, mapcell, and advmgr; there is no TU-private vector projection.
class TSeerHut : private TQuestGuard {
public:
    TSeerReward reward;
    signed char NameIndex;
    unsigned char field_12;

    TSeerHut();

    std::string SeerHutFn_005741B0(unsigned char player);
    // 0x574070, 312 B, carved and unclaimed - the SeerHutList twin of
    // TQuestGuard::QuestGuardFn_00572D60 and reached from the same
    // quest-log body, on the other arm of its SeerHutList/QuestGuardList
    // split. Provisional name.
    std::string SeerHutFn_00574070();
};
SIZE(TSeerHut, 0x13);

#pragma pack(pop)

#endif  // HOMM3_SEERHUT_H
