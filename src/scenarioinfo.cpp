#include "va.h"
#include "text.h"

#include <stdio.h>

#include "scenarioinfo.h"

#include "advmgr.h"
#include "bitmap816.h"
#include "border.h"
#include "button.h"
#include "csprite.h"
#include "font.h"
#include "game.h"
#include "hero.h"
#include "iconwdgt.h"
#include "kb.h"
#include "message.h"
#include "resourcemanager.h"
#include "singleselectionpopups.h"
#include "singleselectionwindow.h"
#include "slider.h"
#include "textresource.h"
#include "textscroller.h"
#include "textwdgt.h"
#include "widget.h"
#include "window.h"
#include "winmgr.h"

// Complete adds this compact row renderer to the scenario-info dialog. It
// has no Dreamcast counterpart, so the class name is role-derived; its base,
// complete 0x60-byte layout, vtable shape, and owned portrait are retail
// facts from 0x5680fd..0x56822c and 0x5693a0..0x5697c4. The class is
// private to this dialog module; no original header location is established.
class CScenarioPlayerInfoWidget : public widget {
public:
    Bitmap816* m_panel;                 // +0x30
    Bitmap816* m_flag;                  // +0x34
    CSprite* m_townSprite;              // +0x38
    int m_townType;                     // +0x3c
    const char* m_playerName;           // +0x40
    const char* m_handicapText;         // +0x44
    const char* m_playerTypeText;       // +0x48
    int m_playerPosition;               // +0x4c
    int m_startingBonus;                // +0x50
    CSprite* m_bonusSprite;             // +0x54
    Bitmap816* m_heroPortrait;          // +0x58, owned
    hero* m_startingHero;               // +0x5c

    // Retail's inlined constructor ends with `mov word ptr [edi+0x10], dx`
    // (0x568160) - the widget id, `390 + playerPosition`, which is exactly
    // the row ProcessRightSelect fetches back with GetWidget to reach
    // heroPortrait.  The store sits INSIDE the new-expression's
    // allocation-succeeded arm, so it is the constructor's, not the caller's.
    CScenarioPlayerInfoWidget(CSprite* town, int widgetId)
    {
        m_townSprite = town;
        m_townType = 0;
        m_panel = 0;
        m_flag = 0;
        m_playerName = 0;
        m_handicapText = 0;
        m_playerTypeText = 0;
        m_playerPosition = 0;
        m_bonusSprite = 0;
        m_heroPortrait = 0;
        m_startingBonus = 4;
        m_startingHero = 0;
        m_id = widgetId;
    }

    virtual ~CScenarioPlayerInfoWidget();
    virtual int main(message& msg) { return widget::main(msg); }
    virtual void zBufferDraw(unsigned short*, int) const {}
    virtual void draw() const;
};
SIZE(CScenarioPlayerInfoWidget, 0x60);

// E:\gamedcs\scenarioinfo.cpp:258, dc 0x129db4
VA(0x00567290, 0x2109)  // anchor CAdvPopup ctor + GSelPop1.pcx + DC source shape, dc 0x129db4
CScenarioInfoDlg::CScenarioInfoDlg()
    // DC 258 centers against SCREEN_WIDTH/HEIGHT; retail +0x2f/+0x31
    // pushes y=7 then x=18 for the fixed 800x600 screen.
    : CAdvPopup((WINDOW_SCREEN_WIDTH - 763) / 2,
                (WINDOW_SCREEN_HEIGHT - 585) / 2, 763, 585, 2)
{
    char tempText[256];
    char lossText[1024];
    char tempName[256];
    char victoryText[1024];

    // The widget vector NAMED AS A REFERENCE across all 42 uses:
    // 96.3650 -> 96.5654.  Retail reads _First/_Last through the vector's
    // own address here, not folded off `this`.
    std::vector<widget*>& widgets = m_widgets;
    widgets.reserve(100);

    widgets.push_back(new bitmapBorder(
        393, 0, 370, 585, 100, "GSelPop1.pcx", 0x800));
    widgets.push_back(new bitmapBorder(
        0, 0, 557, 585, 102, "AdvOptBk.pcx", 0x800));

    sprintf(g_text, "%s:", g_generalText->getText(GENERAL_TEXT_SCENARIO_PLAYER_DIFFICULTY));
    widgets.push_back(new textWidget(
        411, 429, 334, 19, g_text, "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 132,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    sprintf(g_text, "%s:", g_generalText->getText(GENERAL_TEXT_RATING));
    widgets.push_back(new textWidget(
        662, 429, 84, 19, g_text, "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 133,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new textWidget(
        411, 429, 90, 19, g_generalText->getText(GENERAL_TEXT_SCENARIO_MAP_DIFFICULTY_LABEL), "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 134,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new textWidget(
        419, 21, 278, 18, g_generalText->getText(GENERAL_TEXT_SCENARIO_NAME_LABEL), "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 100, font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new textWidget(
        419, 131, 278, 18, g_generalText->getText(GENERAL_TEXT_SCENARIO_DESCRIPTION_LABEL), "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 105, font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new textWidget(
        419, 282, 278, 18, g_generalText->getText(GENERAL_TEXT_SCENARIO_VICTORY_CONDITION_LABEL), "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 100, font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new textWidget(
        419, 338, 278, 18, g_generalText->getText(GENERAL_TEXT_SCENARIO_LOSS_CONDITION_LABEL), "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 100, font::VERT_CENTER_JUSTIFIED, 0, 8));

    iconWidget* mapSizeIcon = new iconWidget(
        711, 22, 29, 23, 189, "scnrmpsz.def", 0, 0, 0, 0,
        iconWidget::ICON_STYLE_PLAIN);
    widgets.push_back(mapSizeIcon);

    sprintf(g_text, "%s:", g_generalText->getText(GENERAL_TEXT_ALLIES));
    widgets.push_back(new textWidget(
        411, 397, 44, 23, g_text, "smalfont.fnt", font::WHITE, 100,
        font::VERT_CENTER_JUSTIFIED | font::RIGHT_JUSTIFIED, 0, 8));
    sprintf(g_text, "%s:", g_generalText->getText(GENERAL_TEXT_ENEMIES));
    widgets.push_back(new textWidget(
        576, 397, 58, 23, g_text, "smalfont.fnt", font::WHITE, 386,
        font::VERT_CENTER_JUSTIFIED | font::RIGHT_JUSTIFIED, 0, 8));

    NewSMapHeader& mapHeader = g_game->m_mapHeader;
    // DC locals and lines 299..300 prove pointers, with initialization
    // before the map-name widget. Retail keeps both addresses until icons.
    VictoryConditionStruct* vc = &mapHeader.m_victoryCondition;
    LossConditionStruct* lc = &mapHeader.m_lossCondition;

    widgets.push_back(new textWidget(
        419, 39, 324, 30, mapHeader.m_mapName.c_str(),
        "bigfont.fnt", font::HEADING_HIGHLIGHT, 100, 0, 0, 8));
    widgets.push_back(new CScrollTextWidget(
        mapHeader.m_mapDescription.c_str(), 419, 149, 319, 115,
        "smalfont.fnt", font::WHITE, slider::BLUE));

    // Retail +0x6c0/+0x743 passes justify=5 for the difficulty labels;
    // +0x7c3/+0x822 passes justify=4 for the victory/loss descriptions.
    widgets.push_back(new textWidget(
        411, 448, 89, 48,
        g_difficulty[mapHeader.m_difficulty], "smalfont.fnt",
        font::WHITE, 100,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    sprintf(tempText, "%d%%",
            g_difficultyRatingPercent[g_game->m_setup.m_difficulty]);
    widgets.push_back(new textWidget(
        663, 448, 83, 48, tempText, "smalfont.fnt", font::WHITE, 100,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));

    g_game->getVictoryConditionText(victoryText);
    g_game->getLossConditionText(lossText);
    widgets.push_back(new textWidget(
        453, 299, 288, 32, victoryText, "smalfont.fnt", font::WHITE, 100,
        font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new textWidget(
        453, 358, 288, 32, lossText, "smalfont.fnt", font::WHITE, 100,
        font::VERT_CENTER_JUSTIFIED, 0, 8));

    int i;
    for (i = 0; i < 8; ++i) {
        widgets.push_back(new iconWidget(
            457 + i * 15, 399, 15, 20, 112 + i, "itgflags.def",
            0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
        widgets.push_back(new iconWidget(
            637 + i * 15, 399, 15, 20, 120 + i, "itgflags.def",
            0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
    }

    widgets.push_back(new CHotspotWidget(453, 396, 310, 25, 387));
    widgets.push_back(new textWidget(
        55, 84, 104, 36, g_generalText->getText(GENERAL_TEXT_PLAYER_NAME_HANDICAP_HEADER), "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 339,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));

    int gameTypeId = 341;
    if (*g_videoGameState == SINGLE_SELECTION_CONTEXT_1
            || *g_videoGameState == SINGLE_SELECTION_CONTEXT_3)
        gameTypeId = 342;
    widgets.push_back(new textWidget(
        160, 84, 75, 36, g_generalText->getText(GENERAL_TEXT_STARTING_TOWN_HEADER), "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, gameTypeId,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new textWidget(
        236, 84, 75, 36, g_generalText->getText(GENERAL_TEXT_STARTING_HERO_HEADER), "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 343,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new textWidget(
        312, 84, 75, 36, g_generalText->getText(GENERAL_TEXT_STARTING_BONUS_HEADER), "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 344,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new textWidget(
        55, 528, 334, 20, g_generalText->getText(GENERAL_TEXT_PLAYER_TURN_DURATION_HEADER), "smalfont.fnt",
        font::PRIMARY_HIGHLIGHT, 340,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new button(
        581, 529, 166, 40, SCENARIO_INFO_ACCEPT_ID, "scnrback.def",
        0, 1, 0, 1, 2));

    m_victoryIcon = ResourceManager::getSprite("scnrvict.def");
    m_lossIcon = ResourceManager::getSprite("scnrloss.def");
    m_townPix = ResourceManager::getSprite("itpa.def");
    m_bonusSprite = ResourceManager::getSprite("ScnrStar.def");

    const char* colorChars = "rbygopts";
    for (i = 0; i < 8; ++i) {
        sprintf(tempName, "adop%cpnl.pcx", colorChars[i]);
        m_panels[i] = ResourceManager::getBitmap816(tempName);
        sprintf(tempName, "adopflg%c.pcx", colorChars[i]);
        m_flags[i] = ResourceManager::getBitmap816(tempName);
    }

    slider* durationSlider = new slider(
        55, 551, 194, 16, 338, 11, 0, slider::BLUE, 0, 0);
    durationSlider->setState(g_game->m_setup.m_turnDuration);
    widgets.push_back(durationSlider);

    widgets.push_back(new textWidget(
        55, 18, 334, 59, g_generalText->getText(GENERAL_TEXT_SCENARIO_INFORMATION), "bigfont.fnt",
        font::HEADING_HIGHLIGHT, -1,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    widgets.push_back(new textWidget(
        253, 550, 134, 18, g_turnDurationText[g_game->m_setup.m_turnDuration],
        "smalfont.fnt", font::WHITE, -1,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));

    // Complete-only rows are absent from DC's body in the 366..412 gap;
    // that gap does not recover their source or explain their omission.
    // Retail +0x1436..+0x1457 increments the display index and y only after
    // adding an active row; +0x1457 advances the player index even on skip.
    // Widget IDs retain i so ProcessRightSelect can find the owning player.
    // Retail +0xf86 reads hero traits +0x30 (small portrait), not +0x34.
    int rowPosition = 0;
    for (i = 0; i < 8; ++i) {
        if (g_game->m_setup.m_playerPos[i] < 0)
            continue;

        int playerType;
        CMapHeaderData::TPlayerSlotAttributes* slot =
            &g_game->m_mapHeader.m_playerSlotAttributes[i];
        if (slot->m_canBeHuman)
            playerType = slot->m_canBeComputer ? 0 : 1;
        else if (slot->m_canBeComputer)
            playerType = 2;
        else
            continue;

        hero* startingHero = g_game->getHero(g_game->m_setup.m_startingHero[i]);
        CScenarioPlayerInfoWidget* row = new CScenarioPlayerInfoWidget(
            m_townPix, i + SCENARIO_INFO_PLAYER_ROW_FIRST_ID);
        row->m_panel = m_panels[i];
        row->m_flag = m_flags[i];
        row->m_townType = g_game->m_setup.m_alignment[i];
        row->m_playerName = g_game->m_players[i].m_name;
        row->m_handicapText = g_handiText[g_game->m_setup.m_handicap[i]];
        row->m_playerTypeText = g_humanCpu[playerType];
        row->m_playerPosition = rowPosition;
        row->m_startingBonus = g_game->m_setup.m_startingBonus[i];
        row->m_bonusSprite = m_bonusSprite;
        row->m_startingHero = startingHero;
        row->m_heroPortrait = ResourceManager::getBitmap816(
            startingHero
                ? g_heroTraits[startingHero->m_portrait].m_smallPortraitName
                : "hpsrand6.pcx");
        widgets.push_back(row);

        int y = 124 + rowPosition * 50;
        widgets.push_back(new CHotspotWidget(173, y, 48, 32, 370 + i));
        widgets.push_back(new CHotspotWidget(249, y, 48, 32, 362 + i));
        widgets.push_back(new CHotspotWidget(325, y, 48, 32, 378 + i));
        ++rowPosition;
    }

    iconWidget* victory = new iconWidget(
        417, 302, 32, 24, -1, "scnrvict.def", 0, 0, 0, 0,
        iconWidget::ICON_STYLE_PLAIN);
    if (vc->m_type >= 0)
        victory->setIconFrame(vc->m_type);
    else
        victory->setIconFrame(11);
    widgets.push_back(victory);

    iconWidget* loss = new iconWidget(
        417, 359, 32, 24, -1, "scnrloss.def", 0, 0, 0, 0,
        iconWidget::ICON_STYLE_PLAIN);
    if (lc->m_type >= 0)
        loss->setIconFrame(lc->m_type);
    else
        loss->setIconFrame(3);
    widgets.push_back(loss);

    widgets.push_back(new button(
        503, 450, 30, 46, 107, "gspbut3.def", 0, 1, 0, 0, 2));
    widgets.push_back(new button(
        535, 450, 30, 46, 108, "gspbut4.def", 0, 1, 0, 0, 2));
    widgets.push_back(new button(
        567, 450, 30, 46, 109, "gspbut5.def", 0, 1, 0, 0, 2));
    widgets.push_back(new button(
        599, 450, 30, 46, 110, "gspbut6.def", 0, 1, 0, 0, 2));
    widgets.push_back(new button(
        631, 450, 30, 46, 111, "gspbut7.def", 0, 1, 0, 0, 2));

    addWidgetsToMessageStream();
    updateAllyEnemyFlags();

    int mapSizeFrame;
    switch (g_game->m_mapHeader.m_size) {
    case MAP_DIMENSION_SMALL:       mapSizeFrame = 0; break;
    case MAP_DIMENSION_MEDIUM:      mapSizeFrame = 1; break;
    case MAP_DIMENSION_LARGE:       mapSizeFrame = 2; break;
    case MAP_DIMENSION_EXTRA_LARGE: mapSizeFrame = 3; break;
    default:  mapSizeFrame = 4; break;
    }
    mapSizeIcon->setIconFrame(mapSizeFrame);

    // SOURCE-SHAPE RATCHET: Dreamcast scenarioinfo.cpp:661 retains this
    // ordinary member boundary; retail expands its five-widget/message body
    // here. Keep the declaration non-inline and let VC6 choose this caller.
    setDifficultyHiLite();
    setHelpText(g_singleSelectionHelp, 104, 345, 0);
    // Retail +0xccd saves the duration slider in [ebp-0x10]; +0x205c
    // reloads that same object for Enable(false), after SetHelpText.
    durationSlider->enable(0);
    m_heroSpecificAbility = ResourceManager::getSprite("un44.def");
}

VA(0x005693a0, 0x394)
void CScenarioPlayerInfoWidget::draw() const
{
    int windowX = m_parentWindow->m_x;
    int windowY = m_parentWindow->m_y;

    m_panel->draw(0, 0, m_panel->getWidth(), m_panel->getHeight(),
                g_windowManager->m_screenBitmap,
                windowX + 54, windowY + m_playerPosition * 50 + 122, 1);
    m_flag->draw(0, 0, m_flag->getWidth(), m_flag->getHeight(),
               g_windowManager->m_screenBitmap,
               windowX + 11, windowY + m_playerPosition * 50 + 124, 1);

    g_smallFont->drawBoundedString(
        m_playerName, g_windowManager->m_screenBitmap,
        windowX + 59, windowY + m_playerPosition * 50 + 124,
        97, 17, font::PRIMARY, font::CENTER_JUSTIFIED, -1);
    g_smallFont->drawBoundedString(
        m_playerTypeText, g_windowManager->m_screenBitmap,
        windowX + 59, windowY + m_playerPosition * 50 + 145,
        46, 24, font::WHITE,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    if (g_game->isMultiplayer()) {
        g_smallFont->drawBoundedString(
            m_handicapText, g_windowManager->m_screenBitmap,
            windowX + 107, windowY + m_playerPosition * 50 + 145,
            50, 24, font::WHITE,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    }

    m_townSprite->draw(0, m_townType * 2 + 2, 0, 0,
                     m_townSprite->getWidth(), m_townSprite->getHeight(),
                     g_windowManager->m_screenBitmap,
                     windowX + 173, windowY + m_playerPosition * 50 + 124,
                     0, 1);
    g_smallFont->drawBoundedString(
        g_townTypeNames[m_townType + 1], g_windowManager->m_screenBitmap,
        windowX + 161, windowY + m_playerPosition * 50 + 156,
        71, 16, font::WHITE,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);

    if (m_heroPortrait) {
        m_heroPortrait->draw(0, 0, m_heroPortrait->getWidth(), m_heroPortrait->getHeight(),
                           g_windowManager->m_screenBitmap,
                           windowX + 249,
                           windowY + m_playerPosition * 50 + 124, 0);
    }
    if (m_startingHero) {
        g_smallFont->drawBoundedString(
            m_startingHero->m_name, g_windowManager->m_screenBitmap,
            windowX + 237, windowY + m_playerPosition * 50 + 156,
            71, 16, font::WHITE,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    } else {
        g_smallFont->drawBoundedString(
            g_generalText->getText(GENERAL_TEXT_NO_HERO), g_windowManager->m_screenBitmap,
            windowX + 237, windowY + m_playerPosition * 50 + 156,
            71, 16, font::WHITE,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    }

    int bonusFrame = 10;
    switch (m_startingBonus) {
    case NEW_MAP_BONUS_ARTIFACT:
        bonusFrame = 9;
        break;
    case NEW_MAP_BONUS_GOLD:
        bonusFrame = 8;
        break;
    case NEW_MAP_BONUS_RESOURCE:
        bonusFrame = m_townType == TOWN_CONFLUX ? 3 : m_townType;
        break;
    case NEW_MAP_BONUS_RANDOM:
    case NEW_MAP_BONUS_NONE:
        bonusFrame = 10;
        break;
    }
    m_bonusSprite->draw(0, bonusFrame, 0, 0,
                      m_bonusSprite->getWidth(), m_bonusSprite->getHeight(),
                      g_windowManager->m_screenBitmap,
                      windowX + 325, windowY + m_playerPosition * 50 + 124,
                      0, 1);

    if (m_startingBonus == NEW_MAP_BONUS_RANDOM) {
        g_smallFont->drawBoundedString(
            g_generalText->getText(GENERAL_TEXT_RANDOM_HERO), g_windowManager->m_screenBitmap,
            windowX + 313, windowY + m_playerPosition * 50 + 156,
            71, 16, font::WHITE,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    } else {
        g_smallFont->drawBoundedString(
            g_agrText[m_startingBonus], g_windowManager->m_screenBitmap,
            windowX + 313, windowY + m_playerPosition * 50 + 156,
            71, 16, font::WHITE,
            font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, -1);
    }
}

// Slot 0 of the Complete-only vtable 0x6416dc.
VA_COMPGEN(0x00569740, 0x21, SCALAR_DELETING_DTOR,
           CScenarioPlayerInfoWidget)

VA(0x00569770, 0x55)
CScenarioPlayerInfoWidget::~CScenarioPlayerInfoWidget()
{
    if (m_heroPortrait)
        m_heroPortrait->dispose();
}

VA_COMPGEN(0x005697d0, 0x21, SCALAR_DELETING_DTOR, CScenarioInfoDlg)

VA(0x00569800, 0x98)  // dc 0x12aa70
CScenarioInfoDlg::~CScenarioInfoDlg()
{
    m_victoryIcon->dispose();
    m_lossIcon->dispose();
    m_townPix->dispose();
    m_bonusSprite->dispose();
    m_heroSpecificAbility->dispose();

    for (int i = 0; i < 8; ++i) {
        m_panels[i]->dispose();
        m_flags[i]->dispose();
    }
}

VA(0x005698a0, 0x1E)  // dc 0x12ab00
int CScenarioInfoDlg::onWidgetDeselect(int id, bool& exitFlag)
{
    if (id == SCENARIO_INFO_ACCEPT_ID)
        exitFlag = 1;
    return CHeroWindowEx::onWidgetDeselect(id, exitFlag);
}

VA(0x005698c0, 0xF2)  // dc 0x12ab20
void CScenarioInfoDlg::updateAllyEnemyFlags()
{
    int localPlayer = g_game->getLocalPlayerGamePos();
    widget* flag;
    int i = 0;
    int nextEnemy = SCENARIO_INFO_ENEMY_FIRST_ID;
    int nextAlly = SCENARIO_INFO_ALLY_FIRST_ID;

    for (; i < 8; ++i) {
        flag = getWidget(i + SCENARIO_INFO_ENEMY_FIRST_ID);
        flag->sendMessage(widget::WIDGET_CLEAR_STATUS,
                           widget::WIDGET_CLEAR_STATUS);
        flag = getWidget(i + SCENARIO_INFO_ALLY_FIRST_ID);
        flag->sendMessage(widget::WIDGET_CLEAR_STATUS,
                           widget::WIDGET_CLEAR_STATUS);

        if (g_game->m_setup.m_playerPos[i] >= 0) {
            if (g_game->onSameTeam(i, localPlayer)) {
                flag = getWidget(nextAlly);
                flag->sendMessage(widget::WIDGET_SET_STATUS,
                                   widget::WIDGET_CLEAR_STATUS);
                getWidget(nextAlly)->sendMessage(
                    widget::WIDGET_SET_ICON_FRAME, i);
                ++nextAlly;
            } else {
                flag = getWidget(nextEnemy);
                flag->sendMessage(widget::WIDGET_SET_STATUS,
                                   widget::WIDGET_CLEAR_STATUS);
                getWidget(nextEnemy)->sendMessage(
                    widget::WIDGET_SET_ICON_FRAME, i);
                ++nextEnemy;
            }
        }
    }
}

// E:\\gamedcs\\scenarioinfo.cpp:661
// Dreamcast keeps this as an ordinary out-of-line member at dc 0x12af90.
// Complete's ctor at 0x567290 expands the same five-widget/message sequence,
// so the declaration remains non-inline and VC6 /Ob2 owns the caller choice.
void CScenarioInfoDlg::setDifficultyHiLite()
{
    message msg;

    for (int i = 107; i <= 111; ++i)
        getWidget(i)->enable(0);

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_STATUS;
    msg.m_codeY = g_game->m_setup.m_difficulty + 107;
    msg.m_extra = widget::WIDGET_HIGHLIGHTED;
    broadcastMessage(msg);
}

VA(0x005699C0, 0x320)  // dc 0x12ac28
unsigned char CScenarioInfoDlg::processRightSelect(int id)
{
    switch (id) {
    case SCENARIO_INFO_TEAM_ID: {
        CTeamAlignmentDlg dlg(0);
        dlg.createWin();
        dlg.doModal(0);
        return 1;
    }

    case SCENARIO_INFO_TOWN_FIRST_ID:
    case SCENARIO_INFO_TOWN_FIRST_ID + 1:
    case SCENARIO_INFO_TOWN_FIRST_ID + 2:
    case SCENARIO_INFO_TOWN_FIRST_ID + 3:
    case SCENARIO_INFO_TOWN_FIRST_ID + 4:
    case SCENARIO_INFO_TOWN_FIRST_ID + 5:
    case SCENARIO_INFO_TOWN_FIRST_ID + 6:
    case SCENARIO_INFO_TOWN_FIRST_ID + 7: {
        int townType =
            g_game->m_setup.m_alignment[id - SCENARIO_INFO_TOWN_FIRST_ID];
        if (townType != -1) {
            CTownDlg dlg(0);
            dlg.createWin(
                m_townPix, townType * 2 + 2,
                static_cast<TTownType>(townType) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
            dlg.doModal(0);
        }
        return 1;
    }

    case SCENARIO_INFO_HERO_FIRST_ID:
    case SCENARIO_INFO_HERO_FIRST_ID + 1:
    case SCENARIO_INFO_HERO_FIRST_ID + 2:
    case SCENARIO_INFO_HERO_FIRST_ID + 3:
    case SCENARIO_INFO_HERO_FIRST_ID + 4:
    case SCENARIO_INFO_HERO_FIRST_ID + 5:
    case SCENARIO_INFO_HERO_FIRST_ID + 6:
    case SCENARIO_INFO_HERO_FIRST_ID + 7: {
        int playerPosition = id - SCENARIO_INFO_HERO_FIRST_ID;
        int heroId = g_game->m_setup.m_startingHero[playerPosition];
        if (heroId != -1) {
            CScenarioPlayerInfoWidget* row =
                static_cast<CScenarioPlayerInfoWidget*>(getWidget(
                    playerPosition + SCENARIO_INFO_PLAYER_ROW_FIRST_ID));
            hero* startingHero = g_game->getHero(heroId);
            CHeroDlg dlg(0);
            dlg.createWin(row->m_heroPortrait, startingHero->m_name,
                          m_heroSpecificAbility, heroId,
                          startingHero->getSpecificAbilityTextShort(),
                          startingHero->heroFn004D8F70());
            dlg.doModal(0);
        }
        return 1;
    }

    case SCENARIO_INFO_BONUS_FIRST_ID:
    case SCENARIO_INFO_BONUS_FIRST_ID + 1:
    case SCENARIO_INFO_BONUS_FIRST_ID + 2:
    case SCENARIO_INFO_BONUS_FIRST_ID + 3:
    case SCENARIO_INFO_BONUS_FIRST_ID + 4:
    case SCENARIO_INFO_BONUS_FIRST_ID + 5:
    case SCENARIO_INFO_BONUS_FIRST_ID + 6:
    case SCENARIO_INFO_BONUS_FIRST_ID + 7: {
        int playerPosition = id - SCENARIO_INFO_BONUS_FIRST_ID;
        CBonusDlg dlg(0);
        int frame = 9;
        const char* title =
            DATA_COMPGEN(0x00691210, adventureRolloverEmptyText, "");
        const char* botTitle =
            DATA_COMPGEN(0x00691210, adventureRolloverEmptyText, "");
        const char* description =
            DATA_COMPGEN(0x00691210, adventureRolloverEmptyText, "");

        switch (g_game->m_setup.m_startingBonus[playerPosition]) {
        case NEW_MAP_BONUS_ARTIFACT:
            frame = 9;
            title = g_generalText->getText(GENERAL_TEXT_ARTIFACT_BONUS);
            description = g_generalText->getText(GENERAL_TEXT_STARTING_ARTIFACT_DESCRIPTION);
            break;
        case NEW_MAP_BONUS_GOLD:
            frame = 8;
            botTitle = g_generalText->getText(GENERAL_TEXT_STARTING_GOLD_RANGE);
            title = g_generalText->getText(GENERAL_TEXT_GOLD_BONUS);
            description = g_generalText->getText(GENERAL_TEXT_STARTING_GOLD_DESCRIPTION);
            break;
        case NEW_MAP_BONUS_RESOURCE:
            // Conflux shares Inferno's icon frame exactly as it shares
            // Inferno's text row in GetStartingResourceName (0x576e00).
            frame = TOWN_INFERNO;
            if (g_game->m_setup.m_alignment[playerPosition] != TOWN_CONFLUX)
                frame = g_game->m_setup.m_alignment[playerPosition];
            title = g_generalText->getText(GENERAL_TEXT_RESOURCE_BONUS);
            botTitle = getStartingResourceName(
                g_game->m_setup.m_alignment[playerPosition]);
            description = getStartingResourceDescription(
                g_game->m_setup.m_alignment[playerPosition]);
            break;
        case NEW_MAP_BONUS_RANDOM:
            frame = 10;
            title = g_generalText->getText(GENERAL_TEXT_RANDOM_BONUS);
            description = g_generalText->getText(GENERAL_TEXT_STARTING_RANDOM_BONUS_DESCRIPTION);
            break;
        }

        dlg.createWin(title, m_bonusSprite, frame, botTitle, description);
        dlg.doModal(0);
        return 1;
    }
    }

    return CHeroWindowEx::processRightSelect(id);
}
