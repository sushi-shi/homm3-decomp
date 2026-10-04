// Shared scalar domain used by game objects and map serialization.
#ifndef HOMM3_SEER_REWARD_TYPE_H
#define HOMM3_SEER_REWARD_TYPE_H

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

#endif
