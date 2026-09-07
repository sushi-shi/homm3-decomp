// advmgr_popup.h - narrow adventure-dialog base used by split-army UI
#ifndef HOMM3_ADVMGR_POPUP_H
#define HOMM3_ADVMGR_POPUP_H

#include "window.h"

// Dreamcast places exitId/exitCodeX/exitCommand at +0x4c/+0x50/+0x54.
// Retail's CHeroWindowEx ends at +0x50, so the fields move by four bytes,
// to +0x50/+0x54/+0x58. DC ExitDialog (0x1ec80) and retail 0x41b190
// independently send the first two through message.id/codeX and the last
// through windowManager.dialogReturn. Complete adds its saved popup-state
// byte at +0x5c, followed by alignment to the 0x60-byte object size.
class CAdvPopup : public CHeroWindowEx {
protected:
    // Previously field_50; original Dreamcast name: exitId.
    int m_exitId;
    // Previously misidentified as exitId; original: exitCodeX.
    int m_exitCodeX;
    // Previously misidentified as exitCodeX; original: exitCommand.
    int m_exitCommand;
    // Complete-only: ctor 0x41b040 saves the handler's popup state and
    // dtor 0x41b120 restores it. This byte does not overlap exitCommand.
    unsigned char m_savedPlayerState;
    // The saved byte ends at +0x5d. The class's four-byte alignment
    // supplies the trailing three bytes; there is no additional member.

public:
    CAdvPopup(int winX, int winY, int winWidth, int winHeight,
              unsigned winType);
    virtual ~CAdvPopup();
    // Before normalization (function): CAdvPopup::WindowHandler.
    virtual int windowHandler(message* msg);             // slot 9
protected:
    // Before normalization (function): CAdvPopup::ExitDialog.
    virtual int exitDialog(message* msg);                 // slot 14
};
SIZE(CAdvPopup, 0x60);

#endif /* HOMM3_ADVMGR_POPUP_H */
