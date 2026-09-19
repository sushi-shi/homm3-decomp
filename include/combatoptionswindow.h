// combatoptionswindow.h - combatoptionswindow.cpp (compiland combatoptionswindow.obj)
#ifndef HOMM3_COMBATOPTIONSWINDOW_H
#define HOMM3_COMBATOPTIONSWINDOW_H

#include "window.h"

class message;
class textWidget;

// Retail's destructor proves the heroWindow base and vtable 0x63d448.
// DoModal reads/writes bPrefsChanged at +0x4c; the constructor's final
// derived-member store and the Dreamcast field list identify the pointer at
// +0x50 as RolloverWidget. Total size is independently proven by
// combatManager::CombatSystemOptions' 0x54-byte stack object.
class TCombatOptionsWindow : public heroWindow {
public:
    // Widget ids, byte-proven by the constructor's creation order and the
    // post-AddWidget preference sweep (every id below is either a literal
    // in a widget constructor or a loop bound in that sweep). Names follow
    // the preference field each id drives; the four Highlight* Dreamcast
    // methods corroborate the grid/shadow/speed group.
    enum EOtherWidgetIDs {
        BACKGROUND_ID = 200,
        DEFAULT_ID = 201,
        MUSIC_VOLUME_0_ID = 202,
        MUSIC_VOLUME_1_ID = 203,
        MUSIC_VOLUME_2_ID = 204,
        MUSIC_VOLUME_3_ID = 205,
        MUSIC_VOLUME_4_ID = 206,
        MUSIC_VOLUME_5_ID = 207,
        MUSIC_VOLUME_6_ID = 208,
        MUSIC_VOLUME_7_ID = 209,
        MUSIC_VOLUME_8_ID = 210,
        MUSIC_VOLUME_9_ID = 211,
        EFFECTS_VOLUME_0_ID = 212,
        EFFECTS_VOLUME_1_ID = 213,
        EFFECTS_VOLUME_2_ID = 214,
        EFFECTS_VOLUME_3_ID = 215,
        EFFECTS_VOLUME_4_ID = 216,
        EFFECTS_VOLUME_5_ID = 217,
        EFFECTS_VOLUME_6_ID = 218,
        EFFECTS_VOLUME_7_ID = 219,
        EFFECTS_VOLUME_8_ID = 220,
        EFFECTS_VOLUME_9_ID = 221,
        AUTO_CREATURES_ID = 225,
        AUTO_SPELLS_ID = 226,
        AUTO_CATAPULT_ID = 227,
        AUTO_BALLISTA_ID = 228,
        AUTO_FIRST_AID_TENT_ID = 229,
        COMBAT_SPEED_0_ID = 230,
        COMBAT_SPEED_1_ID = 231,
        COMBAT_SPEED_2_ID = 232,
        CREATURE_INFO_VERBOSE_ID = 233,
        CREATURE_INFO_COMPACT_ID = 234,
        SHOW_GRID_ID = 235,
        MOVEMENT_SHADOW_ID = 236,
        MOUSE_SHADOW_ID = 237,
        ANIMATE_SPELLBOOK_ID = 238
    };

    // The domain of the prefs block's combatArmyInfoLevel, byte-proven by
    // the constructor: it lights CREATURE_INFO_VERBOSE_ID when the field is
    // 1 and CREATURE_INFO_COMPACT_ID when it is 2.
    enum ECreatureInfoLevel {
        CREATURE_INFO_LEVEL_VERBOSE = 1,
        CREATURE_INFO_LEVEL_COMPACT = 2
    };

    unsigned char m_prefsChanged;  // +0x4c
    // The preceding byte field and following four-byte field establish
    // this alignment gap; the reference layout retains the same boundary.
    char m_paddingBeforeRolloverWidget[3];

    TCombatOptionsWindow();
    virtual ~TCombatOptionsWindow();
    void doModal();

private:
    textWidget* m_rolloverWidget;   // +0x50
    int convertID2HelpID(int id) const;

public:
    // DC CombatOptionsWindowHandler (0x67b7c) directly calls these private
    // methods; retail 0x46f7b0 expands them. Preserve the callback friendship.
    friend int combatOptionsWindowHandler(message& msg);

private:
    void highlightCombatSpeed();
    void highlightGrid();
    void highlightMovementShadow();
    void highlightMouseShadow();
};
SIZE(TCombatOptionsWindow, 0x54);

// Retail /Gr passes the message by reference in ECX, matching DoDialog's
// byte-proven TDialogHandler type.
int combatOptionsWindowHandler(message& msg);

// The rollover/right-click pairs this dialog's help path indexes with
// convertID2HelpID's answer. Stride 8 and base 0x6a55ac are byte-proven by
// the handler's `mov ecx,[8*eax + 0x6a55ac]`; the ID mapping reaches 38,
// so at least 39 rows exist (the next initialised datum is 0x6a5704, which
// leaves room for 43). Definition + DATA claim in src/combatoptionswindow.cpp.
extern THelpText g_combatOptionsHelp[39];

// The "Default" button's callee is misc.obj's SetDefaultCombatOptions
// (declared in misc.h, defined in src/misc.cpp) - NOT a new function: its
// ten stores into the 0x698758 prefs block are exactly the fields this
// dialog's Default case re-reads afterwards, and the register-shared form
// misc.cpp spells (`xor eax,eax` / `mov eax,1` feeding ten six-byte
// stores) accounts for retail 0x50b700's 0x44 bytes exactly. Declared
// once in its owner's header; this TU just includes misc.h.

#endif  /* HOMM3_COMBATOPTIONSWINDOW_H */
