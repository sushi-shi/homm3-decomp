// Campaign.h - the campaign editor's model (Campaign.cpp): a scenario's
// starting options and the bonuses they offer. Class names are the RTTI's.
#ifndef HOMM3_CAMPAIGN_EDITOR_CAMPAIGN_H
#define HOMM3_CAMPAIGN_EDITOR_CAMPAIGN_H

#include <algorithm>
#include <bitset>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "va.h"
#include "campaignmap.h"
#include "primaryskill.h"
#include "editor/Army.h"
#include "editor/Array.h"
#include "editor/RefCountingPtr.h"
#include "Uncopyable.h"

class TRawIStream;
class TRawOStream;

// The game a campaign is made for: its maps' format version and the campaign
// maps it may use (the first 14 are Armageddon's Blade's, the rest The Shadow
// of Death's).
enum TCampaignVersion {
    eCampaignVersionRestorationOfErathia,
    eCampaignVersionArmageddonsBlade,
    eCampaignVersionShadowOfDeath
};

enum {
    kNumArmageddonsBladeCampaigns = 14,
    kNumShadowOfDeathCampaigns = 21
};

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
    eBonusResource,
    kNumBonusTypes
};

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

// The bonuses keep their fields private behind value getters, the accessor
// style of the editors' model classes (Loki h3maped's TVCAquireArtifact::
// getArtifact). Retail reads each field as a full-width value where it is
// used: the bonus writers load the hero and artifact DWORDs before their
// WORD stores, and the name formatter loads each index ahead of its
// _getHeroName call.
class TScenarioHeroBonus : public TScenarioStartingBonus {
public:
    // The heroes a bonus can name besides a hero id: the player's most
    // powerful hero, or the hero generated at the player's main town.
    enum { kMostPowerfulHero = -3, kGeneratedHero = -2 };

    explicit TScenarioHeroBonus(int hero) : m_hero(hero) {}

    int getHero() const { return m_hero; }

private:
    int m_hero;
};

class TScenarioBonusSpell : public TScenarioHeroBonus {
public:
    TScenarioBonusSpell(int hero, int spell) : TScenarioHeroBonus(hero), m_spell(spell) {}

    int getSpell() const { return m_spell; }

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusSpell& other) const
    {
        return getHero() == other.getHero() && m_spell == other.m_spell;
    }

private:
    int m_spell;
};

class TScenarioBonusCreature : public TScenarioHeroBonus {
public:
    TScenarioBonusCreature(int hero, const TCreatureStack& stack) : TScenarioHeroBonus(hero), m_stack(stack) {}

    const TCreatureStack& getStack() const { return m_stack; }

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusCreature& other) const
    {
        return getHero() == other.getHero() && m_stack == other.m_stack;
    }

private:
    TCreatureStack m_stack;
};

class TScenarioBonusBuilding : public TScenarioStartingBonus {
public:
    explicit TScenarioBonusBuilding(int building) : m_building(building) {}

    int getBuilding() const { return m_building; }

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusBuilding& other) const
    {
        return m_building == other.m_building;
    }

private:
    int m_building;
};

class TScenarioBonusArtifact : public TScenarioHeroBonus {
public:
    TScenarioBonusArtifact(int hero, int artifact) : TScenarioHeroBonus(hero), m_artifact(artifact) {}

    int getArtifact() const { return m_artifact; }

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusArtifact& other) const
    {
        return getHero() == other.getHero() && m_artifact == other.m_artifact;
    }

private:
    int m_artifact;
};

class TScenarioBonusSpellScroll : public TScenarioHeroBonus {
public:
    TScenarioBonusSpellScroll(int hero, int spell) : TScenarioHeroBonus(hero), m_spell(spell) {}

    int getSpell() const { return m_spell; }

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusSpellScroll& other) const
    {
        return getHero() == other.getHero() && m_spell == other.m_spell;
    }

private:
    int m_spell;
};

class TScenarioBonusPrimarySkill : public TScenarioHeroBonus {
public:
    TScenarioBonusPrimarySkill(int hero, const TArray<unsigned int, H3_IDX(kNumPrimarySkills)>& skills);

    const TArray<unsigned int, kNumPrimarySkills>& getSkills() const { return m_skills; }

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusPrimarySkill& other) const
    {
        return getHero() == other.getHero() && m_skills == other.m_skills;
    }

private:
    TArray<unsigned int, H3_IDX(kNumPrimarySkills)> m_skills;
};

class TScenarioBonusSecondarySkill : public TScenarioHeroBonus {
public:
    TScenarioBonusSecondarySkill(int hero, int skill, int level);

    int getSkill() const { return m_skill; }
    int getLevel() const { return m_level; }

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusSecondarySkill& other) const
    {
        return getHero() == other.getHero() && m_skill == other.m_skill && m_level == other.m_level;
    }

private:
    int m_skill;
    int m_level;
};

class TScenarioBonusResource : public TScenarioStartingBonus {
public:
    // The resources a bonus can name besides a resource type: wood and
    // ore, or the four rare resources.
    enum { kWoodAndOre = -3, kRareResources = -2 };

    TScenarioBonusResource(int resource, int amount);

    int getResource() const { return m_resource; }
    int getAmount() const { return m_amount; }

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    bool operator==(const TScenarioBonusResource& other) const
    {
        return m_resource == other.m_resource && m_amount == other.m_amount;
    }

private:
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

    VA(0x00405850, 0x7)
    virtual ~TScenarioStartingOptions() {}
    virtual void accept(TVisitor& visitor) const = 0;

    std::auto_ptr<TScenarioStartingOptions> clone() const;
};

bool operator==(const TScenarioStartingOptions& lhs, const TScenarioStartingOptions& rhs);

// The option sets are read through their getters (the bonus page's
// setOptions walks the getBonuses() reference; setMap, the writers and the
// equality tests use the same calls).
class TScenarioOptionsBonus : public TScenarioStartingOptions {
public:
    TScenarioOptionsBonus(int player, const std::vector<std::auto_ptr<TScenarioStartingBonus> >& bonuses);
    TScenarioOptionsBonus(const TScenarioOptionsBonus& other);

    int getPlayer() const { return m_player; }
    const std::vector<std::auto_ptr<TScenarioStartingBonus> >& getBonuses() const { return m_bonuses; }
    void setPlayer(int player) { m_player = player; }

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    void removeBonus(int index);

    int m_player;
    std::vector<std::auto_ptr<TScenarioStartingBonus> > m_bonuses;
};

bool operator==(const TScenarioOptionsBonus& lhs, const TScenarioOptionsBonus& rhs);

class TScenarioOptionsCrossoverScenario : public TScenarioStartingOptions {
public:
    // Readers take a choice's fields through its getters, as the edit
    // dialogs' getScenario/getPlayer do: retail loads each as a value where
    // it is used (the choice list label reads the player ahead of the map
    // name test, the edit dialog its scenario before the allowed-set store).
    struct TChoice {
        TChoice() : m_player(0) {}
        TChoice(int scenario, int player) : m_scenario(scenario), m_player(player) {}

        int getScenario() const { return m_scenario; }
        int getPlayer() const { return m_player; }

        bool operator==(const TChoice& other) const
        {
            return m_scenario == other.m_scenario && m_player == other.m_player;
        }

        int m_scenario;
        int m_player;
    };

    explicit TScenarioOptionsCrossoverScenario(const std::vector<TChoice>& choices);

    const std::vector<TChoice>& getChoices() const { return m_choices; }

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
    enum { kRandomHero = -1 };

    struct TChoice {
        TChoice() : m_hero(kRandomHero), m_player(0) {}
        TChoice(int hero, int player) : m_hero(hero), m_player(player) {}

        int getHero() const { return m_hero; }
        int getPlayer() const { return m_player; }

        bool operator==(const TChoice& other) const
        {
            return m_hero == other.m_hero && m_player == other.m_player;
        }

        int m_hero;
        int m_player;
    };

    explicit TScenarioOptionsStartingHero(const std::vector<TChoice>& choices);

    const std::vector<TChoice>& getChoices() const { return m_choices; }

    virtual void accept(TVisitor& visitor) const { visitor.visit(*this); }

    void removeChoice(int index);

    bool operator==(const TScenarioOptionsStartingHero& other) const
    {
        return m_choices == other.m_choices;
    }

    std::vector<TChoice> m_choices;
};

// The map a scenario plays, as the campaign keeps it: its file name, its
// name, each player's part and the heroes the map defines (RTTI
// TCampaignScenarioMap and its nested failures; one virtual slot, the
// destructor). Layout from the constructor 0x40a1f0: the file name at +4,
// the map's name at +0x14, eight players of 0x1c bytes from +0x24, the
// hero availability at +0x104 and the custom hero names at +0x118.
class TCampaignScenarioMap : private TUncopyable {
public:
    class TCreateFailure : public exception {
    };

    class TMapFileIsInvalid : public TCreateFailure {
    };

    class TMapFileIsIncorrectVersion : public TCreateFailure {
    public:
        TMapFileIsIncorrectVersion(int maxVersion, int version) : m_maxVersion(maxVersion), m_version(version) {}

        int m_maxVersion;
        int m_version;
    };

    class TMapIsUnplayable : public TCreateFailure {
    };

    // A player of the map as a scenario offers it.
    struct TPlayerInfo {
        VA(0x0040a6e0, 0xd1)
        TPlayerInfo()
            : m_bPresent(false), m_bHumanPlayable(false), m_bHasMainTown(false), m_bGenerateHeroAtMainTown(false),
              m_bHasRandomHero(false), m_numPlaceholders(0), m_mainTownType(-1)
        {
        }

        // Accessors in the style of the map editor's own TPlayerInfo, which
        // Loki h3maped fills through setBHumanPlayable, setBComputerPlayable,
        // setBehaviorType and setBGenerateHero and reads through getBPresent.
        void setBPresent(bool bPresent);
        void setMainTown(int townType);
        void setHeroes(const std::map<int, std::string>& newHeroes);
        void setBHumanPlayable(bool bHumanPlayable) { m_bHumanPlayable = bHumanPlayable; }
        void setNumPlaceholders(int numPlaceholders) { m_numPlaceholders = numPlaceholders; }
        void setBGenerateHeroAtMainTown(bool bGenerate) { m_bGenerateHeroAtMainTown = bGenerate; }
        void setBHasRandomHero(bool bHasRandomHero) { m_bHasRandomHero = bHasRandomHero; }

        bool isHumanPlayable() const { return m_bPresent && m_bHumanPlayable; }
        int getMainTownType() const { return m_bHasMainTown ? m_mainTownType : -1; }
        bool getBGenerateHeroAtMainTown() const { return m_bGenerateHeroAtMainTown; }
        bool getBHasRandomHero() const { return m_bHasRandomHero; }
        const std::map<int, std::string>& getHeroes() const { return m_heroes; }

        bool m_bPresent : 1;
        bool m_bHumanPlayable : 1;
        bool m_bHasMainTown : 1;
        bool m_bGenerateHeroAtMainTown : 1;
        bool m_bHasRandomHero : 1;
        int m_numPlaceholders;
        int m_mainTownType;
        std::map<int, std::string> m_heroes;
    };

    enum { kNumPlayers = 8, kNumHeroes = 156 };

    TCampaignScenarioMap(const std::string& fileName, std::streambuf* pStreamBuf, int campaignVersion,
                         bool bAnyVersion);
    virtual ~TCampaignScenarioMap();

    std::string getHeroName(int hero) const;

    // Readers outside the map take its players and hero set through these
    // (the starting-hero page's prepare copies getHeroes(); setMap and the
    // dialogs index players with getPlayer).
    const TPlayerInfo& getPlayer(int player) const { return m_aPlayer[player]; }
    const std::bitset<kNumHeroes>& getHeroes() const { return m_heroes; }

    std::string m_fileName;
    std::string m_name;
    TPlayerInfo m_aPlayer[kNumPlayers];
    std::bitset<kNumHeroes> m_heroes;
    std::map<int, std::string> m_heroNames;
};

// A scenario's prologue or epilogue: a movie (a row of the movie table), a
// music theme and the text shown over them.
class TScenarioPrologue {
public:
    TScenarioPrologue();
    TScenarioPrologue(int movie, int music, const std::string& text);

    int getMovie() const { return m_movie; }
    int getMusic() const { return m_music; }
    const std::string& getText() const { return m_text; }

    bool operator==(const TScenarioPrologue& other) const
    {
        return m_movie == other.m_movie && m_music == other.m_music && m_text == other.m_text;
    }

private:
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

    bool getBRetained(TRetained retained) const { return m_retained[retained]; }
    void setBRetained(TRetained retained, bool bRetained) { m_retained[retained] = bRetained; }
    // The sets are read and replaced whole, as the crossover page does.
    const std::bitset<kNumCreatures>& getCreatures() const { return m_creatures; }
    const std::bitset<kNumArtifacts>& getArtifacts() const { return m_artifacts; }
    void setCreatures(const std::bitset<kNumCreatures>& creatures) { m_creatures = creatures; }
    void setArtifacts(const std::bitset<kNumArtifacts>& artifacts) { m_artifacts = artifacts; }

    bool operator==(const TScenarioCrossover& other) const
    {
        return m_retained == other.m_retained && m_creatures == other.m_creatures
               && m_artifacts == other.m_artifacts;
    }

private:
    std::bitset<kNumRetained> m_retained;
    std::bitset<kNumCreatures> m_creatures;
    std::bitset<kNumArtifacts> m_artifacts;
};

// A scenario of a campaign: a copy-on-write handle.
class TScenario {
public:
    // The properties sheet's region text limit.
    enum { s_kMaxRegionDescLen = 600 };

    enum TDifficulty {
        eDifficultyEasy,
        eDifficultyNormal,
        eDifficultyHard,
        eDifficultyExpert,
        eDifficultyImpossible,
        kNumDifficulties
    };

    explicit TScenario(int numScenarios);

    void setCrossover(const TScenarioCrossover& newCrossover);
    const TCampaignScenarioMap* getMap() const;
    // A player of the scenario's map (callers read the player before the
    // map, h3ccmped 0x42e5a0 and 0x42f780).
    const TCampaignScenarioMap::TPlayerInfo& getPlayer(int player) const { return getMap()->getPlayer(player); }
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

    // The properties dialog's edit limits.
    enum { s_kMaxNameLen = 60, s_kMaxDescriptionLen = 300 };

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
    // The scenario to change: its copy of the campaign's data becomes this
    // handle's own.
    TScenario& modifyScenario(int scenario);
    int getType() const;
    const TCampaignMapTraits& getMapTraits() const { return g_campaignMapTraits[getType()]; }
    const std::string& getName() const;
    const std::string& getDescription() const;
    bool getBDifficultyChoice() const;
    int getMusic() const;
    const TScenario& getScenario(int scenario) const;
    bool getBPrerequisite(int scenario, int prerequisite) const;
    bool getBDirectPrerequisite(int scenario, int prerequisite) const;
    void importText(std::istream& stream, int version);
    void exportText(std::ostream& stream, int version) const;

private:
    class _TImpl;

    TRefCountingPtr<_TImpl> _m_pImpl;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGN_H */
