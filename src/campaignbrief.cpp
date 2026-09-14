// campaignbrief.cpp - E:\gamedcs\campaignbrief.cpp (compiland campaignbrief.obj)
#include <va.h>
#include <stdio.h>
#include <string.h>
class Message;
static int campaignBriefHandler(Message& msg);
#include "campaignbrief.h"
#include "advmgr.h"
#include "border.h"
#include "button.h"
#include "campaignmap.h"
#include "iconwdgt.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "mousemgr.h"
#include "multiplayerwindow.h"
#include "palette.h"
#include "soundmgr.h"
#include "textresource.h"
#include "textwdgt.h"
#include "textscroller.h"
#include "widget.h"
#include "winmgr.h"

// Temporary game snapshot made by the retail campaign-brief constructor.
// Dreamcast names the same cross-TU cell `saveHeader`; UpdateGameVars hands
// it back to BackupGameHeaders when the load-game row has no current file.
DATA(0x0069fdc4) Game* g_saveHeader;

// Complete keeps the current campaign-brief mode and construction-ready
// latch in campaignbrief.obj.  The constructor, Select-family methods, and
// handler all address these same cells; the names are role-based because no
// surviving PC public names either private datum.
DATA(0x00694de8) static unsigned char g_campaignBriefViewFromGame;
DATA(0x00694de0) static unsigned char g_campaignBriefReady;

// The handler's six-frame region flash. Complete retains the counter and
// next-frame deadline beside the ready latch; Dreamcast proves the same
// six/125 sequence and the GameTime helper boundary independently.
DATA(0x00694dec) static int g_campaignBriefFlashLeft;
DATA(0x00694db0) static unsigned long g_campaignBriefFlashTime;

// Dreamcast publishes the semantic table name. Complete's right-click path
// independently fixes its THelpText stride and first-pointer use at this
// address.
DATA(0x006a59cc) extern HelpText g_campaignBriefHelp[];

void backupGameHeaders(Game* dest, Game* src);

// Complete's five campaign-difficulty buttons take paired rollover/right-
// click strings from this contiguous table. Retail fixes the five-row extent
// by advancing from 0x006a6cb8 to 0x006a6ce0 in AddBonusIcons.
DATA(0x006a6cb8) static HelpText g_campaignDifficultyHelp[5];

// Both difficulty arrow buttons retain this shared message callback at
// retail 0x00457cb0. Its source name is not yet independently recovered;
// keep the role name provisional until that function is admitted.
int campaignDifficultyHandler(Message& msg);

#if 0  // Dreamcast-only carcass; retained as evidence, not emitted for retail.
// E:\gamedcs\campaignbrief.cpp:202
DC_ONLY(0x58244, 0x530)
void CampaignWait(int which)
{
    // @stub
}

// E:\gamedcs\campaignbrief.cpp:365
DC_ONLY(0x58774, 0x50)
void showTerritorySmacker(unsigned char bEvil2Post)
{
    // @stub
}

#endif

// E:\gamedcs\campaignbrief.cpp:437. The Dreamcast broadcasts the map
// description as a second widget message; Complete hands it to the
// scroller (type_text_scroller::SetText, 0x5ba6e0) instead.
DC_ONLY(0x58938, 0x6A)
inline void CampaignBrief::resetMapAndDescription(int which)
{
    Message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = Widget::WIDGET_SET_TEXT;
    msg.m_codeY = MAP_NAME_ID;
    msg.m_extraText = m_scenarios[which].m_mapName.c_str();
    broadcastMessage(msg);
    m_scroller->setText(m_scenarios[which].m_mapDescription.c_str());
}

// E:\gamedcs\campaignbrief.cpp:452. Complete keeps this and
// ResetMapAndDescription as header-style inlines: neither has a retail
// body, and Select carries both expanded - which is what makes
// vector::size a NESTED candidate there, called out of line at both
// loop tests (0x423110, the pointer-vector size COMDAT).
DC_ONLY(0x589a4, 0x84)
inline void CampaignBrief::clearSelected()
{
    for (int i = 0; i < static_cast<int>(m_campaign->m_scenarios.size()); i++) {
        if (m_scenarios[i].m_available)
            getWidget(MAP_SELECTED_1_ID + i)->hide();
    }
}

// E:\gamedcs\campaignbrief.cpp:392
// Complete adds the game-setup / map-header copy into gpGame (skipped in
// the in-game view), the WHICHMAP frame chosen by the map's Size, the OK
// button enable when the scenario's options record has no choice to
// make, and the difficulty-button refresh.
VA(0x00457990, 0x319)  // anchor-caller(TCampaignBrief ctor), dc 0x587c4
void CampaignBrief::select(int which)
{
    if (!m_scenarios[which].m_available)
        return;

    clearSelected();
    getWidget(MAP_SELECTED_1_ID + which)->show();
    m_selectedScenario = which;
    resetMapAndDescription(which);

    if (!g_campaignBriefViewFromGame) {
        g_game->m_setup = m_scenarios[which].m_gameSetup;
        g_game->m_mapHeader = m_scenarios[which];
    }

    Message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = Widget::WIDGET_SET_ICON_FRAME;
    msg.m_codeY = WHICHMAP_ID;
    switch (m_scenarios[which].m_size) {
    case MAP_SIZE_SMALL:
        msg.m_extra = 0;
        break;
    case MAP_SIZE_MEDIUM:
        msg.m_extra = 1;
        break;
    case MAP_SIZE_LARGE:
        msg.m_extra = 2;
        break;
    case MAP_SIZE_EXTRA_LARGE:
        msg.m_extra = 3;
        break;
    default:
        msg.m_extra = 4;
        break;
    }
    broadcastMessage(msg);

    if (!m_campaign->m_scenarios[which]->m_options->getCount()) {
        Widget* ok = getWidget(DIALOG_RETURN_OK);
        if (ok)
            ok->enable(1);
    }
    updateBonusIcons();
    updateDifficultyButtons();
    updateAllyEnemyFlags();
    drawWindow(1, 0xffff0001, 0xffff);
}

VA_COMPGEN(0x00457cb0, 0x2B8, IMPLICIT_COPY_ASSIGN, CMapHeaderData)
VA_COMPGEN(0x0054DEB0, 0x13, VECTOR_CAPACITY, Int)

#if 0  // Dreamcast-only carcass; retained as evidence, not emitted for retail.
// E:\gamedcs\campaignbrief.cpp:462
DC_ONLY(0x58a28, 0x74)
void CampaignBrief::setupCurrentTerritory()
{
    // @stub
}

#endif

// The local player's slot in the selected scenario, DC ?playerSlot@@3HA;
// retail .bss 0x694dcc, written by UpdateAllyEnemyFlags below.
DATA(0x00694dcc)
static int g_campaignBriefPlayerSlot;

// E:\gamedcs\campaignbrief.cpp:481. Complete retains the same source helper
// immediately after Select; CampaignBriefHandler calls it at retail +0x4f9.

// Lay the eight player flags out as allies (ALLY_FLAG1_ID..) and enemies
// (ENEMY_FLAG1_ID..) of the local player's slot, hiding every flag first and skipping
// the positions the scenario leaves empty.

// LANDED: naming the briefing-choice argument before the scenario lookup and
// retaining the scenario pointer as a source local restores retail's evaluation
// order; declaring the loop index before the two running widget ids restores its
// EDI lifetime. Together these move 86.8125 -> 99.8958 with all nine CFG blocks,
// five branches and thirteen calls exact. The remaining byte is the commutative
// SIB spelling in the second teamInfo lookup inside Dreamcast-proven OnSameTeam:
// candidate [ecx+eax+0x1f879], retail [eax+ecx+0x1f879]. Naming either the player
// slot or the game receiver is byte-flat, so keep the canonical helper boundary.
VA(0x00458010, 0x10F)  // handler caller + DC source identity, dc 0x58a9c
void CampaignBrief::updateAllyEnemyFlags()
{
    int briefingChoice = g_game->m_campaign.m_briefingChoice;
    ScenarioStruct* scenario =
        m_campaign->m_scenarios[m_selectedScenario];
    g_campaignBriefPlayerSlot =
        scenario->m_options->getPlayer(briefingChoice);
    int i = 0;
    int enemyFlagId = ENEMY_FLAG1_ID;
    int allyFlagId = ALLY_FLAG1_ID;
    for (; i < 8; i++) {
        getWidget(i + ENEMY_FLAG1_ID)->hide();
        getWidget(i + ALLY_FLAG1_ID)->hide();
        if (g_game->m_setup.m_playerPos[i] >= 0) {
            if (g_game->onSameTeam(i, g_campaignBriefPlayerSlot)) {
                getWidget(allyFlagId)->show();
                getWidget(allyFlagId)->sendMessage(
                    Widget::WIDGET_SET_ICON_FRAME, i);
                allyFlagId++;
            } else {
                getWidget(enemyFlagId)->show();
                getWidget(enemyFlagId)->sendMessage(
                    Widget::WIDGET_SET_ICON_FRAME, i);
                enemyFlagId++;
            }
        }
    }
}

#if 0  // Dreamcast-only carcass; retained as evidence, not emitted for retail.

// E:\gamedcs\campaignbrief.cpp:649
DC_ONLY(0x59300, 0x1B8)
void ExtractCampaignMap(int* numPreReqs, unsigned char single_map_only, unsigned char write_file)
{
    // @stub
}

#endif

// E:\gamedcs\campaignbrief.cpp:584. Dreamcast proves the initial label,
// three-choice widget groups, visibility changes, help-text calls and the
// two major lexical branches. Complete replaces the generated bitmap-name
// table with fixed PC controls: three frames, three bitmap choices, three
// sprite highlights, five difficulty buttons, and two optional arrows.
// Retail independently fixes every constructor argument and array extent.
// Residual (98.84%): 130/132 CFG blocks and all 68 branches agree. The sole
// call difference is the final pointer-vector insertion: this compile retains
// the empty `_Destroy` helper while retail expands it away, shifting 29 tail
// instructions. Measured and rejected: two-argument `insert(end(), value)`
// on the first arrow (72.43%), second arrow (71.91%), or both (76.12%), and
// three-argument `insert(end(), 1, value)` on the second arrow (77.52%); each
// changes the whole /Ob2 frontier and destroys the otherwise exact CFG.
VA(0x00458120, 0xC1C)  // anchor-caller(TCampaignBrief ctor), dc 0x58dac
void CampaignBrief::addBonusIcons()
{
    int i;

    m_widgets.push_back(new TextWidget(
        476, 425, 194, 30, (*g_generalText)[72],
        DATA_COMPGEN(0x0065f2ec, campaignBonusMediumFont, "medfont.fnt"),
        static_cast<Font::Color>(4), 242, 5, 0, 8));

    m_startBonusBorders[0] = new ColoredBorderFrame(
        475, 454, 60, 66, 232, g_systemPalette->m_data[45], 0x400);
    m_startBonusBorders[1] = new ColoredBorderFrame(
        543, 454, 60, 66, 233, g_systemPalette->m_data[45], 0x400);
    m_startBonusBorders[2] = new ColoredBorderFrame(
        611, 454, 60, 66, 234, g_systemPalette->m_data[45], 0x400);

    m_bitmapBonusImages[0] = new BitmapBorder(
        476, 455, 58, 64, 226, 0, 0x800);
    m_bitmapBonusImages[1] = new BitmapBorder(
        544, 455, 58, 64, 227, 0, 0x800);
    m_bitmapBonusImages[2] = new BitmapBorder(
        612, 455, 58, 64, 228, 0, 0x800);

    m_spriteBonusImages[0] = new IconWidget(
        476, 455, 58, 64, 229, 0, 0, 0, 0, 0,
        IconWidget::ICON_STYLE_PLAIN);
    m_spriteBonusImages[1] = new IconWidget(
        544, 455, 58, 64, 230, 0, 0, 0, 0, 0,
        IconWidget::ICON_STYLE_PLAIN);
    m_spriteBonusImages[2] = new IconWidget(
        612, 455, 58, 64, 231, 0, 0, 0, 0, 0,
        IconWidget::ICON_STYLE_PLAIN);

    for (i = 0; i < 3; ++i) {
        m_startBonusBorders[i]->setVisible(0);
        m_widgets.push_back(m_startBonusBorders[i]);
        m_bitmapBonusImages[i]->setVisible(0);
        m_widgets.push_back(m_bitmapBonusImages[i]);
        m_spriteBonusImages[i]->setVisible(0);
        m_widgets.push_back(m_spriteBonusImages[i]);
    }

    m_difficultyButtons[0] = new Button(
        710, 455, 30, 46, 235,
        DATA_COMPGEN(0x00660e5c, campaignDifficultyButton3,
                     "gspbut3.def"),
        0, 1, 0, 0, 2);
    m_difficultyButtons[1] = new Button(
        710, 455, 30, 46, 236,
        DATA_COMPGEN(0x00660e50, campaignDifficultyButton4,
                     "gspbut4.def"),
        0, 1, 0, 0, 2);
    m_difficultyButtons[2] = new Button(
        710, 455, 30, 46, 237,
        DATA_COMPGEN(0x00660e44, campaignDifficultyButton5,
                     "gspbut5.def"),
        0, 1, 0, 0, 2);
    m_difficultyButtons[3] = new Button(
        710, 455, 30, 46, 238,
        DATA_COMPGEN(0x00660e38, campaignDifficultyButton6,
                     "gspbut6.def"),
        0, 1, 0, 0, 2);
    m_difficultyButtons[4] = new Button(
        710, 455, 30, 46, 239,
        DATA_COMPGEN(0x00660e2c, campaignDifficultyButton7,
                     "gspbut7.def"),
        0, 1, 0, 0, 2);

    for (i = 0; i < 5; ++i) {
        m_difficultyButtons[i]->setHelpText(
            g_campaignDifficultyHelp[i].m_text,
            g_campaignDifficultyHelp[i].m_rclick, 0);
        m_difficultyButtons[i]->sendMessage(
            Widget::WIDGET_CLEAR_STATUS,
            Widget::WIDGET_ACTIVE | Widget::WIDGET_DRAWN);
        m_widgets.push_back(m_difficultyButtons[i]);
    }

    m_widgets.push_back(new TextWidget(
        680, 425, 90, 30, (*g_generalText)[441],
        DATA_COMPGEN(0x0065f2ec, campaignDifficultyMediumFont,
                     "medfont.fnt"),
        static_cast<Font::Color>(4), -1, 5, 0, 8));

    if (g_campaignBriefViewFromGame) {
        m_difficultyDecrButton = 0;
        m_difficultyIncrButton = 0;
        return;
    }

    m_difficultyDecrButton = new FuncButton(
        704, 506, 16, 16, 240,
        DATA_COMPGEN(0x00660e1c, campaignDifficultyArrowSprite,
                     "SlideBuH.def"),
        campaignDifficultyHandler, 0, 1);
    m_difficultyIncrButton = new FuncButton(
        730, 506, 16, 16, 241,
        DATA_COMPGEN(0x00660e1c, campaignDifficultyArrowSprite,
                     "SlideBuH.def"),
        campaignDifficultyHandler, 2, 3);
    m_widgets.push_back(m_difficultyDecrButton);
    m_widgets.push_back(m_difficultyIncrButton);
}

// E:\gamedcs\campaignbrief.cpp:520. Complete widens the Dreamcast's
// three fixed slots into the options record's own count: a two-choice
// scenario centres its frames, every shown frame carries either the
// bitmap or the sprite form of the bonus with its help text, and the
// frames past the count are hidden.

// Residual (95.44%): 34 blocks against retail's 34, all 16 branches and the
// single return agree, 32 blocks byte-exact. 2026-09-06, polish lane 35: the
// status send_message was a TERNARY ARGUMENT over two commands one apart in
// value, and VC6 folds such a pair branchlessly (`xor ecx,ecx / cmp edi,[..] /
// setne cl / add ecx,K`) where retail branches and cross-jumps the shared call
// (`jne / push SET / jmp / push CLEAR / call`). Writing it as an if/else over
// two send_message calls recovers retail's shape - 31 blocks -> 34, three
// flow-kind divergences -> none, 95.3700 -> 95.4400. What is left is one
// register transposition in the `campaign->scenarios[selected_scenario]`
// chain (retail `[ecx + 4*eax]`, ours `[eax + 4*ecx]`) plus retail loading
// briefingChoice into EAX before the compare where we compare against memory.
// Measured and rejected: naming the choice in an `int choice` local inside the
// loop (byte-flat, 95.4400); naming the receiver widget (91.90, and it costs
// the whole block agreement); both together (91.90); and swapping the
// `scenario` / `int i` declaration order (byte-flat) - the SIB flip is not
// reachable from this loop's index scope because the third loop consumes `i`.
VA(0x00458d40, 0x297)  // Select callee, dc-order-map after AddBonusIcons, dc 0x58c00
void CampaignBrief::updateBonusIcons()
{
    ScenarioStruct* scenario = m_campaign->m_scenarios[m_selectedScenario];
    int i;

    if (scenario->m_options->getCount() == CampaignStartOption::CHOICE_COUNT_PAIR) {
        m_startBonusBorders[0]->m_x = 509;
        m_startBonusBorders[1]->m_x = 577;
    } else {
        m_startBonusBorders[0]->m_x = 475;
        m_startBonusBorders[1]->m_x = 543;
    }
    for (i = 0; i < 2; i++) {
        m_bitmapBonusImages[i]->m_x = m_startBonusBorders[i]->m_x + 1;
        m_spriteBonusImages[i]->m_x = m_startBonusBorders[i]->m_x + 1;
    }

    for (i = 0; i < scenario->m_options->getCount(); i++) {
        m_startBonusBorders[i]->show();
        if (i == g_game->m_campaign.m_briefingChoice)
            m_startBonusBorders[i]->sendMessage(Widget::WIDGET_SET_STATUS, 4);
        else
            m_startBonusBorders[i]->sendMessage(Widget::WIDGET_CLEAR_STATUS,
                                                 4);
        const char* name = scenario->m_options->getIconDefName(&g_game->m_campaign, i);
        if (scenario->m_options->isBuildingBonus(i)) {
            m_bitmapBonusImages[i]->show();
            m_bitmapBonusImages[i]->setImage(name);
            m_spriteBonusImages[i]->hide();
        } else {
            m_spriteBonusImages[i]->show();
            m_spriteBonusImages[i]->setSprite(name);
            m_spriteBonusImages[i]->setIconFrame(scenario->m_options->getIconIndex(i));
            m_bitmapBonusImages[i]->hide();
        }
        std::string text;
        text = scenario->getBonusText(m_campaign, i);
        m_bitmapBonusImages[i]->setHelpText("", text.c_str(), 1);
        m_spriteBonusImages[i]->setHelpText("", text.c_str(), 1);
    }
    for (; i < 3; i++) {
        m_startBonusBorders[i]->hide();
        m_bitmapBonusImages[i]->hide();
        m_spriteBonusImages[i]->hide();
    }
}

VA(0x00458fe0, 0x2C)
std::string CampaignBrief::ScenarioStruct::getBonusText(
    CampaignHeaderStruct* campaign, int option)
{
    return m_options->getText(campaign, option);
}

// Complete-only; see campaignbrief.h.
VA(0x00459010, 0xB0)
void CampaignBrief::updateDifficultyButtons()
{
    for (int i = 0; i < 5; i++) {
        if (i == g_game->m_setup.m_difficulty)
            m_difficultyButtons[i]->show();
        else
            m_difficultyButtons[i]->hide();
    }
    if (!g_campaignBriefViewFromGame && m_difficultyDecrButton) {
        if (g_game->m_setup.m_difficulty > 0 && m_campaign->m_variableDifficulty)
            m_difficultyDecrButton->show();
        else
            m_difficultyDecrButton->hide();
        if (g_game->m_setup.m_difficulty < 4 && m_campaign->m_variableDifficulty)
            m_difficultyIncrButton->show();
        else
            m_difficultyIncrButton->hide();
    }
}

// E:\gamedcs\campaignbrief.cpp:764
// Dreamcast supplies the original helper, local, scope, and statement order;
// Complete independently fixes the PC-only campaign preview layout, widget
// constructors, campaign-header ABI, and every branch below.  The otherwise
// unused numPreReqs local is retained as a positive source-shape fact.
// DEPTH LADDER (docs/vc6/inliner.md 6b), 2026-09-06: every append here is
// `Widgets.insert(Widgets.end(), new W(...))`, not `push_back`.  Polish 29
// re-opened this row on the five APPENDS IT COULD SEE (85.7661 -> 86.6820,
// its line-anchored sweep skipped the twelve whose argument list wraps);
// spelling all seventeen the same way is worth a further 86.6820 -> 88.1039.
// The rung's sign is per-site: flipping the five back to `push_back` is
// -0.92, so the shallower level is the one this body's /Ob2 budget wants at
// all seventeen.
// LOOP-COUNTER SIGNEDNESS (docs/vc6/behavior-catalog.md D23): four of this
// body's nine zero-initialised `for` counters are `unsigned int`, not `int`.
// They only pay TOGETHER - 89.0593 / 90.0586 / 90.1377 / 90.9771 / 91.1880 as
// they accumulate - and the fifth through ninth all fall back.
VA(0x004590c0, 0x1319)  // anchor-caller/callee/string/vtable, dc 0x594b8
CampaignBrief::CampaignBrief(unsigned char newCampaign,
                               unsigned char viewFromGame)
    : HeroWindow(0, 0, 800, 600, 0)
{
    unsigned char bitMask[8];
    int numPreReqs;
    int mx;
    int my;
    Widget* w;

    g_campaignBriefViewFromGame = viewFromGame;

    if (viewFromGame) {
        g_saveHeader = new Game;
        backupGameHeaders(g_saveHeader, g_game);
        m_selectedScenario = g_game->m_campaign.m_currentMap;
    } else {
        for (int i = 0; i < 8; ++i)
            g_game->m_players[i].init();
        g_game->m_campaign.m_briefingChoice = -1;
    }

    m_zBuffer = new unsigned short[m_width * m_height];
    memset(m_zBuffer, 0, m_width * m_height * sizeof(*m_zBuffer));
    // The widget vector NAMED AS A REFERENCE, so every append reads _Last
    // through the vector's own address rather than folding the member offset
    // off `this` (docs/vc6/inliner.md 6b's companion lever).  88.1039 ->
    // 88.3754 across all 24 uses.
    std::vector<Widget*>& widgets = m_widgets;
    widgets.reserve(NWIDGETS);

    const char* campaignFilename =
        g_game->m_campaign.getCampaignFileName().c_str();
    m_campaign = new CampaignHeaderStruct(campaignFilename);
    if (!m_campaign->load()) {
        // Retail's dec/je/dec/jne dispatch proves a sparse switch rather
        // than an if-chain; VC6 sinks these two arms ahead of the shared
        // campaign teardown in reverse case order.
        switch (m_campaign->m_fileError) {
        case CampaignHeaderStruct::CAMPAIGN_FILE_OPEN_FAILED:
            normalDialog(
                formatString((*g_generalText)[11], campaignFilename).c_str(),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            break;
        case CampaignHeaderStruct::CAMPAIGN_FILE_VERSION_UNSUPPORTED:
            normalDialog(
                formatString((*g_generalText)[724], campaignFilename).c_str(),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            break;
        }
        delete m_campaign;
        m_campaign = 0;
        return;
    }

    if (viewFromGame) {
        CampaignScenarioPreview preview;
        static_cast<NewSMapHeader&>(preview) = g_game->m_mapHeader;
        preview.m_gameSetup = g_game->m_setup;
        for (unsigned int i = 0;
             i < static_cast<int>(m_campaign->m_scenarios.size()); ++i) {
            m_scenarios.insert(m_scenarios.end(), preview);
        }
    } else {
        NewSMapHeader mapHeader;
        for (int i = 0;
             i < static_cast<int>(m_campaign->m_scenarios.size()); ++i) {
            CampaignScenarioPreview preview;
            ScenarioStruct* scenario = m_campaign->m_scenarios[i];
            g_game->m_setup.m_fileInitialized = 0;
            if (scenario->m_inflatedSize > 0) {
                m_campaign->loadScenario(i, &mapHeader);
                g_game->setupOrigData();
                g_game->initNewGame(
                    scenario->m_difficulty, i, &mapHeader, 0);
            }
            static_cast<NewSMapHeader&>(preview) = g_game->m_mapHeader;
            preview.m_gameSetup = g_game->m_setup;
            m_scenarios.insert(m_scenarios.end(), preview);
        }
    }

    m_campaign->getAvailableScenarios(bitMask);
    for (unsigned int availableIndex = 0;
         availableIndex < m_scenarios.size(); ++availableIndex) {
        m_scenarios[availableIndex].m_available = bitMask[availableIndex];
    }

    const CampaignMapTraits& mapTraits =
        g_campaignMapTraits[m_campaign->m_regionMap];
    widgets.insert(widgets.end(), new BitmapBorder16(
                    0, 0, 800, 600, BACKGROUND_ID, mapTraits.m_imageName, 0x800));

    for (int regionIndex = 0;
         regionIndex < mapTraits.m_numRegions; ++regionIndex) {
        const CampaignMapTraits::RegionTraits& region =
            mapTraits.m_regionTraits[regionIndex];
        ScenarioStruct* scenario = m_campaign->m_scenarios[regionIndex];
        if (scenario->m_inflatedSize > 0) {
            // BOUND BY `const int&`: retail re-reads the scenario's colour
            // at each of the three image-name subscripts.  88.3754 -> 89.0593.
            const int& color = scenario->m_regionColor;
            if (g_game->m_campaign.m_mapScores[regionIndex].m_completed) {
                widgets.insert(widgets.end(), new BitmapBorder(
                                region.m_offsetX, region.m_offsetY, 20, 20,
                                MAP_CONQUERED_1_ID + regionIndex,
                                region.m_conqueredImageName[color], 0x800));
                m_scenarios[regionIndex].m_available = false;
            }
            if (m_scenarios[regionIndex].m_available) {
                widgets.insert(widgets.end(), new BitmapBorder(
                                region.m_offsetX, region.m_offsetY, 20, 20,
                                MAP_ENABLED_1_ID + regionIndex,
                                region.m_enabledImageName[color], 0x800));
                widgets.insert(widgets.end(), new BitmapBorder(
                                region.m_offsetX, region.m_offsetY, 20, 20,
                                MAP_SELECTED_1_ID + regionIndex,
                                region.m_selectedImageName[color], 0x800));
            }
        } else {
            m_scenarios[regionIndex].m_available = false;
        }
    }

    widgets.insert(widgets.end(), new BitmapBorder(
                    456, 6, 330, 585, BACKGROUND_ID,
                    DATA_COMPGEN(0x00660ea8, campaignBriefPanel, "campbrf.pcx"),
                    0x800));

    if (viewFromGame) {
        widgets.insert(widgets.end(), new Button(
                        476, 536, 146, 40, RESTART_ID,
                        DATA_COMPGEN(0x00660e9c, campaignBriefRestartButton,
                                     "CBRESTB.DEF"),
                        0, 1, 0, 19, 2));
        widgets.insert(widgets.end(), new Button(
                        705, 214, 64, 30, VIDEO_ID,
                        DATA_COMPGEN(0x00660e90, campaignBriefVideoButton,
                                     "CBVIDEB.DEF"),
                        0, 1, 0, 47, 2));
    } else {
        widgets.insert(widgets.end(), new Button(
                        476, 536, 146, 40, 0x7802,
                        DATA_COMPGEN(0x00660e84, campaignBriefBeginButton,
                                     "CBBEGIB.DEF"),
                        0, 1, 0, 28, 2));
        widgets.back()->enable(0);
    }
    widgets.insert(widgets.end(), new Button(
                    624, 536, 146, 40, 0x7801,
                    DATA_COMPGEN(0x00660e78, campaignBriefCancelButton,
                                 "CBCANCB.DEF"),
                    0, 1, 0, 1, 2));

    if (m_campaign->getCampaignName().length() > 0) {
        widgets.insert(widgets.end(), new TextWidget(
                        481, 22, 246, 32, m_campaign->getCampaignName().c_str(),
                        DATA_COMPGEN(0x00660b24, campaignBriefBigFont, "bigfont.fnt"),
                        static_cast<Font::Color>(8), CAMPAIGN_NAME_ID, 4, 0, 8));
    }
    widgets.insert(widgets.end(), new TextWidget(
                    481, 63, 270, 108, (*g_generalText)[39],
                    DATA_COMPGEN(0x0065f2f8, campaignBriefSmallFont, "smalfont.fnt"),
                    static_cast<Font::Color>(2), CAMPAIGN_DESCRIPTION_ID, 0, 0, 8));
    if (m_campaign->getCampaignDescription().length() > 0) {
        widgets.insert(widgets.end(), new TextWidget(
                        481, 86, 277, 120,
                        m_campaign->getCampaignDescription().c_str(),
                        DATA_COMPGEN(0x0065f2f8, campaignBriefSmallFont,
                                     "smalfont.fnt"),
                        Font::WHITE, CAMPAIGN_DESCRIPTION_ID, 0, 0, 8));
    }

    if (g_campaignBriefViewFromGame) {
        m_selectedScenario = g_game->m_campaign.m_currentMap;
    } else {
        for (unsigned int selectedIndex = 0;
             selectedIndex < static_cast<int>(m_scenarios.size());
             ++selectedIndex) {
            if (m_scenarios[selectedIndex].m_available) {
                m_selectedScenario = selectedIndex;
                g_game->m_setup = m_scenarios[selectedIndex].m_gameSetup;
                break;
            }
        }
    }

    widgets.insert(widgets.end(), new TextWidget(
                    481, 213, viewFromGame ? 217 : 281, 32,
                    m_scenarios[m_selectedScenario].m_mapName.c_str(),
                    DATA_COMPGEN(0x00660b24, campaignBriefBigFont, "bigfont.fnt"),
                    static_cast<Font::Color>(8), MAP_NAME_ID, 4, 0, 8));
    widgets.insert(widgets.end(), new TextWidget(
                    481, 253, 270, 108, (*g_generalText)[497],
                    DATA_COMPGEN(0x0065f2f8, campaignBriefSmallFont, "smalfont.fnt"),
                    static_cast<Font::Color>(2), CAMPAIGN_DESCRIPTION_ID, 0, 0, 8));
    m_scroller = new TextScroller(
        m_scenarios[m_selectedScenario].m_mapDescription.c_str(),
        481, 278, 277, 108,
        DATA_COMPGEN(0x0065f2f8, campaignBriefSmallFont, "smalfont.fnt"),
        Font::WHITE, Slider::BLUE);
    widgets.insert(widgets.end(), m_scroller);

    widgets.insert(widgets.end(), new IconWidget(
                    735, 26, 29, 23, WHICHMAP_ID,
                    DATA_COMPGEN(0x00660e68, campaignBriefScenarioMapSize,
                                 "scnrmpsz.def"),
                    0, 0, 0, 0, IconWidget::ICON_STYLE_PLAIN));

    sprintf(g_text,
            DATA_COMPGEN(0x00660d28, campaignBriefLabelFormat, "%s:"),
            (*g_generalText)[391]);
    widgets.insert(widgets.end(), new TextWidget(
                    480, 404, 44, 23, g_text,
                    DATA_COMPGEN(0x0065f2f8, campaignBriefSmallFont, "smalfont.fnt"),
                    Font::WHITE, 100, 6, 0, 8));
    sprintf(g_text,
            DATA_COMPGEN(0x00660d28, campaignBriefLabelFormat, "%s:"),
            (*g_generalText)[392]);
    widgets.insert(widgets.end(), new TextWidget(
                    612, 404, 58, 23, g_text,
                    DATA_COMPGEN(0x0065f2f8, campaignBriefSmallFont, "smalfont.fnt"),
                    Font::WHITE, 100, 6, 0, 8));

    for (unsigned int flagIndex = 0; flagIndex < 8; ++flagIndex) {
        w = new IconWidget(
            526 + flagIndex * 15, 406, 15, 20,
            ALLY_FLAG1_ID + flagIndex,
            DATA_COMPGEN(0x00660d18, campaignBriefFlagSprites,
                         "itgflags.def"),
            0, 0, 0, 0, IconWidget::ICON_STYLE_PLAIN);
        w->sendMessage(Widget::WIDGET_CLEAR_STATUS,
                        Widget::WIDGET_ACTIVE | Widget::WIDGET_DRAWN);
        // TCampaignBrief::TCampaignBrief -> vector<widget*>::insert: DC proves
        // the source operation is push_back, while retail retains its nested
        // three-argument insert at 0x54d120.  This narrow depth-0 control stops
        // the otherwise expanded size/_Ucopy/_Ufill/_Destroy family, although
        // it currently stops one layer early at push_back.  Negative controls:
        // ordinary depth and depth 1 both produce 73.58% / 216 blocks; pinning
        // only this site leaves 202 blocks / 80.97%, versus 185 in retail.
#pragma inline_depth(0)
        widgets.insert(widgets.end(), w);
#pragma inline_depth()

        w = new IconWidget(
            673 + flagIndex * 15, 406, 15, 20,
            ENEMY_FLAG1_ID + flagIndex,
            DATA_COMPGEN(0x00660d18, campaignBriefFlagSprites,
                         "itgflags.def"),
            0, 0, 0, 0, IconWidget::ICON_STYLE_PLAIN);
        w->sendMessage(Widget::WIDGET_CLEAR_STATUS,
                        Widget::WIDGET_ACTIVE | Widget::WIDGET_DRAWN);
        // The second DC push_back independently reaches the same retained
        // retail insert.  Its one-pin negative control leaves 200 blocks and
        // 81.38%; both controls together give the current 187-block / 85.72%
        // checkpoint while the natural source-state threshold is recovered.
#pragma inline_depth(0)
        widgets.insert(widgets.end(), w);
#pragma inline_depth()
    }

    addBonusIcons();

    for (std::vector<Widget*>::iterator it = widgets.begin();
         it != widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    m_oldVolume = g_unk698760;
    if (viewFromGame)
        g_unk698760 /= 2;
    m_campaign->startMusic();

    if (viewFromGame) {
        for (unsigned int buttonIndex = 0; buttonIndex < 3; ++buttonIndex) {
            w = getWidget(232 + buttonIndex);
            if (w) {
                w->sendMessage(
                    buttonIndex == g_game->m_campaign.m_briefingChoice
                        ? Widget::WIDGET_SET_STATUS
                        : Widget::WIDGET_CLEAR_STATUS,
                    Widget::WIDGET_DRAWN);
            }
        }
    }

    for (int drawIndex = 0;
         drawIndex < static_cast<int>(m_campaign->m_scenarios.size());
         ++drawIndex) {
        if (m_campaign->m_scenarios[drawIndex]->m_inflatedSize > 0) {
            if (g_game->m_campaign.m_mapScores[drawIndex].m_completed) {
                w = getWidget(MAP_CONQUERED_1_ID + drawIndex);
                mx = w->getRealWidth();
                w->m_width = mx;
                my = w->getRealHeight();
                w->m_height = my;
                w->zBufferDraw(m_zBuffer,
                               MAP_CONQUERED_1_ID + drawIndex);
            }
            if (m_scenarios[drawIndex].m_available) {
                w = getWidget(MAP_SELECTED_1_ID + drawIndex);
                mx = w->getRealWidth();
                w->m_width = mx;
                my = w->getRealHeight();
                w->m_height = my;
                w->sendMessage(Widget::WIDGET_CLEAR_STATUS,
                                Widget::WIDGET_ACTIVE |
                                    Widget::WIDGET_DRAWN);

                w = getWidget(MAP_ENABLED_1_ID + drawIndex);
                mx = w->getRealWidth();
                w->m_width = mx;
                my = w->getRealHeight();
                w->m_height = my;
                w->zBufferDraw(m_zBuffer, MAP_ENABLED_1_ID + drawIndex);
            }
        }
    }

    select(m_selectedScenario);
    g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
    g_campaignBriefReady = 1;
}

// Complete retains these three by-value accessors adjacent to the campaign
// constructor.  Each body is the ordinary Dinkumware string copy from the
// layout-proven member and provides a named relocation target for the caller.
VA(0x0045a3e0, 0x134)
std::string SCampaign::getCampaignFileName() const
{
    return m_campaignFilename;
}

VA(0x0045a520, 0x134)
std::string CampaignBrief::CampaignHeaderStruct::getCampaignName() const
{
    return m_campaignName;
}

VA(0x0045a660, 0x134)
std::string CampaignBrief::CampaignHeaderStruct::getCampaignDescription() const
{
    return m_campaignDesc;
}

// Canonical body and VA: include/game.h.
// Canonical body and VA: include/game.h.
VA_COMPGEN(0x0045a990, 0x119, CLASS_CTOR, CMapHeaderData)
VA_COMPGEN(0x0045aab0, 0xCC, IMPLICIT_DTOR, CMapHeaderData)
// The owner token has to be the class the DEMANGLER produces from the
// emitted symbol - `??1TPlayerSlotAttributes@CMapHeaderData@@QAE@XZ` keys as
// `tplayerslotattributes_tplayerslotattributes@dtor`, with no enclosing-class
// prefix. Default construction, destruction, copy construction and copy
// assignment each have their own admitted claim.
VA_COMPGEN(0x0045ab80, 0x9B, IMPLICIT_DTOR, PlayerSlotAttributes)
// Canonical body and VA: include/game.h.

VA_COMPGEN(0x0045ad00, 0x13E, IMPLICIT_DTOR, NewSMapHeader)

VA_COMPGEN(0x0045ae40, 0x21, SCALAR_DELETING_DTOR, CampaignBrief)

VA_COMPGEN(0x0045ae70, 0x13E, IMPLICIT_DTOR, CampaignScenarioPreview)

VA_COMPGEN(0x0045b140, 0x6E, IMPLICIT_DTOR, map)

// Canonical body and VA: include/victorylossconditions.h.

// COMDAT pairing: vector<type_map_hero_identity>'s copy assignment, 661 B
// against campaignbrief.obj's single 661-byte COMDAT.
VA_COMPGEN(0x0045bcd0, 0x295, VECTOR_COPY_ASSIGN, MapHeroIdentity)

// The same map's two-argument constructor - `map(const key_compare&, const
// allocator_type&)`, the form the copy path builds through. Byte-verified
// against the emitted COMDAT at 0.984 mnemonic agreement over 190 bytes.
VA_COMPGEN(0x0045bf70, 0xBE, CLASS_CTOR, map)

VA_COMPGEN(0x0045c740, 0x165, TREE_COPY, MapHeroInfo)

// The scenario vector's own teardown, and the sole caller of the preview
// destructor claimed above: it walks its elements with the 0x4d4 stride,
// frees the block and zeroes the three pointers - Dinkumware's `_Tidy`
// shape verbatim.
VA_COMPGEN(0x0045c030, 0x3B, VECTOR_DTOR, CampaignScenarioPreview)

// push_back on the layout-proven 0x4d4-byte preview retains Dinkumware's
// three-argument vector::insert specialization in campaignbrief.obj.
VA_COMPGEN(0x0045c960, 0x3AD, VECTOR_INSERT, CampaignScenarioPreview)

VA_COMPGEN(0x0045cf70, 0x2BE, IMPLICIT_COPY_ASSIGN, PlayerSlotAttributes)

VA_COMPGEN(0x0045cd10, 0x25A, IMPLICIT_COPY_ASSIGN, NewSMapHeader)

VA_COMPGEN(0x0045d270, 0xFF, TREE_COPY_NODE, MapHeroInfo)

VA_COMPGEN(0x0045D370, 0xA3, TREE_CONST_ITERATOR_INC, MapHeroInfo)

// type_map_hero_identity's implicit copy-assign, the element operation the
// map's node copy drives. Byte-verified against the emitted COMDAT at 0.973
// mnemonic agreement over 343 bytes.
VA_COMPGEN(0x0045d8e0, 0x157, IMPLICIT_COPY_ASSIGN, MapHeroIdentity)

// The vector copy-assignment above and both TPlayerSlotAttributes assignment
// loops invoke this wrapper on 0x14-byte hero-identity elements. Its retained
// destructor call tears down the string at +4 before optional scalar delete.
VA_COMPGEN(0x0045DA40, 0x21, SCALAR_DELETING_DTOR,
           MapHeroIdentity)

VA(0x0045afb0, 0x18F)  // dc 0x5a11c
CampaignBrief::~CampaignBrief()
{
    if (g_saveHeader) {
        g_unk698760 = m_oldVolume;
        g_soundManager->switchAmbientMusic(
            g_terrainMusicIds[g_advManager->m_lastTerrain]);
        *g_game = *g_saveHeader;
        delete g_saveHeader;
        g_saveHeader = 0;
        g_advManager->redrawAdvScreen(1, 0);
    }

    delete m_campaign;
    if (m_zBuffer)
        delete[] m_zBuffer;

    for (std::vector<Widget*>::iterator it = m_widgets.begin();
         it != m_widgets.end(); ++it) {
        delete *it;
    }
}

#if 0  // Dreamcast-only carcass; retained as evidence, not emitted for retail.
// E:\gamedcs\campaignbrief.cpp:1054
DC_ONLY(0x5a2b4, 0x54)
int CampaignBrief::convertID2HelpID(int id) const
{
    // @stub
}

// E:\gamedcs\campaignbrief.cpp:1071
DC_ONLY(0x5a308, 0x1A)
void CampaignBrief::doModal()
{
    // @stub
}
#endif

// Dreamcast proves this ordinary private helper and its four source-level
// id groups. Complete expands it into CampaignBriefHandler, so no standalone
// x86 row remains; retaining the call here lets VC6 make that natural /Ob2
// decision from the real body and source order.
int CampaignBrief::convertID2HelpID(int id) const
{
    if (id >= MAP_CONQUERED_1_ID && id <= MAP_CONQUERED_32_ID)
        return id - MAP_CONQUERED_1_ID;
    if (id >= MAP_ENABLED_1_ID && id <= MAP_ENABLED_32_ID)
        return id - MAP_ENABLED_1_ID;
    if (id >= MAP_SELECTED_1_ID && id <= MAP_SELECTED_32_ID)
        return id - MAP_SELECTED_1_ID;
    return id - BACKGROUND_ID;
}

VA(0x0045b1b0, 0x28)  // dc 0x5a308
void CampaignBrief::doModal()
{
    if (!m_campaign) {
        g_windowManager->m_dialogReturn = DIALOG_RETURN_CANCEL;
        return;
    }
    g_windowManager->doDialog(this, campaignBriefHandler, 0);
}

// Dreamcast supplies the original statement groups, helper boundaries and
// six-frame flash loop. Complete's retail body removes the old hero-popup
// arm and replaces the launch sequence with its streamed campaign format;
// every retained operation below is independently present in the x86 CFG.
// Structural checkpoint: the flash prefix is 23/23 CFG blocks exact, and
// retaining widget::set_visible reproduces the retail bonus-choice branch.
// The remaining excess blocks are Dinkumware lowering inside the region
// string assignment and NewSMapHeader construction/destruction: this build
// expands assign/vector/tree bodies which retail retains, while expanding a
// three-argument `text.assign(...)` spelling was a negative control (132
// blocks, 78.07%). Keep the DC-proven operator= source fact and recover the
// surrounding natural inline state; do not flatten or pin these boundaries.
VA(0x0045b1e0, 0x8DB)  // DoModal address-take + full retail CFG, dc 0x5a324
static int campaignBriefHandler(Message& msg)
{
    CampaignBrief* brief = static_cast<CampaignBrief*>(msg.m_window);
    int exitFlag = 0;

    if (g_campaignBriefReady) {
        g_campaignBriefFlashLeft = 6;
        g_campaignBriefReady = 0;
        g_campaignBriefFlashTime =
            GameTime::nextFrameTime(GameTime::get(), 125);
    }

    if (g_campaignBriefFlashLeft) {
        if (GameTime::isPast(g_campaignBriefFlashTime)) {
            g_campaignBriefFlashTime =
                GameTime::nextFrameTime(g_campaignBriefFlashTime, 125);
            --g_campaignBriefFlashLeft;

            for (int i = 0;
                 i < static_cast<int>(brief->m_campaign->m_scenarios.size());
                ++i) {
                if (brief->m_scenarios[i].m_available) {
                    if (g_campaignBriefFlashLeft & 1) {
                        brief->getWidget(
                            CampaignBrief::MAP_SELECTED_1_ID + i)->hide();
                        brief->getWidget(
                            CampaignBrief::MAP_ENABLED_1_ID + i)->hide();
                    } else {
                        brief->getWidget(
                            CampaignBrief::MAP_SELECTED_1_ID + i)->show();
                        brief->getWidget(
                            CampaignBrief::MAP_ENABLED_1_ID + i)->show();
                    }
                }
            }

            if (!g_campaignBriefFlashLeft)
                brief->select(brief->m_selectedScenario);
            else
                brief->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                                  WINDOW_ALL_WIDGETS_HIGH);
        }
        return 0;
    }

    if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT) {
        int id = msg.m_codeY;
        if (id == CampaignBrief::CHOICE_1_ID
            || id == CampaignBrief::CHOICE_2_ID
            || id == CampaignBrief::CHOICE_3_ID
            || id == CampaignBrief::CHOICE_1_HIGHLIGHT_ID
            || id == CampaignBrief::CHOICE_2_HIGHLIGHT_ID
            || id == CampaignBrief::CHOICE_3_HIGHLIGHT_ID
            || (id >= 235 && id <= 239)) {
            Widget* w = brief->getWidget(id);
            if (w) {
                normalDialog(w->getRclickText(), 4, -1, -1, -1, 0, -1, 0,
                             -1, 0, -1, 0);
            }
        } else {
            id = brief->m_zBuffer[msg.m_mouseY * brief->m_width + msg.m_mouseX];
            if (id && id > CampaignBrief::BACKGROUND_ID && id < 243) {
                int helpID = brief->convertID2HelpID(id);
                if (helpID >= 0) {
                    if (helpID < 100) {
                        std::string text =
                            brief->m_campaign->m_scenarios[helpID]
                                ->getRegionDescription();
                        normalDialog(text.c_str(), 4, -1, -1, -1, 0,
                                     -1, 0, -1, 0, -1, 0);
                    } else {
                        normalDialog(g_campaignBriefHelp[helpID].m_text,
                                     4, -1, -1, -1, 0, -1, 0,
                                     -1, 0, -1, 0);
                    }
                }
            }
        }
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_id != MESSAGE_WIDGET
        || msg.m_codeX != Widget::WIDGET_DESELECT)
        return MESSAGE_DISPATCH_CONSUME;

    switch (msg.m_codeY) {
    case CampaignBrief::RESTART_ID:
        exitFlag = 1;
        break;

    case CampaignBrief::VIDEO_ID:
        g_soundManager->stopAllSamples(1);
        g_game->m_campaign.playScenarioPrologue(brief->m_campaign);
        brief->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                          WINDOW_ALL_WIDGETS_HIGH);
        g_soundManager->stopAllSamples(1);
        break;

    case CampaignBrief::MAP_ENABLED_1_ID + 0:
    case CampaignBrief::MAP_ENABLED_1_ID + 1:
    case CampaignBrief::MAP_ENABLED_1_ID + 2:
    case CampaignBrief::MAP_ENABLED_1_ID + 3:
    case CampaignBrief::MAP_ENABLED_1_ID + 4:
    case CampaignBrief::MAP_ENABLED_1_ID + 5:
    case CampaignBrief::MAP_ENABLED_1_ID + 6:
    case CampaignBrief::MAP_ENABLED_1_ID + 7:
    case CampaignBrief::MAP_ENABLED_1_ID + 8:
    case CampaignBrief::MAP_ENABLED_1_ID + 9:
    case CampaignBrief::MAP_ENABLED_1_ID + 10:
    case CampaignBrief::MAP_ENABLED_1_ID + 11:
    case CampaignBrief::MAP_ENABLED_1_ID + 12:
    case CampaignBrief::MAP_ENABLED_1_ID + 13:
    case CampaignBrief::MAP_ENABLED_1_ID + 14:
    case CampaignBrief::MAP_ENABLED_1_ID + 15:
    case CampaignBrief::MAP_ENABLED_1_ID + 16:
    case CampaignBrief::MAP_ENABLED_1_ID + 17:
    case CampaignBrief::MAP_ENABLED_1_ID + 18:
    case CampaignBrief::MAP_ENABLED_1_ID + 19:
    case CampaignBrief::MAP_ENABLED_1_ID + 20:
    case CampaignBrief::MAP_ENABLED_1_ID + 21:
    case CampaignBrief::MAP_ENABLED_1_ID + 22:
    case CampaignBrief::MAP_ENABLED_1_ID + 23:
    case CampaignBrief::MAP_ENABLED_1_ID + 24:
    case CampaignBrief::MAP_ENABLED_1_ID + 25:
    case CampaignBrief::MAP_ENABLED_1_ID + 26:
    case CampaignBrief::MAP_ENABLED_1_ID + 27:
    case CampaignBrief::MAP_ENABLED_1_ID + 28:
    case CampaignBrief::MAP_ENABLED_1_ID + 29:
    case CampaignBrief::MAP_ENABLED_1_ID + 30:
    case CampaignBrief::MAP_ENABLED_1_ID + 31:
        if (!g_campaignBriefViewFromGame)
            brief->select(msg.m_codeY - CampaignBrief::MAP_ENABLED_1_ID);
        break;

    case CampaignBrief::MAP_SELECTED_1_ID + 0:
    case CampaignBrief::MAP_SELECTED_1_ID + 1:
    case CampaignBrief::MAP_SELECTED_1_ID + 2:
    case CampaignBrief::MAP_SELECTED_1_ID + 3:
    case CampaignBrief::MAP_SELECTED_1_ID + 4:
    case CampaignBrief::MAP_SELECTED_1_ID + 5:
    case CampaignBrief::MAP_SELECTED_1_ID + 6:
    case CampaignBrief::MAP_SELECTED_1_ID + 7:
    case CampaignBrief::MAP_SELECTED_1_ID + 8:
    case CampaignBrief::MAP_SELECTED_1_ID + 9:
    case CampaignBrief::MAP_SELECTED_1_ID + 10:
    case CampaignBrief::MAP_SELECTED_1_ID + 11:
    case CampaignBrief::MAP_SELECTED_1_ID + 12:
    case CampaignBrief::MAP_SELECTED_1_ID + 13:
    case CampaignBrief::MAP_SELECTED_1_ID + 14:
    case CampaignBrief::MAP_SELECTED_1_ID + 15:
    case CampaignBrief::MAP_SELECTED_1_ID + 16:
    case CampaignBrief::MAP_SELECTED_1_ID + 17:
    case CampaignBrief::MAP_SELECTED_1_ID + 18:
    case CampaignBrief::MAP_SELECTED_1_ID + 19:
    case CampaignBrief::MAP_SELECTED_1_ID + 20:
    case CampaignBrief::MAP_SELECTED_1_ID + 21:
    case CampaignBrief::MAP_SELECTED_1_ID + 22:
    case CampaignBrief::MAP_SELECTED_1_ID + 23:
    case CampaignBrief::MAP_SELECTED_1_ID + 24:
    case CampaignBrief::MAP_SELECTED_1_ID + 25:
    case CampaignBrief::MAP_SELECTED_1_ID + 26:
    case CampaignBrief::MAP_SELECTED_1_ID + 27:
    case CampaignBrief::MAP_SELECTED_1_ID + 28:
    case CampaignBrief::MAP_SELECTED_1_ID + 29:
    case CampaignBrief::MAP_SELECTED_1_ID + 30:
    case CampaignBrief::MAP_SELECTED_1_ID + 31:
        if (!g_campaignBriefViewFromGame)
            brief->select(msg.m_codeY - CampaignBrief::MAP_SELECTED_1_ID);
        break;

    case CampaignBrief::CHOICE_1_ID:
    case CampaignBrief::CHOICE_2_ID:
    case CampaignBrief::CHOICE_3_ID:
    case CampaignBrief::CHOICE_1_HIGHLIGHT_ID:
    case CampaignBrief::CHOICE_2_HIGHLIGHT_ID:
    case CampaignBrief::CHOICE_3_HIGHLIGHT_ID:
        if (!g_campaignBriefViewFromGame) {
            int choice = msg.m_codeY < CampaignBrief::CHOICE_1_HIGHLIGHT_ID
                             ? msg.m_codeY - CampaignBrief::CHOICE_1_ID
                             : msg.m_codeY
                                   - CampaignBrief::CHOICE_1_HIGHLIGHT_ID;
            g_game->m_campaign.m_briefingChoice = choice;
            for (int i = 0; i < 3; ++i) {
                brief->m_startBonusBorders[i]->setVisible(i == choice);
            }
            Widget* ok = brief->getWidget(DIALOG_RETURN_OK);
            if (ok)
                ok->enable(1);
            brief->updateAllyEnemyFlags();
            brief->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                              WINDOW_ALL_WIDGETS_HIGH);
        }
        break;

    case DIALOG_RETURN_CANCEL:
        exitFlag = 1;
        break;

    case DIALOG_RETURN_OK: {
        int selected = brief->m_selectedScenario;
        int choice = g_game->m_campaign.m_briefingChoice;
        int difficulty = g_game->m_setup.m_difficulty;

        g_game->m_campaign.m_currentMap = static_cast<signed char>(selected);
        if (g_game->m_campaign.m_currentCampaign != GAME_CAMPAIGN_2
            || selected != GAME_SCENARIO_2)
            g_game->m_campaign.playScenarioPrologue(brief->m_campaign);

        showProgressBar();
        incProgressBar(1);

        NewSMapHeader mapHeader;
        brief->m_campaign->loadScenario(selected, &mapHeader);
        g_game->resetGame(difficulty, selected, &mapHeader);
        g_game->m_campaign.applyBriefingChoice(choice);
        memset(g_newMapStartingBonus, 3, sizeof(g_newMapStartingBonus));
        incProgressBar(1);

        int gamePos = brief->m_campaign->m_scenarios[selected]
                          ->m_options->getPlayer(choice);
        strcpy(g_game->m_players[gamePos].m_name, g_localPlayerName);
        g_localGamePos = gamePos;
        brief->m_campaign->startScenario(selected, choice);
        incProgressBar(1);
        g_soundManager->stopMP3();
        incProgressBar(1);
        exitFlag = 1;
        break;
    }
    }

    if (exitFlag) {
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = msg.m_codeY;
        msg.m_codeY = Widget::WIDGET_END_DIALOG;
        msg.m_codeX = Widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x0045bad0, 0x134)  // sole caller CampaignBriefHandler + member offset
std::string CampaignBrief::ScenarioStruct::getRegionDescription() const
{
    return m_regionDesc;
}

VA(0x0045bc10, 0xC0)  // dc 0x5ab48
std::string getCampaignName()
{
    const char* fileName = g_game->m_campaign.getCampaignFileName().c_str();
    CampaignBrief::CampaignHeaderStruct* header =
        new CampaignBrief::CampaignHeaderStruct(fileName);

    header->load();
    return header->getCampaignName();
}

#if 0  // Remaining Dreamcast-only carcass.

// E:\gamedcs\campaignbrief.cpp:1409
DC_ONLY(0x5ab84, 0x60)
void ReadRamDisc(int RamDiscNr, void* buffer, long size, unsigned long* bytesRead)
{
    // @stub
}

// E:\gamedcs\widget.h:236
DC_ONLY(0x5abe4, 0x10)
const char* Widget::getRclickText()
{
    // @stub
}

// E:\gamedcs\widget.h:251
DC_ONLY(0x5abf4, 0x24)
void Widget::hide()
{
    // @stub
}

// E:\gamedcs\widget.h:257
DC_ONLY(0x5ac18, 0x24)
void Widget::show()
{
    // @stub
}

// E:\gamedcs\game.h:234
DC_ONLY(0x5ac3c, 0x44)
void CMapHeaderData::PlayerSlotAttributes::PlayerSlotAttributes()
{
    // @stub
}

// E:\gamedcs\game.h:287
DC_ONLY(0x5ac80, 0x70)
void NewSMapHeader::NewSMapHeader()
{
    // @stub
}

// E:\gamedcs\game.h:308
DC_ONLY(0x5acf0, 0x28)
void CMapHeaderData::CMapHeaderData()
{
    // @stub
}

// E:\gamedcs\game.h:546
DC_ONLY(0x5ad18, 0xB8)
void SGameSetupOptions::SGameSetupOptions()
{
    // @stub
}

// E:\gamedcs\SingleSelectionPopups.h:70
DC_ONLY(0x5add0, 0x18)
void CSingleSelPopup::~CSingleSelPopup()
{
    // @stub
}

// E:\gamedcs\campaignbrief.cpp:192
DC_ONLY(0x5ade8, 0x28)
void CampaignBrief::CampaignHeaderStruct::~CampaignHeaderStruct()
{
    // @stub
}

// E:\gamedcs\campaignbrief.cpp:192
DC_ONLY(0x5ae10, 0x44)
void CampaignBrief::CampaignHeaderStruct::CampaignHeaderStruct()
{
    // @stub
}

// E:\gamedcs\campaignbrief.cpp:192
DC_ONLY(0x5ae54, 0x2C)
void NewSMapHeader::~NewSMapHeader()
{
    // @stub
}

// E:\gamedcs\campaignbrief.cpp:1007
DC_ONLY(0x5ae80, 0x34)
void* CampaignBrief::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\campaignbrief.cpp:1007
DC_ONLY(0x5aeb4, 0x64)
NewSMapHeader* NewSMapHeader::operator=(const NewSMapHeader* __that)
{
    // @stub
}

// E:\gamedcs\campaignbrief.cpp:1050
DC_ONLY(0x5af18, 0x34)
void* Game::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\campaignbrief.cpp:1391
DC_ONLY(0x5af4c, 0x18)
void CHeroDlg::~CHeroDlg()
{
    // @stub
}
#endif

// COMDAT pairing: the unit's own std::_Construct<type_map_hero_identity> COMDAT,
// mnemonic agreement 1.000 over all 142 instructions.
VA_COMPGEN(0x0045d420, 0x170, STD_CONSTRUCT, MapHeroIdentity)

VA_COMPGEN(0x0045d700, 0x1D6, STD_CONSTRUCT, CampaignScenarioPreview)

// COMDAT pairing: _Tree<int, type_map_hero_info>::erase(first, last), 0.961.
VA_COMPGEN(0x0045c070, 0x12B, TREE_ERASE_RANGE, MapHeroInfo)

// COMDAT pairing: std::_Construct<pair<const int, type_map_hero_info>>, 0.993
// against a 368 B object; newgame's two candidates are 347 and 343 B.
VA_COMPGEN(0x0045d590, 0x16E, STD_CONSTRUCT, type_map_hero_info_pair)

// COMDAT pairing: basic_string<char>::_Refcnt, 15 B against this
// compiland's single 15-byte COMDAT. Declarator form: _demangle_key keys
// the basic_string helpers flat, with no owner arm for a compgen kind.
#if 0  // @carcass: Dinkumware instantiations emitted by this compiland

VA(0x0045c1d0, 0xB)  // COMDAT pairing (unique 11 B in this obj)
unsigned char& std::basic_string<char>::_Refcnt(const char* s)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x0045c1e0, 0x13, ALLOCATOR_DEALLOCATE, char)
VA_COMPGEN(0x0045dca0, 0x6, BASIC_STRING_NULLSTR, char)

VA_COMPGEN(0x0045da70, 0x21B, IMPLICIT_COPY_CTOR, PlayerSlotAttributes)

#if 0  // @carcass: Dinkumware instantiations emitted by this compiland

VA(0x0045dc90, 0xF)  // COMDAT pairing (unique 15 B in this obj)VA(0x0045dc90, 0xF)  // COMDAT pairing (unique 15 B in this obj)
void std::char_traits<char>::assign(char& to, const char& from)
{
    // @stub
}

#endif  // @carcass

// COMDAT pairing: map<int, type_map_hero_info>'s COPY constructor, the second
// half of the two-member ctor group whose first half is already claimed at
// 0x45bf70 (190 B, the less+allocator form). 0x45dcb0 calls _Tree::_Copy and
// _Construct<pair<const int, type_map_hero_info>>, which only the copy form
// does, and RVA order matches COFF order in both arms.
VA_COMPGEN(0x0045dcb0, 0x1E5, CLASS_CTOR, map)

// COMDAT pairing: _Tree<int, type_map_hero_info>::_Buynode - reached from the
// copy ctor above and from seg_0019, and campaignbrief.obj emits exactly one.
VA_COMPGEN(0x0045dea0, 0x1D, TREE_BUYNODE, MapHeroInfo)

// COMDAT pairing: vector<type_map_hero_identity>::_Ucopy (thiscall, three
// pointer arguments, `ret 0xc`).
VA_COMPGEN(0x0045d230, 0x38, VECTOR_UCOPY, MapHeroIdentity)

// Original constructor family: campaignbrief.cpp:192, dc 0x5ae10.
// Complete adds the filename argument and changes the record's members;
// keep its retained body in the same owning module as the destructor.
// Retail reads filename at 0x488636, copies it into the string at +4,
// clears data/stream/status at 0x488681..0x488687, and returns with ret 4.
// The four members default-construct (the empty
VA(0x004885d0, 0xCB)  // anchor-caller(TCampaignBrief ctor), retail-only
CampaignBrief::CampaignHeaderStruct::CampaignHeaderStruct(
    const char* filename)
{
    m_fileName = filename;
    m_data = 0;
    m_stream = 0;
    m_fileError = CAMPAIGN_FILE_OK;
}

// campaignbrief.cpp:192, dc 0x5ade8. Complete expands the record and its
// cleanup; preserve that retail body in the original owning module.
// Retail deletes every scenario record (null-checked by
// `delete`), calls vector<ScenarioStruct*>::erase(begin, end) out of line
// (the retail label game_1fd60_sub02_14cdb0 at 0x54cdb0 is that COMDAT,
// i.e. a `scenarios.clear()`), calls FreeData, then destroys scenarios,
// campaign_desc, campaign_name and file_name in reverse order.

// MAX 78.5339; current canonical clear() body 75.72034. Retail calls
// both vector::erase and FreeData; candidate expands them and leaves
// vector::_Destroy called from erase. Keep clear() as the source boundary.
// The 2026-09-07 passive trace corrects the old small-free-class diagnosis:
// FreeData's C2 cost is 101, not <=40, and its site has budget 752 after
// clear/erase. The state gate allows it (body flags 0x8000, callee 0x68).
// Clear's nested erase costs 69 against budget 144; its _Destroy costs 49
// against 29 and stays called. These are ordinary measured budget decisions,
// not proof that caller-side source structure can never affect the frontier.
// Earlier artificial free/charged-site controls were byte-inert; do not
// repeat them or use them to infer the helper's cost. Natural loop controls:
// signed index is byte-identical at 75.72034; naming the scenarios vector
// by reference gives 67.27966. Neither changes the retained source choice.
// E:\gamedcs\campaignbrief.cpp:192, dc 0x5ade8
VA(0x004886a0, 0x132)  // anchor-caller(TCampaignBrief ctor), retail-only
CampaignBrief::CampaignHeaderStruct::~CampaignHeaderStruct()
{
    for (unsigned int i = 0; i < m_scenarios.size(); ++i)
        delete m_scenarios[i];
    m_scenarios.clear();
    freeData();
}

// This delete loop naturally retains ScenarioStruct's compiler-generated
// deleting wrapper. Retail CampaignHeaderStruct::load and selectCampaign
// call the shared 0x488eb0 copy. All 33 bytes and both calls agree: the
// ordinary destructor stays at 0x485fe0 in customcampaign, then flags&1
// gates operator delete. Move only the enrollment from the inlining consumer.
VA_COMPGEN(0x00488eb0, 0x21, SCALAR_DELETING_DTOR, ScenarioStruct)
