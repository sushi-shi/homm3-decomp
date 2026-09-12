// struct.h - the engine's small shared value types (E:\gamedcs\struct.h)
// HAND-OWNED. Class layouts are NOT fabricated from method symbols;
// prototypes stay comments until a retail layout is proven.
#ifndef HOMM3_STRUCT_H
#define HOMM3_STRUCT_H

#include <va.h>
#include <string.h>

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

    // Original: message::message; struct.h:42, dc 0x2d58.
    // Retail RS_CLICK constructs this 32-byte local at 0x588e3d before
    // setting codeY and passing it to OnWidgetDeselect. The retained body
    // zeroes offsets +0 through +0x1c and returns the receiver in EAX.
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

// A packed map coordinate. The DC layout (classes.csv: 4 B, three
// members) puts x at offset 0 and BOTH y and z at offset 2 - the
// signature of short-based bitfields, where x:10 fills the first
// 16-bit unit's low bits and y:10 forces a new unit at +2 with z:4
// tucked into that unit's remaining six.
//
// Every retail reader agrees on the widths. searchArray::get_danger_value
// (0x42ed30) and game::get_cell (0x42ed80) both read the field trio as
//   x: dword @ +0, shl 6, movsx ax, sar 6      -> signed 10 bits @ 0..9
//   y: dword @ +2, shl 6, movsx ax, sar 6      -> signed 10 bits @ 0..9
//   z: dword @ +2, shl 2, movsx ax, sar 12     -> signed  4 bits @ 10..13
// and can_take_town (0x428410) builds one the other way round, masking
// the three source bytes with 0x3ff, 0x3ff and 0xf before packing.
struct type_point {
    // Before normalization: x.
    short m_x : 10;
    // Before normalization: y.
    short m_y : 10;
    // Before normalization: z.
    short m_z : 4;

    type_point() {}
    // E:\\gamedcs\\struct.h:102. Dreamcast CodeView places the body in the
    // shared header, and both Dreamcast and Complete expand it at ordinary
    // call sites. Keep one canonical source definition here so every TU sees
    // the real helper at the original parse point.
    VA(0x004192b0, 0x44)  // anchor-callee, dc 0x1edb0
    type_point(short newX, short newY, short newZ)
    {
        m_x = newX;
        m_y = newY;
        m_z = newZ;
    }
    // Dreamcast S_PUB32 is ??8type_point@@QBA_NABU0@@Z: bool return,
    // const member, const-reference operand.
    VA(0x0042ec20, 0x45)  // exact body + sole caller, dc 0x1ee20
    bool operator==(const type_point& arg) const
    {
        return m_x == arg.m_x && m_y == arg.m_y && m_z == arg.m_z;
    }
    // Dreamcast retains this source helper out of line in
    // AI_AttemptMove; Complete VC6 expands the same three comparisons.
    VA(0x00482340, 0x45)  // call edge + byte-identical point comparison, dc 0x37d2c
    bool operator!=(const type_point& arg) const
    {
        return m_x != arg.m_x || m_y != arg.m_y || m_z != arg.m_z;
    }
    // Before normalization (function): type_point::is_valid.
    // DC S_PUB32 ?is_valid@type_point@@QBA_NXZ proves a const bool member.
    // Its ordinary body remains in findpath.cpp:36; do not move it here
    // to force expansions in adventure drawing.
    bool isValid() const;
    // E:\gamedcs\struct.h:120, and advspells.obj's own Dreamcast roster
    // retains it out of line (dc 0x22fe4, 0x2e B). Retail has no body:
    // advManager::TownGate (0x41d360) is the admitted witness and EXPANDS
    // it twice in one statement pair - once for the `< best` test and
    // once for the store into `best`, each time as
    // `(this->x - p2->x)^2 + (this->y - p2->y)^2` with the two 10-bit
    // bitfields sign-extended through the shl/movsx/sar triple. The z
    // plane takes no part, which is what makes it a MAP distance.
    // DC struct.h:120 proves const type_point& p2 (dc 0x22fe4).
    // Before normalization (function): type_point::DistanceSquared.
    int distanceSquared(const type_point& p2) const
    {
        int dy = m_y - p2.m_y;
        int dx = m_x - p2.m_x;
        return dx * dx + dy * dy;
    }
};

// The shared inclusive rectangle used by the adventure and combat drawing
// code. Dreamcast CodeView fixes the four names, offsets, and 16-byte extent;
// retail's combat drawing wrappers read the same four dwords in this order.
// Its type-handle collateral is banked in score history rather than hidden
// behind consumer-specific declarations.
struct SLimitData {
    // Before normalization: iMinX.
    int m_minX;
    // Before normalization: iMinY.
    int m_minY;
    // Before normalization: iMaxX.
    int m_maxX;
    // Before normalization: iMaxY.
    int m_maxY;

    SLimitData() {}
    SLimitData(int minx, int miny, int maxx, int maxy)
        : m_minX(minx), m_minY(miny), m_maxX(maxx), m_maxY(maxy) {}
    // Before normalization (function): SLimitData::Width.
    int width() const { return m_maxX - m_minX + 1; }
    // Before normalization (function): SLimitData::Height.
    int height() const { return m_maxY - m_minY + 1; }
    // Before normalization (function): SLimitData::Intersects.
    bool intersects(const SLimitData& limits) const
    {
        return m_minX <= limits.m_maxX
            && m_maxX >= limits.m_minX
            && m_minY <= limits.m_maxY
            && m_maxY >= limits.m_minY;
    }
    // Before normalization (function): SLimitData::IsEmpty.
    bool isEmpty() const
    {
        return m_maxX < m_minX || m_maxY < m_minY;
    }
    // Before normalization (function): SLimitData::Clip.
    void clip(const SLimitData& limits)
    {
        if (m_minX < limits.m_minX)
            m_minX = limits.m_minX;
        if (m_minY < limits.m_minY)
            m_minY = limits.m_minY;
        if (m_maxX > limits.m_maxX)
            m_maxX = limits.m_maxX;
        if (m_maxY > limits.m_maxY)
            m_maxY = limits.m_maxY;
    }
    // Before normalization (function): SLimitData::Include.
    void include(const SLimitData& limits)
    {
        if (m_minX > limits.m_minX)
            m_minX = limits.m_minX;
        if (m_minY > limits.m_minY)
            m_minY = limits.m_minY;
        if (m_maxX < limits.m_maxX)
            m_maxX = limits.m_maxX;
        if (m_maxY < limits.m_maxY)
            m_maxY = limits.m_maxY;
    }
};
SIZE(SLimitData, 0x10);

// CodeView struct.h:340/346 owns this network player record. Complete
// extends DC's 28-byte dpid/name pair with the version dword at +0x1c;
// retail's seat-record constructor proves the same base initialization.
extern int* g_videoGameState;

class CNetPlayerInfo {
public:
    unsigned long m_dpid;   // +0x00
    char m_name[24];       // +0x04
    int m_version;          // +0x1c (retail-only extension)

    // E:\gamedcs\struct.h:340. The Dreamcast body initializes the two
    // shared fields; Complete's added version member belongs to the same
    // base boundary in the retail selection-window TU.
    VA(0x0057F720, 0x18)  // DC's ordered dpid/name stores plus Complete's game-version field, dc 0x11f5e4
    CNetPlayerInfo()
    {
        m_dpid = 0;
        m_name[0] = 0;
        m_version = *g_videoGameState;
    }
    // E:\gamedcs\struct.h:346
    CNetPlayerInfo(char* name, unsigned long dpid)
    {
        m_dpid = dpid;
        strcpy(m_name, name);
        m_version = *g_videoGameState;
    }
};
SIZE(CNetPlayerInfo, 32);

// GameTime has header-inline helpers in struct.h (DC lines 411-438);
// Get, DelayTil and Delay remain ordinary definitions in kbwin.cpp.
class GameTime {
public:
    static unsigned long get();             // 0x4f82e0
    static void delayTil(unsigned long time);  // 0x4f82f0
    static void delay(int interval);        // 0x4f83c0
    // DC struct.h:411 / :419 (dc 0x1eed4, 0x1ef04) - the other two
    // header inlines of the same family; no retail out-of-line body
    // exists for either. textEntryWidget::SetupDisplayString 0x5bb660
    // is the expansion that proves the shape: the deadline argument is
    // loaded into a callee-saved register BEFORE the Get() call (an
    // argument evaluated ahead of its guard), and the result is tested
    // with `sub eax, edi; js`, i.e. the SIGN of the difference - not
    // the unsigned `cmp` a hand-spelled `Get() >= deadline` emits.
    // Before normalization (function): GameTime::Elapsed.
    // The stop/start subtraction is retained by the upstream mouse timing helper.
    static long elapsed(unsigned long stop, unsigned long start)
    {
        return static_cast<long>(stop - start);
    }
    static long elapsedSince(unsigned long time)
    {
        return static_cast<long>(get() - time);
    }
    static unsigned char isPast(unsigned long time)
    {
        return elapsedSince(time) >= 0;
    }
    // DC struct.h:438 (dc 0x4c994, 44 B on SH4) - the frame-pacing
    // step, and a HEADER INLINE: no retail out-of-line body exists,
    // /Ob2 expands it at every site. army::Fly (0x4b4a40) is the
    // expansion that proves the shape - `this_frame` is homed to a
    // stack slot BEFORE the Get() call and read back twice afterwards,
    // which a hand-spelled `timer += lag` (two independent global
    // loads) cannot produce, and the clamp compares `cmp interval, lag;
    // jle`, i.e. the INTERVAL is the left operand.
    static unsigned long nextFrameTime(unsigned long thisFrame,
                                       long interval)
    {
        long lag = static_cast<long>(get() - thisFrame);
        if (interval > lag)
            lag = interval;
        return thisFrame + lag;
    }
};

#endif /* HOMM3_STRUCT_H */
