// customcampaign.h - canonical campaign state and Complete campaign types.
// CodeView places SCampaign's constructor and completion query in this header.
#ifndef HOMM3_CUSTOMCAMPAIGN_H
#define HOMM3_CUSTOMCAMPAIGN_H

#include <string>
#include <vector>
#include <string.h>
#include "hero.h"
#include "campaignmusic.h"

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
    // Retail campaignbrief.obj retains this by-value accessor at 0x45a3e0;
    // its body copies the string at +0x14 into the hidden return object.
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

    // Retail's copy assignment and destructor are COMPILER-GENERATED, and
    // the image proves it from both sides of the /Ob2 split:
    // TCampaignWindow's constructor expands both inline, member by member -
    // that expansion is the whole evidence for the four sub-objects above -
    // while game::Load calls the out-of-line COMDATs (operator= 0x4bdc70,
    // destructor 0x45f110) for the same two operations.
    //
    // MEASURED, 2026-08-14: spelling them implicit here is the faithful
    // model and it takes TCampaignWindow's constructor 82.54% -> 91.17%,
    // but our game::Load is only half reconstructed, so its /Ob2 budget
    // does not starve where retail's does and it expands both inline:
    // game::Load 50.46% -> 43.93% (implicit destructor alone costs 2.17 of
    // that) and game::Save 27.94% -> 17.31%. Net negative overall and a
    // ratchet drop, so both stay declared until game::Load carries retail's
    // caller mass; re-take this the moment it does. Declaring
    // ~SavedGameHeader to hold the same line is NOT the way - measured at
    // the same time, it collapses game::Load to 9.88%. RE-TAKEN with the
    // tail landed: 74.6385 -> 58.8959 on Load and 79.1973 -> 55.2266 on
    // Save, game.obj 83.6719 -> 80.1106. Much less catastrophic than
    // 9.88% and still firmly negative, so the conclusion is unchanged
    // and the reason is now visible - retail expands that destructor on
    // ONE exit and calls 0x4bdf80 on the other, which a declarator
    // cannot express either way round.
    // RE-TAKEN 2026-08-20, with game::Load's tail landed and the function
    // at 74.6385% instead of the 50.46% above: STILL net negative, and by
    // a wider margin than the caller-mass argument predicted. Implicit
    // costs game::Load 74.6385 -> 58.5009 and game::Save 79.1973 ->
    // 61.0073, i.e. game.obj 83.6719 -> 80.5175, against campaignwindow
    // at 89.7220. Caller mass was NOT the whole story - both bodies still
    // expand the pair where retail calls the COMDATs.
    // LANDED 2026-08-21 through the narrow campaignwindow layout below. A global
    // implicit re-test at the current baselines still costs game::Load
    // 92.3721 -> 66.6252 and Save 93.5676 -> 78.2887, but campaignwindow.obj
    // alone needs the compiler-generated members and rises 82.5365 ->
    // 91.1679. The completed narrow view also models the nested vector split
    // and implicit padding above, taking that constructor to 98.4726 with an
    // exact 39/39 call ledger and flow-distance zero.
    // PROMOTED 2026-08-31: exact BackupGameHeaders independently proves the
    // copy assignment is compiler-generated. Its global implicit state is now
    // authoritative; the older including-TU dips above remain useful history
    // banked by max/hist, not a reason to retain a source-false declaration.
    // The narrow campaignwindow layout view that once carried that TU's
    // concrete nested-vector/destructor split is RETIRED (2026-09-05): the
    // canonical std::vector members below reproduce the same call ledger
    // and the implicit ~SCampaign COMDAT is emitted here again.
    // Restoring these canonical header bodies preserves all banked RVAs but
    // changes their VC6 emission/expansion decisions. The constructor's
    // standalone comparison is missing (0%, MAX 100); TCampaignWindow's ctor
    // moves 97.27 -> 84.30, game's ctor 87.85 -> 77.16, SavedGameHeader's ctor
    // 98.88 -> 66.98, and oldmain 78.94 -> 77.03. The changed include closure
    // also removes kb's retained CSprite::Draw occurrence (MAX 100). No dummy
    // emitter or private alternate body is introduced; all peaks remain banked.
    // Original: SCampaign::SCampaign; CustomCampaign.h:199, dc 0xbcd90.
    VA(0x00489500, 0x88)  // TCampaignWindow ctor callee + member stores, dc 0xbcd90
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
    // The destructor is compiler-generated. Retail expands it member by
    // member in ~SavedGameHeader and retains the same COMDAT for callers.
    // Copy assignment is compiler-generated; its retained game.obj COMDAT is
    // claimed with VA_COMPGEN beside the game::Load reconstruction.
    // Retail 0x489590, thiscall on gpGame->campaign with the selected
    // campaign's ordinal and its data-file name; CampaignWindowHandler's
    // deselect arm is the caller that proves the shape. Provisional name.
    void selectCampaign(int campaignIndex, const char* filename);
    // Complete's campaign-brief handler supplies an opaque campaign-header
    // record here.  The pointee is nested in TCampaignBrief, which is not
    // nameable before the campaign-brief declarations; the receiver,
    // one-pointer ABI and prologue-video role are retail-byte proven.
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
    void playScenarioEpilogue(void* campaignHeader);
    // Retail 0x48b2e0 banks the selected starting option and applies the
    // campaign-specific setup overrides before the scenario stream is read.
    void applyBriefingChoice(int option);
    // DC CustomCampaign.h:212 (dc 0xe6ef8); retail 0x4897d0 in
    // customcampaign.obj.
    // Original: SCampaign::CampaignComplete; CustomCampaign.h:212, dc 0xe6ef8.
    VA(0x004897d0, 0x43)  // mapScores walk on the 0x14 stride, dc 0xe6ef8
    unsigned char campaignComplete()
    {
        for (unsigned int i = 0; i < m_mapScores.size(); ++i) {
            if (!m_mapScores[i].m_completed)
                return 0;
        }
        return 1;
    }
    int getScore() const;
    int getTotalTime() const;
    // Retail 0x486440 is the 325-byte counterpart of the DC 328-byte
    // CustomCampaign.cpp row and is called on gpGame->campaign here.
    void doPreLoadCustomization();
    // Provisional name; PlaceCrossoverHeroes retains this lookup's nested
    // vector::size calls while expanding the ordinary member itself.
    hero* findCrossoverHero(int heroId);
    void save(TAbstractFile* outfile);
    // Retail-only load surface at 0x48a310; SavedGameHeader::Load passes the
    // stream and save version and the callee reads both.
    void load(TAbstractFile* infile, int saveVersion);
    // Complete-only header accessor selected into singleselectionwindow.obj
    // at 0x57c780. Retail sign-extends currentMap, indexes the +0x5c vector's
    // first pointer with the 0x14 CampaignScenarioInfo stride, and returns it.
    VA(0x0057C780, 0x0E)  // hd-crossbuild masked identity + sole retail caller
    CampaignScenarioInfo* getCurrentScenario()
    {
        return &m_mapScores[m_currentMap];
    }
};
SIZE(SCampaign, 0x7c);

// Retail's reference cell at 0x66c218 contains 0x66c090, the same
// SCampaignMusicCue table populated by initializeCampaignMusicTable.
// The former TCampaignMusicTraits::field_04 was its track pointer, not an
// integer: the loader at 0x45e250 stores pooled CmpMusic.txt strings there.
// Keep the canonical record from campaignmusic.h for both readers/writer.
// Before normalization: akCampaignMusicTraits.
extern const SCampaignMusicCue* g_campaignMusicTraits;

// The ordering both of SCampaign::PruneCrossoverHeroes' std::sort calls
// instantiate: strongest crossover hero first, by primary skills plus the 28
// secondary mastery bytes, then by experience, then by hero id. Retail emits
// the operator() (0x483f80) as a plain customcampaign.obj body immediately
// behind the two TStreamBufFile virtuals, and `this` is dead in it - the
// functor is empty. The class name is a ROLE invention; no Dreamcast row
// survives for it.
class hero;
struct CrossoverHeroStronger {
    bool operator()(hero& lhs, hero& rhs) const;
};

// The map's own hero placeholders are sorted by their power rating before
// the campaign hands out its carried heroes: the strongest placeholder gets
// the first carried hero. Retail instantiates std::sort over it in
// customcampaign.obj (0x48eec0 and its helpers). The rating is compared
// SIGNED. Role name; no Dreamcast row covers it.
struct HeroPlaceholderData;
struct HeroPlaceholderStronger {
    bool operator()(const HeroPlaceholderData& left,
                    const HeroPlaceholderData& right) const;
};

// The crossover-hero score the ordering above compares: the primary-skill
// total plus the 28 secondary mastery bytes. Retail keeps it as a separate
// /Gr free body at 0x483f50 (the hero arrives in ECX and it returns with a
// bare `ret`), which the sort's own helpers CALL while the standalone
// operator() expands it twice. Retail-only, name provisional - no Dreamcast
// row covers it.
// Before normalization (function): GetCrossoverHeroValue.
int getCrossoverHeroValue(hero* candidate);

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
    // 0x484d50. Pure in retail's own vftable, which means each concrete
    // class carried its own copy and the linker folded the seven `false`
    // bodies onto one address; one out-of-line definition here produces
    // that one address.
    // Before normalization (function): TCampaignBonus::IsBuildingBonus.
    virtual bool isBuildingBonus() const;
    // Before normalization (function): TCampaignBonus::GetIconDefName.
    virtual const char* getIconDefName() const = 0;
    // Before normalization (function): TCampaignBonus::GetIconIndex.
    virtual int getIconIndex() const = 0;
    // Before normalization (function): TCampaignBonus::GetText.
    virtual std::string getText() const = 0;
    // Before normalization (function): TCampaignBonus::Apply.
    virtual void apply(int whichPlayer) const = 0;
    // Before normalization (function): TCampaignBonus::Read.
    virtual void read(TAbstractFile* file) = 0;
    // 0x485d80, `ret 4`. Only the building bonus overrides it (0x4847e0),
    // where the town remaps the building index.
    // Before normalization (function): TCampaignBonus::SetTown.
    virtual void setTown(int town);
};

// Spell: the hero id it is granted to and the spell. Read takes a SIGNED
// word then an unsigned byte (0x484050).
class TCampaignSpellBonus : public TCampaignBonus {
public:
    // Before normalization (function): TCampaignSpellBonus::GetIconDefName.
    virtual const char* getIconDefName() const;
    // Before normalization (function): TCampaignSpellBonus::GetIconIndex.
    virtual int getIconIndex() const { return m_spell; }
    // Before normalization (function): TCampaignSpellBonus::GetText.
    virtual std::string getText() const;
    // Before normalization (function): TCampaignSpellBonus::Apply.
    virtual void apply(int whichPlayer) const;
    // Before normalization (function): TCampaignSpellBonus::Read.
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
    // Before normalization (function): TCampaignSpellScrollBonus::GetText.
    virtual std::string getText() const;
    // Before normalization (function): TCampaignSpellScrollBonus::Apply.
    virtual void apply(int whichPlayer) const;
};

// Creature: hero, creature type and count, all three read as words
// (0x4844f0) - the first two signed, the count unsigned.
class TCampaignCreatureBonus : public TCampaignBonus {
public:
    // Before normalization (function): TCampaignCreatureBonus::GetIconDefName.
    virtual const char* getIconDefName() const;
    // Before normalization (function): TCampaignCreatureBonus::GetIconIndex.
    virtual int getIconIndex() const;
    // Before normalization (function): TCampaignCreatureBonus::GetText.
    virtual std::string getText() const;
    // Before normalization (function): TCampaignCreatureBonus::Apply.
    virtual void apply(int whichPlayer) const;
    // Before normalization (function): TCampaignCreatureBonus::Read.
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
    // Before normalization (function): TCampaignBuildingBonus::IsBuildingBonus.
    virtual bool isBuildingBonus() const;
    // Before normalization (function): TCampaignBuildingBonus::GetIconDefName.
    virtual const char* getIconDefName() const;
    // Before normalization (function): TCampaignBuildingBonus::GetIconIndex.
    virtual int getIconIndex() const { return 0; }
    // Before normalization (function): TCampaignBuildingBonus::GetText.
    virtual std::string getText() const;
    // Before normalization (function): TCampaignBuildingBonus::Apply.
    virtual void apply(int whichPlayer) const;
    // Before normalization (function): TCampaignBuildingBonus::Read.
    virtual void read(TAbstractFile* file);
    // Before normalization (function): TCampaignBuildingBonus::SetTown.
    virtual void setTown(int town);

    int m_town;
    int m_building;
};

// Artifact: hero and artifact, both signed words (0x4848a0).
class TCampaignArtifactBonus : public TCampaignBonus {
public:
    // Before normalization (function): TCampaignArtifactBonus::GetIconDefName.
    virtual const char* getIconDefName() const;
    // Before normalization (function): TCampaignArtifactBonus::GetIconIndex.
    virtual int getIconIndex() const { return m_artifact; }
    // Before normalization (function): TCampaignArtifactBonus::GetText.
    virtual std::string getText() const;
    // Before normalization (function): TCampaignArtifactBonus::Apply.
    virtual void apply(int whichPlayer) const;
    // Before normalization (function): TCampaignArtifactBonus::Read.
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
    // Before normalization (function): TCampaignPrimarySkillBonus::GetIconDefName.
    virtual const char* getIconDefName() const;
    // Before normalization (function): TCampaignPrimarySkillBonus::GetIconIndex.
    virtual int getIconIndex() const;
    // Before normalization (function): TCampaignPrimarySkillBonus::GetText.
    virtual std::string getText() const;
    // Before normalization (function): TCampaignPrimarySkillBonus::Apply.
    virtual void apply(int whichPlayer) const;
    // Before normalization (function): TCampaignPrimarySkillBonus::Read.
    virtual void read(TAbstractFile* file);

    int m_hero;
    char m_skills[4];
};

// Secondary skill: a signed word hero id, then the skill and its mastery
// as unsigned bytes (0x484cf0).
class TCampaignSecondarySkillBonus : public TCampaignBonus {
public:
    // Before normalization (function): TCampaignSecondarySkillBonus::GetIconDefName.
    virtual const char* getIconDefName() const;
    // Before normalization (function): TCampaignSecondarySkillBonus::GetIconIndex.
    virtual int getIconIndex() const;
    // Before normalization (function): TCampaignSecondarySkillBonus::GetText.
    virtual std::string getText() const;
    // Before normalization (function): TCampaignSecondarySkillBonus::Apply.
    virtual void apply(int whichPlayer) const;
    // Before normalization (function): TCampaignSecondarySkillBonus::Read.
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
    // Before normalization (function): TCampaignResourceBonus::GetIconDefName.
    virtual const char* getIconDefName() const;
    // Before normalization (function): TCampaignResourceBonus::GetIconIndex.
    virtual int getIconIndex() const;
    // Before normalization (function): TCampaignResourceBonus::GetText.
    virtual std::string getText() const;
    // Before normalization (function): TCampaignResourceBonus::Apply.
    virtual void apply(int whichPlayer) const;
    // Before normalization (function): TCampaignResourceBonus::Read.
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
    // 0x485090, `or eax,-1 / ret 4` - inherited unchanged by ALL THREE
    // concrete classes, so the root is where the -1 lives.
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
    // Before normalization (function): TCampaignStartBonusOption::IsBuildingBonus.
    virtual bool isBuildingBonus(int which) const;
    // Before normalization (function): TCampaignStartBonusOption::GetCount.
    virtual int getCount() const;
    // Before normalization (function): TCampaignStartBonusOption::GetIconDefName.
    virtual const char* getIconDefName(void* scenario, int which) const;
    // Before normalization (function): TCampaignStartBonusOption::GetIconIndex.
    virtual int getIconIndex(int which) const;
    // Before normalization (function): TCampaignStartBonusOption::GetText.
    virtual std::string getText(void* scenario, int which) const;
    // Before normalization (function): TCampaignStartBonusOption::GetPlayer.
    virtual int getPlayer(int which) const;
    // Before normalization (function): TCampaignStartBonusOption::Read.
    virtual void read(TAbstractFile* file);
    // Before normalization (function): TCampaignStartBonusOption::Apply.
    virtual void apply(void* scenario);
    // Before normalization (function): TCampaignStartBonusOption::SetTown.
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
    // Before normalization: player.
    signed char m_player;
    // Before normalization: scenario.
    signed char m_scenario;
};

// Vftable 0x63dad8. Its implicit constructor is inlined at ScenarioStruct::
// Read's `new` site, so no declarator is needed here.
class TCampaignStartCrossoverOption : public TCampaignStartOption {
public:
    // Before normalization (function): TCampaignStartCrossoverOption::IsBuildingBonus.
    virtual bool isBuildingBonus(int which) const;
    // Before normalization (function): TCampaignStartCrossoverOption::GetCount.
    virtual int getCount() const;
    // Before normalization (function): TCampaignStartCrossoverOption::GetIconDefName.
    virtual const char* getIconDefName(void* campaign, int which) const;
    // Before normalization (function): TCampaignStartCrossoverOption::GetIconIndex.
    virtual int getIconIndex(int which) const { return 0; }
    // Before normalization (function): TCampaignStartCrossoverOption::_slot5.
    virtual int slot5(void* scenario, int which) const;
    // Before normalization (function): TCampaignStartCrossoverOption::GetText.
    virtual std::string getText(void* campaign, int which) const;
    // Before normalization (function): TCampaignStartCrossoverOption::GetPlayer.
    virtual int getPlayer(int which) const;
    // Before normalization (function): TCampaignStartCrossoverOption::Read.
    virtual void read(TAbstractFile* file);
    // Before normalization (function): TCampaignStartCrossoverOption::Apply.
    virtual void apply(void* scenario) {}
    // Before normalization (function): TCampaignStartCrossoverOption::SetTown.
    virtual void setTown(CMapHeaderData* header) {}
    // Before normalization (function): TCampaignStartCrossoverOption::_slot12.
    virtual bool slot12(void* scenario, int value) const;

    std::vector<TCampaignCrossoverChoice> m_choices;
};
SIZE(TCampaignStartCrossoverOption, 0x14);

// One starting-hero choice: the player position and the hero id, both read
// as a signed byte and a signed word but held as ints - GetPlayer reads the
// element at stride 8 and slot 7 the dword behind it.
struct TCampaignHeroChoice {
    // Before normalization: player.
    int m_player;
    // Before normalization: hero.
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
    // Before normalization (function): TCampaignStartHeroOption::IsBuildingBonus.
    virtual bool isBuildingBonus(int which) const;
    // Before normalization (function): TCampaignStartHeroOption::GetCount.
    virtual int getCount() const;
    // Before normalization (function): TCampaignStartHeroOption::GetIconDefName.
    virtual const char* getIconDefName(void* campaign, int which) const;
    // Before normalization (function): TCampaignStartHeroOption::GetIconIndex.
    virtual int getIconIndex(int which) const { return 0; }
    // Before normalization (function): TCampaignStartHeroOption::GetText.
    virtual std::string getText(void* campaign, int which) const;
    // Before normalization (function): TCampaignStartHeroOption::_slot7.
    virtual int slot7(int which) const;
    // Before normalization (function): TCampaignStartHeroOption::GetPlayer.
    virtual int getPlayer(int which) const;
    // Before normalization (function): TCampaignStartHeroOption::Read.
    virtual void read(TAbstractFile* file);
    // Before normalization (function): TCampaignStartHeroOption::Apply.
    virtual void apply(void* scenario) {}
    // Before normalization (function): TCampaignStartHeroOption::SetTown.
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
// Before normalization: gCampaignBuildingIconNames.
extern const char* g_campaignBuildingIconNames[][44];
// Before normalization: gCampaignBuildingRemap.
extern const int g_campaignBuildingRemap[][41];

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
// newgame.h / tradpost_widgets.h already declare them; the resource
// bonus's description indexes the same table and this is the cheaper
// include-set edge.
extern const char* g_resourceNames[7];

// The three sentinel hero selectors a campaign bonus can carry, byte-read
// off the picker's own jump chain at 0x4840d0 (`cmp ecx,-3 / -2 / -1`
// with the plain-id arm falling through). Names are role inventions.
enum ECampaignBonusHero {
    CAMPAIGN_BONUS_HERO_STRONGEST = -3,
    CAMPAIGN_BONUS_HERO_FIRST = -2,
    CAMPAIGN_BONUS_HERO_NONE = -1
};

// The bonus applier's shared hero picker (0x4840d0), a /Gr free function
// taking the selector in ECX and the player in EDX. Three selectors are
// sentinels - -3 is "the player's strongest hero" by primary-skill total
// plus secondary-skill levels, -2 is "the player's first hero" and -1 is
// none - and any other value is a hero id that only answers when the hero
// already belongs to that player. Retail-only, name provisional.
// Before normalization (function): GetCampaignBonusHero.
hero* getCampaignBonusHero(int heroSelector, int whichPlayer);

// --- globals ---
// CODEVIEW(E:\gamedcs\customcampaign.cpp:70, dc 0x7cd4c) void InitCampaignMapTraits([]* map_traits);

// --- SCampaign ---
// CODEVIEW(E:\gamedcs\customcampaign.cpp:112, dc 0x7d14c) void SCampaign::clear();
// CODEVIEW(E:\gamedcs\customcampaign.cpp:140, dc 0x7d1ec) void SCampaign::clear_carryover_pool(TCarryOverPoolNumber pool_num);
// CODEVIEW(E:\gamedcs\customcampaign.cpp:147, dc 0x7d22c) void SCampaign::DoPreLoadCustomization();
// CODEVIEW(E:\gamedcs\customcampaign.cpp:198, dc 0x7d374) int SCampaign::get_score();
// CODEVIEW(E:\gamedcs\customcampaign.cpp:209, dc 0x7d3c0) int SCampaign::get_total_time();
// CODEVIEW(E:\gamedcs\customcampaign.cpp:222, dc 0x7d420) void SCampaign::give_custom_items();

// --- TArtifactRequirement ---
// CODEVIEW(E:\gamedcs\CustomCampaign.h:108, dc 0x7ea10) void TArtifactRequirement::set(TArtifact _artifact, char _guard_bit);

// --- TCustomCampaignTraits ---
// CODEVIEW(E:\gamedcs\customcampaign.cpp:29, dc 0x7cc8c) void TCustomCampaignTraits::init();
// CODEVIEW(E:\gamedcs\customcampaign.cpp:55, dc 0x7cccc) void TCustomCampaignTraits::set(int _exp_cap, char _num_incoming_heroes, char _num_outgoing_heroes, TCarryOverPoolNumber _incoming_hero_pool, TCarryOverPoolNumber _outgoing_hero_pool, TArtifact _art_req_1, char _guard_bit_1, TArtifact _art_req_2, char _guard_bit_2, char _pos1, char _pos2, char _difficulty);

// --- type_artifact ---
// CODEVIEW(E:\gamedcs\hero.h:214, dc 0x7ea04) void type_artifact::type_artifact(SpellID new_spell);

#endif  /* HOMM3_CUSTOMCAMPAIGN_H */
