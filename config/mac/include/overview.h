// Mac declaration view for source-owned overview.cpp bodies. The target
// setupNewOverviewType span is code 0:0x135d54..0x136128.
#ifndef HOMM3_MAC_OVERVIEW_H
#define HOMM3_MAC_OVERVIEW_H

#include "va.h"
#include "message_record.h"

extern "C" char* strcpy(char*, const char*);

class playerData {
    char m_beforeNumHeroes[1];
public:
    signed char m_numHeroes;  // Mac getLocalPlayer result +1.
private:
    char m_beforeNumTowns[0x3e - 2];
public:
    signed char m_numTowns;  // Mac getLocalPlayer result +0x3e.
};
class game {
public:
    playerData* getLocalPlayer();
    void setupNewOverviewType(int whichType, unsigned char update);
    void setupDynamicStuff(int forceUpdate, int update);
};
extern game* g_game;

class widget {
public:
    virtual ~widget();
    enum { WIDGET_SET_STATUS = 5, WIDGET_CLEAR_STATUS = 6 };
};
class slider : public widget {
public:
    // Mac target dispatches at vtable +0x3c/+0x40.
    virtual void macSlot1(); virtual void macSlot2();
    virtual void macSlot3(); virtual void macSlot4();
    virtual void macSlot5(); virtual void macSlot6();
    virtual void macSlot7(); virtual void macSlot8();
    virtual void macSlot9(); virtual void macSlot10();
    virtual void macSlot11(); virtual void macSlot12();
    virtual void macSlot13(); virtual void macSlot14();
    virtual void setResolution(int);
    virtual void setState(int);
};
class heroWindow {
public:
    virtual ~heroWindow();
    virtual void macSlot1(); virtual void macSlot2();
    virtual void macSlot3(); virtual void macSlot4();
    virtual void macSlot5(); virtual void macSlot6();
    virtual void drawWindow(unsigned char, int, int);  // vtable +0x1c.
    int broadcastMessage(message&);
    void removeWidget(widget*);
    void addWidget(widget*, int);
};

namespace font {
enum TColor { PRIMARY = 1 };
enum EJustify { CENTER_JUSTIFIED = 1 };
}
class textWidget : public widget {
    char m_tail[0x44 - 4];  // Mac allocation size at 0:0x135fa4.
public:
    textWidget(int, int, int, int, const char*, const char*,
               font::TColor, int, unsigned, int, int);
};

extern int g_overviewType;
extern int g_overviewItemCount;
extern int g_overviewItemCounts[2];
extern int g_overviewTop[2];
extern slider* g_overviewSlider;
extern heroWindow* g_overWin;
extern textWidget* g_textWidgetTitle[3];
extern const char* g_overviewText[16];
extern char g_text[];
enum { MESSAGE_WIDGET = 0x200 };

#endif
