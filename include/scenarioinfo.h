#ifndef HOMM3_SCENARIOINFO_H
#define HOMM3_SCENARIOINFO_H

#include "advmgr_popup.h"

class CSprite;
class Bitmap816;

// Dreamcast publishes the original name; retail scenarioinfo.obj passes this
// exact 0x6a6ca0 table to CHeroWindowEx::SetHelpText.
DATA(0x006a6ca0) extern THelpText g_singleSelectionHelp[];

// Retail's stack owner at 0x513740 reserves 0xb4 bytes for this object.
// The vtable at 0x641710 has the inherited 15-slot CAdvPopup shape, with
// retail overrides in slots 0, 11, and 12. The sprite fields and their order
// are proven by the retail destructor at 0x569800; the names come from DC
// CodeView and retain the retail build's +8 base-class displacement.
class CScenarioInfoDlg : public CAdvPopup {
public:
    // The hotspot bands the constructor lays over each player row and the
    // row renderer's own widget id, all byte-proven by ProcessRightSelect's
    // 26-entry jump table at 0x569ca0/0x569cb4 (base 362, 8+8+8 plus the
    // single team plate at 387) and by the `id = 390 + i` store the row
    // constructor emits at 0x5680ec.
    enum EWidgetIDs {
        SCENARIO_INFO_ALLY_FIRST_ID = 112,
        SCENARIO_INFO_ENEMY_FIRST_ID = 120,
        SCENARIO_INFO_ACCEPT_ID = 188,
        SCENARIO_INFO_HERO_FIRST_ID = 362,
        SCENARIO_INFO_TOWN_FIRST_ID = 370,
        SCENARIO_INFO_BONUS_FIRST_ID = 378,
        SCENARIO_INFO_TEAM_ID = 387,
        SCENARIO_INFO_PLAYER_ROW_FIRST_ID = 390
    };

    CSprite* m_victoryIcon;              // +0x60
    CSprite* m_lossIcon;                 // +0x64
    CSprite* m_townPix;                  // +0x68
    CSprite* m_bonusSprite;              // +0x6c
    Bitmap816* m_panels[8];              // +0x70
    Bitmap816* m_flags[8];               // +0x90
    CSprite* m_heroSpecificAbility;      // +0xb0

    CScenarioInfoDlg();
    virtual ~CScenarioInfoDlg();
    void updateAllyEnemyFlags();
    virtual unsigned char processRightSelect(int id);
    virtual int onWidgetDeselect(int id, bool& exitFlag);
    void setDifficultyHiLite();
};
SIZE(CScenarioInfoDlg, 0xb4);

#endif  /* HOMM3_SCENARIOINFO_H */
