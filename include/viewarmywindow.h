#ifndef HOMM3_VIEWARMYWINDOW_H
#define HOMM3_VIEWARMYWINDOW_H

#include <string>
#include "advmgr_popup.h"
#include "armygrp.h"

class army;
class armyGroup;
class hero;
class town;
class bitmapBackedTextWidget;
class iconWidget;
struct TCreatureTypeTraits;

// DC's named tail begins at +0x58 after its 0x58-byte CAdvPopup. Retail's
// proven CAdvPopup is 0x60, and each of the two STLport 12-byte strings
// expands to a 16-byte VC6 string. Retail's destructor independently
// confirms their data pointers at +0x70/+0x84 and the total allocation
// sites bound the resulting 0xb8-byte object.
// The DC class records agree on method order and private helper/member
// ownership. Retail allocation and member accesses independently fix layout.
class TViewArmyWindow : public CAdvPopup {
public:
    // Complete retains public action IDs; the older DC enum records UPGRADE_ID.
    enum EWidgetIDs {
        UPGRADE_ID = 300,
        OK_ID = 301,
        ACCEPT_ID = 0x7802,
        DISMISS_ID = 0x7803
    };

    TViewArmyWindow(int armyType, int x0, int y0, unsigned char showOk);
    TViewArmyWindow(const army* thisArmy, int x0, int y0,
                    unsigned char showOk);
    // Complete adds the tenth groupAlignments argument (ret 0x28) and
    // uses the mutable group pointer required by GetArmyMorale/GetArmyLuck.
    TViewArmyWindow(armyGroup* group, int iarmy, const hero* thisHero,
                    const town* thisTown, int x0, int y0, int upgrade,
                    unsigned char showDismiss, unsigned char showOk,
                    unsigned char groupAlignments);
    virtual ~TViewArmyWindow();
    void doModal();
    void quickView();
    virtual int windowHandler(message& msg);

private:
    enum EOtherWidgetIDs {
        BACKGROUND_ID = 200,
        SPRITE_ID = 201,
        SPRITE_BACKGROUND_ID = 202,
        NAME_ID = 203,
        NUMBER_ID = 204,
        ATTACK_LABEL_ID = 205,
        ATTACK_ID = 206,
        DEFENSE_LABEL_ID = 207,
        DEFENSE_ID = 208,
        SHOTS_LABEL_ID = 209,
        SHOTS_ID = 210,
        DAMAGE_LABEL_ID = 211,
        DAMAGE_ID = 212,
        HEALTH_LABEL_ID = 213,
        HEALTH_ID = 214,
        HEALTH_REMAINING_LABEL_ID = 215,
        HEALTH_REMAINING_ID = 216,
        SPEED_LABEL_ID = 217,
        SPEED_ID = 218,
        MORALE_ID = 219,
        LUCK_ID = 220,
        AFFECTING_SPELLS_0_ID = 221,
        AFFECTING_SPELLS_1_ID = 222,
        AFFECTING_SPELLS_2_ID = 223,
        // The bitmapBackedTextWidget the one-army constructor parks in
        // RolloverWidget: id 0xe0 pushed at 0x5f3940, one past the
        // spell row and the only widget id in the popup that is not
        // already named.
        ROLLOVER_ID = 224,
        OK_BORDER_ID = 225
    };
    enum { NWIDGETS = 28, VIEW_ARMY_DELAY = 100, NSPELLS = 3 };

    // Original declaration order from both complete DC class records,
    // 0x1a93/0x4aff. Bodies and named source-call boundaries remain in the TU.
    void createBackgroundWidget(const hero* thisHero);
    void createNameWidget(const char* name);
    void createPortraitWidget(const char* spriteName, int townType, int count);
    void createAttackWidget(int normalAttackSkill, int currentAttackSkill);
    void createDefenseWidget(int normalDefenseSkill, int currentDefenseSkill);
    void createDamageWidget(const TCreatureTypeTraits& traits, const hero* ourHero);
    void createShotsWidget(const TCreatureTypeTraits& traits,
                           int normalShots, int currentShots);
    void createHitpointsWidget(int normalHitpoints, int currentHitpoints);
    void createHitpointsLeftWidget(int hitpointsLeft);
    void createSpeedWidget(int normalSpeed, int currentSpeed);
    void createMoraleWidget(int newMorale);
    void createLuckWidget(int newLuck);
    void createSpellInfluenceWidgets(const army* thisArmy);
    void createOkWidget();
    void createUpgradeWidget();
    void createDismissWidget();
    void createRolloverWidget();
    int convertID2HelpID(int id) const;

    // retail keeps this four-byte field at +0x60. Upgrade below is int.
    TCreatureType m_armyType;
    int m_armySize;
    int m_morale;
    std::string m_moraleHelp;
    int m_luck;
    std::string m_luckHelp;
    int m_upgrade;
    unsigned char m_showingUpgradeButton;
    unsigned char m_showingDismissButton;
    unsigned char m_showingOkButton;
    // The three flag bytes leave one byte of natural four-byte alignment
    // before Influence. DC records 14 real members and no padding field.
    int m_influence[3];
    int m_duration[3];
    bitmapBackedTextWidget* m_rolloverWidget;
    iconWidget* m_spriteWidget;

};
SIZE(TViewArmyWindow, 0xb8);

#endif  /* HOMM3_VIEWARMYWINDOW_H */
