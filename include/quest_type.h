// Shared scalar domain used by game objects and map serialization.
#ifndef HOMM3_QUEST_TYPE_H
#define HOMM3_QUEST_TYPE_H

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

#endif
