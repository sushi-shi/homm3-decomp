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
    // The Dreamcast CodeView body at struct.h:42 zeroes the fields in
    // declaration order. The attested consumer sites need the real
    // constructor shape and VC6 removes fields overwritten before first read.
    // Dreamcast type 0x1016 lists this eight-argument overload before the
    // default constructor; both are header-inline source boundaries.
    message(int id_, int codeX_, int codeY_, int qualifier_,
            int mouseX_, int mouseY_, int extra_, heroWindow* window_)
    {
        id = id_;
        codeX = codeX_;
        codeY = codeY_;
        qualifier = qualifier_;
        mouseX = mouseX_;
        mouseY = mouseY_;
        extra = extra_;
        window = window_;
    }

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
    short x : 10;
    short y : 10;
    short z : 4;

    type_point() {}
    // E:\\gamedcs\\struct.h:102. Dreamcast CodeView places the body in the
    // shared header, and both Dreamcast and Complete expand it at ordinary
    // call sites. Keep one canonical source definition here so every TU sees
    // the real helper at the original parse point.
    VA(0x004192b0, 0x44)  // anchor-callee, dc 0x1edb0
    type_point(short new_x, short new_y, short new_z)
    {
        x = new_x;
        y = new_y;
        z = new_z;
    }
    // Dreamcast S_PUB32 is ??8type_point@@QBA_NABU0@@Z: bool return,
    // const member, const-reference operand.
    VA(0x0042ec20, 0x45)  // exact body + sole caller, dc 0x1ee20
    bool operator==(const type_point& arg) const
    {
        return x == arg.x && y == arg.y && z == arg.z;
    }
    // Dreamcast retains this source helper out of line in
    // AI_AttemptMove; Complete VC6 expands the same three comparisons.
    VA(0x00482340, 0x45)  // call edge + byte-identical point comparison, dc 0x37d2c
    bool operator!=(const type_point& arg) const
    {
        return x != arg.x || y != arg.y || z != arg.z;
    }
    unsigned char is_valid();
    // E:\gamedcs\struct.h:120, and advspells.obj's own Dreamcast roster
    // retains it out of line (dc 0x22fe4, 0x2e B). Retail has no body:
    // advManager::TownGate (0x41d360) is the admitted witness and EXPANDS
    // it twice in one statement pair - once for the `< best` test and
    // once for the store into `best`, each time as
    // `(this->x - p2->x)^2 + (this->y - p2->y)^2` with the two 10-bit
    // bitfields sign-extended through the shl/movsx/sar triple. The z
    // plane takes no part, which is what makes it a MAP distance.
    int DistanceSquared(const type_point* p2) const
    {
        int dy = y - p2->y;
        int dx = x - p2->x;
        return dx * dx + dy * dy;
    }
};

// The shared inclusive rectangle used by the adventure and combat drawing
// code. Dreamcast CodeView fixes the four names, offsets, and 16-byte extent;
// retail's combat drawing wrappers read the same four dwords in this order.
// Its type-handle collateral is banked in score history rather than hidden
// behind consumer-specific declarations.
struct SLimitData {
    int iMinX;
    int iMinY;
    int iMaxX;
    int iMaxY;

    SLimitData() {}
    SLimitData(int minx, int miny, int maxx, int maxy)
        : iMinX(minx), iMinY(miny), iMaxX(maxx), iMaxY(maxy) {}
    int Width() const { return iMaxX - iMinX + 1; }
    int Height() const { return iMaxY - iMinY + 1; }
    bool Intersects(const SLimitData& limits) const
    {
        return iMinX <= limits.iMaxX
            && iMaxX >= limits.iMinX
            && iMinY <= limits.iMaxY
            && iMaxY >= limits.iMinY;
    }
    bool IsEmpty() const
    {
        return iMaxX < iMinX || iMaxY < iMinY;
    }
    void Clip(const SLimitData& limits)
    {
        if (iMinX < limits.iMinX)
            iMinX = limits.iMinX;
        if (iMinY < limits.iMinY)
            iMinY = limits.iMinY;
        if (iMaxX > limits.iMaxX)
            iMaxX = limits.iMaxX;
        if (iMaxY > limits.iMaxY)
            iMaxY = limits.iMaxY;
    }
    void Include(const SLimitData& limits)
    {
        if (iMinX > limits.iMinX)
            iMinX = limits.iMinX;
        if (iMinY > limits.iMinY)
            iMinY = limits.iMinY;
        if (iMaxX < limits.iMaxX)
            iMaxX = limits.iMaxX;
        if (iMaxY < limits.iMaxY)
            iMaxY = limits.iMaxY;
    }
};
SIZE(SLimitData, 0x10);

// CodeView struct.h:340/346 owns this network player record. Complete
// extends DC's 28-byte dpid/name pair with the version dword at +0x1c;
// retail's seat-record constructor proves the same base initialization.
extern int* gpVideoGameState;

class CNetPlayerInfo {
public:
    unsigned long dpid;   // +0x00
    char sName[24];       // +0x04
    int version;          // +0x1c (retail-only extension)

    // E:\gamedcs\struct.h:340. The Dreamcast body initializes the two
    // shared fields; Complete's added version member belongs to the same
    // base boundary in the retail selection-window TU.
    VA(0x0057F720, 0x18)  // DC's ordered dpid/name stores plus Complete's game-version field, dc 0x11f5e4
    CNetPlayerInfo()
    {
        dpid = 0;
        sName[0] = 0;
        version = *gpVideoGameState;
    }
    // E:\gamedcs\struct.h:346
    CNetPlayerInfo(char* _sName, unsigned long _dpid)
    {
        dpid = _dpid;
        strcpy(sName, _sName);
        version = *gpVideoGameState;
    }
};
SIZE(CNetPlayerInfo, 32);

// GameTime has header-inline helpers in struct.h (DC lines 411-438);
// Get, DelayTil and Delay remain ordinary definitions in kbwin.cpp.
class GameTime {
public:
    static unsigned long Get();             // 0x4f82e0
    static void DelayTil(unsigned long time);  // 0x4f82f0
    static void Delay(int interval);        // 0x4f83c0
    // DC struct.h:411 / :419 (dc 0x1eed4, 0x1ef04) - the other two
    // header inlines of the same family; no retail out-of-line body
    // exists for either. textEntryWidget::SetupDisplayString 0x5bb660
    // is the expansion that proves the shape: the deadline argument is
    // loaded into a callee-saved register BEFORE the Get() call (an
    // argument evaluated ahead of its guard), and the result is tested
    // with `sub eax, edi; js`, i.e. the SIGN of the difference - not
    // the unsigned `cmp` a hand-spelled `Get() >= deadline` emits.
    static long ElapsedSince(unsigned long time)
    {
        return static_cast<long>(Get() - time);
    }
    static unsigned char IsPast(unsigned long time)
    {
        return ElapsedSince(time) >= 0;
    }
    // DC struct.h:438 (dc 0x4c994, 44 B on SH4) - the frame-pacing
    // step, and a HEADER INLINE: no retail out-of-line body exists,
    // /Ob2 expands it at every site. army::Fly (0x4b4a40) is the
    // expansion that proves the shape - `this_frame` is homed to a
    // stack slot BEFORE the Get() call and read back twice afterwards,
    // which a hand-spelled `timer += lag` (two independent global
    // loads) cannot produce, and the clamp compares `cmp interval, lag;
    // jle`, i.e. the INTERVAL is the left operand.
    static unsigned long NextFrameTime(unsigned long this_frame,
                                       long interval)
    {
        long lag = static_cast<long>(Get() - this_frame);
        if (interval > lag)
            lag = interval;
        return this_frame + lag;
    }
};

#endif /* HOMM3_STRUCT_H */
