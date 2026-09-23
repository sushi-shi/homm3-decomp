// Mac declaration view for the canonical THighScoreWindow constructor in
// src/hiscore.cpp. Offsets below are observed in code 0:0x10b084..0x10b708.
#ifndef HOMM3_MAC_HISCORE_H
#define HOMM3_MAC_HISCORE_H

#include "va.h"

typedef unsigned long size_t;
extern "C" void* memset(void*, int, size_t);
void memError();
namespace GameTime { unsigned long get(); }

class widget {
    virtual ~widget();
    char m_beforeDerived[0x2c - 4];
public:
    void hide();
    void show();
};

namespace std {
template <class T> class vector {
    char m_beforeSize[4];
    unsigned long m_size;
    T* m_first;
public:
    void reserve(unsigned long count);
    void push_back(const T& value);
    T* begin();
    T* end();
};
class string {
    struct rep {
        char m_beforeData[0xc];
        char* m_data;
    }* m_handle;
public:
    const char* c_str() const { return m_handle->m_data; }
};
}

class Bitmap816;
class heroWindow {
    virtual ~heroWindow();
    char m_beforeWidgets[0x30 - 4];
public:
    std::vector<widget*> m_widgets;  // Mac +0x30, size at +0x34, first at +0x38.
private:
    char m_afterWidgets[0x48 - 0x3c];
public:
    heroWindow(int, int, int, int, unsigned);
    void addWidget(widget*, int);
};

class button : public widget {
    char m_buttonTail[0x58 - 0x2c];  // new size 0x58 at 0:0x10b0fc.
public:
    button(int, int, int, int, int, const char*, int, int, bool, int, int);
    void setHotkey(int);
};

class iconWidget : public widget {
    char m_iconTail[0x48 - 0x2c];  // new size 0x48 at 0:0x10b450.
public:
    enum { ICON_STYLE_PLAIN = 0x10 };
    iconWidget(int, int, int, int, int, const char*, int, int, bool,
               unsigned, int);
};

class highScoreManager {
    char m_beforeScores[0x38];
public:
    struct HighScoreRec {
        char m_beforeScore[0x54];
        int m_score;
        char m_afterScore[0x64 - 0x58];
    } m_highScores[2][11];
    int m_highScoreType;  // Mac +0x8d0.
    static int getMonType(int score, int scoreType);
};
extern highScoreManager* g_highScoreManager;

class CObjectType {
public:
    std::string m_imageName;
};
class NewfullMap {
public:
    CObjectType* newfullMapFn00505EA0(int objectType, int extra);
};
class game {
    char m_beforeWorldMap[0x1f3c0];
public:
    NewfullMap m_worldMap;  // 0:0x10b060 uses game +0x1f3c0.
};
extern game* g_game;
enum { MONSTER = 54 };
enum { DIALOG_RETURN_OK = 0x7802 };  // Mac button arg at 0:0x10b13c.

namespace ResourceManager { Bitmap816* getBitmap816(const char*); }
class THighScoreWindow : public heroWindow {
public:
    iconWidget* m_creatures[2][11];  // Mac +0x48, 0x2c-byte bank stride.
    int m_creatureFrames[2][11];  // Mac +0xa0, bzero at 0:0x10b440.
private:
    unsigned char m_isStandard;  // Mac +0xf8.
    char m_align[3];
    int m_creatureFrame;  // Mac +0xfc.
    unsigned long m_lastServe;  // Mac +0x100.
    Bitmap816* m_hiScoreBack[2];  // Mac +0x104/+0x108.
public:
    THighScoreWindow();
};
extern THighScoreWindow* g_highScoreWindow;

#endif
