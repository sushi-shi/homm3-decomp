// CampaignDoc.cpp - the campaign document: loading and saving campaign files.
#include "campaign_editor/stdafx.h"

#include "va.h"
#include "editor/RawStream.h"
#include "campaign_editor/Campaign.h"

namespace {

class TScenarioStartingBonusWriter : public TScenarioStartingBonus::TVisitor {
public:
    void write(const TScenarioStartingBonus& bonus, TRawOStream& oStream, int version);

    virtual void visit(const TScenarioBonusSpell& bonus);
    virtual void visit(const TScenarioBonusCreature& bonus);
    virtual void visit(const TScenarioBonusBuilding& bonus);
    virtual void visit(const TScenarioBonusArtifact& bonus);
    virtual void visit(const TScenarioBonusSpellScroll& bonus);
    virtual void visit(const TScenarioBonusPrimarySkill& bonus);
    virtual void visit(const TScenarioBonusSecondarySkill& bonus);
    virtual void visit(const TScenarioBonusResource& bonus);

private:
    TRawOStream* m_pOStream;
    int m_version;
};

class TScenarioStartingOptionsWriter : public TScenarioStartingOptions::TVisitor {
public:
    virtual void visit(const TScenarioOptionsBonus& options);
    virtual void visit(const TScenarioOptionsCrossoverScenario& options);
    virtual void visit(const TScenarioOptionsStartingHero& options);

private:
    TRawOStream* m_pOStream;
    int m_version;
};

DATA(0x004c6f40) TScenarioStartingBonusWriter bonusWriter;
DATA(0x004c6f30) TScenarioStartingOptionsWriter optionsWriter;

VA(0x00410f30, 0x1d)
void TScenarioStartingBonusWriter::write(const TScenarioStartingBonus& bonus,
                                         TRawOStream& oStream, int version)
{
    m_pOStream = &oStream;
    m_version = version;
    bonus.accept(*this);
}

VA(0x00410f50, 0xb3)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusSpell& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusSpell);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    *m_pOStream << static_cast<ubyte>(bonus.m_spell);
}

VA(0x00411010, 0xe9)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusCreature& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusCreature);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    *m_pOStream << static_cast<short>(bonus.m_creature);
    *m_pOStream << static_cast<short>(bonus.m_count);
}

VA(0x00411100, 0x7b)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusBuilding& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusBuilding);
    *m_pOStream << static_cast<ubyte>(bonus.m_building);
}

VA(0x00411180, 0xb3)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusArtifact& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusArtifact);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    *m_pOStream << static_cast<short>(bonus.m_artifact);
}

VA(0x00411240, 0xb3)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusSpellScroll& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusSpellScroll);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    *m_pOStream << static_cast<ubyte>(bonus.m_spell);
}

VA(0x00411300, 0xd0)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusPrimarySkill& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusPrimarySkill);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    for (unsigned int i = 0; i < kNumPrimarySkills; i++)
        *m_pOStream << static_cast<ubyte>(bonus.m_skills[i]);
}

VA(0x004113d0, 0xe9)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusSecondarySkill& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusSecondarySkill);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    *m_pOStream << static_cast<ubyte>(bonus.m_skill);
    *m_pOStream << static_cast<ubyte>(bonus.m_level);
}

VA(0x004114c0, 0xb3)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusResource& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusResource);
    *m_pOStream << static_cast<ubyte>(bonus.m_resource);
    *m_pOStream << static_cast<long>(bonus.m_amount);
}

VA(0x004115a0, 0x540)
auto_ptr<TScenarioStartingBonus> readStartingBonus(TRawIStream& iStream, int version)
{
    signed char type;
    iStream >> type;
    auto_ptr<TScenarioStartingBonus> pBonus;
    switch (type) {
    case eBonusSpell: {
        short hero;
        iStream >> hero;
        signed char spell;
        iStream >> spell;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusSpell(hero, spell));
        break;
    }
    case eBonusCreature: {
        short hero;
        iStream >> hero;
        short creature;
        iStream >> creature;
        short count;
        iStream >> count;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusCreature(hero, creature, count));
        break;
    }
    case eBonusBuilding: {
        signed char building;
        iStream >> building;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusBuilding(building));
        break;
    }
    case eBonusArtifact: {
        short hero;
        iStream >> hero;
        int artifact;
        if (version >= 3) {
            short wideArtifact;
            iStream >> wideArtifact;
            artifact = wideArtifact;
        } else {
            signed char narrowArtifact;
            iStream >> narrowArtifact;
            artifact = narrowArtifact;
        }
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusArtifact(hero, artifact));
        break;
    }
    case eBonusSpellScroll: {
        short hero;
        iStream >> hero;
        signed char spell;
        iStream >> spell;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusSpellScroll(hero, spell));
        break;
    }
    case eBonusPrimarySkill: {
        short hero;
        iStream >> hero;
        int aSkills[kNumPrimarySkills];
        for (int i = 0; i < kNumPrimarySkills; i++) {
            signed char skill;
            iStream >> skill;
            aSkills[i] = skill;
        }
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusPrimarySkill(hero, aSkills));
        break;
    }
    case eBonusSecondarySkill: {
        short hero;
        iStream >> hero;
        signed char skill;
        iStream >> skill;
        signed char level;
        iStream >> level;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusSecondarySkill(hero, skill, level));
        break;
    }
    default: {
        signed char resource;
        iStream >> resource;
        long amount;
        iStream >> amount;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusResource(resource, amount));
        break;
    }
    }
    if (pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

VA(0x00411b80, 0xf7)
void TScenarioStartingOptionsWriter::visit(const TScenarioOptionsBonus& options)
{
    *m_pOStream << static_cast<ubyte>(eOptionsBonus);
    *m_pOStream << static_cast<ubyte>(options.m_player);
    *m_pOStream << static_cast<ubyte>(options.m_bonuses.size());
    for (unsigned int i = 0; i < options.m_bonuses.size(); i++)
        bonusWriter.write(*options.m_bonuses[i], *m_pOStream, m_version);
}

VA(0x00411c80, 0x11e)
void TScenarioStartingOptionsWriter::visit(const TScenarioOptionsCrossoverScenario& options)
{
    *m_pOStream << static_cast<ubyte>(eOptionsCrossoverScenario);
    *m_pOStream << static_cast<ubyte>(options.m_choices.size());
    for (unsigned int i = 0; i < options.m_choices.size(); i++) {
        const TScenarioOptionsCrossoverScenario::TChoice& choice = options.m_choices[i];
        *m_pOStream << static_cast<ubyte>(choice.m_player);
        *m_pOStream << static_cast<ubyte>(choice.m_scenario);
    }
}

VA(0x00411da0, 0x11e)
void TScenarioStartingOptionsWriter::visit(const TScenarioOptionsStartingHero& options)
{
    *m_pOStream << static_cast<ubyte>(eOptionsStartingHero);
    *m_pOStream << static_cast<ubyte>(options.m_choices.size());
    for (unsigned int i = 0; i < options.m_choices.size(); i++) {
        const TScenarioOptionsStartingHero::TChoice& choice = options.m_choices[i];
        *m_pOStream << static_cast<ubyte>(choice.m_player);
        *m_pOStream << static_cast<short>(choice.m_hero);
    }
}

}
