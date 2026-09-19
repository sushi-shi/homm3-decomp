// combatwindow.h - prototypes of combatwindow.cpp (compiland combatwindow.obj)
#ifndef HOMM3_COMBATWINDOW_H
#define HOMM3_COMBATWINDOW_H

#include <string>
#include "window.h"

class TSubWindow;
class TCombatCreatureSubWindow;
class TCombatHeroSubWindow;
class textWidget;
class textEntryWidget;
class type_combat_sub_window;
class message;

// Eleven interleaved rollover/right-click rows at retail 0x6a6968. The
// combat-window right-click handler consumes the same table as the combat
// sub-window constructors.
extern THelpText g_combatSubWindowHelp[11];

// Retail vtable 0x63d528 and Close independently prove the heroWindow base;
// combatManager::Open allocates the complete 0x8c-byte object. Close deletes
// the polymorphic combat-control subwindow at +0x70 before delegating to
// heroWindow::Close. DrawCreatureAndHeroSubwindows independently proves the
// two hero panels and four creature panels that fill the remaining tail.
class TCombatWindow : public heroWindow {
public:
    enum EWidgetIds {
        COMBAT_LEFT_COMMAND_0_ID = 0x7d1,
        COMBAT_LEFT_COMMAND_1_ID = 0x7d2,
        COMBAT_LEFT_COMMAND_2_ID = 0x7d3,
        COMBAT_LEFT_COMMAND_3_ID = 0x7d4,
        COMBAT_ROLLOVER_ID = 0x7d5,
        COMBAT_LOG_SCROLL_UP_ID = 0x7d6,
        COMBAT_LOG_SCROLL_DOWN_ID = 0x7d7,
        COMBAT_RIGHT_COMMAND_0_ID = 0x7d8,
        COMBAT_RIGHT_COMMAND_1_ID = 0x7d9,
        COMBAT_RIGHT_COMMAND_2_ID = 0x7da,
        COMBAT_PLACEMENT_COMMAND_0_ID = 0x8fc,
        COMBAT_PLACEMENT_COMMAND_1_ID = 0x7802
    };

    // combat_message and handle_widget_hover both follow this pointer to
    // textEntryWidget::bHasFocus at +0x6d. The constructor initially nulls
    // it; the concrete object is the combat chat editor.
    textEntryWidget* m_chatEdit;
    // DrawChatText and DrawFrame both load the same pointer at retail +0x50;
    // its DC counterpart is likewise the combat chat text widget.
    textWidget* m_chatWidget;
    // The four-word VC6 vector begins at +0x54; its pointer triplet at
    // +0x58/+0x5c/+0x60 is byte-proven by combat_message, scroll_rollover,
    // and the destructor.
    std::vector<std::string*> m_combatMessages;
    int m_combatMessageCount;
    int m_combatMessageStart;
    unsigned long m_combatMessageTime;
    type_combat_sub_window* m_controlSubWindow;
    TCombatHeroSubWindow* m_heroSubWindows[2];
    TCombatCreatureSubWindow* m_creatureSubWindows[4];

    virtual ~TCombatWindow();
    virtual void close(unsigned char update);
    virtual void handleWidgetHover(widget* currentWidget);
    virtual void drawWindow(unsigned char update, int low, int high);
    void clearCombatMessages();
    static int convertID2HelpID(int id);
    unsigned char processRightSelect(const message* msg);
    void setRollover(const char* newText);
    void showMessages(long start);
    void scrollRollover(long delta);
    static int scrollUp(message& msg);
    static int scrollDown(message& msg);
    TCombatWindow(unsigned char doPlacement);
    void endPlacementPhase();
    void combatMessage(const char* newText, bool keep,
                        bool priority);
    void drawChatText(unsigned char update);
    void onChatActivate(unsigned char active);
};
SIZE(TCombatWindow, 0x8c);

// --- globals ---
// CODEVIEW(E:\gamedcs\combatwindow.cpp:42, dc 0x69638) void CheckCombatCheatCode(std::basic_string<char,std::char_traits<char>,std::allocator<char>* chatString);

// --- CCombatChatEdit ---
// CODEVIEW(E:\gamedcs\combatwindow.cpp:139, dc 0x6a3ec) void CCombatChatEdit::CCombatChatEdit(int textWidgetX, int textWidgetY, int textWidgetWidth, int textWidgetHeight, int textStringSize, char* textString, char* textFontName, int colorIndex, font::EJustify justification, char* backgroundIconName, int backgroundFrame, int textWidgetId, int textWidgetStyle, int iReadType, int textInsetX, int textInsetY);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:171, dc 0x6a488) void CCombatChatEdit::SendChat(const char* sChat, int toWho);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:213, dc 0x6a5dc) void* CCombatChatEdit::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:213, dc 0x6a610) void CCombatChatEdit::~CCombatChatEdit();

// --- TCombatWindow ---
// CODEVIEW(E:\gamedcs\combatwindow.cpp:293, dc 0x69b6c) void TCombatWindow::~TCombatWindow();
// CODEVIEW(E:\gamedcs\combatwindow.cpp:322, dc 0x69c70) int TCombatWindow::convertID2HelpID(int id);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:356, dc 0x69cdc) unsigned char TCombatWindow::ProcessRightSelect(const message* msg);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:382, dc 0x69d5c) void TCombatWindow::set_rollover(const char* new_text);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:393, dc 0x69da8) void TCombatWindow::handle_widget_hover(widget* current_widget);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:417, dc 0x69e00) void TCombatWindow::ClearCombatMessages();
// CODEVIEW(E:\gamedcs\combatwindow.cpp:431, dc 0x69e3c) void TCombatWindow::show_messages(long start);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:456, dc 0x69f24) void TCombatWindow::scroll_rollover(long delta);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:508, dc 0x69fbc) void TCombatWindow::combat_message(const char* new_text, unsigned char keep, unsigned char priority);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:583, dc 0x6a200) void TCombatWindow::EndPlacementPhase();
// CODEVIEW(E:\gamedcs\combatwindow.cpp:615, dc 0x6a2c0) void TCombatWindow::DrawChatEdit(unsigned char update);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:633, dc 0x6a310) void TCombatWindow::OnChatActivate(unsigned char active);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:652, dc 0x6a3c0) void TCombatWindow::DrawWindow(unsigned char update, int low, int high);
// CODEVIEW(E:\gamedcs\combatwindow.cpp:278, dc 0x6a628) void* TCombatWindow::`scalar deleting destructor'(unsigned __flags);

// --- std ---
// CODEVIEW(E:\gamedcs\combatwindow.cpp:313, dc 0x6a65c) void* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_COMBATWINDOW_H */
