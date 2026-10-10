// Quest.cpp - what a Seer's Hut or a Quest Guard asks of a hero (h3maped
// 0x494d73..0x495ae9; Complete only). Quests are cloned and compared
// through their visitors: the cloner, and an equivalency tester that
// dispatches on the left quest and then on the right one.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Quest.h"

namespace {

template<class T>
class TQuestRHSEquivalencyTester : public TQuest::TVisitor {
public:
    TQuestRHSEquivalencyTester(const T& lhs) : _m_lhs(lhs), _m_bEquivalent(false) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

    virtual void visit(const T& rhs) { _m_bEquivalent = _m_lhs == rhs; }

private:
    const T& _m_lhs;
    bool _m_bEquivalent;
};

class TQuestEquivalencyTester : public TQuest::TVisitor {
public:
    TQuestEquivalencyTester(const TQuest& rhs) : _m_rhs(rhs), _m_bEquivalent(false) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

private:
    template<class T>
    void _test(const T& lhs)
    {
        TQuestRHSEquivalencyTester<T> tester(lhs);
        _m_rhs.accept(tester);
        _m_bEquivalent = tester.getBEquivalent();
    }

public:
    virtual void visit(const TQuestAchieveExperienceLevel& lhs) { _test(lhs); }
    virtual void visit(const TQuestAchievePrimarySkillLevel& lhs) { _test(lhs); }
    virtual void visit(const TQuestDefeatHero& lhs) { _test(lhs); }
    virtual void visit(const TQuestDefeatMonster& lhs) { _test(lhs); }
    virtual void visit(const TQuestBringArtifacts& lhs) { _test(lhs); }
    virtual void visit(const TQuestBringCreatures& lhs) { _test(lhs); }
    virtual void visit(const TQuestBringResources& lhs) { _test(lhs); }
    virtual void visit(const TQuestBeASpecificHero& lhs) { _test(lhs); }
    virtual void visit(const TQuestBelongToASpecificPlayer& lhs) { _test(lhs); }

private:
    const TQuest& _m_rhs;
    bool _m_bEquivalent;
};

class TQuestCloner : public TQuest::TVisitor {
public:
    std::auto_ptr<TQuest> clone(const TQuest& quest)
    {
        quest.accept(*this);
        return _m_pClone;
    }

private:
    template<class T>
    void _clone(const T& quest)
    {
        _m_pClone = std::auto_ptr<TQuest>(new T(quest));
    }

public:
    virtual void visit(const TQuestAchieveExperienceLevel& quest) { _clone(quest); }
    virtual void visit(const TQuestAchievePrimarySkillLevel& quest) { _clone(quest); }
    virtual void visit(const TQuestDefeatHero& quest) { _clone(quest); }
    virtual void visit(const TQuestDefeatMonster& quest) { _clone(quest); }
    virtual void visit(const TQuestBringArtifacts& quest) { _clone(quest); }
    virtual void visit(const TQuestBringCreatures& quest) { _clone(quest); }
    virtual void visit(const TQuestBringResources& quest) { _clone(quest); }
    virtual void visit(const TQuestBeASpecificHero& quest) { _clone(quest); }
    virtual void visit(const TQuestBelongToASpecificPlayer& quest) { _clone(quest); }

private:
    std::auto_ptr<TQuest> _m_pClone;
};

}  // namespace

VA(0x00494f47, 0x28)
bool TQuest::equivalent(const TQuest& lhs, const TQuest& rhs)
{
    TQuestEquivalencyTester tester(rhs);
    lhs.accept(tester);
    return tester.getBEquivalent();
}

VA(0x00494fdb, 0x54)
std::auto_ptr<TQuest> TQuest::clone() const
{
    TQuestCloner cloner;
    return cloner.clone(*this);
}

VA(0x004950c6, 0x12)
TQuestAchieveExperienceLevel::TQuestAchieveExperienceLevel(int level) : _m_level(level)
{
}

VA(0x004950d8, 0xf)
void TQuestAchieveExperienceLevel::accept(TVisitor& visitor) const
{
    visitor.visit(*this);
}

VA(0x00495104, 0x3d)
TQuestAchievePrimarySkillLevel::TQuestAchievePrimarySkillLevel(const TArray<int, kNumPrimarySkills>& aLevel)
    : _m_aLevel(aLevel)
{
}

VA(0x00495141, 0xf)
void TQuestAchievePrimarySkillLevel::accept(TVisitor& visitor) const
{
    visitor.visit(*this);
}

VA(0x0049517c, 0xb)
int TQuestAchievePrimarySkillLevel::getLevel(TPrimarySkill primarySkill) const
{
    return _m_aLevel[primarySkill];
}

VA(0x00495187, 0x12)
TQuestDefeatHero::TQuestDefeatHero(unsigned int heroLinkID) : _m_heroLinkID(heroLinkID)
{
}

VA(0x00495199, 0xf)
void TQuestDefeatHero::accept(TVisitor& visitor) const
{
    visitor.visit(*this);
}

VA(0x004951cb, 0x4)
unsigned int TQuestDefeatHero::getLinkID() const
{
    return getHeroLinkID();
}

VA(0x004951cf, 0x12)
TQuestDefeatMonster::TQuestDefeatMonster(unsigned int monsterLinkID) : _m_monsterLinkID(monsterLinkID)
{
}

// Folded by /OPT:ICF onto the victory conditions' accept of the same slot.
void TQuestDefeatMonster::accept(TVisitor& visitor) const
{
    visitor.visit(*this);
}

// Folded by /OPT:ICF onto TQuestDefeatHero::getLinkID.
unsigned int TQuestDefeatMonster::getLinkID() const
{
    return getMonsterLinkID();
}

VA(0x004951e1, 0x5b)
TQuestBringArtifacts::TQuestBringArtifacts(const std::multiset<TArtifact>& artifacts) : _m_artifacts(artifacts)
{
}

void TQuestBringArtifacts::accept(TVisitor& visitor) const
{
    visitor.visit(*this);
}

VA(0x00495288, 0x5b)
TQuestBringCreatures::TQuestBringCreatures(const TCreatures& creatures) : _m_creatures(creatures)
{
}

void TQuestBringCreatures::accept(TVisitor& visitor) const
{
    visitor.visit(*this);
}

VA(0x0049532f, 0x3d)
TQuestBringResources::TQuestBringResources(const TArray<int, kNumGameResourceTypes>& aQuantity)
    : _m_aQuantity(aQuantity)
{
}

void TQuestBringResources::accept(TVisitor& visitor) const
{
    visitor.visit(*this);
}

// Folded by /OPT:ICF onto TQuestAchievePrimarySkillLevel::getLevel.
int TQuestBringResources::getQuantity(TGameResourceType type) const
{
    return _m_aQuantity[type];
}

VA(0x00495398, 0x12)
TQuestBeASpecificHero::TQuestBeASpecificHero(int heroID) : _m_heroID(heroID)
{
}

void TQuestBeASpecificHero::accept(TVisitor& visitor) const
{
    visitor.visit(*this);
}

VA(0x004953aa, 0x12)
TQuestBelongToASpecificPlayer::TQuestBelongToASpecificPlayer(TPlayer player) : _m_player(player)
{
}

VA(0x004953bc, 0xe)
void TQuestBelongToASpecificPlayer::accept(TVisitor& visitor) const
{
    visitor.visit(*this);
}
