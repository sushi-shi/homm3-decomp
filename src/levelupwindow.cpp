// levelupwindow.cpp - E:\gamedcs\levelupwindow.cpp (compiland levelupwindow.obj)
#include <va.h>
#include <stdio.h>
#include "levelupwindow.h"
#include "border.h"
#include "button.h"
#include "exec.h"
#include "game.h"
#include "hero.h"
#include "iconwdgt.h"
#include "kb.h"
#include "kbwin.h"
#include "message.h"
#include "mousemgr.h"
#include "recruit.h"
#include "remote.h"
#include "soundmgr.h"
#include "sskilltraits.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// Shared absolute deadline used by retail dialogs. Its timer role is proven
// by this handler and the other dialog handlers that compare GameTime::Get()
// against it before synthesizing WIDGET_END_DIALOG.
DATA(0x00697784) extern unsigned long g_dialogDeadline697784;

// Both are source-private in the DC levelupwindow compiland. Retail's ctor
// stores its object through the first, and this handler is the only consumer
// of the second.
DATA(0x00699634) static LevelUpWindow* g_levelUpWindow;
DATA(0x0067fa34) static int g_lastImHoverId = -1;

// Text tables read directly by the retail constructor. The shared four-entry
// primary-skill table is declared with the other game-wide data in game.h.
DATA(0x006a7570) extern const char* g_skillMasteryNames[3];

VA(0x004f8880, 0xE7E)  // dc 0xe8344
LevelUpWindow::LevelUpWindow(Hero* thisHero, int gainedSkill,
                               int firstChoice, int secondChoice)
    : CAdvPopup(205, 65, 385, 470, 0x12),
      m_leftSkill(firstChoice), m_rightSkill(secondChoice), m_selected(0)
{
    m_widgets.reserve(25);

    g_levelUpWindow = this;
    if (g_networkActive69954c && !g_currentPlayer->isLocalHuman())
        g_dialogDeadline697784 = GameTime::get() + 15000;
    g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);

    BitmapBorder* background = new BitmapBorder(
        0, 0, 385, 470, BACKGROUND_ID, "lvlupbkg.pcx", 0x800);
    background->setPlayerPaletteColors(thisHero->m_owner);
    m_widgets.push_back(background);

    m_widgets.push_back(new BitmapBorder(
        171, 66, 58, 64, PORTRAIT_ID,
        g_heroTraits[thisHero->m_portrait].m_largePortraitName, 0x800));

    sprintf(g_text, (*g_generalText)[GENERAL_TEXT_LEVEL_UP_TITLE_FORMAT],
            thisHero->m_name);
    m_widgets.push_back(new TextWidget(
        23, 22, 339, 23, g_text, "medfont.fnt", Font::PRIMARY,
        TEXT1_ID, 5, 0, 8));

    sprintf(g_text, (*g_generalText)[GENERAL_TEXT_LEVEL_UP_HERO_FORMAT],
            thisHero->m_name, thisHero->m_level, thisHero->heroFn004D8F70());
    m_widgets.push_back(new TextWidget(
        23, 151, 339, 23, g_text, "medfont.fnt", Font::PRIMARY,
        TEXT2_ID, 5, 0, 8));

    sprintf(g_text, "%s +1", g_primarySkillNames[gainedSkill]);
    m_widgets.push_back(new TextWidget(
        23, 242, 339, 23, g_text, "medfont.fnt", Font::PRIMARY,
        TEXT3_ID, 5, 0, 8));
    m_widgets.push_back(new IconWidget(
        174, 190, 42, 42, PRISKILL_ID, "pskil42.def", gainedSkill,
        0, 0, 0, 0x10));

    if (secondChoice != -1) {
        sprintf(g_text, (*g_generalText)[GENERAL_TEXT_LEVEL_UP_CHOICE],
                g_skillMasteryNames[firstChoice % 3],
                g_sSkillTraits[firstChoice / 3 - 1].m_name,
                g_skillMasteryNames[secondChoice % 3],
                g_sSkillTraits[secondChoice / 3 - 1].m_name);
        m_widgets.push_back(new TextWidget(
            23, 270, 339, 52, g_text, "medfont.fnt", Font::PRIMARY,
            TEXT4_ID, 1, 0, 8));
        m_widgets.push_back(new TextWidget(
            169, 325, 50, 46,
            (*g_generalText)[GENERAL_TEXT_LEVEL_UP_OR],
            "medfont.fnt", Font::PRIMARY, TEXT5_ID, 5, 0, 8));

        m_widgets.push_back(new ColoredBorderFrame(
            122, 325, 47, 46, SKILLBORDER_1_ID,
            g_unnamed6aacb0->m_data[45], 0x400));
        Widget* addedLeft = m_widgets.back();
        addedLeft->setVisible(0);

        m_widgets.push_back(new ColoredBorderFrame(
            220, 325, 47, 46, SKILLBORDER_2_ID,
            g_unnamed6aacb0->m_data[45], 0x400));
        Widget* addedRight = m_widgets.back();
        addedRight->setVisible(0);

        m_widgets.push_back(new IconWidget(
            124, 326, 44, 44, SKILLICON_1_ID, "secskill.def",
            firstChoice, 0, 0, 0, 0x10));
        m_widgets.push_back(new IconWidget(
            222, 326, 44, 44, SKILLICON_2_ID, "secskill.def",
            secondChoice, 0, 0, 0, 0x10));

        sprintf(g_text, "%s\n%s", g_skillMasteryNames[firstChoice % 3],
                g_sSkillTraits[firstChoice / 3 - 1].m_name);
        m_widgets.push_back(new TextWidget(
            102, 375, 87, 40, g_text, "smalfont.fnt", Font::PRIMARY,
            TEXT6_ID, 5, 0, 8));
        sprintf(g_text, "%s\n%s", g_skillMasteryNames[secondChoice % 3],
                g_sSkillTraits[secondChoice / 3 - 1].m_name);
        m_widgets.push_back(new TextWidget(
            200, 375, 87, 40, g_text, "smalfont.fnt", Font::PRIMARY,
            TEXT7_ID, 5, 0, 8));
    } else if (firstChoice != -1) {
        sprintf(g_text, (*g_generalText)[GENERAL_TEXT_LEVEL_UP_SINGLE_CHOICE],
                g_skillMasteryNames[firstChoice % 3],
                g_sSkillTraits[firstChoice / 3 - 1].m_name);
        m_widgets.push_back(new TextWidget(
            23, 270, 339, 52, g_text, "medfont.fnt", Font::PRIMARY,
            TEXT4_ID, 1, 0, 8));
        m_widgets.push_back(new ColoredBorderFrame(
            169, 325, 47, 46, SKILLBORDER_1_ID,
            g_unnamed6aacb0->m_data[45], 0x400));
        m_widgets.push_back(new IconWidget(
            170, 326, 44, 44, SKILLICON_1_ID, "secskill.def",
            firstChoice, 0, 0, 0, 0x10));
        sprintf(g_text, "%s\n%s", g_skillMasteryNames[firstChoice % 3],
                g_sSkillTraits[firstChoice / 3 - 1].m_name);
        m_widgets.push_back(new TextWidget(
            149, 375, 87, 40, g_text, "smalfont.fnt", Font::PRIMARY,
            TEXT6_ID, 5, 0, 8));
        m_selected = SKILLICON_1_ID;
    }

    Button* accept = new Button(
        296, 413, 64, 30, LEVELUP_ACCEPT_ID, "iokay.def",
        0, 1, 0, 0, 2);
    accept->setHotkey(1);
    accept->setHotkey(28);
    accept->enable(firstChoice == -1 || secondChoice == -1);
    m_widgets.push_back(accept);

    for (Widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }

    if (g_turnDuration69d630.isOn()
            && g_turnDuration69d630.isClose(15000))
        g_dialogDeadline697784 = GameTime::get() + 15000;
    if (g_unk691209)
        g_dialogDeadline697784 = GameTime::get() + 2000;
}

VA_COMPGEN(0x004f9700, 0x21, SCALAR_DELETING_DTOR, LevelUpWindow)

VA(0x004f9730, 0x4E)  // dc 0xe8c2c
LevelUpWindow::~LevelUpWindow()
{
    deleteWidgets();
}

// E:\gamedcs\levelupwindow.cpp:170, dc 0xe8c64
VA(0x004f9780, 0x440)  // vtable slot 9+linkorder, dc 0xe8c64
int LevelUpWindow::windowHandler(Message& msg)
{
    if (!g_dialogDeadline697784) {
        int result = CAdvPopup::windowHandler(msg);
        if (result) {
            if (g_turnDuration69d630.isExpired())
                g_windowManager->m_dialogReturn = 9999;
            return result;
        }
    }

    pollSound();

    unsigned long deadline = g_dialogDeadline697784;
    if (deadline && GameTime::isPast(deadline)) {
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = 9999;
        msg.m_codeY = Widget::WIDGET_END_DIALOG;
        msg.m_codeX = Widget::WIDGET_END_DIALOG;
        g_dialogDeadline697784 = 0;
        return MESSAGE_DISPATCH_FORWARD;
    }

    if (msg.m_id == MESSAGE_KEY_DOWN) {
        switch (msg.m_codeX) {
        case LEVELUP_SELECT_RIGHT_KEY: {
            if (g_levelUpWindow->m_rightSkill == -1)
                break;
            Widget* leftBorder = g_levelUpWindow->getWidget(SKILLBORDER_1_ID);
            leftBorder->sendMessage(
                Widget::WIDGET_CLEAR_STATUS, Widget::WIDGET_DRAWN);
            Widget* rightBorder = g_levelUpWindow->getWidget(SKILLBORDER_2_ID);
            rightBorder->sendMessage(
                Widget::WIDGET_SET_STATUS, Widget::WIDGET_DRAWN);
            g_levelUpWindow->m_selected = SKILLICON_2_ID;
            Widget* accept = g_levelUpWindow->getWidget(LEVELUP_ACCEPT_ID);
            accept->enable(1);
            g_levelUpWindow->drawWindow(1, 0xffff0001, 0xffff);
            return MESSAGE_DISPATCH_CONSUME;
        }

        case LEVELUP_SELECT_LEFT_KEY: {
            if (g_levelUpWindow->m_leftSkill == -1)
                break;
            Widget* leftBorder = g_levelUpWindow->getWidget(SKILLBORDER_1_ID);
            leftBorder->sendMessage(
                Widget::WIDGET_SET_STATUS, Widget::WIDGET_DRAWN);
            if (g_levelUpWindow->m_rightSkill != -1) {
                Widget* rightBorder =
                    g_levelUpWindow->getWidget(SKILLBORDER_2_ID);
                rightBorder->sendMessage(
                    Widget::WIDGET_CLEAR_STATUS, Widget::WIDGET_DRAWN);
            }
            g_levelUpWindow->m_selected = SKILLICON_1_ID;
            Widget* accept = g_levelUpWindow->getWidget(LEVELUP_ACCEPT_ID);
            accept->enable(1);
            g_levelUpWindow->drawWindow(1, 0xffff0001, 0xffff);
            return MESSAGE_DISPATCH_CONSUME;
        }
        }
    } else if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        int hoverID = g_levelUpWindow->findWidget(msg.m_mouseX, msg.m_mouseY);
        if (hoverID != g_lastImHoverId) {
            g_lastImHoverId = hoverID;
            if (hoverID != -1)
                g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
        }
    } else if (msg.m_id == MESSAGE_WIDGET) {
        switch (msg.m_codeX) {
        case Widget::WIDGET_RIGHT_SELECT:
            switch (msg.m_codeY) {
            case SKILLICON_1_ID:
            case SKILLBORDER_1_ID:
                normalDialog(
                    g_sSkillTraits[g_levelUpWindow->m_leftSkill / 3 - 1]
                        .m_levelNames[g_levelUpWindow->m_leftSkill % 3],
                    4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                break;
            case SKILLICON_2_ID:
            case SKILLBORDER_2_ID:
                normalDialog(
                    g_sSkillTraits[g_levelUpWindow->m_rightSkill / 3 - 1]
                        .m_levelNames[g_levelUpWindow->m_rightSkill % 3],
                    4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                break;
            }
            // The right-click arm shares the selection tail below.
        case Widget::WIDGET_DESELECT:
            if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)
                break;

            switch (msg.m_codeY) {
            case SKILLICON_1_ID:
            case SKILLBORDER_1_ID: {
                Widget* rightBorder =
                    g_levelUpWindow->getWidget(SKILLBORDER_2_ID);
                Widget* leftBorder =
                    g_levelUpWindow->getWidget(SKILLBORDER_1_ID);
                leftBorder->sendMessage(
                    Widget::WIDGET_SET_STATUS, Widget::WIDGET_DRAWN);
                if (rightBorder)
                    rightBorder->sendMessage(
                        Widget::WIDGET_CLEAR_STATUS, Widget::WIDGET_DRAWN);
                g_levelUpWindow->m_selected = SKILLICON_1_ID;
                Widget* accept =
                    g_levelUpWindow->getWidget(LEVELUP_ACCEPT_ID);
                accept->enable(1);
                g_levelUpWindow->drawWindow(1, 0xffff0001, 0xffff);
                return MESSAGE_DISPATCH_CONSUME;
            }

            case SKILLICON_2_ID:
            case SKILLBORDER_2_ID: {
                Widget* leftBorder =
                    g_levelUpWindow->getWidget(SKILLBORDER_1_ID);
                leftBorder->sendMessage(
                    Widget::WIDGET_CLEAR_STATUS, Widget::WIDGET_DRAWN);
                Widget* rightBorder =
                    g_levelUpWindow->getWidget(SKILLBORDER_2_ID);
                rightBorder->sendMessage(
                    Widget::WIDGET_SET_STATUS, Widget::WIDGET_DRAWN);
                g_levelUpWindow->m_selected = SKILLICON_2_ID;
                Widget* accept =
                    g_levelUpWindow->getWidget(LEVELUP_ACCEPT_ID);
                accept->enable(1);
                g_levelUpWindow->drawWindow(1, 0xffff0001, 0xffff);
                return MESSAGE_DISPATCH_CONSUME;
            }

            case LEVELUP_ACCEPT_ID:
                msg.m_id = MESSAGE_WIDGET;
                g_windowManager->m_dialogReturn = g_levelUpWindow->m_selected;
                msg.m_codeY = Widget::WIDGET_END_DIALOG;
                msg.m_codeX = Widget::WIDGET_END_DIALOG;
                return MESSAGE_DISPATCH_FORWARD;
            }
            return 0;
        }
    }

    return 0;
}
