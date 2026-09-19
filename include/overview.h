// overview.h - prototypes of overview.cpp (compiland overview.obj)
#ifndef HOMM3_OVERVIEW_H
#define HOMM3_OVERVIEW_H

#include <vector>
#include "advmgr_popup.h"

class textWidget;

// Per-hero artifact subpage selected in the kingdom overview. Retail's
// SetupDynamicStuff and click handler prove that the two equipped pages are
// nine slots each and the third page is the backpack. Names are provisional;
// the domain and values are byte-proven.
enum EOverviewHeroArtifactPage {
    OVERVIEW_HERO_EQUIPPED_PAGE_1 = 0,
    OVERVIEW_HERO_EQUIPPED_PAGE_2 = 1,
    OVERVIEW_HERO_BACKPACK_PAGE = 2
};

// Actions consumed by TOverviewWindow::DoFlaggableButtons. The values are
// fixed by its four-entry retail jump table; names describe the proven
// effects because neither debug stream preserves the original enum.
enum EOverviewFlaggableAction {
    OVERVIEW_FLAGGABLE_HOME = 0,
    OVERVIEW_FLAGGABLE_PREVIOUS = 1,
    OVERVIEW_FLAGGABLE_NEXT = 2,
    OVERVIEW_FLAGGABLE_END = 3
};

// Widget/message ids consumed by game::ProcessIconSelect. Dreamcast proves
// the source-domain groupings and Complete extends the roster with the level,
// mana, specialty, summoning-portal, and second help bands. The band bases
// and unique controls below are byte-proven by the retail switch tables;
// individual consecutive slots are written as base-plus-offset at use sites.
enum EOverviewIconId {
    // The four fixed strip-navigation buttons and the unlabelled control
    // dispatched between them. Their actions are fixed by the retail
    // WindowHandler switch; the original names do not survive.
    OVERVIEW_FLAGGABLE_HOME_ID = 12,
    OVERVIEW_FLAGGABLE_END_ID = 13,
    OVERVIEW_CONTROL_14_ID = 14,
    OVERVIEW_FLAGGABLE_PREVIOUS_ID = 15,
    OVERVIEW_FLAGGABLE_NEXT_ID = 16,
    OVERVIEW_TOWN_EXIT_ID = 4,
    OVERVIEW_TOWN_GARRISON_ARMY_FIRST_ID = 5,
    OVERVIEW_TOWN_GARRISON_ARMY_SECOND_ROW_FIRST_ID = 12,
    OVERVIEW_TOWN_RECRUIT_FIRST_ID = 19,
    OVERVIEW_RESOURCE_TOTAL_ID = 28,
    OVERVIEW_MINE_FIRST_ID = 29,
    OVERVIEW_TOWN_RECRUIT_SECOND_ROW_FIRST_ID = 33,
    OVERVIEW_FLAGGABLE_FIRST_ID = 40,
    OVERVIEW_TOWN_VISITING_HERO_LEFT_ID = 48,
    OVERVIEW_TOWN_VISITING_HERO_RIGHT_ID = 50,
    OVERVIEW_TOWN_GARRISON_HERO_ID = 53,
    OVERVIEW_TOWN_VISITING_ARMY_FIRST_ID = 54,
    OVERVIEW_TOWN_VISITING_ARMY_SECOND_ROW_FIRST_ID = 61,
    OVERVIEW_TOWN_GROWTH_ICON_FIRST_ID = 69,
    OVERVIEW_TOWN_GROWTH_TEXT_FIRST_ID = 83,
    OVERVIEW_TOWN_SUMMONING_GROWTH_ICON_ID = 99,
    OVERVIEW_TOWN_SUMMONING_GROWTH_TEXT_ID = 100,
    OVERVIEW_TOWN_SUMMONING_PORTAL_ICON_ID = 101,
    OVERVIEW_TOWN_SUMMONING_PORTAL_TEXT_ID = 102,
    OVERVIEW_HERO_VIEW_ICON_ID = 103,
    OVERVIEW_HERO_VIEW_NAME_ID = 104,
    OVERVIEW_HERO_ARMY_FIRST_ID = 105,
    OVERVIEW_HERO_ARMY_SECOND_ROW_FIRST_ID = 112,
    OVERVIEW_HERO_ARTIFACT_FIRST_ID = 119,
    OVERVIEW_HERO_ARTIFACT_PAGE_1_ID = 128,
    OVERVIEW_HERO_ARTIFACT_PAGE_2_ID = 129,
    OVERVIEW_HERO_BACKPACK_FIRST_ID = 130,
    OVERVIEW_HERO_ARTIFACT_PAGE_3_ID = 138,
    OVERVIEW_HERO_SECONDARY_SKILL_FIRST_ID = 158,
    OVERVIEW_HERO_PRIMARY_STAT_FIRST_ID = 182,
    OVERVIEW_HERO_LUCK_ID = 187,
    OVERVIEW_HERO_MORALE_ID = 188,
    OVERVIEW_HERO_LEVEL_ID = 189,
    OVERVIEW_HERO_MANA_ID = 191,
    OVERVIEW_HERO_SPECIALTY_ID = 193,
    OVERVIEW_HERO_BACKPACK_SCROLL_LEFT_ID = 194,
    OVERVIEW_HERO_BACKPACK_SCROLL_RIGHT_ID = 195,
    OVERVIEW_SELECT_HEROES_ID = 195,
    OVERVIEW_SELECT_TOWNS_ID = 196,
    OVERVIEW_ROW_FIRST_ID = 200,
    OVERVIEW_ROW_STRIDE = 200,
    OVERVIEW_HELP_FIRST_ID = 1001,
    OVERVIEW_HELP_SECOND_BAND_FIRST_ID = 1009
};

// Complete allocates TOverviewWindow as exactly 0x80 bytes. Its destructor
// proves that the CAdvPopup base is followed by two VC6 vectors: it tears down
// their allocation pointers at +0x74 and +0x64, in reverse construction
// order. The earlier vector's element stride is independently eight bytes in
// the retail helper at 0x51e670. Dreamcast only exposes a forward reference
// for this class; the member names below describe retail-proven roles.
struct overview_item_record {
    // this category; updateFlaggableIcon (0x51e670) also uses it as the icon
    // frame, and doRollover selects the corresponding object description.
    int m_itemType;
    // category, incremented during construction and formatted as "%i".
    int m_count;
};
SIZE(overview_item_record, 8);

class TOverviewWindow : public CAdvPopup {
public:
    TOverviewWindow();
    virtual ~TOverviewWindow();
    virtual int windowHandler(message& msg);

    void updateFlaggableIcons();

private:
    void updateFlaggableIcon(int i);
    void doFlaggableButtons(int which);
    void clearButtons(int slot);
    void updateRollover(char* text);
    void doRollover(int codeY);

    // records consumed by updateFlaggableIcon and the scrolling controls.
    std::vector<overview_item_record> m_flaggableItems;
    // UpdateFlaggableIcon invokes textWidget's SetText virtual on every
    // element; retail therefore proves the derived pointer type, not the
    // earlier widget* placeholder.
    // count labels here; updateFlaggableIcon updates/hides each label.
    std::vector<textWidget*> m_flaggableCountWidgets;
};
SIZE(TOverviewWindow, 0x80);

// --- globals ---
// CODEVIEW(E:\gamedcs\overview.cpp:1279, dc 0x106d98) void UpdateFlaggableIcons();
// CODEVIEW(E:\gamedcs\overview.cpp:1562, dc 0x1077e8) void UpdateArtifacts(int iSlot);
// CODEVIEW(E:\gamedcs\overview.cpp:1612, dc 0x107974) void increment_backpack_start(long slot);
// CODEVIEW(E:\gamedcs\overview.cpp:1629, dc 0x1079b8) void decrement_backpack_start(long slot);
// CODEVIEW(E:\gamedcs\overview.cpp:1647, dc 0x1079fc) void show_artifact(hero* currHero, const type_artifact* artifact, unsigned char right_mouse);

// --- TOverviewWindow ---
// CODEVIEW(E:\gamedcs\overview.cpp:2017, dc 0x1084f0) void TOverviewWindow::TOverviewWindow();
// CODEVIEW(E:\gamedcs\overview.cpp:2096, dc 0x108fdc) void TOverviewWindow::ClearButtons(int slot);
// CODEVIEW(E:\gamedcs\overview.cpp:2103, dc 0x10902c) void TOverviewWindow::UpdateRollover(char* cText);
// CODEVIEW(E:\gamedcs\overview.cpp:2115, dc 0x10906c) void TOverviewWindow::DoRollover(int codeY);
// CODEVIEW(E:\gamedcs\overview.cpp:2546, dc 0x10997c) int TOverviewWindow::WindowHandler(message* msg);
// CODEVIEW(E:\gamedcs\overview.cpp:2083, dc 0x10a210) void* TOverviewWindow::`scalar deleting destructor'(unsigned __flags);

// --- game ---
// CODEVIEW(E:\gamedcs\overview.cpp:220, dc 0x104458) void game::SetupDynamicStuff(int bUpdate, int bForceUpdate);
// CODEVIEW(E:\gamedcs\overview.cpp:1170, dc 0x1069fc) void game::SetupNewOverviewType(int iWhichType, unsigned char bUpdate);
// CODEVIEW(E:\gamedcs\overview.cpp:1663, dc 0x107a90) int game::ProcessIconSelect(int codeY, unsigned char bRightMouse);

#endif  /* HOMM3_OVERVIEW_H */
