// Mac declaration view for the source-owned TSellCreatureWindow::update body.
// Retail counterpart is pinned at code 0:0x1f7964..0x1f8144.
#ifndef HOMM3_MAC_TRADPOST_H
#define HOMM3_MAC_TRADPOST_H

#include "va.h"
#include "message_record.h"

extern "C" int sprintf(char*, const char*, ...);
extern "C" char* strcpy(char*, const char*);

enum { MESSAGE_WIDGET = 0x200, MARKET_SOURCE_FREELANCER = 3 };

namespace std {
template <class T> class vector {
    char m_beforeFirst[8];
    T* m_first;
public:
    T& operator[](int index) const;
};
}
class TTextResource {
    char m_beforeText[0x1c];
public:
    std::vector<char*> m_text;
#include "mac_shared/textresource_get_text.h"
#include "mac_shared/textresource_index.h"
};
extern const TTextResource* g_generalText;

class armyGroup {
public:
    int m_armies[7];
    int m_numTroops[7];
};
class hero {
    char m_beforeName[0x23];
public:
    char m_name[13];
private:
    char m_beforeArmy[0x91 - 0x23 - 13];
public:
    armyGroup m_army;
};
struct TCreatureTypeTraits {
    char m_beforeName[0x14];
    const char* m_name;
    const char* m_pluralName;
    char m_afterNames[0x74 - 0x1c];
};
extern const TCreatureTypeTraits (&g_creatureTypeTraits)[150];
extern const char* g_resourceNames[7];

class widget {
public:
    virtual void macSlot0(); virtual void macSlot1();
    virtual void macSlot2(); virtual void macSlot3();
    virtual void macSlot4(); virtual void macSlot5();
    virtual void macSlot6(); virtual void macSlot7();
    virtual void macSlot8(); virtual void macSlot9();
    virtual void macSlot10();
    virtual void enable(unsigned char);
};
class slider : public widget {
public:
    virtual void macSlot12(); virtual void macSlot13();
    virtual void macSlot14(); virtual void macSlot15();
    virtual void setState(int);
};
class heroWindow {
public:
    virtual void macSlot0(); virtual void macSlot1();
    virtual void macSlot2(); virtual void macSlot3();
    virtual void macSlot4(); virtual void macSlot5();
    virtual void macSlot6();
    virtual void drawWindow(unsigned char, int, int);
    int broadcastMessage(message&);
};
class CAdvPopup : public heroWindow {
    char m_toSellCreatureSlider[0x5c - 4];
};
class TSellCreatureWindow : public CAdvPopup {
    slider* m_creatureSlider;
    int m_lastHoverId;
public:
    void setWidgetOn(short);
    void setWidgetOff(short);
    void setWidgetDisabled(short);
    void update(bool);
    void computeTradeRatios(int, int, int*, int*, int*);
};

struct THelpText { const char* m_text; const char* m_rclick; };
extern THelpText g_sellCreaHelpText[5];
extern hero* g_marketHero;
extern int g_selectedArtifact;
extern int g_leftResource;
extern int g_ratioInverted;
extern int g_leftDenominated;
extern int g_giveQuantity;
extern int g_rightAmount;
extern int g_marketSource;
extern int g_creatureRowY[7];
extern char g_text[];
extern const char g_emptyRolloverText[];

#endif
