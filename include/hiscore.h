// hiscore.h - hiscore.cpp (compiland hiscore.obj)
#ifndef HOMM3_HISCORE_H
#define HOMM3_HISCORE_H

#include "basemgr.h"
#include "window.h"
#include "textntry.h"

#include "bitmap16.h"

class message;
class textWidget;
class iconWidget;
class Bitmap816;
class highScoreManager : public baseManager {
public:

// ResetHighScores 0x4e8fb0 zeroes exactly 22 0x64-byte records, then copies
// two 41-byte strings and parses the dwords at +0x54/+0x58. AddScoreToHighScore
// 0x4e91d0 stores difficulty at +0x5c and its one-byte cheat latch at +0x60;
// the three trailing bytes remain padding.
// Semantic names below are retail-derived: addScoreToHighScore 0x4e91d0
// copies the local player's edited name at +0, land at +0x29, and its
// score/days/difficulty arguments at +0x54/+0x58/+0x5c. No original record
// field spellings were recovered in the current DC/NH3API corpora.
struct HighScoreRec {
    char m_playerName[41];
    char m_land[41];
    char m_paddingAfterNames[2]; // +0x52: align the first int to four bytes
    int m_score;
    int m_days;
    int m_difficulty;
    unsigned char m_cheated;
    char m_paddingAfterCheated[3]; // +0x61: round the 0x64-byte record stride
};

    HighScoreRec m_highScores[2][11];
    int m_highScoreType;

    highScoreManager();
    ~highScoreManager();
    virtual int open(int newPriority);
    virtual void close();
    virtual int main(message& msg);

    void resetHighScores();
    void viewHiScore();
    int addScoreToHighScore(int score, int days, int difficulty,
                            int scoreType, const char* land);
    static int getMonType(int score, int scoreType);
};
SIZE(highScoreManager::HighScoreRec, 0x64);
SIZE(highScoreManager, 0x8d4);

// Retail oldmain opens this manager and invokes ViewHiScore through the
// pointer at 0x6993cc; hiscore.cpp owns the DATA definition.
DATA(0x006993cc) extern highScoreManager* g_highScoreManager;
DATA(0x0069955c) extern int g_showHighScore;

// DC names the three CHeroWindowEx-tail pointers at +0x4c/+0x50/+0x54.
// Retail's proven CHeroWindowEx is four bytes wider, putting them at
// +0x50/+0x54/+0x58; GetRolloverWidget 0x4e97f0 directly confirms the
// last shifted offset.

// DC names the two eleven-entry icon arrays Creatures and their matching
// frame slots CreatureFrames.  Retail retains both arrays at +0x4c/+0xa4,
// but drops DC's intervening m_h_index dword: bIsStandard consequently lands
// at +0xfc, followed by iCreatureFrame/lLastServe at +0x100/+0x104.
// Its GetBitmap816 calls store the two captured backgrounds at +0x108/+0x10c;
// the destructor independently reads those same final two slots.
// DC retains UpdateCreatures as a free TU helper, preceding the handler.
static void updateCreatures();
class CHighScoreEdit : public textEntryWidget {
public:
    CHighScoreEdit* m_nextEdit;
    CHighScoreEdit* m_prevEdit;

    // DC179 and its retained public prove writable string-pointer parameters
    // and font::EJustify, independently of textEntryWidget's current API.
    // The remaining DC int colorIndex / base TColor disagreement needs the
    // owning text-entry constructor review, not an invented overload here.
    CHighScoreEdit(int x, int y, int w, int h, int textSize,
                   char* text, char* fontName,
                   font::TColor color, font::EJustify justification,
                   char* backgroundIcon, int backgroundFrame, int id,
                   int style, int readType, int insetX, int insetY);
    // DC195 takes message&; the shared text-entry interface still uses message*.
    virtual int onKeyPress(message* msg);  // slot 15, retail 0x4e9710
};
SIZE(CHighScoreEdit, 0x78);

class CHSInputDlg : public CHeroWindowEx {
public:
    enum EWidgetIDs {
        FIELD1_ID = 501,
        OKAY_ID = 503,
        ROLLOVER_ID = 504
    };

    // First text-entry field; the wider PC base places it at +0x50.
    CHighScoreEdit* m_field1;
    textWidget* m_header1;
    textWidget* m_rollover;

    // DC293..326 places the constructor body in hiscore.cpp. The declaration
    // stays in this header, with an inline definition beside the other methods.
    // DC301 forwards maxChars1 to CHighScoreEdit; retail passes 40.
    CHSInputDlg(int maxChars);
    virtual ~CHSInputDlg();
    virtual int windowHandler(message& msg);
    virtual int onWidgetDeselect(int id, bool& exitFlag);
    virtual textWidget* getRolloverWidget();
    bool onOK();
};
SIZE(CHSInputDlg, 0x5c);

class THighScoreWindow : public heroWindow {
    // DC hiscore.cpp:1014-1031/1034-1184 directly reads/writes private
    // bIsStandard and lLastServe in these free functions; the class method
    // record contains no category/clock accessors. Retail 0x4ea1d0 agrees.
    friend void updateCreatures();
    friend int highScoreWindowHandler(message& msg);

public:
    // The two family selectors and the reset control the constructor gives
    // ids 1001/1002/1003, and the three cases HighScoreWindowHandler's
    // deselect switch dispatches on (0x3e9/0x3ea/0x3eb at 0x4ea241).
    enum EWidgetIDs {
        CAMPAIGN_ID = 1001,
        STANDARD_ID = 1002,
        RESET_ID = 1003
    };

    iconWidget* m_creatures[2][11];
    int m_creatureFrames[2][11];

private:
    unsigned char m_isStandard;

public:
    // The preceding byte field and following four-byte field establish
    // this alignment gap; the reference layout retains the same boundary.
    char m_paddingBeforeCreatureFrame[3];

    THighScoreWindow();
    virtual ~THighScoreWindow();
    void doModal();
    void update();

private:
    int m_creatureFrame;
    unsigned long m_lastServe;
    Bitmap816* m_hiScoreBack[2];
};
SIZE(THighScoreWindow, 0x110);

#endif  /* HOMM3_HISCORE_H */
