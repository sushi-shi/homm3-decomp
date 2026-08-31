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
    // The empty-queue message. inputManager::GetEvent (0x4ec590) and its
    // PeekEvent twin build it by hand - `id = 0; codeY = 0; codeX = 0;
    // qualifier = 0` - whenever the ring is drained, and
    // advManager::Main cases on it to run the idle animation frame.
    MESSAGE_NONE = 0,
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
// verbatim; retail's MainLoop keeps TERMINATE_LOOP as a done-only arm
// and stores dialogReturn only for RETURN_RESULT).
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
    // CONTROL's pair-mask, the same shape SHIFT_KEYS has three lines up:
    // advManager::ProcessMapSelect gates its "keep the existing path"
    // arm on `qualifier & 0xc`, and 0xc is CONTROL plus the bit the homm2
    // lineage splits off as the second control key. Only the MASK is
    // byte-proven here; no located producer sets bit 3, which is why bit 3
    // gets no enumerator of its own.
    MESSAGE_MODIFIER_CONTROL_KEYS = 0xc,
    MESSAGE_MODIFIER_ALT = 0x20,
    MESSAGE_MODIFIER_RIGHT = 0x200,
    MESSAGE_MODIFIER_MASK = 0x300
};

// Dreamcast roster: id, codeX, codeY, qualifier, mouseX, mouseY,
// extra, window, oldX@32, oldY@36 (40 B). The retail frames in
// widget::send_message/enable are 0x20 B - retail dropped oldX/oldY.
// The Dreamcast xref graph also proves the default constructor at dc 0x2d58.
// Keep its concrete inline view consumer-scoped because many reconstructed
// retail sites still model already-optimized member stores directly.
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
        const char* extraText;
    };
    heroWindow* window;
#if defined(HOMM3_HERO_MESSAGE_CTOR_VIEW) || \
    defined(HOMM3_ARMYGRP_MESSAGE_CTOR_VIEW) || \
    defined(HOMM3_COMMAND_GRID_VIEW) || \
    defined(HOMM3_QUESTLOG_MESSAGE_CTOR_VIEW) || \
    defined(HOMM3_RECRUIT_MESSAGE_CTOR_VIEW) || \
    defined(HOMM3_SWAPMGR_MESSAGE_CTOR_VIEW) || \
    defined(HOMM3_TRADPOST_MESSAGE_CTOR_VIEW)
    // The Dreamcast CodeView body at struct.h:42 zeroes the fields in
    // declaration order.  Keep this TU-scoped because other reconstructed
    // units still use aggregate initializers; the attested consumer sites
    // need the real constructor shape and VC6 removes fields overwritten
    // before their first read.
    message()
    {
        id = 0;
        codeX = 0;
        codeY = 0;
        qualifier = 0;
        mouseX = 0;
        mouseY = 0;
        extra = 0;
        window = 0;
    }
#endif
};
SIZE(message, 32);

#ifdef HOMM3_TOWNMGR_MESSAGE_CTOR_CARRIER
// Compiler-history carrier for townManager::SetupMage. Dreamcast proves the
// original source called message::message(), whose body zeroes this complete
// base plus its two DC-only tail fields. Giving message itself a default ctor
// would change every reconstructed POD-style site in the include closure, so
// this layout-identical subtype isolates the one retail-proven call site.
class mage_message : public message {
public:
    mage_message()
    {
        id = 0;
        codeX = 0;
        codeY = 0;
        qualifier = 0;
        mouseX = 0;
        mouseY = 0;
        extra = 0;
        window = 0;
    }
};
#endif

#endif  /* HOMM3_MESSAGE_H */
