// Canonical shared message declaration and inline constructor.
#ifndef HOMM3_MESSAGE_RECORD_H
#define HOMM3_MESSAGE_RECORD_H

#include "va.h"

class heroWindow;

// Dreamcast roster: id, codeX, codeY, qualifier, mouseX, mouseY,
// extra, window, oldX@32, oldY@36 (40 B). The retail frames in
// widget::send_message/enable are 0x20 B - retail dropped oldX/oldY.
// The Dreamcast xref graph also proves the default constructor at dc 0x2d58.
// This is one class shape, not a per-TU optimizer view: the constructor is
// canonical and VC6 may remove fields overwritten before their first read.
class message {
public:
    int m_id;
    int m_codeX;
    int m_codeY;
    int m_qualifier;
    int m_mouseX;
    int m_mouseY;
    union {
        int m_extra;
        const char* m_extraText;
    };
    heroWindow* m_window;
    // DC type 0x1020 proves this overload's declaration, but no body or
    // inline source row has been recovered. Keep the declaration alone;
    // overview's zero-initialization uses the proven default constructor.
    message(int id, int codeX, int codeY, int qualifier,
            int mouseX, int mouseY, int extra, heroWindow* window);
    // Retail RS_CLICK constructs this 32-byte local at 0x588e3d before
    // setting codeY and passing it to OnWidgetDeselect. The retained body
    // zeroes offsets +0 through +0x1c and returns the receiver in EAX.
    // E:\gamedcs\struct.h:42, dc 0x2d58
    VA(0x00589190, 0x1c)  // RS_CLICK constructor + field stores, dc 0x2d58
    message()
    {
        m_id = 0;
        m_codeX = 0;
        m_codeY = 0;
        m_qualifier = 0;
        m_mouseX = 0;
        m_mouseY = 0;
        m_extra = 0;
        m_window = 0;
    }
};
SIZE(message, 32);

#endif
