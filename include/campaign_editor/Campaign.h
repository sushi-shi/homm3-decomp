// Campaign.h - the campaign editor's model (Campaign.cpp): a scenario's
// starting options and the bonuses they offer. Class names are the RTTI's.
#ifndef HOMM3_CAMPAIGN_EDITOR_CAMPAIGN_H
#define HOMM3_CAMPAIGN_EDITOR_CAMPAIGN_H

#include <memory>
#include <vector>

class TScenarioBonusSpell;
class TScenarioBonusCreature;
class TScenarioBonusBuilding;
class TScenarioBonusArtifact;
class TScenarioBonusSpellScroll;
class TScenarioBonusPrimarySkill;
class TScenarioBonusSecondarySkill;
class TScenarioBonusResource;

// The bonus types as a campaign file numbers them.
enum TScenarioBonusType {
    eBonusSpell,
    eBonusCreature,
    eBonusBuilding,
    eBonusArtifact,
    eBonusSpellScroll,
    eBonusPrimarySkill,
    eBonusSecondarySkill,
    eBonusResource
};

enum { kNumPrimarySkills = 4 };

class TScenarioStartingBonus {
public:
    // VC6 lays overloaded virtuals out in reverse declaration order: the
    // spell's visit is the last slot.
    class TVisitor {
    public:
        virtual void visit(const TScenarioBonusSpell&) {}
        virtual void visit(const TScenarioBonusCreature&) {}
        virtual void visit(const TScenarioBonusBuilding&) {}
        virtual void visit(const TScenarioBonusArtifact&) {}
        virtual void visit(const TScenarioBonusSpellScroll&) {}
        virtual void visit(const TScenarioBonusPrimarySkill&) {}
        virtual void visit(const TScenarioBonusSecondarySkill&) {}
        virtual void visit(const TScenarioBonusResource&) {}
    };

    virtual ~TScenarioStartingBonus() {}
    virtual void accept(TVisitor& visitor) const = 0;
};

class TScenarioHeroBonus : public TScenarioStartingBonus {
public:
    explicit TScenarioHeroBonus(int hero) : m_hero(hero) {}

    int m_hero;
};

class TScenarioBonusSpell : public TScenarioHeroBonus {
public:
    TScenarioBonusSpell(int hero, int spell) : TScenarioHeroBonus(hero), m_spell(spell) {}

    virtual void accept(TVisitor& visitor) const;

    int m_spell;
};

class TScenarioBonusCreature : public TScenarioHeroBonus {
public:
    TScenarioBonusCreature(int hero, int creature, int count)
        : TScenarioHeroBonus(hero), m_creature(creature), m_count(count) {}

    virtual void accept(TVisitor& visitor) const;

    int m_creature;
    int m_count;
};

class TScenarioBonusBuilding : public TScenarioStartingBonus {
public:
    explicit TScenarioBonusBuilding(int building) : m_building(building) {}

    virtual void accept(TVisitor& visitor) const;

    int m_building;
};

class TScenarioBonusArtifact : public TScenarioHeroBonus {
public:
    TScenarioBonusArtifact(int hero, int artifact) : TScenarioHeroBonus(hero), m_artifact(artifact) {}

    virtual void accept(TVisitor& visitor) const;

    int m_artifact;
};

class TScenarioBonusSpellScroll : public TScenarioHeroBonus {
public:
    TScenarioBonusSpellScroll(int hero, int spell) : TScenarioHeroBonus(hero), m_spell(spell) {}

    virtual void accept(TVisitor& visitor) const;

    int m_spell;
};

class TScenarioBonusPrimarySkill : public TScenarioHeroBonus {
public:
    TScenarioBonusPrimarySkill(int hero, const int aSkills[kNumPrimarySkills]);

    virtual void accept(TVisitor& visitor) const;

    int m_skills[kNumPrimarySkills];
};

class TScenarioBonusSecondarySkill : public TScenarioHeroBonus {
public:
    TScenarioBonusSecondarySkill(int hero, int skill, int level);

    virtual void accept(TVisitor& visitor) const;

    int m_skill;
    int m_level;
};

class TScenarioBonusResource : public TScenarioStartingBonus {
public:
    TScenarioBonusResource(int resource, int amount);

    virtual void accept(TVisitor& visitor) const;

    int m_resource;
    int m_amount;
};

class TScenarioOptionsBonus;
class TScenarioOptionsCrossoverScenario;
class TScenarioOptionsStartingHero;

// The starting option types as a campaign file numbers them.
enum TScenarioOptionsType {
    eOptionsNone,
    eOptionsBonus,
    eOptionsCrossoverScenario,
    eOptionsStartingHero
};

class TScenarioStartingOptions {
public:
    class TVisitor {
    public:
        virtual void visit(const TScenarioOptionsBonus&) {}
        virtual void visit(const TScenarioOptionsCrossoverScenario&) {}
        virtual void visit(const TScenarioOptionsStartingHero&) {}
    };

    virtual ~TScenarioStartingOptions() {}
    virtual void accept(TVisitor& visitor) const = 0;
};

class TScenarioOptionsBonus : public TScenarioStartingOptions {
public:
    virtual void accept(TVisitor& visitor) const;

    int m_player;
    std::vector<std::auto_ptr<TScenarioStartingBonus> > m_bonuses;
};

class TScenarioOptionsCrossoverScenario : public TScenarioStartingOptions {
public:
    struct TChoice {
        int m_scenario;
        int m_player;
    };

    virtual void accept(TVisitor& visitor) const;

    std::vector<TChoice> m_choices;
};

class TScenarioOptionsStartingHero : public TScenarioStartingOptions {
public:
    struct TChoice {
        int m_hero;
        int m_player;
    };

    virtual void accept(TVisitor& visitor) const;

    std::vector<TChoice> m_choices;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGN_H */
