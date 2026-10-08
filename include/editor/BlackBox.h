// BlackBox.h - Pandora's box and its contents (Loki h3maped BlackBox.cpp,
// object 7). TBlackBox is a TTreasure whose contents (+0x4c) are a
// copy-on-write TContents: experience, mana, morale and luck bonuses, a
// resource grant, primary skill bonuses, and vectors of secondary skills,
// artifacts, spells and creature stacks. The bonus limits (s_kMax*/s_kMin*)
// are the asserts' names with the bodies' values.
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
    static const unsigned int s_kMax = 99;

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

// A secondary skill at a mastery (basic by default). The editor counts
// three masteries from basic ("mastery < eMasteryBasic + kNumMasteries"
// compares with 4) where the game's TSkillMastery counts four from none;
// the record carries the editor's count so herospec.h keeps the game's.
class TSecondarySkillRecord {
public:
    enum {
        kNumMasteries = 3
    };

    TSecondarySkillRecord() : _m_type(TSecondarySkill(0)), _m_mastery(TSkillMastery(1)) {}
    TSecondarySkillRecord(TSecondarySkill type, TSkillMastery mastery);

    TSecondarySkill getType() const { return _m_type; }
    void setType(TSecondarySkill newType);
    TSkillMastery getMastery() const { return _m_mastery; }
    void setMastery(TSkillMastery newMastery);

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
        static const int s_kMaxExperienceBonus = 99999999;
        static const int s_kMinManaBonus = -999;
        static const int s_kMaxManaBonus = 999;
        static const int s_kMinMoraleBonus = -3;
        static const int s_kMaxMoraleBonus = 3;
        static const int s_kMinLuckBonus = -3;
        static const int s_kMaxLuckBonus = 3;
        static const unsigned int s_kMaxSecSkills = 8;
        static const unsigned int s_kMaxArtifacts = 64;
        static const unsigned int s_kMaxCreatureStacks = 7;

        TContents();

        int getExperienceBonus() const { return _m_experienceBonus; }
        void setExperienceBonus(int newExperienceBonus);
        int getManaBonus() const { return _m_manaBonus; }
        void setManaBonus(int newManaBonus);
        int getMoraleBonus() const { return _m_moraleBonus; }
        void setMoraleBonus(int newMoraleBonus);
        int getLuckBonus() const { return _m_luckBonus; }
        void setLuckBonus(int newLuckBonus);
        const TResourceQuantities& getResourceQuantities() const { return _m_resourceQuantities; }
        void setResourceQuantities(const TResourceQuantities& newQuantities) { _m_resourceQuantities = newQuantities; }
        const TPrimarySkillBonuses& getPrimarySkillBonuses() const { return _m_primarySkillBonuses; }
        void setPrimarySkillBonuses(const TPrimarySkillBonuses& newBonuses) { _m_primarySkillBonuses = newBonuses; }
        const vector<TSecondarySkillRecord>& getSecondarySkills() const { return _m_aSecondarySkill; }
        void setSecondarySkills(const vector<TSecondarySkillRecord>& aNewSecondarySkill);
        const vector<TArtifact>& getArtifacts() const { return _m_aArtifact; }
        void setArtifacts(const vector<TArtifact>& aNewArtifact);
        const vector<SpellID>& getSpells() const { return _m_aSpell; }
        void setSpells(const vector<SpellID>& aNewSpell);
        const vector<TCreatureStack>& getCreatureStacks() const { return _m_aCreatureStack; }
        void setCreatureStacks(const vector<TCreatureStack>& aNewCreatureStack);

        bool isCustomized() const;

    private:
        int _m_experienceBonus;
        int _m_manaBonus;
        int _m_moraleBonus;
        int _m_luckBonus;
        TResourceQuantities _m_resourceQuantities;
        TPrimarySkillBonuses _m_primarySkillBonuses;
        vector<TSecondarySkillRecord> _m_aSecondarySkill;
        vector<TArtifact> _m_aArtifact;
        vector<SpellID> _m_aSpell;
        vector<TCreatureStack> _m_aCreatureStack;
    };

    TBlackBox(const TObjectType& objType);
    TBlackBox(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;

    TContents* getPContents() { return _m_pContents.get(); }
    const TContents* getPContents() const { return _m_pContents.get(); }
    const TContents& getContents() const { return *getPContents(); }

    virtual bool isCustomized() const { return TTreasure::isCustomized() || _m_pContents->isCustomized(); }
    virtual bool hasText() const { return true; }

private:
    TRefCountingPtr<TContents> _m_pContents;
};

#endif  /* HOMM3_EDITOR_BLACKBOX_H */
