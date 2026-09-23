// Declaration view for the canonical advmgr.cpp setHeroContext body.
// Offsets below are read from the bounded Mac body at 0+0x18220..0x187b4.
#ifndef HOMM3_MAC_ADVMGR_H
#define HOMM3_MAC_ADVMGR_H

#define HOMM3_TARGET_MAC 1
#include "va.h"

enum { MESSAGE_WIDGET = 0x200, STATUS_ACTIVE = 1,
       HOVER_SCREEN_WIDTH = 800, HOVER_SCREEN_HEIGHT = 600 };
enum { ADV_SCROLL_POINTER = 32, ADV_SCROLL_NORTH = 32,
       ADV_SCROLL_NORTHEAST = 33, ADV_SCROLL_EAST = 34,
       ADV_SCROLL_SOUTHEAST = 35, ADV_SCROLL_SOUTH = 36,
       ADV_SCROLL_SOUTHWEST = 37, ADV_SCROLL_WEST = 38,
       ADV_SCROLL_NORTHWEST = 39,
       GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT = 0 };
#include "inline/t_limit.inl"
#include "inline/limit.inl"
namespace GameTime { unsigned long get(); }
namespace widget {
enum { WIDGET_SET_STATUS = 5, WIDGET_DIMMED = 8, WIDGET_UPDATE = 16384 };
}

struct type_point {
    short m_x : 10;
    short m_y : 10;
    short m_z : 4;
    type_point() {}
#include "inline/type_point_ctor.inl"
    bool isValid() const;
};

enum hero_seqid { hs_stand_n = 0 };
#pragma options align=packed
class hero {
public:
    short m_x;
    short m_y;
    short m_z;
private:
    char m_beforeTarget[0x35 - 6];
public:
    int m_pathTargetX;
    int m_pathTargetY;
    short m_pathTargetZ;
private:
    char m_beforeFacing[0x47 - 0x3f];
public:
    unsigned char m_facing;
private:
    char m_beforeFlags[0x105 - 0x48];
public:
    unsigned int m_flags;
private:
    char m_beforeSleeping[0x11c - 0x109];
public:
    unsigned char m_isSleeping;
private:
    char m_afterSleeping[0x486 - 0x11d];
public:
    hero_seqid getStandSequence();
    void restoreCell();
#include "inline/hero_get_target.inl"
};
#pragma options align=reset

class playerData {
public:
    signed char m_color;
private:
    char m_beforeHeroId[3];
public:
    int m_currHeroId;
private:
    char m_beforeTownId[0x3f - 8];
public:
    signed char m_currTownId;
private:
    char m_afterTownId[0x15c - 0x40];
public:
    bool isHuman() const;
    bool isLocalHuman() const;
    int findHero(int heroId) const;
};

class game {
    char m_beforeHeroes[0x20a34];
public:
    hero m_heroes[156];
    playerData* getLocalPlayer();
    int getLocalPlayerGamePos() const;
    bool isLastHuman(int gamePos) const;
};

class NewmapCell {
    char m_beforeGround[4];
public:
    signed char m_groundSet;
};
class NewfullMap {
public:
    NewmapCell* cell(int x, int y, int z);
};
class TAdventureMapWindow {
public:
    enum { MOVE_ID = 7 };
    unsigned char setElevationToggleImage(int level);
    void updateHeroLocators(int top, unsigned char drawWin, unsigned char update);
    void updateTownLocators(int top, unsigned char drawWin, unsigned char update);
    void updateSpellButton(const hero* currentHero);
    void setSleepImage(int image);
    void updateSleepButton(const hero* currentHero);
    void updateResourceDisplay(bool draw, bool update);
    virtual void drawWindow(int windowId, int x, int y);
};
class heroWindowManager {
public:
    int broadcastMessage(int msgId, int codeX, int codeY, int extra);
    void updateScreen(int x, int y, int width, int height);
};
class soundManager { public: void switchAmbientMusic(int musicId); };
class inputManager { public: void forceMouseMove(); };
class mouseManager {
public:
    enum EPointerSet { ADVENTURE_SET = 1 };
    void setPointer(int newFrame, EPointerSet newSet);
};

class advManager {
    char m_beforeStatus[0x30];
public:
    int m_status;
private:
    char m_beforeAdvWindow[0x44 - 0x34];
public:
    TAdventureMapWindow* m_advWindow;
    char m_beforeShowRoute[0x4c - 0x48];
    int m_showRoute;
    int m_seedingValid;
    int m_fullySeeded;
    int m_lastTerrain;
    NewfullMap* m_fullMap;
private:
    char m_beforeRadarOrigin[0xe0 - 0x60];
public:
    type_point m_radarOrigin;
private:
    char m_beforeLastHoverX[4];
public:
    int m_lastHoverX;
private:
    char m_beforeDrawCursor[0x1e8 - 0xec];
public:
    unsigned char m_drawCursor;
private:
    char m_beforeCursorType[3];
public:
    int m_cursorType;
    int m_cursorDirection;
    int m_cursorBaseFrame;
    int m_cursorSequence;
    int m_cursorFrameCount;
    int m_cursorTurning;
    int m_cursorDrawn;
    unsigned char m_curHeroMobile;
private:
    char m_beforeHeroMoving[0x38c - 0x209];
public:
    unsigned char m_heroMoving;
    enum { CURSOR_TYPE_8 = 8, CURSOR_TYPE_34 = 0x22 };
    void setHeroContext(int heroId, int inMove, unsigned char waitingPlayer,
                        unsigned char drawChanges);
    void deactivateCurrTown(unsigned char waitingPlayer);
    void deactivateCurrHero(unsigned char waitingPlayer);
    void hideRoute(int updateScreen, int removeTarget, int changeButton);
    void redrawAdvScreen(unsigned char update, unsigned char saveBorder);
    void seedTo(type_point target);
    void showRoute(int updateScreen, int reseed, int changeButton);
    void updateRadar(type_point origin, unsigned char updateFlag,
                     unsigned char partialUpdate, unsigned char viewMines,
                     unsigned char viewHeroes, unsigned char viewTowns);
    void updateRadar(unsigned char updateFlag, unsigned char partialUpdate,
                     unsigned char viewMines, unsigned char viewHeroes,
                     unsigned char viewTowns);
    void completeDraw(int x, int y, int z, unsigned char forceDraw,
                      unsigned char updateBottomView);
    void completeDraw(unsigned char forceDraw);
    void updateScreen(int allowIntermediateMouse, int forceDraw);
    void screenScroll(int dir, int changeMouse);
    void demobilizeCurrHero(unsigned char updateScreen, unsigned char updateFlags);
    void setEnvironmentOrigin(type_point point, int reset);
    void updBottomView(unsigned char forceUpdate, unsigned char drawWindow,
                       unsigned char update);
};

extern game* g_game;
extern playerData* g_currentPlayer;
extern heroWindowManager* g_windowManager;
extern soundManager* g_soundManager;
extern inputManager* g_inputManager;
extern advManager* g_advManager;
extern int g_unnamed6989c8;
extern int g_unnamed69ccd4;
extern int g_videoPaused;
extern unsigned char g_followPlayerMode;
extern unsigned char g_completeDrawMessageBypass;
extern unsigned char g_unnamed691209;
// Dreamcast and Mac address this visibility flag inside g_config at +0x38.
// PHILAI already binds the source-owned retail alias to that Mac object.
struct MacConfig {
    char m_beforeScrollSpeed[0x34];
    int m_windowScrollSpeed;
    int m_visibilityScanSuppressed;
};
extern MacConfig g_config;
#define g_unnamed698758 g_config
#define g_unnamed698790 (g_config.m_visibilityScanSuppressed)
extern unsigned char g_mapVisibilityBit;
extern int g_completeDrawEnabled;
extern unsigned char g_terrainMusicIds[9];
extern mouseManager* g_mouseManager;
extern int g_mapWidth;
extern int g_mapHeight;
extern unsigned long g_unnamed691674;
#include "inline/advmgr_scroll_speed_inc.inl"
int mapExtraPosAndAdjacentsSet(int x, int y, int z, unsigned char bit);

#endif
