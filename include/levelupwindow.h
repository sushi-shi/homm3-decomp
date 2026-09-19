#ifndef HOMM3_LEVELUPWINDOW_H
#define HOMM3_LEVELUPWINDOW_H

#include "advmgr_popup.h"

class hero;
class message;

// Retail's vtable at 0x63fe60 has the inherited CAdvPopup shape: slot 0 is
// the scalar-deleting destructor at 0x4f9700 and slot 9 is WindowHandler at
// 0x4f9780.  The destructor touches no tail state, and the constructor's
// allocation/call sites allocate only the CAdvPopup-sized object; no derived
// data members are presently evidenced.
class TLevelUpWindow : public CAdvPopup {
public:
    enum EOtherWidgetIDs {
        BACKGROUND_ID = 2000,
        PORTRAIT_ID,
        TEXT1_ID,
        TEXT2_ID,
        TEXT3_ID,
        TEXT4_ID,
        TEXT5_ID,
        TEXT6_ID,
        TEXT7_ID,
        PRISKILL_ID,
        SKILLICON_1_ID,
        SKILLICON_2_ID,
        SKILLBORDER_1_ID,
        SKILLBORDER_2_ID
    };
    enum ERetailDialogIDs {
        LEVELUP_ACCEPT_ID = 0x7802
    };
    enum ESelectionKeys {
        LEVELUP_SELECT_LEFT_KEY = 2,
        LEVELUP_SELECT_RIGHT_KEY = 3
    };

    TLevelUpWindow(hero* thisHero, int gainedSkill,
                   int firstChoice, int secondChoice);
    virtual ~TLevelUpWindow();
    virtual int windowHandler(message& msg); // slot 9

    int m_leftSkill;   // +0x60 retail (+0x58 DC)
    int m_rightSkill;  // +0x64 retail (+0x5c DC)

private:
    int m_selected;     // +0x68 retail (+0x60 DC)
};
SIZE(TLevelUpWindow, 0x6c);

#endif  /* HOMM3_LEVELUPWINDOW_H */
