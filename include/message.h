// message.h - the basewin message record (header-only in the original;
// homm2's BASE/message.h is the template).
// HAND-OWNED after admission.
#ifndef HOMM3_MESSAGE_H
#define HOMM3_MESSAGE_H

#include <va.h>

class heroWindow;

// Message-id domain. Only the value the widget bodies prove is listed:
// homm2's BASE/message.h MESSAGE_WIDGET = 0x200 carried over verbatim
// (widget::send_message and widget::enable store it into message::id).
// Enum NAME is homm2 lineage, unattested on DC - grow the roster as
// consumers prove values.
enum EMessageId {
    MESSAGE_WIDGET = 0x200
};

// Dreamcast roster: id, codeX, codeY, qualifier, mouseX, mouseY,
// extra, window, oldX@32, oldY@36 (40 B). The retail frames in
// widget::send_message/enable are 0x20 B - retail dropped oldX/oldY.
// DC shows two ctors (heroWindow*, int, uchar / heroWindow*) and no
// default; the retail widget bodies build locals member-by-member with
// no ctor call, so none is modeled yet.
class message {
public:
    int id;
    int codeX;
    int codeY;
    int qualifier;
    int mouseX;
    int mouseY;
    int extra;
    heroWindow* window;
};
SIZE(message, 32);

#endif  /* HOMM3_MESSAGE_H */
