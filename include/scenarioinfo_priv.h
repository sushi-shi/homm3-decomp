// scenarioinfo_priv.h - Complete-only private widget of scenarioinfo.cpp.
#ifndef HOMM3_SCENARIOINFO_PRIV_H
#define HOMM3_SCENARIOINFO_PRIV_H

#include "bitmap816.h"
#include "csprite.h"
#include "hero.h"
#include "message.h"
#include "widget.h"

// Complete adds this compact row renderer to the scenario-info dialog. It
// has no Dreamcast counterpart, so the class name is role-derived; its base,
// complete 0x60-byte layout, vtable shape, and owned portrait are retail
// facts from 0x5680fd..0x56822c and 0x5693a0..0x5697c4.
class CScenarioPlayerInfoWidget : public widget {
public:
    // Before normalization: panel.
    Bitmap816* m_panel;                 // +0x30
    // Before normalization: flag.
    Bitmap816* m_flag;                  // +0x34
    // Before normalization: townSprite.
    CSprite* m_townSprite;              // +0x38
    // Before normalization: townType.
    int m_townType;                     // +0x3c
    // Before normalization: playerName.
    const char* m_playerName;           // +0x40
    // Before normalization: handicapText.
    const char* m_handicapText;         // +0x44
    // Before normalization: playerTypeText.
    const char* m_playerTypeText;       // +0x48
    // Before normalization: playerPosition.
    int m_playerPosition;               // +0x4c
    // Before normalization: startingBonus.
    int m_startingBonus;                // +0x50
    // Before normalization: bonusSprite.
    CSprite* m_bonusSprite;             // +0x54
    // Before normalization: heroPortrait.
    Bitmap816* m_heroPortrait;          // +0x58, owned
    // Before normalization: startingHero.
    hero* m_startingHero;               // +0x5c

    // Retail's inlined constructor ends with `mov word ptr [edi+0x10], dx`
    // (0x5680ec) - the widget id, `390 + playerPosition`, which is exactly
    // the row ProcessRightSelect fetches back with GetWidget to reach
    // heroPortrait.  The store sits INSIDE the new-expression's
    // allocation-succeeded arm, so it is the constructor's, not the caller's.
    CScenarioPlayerInfoWidget(CSprite* town, int widgetId)
    {
        m_townSprite = town;
        m_townType = 0;
        m_panel = 0;
        m_flag = 0;
        m_playerName = 0;
        m_handicapText = 0;
        m_playerTypeText = 0;
        m_playerPosition = 0;
        m_bonusSprite = 0;
        m_heroPortrait = 0;
        m_startingBonus = 4;
        m_startingHero = 0;
        m_id = widgetId;
    }

    virtual ~CScenarioPlayerInfoWidget();
    // Before normalization (function): CScenarioPlayerInfoWidget::Main.
    virtual int main(message& msg) { return widget::main(msg); }
    virtual void zBufferDraw(unsigned short*, int) const {}
    // Before normalization (function): CScenarioPlayerInfoWidget::Draw.
    virtual void draw();
};
SIZE(CScenarioPlayerInfoWidget, 0x60);

#endif  /* HOMM3_SCENARIOINFO_PRIV_H */
