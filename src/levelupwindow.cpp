#include "text.h"
#include "va.h"

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

// Retail scalar state; startup initial values come from the pinned image.
DATA(0x00697784) unsigned long g_dialogDeadline;

// Shared absolute deadline used by retail dialogs. Its timer role is proven
// by this handler and the other dialog handlers that compare GameTime::Get()
// against it before synthesizing WIDGET_END_DIALOG.


// Both are source-private in the DC levelupwindow compiland. Retail's ctor
// stores its object through the first, and this handler is the only consumer
// of the second.
DATA(0x00699634) static TLevelUpWindow* g_levelUpWindow;
DATA(0x0067fa34) static int g_lastImHoverId = -1;

// Text tables read directly by the retail constructor. The shared four-entry
// primary-skill table is declared with the other game-wide data in game.h.

VA(0x004f8880, 0xE7E)  // dc 0xe8344
TLevelUpWindow::TLevelUpWindow(hero* thisHero, int gainedSkill,
                               int firstChoice, int secondChoice)
    : CAdvPopup(205, 65, 385, 470, 0x12),
      m_leftSkill(firstChoice), m_rightSkill(secondChoice), m_selected(0)
{
    m_widgets.reserve(25);

    g_levelUpWindow = this;
    if (g_remoteOn && !g_currentPlayer->isLocalHuman())
        g_dialogDeadline = GameTime::get() + 15000;
    g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);

    bitmapBorder* background = new bitmapBorder(
        0, 0, 385, 470, BACKGROUND_ID, "lvlupbkg.pcx", 0x800);
    background->setPlayerPaletteColors(thisHero->m_owner);
    m_widgets.push_back(background);

    m_widgets.push_back(new bitmapBorder(
        171, 66, 58, 64, PORTRAIT_ID,
        g_heroTraits[thisHero->m_portrait].m_largePortraitName, 0x800));

    sprintf(g_text, g_generalText->getText(GENERAL_TEXT_LEVEL_UP_TITLE_FORMAT),
            thisHero->m_name);
    m_widgets.push_back(new textWidget(
        23, 22, 339, 23, g_text, "medfont.fnt", font::PRIMARY,
        TEXT1_ID, 5, 0, 8));

    sprintf(g_text, g_generalText->getText(GENERAL_TEXT_LEVEL_UP_HERO_FORMAT),
            thisHero->m_name, thisHero->m_level, thisHero->heroFn004D8F70());
    m_widgets.push_back(new textWidget(
        23, 151, 339, 23, g_text, "medfont.fnt", font::PRIMARY,
        TEXT2_ID, 5, 0, 8));

    sprintf(g_text, "%s +1", g_statNames[gainedSkill]);
    m_widgets.push_back(new textWidget(
        23, 242, 339, 23, g_text, "medfont.fnt", font::PRIMARY,
        TEXT3_ID, 5, 0, 8));
    m_widgets.push_back(new iconWidget(
        174, 190, 42, 42, PRISKILL_ID, "pskil42.def", gainedSkill,
        0, 0, 0, 0x10));

    if (secondChoice != -1) {
        sprintf(g_text, g_generalText->getText(GENERAL_TEXT_LEVEL_UP_CHOICE),
                g_secondarySkillLevels[firstChoice % 3],
                g_sSkillTraits[firstChoice / 3 - 1].m_name,
                g_secondarySkillLevels[secondChoice % 3],
                g_sSkillTraits[secondChoice / 3 - 1].m_name);
        m_widgets.push_back(new textWidget(
            23, 270, 339, 52, g_text, "medfont.fnt", font::PRIMARY,
            TEXT4_ID, 1, 0, 8));
        m_widgets.push_back(new textWidget(
            169, 325, 50, 46,
            g_generalText->getText(GENERAL_TEXT_LEVEL_UP_OR),
            "medfont.fnt", font::PRIMARY, TEXT5_ID, 5, 0, 8));

        m_widgets.push_back(new coloredBorderFrame(
            122, 325, 47, 46, SKILLBORDER_1_ID,
            g_systemPalette->m_data[45], 0x400));
        widget* addedLeft = m_widgets.back();
        addedLeft->setVisible(0);

        m_widgets.push_back(new coloredBorderFrame(
            220, 325, 47, 46, SKILLBORDER_2_ID,
            g_systemPalette->m_data[45], 0x400));
        widget* addedRight = m_widgets.back();
        addedRight->setVisible(0);

        m_widgets.push_back(new iconWidget(
            124, 326, 44, 44, SKILLICON_1_ID, "secskill.def",
            firstChoice, 0, 0, 0, 0x10));
        m_widgets.push_back(new iconWidget(
            222, 326, 44, 44, SKILLICON_2_ID, "secskill.def",
            secondChoice, 0, 0, 0, 0x10));

        sprintf(g_text, "%s\n%s", g_secondarySkillLevels[firstChoice % 3],
                g_sSkillTraits[firstChoice / 3 - 1].m_name);
        m_widgets.push_back(new textWidget(
            102, 375, 87, 40, g_text, "smalfont.fnt", font::PRIMARY,
            TEXT6_ID, 5, 0, 8));
        sprintf(g_text, "%s\n%s", g_secondarySkillLevels[secondChoice % 3],
                g_sSkillTraits[secondChoice / 3 - 1].m_name);
        m_widgets.push_back(new textWidget(
            200, 375, 87, 40, g_text, "smalfont.fnt", font::PRIMARY,
            TEXT7_ID, 5, 0, 8));
    } else if (firstChoice != -1) {
        sprintf(g_text, g_generalText->getText(GENERAL_TEXT_LEVEL_UP_SINGLE_CHOICE),
                g_secondarySkillLevels[firstChoice % 3],
                g_sSkillTraits[firstChoice / 3 - 1].m_name);
        m_widgets.push_back(new textWidget(
            23, 270, 339, 52, g_text, "medfont.fnt", font::PRIMARY,
            TEXT4_ID, 1, 0, 8));
        m_widgets.push_back(new coloredBorderFrame(
            169, 325, 47, 46, SKILLBORDER_1_ID,
            g_systemPalette->m_data[45], 0x400));
        m_widgets.push_back(new iconWidget(
            170, 326, 44, 44, SKILLICON_1_ID, "secskill.def",
            firstChoice, 0, 0, 0, 0x10));
        sprintf(g_text, "%s\n%s", g_secondarySkillLevels[firstChoice % 3],
                g_sSkillTraits[firstChoice / 3 - 1].m_name);
        m_widgets.push_back(new textWidget(
            149, 375, 87, 40, g_text, "smalfont.fnt", font::PRIMARY,
            TEXT6_ID, 5, 0, 8));
        m_selected = SKILLICON_1_ID;
    }

    button* accept = new button(
        296, 413, 64, 30, LEVELUP_ACCEPT_ID, "iokay.def",
        0, 1, 0, 0, 2);
    accept->setHotkey(1);
    accept->setHotkey(28);
    accept->enable(firstChoice == -1 || secondChoice == -1);
    m_widgets.push_back(accept);

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }

    if (g_turnDuration.isOn()
            && g_turnDuration.isClose(15000))
        g_dialogDeadline = GameTime::get() + 15000;
    if (g_goSolo)
        g_dialogDeadline = GameTime::get() + 2000;
}

VA_COMPGEN(0x004f9700, 0x21, SCALAR_DELETING_DTOR, TLevelUpWindow)

VA(0x004f9730, 0x4E)  // dc 0xe8c2c
TLevelUpWindow::~TLevelUpWindow()
{
    deleteWidgets();
}

// E:\gamedcs\levelupwindow.cpp:170, dc 0xe8c64
VA(0x004f9780, 0x440)  // vtable slot 9+linkorder, dc 0xe8c64
int TLevelUpWindow::windowHandler(message& msg)
{
    if (!g_dialogDeadline) {
        int result = CAdvPopup::windowHandler(msg);
        if (result) {
            if (g_turnDuration.isExpired())
                g_windowManager->m_dialogReturn = 9999;
            return result;
        }
    }

    pollSound();

    unsigned long deadline = g_dialogDeadline;
    if (deadline && GameTime::isPast(deadline)) {
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = 9999;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        g_dialogDeadline = 0;
        return MESSAGE_DISPATCH_FORWARD;
    }

    if (msg.m_id == MESSAGE_KEY_DOWN) {
        switch (msg.m_codeX) {
        case LEVELUP_SELECT_RIGHT_KEY: {
            if (g_levelUpWindow->m_rightSkill == -1)
                break;
            widget* leftBorder = g_levelUpWindow->getWidget(SKILLBORDER_1_ID);
            leftBorder->sendMessage(
                widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
            widget* rightBorder = g_levelUpWindow->getWidget(SKILLBORDER_2_ID);
            rightBorder->sendMessage(
                widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
            g_levelUpWindow->m_selected = SKILLICON_2_ID;
            widget* accept = g_levelUpWindow->getWidget(LEVELUP_ACCEPT_ID);
            accept->enable(1);
            g_levelUpWindow->drawWindow(1, 0xffff0001, 0xffff);
            return MESSAGE_DISPATCH_CONSUME;
        }

        case LEVELUP_SELECT_LEFT_KEY: {
            if (g_levelUpWindow->m_leftSkill == -1)
                break;
            widget* leftBorder = g_levelUpWindow->getWidget(SKILLBORDER_1_ID);
            leftBorder->sendMessage(
                widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
            if (g_levelUpWindow->m_rightSkill != -1) {
                widget* rightBorder =
                    g_levelUpWindow->getWidget(SKILLBORDER_2_ID);
                rightBorder->sendMessage(
                    widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
            }
            g_levelUpWindow->m_selected = SKILLICON_1_ID;
            widget* accept = g_levelUpWindow->getWidget(LEVELUP_ACCEPT_ID);
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
                g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
        }
    } else if (msg.m_id == MESSAGE_WIDGET) {
        switch (msg.m_codeX) {
        case widget::WIDGET_RIGHT_SELECT:
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
        case widget::WIDGET_DESELECT:
            if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)
                break;

            switch (msg.m_codeY) {
            case SKILLICON_1_ID:
            case SKILLBORDER_1_ID: {
                widget* rightBorder =
                    g_levelUpWindow->getWidget(SKILLBORDER_2_ID);
                widget* leftBorder =
                    g_levelUpWindow->getWidget(SKILLBORDER_1_ID);
                leftBorder->sendMessage(
                    widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
                if (rightBorder)
                    rightBorder->sendMessage(
                        widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
                g_levelUpWindow->m_selected = SKILLICON_1_ID;
                widget* accept =
                    g_levelUpWindow->getWidget(LEVELUP_ACCEPT_ID);
                accept->enable(1);
                g_levelUpWindow->drawWindow(1, 0xffff0001, 0xffff);
                return MESSAGE_DISPATCH_CONSUME;
            }

            case SKILLICON_2_ID:
            case SKILLBORDER_2_ID: {
                widget* leftBorder =
                    g_levelUpWindow->getWidget(SKILLBORDER_1_ID);
                leftBorder->sendMessage(
                    widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
                widget* rightBorder =
                    g_levelUpWindow->getWidget(SKILLBORDER_2_ID);
                rightBorder->sendMessage(
                    widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
                g_levelUpWindow->m_selected = SKILLICON_2_ID;
                widget* accept =
                    g_levelUpWindow->getWidget(LEVELUP_ACCEPT_ID);
                accept->enable(1);
                g_levelUpWindow->drawWindow(1, 0xffff0001, 0xffff);
                return MESSAGE_DISPATCH_CONSUME;
            }

            case LEVELUP_ACCEPT_ID:
                msg.m_id = MESSAGE_WIDGET;
                g_windowManager->m_dialogReturn = g_levelUpWindow->m_selected;
                msg.m_codeY = widget::WIDGET_END_DIALOG;
                msg.m_codeX = widget::WIDGET_END_DIALOG;
                return MESSAGE_DISPATCH_FORWARD;
            }
            return 0;
        }
    }

    return 0;
}
