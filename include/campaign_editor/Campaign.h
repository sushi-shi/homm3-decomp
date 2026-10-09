// Campaign.h - the campaign editor's model (Campaign.cpp): a scenario's
// starting options and the bonuses they offer. Class names are the RTTI's.
#ifndef HOMM3_CAMPAIGN_EDITOR_CAMPAIGN_H
#define HOMM3_CAMPAIGN_EDITOR_CAMPAIGN_H

#include <algorithm>
#include <bitset>
#include <memory>
#include <string>
#include <vector>

#include "editor/RefCountingPtr.h"

class TRawIStream;
class TRawOStream;

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

    std::auto_ptr<TScenarioStartingBonus> clone() const;
};

bool operator==(const TScenarioStartingBonus& lhs, const TScenarioStartingBonus& rhs);

class TScenarioHeroBonus : public TScenarioStartingBonus {
public:
    explicit TScenarioHeroBonus(int hero) : m_hero(hero) {}

    int m_hero;
};

class TScenarioBonusSpell : public TScenarioHeroBonus {
public:
    TScenarioBonusSpell(int hero, int spell) : TScenarioHeroBonus(hero), m_spell(spell) {}

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusSpell& other) const
    {
        return m_hero == other.m_hero && m_spell == other.m_spell;
    }

    int m_spell;
};

class TScenarioBonusCreature : public TScenarioHeroBonus {
public:
    TScenarioBonusCreature(int hero, int creature, int count)
        : TScenarioHeroBonus(hero), m_creature(creature), m_count(count) {}

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusCreature& other) const
    {
        return m_hero == other.m_hero && m_creature == other.m_creature && m_count == other.m_count;
    }

    int m_creature;
    int m_count;
};

class TScenarioBonusBuilding : public TScenarioStartingBonus {
public:
    explicit TScenarioBonusBuilding(int building) : m_building(building) {}

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusBuilding& other) const
    {
        return m_building == other.m_building;
    }

    int m_building;
};

class TScenarioBonusArtifact : public TScenarioHeroBonus {
public:
    TScenarioBonusArtifact(int hero, int artifact) : TScenarioHeroBonus(hero), m_artifact(artifact) {}

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusArtifact& other) const
    {
        return m_hero == other.m_hero && m_artifact == other.m_artifact;
    }

    int m_artifact;
};

class TScenarioBonusSpellScroll : public TScenarioHeroBonus {
public:
    TScenarioBonusSpellScroll(int hero, int spell) : TScenarioHeroBonus(hero), m_spell(spell) {}

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusSpellScroll& other) const
    {
        return m_hero == other.m_hero && m_spell == other.m_spell;
    }

    int m_spell;
};

class TScenarioBonusPrimarySkill : public TScenarioHeroBonus {
public:
    TScenarioBonusPrimarySkill(int hero, const int aSkills[kNumPrimarySkills]);

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusPrimarySkill& other) const
    {
        return m_hero == other.m_hero && std::equal(m_skills, m_skills + kNumPrimarySkills, other.m_skills);
    }

    int m_skills[kNumPrimarySkills];
};

class TScenarioBonusSecondarySkill : public TScenarioHeroBonus {
public:
    TScenarioBonusSecondarySkill(int hero, int skill, int level);

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusSecondarySkill& other) const
    {
        return m_hero == other.m_hero && m_skill == other.m_skill && m_level == other.m_level;
    }

    int m_skill;
    int m_level;
};

class TScenarioBonusResource : public TScenarioStartingBonus {
public:
    TScenarioBonusResource(int resource, int amount);

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusResource& other) const
    {
        return m_resource == other.m_resource && m_amount == other.m_amount;
    }

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

    std::auto_ptr<TScenarioStartingOptions> clone() const;
};

bool operator==(const TScenarioStartingOptions& lhs, const TScenarioStartingOptions& rhs);

class TScenarioOptionsBonus : public TScenarioStartingOptions {
public:
    TScenarioOptionsBonus(int player, const std::vector<std::auto_ptr<TScenarioStartingBonus> >& bonuses);
    TScenarioOptionsBonus(const TScenarioOptionsBonus& other);

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    void removeBonus(int index);

    int m_player;
    std::vector<std::auto_ptr<TScenarioStartingBonus> > m_bonuses;
};

bool operator==(const TScenarioOptionsBonus& lhs, const TScenarioOptionsBonus& rhs);

class TScenarioOptionsCrossoverScenario : public TScenarioStartingOptions {
public:
    struct TChoice {
        bool operator==(const TChoice& other) const
        {
            return m_scenario == other.m_scenario && m_player == other.m_player;
        }

        int m_scenario;
        int m_player;
    };

    explicit TScenarioOptionsCrossoverScenario(const std::vector<TChoice>& choices) : m_choices(choices) {}

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    void removeChoice(int index);

    bool operator==(const TScenarioOptionsCrossoverScenario& other) const
    {
        return m_choices == other.m_choices;
    }

    std::vector<TChoice> m_choices;
};

class TScenarioOptionsStartingHero : public TScenarioStartingOptions {
public:
    struct TChoice {
        bool operator==(const TChoice& other) const
        {
            return m_hero == other.m_hero && m_player == other.m_player;
        }

        int m_hero;
        int m_player;
    };

    explicit TScenarioOptionsStartingHero(const std::vector<TChoice>& choices) : m_choices(choices) {}

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    void removeChoice(int index);

    bool operator==(const TScenarioOptionsStartingHero& other) const
    {
        return m_choices == other.m_choices;
    }

    std::vector<TChoice> m_choices;
};

// The map a scenario plays, as the campaign keeps it (RTTI
// TCampaignScenarioMap; one virtual slot, the destructor).
class TCampaignScenarioMap {
public:
    virtual ~TCampaignScenarioMap();
};

// A scenario's prologue or epilogue: a movie (a row of the movie table), a
// music theme and the text shown over them.
class TScenarioPrologue {
public:
    TScenarioPrologue();
    TScenarioPrologue(int movie, int music, const std::string& text);

    int m_movie;
    int m_music;
    std::string m_text;
};

TRawIStream& operator>>(TRawIStream& stream, TScenarioPrologue& prologue);
TRawOStream& operator<<(TRawOStream& stream, const TScenarioPrologue& prologue);

// What crosses over into the scenario from the previous one: which hero
// properties are kept, and the creatures and artifacts the heroes keep.
class TScenarioCrossover {
public:
    enum TRetained {
        eRetainExperience,
        eRetainPrimarySkills,
        eRetainSecondarySkills,
        eRetainSpells,
        eRetainArtifacts,
        kNumRetained
    };

    enum { kNumCreatures = 145, kNumArtifacts = 144 };

    TScenarioCrossover();

    void read(TRawIStream& stream, int version);
    void write(TRawOStream& stream, int version) const;

    std::bitset<kNumRetained> m_retained;
    std::bitset<kNumCreatures> m_creatures;
    std::bitset<kNumArtifacts> m_artifacts;
};

// A scenario of a campaign: a copy-on-write handle.
class TScenario {
public:
    explicit TScenario(int numScenarios);

    void setCrossover(const TScenarioCrossover& newCrossover);
    const TCampaignScenarioMap* getMap() const;
    int getRegionColor() const;
    int getDifficulty() const;
    const std::string& getRegionDesc() const;
    const TScenarioPrologue* getPrologue() const;
    const TScenarioPrologue* getEpilogue() const;
    const TScenarioCrossover& getCrossover() const;
    const TScenarioStartingOptions* getStartingOptions() const;
    void setMap(std::auto_ptr<TCampaignScenarioMap> pMap);
    void removeMap();
    void setBPrerequisite(int scenario, bool bPrerequisite);
    void setStartingOptions(std::auto_ptr<TScenarioStartingOptions> pOptions);
    void removeStartingOptionsChoice(int index);
    bool getBPrerequisite(int scenario) const;
    void setRegionColor(int newRegionColor);
    void setDifficulty(int newDifficulty);
    void setRegionDesc(const std::string& newRegionDesc);
    void setPrologue(std::auto_ptr<TScenarioPrologue> pPrologue);
    void removePrologue();
    void setEpilogue(std::auto_ptr<TScenarioPrologue> pEpilogue);
    void removeEpilogue();

private:
    class _TImpl;

    TRefCountingPtr<_TImpl> _m_pImpl;
};

// The campaign: a copy-on-write handle.
class TCampaign {
public:
    class TImportTextFailure : public exception {
    };

    explicit TCampaign(int type);
    TCampaign(const TCampaign& other);
    ~TCampaign();

    TCampaign& operator=(const TCampaign& other);

    void setName(const std::string& newName);
    void setDescription(const std::string& newDescription);
    void setBDifficultyChoice(bool bDifficultyChoice);
    void setMusic(int newMusic);
    void setScenarioMap(int scenario, std::auto_ptr<TCampaignScenarioMap> pMap);
    void removeScenarioMap(int scenario);
    void setBPrerequisite(int scenario, int prerequisite, bool bPrerequisite);
    void setScenarioStartingOptions(int scenario, std::auto_ptr<TScenarioStartingOptions> pOptions);
    TScenario& getScenario(int scenario);
    int getType() const;
    const std::string& getName() const;
    const std::string& getDescription() const;
    bool getBDifficultyChoice() const;
    int getMusic() const;
    const TScenario& getScenario(int scenario) const;
    bool getBPrerequisite(int scenario, int prerequisite) const;
    bool getBDirectPrerequisite(int scenario, int prerequisite) const;

private:
    class _TImpl;

    TRefCountingPtr<_TImpl> _m_pImpl;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGN_H */
