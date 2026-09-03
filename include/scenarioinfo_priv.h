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
    Bitmap816* panel;                 // +0x30
    Bitmap816* flag;                  // +0x34
    CSprite* townSprite;              // +0x38
    int townType;                     // +0x3c
    const char* playerName;           // +0x40
    const char* handicapText;         // +0x44
    const char* playerTypeText;       // +0x48
    int playerPosition;               // +0x4c
    int startingBonus;                // +0x50
    CSprite* bonusSprite;             // +0x54
    Bitmap816* heroPortrait;          // +0x58, owned
    hero* startingHero;               // +0x5c

    CScenarioPlayerInfoWidget(CSprite* town)
    {
        townSprite = town;
        townType = 0;
        panel = 0;
        flag = 0;
        playerName = 0;
        handicapText = 0;
        playerTypeText = 0;
        playerPosition = 0;
        bonusSprite = 0;
        heroPortrait = 0;
        startingBonus = 4;
        startingHero = 0;
    }

    virtual ~CScenarioPlayerInfoWidget();
    virtual int Main(message* msg) { return widget::Main(msg); }
    virtual void zBufferDraw(unsigned short*, int) {}
    virtual void Draw();
};
SIZE(CScenarioPlayerInfoWidget, 0x60);

#endif  /* HOMM3_SCENARIOINFO_PRIV_H */
