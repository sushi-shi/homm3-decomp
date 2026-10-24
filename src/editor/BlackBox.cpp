// BlackBox.cpp - Pandora's box (h3maped 0x4065a0..0x407a25; Loki h3maped
// object 7): its copy-on-write contents, and the primary skill bonuses and
// secondary skill records the box and the heroes share. The Windows box
// reads its treasure part and contents from map version 6 (before it a
// Pandora's box held one artifact), artifacts as shorts from map version
// 21, and writes them as shorts from Armageddon's Blade on. The release
// drops Loki's asserts.
#include "editor/stdafx.h"

#include <assert.h>
#include <algorithm>

#include "va.h"
#include "adventureobjecttype.h"
#include "editor/BlackBox.h"
#include "editor/RawStream.h"

// The reserved byte counts of the map format records this file reads and
// writes.
const unsigned int kNumOldBlackBoxReserved = 3;
const unsigned int kNumBlackBoxReserved = 8;

void TPrimarySkillBonuses::set(TPrimarySkill primarySkill, unsigned int newBonus)
{
    assert(primarySkill >= 0 && primarySkill < kNumPrimarySkills);
    assert(newBonus <= s_kMax);
    _m_bonuses[primarySkill] = newBonus;
}

unsigned int TPrimarySkillBonuses::get(TPrimarySkill primarySkill) const
{
    assert(primarySkill >= 0 && primarySkill < kNumPrimarySkills);
    return _m_bonuses[primarySkill];
}

TRawOStream& operator<<(TRawOStream& stream, const TPrimarySkillBonuses& bonuses)
{
    for (unsigned int primarySkill = 0; primarySkill < kNumPrimarySkills; ++primarySkill)
        stream << static_cast<signed char>(bonuses.get(TPrimarySkill(primarySkill)));
    return stream;
}

VA(0x00406774, 0x2a)
TRawIStream& operator>>(TRawIStream& stream, TPrimarySkillBonuses& bonuses)
{
    for (unsigned int primarySkill = 0; primarySkill < kNumPrimarySkills; ++primarySkill) {
        signed char bonus;
        stream >> bonus;
        bonuses.set(TPrimarySkill(primarySkill), bonus);
    }
    return stream;
}

VA(0x0040679e, 0x12)
TSecondarySkillRecord::TSecondarySkillRecord(TSecondarySkill type, TSkillMastery mastery)
    : _m_type(type), _m_mastery(mastery)
{
    assert(type >= 0 && type < kNumSecSkills);
    assert(mastery >= eMasteryBasic && mastery < eMasteryBasic + kNumMasteries);
}

void TSecondarySkillRecord::setType(TSecondarySkill newType)
{
    assert(newType >= 0 && newType < kNumSecSkills);
    _m_type = newType;
}

void TSecondarySkillRecord::setMastery(TSkillMastery newMastery)
{
    assert(newMastery >= eMasteryBasic && newMastery < eMasteryBasic + kNumMasteries);
    _m_mastery = newMastery;
}

VA(0x004067b0, 0x2e)
TRawOStream& operator<<(TRawOStream& stream, const TSecondarySkillRecord& record)
{
    stream << static_cast<signed char>(record.getType()) << static_cast<signed char>(record.getMastery());
    return stream;
}

VA(0x004067de, 0x30)
TRawIStream& operator>>(TRawIStream& stream, TSecondarySkillRecord& record)
{
    signed char type;
    signed char mastery;
    stream >> type >> mastery;
    record = TSecondarySkillRecord(TSecondarySkill(type), TSkillMastery(mastery));
    return stream;
}

VA(0x0040680e, 0x83)
TBlackBox::TContents::TContents() : _m_experienceBonus(0), _m_manaBonus(0), _m_moraleBonus(0), _m_luckBonus(0)
{
}

VA(0x00406891, 0x9)
void TBlackBox::TContents::setExperienceBonus(int newExperienceBonus)
{
    assert(newExperienceBonus >= 0 && newExperienceBonus <= s_kMaxExperienceBonus);
    _m_experienceBonus = newExperienceBonus;
}

void TBlackBox::TContents::setManaBonus(int newManaBonus)
{
    assert(newManaBonus >= s_kMinManaBonus && newManaBonus <= s_kMaxManaBonus);
    _m_manaBonus = newManaBonus;
}

void TBlackBox::TContents::setMoraleBonus(int newMoraleBonus)
{
    assert(newMoraleBonus >= s_kMinMoraleBonus && newMoraleBonus <= s_kMaxMoraleBonus);
    _m_moraleBonus = newMoraleBonus;
}

VA(0x0040689a, 0xa)
void TBlackBox::TContents::setLuckBonus(int newLuckBonus)
{
    assert(newLuckBonus >= s_kMinLuckBonus && newLuckBonus <= s_kMaxLuckBonus);
    _m_luckBonus = newLuckBonus;
}

VA(0x004068a4, 0xf)
void TBlackBox::TContents::setSecondarySkills(const std::vector<TSecondarySkillRecord>& aNewSecondarySkill)
{
    assert(aNewSecondarySkill.size() <= s_kMaxSecSkills);
    _m_aSecondarySkill = aNewSecondarySkill;
}

VA(0x004068b3, 0xf)
void TBlackBox::TContents::setArtifacts(const std::vector<TArtifact>& aNewArtifact)
{
    assert(aNewArtifact.size() < s_kMaxArtifacts);
    _m_aArtifact = aNewArtifact;
}

VA(0x004068c2, 0xf)
void TBlackBox::TContents::setSpells(const std::vector<SpellID>& aNewSpell)
{
    _m_aSpell = aNewSpell;
}

VA(0x004068d1, 0xf)
void TBlackBox::TContents::setCreatureStacks(const std::vector<TCreatureStack>& aNewCreatureStack)
{
    assert(aNewCreatureStack.size() <= s_kMaxCreatureStacks);
    _m_aCreatureStack = aNewCreatureStack;
}

VA(0x004068e0, 0x113)
bool TBlackBox::TContents::isCustomized() const
{
    return _m_experienceBonus != 0 || _m_manaBonus != 0 || _m_moraleBonus != 0 || _m_luckBonus != 0
           || !_m_aSecondarySkill.empty() || !_m_aArtifact.empty() || !_m_aSpell.empty()
           || !_m_aCreatureStack.empty() || _m_resourceQuantities != TResourceQuantities()
           || _m_primarySkillBonuses != TPrimarySkillBonuses();
}

VA(0x004069f3, 0x79)
TBlackBox::TBlackBox(const TObjectType& objType) : TGameObject(objType), TTreasure(objType)
{
    assert(objType.getType() == EVENT || objType.getType() == BLACK_BOX);
}

// Complete constructs creature stacks from the stream without default stores.
// Use the canonical contents accessor, as in Loki Event loading: together
// these recover the retained reader and final copy-on-write expansion.
// Normalize each version-specific wire value into a named TArtifact before
// vector insertion; a cast temporary at push_back changes the stack homes.
VA(0x00406a80, 0x410)
TBlackBox::TBlackBox(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TTreasure(objType)
{
    assert(pIStream != NULL);
    assert(objType.getType() == EVENT || objType.getType() == BLACK_BOX);
    if (version >= 6) {
        read(pIStream, version);
        long experienceBonus;
        long manaBonus;
        signed char moraleBonus;
        signed char luckBonus;
        TResourceQuantities resourceQuantities;
        TPrimarySkillBonuses primarySkillBonuses;
        *pIStream >> experienceBonus >> manaBonus >> moraleBonus >> luckBonus >> resourceQuantities
                  >> primarySkillBonuses;
        getPContents()->setExperienceBonus(experienceBonus);
        getPContents()->setManaBonus(manaBonus);
        getPContents()->setMoraleBonus(moraleBonus);
        getPContents()->setLuckBonus(luckBonus);
        getPContents()->setResourceQuantities(resourceQuantities);
        getPContents()->setPrimarySkillBonuses(primarySkillBonuses);

        std::vector<TSecondarySkillRecord> aSecondarySkill;
        signed char numSecondarySkills;
        *pIStream >> numSecondarySkills;
        aSecondarySkill.reserve(numSecondarySkills);
        while (numSecondarySkills > 0) {
            --numSecondarySkills;
            TSecondarySkillRecord record;
            *pIStream >> record;
            aSecondarySkill.push_back(record);
        }
        getPContents()->setSecondarySkills(aSecondarySkill);

        std::vector<TArtifact> aArtifact;
        signed char numArtifacts;
        *pIStream >> numArtifacts;
        aArtifact.reserve(numArtifacts);
        while (numArtifacts > 0) {
            --numArtifacts;
            TArtifact artifact;
            if (version >= 21) {
                short value;
                *pIStream >> value;
                artifact = TArtifact(value);
            } else {
                signed char value;
                *pIStream >> value;
                artifact = TArtifact(value);
            }
            aArtifact.push_back(artifact);
        }
        getPContents()->setArtifacts(aArtifact);

        std::vector<SpellID> aSpell;
        signed char numSpells;
        *pIStream >> numSpells;
        aSpell.reserve(numSpells);
        while (numSpells > 0) {
            --numSpells;
            signed char spell;
            *pIStream >> spell;
            aSpell.push_back(SpellID(spell));
        }
        getPContents()->setSpells(aSpell);

        std::vector<TCreatureStack> aCreatureStack;
        signed char numCreatureStacks;
        *pIStream >> numCreatureStacks;
        aCreatureStack.reserve(numCreatureStacks);
        while (numCreatureStacks > 0) {
            --numCreatureStacks;
            TCreatureStack stack(pIStream, version);
            aCreatureStack.push_back(stack);
        }
        getPContents()->setCreatureStacks(aCreatureStack);

        signed char aReserved[kNumBlackBoxReserved];
        *pIStream >> aReserved;
    } else {
        if (objType.getType() == BLACK_BOX) {
            signed char artifact;
            *pIStream >> artifact;
            signed char aReserved[kNumOldBlackBoxReserved];
            *pIStream >> aReserved;
            if (artifact >= 0)
                getPContents()->setArtifacts(std::vector<TArtifact>(1, TArtifact(artifact)));
        }
    }
}

VA(0x00406e90, 0x1e5)
void TBlackBox::write(TRawOStream* pOStream, int version) const
{
    assert(pOStream != NULL);
    TTreasure::write(pOStream, version);
    *pOStream << static_cast<long>(_m_pContents->getExperienceBonus())
              << static_cast<long>(_m_pContents->getManaBonus())
              << static_cast<signed char>(_m_pContents->getMoraleBonus())
              << static_cast<signed char>(_m_pContents->getLuckBonus())
              << _m_pContents->getResourceQuantities() << _m_pContents->getPrimarySkillBonuses();

    *pOStream << static_cast<signed char>(_m_pContents->getSecondarySkills().size());
    for (const TSecondarySkillRecord* pRecord = _m_pContents->getSecondarySkills().begin();
         pRecord != _m_pContents->getSecondarySkills().end(); ++pRecord)
        *pOStream << *pRecord;

    *pOStream << static_cast<signed char>(_m_pContents->getArtifacts().size());
    for (const TArtifact* pArtifact = _m_pContents->getArtifacts().begin();
         pArtifact != _m_pContents->getArtifacts().end(); ++pArtifact)
        if (version >= 1)
            *pOStream << static_cast<short>(*pArtifact);
        else
            *pOStream << static_cast<signed char>(*pArtifact);

    *pOStream << static_cast<signed char>(_m_pContents->getSpells().size());
    for (const SpellID* pSpell = _m_pContents->getSpells().begin(); pSpell != _m_pContents->getSpells().end();
         ++pSpell)
        *pOStream << static_cast<signed char>(*pSpell);

    *pOStream << static_cast<signed char>(_m_pContents->getCreatureStacks().size());
    for (const TCreatureStack* pStack = _m_pContents->getCreatureStacks().begin();
         pStack != _m_pContents->getCreatureStacks().end(); ++pStack)
        pStack->write(pOStream, version);

    signed char aReserved[kNumBlackBoxReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}
