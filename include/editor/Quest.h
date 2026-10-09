// Quest.h - what a Seer's Hut or a Quest Guard asks of a hero
// (C:\Dev\Heroes 3 Exp 2\Editor\Quest.cpp, by its RTTI). A quest takes a
// visitor (TQuest::TVisitor: the cloner, the writer, the equivalency
// testers and the quest dialogs); the defeat-hero and defeat-monster
// quests name their target by its link id.
//
// Ported so far: the declarations the map needs.
#ifndef HOMM3_EDITOR_QUEST_H
#define HOMM3_EDITOR_QUEST_H

#include <set>

#include "artifact.h"

// The quest base (RTTI TQuest: its vtable is the scalar deleting
// destructor, then the pure accept slot; a quest's accept, 0x495199,
// hands the visitor its own type).
class TQuest {
public:
    class TVisitor;

    virtual ~TQuest() {}
    virtual void accept(TVisitor& visitor) const = 0;
};

// A quest whose target is a linkable object: its third slot, pure here,
// yields the target's link id (both defeat quests share 0x4951cb). The map
// clears such quests when their target leaves (h3maped 0x4278a6).
class TLinkedQuest : public TQuest {
public:
    virtual unsigned int getLinkID() const = 0;
};

// Bring these artifacts: the vtable, then the artifacts in a tree (the
// map's playability check walks them, 0x428c73).
class TQuestBringArtifacts : public TQuest {
public:
    const std::multiset<TArtifact>& getArtifacts() const { return _m_artifacts; }

private:
    std::multiset<TArtifact> _m_artifacts;
};

// Defeat a hero or a monster: the target's link id follows the vtable (the
// map reads it in place when it places a quest location, 0x42757b).
class TQuestDefeatHero : public TLinkedQuest {
public:
    virtual unsigned int getLinkID() const;

    unsigned int getHeroLinkID() const { return _m_heroLinkID; }

private:
    unsigned int _m_heroLinkID;
};

class TQuestDefeatMonster : public TLinkedQuest {
public:
    virtual unsigned int getLinkID() const;

    unsigned int getMonsterLinkID() const { return _m_monsterLinkID; }

private:
    unsigned int _m_monsterLinkID;
};

#endif  /* HOMM3_EDITOR_QUEST_H */
