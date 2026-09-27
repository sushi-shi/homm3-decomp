#include "text.h"
#include "va.h"

#include "combatwindow.h"

#include "border.h"
#include "cmbtmgr.h"
#include "combatcontrolsubwindow.h"
#include "combatwindowchatedit.h"
#include "font.h"
#include "game.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "message.h"
#include "remote.h"
#include "subwindow.h"
#include "textntry.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// Retail .bss 0x695000. The constructor publishes itself here for the chat
// edit callbacks and the destructor clears the slot. The Dreamcast image has
// the corresponding compiland-local pointer at 0x1bf12c.
static TCombatWindow* g_combatWindow;

// E:\gamedcs\combatwindow.cpp:42, dc 0x69638
VA(0x00472010, 0x1C0) MAC_ADDRESS(0x07fef4, 0x198)  // anchor-caller SendChat + three cheat arms, dc 0x69638
void checkCombatCheatCode(std::string& chatString)
{
    hero* currentHero =
        g_combatManager->m_heroes[g_combatManager->m_currentSide];
    std::string* chat = &chatString;
    bool recognized = 0;
    TCheatCode code(chat->c_str());

    if (code.compare(DATA_COMPGEN(
            0x0063d490, combatCheatBluePill, "ajpoyhrcvyy"))) {
        recognized = 1;
        g_combatManager->unnamed4693a0(g_combatManager->m_currentSide);
    } else if (code.compare(DATA_COMPGEN(
                   0x0063d49c, combatCheatRedPill, "ajperqcvyy"))) {
        recognized = 1;
        g_combatManager->unnamed4693a0(1 - g_combatManager->m_currentSide);
    } else if (code.compare(DATA_COMPGEN(
                   0x0063d4a8, combatCheatAllSpells,
                   "ajpgurervfabfcbba"))
               && currentHero) {
        recognized = 1;
        currentHero->m_mana = 999;
        if (!currentHero->isWieldingArtifact(ARTIFACT_SPELLBOOK)) {
            type_artifact spellbook(ARTIFACT_SPELLBOOK);
            currentHero->giveArtifact(&spellbook, 1, 1);
        }
        for (int spell = 0; spell < hero::NUM_SPELLS; spell++) {
            currentHero->addSpell(spell);
        }
    }

    if (recognized) {
        *chat = (*g_generalText)[GENERAL_TEXT_CHEATER];
        g_game->m_isCheater = 1;
        if (g_inCampaign) {
            g_game->m_campaign.m_isCheater = 1;
        }
    }
}

// DC139 calls CGameChatEdit's constructor. Retail vtable 0x63d4bc has
// 27 slots; slots 25/26 are CGameChatEdit::sendChatCleanup and activate.
// The base owns activated at +0x70 and its alignment.

// Retail expands this ordinary forwarding constructor into TCombatWindow.
// The canonical CGameChatEdit base owns the +0x70 clear.

CCombatChatEdit::CCombatChatEdit(
    int x, int y, int w, int h, int textSize, char* text, char* fontName,
    font::TColor color, font::EJustify justification, char* backgroundIcon,
    int backgroundFrame, int id, int style, int readType, int insetX,
    int insetY)
    : CGameChatEdit(x, y, w, h, textSize, text, fontName, color, justification,
                backgroundIcon, backgroundFrame, id, style, readType,
                insetX, insetY)
{
}

VA(0x004721d0, 0x42A) MAC_ADDRESS(0x08008c, 0x52c)  // dc 0x69850
TCombatWindow::TCombatWindow(unsigned char doPlacement)
    : heroWindow(0, 0, 800, 600, 1)
{
    g_combatWindow = this;
    m_chatWidget = 0;
    m_chatEdit = 0;

    m_widgets.reserve(3);
    m_widgets.push_back(new border(0, 0, 800, 556, 0, 1));

    m_chatWidget = new textWidget(
        75, 100, 520, 440, 0,
        DATA_COMPGEN(0x0065f2ec, combatWindowMedfont, "medfont.fnt"),
        font::CHAT, 1, font::BOTTOM_JUSTIFIED, 0, 8);
    m_chatEdit = new CCombatChatEdit(
        214, 563, 400, 32, 127,
        DATA_COMPGEN(0x00691210, combatChatEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, combatChatSmallFont, "smalfont.fnt"),
        font::WHITE, font::LEFT_JUSTIFIED,
        DATA_COMPGEN(0x00670020, combatChatBackground, "cRollovr.pcx"),
        0, 2, 0x100, textEntryWidget::READ_TYPE_INSET, 3, 0);
    m_widgets.push_back(m_chatEdit);
    m_widgets.push_back(m_chatWidget);

    for (std::vector<widget*>::iterator it = m_widgets.begin();
         it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    if (doPlacement) {
        m_controlSubWindow = new TCombatPlacementSubWindow(this);
        widgetSetStatus(COMBAT_RIGHT_COMMAND_0_ID, 8);
        widgetSetStatus(COMBAT_RIGHT_COMMAND_1_ID, 8);
        widgetSetStatus(COMBAT_RIGHT_COMMAND_2_ID, 8);
    } else {
        m_controlSubWindow = new TCombatControlSubWindow(this);
    }

    m_heroSubWindows[0] = new TCombatHeroSubWindow(1, 135, 78, 202, this);
    m_heroSubWindows[1] = new TCombatHeroSubWindow(721, 135, 78, 202, this);
    m_creatureSubWindows[0] =
        new TCombatCreatureSubWindow(1, 267, 78, 288, this, 1);
    m_creatureSubWindows[1] =
        new TCombatCreatureSubWindow(721, 267, 78, 288, this, 1);
    m_creatureSubWindows[2] =
        new TCombatCreatureSubWindow(1, 429, 78, 126, this, 2);
    m_creatureSubWindows[3] =
        new TCombatCreatureSubWindow(721, 429, 78, 126, this, 2);

    m_combatMessageCount = 0;
    m_combatMessageStart = 0;
}

VA(0x00472600, 0xA5) MAC_ADDRESS(0x08146c, 0x118)  // dc 0x6a484
int CCombatChatEdit::onKeyPress(message* msg)
{
    if (m_activated)
        return CChatEdit::onKeyPress(msg);

    if (getCharPressed(msg) == KEYCODE_TAB) {
        g_combatWindow->onChatActivate(1);
        m_activated = 1;
        setFocus(1);
        m_parentWindow->setFocus(m_id);
        draw();
        g_windowManager->updateScreen(
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
        return 1;
    }
    return 0;
}

// E:\gamedcs\combatwindow.cpp:171
VA(0x004726b0, 0x131) MAC_ADDRESS(0x081584, 0xb0)  // vtable slot + SendChat/IsMultiplayer, dc 0x6a488
void CCombatChatEdit::sendChat(const char* chat, int toWho)
{
    std::string chatString(chat);
    if (!g_game->isMultiplayer()) {
        checkCombatCheatCode(chatString);
    }

    m_activated = 0;
    ::sendChat(chatString.c_str(), toWho);
    m_parentWindow->setFocus(-1);
    setFocus(0);

    g_combatWindow->onChatActivate(0);
}

VA(0x004727f0, 0x5E) MAC_ADDRESS(0x081634, 0x78)  // dc 0x6a540
int CCombatChatEdit::onEscape(message msg)
{
    m_activated = 0;
    m_parentWindow->setFocus(-1);
    setFocus(0);
    draw();

    g_combatWindow->onChatActivate(0);
    return 1;
}

VA(0x00472850, 0x3D) MAC_ADDRESS(0x0816ac, 0x78)  // dc 0x6a590
void CCombatChatEdit::updateScreen()
{
    if (m_activated) {
        draw();
        g_windowManager->updateScreen(
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
    }
}

VA_COMPGEN(0x00472890, 0x05, IMPLICIT_DTOR, CCombatChatEdit)

// Vtable 0x63d528 slot 0.
VA_COMPGEN(0x004728a0, 0x21, SCALAR_DELETING_DTOR, TCombatWindow)

VA(0x004728d0, 0x2A) MAC_ADDRESS(0x08063c, 0x6c)  // dc 0x69b2c
void TCombatWindow::close(unsigned char update)
{
    if (m_controlSubWindow) {
        delete m_controlSubWindow;
        m_controlSubWindow = 0;
    }
    heroWindow::close(update);
}

VA(0x00472900, 0x14E) MAC_ADDRESS(0x0806a8, 0x1ec)
TCombatWindow::~TCombatWindow()
{
    if (m_controlSubWindow)
        delete m_controlSubWindow;

    for (std::vector<widget*>::iterator it = m_widgets.begin();
         it != m_widgets.end(); ++it)
        delete *it;

    for (unsigned int i = 0; i < m_combatMessages.size(); ++i)
        delete m_combatMessages[i];

    delete m_heroSubWindows[0];
    delete m_heroSubWindows[1];
    delete m_creatureSubWindows[0];
    delete m_creatureSubWindows[1];
    delete m_creatureSubWindows[2];
    delete m_creatureSubWindows[3];

    g_combatWindow = 0;
}

// Retail folds the DC helper at combatwindow.cpp:322-346 into its only caller.
// The two scroll arrows deliberately share help row 5.
MAC_ADDRESS(0x080894, 0xe0)
inline int TCombatWindow::convertID2HelpID(int id)
{
    if (id < 0)
        return -1;

    switch (id) {
    case COMBAT_LEFT_COMMAND_0_ID: return 0;
    case COMBAT_LEFT_COMMAND_1_ID: return 1;
    case COMBAT_LEFT_COMMAND_2_ID: return 2;
    case COMBAT_LEFT_COMMAND_3_ID: return 3;
    case COMBAT_ROLLOVER_ID: return 4;
    case COMBAT_LOG_SCROLL_UP_ID:
    case COMBAT_LOG_SCROLL_DOWN_ID: return 5;
    case COMBAT_RIGHT_COMMAND_0_ID: return 6;
    case COMBAT_RIGHT_COMMAND_1_ID: return 7;
    case COMBAT_RIGHT_COMMAND_2_ID: return 8;
    case COMBAT_PLACEMENT_COMMAND_0_ID: return 9;
    case COMBAT_PLACEMENT_COMMAND_1_ID: return 10;
    default: return -1;
    }
}

VA(0x00472a50, 0x124) MAC_ADDRESS(0x080974, 0xb4)
unsigned char TCombatWindow::processRightSelect(const message* msg)
{
    int helpID = convertID2HelpID(msg->m_codeY);
    if (helpID < 0)
        return 0;

    const char* text = g_combatWindowHelp[helpID].m_rclick;
    int width;
    int height;
    getQuickviewSize(text, &width, &height);
    normalDialog(text, 4, (WINDOW_SCREEN_WIDTH - width) / 2,
        (WINDOW_SCREEN_HEIGHT - height) / 2,
        -1, 0, -1, 0, -1, 0, -1, 0);
    return 1;
}

// Dreamcast keeps this source helper out of line; retail /Ob2 folds both call
// sites in handle_widget_hover. The control-bar vtable and the chat editor's
// +0x6d focus byte independently prove the two member types.
MAC_ADDRESS(0x080a28, 0x4c)
inline void TCombatWindow::setRollover(const char* newText)
{
    if (m_controlSubWindow && !m_chatEdit->m_hasFocus)
        m_controlSubWindow->setRollover(newText);
}

VA(0x00472b80, 0x67) MAC_ADDRESS(0x080a74, 0x68)
void TCombatWindow::handleWidgetHover(widget* currentWidget)
{
    const char* newText = currentWidget->getHelpText();
    if (m_combatMessageCount > 0
        && (!newText || currentWidget->m_id == COMBAT_LOG_SCROLL_UP_ID
            || currentWidget->m_id == COMBAT_LOG_SCROLL_DOWN_ID))
        return;

    if (!newText)
        setRollover(DATA_COMPGEN(
            0x00691210, combatHoverRolloverEmptyText, ""));
    else
        setRollover(newText);
}

VA(0x00472bf0, 0x36) MAC_ADDRESS(0x080adc, 0x6c)
void TCombatWindow::clearCombatMessages()
{
    if (m_combatMessageCount
        && GameTime::elapsedSince(m_combatMessageTime) >= 3000) {
        m_combatMessageCount = 0;
        combatMessage(
            DATA_COMPGEN(0x00691210, combatRolloverEmptyText, ""), 0, 0);
    }
}

VA(0x00472c30, 0x173) MAC_ADDRESS(0x080b48, 0x110)
void TCombatWindow::showMessages(long start)
{
    if (start < m_combatMessages.size()) {
        std::string result;
        m_combatMessageStart = start;
        result = *m_combatMessages[start];
        if (start + 1 < m_combatMessages.size()) {
            result += '\n';
            result += *m_combatMessages[start + 1];
        }
        if (m_controlSubWindow)
            m_controlSubWindow->setRolloverButtons(
                start, m_combatMessages.size());
        setRollover(result.c_str());
        m_combatMessageTime = GameTime::get();
    }
}

VA(0x00472db0, 0x40) MAC_ADDRESS(0x080c58, 0x58)
void TCombatWindow::scrollRollover(long delta)
{
    if (m_controlSubWindow) {
        long start = m_combatMessageStart;
        long last = m_combatMessages.size() - 2;
        start += delta;
        if (start > last)
            start = last;
        if (start < 0)
            start = 0;
        showMessages(start);
    }
}

VA(0x00472df0, 0x50) MAC_ADDRESS(0x080cb0, 0x4c)  // dc 0x69f6c
int TCombatWindow::scrollUp(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        static_cast<TCombatWindow*>(msg.m_window)->scrollRollover(-1);
        return 1;
    }
    return 0;
}

VA(0x00472e40, 0x50) MAC_ADDRESS(0x080cfc, 0x4c)  // dc 0x69f94
int TCombatWindow::scrollDown(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        static_cast<TCombatWindow*>(msg.m_window)->scrollRollover(1);
        return 1;
    }
    return 0;
}

VA(0x00472e90, 0x35E) MAC_ADDRESS(0x080d48, 0x400)
void TCombatWindow::combatMessage(const char* newText,
                                   bool keep, bool priority)
{
    if (g_combatManager->isQuickCombat())
        return;
    if (!m_controlSubWindow->m_rolloverWidget)
        return;
    if (!g_combatManager->m_combatShowIt)
        return;
    if (g_combatManager->m_battleOver)
        return;

    if (!keep) {
        if (m_combatMessageCount > 0
                && GameTime::elapsedSince(m_combatMessageTime) < 3000
                && !priority)
            return;
        setRollover(newText);
        return;
    }

    std::string temp(newText);
    unsigned int split = temp.find('\n');
    m_combatMessageTime = GameTime::get();
    m_combatMessageCount++;

    if (split == std::string::npos) {
        m_combatMessages.push_back(new std::string(temp));
    } else {
        temp[split] = ' ';
        if (g_smallFont->lineLength(
                temp.c_str(), m_controlSubWindow->m_rolloverWidget->m_width) < 2) {
            m_combatMessages.push_back(new std::string(temp));
        } else {
            m_combatMessageCount++;
            m_combatMessages.push_back(new std::string(temp.substr(0, split)));
            m_combatMessages.push_back(new std::string(
                temp.substr(split + 1, std::string::npos)));
        }
    }

    if (m_combatMessageCount > 2)
        m_combatMessageCount = 2;
    if (!m_controlSubWindow)
        return;
    showMessages(m_combatMessages.size() - m_combatMessageCount);
}

VA(0x004731f0, 0x99) MAC_ADDRESS(0x081148, 0xc0)
void TCombatWindow::endPlacementPhase()
{
    if (m_controlSubWindow) {
        delete m_controlSubWindow;
        m_controlSubWindow = 0;
    }
    m_controlSubWindow = new TCombatControlSubWindow(this);
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    g_windowManager->updateScreen(
        0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
}

VA(0x00473290, 0x52) MAC_ADDRESS(0x081208, 0x8c)  // dc 0x6a264
void TCombatWindow::drawChatText(unsigned char update)
{
    if (m_chatWidget) {
        g_chatMan.updateWidget(m_chatWidget, 1, 20);
        m_chatWidget->draw();
        if (update) {
            g_windowManager->updateScreen(
                m_chatWidget->m_x, m_chatWidget->m_y,
                m_chatWidget->m_width, m_chatWidget->m_height);
        }
    }
}

// Original: TCombatWindow::DrawChatEdit; combatwindow.cpp:615, dc 0x6a2c0
MAC_ADDRESS(0x081294, 0x84)
void TCombatWindow::drawChatEdit(unsigned char update)
{
    if (m_chatEdit && m_chatEdit->m_hasFocus) {
        m_chatEdit->draw();
        if (update) {
            g_windowManager->updateScreen(
                m_chatEdit->m_x, m_chatEdit->m_y,
                m_chatEdit->m_width, m_chatEdit->m_height);
        }
    }
}

VA(0x004732f0, 0x59) MAC_ADDRESS(0x0813b0, 0x44)
void TCombatWindow::drawWindow(unsigned char update, int low, int high)
{
    heroWindow::drawWindow(update, low, high);
    drawChatEdit(update);
}

// E:\gamedcs\combatwindow.cpp:633..649. Original name: OnChatActivate.
// DC calls this ordinary member from SendChat:189 and OnEscape:201.
// Complete expands the same conditional show/hide and subwindow redraw.
// Keep the shared helper and widget::show/hide source calls.
MAC_ADDRESS(0x081318, 0x98)
void TCombatWindow::onChatActivate(unsigned char active)
{
    if (!active) {
        if (m_controlSubWindow) {
            if (m_controlSubWindow->m_rolloverWidget) {
                m_controlSubWindow->m_rolloverWidget->show();
            }
            m_controlSubWindow->draw(1, -0xffff, 0xffff);
        }
    } else {
        if (m_controlSubWindow && m_controlSubWindow->m_rolloverWidget) {
            m_controlSubWindow->m_rolloverWidget->hide();
        }
    }
}

// COMDAT pairing: substr on the char instantiation, mnemonic agreement 1.000.
VA_COMPGEN(0x00473350, 0x1A0, BASIC_STRING_SUBSTR, char)

// COMDAT pairing: find on the char instantiation, mnemonic agreement 0.923.
VA_COMPGEN(0x004734f0, 0x87, BASIC_STRING_FIND, char)

// COMDAT pairing: _Freeze on the char instantiation, mnemonic agreement 0.927.
VA_COMPGEN(0x00473580, 0x76, BASIC_STRING_FREEZE, char)

// COMDAT pairing: vector<std::string*>::insert. Three addresses resemble the
// pointer-element insert and the widget one is already claimed at 0x14d120
// (lane 16, on the caller set); this is the string* instantiation, reached
// from TCombatWindow::combat_message and from TCustomCampaignWindow's
// LoadCampaignList - two different units, which a compiland-local look-alike
// never is.
VA_COMPGEN(0x00473600, 0x209, VECTOR_INSERT, string_ptr)
