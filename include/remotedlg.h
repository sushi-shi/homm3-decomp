// remotedlg.h - the window/dialog classes retail's remote.cpp defines
// (compiland remote.obj).
// HAND-OWNED. Class layouts are NOT fabricated from method symbols;
// prototypes stay comments until a retail layout is proven.
//
// This is deliberately NOT in remote.h. The Dreamcast field list homes
// these classes in E:\gamedcs\remote.h, but seven other compiled TUs include
// our remote.h (advmgr, ai_player, cmbtmgr, command, levelupwindow, mainmenu,
// systemoptionswindow) and none of them needs a dialog or bitmap base; pulling
// dialogbox.h and bitmap16.h into their include closures is exactly the
// perturbation the include-set residual class warns about. remote.cpp is the
// only consumer.
#ifndef HOMM3_REMOTEDLG_H
#define HOMM3_REMOTEDLG_H

#include "bitmap16.h"
#include "dialogbox.h"

class CSprite;

// Layout is DC's field list shifted by the CTextDialog widening (DC's
// CTextDialog is 0x50, retail's is 0x58, so every DC offset moves +8), and
// every shifted offset is corroborated by retail bytes:
//   m_lastTick     DC 80  -> 0x58   zeroed by the constructor 0x554a50
//   m_spriteX      DC 84  -> 0x5c
//   m_spriteY      DC 88  -> 0x60
//   m_spriteFrame  DC 92  -> 0x64   zeroed by the constructor
//   m_seq          DC 96  -> 0x68   written by Setup 0x554b10 (4th argument)
//   m_sSprite      DC 100 -> 0x6c   written by Setup 0x554b10 (3rd argument)
//   m_palUpdated   DC 104 -> 0x70   zeroed (byte) by the constructor
//   m_pSprite      DC 108 -> 0x74   zeroed by the constructor; the destructor
//                                   0x554ab0 Disposes it through slot 1
// DC's total 112 becomes 0x78.
//
// The vtable 0x640e94 is 14 slots against CTextDialog's 13, and DC names the
// extra one exactly: Setup is INTRODUCING VIRTUAL at "vfptr offset = 52",
// i.e. slot 13 - which is where retail's 0x554b10 sits, and that body writes
// m_sSprite/m_seq and then chains to CTextDialog::Setup (0x490820, slot 10).
// The three remaining entries are overrides at their inherited slots: slot 3
// handle_message (0x554d90), slot 5 DrawWindow (0x554e90), slot 12
// CalcDimensions (0x554c30). Declared only, no local definition - the
// widget::Close idiom.
class CAnimatedDlg : public CTextDialog {
public:
    CAnimatedDlg();
    virtual ~CAnimatedDlg();
    virtual unsigned char Setup(const char* cText, font* pFont,
                                const char* sSprite, int seq);  // slot 13
    virtual void CalcDimensions(const char* cText, font* pFont,
                                int& winX, int& winY,
                                int& winWidth, int& winHeight);  // slot 12
    virtual int handle_message(message& msg);                    // slot 3
    virtual void DrawWindow(unsigned char update, int iLowID,
                            int iHighID);                        // slot 5

protected:
    unsigned long m_lastTick;    // +0x58
    int m_spriteX;               // +0x5c
    int m_spriteY;               // +0x60
    int m_spriteFrame;           // +0x64
    int m_seq;                   // +0x68
    const char* m_sSprite;       // +0x6c
    unsigned char m_palUpdated;  // +0x70
    CSprite* m_pSprite;          // +0x74
};
SIZE(CAnimatedDlg, 0x78);

// --- CAnimatedDlg ---
// CODEVIEW(E:\gamedcs\remote.cpp:1539, dc 0x11d1ec) void CAnimatedDlg::CAnimatedDlg();
// CODEVIEW(E:\gamedcs\remote.cpp:1547, dc 0x11d250) void CAnimatedDlg::~CAnimatedDlg();
// CODEVIEW(E:\gamedcs\remote.cpp:1553, dc 0x11d290) bool CAnimatedDlg::Setup(const char* cText, font* pFont, const char* sSprite, int seq);
// CODEVIEW(E:\gamedcs\remote.cpp:1563, dc 0x11d2b0) void CAnimatedDlg::CalcSpriteDimensions(CSprite* pSprite, int& a, int& b, int& c);
// CODEVIEW(E:\gamedcs\remote.cpp:1606, dc 0x11d394) void CAnimatedDlg::CalcDimensions(const char* cText, font* pFont, int& winX, int& winY, int& winWidth, int& winHeight);
// CODEVIEW(E:\gamedcs\remote.cpp:1634, dc 0x11d490) void CAnimatedDlg::DrawSprite();
// CODEVIEW(E:\gamedcs\remote.cpp:1651, dc 0x11d558) int CAnimatedDlg::handle_message(message& msg);
// CODEVIEW(E:\gamedcs\remote.cpp:1658, dc 0x11d56c) void CAnimatedDlg::TickAnimation();
// CODEVIEW(E:\gamedcs\remote.cpp:1673, dc 0x11d5dc) void CAnimatedDlg::DrawWindow(bool update, int iLowID, int iHighID);

// CSaveScreen - the off-screen backing store the transfer dialog parks the
// framebuffer in. DC's field list sits on a 184-byte Bitmap16Bit; retail's
// Bitmap16Bit is 0x38, and the three trailing members land on exactly the
// three offsets the constructor 0x5572e0 writes after its base call:
//   screenSaved  DC 184 -> 0x38 (byte)
//   m_x          DC 188 -> 0x3c
//   m_y          DC 192 -> 0x40
// The vtable 0x640fb0 is three slots wide - Bitmap16Bit's own - so this class
// introduces no virtual of its own, and DC marks ~CSaveScreen (compgenx):
// there is no user-written destructor to declare, which is why 0x557340 is a
// bare five-byte tail jump into ??1Bitmap16Bit.
class CSaveScreen : public Bitmap16Bit {
public:
    CSaveScreen(int w, int h);
    void Save(int x, int y);
    void Restore(unsigned char update);
    unsigned char IsSaved();

protected:
    unsigned char screenSaved;  // +0x38
    int m_x;                    // +0x3c
    int m_y;                    // +0x40
};
SIZE(CSaveScreen, 0x44);

// CGameTransferSmack - the Smacker-backed progress animation the transfer
// dialog owns by value. DC's record is 20 bytes with no vftable, and every
// member lands on a store in the constructor 0x557410:
//   m_x           0   m_started     12 (byte)   m_saveScreen  16
//   m_y           4   m_sending     13 (byte)
//   m_lastFrame   8   m_drawText    14 (byte)
// The destructor closes the Smacker handle through the 0x198xxx video
// free function at 0x599050 and then deletes m_saveScreen through
// CSaveScreen's slot 0. Its guard-and-clear half is CGameTransferSmack::Stop
// byte for byte (0x5575e0 is those nine instructions and nothing else),
// which is what proves the destructor's first statement is a Stop() call
// rather than an open-coded repeat.
class CGameTransferSmack {
public:
    CGameTransferSmack();
    ~CGameTransferSmack();
    void Setup(int x, int y, unsigned char sending, unsigned char drawText);
    void Start();
    void Stop();
    void SaveScreen();
    void RestoreScreen();

protected:
    int m_x;                     // +0x00
    int m_y;                     // +0x04
    int m_lastFrame;             // +0x08
    unsigned char m_started;     // +0x0c
    unsigned char m_sending;     // +0x0d
    unsigned char m_drawText;    // +0x0e
    CSaveScreen* m_saveScreen;   // +0x10
};
SIZE(CGameTransferSmack, 0x14);

// CGameTransferDlg - the save-game transfer progress dialog. DC's field list
// is a CTextDialog carrying `smack` at 80 and `m_sending` at 100 in a
// 104-byte record; retail's CTextDialog is 0x58, so smack occupies
// 0x58..0x6b - exactly the twenty bytes the constructor 0x557720 fills with
// CGameTransferSmack's inlined constructor - and m_sending lands on 0x6c,
// which is where that constructor stores its one byte argument.
//
// The vtable 0x640fbc is thirteen slots, CTextDialog's own width, so this
// class introduces no virtual; slot 12 holds its CalcDimensions override at
// 0x557790 and every other slot is inherited. DC marks ~CGameTransferDlg
// (compgenx), and retail's copy of it sits at 0x4cbcf0 - outside remote's
// band, i.e. a COMDAT the linker took from another object - so there is
// nothing here to declare or claim for it.
class CGameTransferDlg : public CTextDialog {
public:
    CGameTransferDlg(unsigned char sending);
    virtual void CalcDimensions(const char* cText, font* pFont,
                                int& winX, int& winY,
                                int& winWidth, int& winHeight);  // slot 12

protected:
    CGameTransferSmack smack;    // +0x58
    unsigned char m_sending;     // +0x6c
};
SIZE(CGameTransferDlg, 0x70);

// --- CSaveScreen ---
// CODEVIEW(E:\gamedcs\remote.cpp:2673, dc 0x11eb40) void CSaveScreen::CSaveScreen(int w, int h);
// CODEVIEW(E:\gamedcs\remote.cpp:2680, dc 0x11eb9c) void CSaveScreen::Save(int x, int y);
// CODEVIEW(E:\gamedcs\remote.cpp:2689, dc 0x11ebc8) void CSaveScreen::Restore();
// CODEVIEW(E:\gamedcs\remote.cpp:2701, dc 0x11ec5c) bool CSaveScreen::IsSaved();

// --- CGameTransferSmack ---
// CODEVIEW(E:\gamedcs\remote.cpp:2713, dc 0x11ec64) void CGameTransferSmack::CGameTransferSmack();
// CODEVIEW(E:\gamedcs\remote.cpp:2724, dc 0x11ec88) void CGameTransferSmack::~CGameTransferSmack();
// CODEVIEW(E:\gamedcs\remote.cpp:2733, dc 0x11ecbc) void CGameTransferSmack::Setup(int x, int y, bool sending, int a);
// CODEVIEW(E:\gamedcs\remote.cpp:2741, dc 0x11ecd8) void CGameTransferSmack::Start();
// CODEVIEW(E:\gamedcs\remote.cpp:2747, dc 0x11ece4) void CGameTransferSmack::SetPercentage(float pct);
// CODEVIEW(E:\gamedcs\remote.cpp:2784, dc 0x11ede8) void CGameTransferSmack::DrawCurrentFrame();
// CODEVIEW(E:\gamedcs\remote.cpp:2789, dc 0x11edec) void CGameTransferSmack::Stop();
// CODEVIEW(E:\gamedcs\remote.cpp:2799, dc 0x11ee04) void CGameTransferSmack::SaveScreen();
// CODEVIEW(E:\gamedcs\remote.cpp:2807, dc 0x11ee3c) void CGameTransferSmack::RestoreScreen();

// --- CGameTransferDlg ---
// CODEVIEW(E:\gamedcs\remote.cpp:2816, dc 0x11ee54) void CGameTransferDlg::CGameTransferDlg(bool sending);
// CODEVIEW(E:\gamedcs\remote.cpp:2821, dc 0x11eed8) void CGameTransferDlg::CalcDimensions(const char* cText, font* pFont, int& winX, int& winY, int& winWidth, int& winHeight);

#endif  /* HOMM3_REMOTEDLG_H */
