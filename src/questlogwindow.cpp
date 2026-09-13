// questlogwindow.cpp - E:\gamedcs\questlogwindow.cpp (compiland questlogwindow.obj)
// The quest log is the one reader of the map's two quest pools
// (NewfullMap's SeerHutList / QuestGuardList vectors).
#include <va.h>
#include <string.h>
#include "questlogwindow.h"
#include "border.h"
#include "button.h"
#include "game.h"
#include "kb.h"
#include "message.h"
#include "mousemgr.h"
#include "quest.h"
#include "seerhut.h"
#include "slider.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

DATA(0x0069cd20) static TQuestLogWindow* g_questLogWindow;

// Dreamcast names QuestActiveforPlayer as a const byte-returning TSeerHut
// helper.  Its old body tested playerGivenQuest and then !QuestCompleted.
// Complete's virtual quest model replaces the latter byte with a live quest
// and a non-empty quest-log line, but retail keeps the same final visited-bit
// and fresh quest-pointer tests.  Keep both pool-specific spellings: retail
// forms a named quest_text_row pointer for SeerHutList, while the exact
// UpdateQuestLogButton sibling proves quest_texts()[LOG] for guards.
inline unsigned char TSeerHut::questActiveforPlayer(
    const unsigned char playerNum) const
{
    type_quest* thisQuest = m_quest;
    if (!thisQuest)
        return 0;

    const std::string* questTexts = thisQuest->questTextRow()
        + type_quest::QUEST_TEXT_COLUMNS * thisQuest->questType();
    return questTexts[type_quest::QUEST_TEXT_LOG].length()
        && (m_visitedPlayers & (1 << playerNum))
        && m_quest;
}

inline unsigned char TQuestGuard::questActiveforPlayer(
    const unsigned char playerNum) const
{
    return m_quest
        && m_quest->questTexts()[type_quest::QUEST_TEXT_LOG].length()
        && (m_visitedPlayers & (1 << playerNum))
        && m_quest;
}

static void questSliderCallback(int state, heroWindow* parentWindow);

VA(0x0052d8c0, 0x8AF)  // dc 0x116604
TQuestLogWindow::TQuestLogWindow()
  : CAdvPopup(205, 32, 389, 535, 2), m_firstVisibleQuest(0)
{
    m_widgets.reserve(20);
    m_widgets.push_back(new bitmapBorder(
        0, 0, m_width, m_height, 0, "QuestLog.pcx", 0x800));

    m_widgets.push_back(new textWidget(
        45, 122, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 1, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 142, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 2, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 162, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 3, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 182, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 4, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 202, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 5, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 222, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 6, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 242, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 7, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 262, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 8, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 282, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 9, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 302, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 10, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 322, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 11, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 342, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 12, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 362, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 13, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 382, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 14, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 402, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 15, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        45, 422, 285, 20, 0, "smalfont.fnt", font::PRIMARY, 16, 1, 0, 8));

    m_widgets.push_back(new slider(
        335, 112, 16, 343, 17, 10, questSliderCallback,
        slider::BROWN, 16, 0));
    m_widgets.push_back(new button(
        324, 470, 32, 32, DIALOG_RETURN_OK, "QLexit.def",
        0, 1, 1, 28, 2));

    for (std::vector<widget*>::iterator it = m_widgets.begin();
         it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

VA(0x0052e170, 0x3A)  // dc 0x1165dc
static void questSliderCallback(int state, heroWindow* parentWindow)
{
    g_questLogWindow->m_firstVisibleQuest = state;
    g_questLogWindow->updateQuestLocators();
    g_questLogWindow->drawWindow(
        1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

#if 0  // @carcass

// E:\gamedcs\questlogwindow.cpp:81
DC_ONLY(0x116b6c, 0x6A)
void TQuestLogWindow::~TQuestLogWindow()
{
    // @stub
}

// E:\gamedcs\questlogwindow.cpp:89
DC_ONLY(0x116bd8, 0xA0)
void TQuestLogWindow::updateQuestLocator(int i)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x0052e1b0, 0x21, SCALAR_DELETING_DTOR, TQuestLogWindow)

VA(0x0052e1e0, 0x8F)  // dc 0x116b6c
TQuestLogWindow::~TQuestLogWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// It refreshes ONE row of the log. The row index is
// `firstVisibleQuest + i`, so firstVisibleQuest is the slider's scroll
// offset - the one thing the constructor zeroed and neither of the
// other two bodies touched.

VA(0x0052e270, 0x19F)  // dc 0x116bd8
void TQuestLogWindow::updateQuestLocator(int i)
{
    message msg;
    msg.m_id = MESSAGE_WIDGET;

    if (m_firstVisibleQuest + i < m_seerHutLogList.size()) {
        int quest = m_seerHutLogList[m_firstVisibleQuest + i];

        if (quest >= g_game->m_worldMap.m_seerHutList.size())
            strcpy(g_text, g_game->m_worldMap.m_questGuardList[
                       quest - g_game->m_worldMap.m_seerHutList.size()]
                       .questGuardFn00572D60().c_str());
        else
            strcpy(g_text, g_game->m_worldMap.m_seerHutList[quest]
                       .getSeerLogText().c_str());

        msg.m_codeX = widget::WIDGET_SET_TEXT;
        msg.m_codeY = i + 1;
        msg.m_extraText = g_text;
        broadcastMessage(msg);
    }
}

// Dreamcast preserves this helper as a 12-row loop.  Complete expands the
// same source boundary into DoQuestLog after increasing the visible list to
// 16 rows; the singular retail callee and loop schedule prove the revision.
// Keep the ordinary source helper (questlogwindow.cpp:105..107); retail's
// expansions in DoQuestLog and QuestSliderCallback do not prove `inline`.
void TQuestLogWindow::updateQuestLocators()
{
    for (int i = 0; i < 16; ++i)
        updateQuestLocator(i);
}

// E:\gamedcs\questlogwindow.cpp:111
VA(0x0052e410, 0x1d)  // source-order map + both retail call edges, dc 0x116ca4
int TQuestLogWindow::windowHandler(message& msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (!result)
        result = trueFalseDialogHandler(&msg);
    return result;
}

// E:\gamedcs\questlogwindow.cpp:142
// The Complete two-pool reconstruction moved 85.0157 -> 94.0047 when the
// SeerHut helper's first quest read was named `thisQuest`: retail homes that
// pointer across quest_type/quest_text_row, reducing reg_dist 150 -> 52 and
// making all 38 CFG blocks exact.  Keep the final `&& quest` separate: the
// virtual text calls may change the hut, and retail proves a fresh member
// load after the visited-player test.  The remaining delta is register
// scheduling inside otherwise exact 19-branch/1-ret flow, not permission to
// flatten either QuestActiveforPlayer or UpdateQuestLocators.
// Current residual (84.9444%, HIST 96.5463): the retained questTextRow
// definition is now visible and VC6 expands it in the SeerHut predicate.
// Retail calls it after questType. The current body has 41/38 blocks,
// 20/19 branches and 12/13 calls; this precedes the old register-only delta.
// A passive C2 trace reproduces the object: caller cb 348, budget 1000;
// SeerHut predicate cb 91 receives budget 98, allowing the 47-byte-cost
// row helper. The guard predicate receives 144 and expands questTexts.
// Making UpdateQuestLocators ordinary preserves all eight TU scores.
//
// The 18-state shared-helper family and its 36-state type-order follow-up
// each reproduced ten retained controls (18 and 36 distinct objects).
// Replacing the predicates with a shared questTexts composition restores
// the row call at 93.7917, but orders it BEFORE questType, unlike retail.
// Explicit type/index-then-row locals match all 13 named calls but score
// 82.6296..88.3056 and change the guard-loop order and other callers.
// A header definition also over-inlines questTextRow in the previously
// exact belong-to-player/resource setDefaultText bodies. Keeping it in this
// TU while adding calls to questTexts instead retains extra calls in the
// smaller seerhut/adventuremapwindow users. Thus neither visibility model
// establishes the proposed shared composition; no shared-header alternative
// is adopted. Do not restore the stale assertion that this is a mispaired
// call or that the helper has no available definition.
VA(0x0052e430, 0x27E)  // dc 0x116ccc; Complete adds QuestGuardList
void doQuestLog(int player)
{
    message msg;

    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);

    g_questLogWindow = new TQuestLogWindow;
    if (!g_questLogWindow)
        memError();

    int numberSeerHuts = g_game->m_worldMap.m_seerHutList.size();
    int i;
    for (i = 0; i < numberSeerHuts; ++i) {
        if (g_game->m_worldMap.m_seerHutList[i]
                .questActiveforPlayer(player))
            g_questLogWindow->m_seerHutLogList.push_back(i);
    }

    for (i = 0; i < g_game->m_worldMap.m_questGuardList.size(); ++i) {
        if (g_game->m_worldMap.m_questGuardList[i]
                .questActiveforPlayer(player))
            g_questLogWindow->m_seerHutLogList.push_back(numberSeerHuts + i);
    }

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_SLIDER_RESOLUTION;
    msg.m_codeY = 17;
    msg.m_extra = g_questLogWindow->m_seerHutLogList.size() - 15;
    g_questLogWindow->broadcastMessage(msg);

    g_questLogWindow->updateQuestLocators();
    g_questLogWindow->doModal(0);
    delete g_questLogWindow;
}

VA(0x0052e6b0, 0x2E)
const std::string* type_quest::questTextRow()
{
    return m_seerHut ? g_questTextA[m_textVariant] : g_questTextB[m_textVariant];
}

#if 0  // @carcass

// E:\gamedcs\questlogwindow.cpp:78
DC_ONLY(0x116e28, 0x34)
void* TQuestLogWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass
