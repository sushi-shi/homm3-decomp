// Quest.h - what a Seer's Hut or a Quest Guard asks of a hero
// (C:\Dev\Heroes 3 Exp 2\Editor\Quest.cpp, by its RTTI). A quest takes a
// visitor (TQuest::TVisitor: the cloner, the writer, the equivalency
// testers and the quest dialogs); the defeat-hero and defeat-monster
// quests name their target by its link id. The kinds follow the map
// format's quest numbers 1..9.
#ifndef HOMM3_EDITOR_QUEST_H
#define HOMM3_EDITOR_QUEST_H

#include <map>
#include <memory>
#include <set>

#include "armygrp.h"
#include "artifact.h"
#include "primaryskill.h"
#include "editor/Array.h"
#include "editor/GameResource.h"
#include "editor/Player.h"

class TQuestAchieveExperienceLevel;
class TQuestAchievePrimarySkillLevel;
class TQuestDefeatHero;
class TQuestDefeatMonster;
class TQuestBringArtifacts;
class TQuestBringCreatures;
class TQuestBringResources;
class TQuestBeASpecificHero;
class TQuestBelongToASpecificPlayer;

// The quest base (RTTI TQuest: its vtable is the scalar deleting
// destructor, then the pure accept slot).
class TQuest {
public:
    // Every visit does nothing unless a visitor overrides it (one shared
    // empty body, 0x411916, fills TQuest::TVisitor's vtable). VC6 lists
    // the overloads in its vtable in reverse.
    class TVisitor {
    public:
        virtual void visit(const TQuestAchieveExperienceLevel& quest) {}
        virtual void visit(const TQuestAchievePrimarySkillLevel& quest) {}
        virtual void visit(const TQuestDefeatHero& quest) {}
        virtual void visit(const TQuestDefeatMonster& quest) {}
        virtual void visit(const TQuestBringArtifacts& quest) {}
        virtual void visit(const TQuestBringCreatures& quest) {}
        virtual void visit(const TQuestBringResources& quest) {}
        virtual void visit(const TQuestBeASpecificHero& quest) {}
        virtual void visit(const TQuestBelongToASpecificPlayer& quest) {}
    };

    virtual ~TQuest() {}
    virtual void accept(TVisitor& visitor) const = 0;

    std::auto_ptr<TQuest> clone() const;
    static bool equivalent(const TQuest& lhs, const TQuest& rhs);
};

// A quest whose target is a linkable object: its third slot, pure here,
// yields the target's link id (both defeat quests share 0x4951cb). The map
// clears such quests when their target leaves (h3maped 0x4278a6).
class TLinkedQuest : public TQuest {
public:
    virtual unsigned int getLinkID() const = 0;
};

class TQuestAchieveExperienceLevel : public TQuest {
public:
    TQuestAchieveExperienceLevel(int level);

    virtual void accept(TVisitor& visitor) const;

    int getLevel() const { return _m_level; }

    friend bool operator==(const TQuestAchieveExperienceLevel& lhs, const TQuestAchieveExperienceLevel& rhs)
    {
        return lhs._m_level == rhs._m_level;
    }

private:
    int _m_level;
};

class TQuestAchievePrimarySkillLevel : public TQuest {
public:
    TQuestAchievePrimarySkillLevel(const TArray<int, kNumPrimarySkills>& aLevel);

    virtual void accept(TVisitor& visitor) const;

    int getLevel(TPrimarySkill primarySkill) const;
    const TArray<int, kNumPrimarySkills>& getLevels() const { return _m_aLevel; }

    friend bool operator==(const TQuestAchievePrimarySkillLevel& lhs, const TQuestAchievePrimarySkillLevel& rhs)
    {
        return lhs._m_aLevel == rhs._m_aLevel;
    }

private:
    TArray<int, kNumPrimarySkills> _m_aLevel;
};

// Defeat a hero or a monster: the target's link id follows the vtable (the
// map reads it in place when it places a quest location, 0x42757b).
class TQuestDefeatHero : public TLinkedQuest {
public:
    TQuestDefeatHero(unsigned int heroLinkID);

    virtual void accept(TVisitor& visitor) const;
    virtual unsigned int getLinkID() const;

    unsigned int getHeroLinkID() const { return _m_heroLinkID; }

    friend bool operator==(const TQuestDefeatHero& lhs, const TQuestDefeatHero& rhs)
    {
        return lhs._m_heroLinkID == rhs._m_heroLinkID;
    }

private:
    unsigned int _m_heroLinkID;
};

class TQuestDefeatMonster : public TLinkedQuest {
public:
    TQuestDefeatMonster(unsigned int monsterLinkID);

    virtual void accept(TVisitor& visitor) const;
    virtual unsigned int getLinkID() const;

    unsigned int getMonsterLinkID() const { return _m_monsterLinkID; }

    friend bool operator==(const TQuestDefeatMonster& lhs, const TQuestDefeatMonster& rhs)
    {
        return lhs._m_monsterLinkID == rhs._m_monsterLinkID;
    }

private:
    unsigned int _m_monsterLinkID;
};

// Bring these artifacts: the vtable, then the artifacts in a tree (the
// map's playability check walks them, 0x428c73).
class TQuestBringArtifacts : public TQuest {
public:
    TQuestBringArtifacts(const std::multiset<TArtifact>& artifacts);

    virtual void accept(TVisitor& visitor) const;

    const std::multiset<TArtifact>& getArtifacts() const { return _m_artifacts; }

    friend bool operator==(const TQuestBringArtifacts& lhs, const TQuestBringArtifacts& rhs)
    {
        return lhs._m_artifacts == rhs._m_artifacts;
    }

private:
    std::multiset<TArtifact> _m_artifacts;
};

// Bring these creatures: a count per creature type (the map file stores
// each pair as two shorts).
class TQuestBringCreatures : public TQuest {
public:
    typedef std::map<TCreatureType, int> TCreatures;

    TQuestBringCreatures(const TCreatures& creatures);

    virtual void accept(TVisitor& visitor) const;

    const TCreatures& getCreatures() const { return _m_creatures; }

    friend bool operator==(const TQuestBringCreatures& lhs, const TQuestBringCreatures& rhs)
    {
        return lhs._m_creatures == rhs._m_creatures;
    }

private:
    TCreatures _m_creatures;
};

class TQuestBringResources : public TQuest {
public:
    TQuestBringResources(const TArray<int, kNumGameResourceTypes>& aQuantity);

    virtual void accept(TVisitor& visitor) const;

    int getQuantity(TGameResourceType type) const;
    const TArray<int, kNumGameResourceTypes>& getQuantities() const { return _m_aQuantity; }

    friend bool operator==(const TQuestBringResources& lhs, const TQuestBringResources& rhs)
    {
        return lhs._m_aQuantity == rhs._m_aQuantity;
    }

private:
    TArray<int, kNumGameResourceTypes> _m_aQuantity;
};

class TQuestBeASpecificHero : public TQuest {
public:
    TQuestBeASpecificHero(int heroID);

    virtual void accept(TVisitor& visitor) const;

    int getHeroID() const { return _m_heroID; }

    friend bool operator==(const TQuestBeASpecificHero& lhs, const TQuestBeASpecificHero& rhs)
    {
        return lhs._m_heroID == rhs._m_heroID;
    }

private:
    int _m_heroID;
};

class TQuestBelongToASpecificPlayer : public TQuest {
public:
    TQuestBelongToASpecificPlayer(TPlayer player);

    virtual void accept(TVisitor& visitor) const;

    TPlayer getPlayer() const { return _m_player; }

    friend bool operator==(const TQuestBelongToASpecificPlayer& lhs, const TQuestBelongToASpecificPlayer& rhs)
    {
        return lhs._m_player == rhs._m_player;
    }

private:
    TPlayer _m_player;
};

#endif  /* HOMM3_EDITOR_QUEST_H */
