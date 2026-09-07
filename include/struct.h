// struct.h - the engine's small shared value types (E:\gamedcs\struct.h)
// HAND-OWNED. Class layouts are NOT fabricated from method symbols;
// prototypes stay comments until a retail layout is proven.
#ifndef HOMM3_STRUCT_H
#define HOMM3_STRUCT_H

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
    // Before normalization (locals): new_x, new_y, new_z.
    type_point(short newX, short newY, short newZ)
    {
        m_x = newX;
        m_y = newY;
        m_z = newZ;
    }
    unsigned char operator==(const type_point* arg);
    // Dreamcast S_PUB32 is ??8type_point@@QBA_NABU0@@Z: bool return,
    // const member, const-reference operand. Keep the pointer overload above
    // temporarily for older reconstructed callers while source-aligned sites
    // use the proven operator.
    bool operator==(const type_point& arg) const
    {
        return m_x == arg.m_x && m_y == arg.m_y && m_z == arg.m_z;
    }
    // Dreamcast retains this source helper out of line in
    // AI_AttemptMove; Complete VC6 expands the same three comparisons.
    bool operator!=(const type_point& arg) const
    {
        return m_x != arg.m_x || m_y != arg.m_y || m_z != arg.m_z;
    }
    // Before normalization (function): type_point::is_valid.
    unsigned char isValid();
    // E:\gamedcs\struct.h:120, and advspells.obj's own Dreamcast roster
    // retains it out of line (dc 0x22fe4, 0x2e B). Retail has no body:
    // advManager::TownGate (0x41d360) is the admitted witness and EXPANDS
    // it twice in one statement pair - once for the `< best` test and
    // once for the store into `best`, each time as
    // `(this->x - p2->x)^2 + (this->y - p2->y)^2` with the two 10-bit
    // bitfields sign-extended through the shl/movsx/sar triple. The z
    // plane takes no part, which is what makes it a MAP distance.
    // Before normalization (function): type_point::DistanceSquared.
    int distanceSquared(const type_point* p2) const
    {
        int dy = m_y - p2->m_y;
        int dx = m_x - p2->m_x;
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

#endif /* HOMM3_STRUCT_H */
