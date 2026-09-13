// struct.h - the engine's small shared value types (E:\gamedcs\struct.h)
#ifndef HOMM3_STRUCT_H
#define HOMM3_STRUCT_H

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
    type_point(short newX, short newY, short newZ)
    {
        m_x = newX;
        m_y = newY;
        m_z = newZ;
    }
    bool isValid() const;
    unsigned char operator==(const type_point* arg);
    bool operator==(const type_point& arg) const
    {
        return m_x == arg.m_x && m_y == arg.m_y && m_z == arg.m_z;
    }
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

#endif /* HOMM3_STRUCT_H */
