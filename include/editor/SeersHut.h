// SeersHut.h - the seer's hut and its quest rewards (Loki h3maped
// SeersHut.cpp, object 29). A hut asks for an artifact (+4) and pays a
// TReward (+8). Rewards are visited (TReward::TVisitor, no virtual
// destructor, slots in retail's order); the simple bonus rewards share
// TSimpleBonusReward<max> (SeersHut.h asserts "bonus >= 1 && bonus <=
// s_kMaxBonus").
#ifndef HOMM3_EDITOR_SEERSHUT_H
#define HOMM3_EDITOR_SEERSHUT_H

#include "artifact_type.h"
#include "armygrp.h"
#include "herospec.h"
#include "primaryskill.h"
#include "secondaryskill.h"
#include "editor/Army.h"
#include "editor/GameObject.h"
#include "editor/GameResource.h"

class TRawIStream;
class TRawOStream;

class TSeersHut : public virtual TGameObject {
public:
    class TExperienceReward;
    class TManaReward;
    class TMoraleReward;
    class TLuckReward;
    class TResourceReward;
    class TPrimarySkillReward;
    class TSecondarySkillReward;
    class TArtifactReward;
    class TSpellReward;
    class TCreatureReward;

    class TReward {
    public:
        class TVisitor {
        public:
            virtual void visit(const TExperienceReward& reward) = 0;
            virtual void visit(const TManaReward& reward) = 0;
            virtual void visit(const TMoraleReward& reward) = 0;
            virtual void visit(const TLuckReward& reward) = 0;
            virtual void visit(const TResourceReward& reward) = 0;
            virtual void visit(const TPrimarySkillReward& reward) = 0;
            virtual void visit(const TSecondarySkillReward& reward) = 0;
            virtual void visit(const TArtifactReward& reward) = 0;
            virtual void visit(const TSpellReward& reward) = 0;
            virtual void visit(const TCreatureReward& reward) = 0;
        };

        virtual void accept(TVisitor* pVisitor) const = 0;

        static TReward* clone(const TReward& reward, void* (*pfnAllocator)(unsigned int));
        static bool equivalent(const TReward& lhs, const TReward& rhs);
    };

    TSeersHut(const TObjectType& objType);
    TSeersHut(const TObjectType& objType, TRawIStream* pIStream, int version);
    TSeersHut(const TSeersHut& other);
    virtual ~TSeersHut();

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;

    TArtifact getQuestArtifact() const { return _m_questArtifact; }
    void setQuestArtifact(TArtifact newArtifact);
    const TReward* getPQuestReward() const { return _m_pQuestReward; }
    TReward* getPQuestReward() { return _m_pQuestReward; }
    void setQuestReward(const TReward* pNewReward);

private:
    TArtifact _m_questArtifact;
    TReward* _m_pQuestReward;
};

template<int s_kMaxBonus>
class TSimpleBonusReward : public TSeersHut::TReward {
public:
    TSimpleBonusReward(int bonus);

    int getBonus() const { return _m_bonus; }

private:
    int _m_bonus;
};

class TSeersHut::TExperienceReward : public TSimpleBonusReward<99999999> {
public:
    TExperienceReward(int bonus) : TSimpleBonusReward<99999999>(bonus) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }
};

class TSeersHut::TManaReward : public TSimpleBonusReward<999> {
public:
    TManaReward(int bonus) : TSimpleBonusReward<999>(bonus) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }
};

class TSeersHut::TMoraleReward : public TSimpleBonusReward<3> {
public:
    TMoraleReward(int bonus) : TSimpleBonusReward<3>(bonus) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }
};

class TSeersHut::TLuckReward : public TSimpleBonusReward<3> {
public:
    TLuckReward(int bonus) : TSimpleBonusReward<3>(bonus) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }
};

class TSeersHut::TResourceReward : public TSeersHut::TReward {
public:
    TResourceReward(TGameResourceType type, unsigned int quantity);

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    TGameResourceType getType() const { return _m_type; }
    unsigned int getQuantity() const { return _m_quantity; }

private:
    TGameResourceType _m_type;
    unsigned int _m_quantity;
};

class TSeersHut::TPrimarySkillReward : public TSimpleBonusReward<99> {
public:
    TPrimarySkillReward(TPrimarySkill skill, int bonus);

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    TPrimarySkill getSkill() const { return _m_skill; }

private:
    TPrimarySkill _m_skill;
};

class TSeersHut::TSecondarySkillReward : public TSeersHut::TReward {
public:
    TSecondarySkillReward(TSecondarySkill skill, TSkillMastery mastery);

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    TSecondarySkill getSkill() const { return _m_skill; }
    TSkillMastery getMastery() const { return _m_mastery; }

private:
    TSecondarySkill _m_skill;
    TSkillMastery _m_mastery;
};

class TSeersHut::TArtifactReward : public TSeersHut::TReward {
public:
    TArtifactReward(TArtifact artifact);

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    TArtifact getArtifact() const { return _m_artifact; }

private:
    TArtifact _m_artifact;
};

class TSeersHut::TSpellReward : public TSeersHut::TReward {
public:
    TSpellReward(SpellID spell);

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    SpellID getSpell() const { return _m_spell; }

private:
    SpellID _m_spell;
};

class TSeersHut::TCreatureReward : public TSeersHut::TReward {
public:
    TCreatureReward(const TCreatureStack& creatureStack) : _m_creatureStack(creatureStack) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    const TCreatureStack& getCreatureStack() const { return _m_creatureStack; }

private:
    TCreatureStack _m_creatureStack;
};

#endif  /* HOMM3_EDITOR_SEERSHUT_H */
