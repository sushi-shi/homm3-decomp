// advmgr_popup.h - narrow adventure-dialog base used by split-army UI
#ifndef HOMM3_ADVMGR_POPUP_H
#define HOMM3_ADVMGR_POPUP_H

#include "window.h"

// The adventure-dialog base. The Dreamcast field list gives the three
// protected ints at +0x4c/+0x50/+0x54; retail's 8-byte wider Dinkumware
// vector in heroWindow shifts them to +0x54/+0x58/+0x5c. Retail's ctor
// independently proves the preceding +0x50 dword and the byte at +0x5c;
// the latter overlaps the low byte of exitCommand and is restored by the
// destructor. Total retail size is therefore 0x60.
class CAdvPopup : public CHeroWindowEx {
public:
    int field_50;
protected:
    int exitId;
    int exitCodeX;
    union {
        int exitCommand;
        unsigned char savedPlayerState;
    };

public:
    CAdvPopup(int winX, int winY, int winWidth, int winHeight,
              unsigned winType);
    virtual ~CAdvPopup();
    virtual int WindowHandler(message* msg);             // slot 9
protected:
    virtual int ExitDialog(message* msg);                 // slot 14
};
SIZE(CAdvPopup, 0x60);

#endif /* HOMM3_ADVMGR_POPUP_H */
