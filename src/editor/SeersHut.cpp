// SeersHut.cpp - Loki h3maped object 29: the seer's hut, the artifact its
// quest asks for and the reward it gives. Rewards are cloned, compared
// and written through their visitors (the cloner, the left and right
// dispatchers and the writer, as VictoryCondition.cpp does for the
// conditions). Assert lines come from the retail immediates.
#include <assert.h>
#include <new>

#include "adventureobjecttype.h"
#include "artifact.h"
#include "exceptions.h"
#include "editor/RawStream.h"
#include "editor/SeersHut.h"

namespace {

// The reward kinds as the map file numbers them; the names other than
// eRewardNone and kNumRewardTypes (the asserts') are not recorded.
enum TRewardType {
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
    eRewardCreature = 10,
    kNumRewardTypes = 11
};

class TRewardCloner : public TSeersHut::TReward::TVisitor {
public:
    TRewardCloner(void* (*pfnAllocator)(unsigned int))
        : _m_pfnAllocator(pfnAllocator), _m_pClone(NULL)
    {
#line 68
        assert(pfnAllocator != __null);
    }
    ~TRewardCloner()
    {
#line 70
        assert(_m_pClone == __null);
    }

private:
    template<class T>
    void _clone(const T& reward)
    {
        _m_pClone = new ((*_m_pfnAllocator)(sizeof(T))) T(reward);
    }

public:
    TSeersHut::TReward* release()
    {
        TSeersHut::TReward* result = _m_pClone;
        _m_pClone = NULL;
        return result;
    }

    virtual void visit(const TSeersHut::TExperienceReward& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TManaReward& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TMoraleReward& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TLuckReward& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TResourceReward& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TPrimarySkillReward& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TSecondarySkillReward& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TArtifactReward& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TSpellReward& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TCreatureReward& reward) { _clone(reward); }

private:
    void* (*_m_pfnAllocator)(unsigned int);
    TSeersHut::TReward* _m_pClone;
};

class TRewardRHSDispatcherBase : public TSeersHut::TReward::TVisitor {
public:
    virtual void visit(const TSeersHut::TExperienceReward& rhs) {}
    virtual void visit(const TSeersHut::TManaReward& rhs) {}
    virtual void visit(const TSeersHut::TMoraleReward& rhs) {}
    virtual void visit(const TSeersHut::TLuckReward& rhs) {}
    virtual void visit(const TSeersHut::TResourceReward& rhs) {}
    virtual void visit(const TSeersHut::TPrimarySkillReward& rhs) {}
    virtual void visit(const TSeersHut::TSecondarySkillReward& rhs) {}
    virtual void visit(const TSeersHut::TArtifactReward& rhs) {}
    virtual void visit(const TSeersHut::TSpellReward& rhs) {}
    virtual void visit(const TSeersHut::TCreatureReward& rhs) {}
};

template<class T>
class TRewardRHSDispatcher : public TRewardRHSDispatcherBase {
public:
    TRewardRHSDispatcher(const T& lhs) : _m_lhs(lhs), _m_bEquivalent(false) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

    virtual void visit(const T& rhs) { _m_bEquivalent = _m_lhs == rhs; }

private:
    const T& _m_lhs;
    bool _m_bEquivalent;
};

class TRewardLHSDispatcher : public TSeersHut::TReward::TVisitor {
public:
    TRewardLHSDispatcher(const TSeersHut::TReward& rhs) : _m_rhs(rhs) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

private:
    template<class T>
    void _rhsDispatch(const T& lhs)
    {
        TRewardRHSDispatcher<T> rhsDispatcher(lhs);
        _m_rhs.accept(&rhsDispatcher);
        _m_bEquivalent = rhsDispatcher.getBEquivalent();
    }

public:
    virtual void visit(const TSeersHut::TExperienceReward& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TSeersHut::TManaReward& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TSeersHut::TMoraleReward& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TSeersHut::TLuckReward& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TSeersHut::TResourceReward& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TSeersHut::TPrimarySkillReward& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TSeersHut::TSecondarySkillReward& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TSeersHut::TArtifactReward& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TSeersHut::TSpellReward& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TSeersHut::TCreatureReward& lhs) { _rhsDispatch(lhs); }

private:
    const TSeersHut::TReward& _m_rhs;
    bool _m_bEquivalent;
};

class TRewardWriter : public TSeersHut::TReward::TVisitor {
public:
    TRewardWriter(TRawOStream* pOStream) : _m_pOStream(pOStream)
    {
#line 98
        assert(pOStream != __null);
    }

    virtual void visit(const TSeersHut::TExperienceReward& reward);
    virtual void visit(const TSeersHut::TManaReward& reward);
    virtual void visit(const TSeersHut::TMoraleReward& reward);
    virtual void visit(const TSeersHut::TLuckReward& reward);
    virtual void visit(const TSeersHut::TResourceReward& reward);
    virtual void visit(const TSeersHut::TPrimarySkillReward& reward);
    virtual void visit(const TSeersHut::TSecondarySkillReward& reward);
    virtual void visit(const TSeersHut::TArtifactReward& reward);
    virtual void visit(const TSeersHut::TSpellReward& reward);
    virtual void visit(const TSeersHut::TCreatureReward& reward);

private:
    void writeType(TRewardType type)
    {
#line 117
        assert(type > eRewardNone && type < kNumRewardTypes);
        *_m_pOStream << (signed char) type;
    }

    TRawOStream* _m_pOStream;
};

void TRewardWriter::visit(const TSeersHut::TExperienceReward& reward)
{
    writeType(eRewardExperience);
    *_m_pOStream << static_cast< long >( reward.getBonus() );
}

void TRewardWriter::visit(const TSeersHut::TManaReward& reward)
{
    writeType(eRewardMana);
    *_m_pOStream << static_cast< long >( reward.getBonus() );
}

void TRewardWriter::visit(const TSeersHut::TMoraleReward& reward)
{
    writeType(eRewardMorale);
    *_m_pOStream << static_cast< signed char >( reward.getBonus() );
}

void TRewardWriter::visit(const TSeersHut::TLuckReward& reward)
{
    writeType(eRewardLuck);
    *_m_pOStream << static_cast< signed char >( reward.getBonus() );
}

void TRewardWriter::visit(const TSeersHut::TResourceReward& reward)
{
    writeType(eRewardResource);
    *_m_pOStream << static_cast< signed char >( reward.getType() ) << static_cast< long >( reward.getQuantity() );
}

void TRewardWriter::visit(const TSeersHut::TPrimarySkillReward& reward)
{
    writeType(eRewardPrimarySkill);
    *_m_pOStream << static_cast< signed char >( reward.getSkill() ) << static_cast< signed char >( reward.getBonus() );
}

void TRewardWriter::visit(const TSeersHut::TSecondarySkillReward& reward)
{
    writeType(eRewardSecondarySkill);
    *_m_pOStream << static_cast< signed char >( reward.getSkill() )
                 << static_cast< signed char >( reward.getMastery() );
}

void TRewardWriter::visit(const TSeersHut::TArtifactReward& reward)
{
    writeType(eRewardArtifact);
    *_m_pOStream << static_cast< signed char >( reward.getArtifact() );
}

void TRewardWriter::visit(const TSeersHut::TSpellReward& reward)
{
    writeType(eRewardSpell);
    *_m_pOStream << static_cast< signed char >( reward.getSpell() );
}

void TRewardWriter::visit(const TSeersHut::TCreatureReward& reward)
{
    writeType(eRewardCreature);
    *_m_pOStream << reward.getCreatureStack();
}

}  // namespace

TSeersHut::TReward* TSeersHut::TReward::clone(const TReward& reward, void* (*pfnAllocator)(unsigned int))
{
#line 272
    assert(pfnAllocator != NULL);
    TRewardCloner cloner(pfnAllocator);
    reward.accept(&cloner);
    return cloner.release();
}

bool TSeersHut::TReward::equivalent(const TReward& lhs, const TReward& rhs)
{
    TRewardLHSDispatcher lhsDispatcher(rhs);
    lhs.accept(&lhsDispatcher);
    return lhsDispatcher.getBEquivalent();
}

TSeersHut::TResourceReward::TResourceReward(TGameResourceType type, unsigned int quantity)
    : _m_type(type), _m_quantity(quantity)
{
#line 295
    assert(type >= 0 && type < kNumGameResourceTypes);
    assert(quantity >= 1 && quantity <= s_kMaxQuantity);
}

TSeersHut::TPrimarySkillReward::TPrimarySkillReward(TPrimarySkill skill, int bonus)
    : TSimpleBonusReward<99>(bonus), _m_skill(skill)
{
#line 307
    assert(skill >= 0 && skill < kNumPrimarySkills);
}

TSeersHut::TSecondarySkillReward::TSecondarySkillReward(TSecondarySkill skill, TSkillMastery mastery)
    : _m_skill(skill), _m_mastery(mastery)
{
#line 318
    assert(skill >= 0 && skill < kNumSecSkills);
    assert(mastery >= eMasteryBasic && mastery < eMasteryBasic + kNumMasteries);
}

TSeersHut::TArtifactReward::TArtifactReward(TArtifact artifact) : _m_artifact(artifact)
{
#line 328
    assert(artifact >= 0 && artifact < kNumArtifacts);
}

TSeersHut::TSpellReward::TSpellReward(SpellID spell) : _m_spell(spell)
{
#line 337
    assert(spell >= 0 && spell < kNumSpells);
}

TSeersHut::TSeersHut(const TSeersHut& other)
    : TGameObject(other), _m_questArtifact(other._m_questArtifact), _m_pQuestReward(NULL)
{
    if (other._m_pQuestReward != NULL)
        if ((_m_pQuestReward = TReward::clone(*other._m_pQuestReward, ::operator new)) == NULL)
#line 350
            throw TAllocationFailure(__FILE__, __LINE__);
}

TSeersHut::TSeersHut(const TObjectType& objType)
    : TGameObject(objType), _m_questArtifact(eArtifactNone), _m_pQuestReward(NULL)
{
#line 359
    assert(objType.getType() == SEER);
}

TSeersHut::TSeersHut(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), _m_pQuestReward(NULL)
{
#line 367
    assert(objType.getType() == SEER);
    assert(pIStream != NULL);
    try {
        signed char questArtifact;
        *pIStream >> questArtifact;
        setQuestArtifact(TArtifact(questArtifact));
        signed char rewardType;
        *pIStream >> rewardType;
#line 377
        assert(rewardType >= 0 && rewardType < kNumRewardTypes);
        switch (rewardType) {
        case eRewardExperience: {
            long bonus;
            *pIStream >> bonus;
            TExperienceReward reward(bonus);
            setQuestReward(&reward);
            break;
        }
        case eRewardMana: {
            long bonus;
            *pIStream >> bonus;
            TManaReward reward(bonus);
            setQuestReward(&reward);
            break;
        }
        case eRewardMorale: {
            signed char bonus;
            *pIStream >> bonus;
            TMoraleReward reward(bonus);
            setQuestReward(&reward);
            break;
        }
        case eRewardLuck: {
            signed char bonus;
            *pIStream >> bonus;
            TLuckReward reward(bonus);
            setQuestReward(&reward);
            break;
        }
        case eRewardResource: {
            signed char type;
            long quantity;
            *pIStream >> type >> quantity;
            TResourceReward reward((TGameResourceType) type, quantity);
            setQuestReward(&reward);
            break;
        }
        case eRewardPrimarySkill: {
            signed char skill;
            signed char bonus;
            *pIStream >> skill >> bonus;
            TPrimarySkillReward reward((TPrimarySkill) skill, bonus);
            setQuestReward(&reward);
            break;
        }
        case eRewardSecondarySkill: {
            signed char skill;
            signed char mastery;
            *pIStream >> skill >> mastery;
            TSecondarySkillReward reward((TSecondarySkill) skill, (TSkillMastery) mastery);
            setQuestReward(&reward);
            break;
        }
        case eRewardArtifact: {
            signed char artifact;
            *pIStream >> artifact;
            TArtifactReward reward((TArtifact) artifact);
            setQuestReward(&reward);
            break;
        }
        case eRewardSpell: {
            signed char spell;
            *pIStream >> spell;
            TSpellReward reward((SpellID) spell);
            setQuestReward(&reward);
            break;
        }
        case eRewardCreature: {
            TCreatureStack creatureStack;
            *pIStream >> creatureStack;
            TCreatureReward reward(creatureStack);
            setQuestReward(&reward);
            break;
        }
        }
        signed char aReserved[2];
        *pIStream >> aReserved;
    } catch (...) {
        delete _m_pQuestReward;
        throw;
    }
}

TSeersHut::~TSeersHut()
{
    delete _m_pQuestReward;
}

void TSeersHut::setQuestArtifact(TArtifact newQuestArtifact)
{
#line 496
    assert(newQuestArtifact >= eArtifactNone && newQuestArtifact < kNumArtifacts);
    assert(newQuestArtifact == eArtifactNone || ( akArtifactTraits[ newQuestArtifact ].m_class & ArtifactClassSpecial ) == 0);
    _m_questArtifact = newQuestArtifact;
    if (_m_questArtifact == eArtifactNone && _m_pQuestReward != NULL) {
        delete _m_pQuestReward;
        _m_pQuestReward = NULL;
    }
}

void TSeersHut::setQuestReward(const TReward* pNewReward)
{
    if ((_m_pQuestReward == NULL && pNewReward == NULL)
        || (_m_pQuestReward != NULL && pNewReward != NULL && TReward::equivalent(*_m_pQuestReward, *pNewReward)))
        return;
    if (_m_pQuestReward != NULL) {
        delete _m_pQuestReward;
        _m_pQuestReward = NULL;
    }
    if (pNewReward != NULL) {
#line 523
        assert(_m_questArtifact != eArtifactNone);
        if ((_m_pQuestReward = TReward::clone(*pNewReward, ::operator new)) == NULL)
#line 526
            throw TAllocationFailure(__FILE__, __LINE__);
    }
}

bool TSeersHut::isCustomized() const
{
    return _m_pQuestReward != NULL || _m_questArtifact != eArtifactNone;
}

void TSeersHut::write(TRawOStream* pOStream) const
{
#line 539
    assert(pOStream != NULL);
    *pOStream << (signed char) _m_questArtifact;
    if (_m_pQuestReward != NULL) {
#line 545
        assert(_m_questArtifact != eArtifactNone);
        TRewardWriter writer(pOStream);
        _m_pQuestReward->accept(&writer);
    } else {
        *pOStream << (signed char) eRewardNone;
    }
    signed char aReserved[2];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}
