#ifndef HOMM3_KBWIN_H
#define HOMM3_KBWIN_H

#include <windows.h>

#include "struct.h"

void appExit();                         // 0x4f7fa0
void process1WindowsMessage();          // 0x4f7fb0
void kbChangeMenu(HMENU newMenu);       // 0x4f8180, fastcall under /Gr
void setNoDialogMenus(int noMenus);     // 0x4f81e0, fastcall under /Gr
void setMenus(HMENU menu, int enabled);
BOOL CALLBACK appAbout(HWND dialog, UINT message, WPARAM messageParam,
    LPARAM messageData);                // 0x4f8140
LRESULT appCommand(HWND window, UINT message, WPARAM messageParam,
    LPARAM messageData);                // 0x4f8060, fastcall under /Gr
LRESULT CALLBACK appWndProc(HWND window, UINT message,
    WPARAM messageParam, LPARAM messageData);  // 0x4f7c00

// The three surviving menu commands (homm2 kbwin.h values; homm3
// dropped the four window-size entries but kept the IDs, byte-proven
// by AppCommand's case values).
enum KbWinMenuCommand {
    KBWIN_MENU_FULLSCREEN = 0x9c49,
    KBWIN_MENU_HELP = 0x9c74,
    KBWIN_MENU_ABOUT = 0x9c75
};

// homm2 SMenuEnableStatus (KB_TYPES.h) minus its pack(1) reserved
// byte: retail's SetMenus scales the table index by 8 and reads the
// enable bytes at +4/+5, so the homm3 struct is naturally aligned.
struct SMenuEnableStatus {
    unsigned int m_command;
    unsigned char m_normalEnabled;
    unsigned char m_setupEnabled;
};

// Retail table 0x67f930..0x67f958 = 5 entries (homm2's
// MENU_ENABLE_STATUS_COUNT lineage; the four window-size rows are
// gone, a zero sentinel row leads).
enum { KBWIN_MENU_ENTRY_COUNT = 5 };

// kbwin globals (DC attests hwndApp; the rest are retail-only .bss
// slots named for their role - provisional).
extern HWND g_hwndApp;                    // 0x699600
extern HINSTANCE g_instance;            // 0x6995b4
extern unsigned char g_inMessageLoop;    // 0x6995b8
extern HMENU g_currMenu;                  // 0x6995bc
extern HMENU g_activeMenu;                // 0x699604
extern int g_menusSuppressed;            // 0x699618
extern int g_windowedMode;               // 0x6987b8
extern int g_videoPaused;                // 0x69954c
extern HMENU g_dfltMenu;                  // 0x6989e4 (CallManager's resume
                                        // arm restores it; name provisional)
// .bss 0x698a34, the single-player menu gate advmgr's Open tests: when
// clear, the non-multiplayer LoadMenu pair (0x6f/0x71) is skipped
// entirely. Role wider than that is unattested; ordinal name.
extern int g_unnamed698a34;
extern HMENU g_gameMenu;                  // 0x6989e8 (dfltMenu's .bss
                                        // neighbour: kb's InitMainClasses
                                        // loads the pair, kb's CleanUpMenus
                                        // destroys the pair, and
                                        // advManager::Open /
                                        // combatManager::Open install it.
                                        // Name provisional)
extern int g_inSetupDialog;             // 0x6989d0 (homm2 name; selects the
                                        // setupEnabled column in SetMenus)
extern char g_appName[];                // 0x67f820 "Heroes III" (homm2 name)
extern char g_title[];                  // 0x67f82c (homm2 name)
extern HANDLE g_gameEvent;              // 0x69960c (single-instance event)
extern char g_commandLine[61];          // 0x6995c0 (homm2 gcCommandLine)
extern int g_windowX;                    // 0x6987b0 (windowed x, saved on move)
extern int g_windowY;                    // 0x6987b4
extern LONG g_appWindowStyle;            // 0x6995a8 (WM_MOVE style snapshot)
extern RECT g_rcAppWindow;                // 0x699598
extern int g_closingApp;                 // 0x6989fc (homm2 gbClosingApp)
extern unsigned char g_shutDownDone;     // 0x699608 (WM_QUIT ShutDown guard;
                                        // name provisional)
extern unsigned char g_appDeactivated;   // 0x699609 (system cursor shown while
                                        // switched away; name provisional)
extern unsigned char g_musicWasPlaying;  // 0x699614 (deactivate latch; name
                                        // provisional)
extern SMenuEnableStatus g_menuEnableStatus[KBWIN_MENU_ENTRY_COUNT];
    // 0x67f930 (.data; homm2 kept the
                                        // table in KB.cpp - homm3 owner TU
                                        // unproven, defined in kbwin.cpp as
                                        // its only known consumer)

// --- globals ---
// The video bring-up hook. Retail's body is EMPTY - it ICF-folded onto the
// image's shared one-byte `ret` - but the CALL survives in
// heroWindowManager::Open, which is what makes the declarator needed here
// rather than only in the CODEVIEW roster below.
void initVideo();                                        // 0x5bc690 (ICF)

#endif  /* HOMM3_KBWIN_H */
