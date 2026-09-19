// campaignbrief.h - prototypes of campaignbrief.cpp (compiland campaignbrief.obj)
#ifndef HOMM3_CAMPAIGNBRIEF_H
#define HOMM3_CAMPAIGNBRIEF_H

#include <bitset>
#include <string>
#include <vector>
#include "game.h"
#include "window.h"

// Shared saved game snapshot; original Dreamcast name: saveHeader.
// campaignbrief.cpp owns retail 0x69fdc4.
extern game* g_saveHeader;

class bitmapBorder;
class button;
class coloredBorderFrame;
class iconWidget;
class type_func_button;
class type_text_scroller;

// Retail's vector insert and constructor cleanup both prove this exact
// source-level aggregate: a NewSMapHeader, the trivially copied setup record,
// and one availability byte at +0x4d0.
// The four map extents NewSMapHeader::Size takes; TCampaignBrief::Select
// maps them onto the WHICHMAP icon's frames 0..3 (anything else is frame
// 4). Declared with its one consumer.
enum EMapSize {
    MAP_SIZE_SMALL = 36,
    MAP_SIZE_MEDIUM = 72,
    MAP_SIZE_LARGE = 108,
    MAP_SIZE_EXTRA_LARGE = 144
};

struct CampaignScenarioPreview : public NewSMapHeader {
    SGameSetupOptions m_gameSetup;
    bool m_available;
};
SIZE(CampaignScenarioPreview, 0x4d4);

// The scenario's "starting options" chooser, and it is a HIERARCHY: three
// concrete 13-slot vftables (0x63d98c, 0x63dad8, 0x63db0c) sit under an
// abstract root at 0x63d958 whose six unimplemented slots are __purecall
// and whose slots 5, 7 and 12 carry real shared bodies (0x484f80,
// 0x485090, 0x485000). The SLOT ROLES read straight off 0x63d98c, the
// start-BONUS option, whose every override is a one-line forward to one
// element of a std::vector<TCampaignBonus*> at this+8:
//   slot 1 -> the element's building predicate (TCampaignBonus+0x04)
//   slot 2 -> the element COUNT, `(_Last - _First) / 4`
//   slot 3 -> the element's icon .def name (TCampaignBonus+0x08)
//   slot 4 -> the element's icon frame  (TCampaignBonus+0x0c)
//   slot 6 -> the element's description (TCampaignBonus+0x10)
//   slot 9 -> the list reader (the type byte 0..7 switch)
//   slot 10 -> Apply on the element campaign.briefingChoice selects
//   slot 11 -> SetTown from the map header's own main-town type
// and the player the whole list belongs to is the dword at +4, byte-proven
// three ways: the reader stores the FIRST byte it reads there, slot 10
// hands it to TCampaignBonus::Apply as `whichPlayer`, and slot 11 indexes
// `header->playerSlotAttributes[+4]` with the 0x44 stride.
// NAMES ARE ROLE INVENTIONS: customcampaign.obj has no Dreamcast twin and
// no RTTI descriptor names any of these classes. Slots 5, 7 and 12 keep
// ordinal-derived names because only their bodies, not their callers, are
// decoded; their first parameter is the same opaque per-scenario record in
// all three (it carries a byte vector at +0x18, an int row at +0x4c, a
// vector at +0x70 and a five-dword bit block at +0x90).
class TCampaignStartOption;

// Retail Complete diverges from the Dreamcast class after heroWindow, but
// fixes every field used by the campaign constructor and destructor.
class TCampaignBrief : public heroWindow {
public:
    struct ScenarioStruct;
    struct CampaignHeaderStruct;

    struct MapTextStruct {
        int m_video;
        int m_audio;
        std::string m_subtitles;

        // Retail 0x488fb0, the thiscall SCampaign::PlayScenarioPrologue
        // makes on a scenario's prologue record (name provisional).
        void play();
    };

    // The empty NewMapCampaignContext base is how game::NewMap receives the
    // selected scenario: StartScenario (0x4884c0) passes `this` in that
    // slot and NewMap calls two customcampaign.obj bodies on it. game.h
    // cannot name a nested type, so the base carries the relationship;
    // being empty it leaves every proven offset in place.
    struct ScenarioStruct : public NewMapCampaignContext {
        std::string m_name;
        int m_offset;
        // Retail tests this field with a signed `jle` before loading a
        // scenario.  The width agrees with the cross-build record, but the
        // Complete codegen proves the signed PC spelling.
        int m_inflatedSize;
        // Byte elements: GetAvailableScenarios (0x488f00) walks _First at
        // +0x1c with `cmp byte ptr [ebx+edx],0` on a unit stride, and the
        // prologue pointer follows at +0x3c (a Dinkumware vector<bool>
        // would push it to +0x40).
        std::vector<unsigned char> m_prerequisites;
        std::string m_regionDesc;
        unsigned char m_regionColor;
        signed char m_difficulty;
        // The loader reads regionColor/difficulty as bytes at +0x38/39;
        // prologue starts at +0x3c. These two bytes align the pointer.
        char m_paddingBeforePrologue[2];
        MapTextStruct* m_prologue;
        MapTextStruct* m_epilogue;
        bool m_retainXp;
        bool m_retainPskills;
        bool m_retainSskills;
        bool m_retainSpellbook;
        bool m_retainArtifacts;
        // The loader expands five retention bits into booleans at +0x44..48;
        // heroesStatus starts at +0x4c. These three bytes align the integer array.
        char m_paddingBeforeHeroesStatus[3];
        int m_heroesStatus[8];
        std::vector<int> m_heroPlaceholders;
        std::bitset<145> m_crossoverCreatures;
        std::bitset<144> m_crossoverArtifacts;
        TCampaignStartOption* m_options;

        ScenarioStruct();
        // Retail 0x487e40 (`ret 0xc`): reads one scenario record out of
        // the campaign stream. The second argument is the region's own
        // scenario count (the prerequisite bitmap's width) and the third
        // the campaign file version. Name provisional - no Dreamcast row
        // covers this Complete-only type.
        void read(TAbstractFile* infile, int numScenarios,
                  int campaignVersion);
        // Complete's campaign-map loader calls this on the selected
        // scenario record for each matching map hero placeholder.  The
        // receiver offsets prove this is ScenarioStruct itself; both
        // arguments are fixed by the 0x10-byte placeholder stride and the
        // 0x492-byte carry-over hero vector stride.
        void initializeCrossoverHero(HeroPlaceholderData* placeholder,
                                     hero* sourceHero);
        // Retail 0x487020, the placeholder half of the same pass: a map
        // hero placeholder with no carried hero behind it becomes a live
        // hero of the player's own alignment (or the carried record is
        // re-homed first), is dropped on the object's trigger cell and
        // registered with its player, the availability table and the
        // fog. `this` is dead in the body - the receiver is fixed by the
        // call site, not by the code. Name provisional.
        void placeStartingHero(HeroPlaceholderData* placeholder);
        // Retail 0x487290, game::NewMap's second campaign callee: the map's
        // hero placeholders are sorted by power rating and handed the
        // scenario's carried heroes, strongest first. Name provisional.
        void placeCrossoverHeroes();
        // Retail 0x487900, game::NewMap's third campaign callee: every
        // artifact the scenario's carry-over pool still holds is offered to
        // the option's player, and the option's own Apply runs last. Name
        // provisional.
        void giveCrossoverArtifacts();
        // Complete-only retained wrapper at 0x4884c0.  The campaign-header
        // wrapper below is its sole direct caller.
        void startScenario(std::streambuf* stream, int option);
        std::string getRegionDescription() const;
        std::string getBonusText(CampaignHeaderStruct* campaign, int option);
        ~ScenarioStruct();
        void loadMapHeader(std::streambuf* stream, NewSMapHeader* mapHeader,
                           int which);
        void markCrossoverHeroes(unsigned char* wanted);
    };

    struct CampaignHeaderStruct {
        // Complete's Load body sets OPEN_FAILED when its reader factory
        // returns null and VERSION_UNSUPPORTED when campaign_version < 4.
        enum EFileError {
            CAMPAIGN_FILE_OK = 0,
            CAMPAIGN_FILE_OPEN_FAILED = 1,
            CAMPAIGN_FILE_VERSION_UNSUPPORTED = 2
        };

        EFileError m_fileError;
        std::string m_fileName;
        int m_campaignVersion;
        int m_regionMap;
        std::string m_campaignName;
        std::string m_campaignDesc;
        std::vector<ScenarioStruct*> m_scenarios;
        unsigned char* m_data;
        // FreeData (0x4887e0) destroys it through vtable slot 0 with the
        // deleting flag, and the two loaders hand it to ScenarioStruct.
        // A std::streambuf (a filebuf or a strstreambuf by Load's vftable
        // stores): the loaders seek it through slot 8 with seekoff's
        // hidden-return-plus-three ABI and wrap it in a TGzInflateBuf.
        std::streambuf* m_stream;
        bool m_variableDifficulty;
        // The loader stores variableDifficulty as a bool at +0x54 and
        // campaignMusic as an int at +0x58. These three bytes align the integer.
        char m_paddingBeforeCampaignMusic[3];
        int m_campaignMusic;

        CampaignHeaderStruct(const char* filename);
        ~CampaignHeaderStruct();
        bool load();
        bool loadScenario(int which, NewSMapHeader* mapHeader);
        std::string getCampaignName() const;
        std::string getCampaignDescription() const;
        std::string getFileName() const;
        void startMusic();
        void getAvailableScenarios(unsigned char* available) const;
        void startScenario(int which, int option);
        void freeData();
        int getNumMaps() const;
    };

    // Dreamcast's LF_FIELDLIST preserves this complete nested enum.  The
    // Complete constructor independently uses the same 100..235 ranges for
    // the background, campaign text, flags and three region-image states.
    // Keeping the names in the class also restores the real C1 declaration
    // environment instead of steering /Ob2 from a stripped-down surrogate.
    enum EOtherWidgetIDs {
        BACKGROUND_ID = 100,
        CAMPAIGN_NAME_ID,
        CAMPAIGN_DESCRIPTION_ID,
        MAP_NAME_ID,
        MAP_DESCRIPTION_ID,
        HERO_FACE_ID,
        HERO_LEFT_ID,
        HERO_RITE_ID,
        ARTIFACT_ID,
        GOLD_ID,
        RESOURCE_ID,
        RESTART_ID,
        VIDEO_ID,
        ALLY_FLAG1_ID,
        ALLY_FLAG2_ID,
        ALLY_FLAG3_ID,
        ALLY_FLAG4_ID,
        ALLY_FLAG5_ID,
        ALLY_FLAG6_ID,
        ALLY_FLAG7_ID,
        ALLY_FLAG8_ID,
        ENEMY_FLAG1_ID,
        ENEMY_FLAG2_ID,
        ENEMY_FLAG3_ID,
        ENEMY_FLAG4_ID,
        ENEMY_FLAG5_ID,
        ENEMY_FLAG6_ID,
        ENEMY_FLAG7_ID,
        ENEMY_FLAG8_ID,
        WHICHMAP_ID,
        MAP_CONQUERED_1_ID,
        MAP_CONQUERED_2_ID,
        MAP_CONQUERED_3_ID,
        MAP_CONQUERED_4_ID,
        MAP_CONQUERED_5_ID,
        MAP_CONQUERED_6_ID,
        MAP_CONQUERED_7_ID,
        MAP_CONQUERED_8_ID,
        MAP_CONQUERED_9_ID,
        MAP_CONQUERED_10_ID,
        MAP_CONQUERED_11_ID,
        MAP_CONQUERED_12_ID,
        MAP_CONQUERED_13_ID,
        MAP_CONQUERED_14_ID,
        MAP_CONQUERED_15_ID,
        MAP_CONQUERED_16_ID,
        MAP_CONQUERED_17_ID,
        MAP_CONQUERED_18_ID,
        MAP_CONQUERED_19_ID,
        MAP_CONQUERED_20_ID,
        MAP_CONQUERED_21_ID,
        MAP_CONQUERED_22_ID,
        MAP_CONQUERED_23_ID,
        MAP_CONQUERED_24_ID,
        MAP_CONQUERED_25_ID,
        MAP_CONQUERED_26_ID,
        MAP_CONQUERED_27_ID,
        MAP_CONQUERED_28_ID,
        MAP_CONQUERED_29_ID,
        MAP_CONQUERED_30_ID,
        MAP_CONQUERED_31_ID,
        MAP_CONQUERED_32_ID,
        MAP_ENABLED_1_ID,
        MAP_ENABLED_2_ID,
        MAP_ENABLED_3_ID,
        MAP_ENABLED_4_ID,
        MAP_ENABLED_5_ID,
        MAP_ENABLED_6_ID,
        MAP_ENABLED_7_ID,
        MAP_ENABLED_8_ID,
        MAP_ENABLED_9_ID,
        MAP_ENABLED_10_ID,
        MAP_ENABLED_11_ID,
        MAP_ENABLED_12_ID,
        MAP_ENABLED_13_ID,
        MAP_ENABLED_14_ID,
        MAP_ENABLED_15_ID,
        MAP_ENABLED_16_ID,
        MAP_ENABLED_17_ID,
        MAP_ENABLED_18_ID,
        MAP_ENABLED_19_ID,
        MAP_ENABLED_20_ID,
        MAP_ENABLED_21_ID,
        MAP_ENABLED_22_ID,
        MAP_ENABLED_23_ID,
        MAP_ENABLED_24_ID,
        MAP_ENABLED_25_ID,
        MAP_ENABLED_26_ID,
        MAP_ENABLED_27_ID,
        MAP_ENABLED_28_ID,
        MAP_ENABLED_29_ID,
        MAP_ENABLED_30_ID,
        MAP_ENABLED_31_ID,
        MAP_ENABLED_32_ID,
        MAP_SELECTED_1_ID,
        MAP_SELECTED_2_ID,
        MAP_SELECTED_3_ID,
        MAP_SELECTED_4_ID,
        MAP_SELECTED_5_ID,
        MAP_SELECTED_6_ID,
        MAP_SELECTED_7_ID,
        MAP_SELECTED_8_ID,
        MAP_SELECTED_9_ID,
        MAP_SELECTED_10_ID,
        MAP_SELECTED_11_ID,
        MAP_SELECTED_12_ID,
        MAP_SELECTED_13_ID,
        MAP_SELECTED_14_ID,
        MAP_SELECTED_15_ID,
        MAP_SELECTED_16_ID,
        MAP_SELECTED_17_ID,
        MAP_SELECTED_18_ID,
        MAP_SELECTED_19_ID,
        MAP_SELECTED_20_ID,
        MAP_SELECTED_21_ID,
        MAP_SELECTED_22_ID,
        MAP_SELECTED_23_ID,
        MAP_SELECTED_24_ID,
        MAP_SELECTED_25_ID,
        MAP_SELECTED_26_ID,
        MAP_SELECTED_27_ID,
        MAP_SELECTED_28_ID,
        MAP_SELECTED_29_ID,
        MAP_SELECTED_30_ID,
        MAP_SELECTED_31_ID,
        MAP_SELECTED_32_ID,
        CHOICE_1_ID,
        CHOICE_2_ID,
        CHOICE_3_ID,
        CHOICE_1_HIGHLIGHT_ID,
        CHOICE_2_HIGHLIGHT_ID,
        CHOICE_3_HIGHLIGHT_ID,
        BONUS_TEXT_ID,
        ROLLOVER_ID,
        CAMPAIGN_DESCRIPTION_BUTTON_ID,
        SCENARIO_DESCRIPTION_ID
    };
    enum { NWIDGETS = 80 };

    unsigned short* m_zBuffer;
    int m_oldVolume;
    std::vector<CampaignScenarioPreview> m_scenarios;
    CampaignHeaderStruct* m_campaign;
    int m_field68;
    int m_selectedScenario;
    coloredBorderFrame* m_startBonusBorders[3];
    bitmapBorder* m_bitmapBonusImages[3];
    iconWidget* m_spriteBonusImages[3];
    button* m_difficultyButtons[5];
    type_func_button* m_difficultyDecrButton;
    type_func_button* m_difficultyIncrButton;
    type_text_scroller* m_scroller;

    TCampaignBrief(unsigned char newCampaign, unsigned char viewFromGame);
    virtual ~TCampaignBrief();
    void addBonusIcons();
    void updateBonusIcons();
    void doModal();
    void select(int which);
    void clearSelected();
    void resetMapAndDescription(int which);
    void setHumanSlot();
    void setupCurrentTerritory();
    void updateAllyEnemyFlags();
    void updateDifficultyButtons();

private:
    // The Dreamcast procedure is S_LPROC32 (file-static) yet calls this
    // private helper, proving the owning header grants it friendship;
    // campaignbrief.cpp declares the static ahead of this header so the
    // friend binds to it.
    friend int campaignBriefHandler(message& msg);
    // The DC class type contains this private member in addition to the
    // same-named file-scope helper emitted by campaignbrief.obj.
    void showTerritorySmacker(unsigned char evilPost);
    int convertID2HelpID(int id) const;
};
SIZE(TCampaignBrief::MapTextStruct, 0x18);
SIZE(TCampaignBrief::ScenarioStruct, 0xa8);
SIZE(TCampaignBrief::CampaignHeaderStruct, 0x5c);
SIZE(TCampaignBrief, 0xb4);

// --- globals ---
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:202, dc 0x58244) void CampaignWait(int which);
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:365, dc 0x58774) void ShowTerritorySmacker(unsigned char bEvil2Post);
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:649, dc 0x59300) void ExtractCampaignMap(int* numPreReqs, unsigned char single_map_only, unsigned char write_file);
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:1076, dc 0x5a324) int CampaignBriefHandler(message* msg);
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:1409, dc 0x5ab84) void ReadRamDisc(int RamDiscNr, void* buffer, long size, unsigned long* bytesRead);

// --- CHeroDlg ---
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:1391, dc 0x5af4c) void CHeroDlg::~CHeroDlg();

// --- NewSMapHeader ---
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:192, dc 0x5ae54) void NewSMapHeader::~NewSMapHeader();
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:1007, dc 0x5aeb4) NewSMapHeader* NewSMapHeader::operator=(const NewSMapHeader* __that);

// --- TCampaignBrief ---
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:392, dc 0x587c4) void TCampaignBrief::Select(int which);
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:437, dc 0x58938) void TCampaignBrief::ResetMapAndDescription(int which);
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:452, dc 0x589a4) void TCampaignBrief::ClearSelected();
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:462, dc 0x58a28) void TCampaignBrief::SetupCurrentTerritory();
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:481, dc 0x58a9c) void TCampaignBrief::UpdateAllyEnemyFlags();
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:520, dc 0x58c00) void TCampaignBrief::UpdateBonusIcons();
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:584, dc 0x58dac) void TCampaignBrief::AddBonusIcons();
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:764, dc 0x594b8) void TCampaignBrief::TCampaignBrief(unsigned char newCampaign, unsigned char bViewFromGame);
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:1054, dc 0x5a2b4) int TCampaignBrief::convertID2HelpID(int id);
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:192, dc 0x5ade8) void TCampaignBrief::CampaignHeaderStruct::~CampaignHeaderStruct();
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:192, dc 0x5ae10) void TCampaignBrief::CampaignHeaderStruct::CampaignHeaderStruct();
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:1007, dc 0x5ae80) void* TCampaignBrief::`scalar deleting destructor'(unsigned __flags);

// --- game ---
// CODEVIEW(E:\gamedcs\campaignbrief.cpp:1050, dc 0x5af18) void* game::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_CAMPAIGNBRIEF_H */
