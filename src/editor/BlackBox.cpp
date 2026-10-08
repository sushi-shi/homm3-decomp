// BlackBox.cpp - Loki h3maped object 7: Pandora's box, its copy-on-write
// contents, and the primary skill bonuses and secondary skill records the
// box and the heroes share. Assert lines come from the retail immediates.
#include <assert.h>
#include <algorithm>
#include <vector>

#include "adventureobjecttype.h"
#include "exceptions.h"
#include "editor/BlackBox.h"
#include "editor/RawStream.h"

void TPrimarySkillBonuses::set(TPrimarySkill primarySkill, unsigned int newBonus)
{
#line 36
    assert(primarySkill >= 0 && primarySkill < kNumPrimarySkills);
    assert(newBonus <= s_kMax);
    _m_bonuses[primarySkill] = newBonus;
}

unsigned int TPrimarySkillBonuses::get(TPrimarySkill primarySkill) const
{
#line 45
    assert(primarySkill >= 0 && primarySkill < kNumPrimarySkills);
    return _m_bonuses[primarySkill];
}

TRawOStream& operator<<(TRawOStream& stream, const TPrimarySkillBonuses& bonuses)
{
    for (unsigned int primarySkill = 0; primarySkill < kNumPrimarySkills; ++primarySkill)
        stream << static_cast< signed char >( bonuses.get(TPrimarySkill(primarySkill)) );
    return stream;
}

TRawIStream& operator>>(TRawIStream& stream, TPrimarySkillBonuses& bonuses)
{
    for (unsigned int primarySkill = 0; primarySkill < kNumPrimarySkills; ++primarySkill) {
        signed char bonus;
        stream >> bonus;
        bonuses.set(TPrimarySkill(primarySkill), bonus);
    }
    return stream;
}

TSecondarySkillRecord::TSecondarySkillRecord(TSecondarySkill type, TSkillMastery mastery)
    : _m_type(type), _m_mastery(mastery)
{
#line 82
    assert(type >= 0 && type < kNumSecSkills);
    assert(mastery >= eMasteryBasic && mastery < eMasteryBasic + kNumMasteries);
}

void TSecondarySkillRecord::setType(TSecondarySkill newType)
{
#line 89
    assert(newType >= 0 && newType < kNumSecSkills);
    _m_type = newType;
}

void TSecondarySkillRecord::setMastery(TSkillMastery newMastery)
{
#line 97
    assert(newMastery >= eMasteryBasic && newMastery < eMasteryBasic + kNumMasteries);
    _m_mastery = newMastery;
}

TRawOStream& operator<<(TRawOStream& stream, const TSecondarySkillRecord& record)
{
    stream << static_cast< signed char >( record.getType() ) << static_cast< signed char >( record.getMastery() );
    return stream;
}

TRawIStream& operator>>(TRawIStream& stream, TSecondarySkillRecord& record)
{
    signed char type;
    signed char mastery;
    stream >> type >> mastery;
    record = TSecondarySkillRecord(TSecondarySkill(type), TSkillMastery(mastery));
    return stream;
}

TBlackBox::TContents::TContents() : _m_experienceBonus(0), _m_manaBonus(0), _m_moraleBonus(0), _m_luckBonus(0)
{
}

void TBlackBox::TContents::setExperienceBonus(int newExperienceBonus)
{
#line 136
    assert(newExperienceBonus >= 0 && newExperienceBonus <= s_kMaxExperienceBonus);
    _m_experienceBonus = newExperienceBonus;
}

void TBlackBox::TContents::setManaBonus(int newManaBonus)
{
#line 144
    assert(newManaBonus >= s_kMinManaBonus && newManaBonus <= s_kMaxManaBonus);
    _m_manaBonus = newManaBonus;
}

void TBlackBox::TContents::setMoraleBonus(int newMoraleBonus)
{
#line 152
    assert(newMoraleBonus >= s_kMinMoraleBonus && newMoraleBonus <= s_kMaxMoraleBonus);
    _m_moraleBonus = newMoraleBonus;
}

void TBlackBox::TContents::setLuckBonus(int newLuckBonus)
{
#line 160
    assert(newLuckBonus >= s_kMinLuckBonus && newLuckBonus <= s_kMaxLuckBonus);
    _m_luckBonus = newLuckBonus;
}

void TBlackBox::TContents::setSecondarySkills(const vector<TSecondarySkillRecord>& aNewSecondarySkill)
{
#line 168
    assert(aNewSecondarySkill.size() <= s_kMaxSecSkills);
    _m_aSecondarySkill = aNewSecondarySkill;
}

void TBlackBox::TContents::setArtifacts(const vector<TArtifact>& aNewArtifact)
{
#line 176
    assert(aNewArtifact.size() < s_kMaxArtifacts);
    for (const TArtifact* pArtifact = aNewArtifact.begin(); pArtifact != aNewArtifact.end(); ++pArtifact) {
#line 181
        assert(*pArtifact >= 0 && *pArtifact < kNumArtifacts);
        assert(( akArtifactTraits[ *pArtifact ].m_class & ArtifactClassSpecial ) == 0);
    }
    _m_aArtifact = aNewArtifact;
}

void TBlackBox::TContents::setSpells(const vector<SpellID>& aNewSpell)
{
    for (const SpellID* pSpell = aNewSpell.begin(); pSpell != aNewSpell.end(); ++pSpell)
#line 195
        assert(*pSpell >= 0 && *pSpell < kNumSpells);
    if (aNewSpell.size() != 0)
        for (pSpell = aNewSpell.begin(); pSpell != aNewSpell.end() - 1; ++pSpell)
#line 201
            assert(std::find( pSpell + 1, aNewSpell.end(), *pSpell ) == aNewSpell.end());
    _m_aSpell = aNewSpell;
}

void TBlackBox::TContents::setCreatureStacks(const vector<TCreatureStack>& aNewCreatureStack)
{
#line 211
    assert(aNewCreatureStack.size() <= s_kMaxCreatureStacks);
    for (const TCreatureStack* pStack = aNewCreatureStack.begin(); pStack != aNewCreatureStack.end(); ++pStack)
#line 215
        assert(pStack->getCreatureType() != eCreatureNone && pStack->getQuantity() != 0);
    _m_aCreatureStack = aNewCreatureStack;
}

bool TBlackBox::TContents::isCustomized() const
{
    return _m_experienceBonus != 0 || _m_manaBonus != 0 || _m_moraleBonus != 0 || _m_luckBonus != 0
           || !_m_aSecondarySkill.empty() || !_m_aArtifact.empty() || !_m_aSpell.empty()
           || !_m_aCreatureStack.empty() || _m_resourceQuantities != TResourceQuantities()
           || _m_primarySkillBonuses != TPrimarySkillBonuses();
}

TBlackBox::TBlackBox(const TObjectType& objType) : TGameObject(objType), TTreasure(objType)
{
#line 244
    assert(objType.getType() == EVENT || objType.getType() == BLACK_BOX);
}

TBlackBox::TBlackBox(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TTreasure(objType)
{
#line 252
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
        _m_pContents->setExperienceBonus(experienceBonus);
        _m_pContents->setManaBonus(manaBonus);
        _m_pContents->setMoraleBonus(moraleBonus);
        _m_pContents->setLuckBonus(luckBonus);
        _m_pContents->setResourceQuantities(resourceQuantities);
        _m_pContents->setPrimarySkillBonuses(primarySkillBonuses);

        vector<TSecondarySkillRecord> aSecondarySkill;
        signed char numSecondarySkills;
        *pIStream >> numSecondarySkills;
        aSecondarySkill.reserve(numSecondarySkills);
        while (numSecondarySkills > 0) {
            --numSecondarySkills;
            TSecondarySkillRecord record;
            *pIStream >> record;
            aSecondarySkill.push_back(record);
        }
        _m_pContents->setSecondarySkills(aSecondarySkill);

        vector<TArtifact> aArtifact;
        signed char numArtifacts;
        *pIStream >> numArtifacts;
        aArtifact.reserve(numArtifacts);
        while (numArtifacts > 0) {
            --numArtifacts;
            signed char artifact;
            *pIStream >> artifact;
            aArtifact.push_back(TArtifact(artifact));
        }
        _m_pContents->setArtifacts(aArtifact);

        vector<SpellID> aSpell;
        signed char numSpells;
        *pIStream >> numSpells;
        aSpell.reserve(numSpells);
        while (numSpells > 0) {
            --numSpells;
            signed char spell;
            *pIStream >> spell;
            aSpell.push_back(SpellID(spell));
        }
        _m_pContents->setSpells(aSpell);

        vector<TCreatureStack> aCreatureStack;
        signed char numCreatureStacks;
        *pIStream >> numCreatureStacks;
        aCreatureStack.reserve(numCreatureStacks);
        while (numCreatureStacks > 0) {
            --numCreatureStacks;
            TCreatureStack stack;
            *pIStream >> stack;
            aCreatureStack.push_back(stack);
        }
        _m_pContents->setCreatureStacks(aCreatureStack);

        signed char aReserved[8];
        *pIStream >> aReserved;
    } else {
        if (objType.getType() == BLACK_BOX) {
            signed char artifact;
            *pIStream >> artifact;
            signed char aReserved[3];
            *pIStream >> aReserved;
            if (artifact >= 0)
                _m_pContents->setArtifacts(vector<TArtifact>(1, TArtifact(artifact)));
        }
    }
}

void TBlackBox::write(TRawOStream* pOStream) const
{
#line 366
    assert(pOStream != NULL);
    TTreasure::write(pOStream);
    *pOStream << static_cast< long >( _m_pContents->getExperienceBonus() )
              << static_cast< long >( _m_pContents->getManaBonus() )
              << static_cast< signed char >( _m_pContents->getMoraleBonus() )
              << static_cast< signed char >( _m_pContents->getLuckBonus() )
              << _m_pContents->getResourceQuantities() << _m_pContents->getPrimarySkillBonuses();

    *pOStream << static_cast< signed char >( _m_pContents->getSecondarySkills().size() );
    for (const TSecondarySkillRecord* pRecord = _m_pContents->getSecondarySkills().begin();
         pRecord != _m_pContents->getSecondarySkills().end(); ++pRecord)
        *pOStream << *pRecord;

    *pOStream << static_cast< signed char >( _m_pContents->getArtifacts().size() );
    for (const TArtifact* pArtifact = _m_pContents->getArtifacts().begin();
         pArtifact != _m_pContents->getArtifacts().end(); ++pArtifact)
        *pOStream << (signed char) *pArtifact;

    *pOStream << static_cast< signed char >( _m_pContents->getSpells().size() );
    for (const SpellID* pSpell = _m_pContents->getSpells().begin(); pSpell != _m_pContents->getSpells().end();
         ++pSpell)
        *pOStream << (signed char) *pSpell;

    *pOStream << static_cast< signed char >( _m_pContents->getCreatureStacks().size() );
    for (const TCreatureStack* pStack = _m_pContents->getCreatureStacks().begin();
         pStack != _m_pContents->getCreatureStacks().end(); ++pStack)
        *pOStream << *pStack;

    signed char aReserved[8];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}
