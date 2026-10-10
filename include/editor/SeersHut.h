// SeersHut.h - a Seer's Hut: a quest location that grants a reward
// (C:\Dev\Heroes 3 Exp 2\Editor\SeersHut.cpp, by its RTTI; Loki h3maped
// object 29).
//
// RTTI TSeersHut <- TQuestLocation <- virtual TGameObject: the location's
// members, then the owned reward (+0x44), the vtordisp and TGameObject.
// Rewards are visited (TReward::TVisitor: an empty body per kind, no
// virtual destructor; VC6 lists the overloads in its vtable in reverse).
// Loki's T*Reward classes are Complete's TReward*; the simple bonus
// rewards share TSimpleBonusReward<max>.
#ifndef HOMM3_EDITOR_SEERSHUT_H
#define HOMM3_EDITOR_SEERSHUT_H

#include <memory>

#include "artifact_type.h"
#include "armygrp.h"
#include "herospec.h"
#include "primaryskill.h"
#include "secondaryskill.h"
#include "editor/Army.h"
#include "editor/GameResource.h"
#include "editor/QuestLocation.h"

class TSeersHut : public TQuestLocation {
public:
    class TRewardExperience;
    class TRewardMana;
    class TRewardMorale;
    class TRewardLuck;
    class TRewardResource;
    class TRewardPrimarySkill;
    class TRewardSecondarySkill;
    class TRewardArtifact;
    class TRewardSpell;
    class TRewardCreature;

    class TReward;

    TSeersHut(const TSeersHut& other);
    TSeersHut(const TObjectType& objType);
    TSeersHut(const TObjectType& objType, TRawIStream* pIStream, int version);
    virtual ~TSeersHut();

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const;
    virtual void resetQuestTerms();

    const TReward* getPReward() const { return _m_pReward.get(); }
    void setReward(std::auto_ptr<TReward> pReward);
    void clearReward();

private:
    std::auto_ptr<TReward> _m_pReward;
};

class TSeersHut::TReward {
public:
    class TVisitor {
    public:
        virtual void visit(const TRewardExperience& reward) {}
        virtual void visit(const TRewardMana& reward) {}
        virtual void visit(const TRewardMorale& reward) {}
        virtual void visit(const TRewardLuck& reward) {}
        virtual void visit(const TRewardResource& reward) {}
        virtual void visit(const TRewardPrimarySkill& reward) {}
        virtual void visit(const TRewardSecondarySkill& reward) {}
        virtual void visit(const TRewardArtifact& reward) {}
        virtual void visit(const TRewardSpell& reward) {}
        virtual void visit(const TRewardCreature& reward) {}
    };

    virtual void accept(TVisitor* pVisitor) const = 0;

    std::auto_ptr<TReward> clone() const;
    static bool equivalent(const TReward& lhs, const TReward& rhs);
};

template<int s_kMaxBonus>
class TSimpleBonusReward : public TSeersHut::TReward {
public:
    TSimpleBonusReward(int bonus) : _m_bonus(bonus) {}

    int getBonus() const { return _m_bonus; }

private:
    int _m_bonus;
};

template<int s_kMaxBonus>
inline bool operator==(const TSimpleBonusReward<s_kMaxBonus>& lhs, const TSimpleBonusReward<s_kMaxBonus>& rhs)
{
    return lhs.getBonus() == rhs.getBonus();
}

class TSeersHut::TRewardExperience : public TSimpleBonusReward<99999999> {
public:
    TRewardExperience(int bonus) : TSimpleBonusReward<99999999>(bonus) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }
};

class TSeersHut::TRewardMana : public TSimpleBonusReward<999> {
public:
    TRewardMana(int bonus) : TSimpleBonusReward<999>(bonus) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }
};

class TSeersHut::TRewardMorale : public TSimpleBonusReward<3> {
public:
    TRewardMorale(int bonus) : TSimpleBonusReward<3>(bonus) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }
};

class TSeersHut::TRewardLuck : public TSimpleBonusReward<3> {
public:
    TRewardLuck(int bonus) : TSimpleBonusReward<3>(bonus) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }
};

class TSeersHut::TRewardResource : public TSeersHut::TReward {
public:
    enum { s_kMaxQuantity = 99999 };

    TRewardResource(TGameResourceType type, unsigned int quantity);

    TGameResourceType getType() const { return _m_type; }
    unsigned int getQuantity() const { return _m_quantity; }

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    friend bool operator==(const TRewardResource& lhs, const TRewardResource& rhs)
    {
        return lhs._m_type == rhs._m_type && lhs._m_quantity == rhs._m_quantity;
    }

private:
    TGameResourceType _m_type;
    unsigned int _m_quantity;
};

class TSeersHut::TRewardPrimarySkill : public TSimpleBonusReward<99> {
public:
    TRewardPrimarySkill(TPrimarySkill skill, int bonus);

    TPrimarySkill getSkill() const { return _m_skill; }

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    friend bool operator==(const TRewardPrimarySkill& lhs, const TRewardPrimarySkill& rhs)
    {
        return static_cast<const TSimpleBonusReward<99>&>(lhs) == rhs && lhs._m_skill == rhs._m_skill;
    }

private:
    TPrimarySkill _m_skill;
};

class TSeersHut::TRewardSecondarySkill : public TSeersHut::TReward {
public:
    TRewardSecondarySkill(TSecondarySkill skill, TSkillMastery mastery);

    TSecondarySkill getSkill() const { return _m_skill; }
    TSkillMastery getMastery() const { return _m_mastery; }

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    friend bool operator==(const TRewardSecondarySkill& lhs, const TRewardSecondarySkill& rhs)
    {
        return lhs._m_skill == rhs._m_skill && lhs._m_mastery == rhs._m_mastery;
    }

private:
    TSecondarySkill _m_skill;
    TSkillMastery _m_mastery;
};

class TSeersHut::TRewardArtifact : public TSeersHut::TReward {
public:
    TRewardArtifact(TArtifact artifact);

    TArtifact getArtifact() const { return _m_artifact; }

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    friend bool operator==(const TRewardArtifact& lhs, const TRewardArtifact& rhs)
    {
        return lhs._m_artifact == rhs._m_artifact;
    }

private:
    TArtifact _m_artifact;
};

class TSeersHut::TRewardSpell : public TSeersHut::TReward {
public:
    TRewardSpell(SpellID spell);

    SpellID getSpell() const { return _m_spell; }

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    friend bool operator==(const TRewardSpell& lhs, const TRewardSpell& rhs)
    {
        return lhs._m_spell == rhs._m_spell;
    }

private:
    SpellID _m_spell;
};

class TSeersHut::TRewardCreature : public TSeersHut::TReward {
public:
    TRewardCreature(const TCreatureStack& creatureStack) : _m_creatureStack(creatureStack) {}

    const TCreatureStack& getCreatureStack() const { return _m_creatureStack; }

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    friend bool operator==(const TRewardCreature& lhs, const TRewardCreature& rhs)
    {
        return lhs._m_creatureStack == rhs._m_creatureStack;
    }

private:
    TCreatureStack _m_creatureStack;
};

#endif  /* HOMM3_EDITOR_SEERSHUT_H */
