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
    MESSAGE_KEY_DOWN = 1,
    MESSAGE_KEY_UP = 2,
    MESSAGE_MOUSE_MOVE = 4,
    MESSAGE_LEFT_BUTTON_DOWN = 8,
    MESSAGE_LEFT_BUTTON_UP = 0x10,
    MESSAGE_RIGHT_BUTTON_DOWN = 0x20,
    MESSAGE_RIGHT_BUTTON_UP = 0x40,
    MESSAGE_WIDGET = 0x200,
    // The executive-arm flag bit (homm2 MESSAGE_EXECUTIVE):
    // executive::MainLoop only honours the command in codeX when a
    // forwarded message carries it.
    MESSAGE_EXECUTIVE = 0x4000
};

// baseManager::Main's dispatch verdicts as executive::MainLoop
// switches on them (homm2 BASE/message.h MessageDispatchResult
// names/values carried over verbatim; CONTINUE = 0 is the untaken
// default arm).
enum EMessageDispatchResult {
    MESSAGE_DISPATCH_CONSUME = 0x1,
    MESSAGE_DISPATCH_FORWARD = 0x2
};

// Executive commands carried in codeX of a MESSAGE_EXECUTIVE-flagged
// message (homm2 BASE/message.h ExecutiveCommand names/values
// verbatim; retail's MainLoop merges the TERMINATE_LOOP and
// RETURN_RESULT arms into one dialogReturn store).
enum EExecutiveCommand {
    EXECUTIVE_COMMAND_TERMINATE_LOOP = 0x1,
    EXECUTIVE_COMMAND_REMOVE_MANAGER = 0x2,
    EXECUTIVE_COMMAND_RETURN_RESULT = 0x4
};

// Modifier bits carried in message::qualifier (homm2 lineage names):
// button::Select latches qualifier & MASK into iLeftRightSave and the
// right-select stamp writes RIGHT.
enum EMessageModifiers {
    // Byte-proven by inputManager::ForceMouseMove, which builds the
    // qualifier from three GetKeyState probes: VK_SHIFT -> 1,
    // VK_CONTROL -> 4, VK_MENU -> 0x20. homm2 splits bit 0/1 into
    // right/left shift; retail sets only bit 0 for VK_SHIFT, so the
    // spelling here stays SHIFT until a producer proves the split.
    MESSAGE_MODIFIER_SHIFT = 1,
    // AsciiConvert gates the shifted-symbol map on `qualifier & 3`;
    // homm2 splits those two bits into right/left shift.
    MESSAGE_MODIFIER_SHIFT_KEYS = 3,
    MESSAGE_MODIFIER_CONTROL = 4,
    MESSAGE_MODIFIER_ALT = 0x20,
    MESSAGE_MODIFIER_RIGHT = 0x200,
    MESSAGE_MODIFIER_MASK = 0x300
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
    union {
        int extra;
        char* extraText;
    };
    heroWindow* window;
};
SIZE(message, 32);

#endif  /* HOMM3_MESSAGE_H */
