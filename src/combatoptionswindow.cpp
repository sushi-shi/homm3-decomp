// combatoptionswindow.cpp - E:\gamedcs\combatoptionswindow.cpp (compiland combatoptionswindow.obj)
// HAND-OWNED after admission - retail-byte claims with Dreamcast CodeView prototypes.
#include <va.h>
#include "combatoptionswindow.h"
#include "misc.h"
#include "widget.h"
#include "winmgr.h"

// Source-private in the Dreamcast compiland. Retail's constructor stores the
// active dialog here and its destructor clears it before widget teardown.
DATA(0x00694f90) static TCombatOptionsWindow* gpCombatOptionsWindow;

// E:\gamedcs\combatoptionswindow.cpp:60
#if 0  // @carcass
VA(0x0046e3b0, 0x1320)  // combatManager caller + cmpopbck.pcx + vtable/global stores, dc 0x66c48
TCombatOptionsWindow::TCombatOptionsWindow()
{
    // @stub
}
#endif

// Retail emits the generated wrapper immediately after the constructor;
// Dreamcast appends it to the compiland.
VA_COMPGEN(0x0046f6d0, 0x21, SCALAR_DELETING_DTOR, TCombatOptionsWindow)

// E:\gamedcs\combatoptionswindow.cpp:180
VA(0x0046f700, 0x75)  // vtable/global/widget teardown, dc 0x679ac
TCombatOptionsWindow::~TCombatOptionsWindow()
{
    gpCombatOptionsWindow = 0;
    for (widget** it = Widgets.begin(); it != Widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\combatoptionswindow.cpp:194
// Retail inlines this switch into CombatOptionsWindowHandler.
#if 0  // @carcass
DC_ONLY(0x67a14, 0x2C)
int TCombatOptionsWindow::convertID2HelpID(int id) const
{
    // @stub
}
#endif

// E:\gamedcs\combatoptionswindow.cpp:214
VA(0x0046f780, 0x28)  // handler address-take + WritePrefs tail, dc 0x67a40
void TCombatOptionsWindow::DoModal()
{
    bPrefsChanged = 0;
    gpWindowManager->DoDialog(this, CombatOptionsWindowHandler, 0);
    if (bPrefsChanged)
        WritePrefs();
}

// The four highlight helpers are present out of line on Dreamcast, but every
// retail use is inlined into the constructor/handler; no retail entries occur
// between DoModal and the handler.
#if 0  // @carcass
DC_ONLY(0x67a78, 0x52)
void TCombatOptionsWindow::HighlightCombatSpeed() { /* @stub */ }
DC_ONLY(0x67acc, 0x22)
void TCombatOptionsWindow::HighlightGrid() { /* @stub */ }
DC_ONLY(0x67af0, 0x22)
void TCombatOptionsWindow::HighlightMovementShadow() { /* @stub */ }
DC_ONLY(0x67b14, 0x68)
void TCombatOptionsWindow::HighlightMouseShadow() { /* @stub */ }
#endif

// E:\gamedcs\combatoptionswindow.cpp:278
#if 0  // @carcass
VA(0x0046f7b0, 0x72A)  // DoModal address-take + complete message CFG, dc 0x67b7c
int CombatOptionsWindowHandler(message& msg)
{
    // @stub
}
#endif

// Dreamcast's UpdateCombatOptions follows the handler, but retail inlines it
// at both call sites. The next retail entry (0x46fee0) is an unrelated cinit.
#if 0  // @carcass
DC_ONLY(0x68098, 0x38)
void UpdateCombatOptions(unsigned char bFirstUpdate)
{
    // @stub
}
#endif

// E:\gamedcs\combatoptionswindow.cpp:171
#if 0  // @carcass -- represented by VA_COMPGEN above
DC_ONLY(0x680d0, 0x34)
void* TCombatOptionsWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}
#endif
