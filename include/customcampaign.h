// customcampaign.h - canonical campaign state and Complete campaign types.
// CodeView places SCampaign's constructor and completion query in this header.
#ifndef HOMM3_CUSTOMCAMPAIGN_H
#define HOMM3_CUSTOMCAMPAIGN_H

#include <string>
#include <string.h>
#include <vector>

#include "campaignmusic.h"
#include "hero.h"

class CMapHeaderData;

// The two 0x10-byte sub-objects SCampaign carries at +0x3c and +0x4c.
// TCampaignWindow's constructor is the proof: `gpGame->campaign =
// SCampaign()` is a compiler-generated memberwise assignment there, and it
// calls a distinct out-of-line operator= for each slot (0x45f5e0, 0x45f810)
// with the mirror destructors in the temporary's teardown (0x45f560,
// 0x45f7b0). Both destructors walk an outer array of 0x10-byte vectors and
// free each one, i.e. both members are std::vector<std::vector<T> >:
// 0x45f560's leaves are destroyed one at a time with a 0x492 stride - the
// byte-for-byte size of `hero`, which with the Dreamcast's
// SCampaign::clear_carryover_pool(TCarryOverPoolNumber) identifies the
// +0x3c slot as the campaign's carry-over hero pools - while 0x45f7b0's
// inner elements are trivially destroyed and its element type stays
// unidentified. SCampaign carries both slots as those nested vectors (the
// retail PC layout agrees with IDA's independently recovered type record).
// The two four-dword opaque twins that once shadowed them for
// campaignwindow.obj alone are RETIRED (2026-09-05).

// Complete's per-scenario campaign progress record. The name and return type
// survive in the independently located HD GetCurrentScenario signature;
// retail fixes the 0x14 stride and the completed/days/score head. The two
// four-byte tail fields complete the same cross-build record instead of
// leaving source-visible state as anonymous padding.
struct CampaignScenarioInfo {
public:
    bool m_completed;
    int m_days;
    int m_score;
    int m_index;
    int m_completeOrder;
    CampaignScenarioInfo()
        : m_completed(false), m_days(0), m_score(0), m_index(-1), m_completeOrder(0)
    {
    }
};
SIZE(CampaignScenarioInfo, 0x14);

// The DC campaign backend stored map_traits[8][32] at SCampaign+1408,
// each 36-byte TCustomCampaignTraits owning two 8-byte TArtifactRequirement
// records. Its init/set/constructor helpers and InitCampaignMapTraits populated
// that fixed table (dc 0x7cc8c/0x7cccc/0x7cd4c, 0xbcd20..0xbcd90).
// Complete's 0x7c-byte SCampaign instead owns dynamic hero/artifact pools and
// CampaignScenarioInfo progress records; ScenarioStruct::read (0x487e40) loads
// per-scenario retention flags and crossover artifact masks from the campaign
// file. The fixed-table helpers have no storage-compatible desktop owner.
// DC GetExpCap (CustomCampaign.h:225, dc 0xd5944) indexed that table; retail
// hero::giveExperience (0x4e33b0) reads NewSMapHeader::m_maxHeroLevel and converts
// the level through getExperience. DC game::mark_campaign_map_won (0xbc384)
// wrote three fixed progress arrays. Complete's SCampaign::completeCurrentMap
// (0x489820) owns the dynamic scenario record, completion order and crossover
// pools, and is called directly by oldmain. These retired interfaces are
// recorded individually in config/source/dc_only.tsv.

class SCampaign {
public:
    enum {
        PRE36_CAMPAIGN_REMAP_SOURCE = 13,
        PRE36_CAMPAIGN_REMAP_TARGET = 20,
        // The constructor's currentCampaign sentinel, one past the last
        // built-in campaign ordinal.
        CAMPAIGN_NONE = 21,
        // ApplyBriefingChoice's two alignment-choice sites: the third map
        // of campaigns 0 and 4 rewrites setup.alignment[2] (names
        // provisional, from the retail compares).
        ALIGNMENT_CHOICE_CAMPAIGN_A = 0,
        ALIGNMENT_CHOICE_CAMPAIGN_B = 4,
        ALIGNMENT_CHOICE_MAP = 2,
        BRIEFING_CHOICE_FIRST = 0,
        BRIEFING_CHOICE_SECOND = 1
    };
    // Compatibility spelling for the already reconstructed vector helpers;
    // as a typedef it still gives VC6 the authoritative global element type.
    typedef CampaignScenarioInfo MapScore;
    unsigned char m_isCheater;
    unsigned char m_secretActive;
    signed char m_currentMap;
    // The alignment gaps after currentMap, crossoverArrayIndex and
    // campaignCompleted are deliberately IMPLICIT. Retail's generated
    // assignment does not copy them; naming them as char members created two
    // extra byte-copy loops in TCampaignWindow's constructor.
    int m_currentCampaign;
    int m_numMapRegions;
    signed char m_crossoverArrayIndex;
    int m_briefingChoice;
    // +0x14, and a std::string rather than the char[0x10] this was: the
    // memberwise assignment in TCampaignWindow's constructor drives the
    // slot through basic_string::assign(that, 0, npos) (0x404860, with the
    // npos word at 0x63a60c) and the temporary's teardown ends on
    // basic_string::_Tidy(true) against it. Same 0x10 width, so nothing
    // after it moves.
    std::string m_campaignFilename;
    std::string getCampaignFileName() const;
    unsigned char m_campaignCompleted[21];
    // +0x3c / +0x4c: the carry-over hero pools and the artifact pools
    // (proved by the two out-of-line operator=/destructor pairs above).
    std::vector<std::vector<hero> > m_carryOverHeroes;
    std::vector<std::vector<type_artifact> > m_carryoverArtifact;
    std::vector<MapScore> m_mapScores;
    // +0x6c, the fourth assignable sub-object. Its operator= is the
    // four-byte-element vector::operator= at 0x50ac00 and its teardown is
    // INLINE in the same constructor - _Destroy over [_First, _Last),
    // operator delete on _First, then all three words zeroed - so the slot
    // is a std::vector over a 4-byte element whose identity is unproven.
    std::vector<int> m_assignedCarryover;
    // E:\gamedcs\CustomCampaign.h:199, dc 0xbcd90
    VA(0x00489500, 0x88)  // dc 0xbcd90
    SCampaign()
    {
        m_isCheater = 0;
        m_secretActive = 0;
        m_currentMap = -1;
        m_numMapRegions = -1;
        m_briefingChoice = -1;
        m_crossoverArrayIndex = -1;
        m_currentCampaign = CAMPAIGN_NONE;
        memset(m_campaignCompleted, 0, sizeof(m_campaignCompleted));
    }
    void selectCampaign(int campaignIndex, const char* filename);
    // nameable before the campaign-brief declarations; the receiver,
    void playScenarioPrologue(void* campaignHeader);
    // Retail 0x48a2a0, the prologue player's twin on the scenario's
    // epilogue record; oldmain's end-of-campaign arm calls the two
    // Complete-only members below on gpGame->campaign, 0x489820 before
    // SaveGame(1) and this one after it. 0x489e20 is 0x489820's own tail
    // call. Same opaque campaign-header parameter and the same reason,
    // and all three names are role-based and provisional: the Dreamcast
    // customcampaign.obj roster stops before them.
    void completeCurrentMap(void* campaignHeader);
    void pruneCrossoverHeroes(void* campaignHeader);
    int findLatestCrossoverScenario(int slot) const;
    void playScenarioEpilogue(void* campaignHeader);
    void applyBriefingChoice(int option);
    void doPreLoadCustomization();
    // E:\gamedcs\CustomCampaign.h:212, dc 0xe6ef8
    // Original CampaignComplete@SCampaign@@QAA_NXZ (native bool, mutable).
    // DC's older fixed-array body also marks campaignCompleted. Retail's
    // 67-byte vector scan has no such store; preserve the Complete behavior.
    VA(0x004897d0, 0x43)  // dc 0xe6ef8
    bool campaignComplete()
    {
        for (unsigned int i = 0; i < m_mapScores.size(); ++i) {
            if (!m_mapScores[i].m_completed)
                return 0;
        }
        return 1;
    }
    int getScore() const;
    int getTotalTime() const;
    // Provisional name; PlaceCrossoverHeroes retains this lookup's nested
    // vector::size calls while expanding the ordinary member itself.
    hero* findCrossoverHero(int heroId);
    void save(TAbstractFile* outfile);
    // Retail-only load surface at 0x48a310; SavedGameHeader::Load passes the
    // stream and save version and the callee reads both.
    void load(TAbstractFile* infile, int saveVersion);
    VA(0x0057C780, 0x0E)  // hd-crossbuild masked identity + sole retail caller
    CampaignScenarioInfo* getCurrentScenario()
    {
        return &m_mapScores[m_currentMap];
    }
};
SIZE(SCampaign, 0x7c);

extern const SCampaignMusicCue* g_campaignMusicTraits;

// The eight campaign start bonuses. THE HIERARCHY IS BYTE-PROVEN by the
// bonus-list reader at 0x485190, which switches a type byte 0..7 and
// `new`s an object of the matching vftable:
//   0 spell 0x63daa0 (12 B)   1 creature 0x63da80 (16 B)
//   2 building 0x63da60 (12)  3 artifact 0x63da40 (12)
//   4 scroll 0x63da20 (12)    5 primary skill 0x63da00 (12)
//   6 secondary skill 0x63d9e0 (16)  7 resource 0x63d9c0 (12)
// Every vftable is eight slots wide and the abstract root's own is
// 0x63d938 - six `__purecall` entries between the deleting destructor at
// slot 0 and the do-nothing slot 7. The slot ROLES read off the bodies:
// slot 1 is the building predicate (only 0x63da60 answers true), slot 2
// returns the icon .def name, slot 3 the frame in it, slot 4 the
// format_string'd description, slot 5 applies the bonus to a player and
// slot 6 deserializes it. NAMES ARE ROLE INVENTIONS - the compiland has
// no Dreamcast twin (the port's SCampaign::give_custom_items did all of
// this longhand) and no RTTI descriptor names any of these classes.
class TAbstractFile;

class TCampaignBonus {
public:
    // Out of line at 0x485370, seven bytes of vftable restore; its
    // scalar deleting destructor is 0x484020.
    virtual ~TCampaignBonus();
    virtual bool isBuildingBonus() const;
    virtual const char* getIconDefName() const = 0;
    virtual int getIconIndex() const = 0;
    virtual std::string getText() const = 0;
    virtual void apply(int whichPlayer) const = 0;
    virtual void read(TAbstractFile* file) = 0;
    // 0x485d80, `ret 4`. Only the building bonus overrides it (0x4847e0),
    // where the town remaps the building index.
    virtual void setTown(int town);
};

// Spell: the hero id it is granted to and the spell. Read takes a SIGNED
// word then an unsigned byte (0x484050).
class TCampaignSpellBonus : public TCampaignBonus {
public:
    virtual const char* getIconDefName() const;
    virtual int getIconIndex() const { return m_spell; }
    virtual std::string getText() const;
    virtual void apply(int whichPlayer) const;
    virtual void read(TAbstractFile* file);

    int m_hero;
    int m_spell;
};

// The scroll shares the spell's whole surface except the description and
// the apply, which is what puts its Read, icon name and icon frame at the
// SAME addresses in both vftables (0x484050 / 0x484090 / the folded
// three-byte getter).
class TCampaignSpellScrollBonus : public TCampaignSpellBonus {
public:
    virtual std::string getText() const;
    virtual void apply(int whichPlayer) const;
};

// Creature: hero, creature type and count, all three read as words
// (0x4844f0) - the first two signed, the count unsigned.
class TCampaignCreatureBonus : public TCampaignBonus {
public:
    virtual const char* getIconDefName() const;
    virtual int getIconIndex() const;
    virtual std::string getText() const;
    virtual void apply(int whichPlayer) const;
    virtual void read(TAbstractFile* file);

    int m_hero;
    int m_creature;
    int m_count;
};

// Building: the only bonus whose Read fills ONE field (0x4845f0 reads a
// single byte into the building slot); the town arrives later through
// SetTown, which is also where the building index is remapped.
class TCampaignBuildingBonus : public TCampaignBonus {
public:
    virtual bool isBuildingBonus() const;
    virtual const char* getIconDefName() const;
    virtual int getIconIndex() const { return 0; }
    virtual std::string getText() const;
    virtual void apply(int whichPlayer) const;
    virtual void read(TAbstractFile* file);
    virtual void setTown(int town);

    int m_town;
    int m_building;
};

// Artifact: hero and artifact, both signed words (0x4848a0).
class TCampaignArtifactBonus : public TCampaignBonus {
public:
    virtual const char* getIconDefName() const;
    virtual int getIconIndex() const { return m_artifact; }
    virtual std::string getText() const;
    virtual void apply(int whichPlayer) const;
    virtual void read(TAbstractFile* file);

    int m_hero;
    int m_artifact;
};

// Primary skills: a signed word hero id and then the four skill deltas
// read as raw bytes straight into the object (0x484bf0's `add edi,8`
// before a four-byte Read is what proves the array is the member, not
// four ints).
class TCampaignPrimarySkillBonus : public TCampaignBonus {
public:
    virtual const char* getIconDefName() const;
    virtual int getIconIndex() const;
    virtual std::string getText() const;
    virtual void apply(int whichPlayer) const;
    virtual void read(TAbstractFile* file);

    int m_hero;
    char m_skills[4];
};

// Secondary skill: a signed word hero id, then the skill and its mastery
// as unsigned bytes (0x484cf0).
class TCampaignSecondarySkillBonus : public TCampaignBonus {
public:
    virtual const char* getIconDefName() const;
    virtual int getIconIndex() const;
    virtual std::string getText() const;
    virtual void apply(int whichPlayer) const;
    virtual void read(TAbstractFile* file);

    int m_hero;
    int m_skill;
    int m_level;
};

// Resource: a SIGNED byte selector and a full dword amount (0x484f00).
// The negative selectors are the two mixed rows the icon frame folds onto
// 7 and 8 (0x484d70).
class TCampaignResourceBonus : public TCampaignBonus {
public:
    virtual const char* getIconDefName() const;
    virtual int getIconIndex() const;
    virtual std::string getText() const;
    virtual void apply(int whichPlayer) const;
    virtual void read(TAbstractFile* file);

    int m_resource;
    int m_amount;
};

// The start-bonus type byte the list reader at 0x485190 switches on, in
// the jump table's own order - which is the roster order the eight
// vftables 0x63daa0..0x63d9c0 already carry. Names are role inventions.
enum ECampaignBonusType {
    CAMPAIGN_BONUS_SPELL = 0,
    CAMPAIGN_BONUS_CREATURE = 1,
    CAMPAIGN_BONUS_BUILDING = 2,
    CAMPAIGN_BONUS_ARTIFACT = 3,
    CAMPAIGN_BONUS_SPELL_SCROLL = 4,
    CAMPAIGN_BONUS_PRIMARY_SKILL = 5,
    CAMPAIGN_BONUS_SECONDARY_SKILL = 6,
    CAMPAIGN_BONUS_RESOURCE = 7
};

class TCampaignStartOption {
public:
    // UpdateBonusIcons centres the frames when there are two choices.
    enum EChoiceCount {
        CHOICE_COUNT_PAIR = 2
    };
    // 0x484f40, and it is the DESTRUCTOR, not a constructor: the body is
    // one vptr store with no `mov eax,ecx`, which no VC6 constructor emits.
    // Defined out of line in the .cpp so the plain body is emitted at all;
    // 0x484f50, the root's `??_G`, then inlines it, as does every derived
    // destructor.
    virtual ~TCampaignStartOption();
    virtual bool isBuildingBonus(int which) const = 0;
    virtual int getCount() const = 0;
    virtual const char* getIconDefName(void* scenario, int which) const = 0;
    virtual int getIconIndex(int which) const = 0;
    // 0x484f80, inherited by the bonus and the third option: sums the
    // 5-dword bit block through the nibble table at 0x67729c and answers
    // the campaign's crossover index.
    virtual int slot5(void* scenario, int which) const;
    virtual std::string getText(void* scenario, int which) const = 0;
    virtual int slot7(int which) const;
    virtual int getPlayer(int which) const = 0;
    virtual void read(TAbstractFile* file) = 0;
    // `ret 4`: the slot takes one argument this option never reads, and
    // both sibling options answer it with the shared do-nothing at
    // 0x485d80.
    virtual void apply(void* scenario) = 0;
    virtual void setTown(CMapHeaderData* header) = 0;
    // 0x485000: every prerequisite scenario the record marks must already
    // be completed in gpGame->campaign.mapScores.
    virtual bool slot12(void* scenario, int value) const;
};

// Vftable 0x63d98c, 0x18 bytes: the player at +4 and the bonus list at +8.
class TCampaignStartBonusOption : public TCampaignStartOption {
public:
    virtual ~TCampaignStartBonusOption();
    virtual bool isBuildingBonus(int which) const;
    virtual int getCount() const;
    virtual const char* getIconDefName(void* scenario, int which) const;
    virtual int getIconIndex(int which) const;
    virtual std::string getText(void* scenario, int which) const;
    virtual int getPlayer(int which) const;
    virtual void read(TAbstractFile* file);
    virtual void apply(void* scenario);
    virtual void setTown(CMapHeaderData* header);

    int m_player;
    std::vector<TCampaignBonus*> m_bonuses;
};
SIZE(TCampaignStartBonusOption, 0x18);

// The scenario's OTHER two starting-options records, both 0x14 bytes and
// both one std::vector at +4 behind the shared 13-slot base. The type byte
// ScenarioStruct::Read switches on picks between the three: 1 is the bonus
// list above, 2 the crossover-hero choices and 3 the starting-hero choices.
// Names are role inventions. Dreamcast's older customcampaign.obj has no
// counterpart of either starting-option class; their roles are proven by
// the icon getters: the crossover option's icon is the large portrait of
// the first hero in gpGame->campaign.carryOverHeroes[mapScores[s].index]
// (0x4854c0) and the starting-hero option's is akHeroTraits[hero]'s own
// (0x485a60). Both answer slot 1 with the shared `mov al,1; ret 4` at
// 0x485a30, slot 4 with the program-wide `xor eax,eax; ret 4` at 0x4ec560
// and slots 10/11 with the `ret 4` at 0x485d80 - three /OPT:ICF folds, so
// only one copy of each is claimed anywhere in the tree.

// One crossover choice: the player position the carry-over pool is handed
// to, and the scenario whose mapScores row names that pool. Both are one
// byte in the file and every consumer sign-extends them (`movsx`).
struct TCampaignCrossoverChoice {
    signed char m_player;
    signed char m_scenario;
};

// Vftable 0x63dad8. Its implicit constructor is inlined at ScenarioStruct::
// Read's `new` site, so no declarator is needed here.
class TCampaignStartCrossoverOption : public TCampaignStartOption {
public:
    virtual bool isBuildingBonus(int which) const;
    virtual int getCount() const;
    virtual const char* getIconDefName(void* campaign, int which) const;
    virtual int getIconIndex(int which) const { return 0; }
    virtual int slot5(void* scenario, int which) const;
    virtual std::string getText(void* campaign, int which) const;
    virtual int getPlayer(int which) const;
    virtual void read(TAbstractFile* file);
    virtual void apply(void* scenario) {}
    virtual void setTown(CMapHeaderData* header) {}
    virtual bool slot12(void* scenario, int value) const;

    std::vector<TCampaignCrossoverChoice> m_choices;
};
SIZE(TCampaignStartCrossoverOption, 0x14);

// One starting-hero choice: the player position and the hero id, both read
// as a signed byte and a signed word but held as ints - GetPlayer reads the
// element at stride 8 and slot 7 the dword behind it.
struct TCampaignHeroChoice {
    int m_player;
    int m_hero;
};

// Vftable 0x63db0c. ScenarioStruct::Read calls the retained default
// constructor at 0x4883d0. This Complete-only class has no CodeView source
// counterpart; the written constructor below preserves that call boundary.
// The implicit-form probe in customcampaign.cpp explains its current spelling.
// This class overrides slot 7 and inherits slots 5 and 12 unchanged.
class TCampaignStartHeroOption : public TCampaignStartOption {
public:
    TCampaignStartHeroOption();
    virtual bool isBuildingBonus(int which) const;
    virtual int getCount() const;
    virtual const char* getIconDefName(void* campaign, int which) const;
    virtual int getIconIndex(int which) const { return 0; }
    virtual std::string getText(void* campaign, int which) const;
    virtual int slot7(int which) const;
    virtual int getPlayer(int which) const;
    virtual void read(TAbstractFile* file);
    virtual void apply(void* scenario) {}
    virtual void setTown(CMapHeaderData* header) {}

    std::vector<TCampaignHeroChoice> m_choices;
};
SIZE(TCampaignStartHeroOption, 0x14);

// The starting-options type byte ScenarioStruct::Read switches on, in its
// own `dec/je` chain order. Zero (and anything past three) leaves the
// scenario without a record at all. Names are role inventions.
enum ECampaignStartOptionType {
    CAMPAIGN_START_OPTION_NONE = 0,
    CAMPAIGN_START_OPTION_BONUS = 1,
    CAMPAIGN_START_OPTION_CROSSOVER = 2,
    CAMPAIGN_START_OPTION_HERO = 3
};

// The building bonus's two per-town tables, both indexed with the town
// as the outer row: 0x6755b8 gives the icon .def name for each of a
// town's 44 building slots and 0x6888c0 remaps a bonus's building index
// when the town is set (41 rows a town). Neither table is claimed yet, so
// the outer bound is left open rather than invented.
extern const char* g_campaignBuildingIconNames[][44];
extern const int g_eventBuildingIds[][41];

// The two mixed resource selectors a resource bonus can carry beside the
// seven EGameResource rows, byte-read off the ten-entry jump tables the
// description (0x484d90) and the applier (0x484e20) share: -3 pays wood
// AND ore, -2 pays all four rare resources. Names are role inventions.
enum ECampaignBonusResource {
    CAMPAIGN_BONUS_RESOURCE_WOOD_AND_ORE = -3,
    CAMPAIGN_BONUS_RESOURCE_RARE = -2,
    CAMPAIGN_BONUS_RESOURCE_NONE = -1
};

// The seven localized resource names, as advmgr.h / ai_player.h /
// newgame.h / tradpost.cpp already declare them; the resource
// bonus's description indexes the same table and this is the cheaper
// include-set edge.
extern const char* g_resourceNames[8];

// The three sentinel hero selectors a campaign bonus can carry, byte-read
// off the picker's own jump chain at 0x4840d0 (`cmp ecx,-3 / -2 / -1`
// with the plain-id arm falling through). Names are role inventions.
enum ECampaignBonusHero {
    CAMPAIGN_BONUS_HERO_STRONGEST = -3,
    CAMPAIGN_BONUS_HERO_FIRST = -2,
    CAMPAIGN_BONUS_HERO_NONE = -1
};

hero* getCampaignBonusHero(int heroSelector, int whichPlayer);

#endif  /* HOMM3_CUSTOMCAMPAIGN_H */
