// SeersHut.cpp - a Seer's Hut: its quest and the reward it grants (h3maped
// 0x4b6002..0x4b732f; Loki h3maped object 29). Rewards are written,
// compared and cloned through their visitors (the writer, the equivalency
// testers and the cloner, as Quest.cpp does for the quests). The map
// format writes a reward as its kind (0 for none, then 1..10) and its
// terms; maps before version 15 kept a single artifact in place of the
// quest, and edition 0 still writes one.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/RawStream.h"
#include "editor/SeersHut.h"

// The reserved bytes after a hut's reward (Loki's kNumSeersHutReserved).
const unsigned int kNumSeersHutReserved = 2;

namespace {

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

class TRewardWriter : public TSeersHut::TReward::TVisitor {
public:
    TRewardWriter(TRawOStream* pOStream, int version) : _m_pOStream(pOStream), _m_version(version) {}

    void write(const TSeersHut::TReward* pReward);

    virtual void visit(const TSeersHut::TRewardExperience& reward);
    virtual void visit(const TSeersHut::TRewardMana& reward);
    virtual void visit(const TSeersHut::TRewardMorale& reward);
    virtual void visit(const TSeersHut::TRewardLuck& reward);
    virtual void visit(const TSeersHut::TRewardResource& reward);
    virtual void visit(const TSeersHut::TRewardPrimarySkill& reward);
    virtual void visit(const TSeersHut::TRewardSecondarySkill& reward);
    virtual void visit(const TSeersHut::TRewardArtifact& reward);
    virtual void visit(const TSeersHut::TRewardSpell& reward);
    virtual void visit(const TSeersHut::TRewardCreature& reward);

private:
    void writeType(TRewardType type) { *_m_pOStream << static_cast<signed char>(type); }

    TRawOStream* _m_pOStream;
    int _m_version;
};

class TRewardReader {
public:
    TRewardReader(TRawIStream* pIStream, int version) : _m_pIStream(pIStream), _m_version(version) {}

    std::auto_ptr<TSeersHut::TReward> read();

private:
    TSeersHut::TReward* readExperience();
    TSeersHut::TReward* readMana();
    TSeersHut::TReward* readMorale();
    TSeersHut::TReward* readLuck();
    TSeersHut::TReward* readResource();
    TSeersHut::TReward* readPrimarySkill();
    TSeersHut::TReward* readSecondarySkill();
    TSeersHut::TReward* readArtifact();
    TSeersHut::TReward* readSpell();
    TSeersHut::TReward* readCreature();

    static TSeersHut::TReward* (TRewardReader::* const _s_apfnRead[kNumRewardTypes])();

    TRawIStream* _m_pIStream;
    int _m_version;
};

DATA(0x00541f6c)
TSeersHut::TReward* (TRewardReader::* const TRewardReader::_s_apfnRead[kNumRewardTypes])() = {
    NULL,
    &TRewardReader::readExperience,
    &TRewardReader::readMana,
    &TRewardReader::readMorale,
    &TRewardReader::readLuck,
    &TRewardReader::readResource,
    &TRewardReader::readPrimarySkill,
    &TRewardReader::readSecondarySkill,
    &TRewardReader::readArtifact,
    &TRewardReader::readSpell,
    &TRewardReader::readCreature
};

template<class T>
class TRewardRHSEquivalencyTester : public TSeersHut::TReward::TVisitor {
public:
    TRewardRHSEquivalencyTester(const T& lhs) : _m_lhs(lhs), _m_bEquivalent(false) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

    virtual void visit(const T& rhs) { _m_bEquivalent = _m_lhs == rhs; }

private:
    const T& _m_lhs;
    bool _m_bEquivalent;
};

class TRewardEquivalencyTester : public TSeersHut::TReward::TVisitor {
public:
    TRewardEquivalencyTester(const TSeersHut::TReward& rhs) : _m_rhs(rhs), _m_bEquivalent(false) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

private:
    template<class T>
    void _test(const T& lhs)
    {
        TRewardRHSEquivalencyTester<T> tester(lhs);
        _m_rhs.accept(&tester);
        _m_bEquivalent = tester.getBEquivalent();
    }

public:
    virtual void visit(const TSeersHut::TRewardExperience& lhs) { _test(lhs); }
    virtual void visit(const TSeersHut::TRewardMana& lhs) { _test(lhs); }
    virtual void visit(const TSeersHut::TRewardMorale& lhs) { _test(lhs); }
    virtual void visit(const TSeersHut::TRewardLuck& lhs) { _test(lhs); }
    virtual void visit(const TSeersHut::TRewardResource& lhs) { _test(lhs); }
    virtual void visit(const TSeersHut::TRewardPrimarySkill& lhs) { _test(lhs); }
    virtual void visit(const TSeersHut::TRewardSecondarySkill& lhs) { _test(lhs); }
    virtual void visit(const TSeersHut::TRewardArtifact& lhs) { _test(lhs); }
    virtual void visit(const TSeersHut::TRewardSpell& lhs) { _test(lhs); }
    virtual void visit(const TSeersHut::TRewardCreature& lhs) { _test(lhs); }

private:
    const TSeersHut::TReward& _m_rhs;
    bool _m_bEquivalent;
};

class TRewardCloner : public TSeersHut::TReward::TVisitor {
public:
    std::auto_ptr<TSeersHut::TReward> clone(const TSeersHut::TReward& reward)
    {
        reward.accept(this);
        return _m_pClone;
    }

private:
    template<class T>
    void _clone(const T& reward)
    {
        _m_pClone = std::auto_ptr<TSeersHut::TReward>(new T(reward));
    }

public:
    virtual void visit(const TSeersHut::TRewardExperience& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TRewardMana& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TRewardMorale& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TRewardLuck& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TRewardResource& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TRewardPrimarySkill& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TRewardSecondarySkill& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TRewardArtifact& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TRewardSpell& reward) { _clone(reward); }
    virtual void visit(const TSeersHut::TRewardCreature& reward) { _clone(reward); }

private:
    std::auto_ptr<TSeersHut::TReward> _m_pClone;
};

}  // namespace

VA(0x004b61d6, 0x27)
void TRewardWriter::write(const TSeersHut::TReward* pReward)
{
    if (pReward != NULL)
        pReward->accept(this);
    else
        writeType(eRewardNone);
}

VA(0x004b61fd, 0x31)
void TRewardWriter::visit(const TSeersHut::TRewardExperience& reward)
{
    writeType(eRewardExperience);
    *_m_pOStream << static_cast<long>(reward.getBonus());
}

VA(0x004b622e, 0x31)
void TRewardWriter::visit(const TSeersHut::TRewardMana& reward)
{
    writeType(eRewardMana);
    *_m_pOStream << static_cast<long>(reward.getBonus());
}

VA(0x004b625f, 0x31)
void TRewardWriter::visit(const TSeersHut::TRewardMorale& reward)
{
    writeType(eRewardMorale);
    *_m_pOStream << static_cast<signed char>(reward.getBonus());
}

VA(0x004b6290, 0x31)
void TRewardWriter::visit(const TSeersHut::TRewardLuck& reward)
{
    writeType(eRewardLuck);
    *_m_pOStream << static_cast<signed char>(reward.getBonus());
}

VA(0x004b62c1, 0x43)
void TRewardWriter::visit(const TSeersHut::TRewardResource& reward)
{
    writeType(eRewardResource);
    *_m_pOStream << static_cast<signed char>(reward.getType()) << static_cast<long>(reward.getQuantity());
}

VA(0x004b6304, 0x42)
void TRewardWriter::visit(const TSeersHut::TRewardPrimarySkill& reward)
{
    writeType(eRewardPrimarySkill);
    *_m_pOStream << static_cast<signed char>(reward.getSkill()) << static_cast<signed char>(reward.getBonus());
}

VA(0x004b6346, 0x42)
void TRewardWriter::visit(const TSeersHut::TRewardSecondarySkill& reward)
{
    writeType(eRewardSecondarySkill);
    *_m_pOStream << static_cast<signed char>(reward.getSkill()) << static_cast<signed char>(reward.getMastery());
}

VA(0x004b6388, 0x4b)
void TRewardWriter::visit(const TSeersHut::TRewardArtifact& reward)
{
    writeType(eRewardArtifact);
    if (_m_version >= 1)
        *_m_pOStream << static_cast<short>(reward.getArtifact());
    else
        *_m_pOStream << static_cast<signed char>(reward.getArtifact());
}

VA(0x004b63d3, 0x31)
void TRewardWriter::visit(const TSeersHut::TRewardSpell& reward)
{
    writeType(eRewardSpell);
    *_m_pOStream << static_cast<signed char>(reward.getSpell());
}

VA(0x004b6404, 0x2d)
void TRewardWriter::visit(const TSeersHut::TRewardCreature& reward)
{
    writeType(eRewardCreature);
    reward.getCreatureStack().write(_m_pOStream, _m_version);
}

VA(0x004b6431, 0x8b)
std::auto_ptr<TSeersHut::TReward> TRewardReader::read()
{
    signed char type;
    *_m_pIStream >> type;
    if (type == eRewardNone)
        return std::auto_ptr<TSeersHut::TReward>();
    std::auto_ptr<TSeersHut::TReward> pReward((this->*_s_apfnRead[type])());
    if (pReward.get() == NULL)
        throw TAllocationFailure();
    return pReward;
}

VA(0x004b64bc, 0x2d)
TSeersHut::TReward* TRewardReader::readExperience()
{
    long bonus;
    *_m_pIStream >> bonus;
    return new TSeersHut::TRewardExperience(bonus);
}

VA(0x004b64e9, 0x2d)
TSeersHut::TReward* TRewardReader::readMana()
{
    long bonus;
    *_m_pIStream >> bonus;
    return new TSeersHut::TRewardMana(bonus);
}

VA(0x004b6516, 0x2e)
TSeersHut::TReward* TRewardReader::readMorale()
{
    signed char bonus;
    *_m_pIStream >> bonus;
    return new TSeersHut::TRewardMorale(bonus);
}

VA(0x004b6544, 0x2e)
TSeersHut::TReward* TRewardReader::readLuck()
{
    signed char bonus;
    *_m_pIStream >> bonus;
    return new TSeersHut::TRewardLuck(bonus);
}

VA(0x004b6572, 0x40)
TSeersHut::TReward* TRewardReader::readResource()
{
    signed char type;
    long quantity;
    *_m_pIStream >> type >> quantity;
    return new TSeersHut::TRewardResource(TGameResourceType(type), quantity);
}

VA(0x004b65b2, 0x40)
TSeersHut::TReward* TRewardReader::readPrimarySkill()
{
    signed char skill;
    signed char bonus;
    *_m_pIStream >> skill >> bonus;
    return new TSeersHut::TRewardPrimarySkill(TPrimarySkill(skill), bonus);
}

VA(0x004b65f2, 0x40)
TSeersHut::TReward* TRewardReader::readSecondarySkill()
{
    signed char skill;
    signed char mastery;
    *_m_pIStream >> skill >> mastery;
    return new TSeersHut::TRewardSecondarySkill(TSecondarySkill(skill), TSkillMastery(mastery));
}

VA(0x004b6632, 0x47)
TSeersHut::TReward* TRewardReader::readArtifact()
{
    int artifact;
    if (_m_version >= 21) {
        short value;
        *_m_pIStream >> value;
        artifact = value;
    } else {
        signed char value;
        *_m_pIStream >> value;
        artifact = value;
    }
    return new TSeersHut::TRewardArtifact(TArtifact(artifact));
}

VA(0x004b6679, 0x2e)
TSeersHut::TReward* TRewardReader::readSpell()
{
    signed char spell;
    *_m_pIStream >> spell;
    return new TSeersHut::TRewardSpell(SpellID(spell));
}

VA(0x004b66a7, 0x5c)
TSeersHut::TReward* TRewardReader::readCreature()
{
    return new TSeersHut::TRewardCreature(TCreatureStack(_m_pIStream, _m_version));
}

VA(0x004b6703, 0x27)
bool TSeersHut::TReward::equivalent(const TReward& lhs, const TReward& rhs)
{
    TRewardEquivalencyTester tester(rhs);
    lhs.accept(&tester);
    return tester.getBEquivalent();
}

VA(0x004b67a2, 0x4f)
std::auto_ptr<TSeersHut::TReward> TSeersHut::TReward::clone() const
{
    TRewardCloner cloner;
    return cloner.clone(*this);
}

VA(0x004b68a3, 0x19)
TSeersHut::TRewardResource::TRewardResource(TGameResourceType type, unsigned int quantity)
    : _m_type(type), _m_quantity(quantity)
{
}

VA(0x004b68bc, 0x19)
TSeersHut::TRewardPrimarySkill::TRewardPrimarySkill(TPrimarySkill skill, int bonus)
    : TSimpleBonusReward<99>(bonus), _m_skill(skill)
{
}

VA(0x004b68d5, 0x19)
TSeersHut::TRewardSecondarySkill::TRewardSecondarySkill(TSecondarySkill skill, TSkillMastery mastery)
    : _m_skill(skill), _m_mastery(mastery)
{
}

VA(0x004b68ee, 0x12)
TSeersHut::TRewardArtifact::TRewardArtifact(TArtifact artifact) : _m_artifact(artifact)
{
}

VA(0x004b6900, 0x12)
TSeersHut::TRewardSpell::TRewardSpell(SpellID spell) : _m_spell(spell)
{
}

VA(0x004b6912, 0xea)
TSeersHut::TSeersHut(const TSeersHut& other) : TGameObject(other), TQuestLocation(other)
{
    if (other.getPReward() != NULL) {
        _m_pReward = other.getPReward()->clone();
        if (_m_pReward.get() == NULL)
            throw TAllocationFailure();
    }
}

VA(0x004b69fc, 0x7c)
TSeersHut::TSeersHut(const TObjectType& objType) : TGameObject(objType), TQuestLocation(objType)
{
}

VA(0x004b6a78, 0x1b0)
TSeersHut::TSeersHut(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TQuestLocation(objType)
{
    if (version < 15) {
        signed char artifact;
        *pIStream >> artifact;
        if (artifact != ARTIFACT_NONE) {
            std::multiset<TArtifact> artifacts;
            artifacts.insert(TArtifact(artifact));
            std::auto_ptr<TQuest> pQuest(new TQuestBringArtifacts(artifacts));
            if (pQuest.get() == NULL)
                throw TAllocationFailure();
            setQuest(pQuest);
        }
    } else {
        read(pIStream, version);
    }
    TRewardReader reader(pIStream, version);
    _m_pReward = reader.read();
    signed char aReserved[kNumSeersHutReserved];
    *pIStream >> aReserved;
}

VA(0x004b6c28, 0x3e)
TSeersHut::~TSeersHut()
{
}

VA(0x004b6c66, 0x21)
void TSeersHut::setReward(std::auto_ptr<TReward> pReward)
{
    _m_pReward = pReward;
}

VA(0x004b6c87, 0x2a)
void TSeersHut::clearReward()
{
    _m_pReward = std::auto_ptr<TReward>();
}

VA(0x004b6cb1, 0x1a)
bool TSeersHut::isCustomized() const
{
    return TQuestLocation::isCustomized() || getPReward() != NULL;
}

VA(0x004b6ccb, 0x7b)
void TSeersHut::write(TRawOStream* pOStream, int version) const
{
    if (version < 1) {
        const TQuestBringArtifacts* pQuest = static_cast<const TQuestBringArtifacts*>(getPQuest());
        if (pQuest != NULL)
            *pOStream << static_cast<signed char>(*pQuest->getArtifacts().begin());
        else
            *pOStream << static_cast<signed char>(ARTIFACT_NONE);
    } else {
        TQuestLocation::write(pOStream, version);
    }
    TRewardWriter writer(pOStream, version);
    writer.write(getPReward());
    signed char aReserved[kNumSeersHutReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x004b6d46, 0x17)
void TSeersHut::resetQuestTerms()
{
    if (getPReward() != NULL)
        clearReward();
    TQuestLocation::resetQuestTerms();
}
