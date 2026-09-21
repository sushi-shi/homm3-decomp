#ifndef HOMM3_SINGLESELECTIONPOPUPS_H
#define HOMM3_SINGLESELECTIONPOPUPS_H

#include "dialogbox.h"
#include "kbwin.h"
#include "message.h"
#include "remote.h"
#include "rmg.h"
#include "widget.h"
#include "winmgr.h"

class CSprite;

class CSprite;
class Bitmap816;
enum TTownType;

const char* getStartingResourceName(int town);
const char* getStartingResourceDescription(int town);

// Retail's constructor allocates 0x38 bytes and writes the sprite and frame
// immediately after widget's proven 0x30-byte base. Its vtable at 0x641a00
// independently fixes the four overrides below.
class CSpriteWidget : public widget {
public:
    CSprite* m_sprite;
    int m_frame;

    CSpriteWidget(int xPos, int yPos, CSprite* sprite, int spriteFrame);
    virtual ~CSpriteWidget();
    virtual int main(message& msg);  // slot 2, retail 0x575a10
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const; // slot 3
    virtual void draw() const;             // slot 4, retail 0x575750
};
SIZE(CSpriteWidget, 0x38);

// A widget wrapping a Bitmap816. image sits at +0x30 (first past widget),
// byte-proven by Draw reading it at [this+0x30]. Vtable 0x641a34; Main
// ICF-folds onto CSpriteWidget::Main (0x575a10, `return widget::Main`) and
// zBufferDraw onto the shared empty.
class CBitmapWidget : public widget {
public:
    Bitmap816* m_image;

    CBitmapWidget(int xPos, int yPos, Bitmap816* image);
    virtual int main(message& msg);  // slot 2, folds onto 0x575a10
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const; // slot 3
    virtual void draw() const;             // slot 4, retail 0x575a20
};

class CNetMsgHandler;

// The shared base of the four single-selection dialogs. Never directly
// instantiated (no own retail vtable): its ctor, Add and ExitDialog are all
// inlined into the derived dialogs, and only handle_message (slot 3 of every
// derived vtable, 0x575430) survives out of line. Adds one byte, gameMode,
// at +0x54 - the first byte past the 0x54-byte TDialogBox base.
class CSingleSelPopup : public TDialogBox {
public:
    unsigned char m_gameMode;

    CSingleSelPopup(int type, unsigned char newGameMode)
        : TDialogBox(type)
    {
        m_gameMode = newGameMode;
    }
    // Inlined into every CreateWin: push the widget onto the window's vector
    // and register it with priority -1 (dc 0x12eef4).
    void add(widget* w)
    {
        m_widgets.push_back(w);
        addWidget(w, -1);
    }

    VA(0x00575430, 0x8f)  // dc 0x12ef28
    virtual int handleMessage(message& msg)
    {
        if (msg.m_id != MESSAGE_RIGHT_BUTTON_UP) {
            if (g_networkActive69954c && g_dPlay) {
                CNetMsgHandler* handler = g_dPlay->getNetMsgHandler();
                if (handler) {
                    handler->checkHandleNet(1, 0);
                    if (handler->getAbortPopupMsg()) {
                        return exitDialog(msg);
                    }
                }
            }
            return heroWindow::handleMessage(msg);
        }
        return exitDialog(msg);
    }
    // Original: CSingleSelPopup::ExitDialog; singleselectionpopups.h:75, dc 0x12efc0.
    // handleMessage0x575430 expands both exits with this message rewrite.
    int exitDialog(message& msg)
    {
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = msg.m_codeY;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return 2;
    }

};

// A bare rectangular click target. Retail's 0x575220 ctor calls
// ??0widget@@QAE@XZ (the default base ctor) and writes x/y/width/height/id
// straight into the widget base (DC's trailing `focus` byte is not a retail
// parameter: `ret 0x14` is five dwords). Vtable 0x6419a4; zBufferDraw/Draw
// are folded shared empties (0x404140 / 0x404df0); their canonical
// source bodies remain here. The deleting dtor tail-jumps to ~widget.
class CHotspotWidget : public widget {
public:
    CHotspotWidget(int xPos, int yPos, int w, int h, int widgetId);
    virtual ~CHotspotWidget();
    virtual int main(message& msg);  // slot 2, retail 0x575290
    // Original: CHotspotWidget::zBufferDraw; singleselectionpopups.h:120, dc 0x12f010.
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const {}
    // Original: CHotspotWidget::Draw; singleselectionpopups.h:121, dc 0x12f014.
    virtual void draw() const {}
};

// The four dialogs. Each ctor pushes 0x12 through TDialogBox, stores its own
// vtable and the gameMode byte (the inlined CSingleSelPopup ctor). None adds
// storage except CTeamAlignmentDlg (its team table below).
// None of the three declares a destructor: scenarioinfo.obj's
// ProcessRightSelect destroys all four dialogs with a direct
// `call ??1TDialogBox` (0x48fe90), which is what the 5-byte IMPLICIT_DTOR
// representative at 0x576530 inlines to.  An explicit empty body inserts a
// vptr store and emits an out-of-line ??1C*Dlg the caller would have to call
// instead - the same rule the CTeamAlignmentDlg claim already records.
class CBonusDlg : public CSingleSelPopup {
public:
    CBonusDlg(unsigned char newGameMode);
    unsigned char createWin(const char* title, CSprite* sprite, int frame, const char* botTitle, const char* description);
    unsigned char createWin(const char* title, Bitmap816* image, const char* botTitle, const char* description);
};

class CHeroDlg : public CSingleSelPopup {
public:
    CHeroDlg(unsigned char newGameMode);
    unsigned char createWin(Bitmap816* heroPick, const char* heroName, CSprite* specialtyIcon, int frame, const char* specialtyName, const char* desc);
};

class CTownDlg : public CSingleSelPopup {
public:
    CTownDlg(unsigned char newGameMode);        // retail 0x575e10
    unsigned char createWin(CSprite* town, int frame, TTownType townType);
};

// The team-alignment picker adds a team-mask table at +0x58 and a count at
// +0x78, byte-proven by GetTeams (rep stosd over [+0x58..+0x78], count at
// [+0x78]).
class CTeamAlignmentDlg : public CSingleSelPopup {
public:
    int m_teamMasks[8];
    int m_numTeams;

    CTeamAlignmentDlg(unsigned char newGameMode);  // retail 0x5764d0
    unsigned char createWin();

protected:
    void getTeams();
    int countNumPlayers(int teamNbr);
};

// The modal progress bar retail raises around the generator run.  Vtable
// 0x641b14 names three of its four bodies outright (0x577090 scalar deleting
// destructor, 0x577300 SetTotal, 0x577320 Advance); the constructor 0x576f00
// and the repaint 0x577180 reach the rest.  Ordinal name.
class TRandomMapProgress : public TProgressSink {
public:
    std::vector<widget*> m_widgets;   // +0x0c
    heroWindow* m_window;             // +0x1c
    CSprite* m_barSprite;             // +0x20
    // The last permille-ish position Draw painted, cached so a repaint at an
    // unchanged position costs nothing.  Retail compares the fresh
    // `done * 256 / total` against it and returns when they agree.
    int m_drawnPosition;              // +0x24
    // retail's live monsterStrength/min temporary: constructor receiver
    // ebp-0x5c at 0x5862ef, independent word ebp-0x30 at 0x586306/0x586437.
    // Retain only the unresolved word at +0x28. The known fields fill
    // 0x28 bytes; 0x2c is an extent bound, not a proven retail sizeof.
    char m_pad28[4];

    TRandomMapProgress(int totalSteps);
    virtual ~TRandomMapProgress();
    virtual void setTotal(int totalSteps);
    virtual void advance(int amount);
    // Ordinal name, retained from the earlier singleselectionwindow.h model
    // because that TU already calls it by this spelling.
    void loadProgFn00577180();  // retail 0x577180
};
// Check this provisional view; exact retail extent remains unresolved.
SIZE(TRandomMapProgress, 0x2c);

#endif  /* HOMM3_SINGLESELECTIONPOPUPS_H */
