// BlackBox.h - a Pandora's Box and its contents
// (C:\Dev\Heroes 3 Exp 2\Editor\BlackBox.cpp, by its RTTI; Loki h3maped
// object 7).
//
// RTTI TBlackBox <- TTreasure <- virtual TGameObject: the treasure's vbptr
// comes first. Its contents (+0x50) are a copy-on-write TContents:
// experience, mana, morale and luck bonuses, a resource grant, primary
// skill bonuses, and vectors of secondary skills, artifacts, spells and
// creature stacks (0x7c bytes in the handle's wrapper, after the count).
#ifndef HOMM3_EDITOR_BLACKBOX_H
#define HOMM3_EDITOR_BLACKBOX_H

#include <vector>

#include "armygrp.h"
#include "artifact_type.h"
#include "herospec.h"
#include "primaryskill.h"
#include "secondaryskill.h"
#include "editor/Army.h"
#include "editor/Array.h"
#include "editor/ObjectSpecializations.h"
#include "editor/RefCountingPtr.h"
#include "editor/ResourceQuantities.h"

class TRawIStream;
class TRawOStream;

// A bonus to each primary skill.
class TPrimarySkillBonuses {
public:
    enum { s_kMax = 99 };

    TPrimarySkillBonuses() : _m_bonuses(0) {}

    unsigned int get(TPrimarySkill primarySkill) const;
    void set(TPrimarySkill primarySkill, unsigned int newBonus);

    friend bool operator==(const TPrimarySkillBonuses& lhs, const TPrimarySkillBonuses& rhs)
    {
        return lhs._m_bonuses == rhs._m_bonuses;
    }
    friend bool operator!=(const TPrimarySkillBonuses& lhs, const TPrimarySkillBonuses& rhs)
    {
        return !(lhs == rhs);
    }

private:
    TArray<unsigned int, kNumPrimarySkills> _m_bonuses;
};

TRawOStream& operator<<(TRawOStream& stream, const TPrimarySkillBonuses& bonuses);
TRawIStream& operator>>(TRawIStream& stream, TPrimarySkillBonuses& bonuses);

// A secondary skill at a mastery (basic by default).
class TSecondarySkillRecord {
public:
    TSecondarySkillRecord() : _m_type(eSecSkillPathfinding), _m_mastery(eMasteryBasic) {}
    TSecondarySkillRecord(TSecondarySkill type, TSkillMastery mastery);

    TSecondarySkill getType() const { return _m_type; }
    void setType(TSecondarySkill newType);
    TSkillMastery getMastery() const { return _m_mastery; }
    void setMastery(TSkillMastery newMastery);

    friend bool operator==(const TSecondarySkillRecord& lhs, const TSecondarySkillRecord& rhs)
    {
        return lhs._m_type == rhs._m_type && lhs._m_mastery == rhs._m_mastery;
    }

private:
    TSecondarySkill _m_type;
    TSkillMastery _m_mastery;
};

TRawOStream& operator<<(TRawOStream& stream, const TSecondarySkillRecord& record);
TRawIStream& operator>>(TRawIStream& stream, TSecondarySkillRecord& record);

class TBlackBox : public TTreasure {
public:
    class TContents {
    public:
        enum { s_kMaxExperienceBonus = 99999999 };
        enum { s_kMinManaBonus = -999, s_kMaxManaBonus = 999 };
        enum { s_kMinMoraleBonus = -3, s_kMaxMoraleBonus = 3 };
        enum { s_kMinLuckBonus = -3, s_kMaxLuckBonus = 3 };
        enum { s_kMaxSecSkills = 8, s_kMaxArtifacts = 64, s_kMaxCreatureStacks = 7 };

        TContents();

        void setExperienceBonus(int newExperienceBonus);
        void setManaBonus(int newManaBonus);
        void setMoraleBonus(int newMoraleBonus);
        void setLuckBonus(int newLuckBonus);
        void setResourceQuantities(const TResourceQuantities& newQuantities) { _m_resourceQuantities = newQuantities; }
        void setPrimarySkillBonuses(const TPrimarySkillBonuses& newBonuses) { _m_primarySkillBonuses = newBonuses; }
        void setSecondarySkills(const std::vector<TSecondarySkillRecord>& aNewSecondarySkill);
        void setArtifacts(const std::vector<TArtifact>& aNewArtifact);
        void setSpells(const std::vector<SpellID>& aNewSpell);
        void setCreatureStacks(const std::vector<TCreatureStack>& aNewCreatureStack);

        int getExperienceBonus() const { return _m_experienceBonus; }
        int getManaBonus() const { return _m_manaBonus; }
        int getMoraleBonus() const { return _m_moraleBonus; }
        int getLuckBonus() const { return _m_luckBonus; }
        const TResourceQuantities& getResourceQuantities() const { return _m_resourceQuantities; }
        const TPrimarySkillBonuses& getPrimarySkillBonuses() const { return _m_primarySkillBonuses; }
        const std::vector<TSecondarySkillRecord>& getSecondarySkills() const { return _m_aSecondarySkill; }
        const std::vector<TArtifact>& getArtifacts() const { return _m_aArtifact; }
        const std::vector<SpellID>& getSpells() const { return _m_aSpell; }
        const std::vector<TCreatureStack>& getCreatureStacks() const { return _m_aCreatureStack; }

        bool isCustomized() const;

    private:
        int _m_experienceBonus;
        int _m_manaBonus;
        int _m_moraleBonus;
        int _m_luckBonus;
        TResourceQuantities _m_resourceQuantities;
        TPrimarySkillBonuses _m_primarySkillBonuses;
        std::vector<TSecondarySkillRecord> _m_aSecondarySkill;
        std::vector<TArtifact> _m_aArtifact;
        std::vector<SpellID> _m_aSpell;
        std::vector<TCreatureStack> _m_aCreatureStack;
    };

    TBlackBox(const TObjectType& objType);
    TBlackBox(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;

    TContents* getPContents() { return _m_pContents.get(); }
    const TContents* getPContents() const { return _m_pContents.get(); }
    const TContents& getContents() const { return *getPContents(); }

    virtual bool isCustomized() const { return TTreasure::isCustomized() || _m_pContents->isCustomized(); }
    virtual bool hasText() const { return true; }

private:
    TRefCountingPtr<TContents> _m_pContents;
};

#endif  /* HOMM3_EDITOR_BLACKBOX_H */
