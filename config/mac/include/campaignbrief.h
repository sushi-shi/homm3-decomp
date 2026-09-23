// Declaration view for the independently bounded Mac ally/enemy flag body at
// 0:0x64584..0x64720. All offsets below are reads in that retained body.
#ifndef HOMM3_MAC_CAMPAIGNBRIEF_H
#define HOMM3_MAC_CAMPAIGNBRIEF_H

#include "va.h"
#include "message_record.h"

enum { MAP_SIZE_SMALL = 36, MAP_SIZE_MEDIUM = 72,
       MAP_SIZE_LARGE = 108, MAP_SIZE_EXTRA_LARGE = 144,
       MESSAGE_WIDGET = 0x200 };

class CampaignScenarioPreview;

namespace std {
class string;
template<class T> class vector {
    char m_beforeFirst[8];
    T* m_first;
public:
    T* const* begin() const;
    T& operator[](int index) const { return (*begin())[index]; }
};
}

class TAbstractFile;
class widget {
public:
    enum ECommands {
        WIDGET_SET_ICON_FRAME = 4,
        WIDGET_SET_STATUS = 5,
        WIDGET_CLEAR_STATUS = 6
    };
    enum EStatusFlags { WIDGET_ACTIVE = 2, WIDGET_DRAWN = 4 };
    virtual ~widget();
    virtual int open(int newPriority, class heroWindow* parent);
    virtual int main(message& msg) = 0;
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const = 0;
    virtual void draw() const = 0;
    virtual int getRealHeight() const;
    virtual int getRealWidth() const;
    virtual void processHover();
    virtual void dim() const;
    virtual void enable(unsigned char on);
    int sendMessage(ECommands command, int extra);
#include "inline/widget_hide.inl"
#include "inline/widget_show.inl"
};

class heroWindow {
public:
    virtual ~heroWindow();
    virtual int open(int zOrder, unsigned char update);
    virtual void close(unsigned char update);
    virtual int handleMessage(message& msg);
    virtual void handleWidgetHover(widget* w);
    virtual void drawWindow(unsigned char update, int lowID, int highID);
    virtual void doModal(bool fadeIn);
    widget* getWidget(int id);
    int broadcastMessage(message& msg);
};

// This covers the offsets read by select and updateAllyEnemyFlags. The
// canonical CMapHeaderData string members are not represented yet, so this
// selected-body view cannot reproduce the implicit Mac map-header assignment.
struct NewSMapHeader {
    char m_beforeTeamInfo[0xd];
    signed char m_teamInfo[8];
    char m_beforeSize[0x18 - 0x15];
    int m_size;
    char m_afterSize[0x2c8 - 0x1c];
};
struct SGameSetupOptions {
    char m_beforePlayerPos[0x30];
    signed char m_playerPos[8];  // Mac game+0x1ef5c.
    char m_afterPlayerPos[0x1cc - 0x38];
};
class CampaignScenarioPreview : public NewSMapHeader {
public:
    SGameSetupOptions m_gameSetup;  // Mac scenario+0x2c8.
    unsigned char m_available;  // Mac scenario+0x494.
    char m_afterAvailable[3];
};
namespace std {
template<> class vector<CampaignScenarioPreview> {
    char m_beforeFirst[8];
    CampaignScenarioPreview* m_first;
public:
    CampaignScenarioPreview& operator[](int index) const { return m_first[index]; }
};
}

class TCampaignStartOption {
public:
    virtual ~TCampaignStartOption();
    virtual bool isBuildingBonus(int which) const = 0;
    virtual int getCount() const = 0;
    virtual const char* getIconDefName(void* scenario, int which) const = 0;
    virtual int getIconIndex(int which) const = 0;
    virtual int slot5(void* scenario, int which) const;
    virtual std::string getText(void* scenario, int which) const = 0;
    virtual int slot7(int which) const;
    virtual int getPlayer(int which) const = 0;
};

class TCampaignBrief : public heroWindow {
public:
    struct ScenarioStruct {
        char m_beforeOptions[0x84];
        TCampaignStartOption* m_options;  // Mac option vtable dispatch at +0x28.
    };
    struct CampaignHeaderStruct {
        char m_beforeScenarios[0x18];
        std::vector<ScenarioStruct*> m_scenarios;
    };
    enum { ALLY_FLAG1_ID = 113, ENEMY_FLAG1_ID = 121,
           MAP_SELECTED_1_ID = 194, WHICHMAP_ID = 129,
           DIALOG_RETURN_OK = 0x7802 };
private:
    char m_beforeScenarios[0x50 - sizeof(heroWindow)];
public:
    std::vector<CampaignScenarioPreview> m_scenarios;
public:
    CampaignHeaderStruct* m_campaign;
private:
    char m_beforeSelectedScenario[4];
public:
    int m_selectedScenario;
    void select(int which);
    void clearSelected();
    void resetMapAndDescription(int which);
    void updateBonusIcons();
    void updateDifficultyButtons();
    void updateAllyEnemyFlags();
};
struct SCampaign {
    char m_beforeBriefingChoice[0x10];
    int m_briefingChoice;  // Mac game+0x1ed14.
};
class game {
    char m_beforeCampaign[0x1ed04];
public:
    SCampaign m_campaign;
private:
    char m_beforeSetup[0x1ef2c - 0x1ed04 - sizeof(SCampaign)];
public:
    SGameSetupOptions m_setup;
private:
public:
    NewSMapHeader m_mapHeader;
#include "inline/game_on_same_team.inl"
};
extern game* g_game;  // Mac TOC 1+0x630 -> pointer storage 1+0x528940.
// Canonical src/campaignbrief.cpp owns this same-TU byte at retail
// DATA(0x00694de8); select reads the Mac byte directly from TOC 1+0x43ec.
static unsigned char g_campaignBriefViewFromGame;
// The body-only compilation view refers to the source-owned file-static slot
// defined outside this selected body. Mac TOC 1+0x95c points to 1+0x659f0.
extern int g_campaignBriefPlayerSlot;

#endif
