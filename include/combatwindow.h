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
class Bitmap16Bit;

// Eleven interleaved rollover/right-click rows at retail 0x6a6968. The
// combat-window right-click handler consumes the same table as the combat
// sub-window constructors.

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

    // Project-inferred owner boundary for the complete data block. DC type
    // 0x43dc is only a forward declaration, so it supplies no access levels.
    // Keep the retail field order; callers borrow panels through the public
    // interface while the window owns chat, message history and panel lifetime.
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

public:
    virtual ~TCombatWindow();
    virtual void close(unsigned char update);
    virtual void handleWidgetHover(widget* currentWidget);
    // Original DC DrawWindow@TCombatWindow@@UAAX_NHH@Z proves bool.
    virtual void drawWindow(bool update, int low, int high);
    void clearCombatMessages();
    static int convertID2HelpID(int id);
    bool processRightSelect(const message& msg);
    void setRollover(const char* newText);
    void showMessages(long start);
    void scrollRollover(long delta);
    static int scrollUp(message& msg);
    static int scrollDown(message& msg);
    TCombatWindow(unsigned char doPlacement);
    void endPlacementPhase();
    void combatMessage(const char* newText, bool keep,
                        bool priority);
    void drawChatText(bool update);
    void drawChatEdit(bool update);
    void onChatActivate(bool active);

};
SIZE(TCombatWindow, 0x8c);

#endif  /* HOMM3_COMBATWINDOW_H */
