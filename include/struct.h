// struct.h - the engine's small shared value types (E:\gamedcs\struct.h)
#ifndef HOMM3_STRUCT_H
#define HOMM3_STRUCT_H

#include "va.h"

#include <string.h>

#include "message_record.h"

// A packed map coordinate. The DC layout (classes.csv: 4 B, three
// members) puts x at offset 0 and BOTH y and z at offset 2 - the
// signature of short-based bitfields, where x:10 fills the first
// 16-bit unit's low bits and y:10 forces a new unit at +2 with z:4
// tucked into that unit's remaining six.

// Every retail reader agrees on the widths. searchArray::get_danger_value
// (0x42ed30) and game::get_cell (0x42ed80) both read the field trio as
//   x: dword @ +0, shl 6, movsx ax, sar 6      -> signed 10 bits @ 0..9
//   y: dword @ +2, shl 6, movsx ax, sar 6      -> signed 10 bits @ 0..9
//   z: dword @ +2, shl 2, movsx ax, sar 12     -> signed  4 bits @ 10..13
// and can_take_town (0x428410) builds one the other way round, masking
// the three source bytes with 0x3ff, 0x3ff and 0xf before packing.
struct type_point {
public:
    short m_x : 10;
    short m_y : 10;
    short m_z : 4;
    type_point() {}
    VA(0x004192b0, 0x44)  // anchor-callee, dc 0x1edb0
    type_point(short newX, short newY, short newZ)
    {
        m_x = newX;
        m_y = newY;
        m_z = newZ;
    }
    // DC S_PUB32 ?is_valid@type_point@@QBA_NXZ proves a const bool member.
    bool isValid() const;
    // Dreamcast S_PUB32 is ??8type_point@@QBA_NABU0@@Z: bool return,
    // const member, const-reference operand.
    VA(0x0042ec20, 0x45)  // exact body + sole caller, dc 0x1ee20
    bool operator==(const type_point& arg) const
    {
        return m_x == arg.m_x && m_y == arg.m_y && m_z == arg.m_z;
    }
    VA(0x00482340, 0x45)  // call edge + byte-identical point comparison, dc 0x37d2c
    bool operator!=(const type_point& arg) const
    {
        return m_x != arg.m_x || m_y != arg.m_y || m_z != arg.m_z;
    }
    // E:\gamedcs\struct.h:120, and advspells.obj's own Dreamcast roster
    // retains it out of line (dc 0x22fe4, 0x2e B). Retail has no body:
    // advManager::TownGate (0x41d360) is the admitted witness and EXPANDS
    // it twice in one statement pair - once for the `< best` test and
    // once for the store into `best`, each time as
    // `(this->x - p2->x)^2 + (this->y - p2->y)^2` with the two 10-bit
    // bitfields sign-extended through the shl/movsx/sar triple. The z
    // plane takes no part, which is what makes it a MAP distance.
    // DC struct.h:120 proves const type_point& p2 (dc 0x22fe4).
    int distanceSquared(const type_point& p2) const
    {
        int dx = m_x - p2.m_x;
        int dy = m_y - p2.m_y;
        return dx * dx + dy * dy;
    }
};

// The shared inclusive rectangle used by the adventure and combat drawing
// code. Dreamcast CodeView fixes the four names, offsets, and 16-byte extent;
// retail's combat drawing wrappers read the same four dwords in this order.
// Its type-handle collateral is banked in score history rather than hidden
// behind consumer-specific declarations.
struct SLimitData {
public:
    int m_minX;
    int m_minY;
    int m_maxX;
    int m_maxY;
    SLimitData() {}
    SLimitData(int minx, int miny, int maxx, int maxy)
        : m_minX(minx), m_minY(miny), m_maxX(maxx), m_maxY(maxy) {}
    int width() const { return m_maxX - m_minX + 1; }
    int height() const { return m_maxY - m_minY + 1; }
    bool intersects(const SLimitData& limits) const
    {
        return m_minX <= limits.m_maxX
            && m_maxX >= limits.m_minX
            && m_minY <= limits.m_maxY
            && m_maxY >= limits.m_minY;
    }
    // Original: SLimitData::Contains; struct.h:293, dc 0x639ec.
    unsigned char contains(int x, int y) const
    {
        return m_minX <= x && x <= m_maxX
            && m_minY <= y && y <= m_maxY;
    }
    bool isEmpty() const
    {
        return m_maxX < m_minX || m_maxY < m_minY;
    }
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
    unsigned long m_dpid;  // +0x00
    char m_name[24];  // +0x04
    int m_version;
    VA(0x0057F720, 0x18)  // dc 0x11f5e4
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
// The public names use @@YA (namespace functions), and their NB11 records
// are LF_PROCEDURE; static class members in the same corpus use @@SA and
// member-function records. GameTime has no object type or instance users.
namespace GameTime {
    unsigned long get();             // 0x4f82e0
    void delayTil(unsigned long time);  // 0x4f82f0
    void delay(int interval);        // 0x4f83c0
    // DC struct.h:411 / :419 (dc 0x1eed4, 0x1ef04) - the other two
    // header inlines of the same family; no retail out-of-line body
    // exists for either. textEntryWidget::SetupDisplayString 0x5bb660
    // is the expansion that proves the shape: the deadline argument is
    // loaded into a callee-saved register BEFORE the Get() call (an
    // argument evaluated ahead of its guard), and the result is tested
    // with `sub eax, edi; js`, i.e. the SIGN of the difference - not
    // the unsigned `cmp` a hand-spelled `Get() >= deadline` emits.
    // The stop/start subtraction is retained by the upstream mouse timing helper.
    inline long elapsed(unsigned long stop, unsigned long start)
    {
        return static_cast<long>(stop - start);
    }
    // DC struct.h:412 explicitly calls Get then Elapsed(stop, start).
    // Retain the canonical call even where retail expands the subtraction.
    inline long elapsedSince(unsigned long time)
    {
        return elapsed(get(), time);
    }
    // DC public ?IsPast@GameTime@@YA_NK@Z proves native bool although
    // NB11 lowers its return record to T_UCHAR. DC struct.h:420 calls
    // ElapsedSince then tests the sign, as retail caller expansions do.
    inline bool isPast(unsigned long time)
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
    // DC struct.h:439 calls ElapsedSince before the clamp/add at line442.
    // The old direct subtraction erased that proven helper boundary.
    inline unsigned long nextFrameTime(unsigned long thisFrame,
                                       long interval)
    {
        long lag = elapsedSince(thisFrame);
        if (interval > lag)
            lag = interval;
        return thisFrame + lag;
    }
} // namespace GameTime

#endif /* HOMM3_STRUCT_H */
