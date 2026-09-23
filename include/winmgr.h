#ifndef HOMM3_WINMGR_H
#define HOMM3_WINMGR_H

#include "basemgr.h"
#include "message.h"
#include "struct.h"

class Bitmap16Bit;
class Bitmap816;
class heroWindow;

// The dialog pump's per-message handler: retail calls it through
// `lea ecx,[msg]; call [ebp+0xc]` - one argument, no stack, i.e. the
// /Gr default fastcall applied to a function pointer. Returns an
// EMessageDispatchResult.
typedef int (*TDialogHandler)(message& msg);

// heroWindowManager::dialogReturn's domain: only the byte-proven
// value is listed (AppWndProc's WM_CLOSE confirm test at 0x4f7cb9);
// NH3API window_manager.hpp DialogReturnType spelling - the roster
// grows as consumers prove values.
enum EDialogReturnType {
    DIALOG_RETURN_TIMEOUT = 9999,
    DIALOG_RETURN_CANCEL = 0x7801,
    DIALOG_RETURN_OK = 0x7802,
    DIALOG_RETURN_SPLIT_ACCEPT = 0x7802,
    DIALOG_RETURN_ACCEPT = 0x7805,
    // ACCEPT's partner in the iMBType-2 yes/no pair, byte-proven by the
    // two wandering-stack refusals: advManager::monsters_join (0x4a7000)
    // and monsters_sell_out (0x4a7250) both ask with iMBType 2 and take
    // the DECLINE arm on 0x7806, where monsters_flee (0x4a6df0) tests the
    // same dialog's reply against 0x7805 to take the ACCEPT arm. Gated to
    // the events view: no other modeled consumer proves the value, and an
    // ungated enumerator counts toward the include-set threshold.
    // (advmgr's turn view joined 2026-08-20: StartLocalPlayerTurn takes
    // the DECLINE arm of the gosolo hand-back dialog on the same value.)
    DIALOG_RETURN_DECLINE = 0x7806,
    // The two picture-choice replies of the iMBType-10 dialog, byte-proven
    // by advManager::DoEventWarSchool (0x4a7a40): it offers the two
    // primary-skill pictures 0x1f and 0x20 as iResType1/iResType2 and then
    // switches the reply over 0x7801 / 0x7809 / 0x780a, taking the FIRST
    // picture on 0x7809 and the second on 0x780a. Ordinal spellings - the
    // pairing is proven, the words are not attested.
    DIALOG_RETURN_CHOICE_1 = 0x7809,
    DIALOG_RETURN_CHOICE_2 = 0x780a
};

// Bootstrap VIEW of heroWindowManager (Dreamcast size 104): only the
// screen bitmap is consumed so far. DC puts dialogReturn@56,
// lastHover@60, lastHoverAK@64, screenBitmap@68; retail
// widget::Dim reads the screen at [gpWindowManager+0x40], so retail
// dropped one of the two hover ints - the pad below spans 0x38..0x3f
// until those members earn byte-proven names.
class heroWindowManager : public baseManager {
public:
    // +0x38: NormalDialog's result slot (AppWndProc's WM_CLOSE tests
    // it against 0x7805, the confirm-OK command).
    int m_dialogReturn;
    // +0x3c: DC's lastHover (members.csv heroWindowManager@60), the
    // first of DC's lastHover/lastHoverAK pair - retail dropped the
    // second, which is why screenBitmap lands at 0x40 here and 68
    // there. Byte-proven: DoDialog stores -1 into it right before
    // AddWindow, exactly where the buka twin writes m_lastHoverId.
    int m_lastHover;
    Bitmap16Bit* m_screenBitmap;
    int m_colorCyclingOn;
    unsigned char m_isWaitingForFadeIn;
    // NH3API confirms three alignment bytes between the PC fade-in
    // flag at +0x48 and bmpFizzleSource at +0x4c. Close proves the pointer role.
    char m_paddingBeforeFizzleSource[3];
    // +0x4c: the manager's SECOND owned bitmap. Byte-proven by Close
    // (0x6022d0), which deletes it through the same virtual slot-0 +
    // flag-1 tail it uses on screenBitmap, and it is the only member
    // besides screenBitmap the destructor path touches - the fizzle
    // source the Save/Release pair works on. Dreamcast supplies the
    // bmpFizzleSource member name.
    Bitmap16Bit* m_bmpFizzleSource;
    heroWindowManager();
    virtual int open(int newPriority);
    virtual void close();
    virtual int main(message& msg);
    void addWindow(heroWindow* newWindow, int newPriority,
                   unsigned char update);
    void removeWindow(heroWindow* killWindow);
    int broadcastMessage(int msgId, int msgCodeX, int msgCodeY, int msgExtra);
    int doDialog(heroWindow* dialogWindow, TDialogHandler dialogFunction,
                 int fadeIn);
    int doDialogDraw(heroWindow* dialogWindow, TDialogHandler dialogFunction,
                     TDialogHandler dialogDrawFunction, int fadeIn);
    void doQuickView(heroWindow* window);
    void updateScreen();
    void updateScreen(int x, int y, int w, int h);
    void screenShot();
    void saveFizzleSource(int startX, int startY, int width, int height);
    void fizzleForward(int startX, int startY, int width, int height, int fadeTime);
    void nextFlashFrame(int startX, int startY, int width, int height, int fadeTime);
    void flash(int startX, int startY, int width, int height, int fadeTime);
    void fadeBlit(int sx, int sy, int sw, int sh, const Bitmap816* srcBitmap,
                  int dx, int dy, unsigned char transparent, int frames, int period);
    void fadeScreen(int inOut, int speed, unsigned char expectFadein);
    void saveFizzleSourceX(int startX, int startY, int width, int height);
    void fizzleForwardX(int startX, int startY, int width, int height,
                        int fadeTime);
    // Original: heroWindowManager::SaveFizzleSource; WinMgr.h:181, dc 0x230bc.
    void saveFizzleSource(const SLimitData& limits)
    {
        saveFizzleSource(limits.m_minX, limits.m_minY,
                         limits.width(), limits.height());
    }
    // Original: heroWindowManager::FizzleForward; WinMgr.h:187, dc 0x23104.
    void fizzleForward(const SLimitData& limits, int fadeTime)
    {
        fizzleForward(limits.m_minX, limits.m_minY,
                      limits.width(), limits.height(), fadeTime);
    }
    // WinMgr.h:193..200 (dc 0x70af0/0x70b40) proves the const-reference
    // rectangle overloads and their Width/Height calls. Complete uses the X
    // pixel path at the adventure-spell sites as well as in combat drawing.
    void fizzleForwardX(const SLimitData& limits, int fadeTime)
    {
        fizzleForwardX(limits.m_minX, limits.m_minY,
                       limits.width(), limits.height(), fadeTime);
    }
    void saveFizzleSourceX(const SLimitData& limits)
    {
        saveFizzleSourceX(limits.m_minX, limits.m_minY,
                          limits.width(), limits.height());
    }
    // 0x6030c0, the fizzle buffer's release. Order-mapped between
    // FizzleForwardX and FadeToBlack and byte-shaped: it deletes
    // field_4C through the virtual slot-0 tail and nulls it.
    void releaseFizzleSource();
    int convertToHover(message& msg);
    void fadeToBlack(int speed, unsigned char expectFadein);
    void fadeFromBlack(int speed);

private:
    void blitToScreenWithPointer(int x, int y, int w, int h);

    // The window list, byte-proven by RemoveWindow: headWindow@0x50,
    // tailWindow@0x54, lastActive@0x58, activeWindow@0x5c.
    heroWindow* m_headWindow;
    heroWindow* m_tailWindow;

public:
    heroWindow* m_lastActive;
    heroWindow* m_activeWindow;
};

// Retail .bss 0x699280 (DC ?gpWindowManager@@3PAVheroWindowManager@@A);
// the DATA claim lands with winmgr.cpp.
DATA(0x00699280) extern heroWindowManager* g_windowManager;

// Three cross-TU dialog globals DoDialog drives. None of them is
// winmgr-owned - they are declared here (the gUnnamed69d808 precedent)
// until their own TU lands and takes the DATA claims.
//   0x6989cc  DC public ?gbInDialog@@3HA (int) - set 1 on entry, 0 on
//             the level-1 cleanup, both normal and unwind paths.
//   0x698a1c  DC public ?gbSendMouseMoveMessages@@3HA (int) - gates
//             whether MESSAGE_MOUSE_MOVE reaches the dialog window.
//   0x6aad20  the dialog nest counter: 0->1 arms SetNoDialogMenus(0),
//             1->0 fires SetNoDialogMenus(1). No DC public; the name is
//             homm2(buka) BASE lineage (iDialogNestCount), PROVISIONAL.
extern int g_inDialog;
extern int g_sendMouseMoveMessages;
extern int g_dialogNestCount;

// Shared absolute deadline consumed by modal-dialog handlers. The DATA claim
// currently lives with levelupwindow.cpp, the first admitted owner/consumer.
extern unsigned long g_dialogDeadline697784;

#endif  /* HOMM3_WINMGR_H */
