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
#if defined(HOMM3_PHILAI_OBJ_DECLS) || defined(HOMM3_AI_PLAYER_OBJ_DECLS)
#pragma pack(push, 1)
#endif
struct type_point {
    short x : 10;
    short y : 10;
    short z : 4;

    type_point() {}
    type_point(short new_x, short new_y, short new_z);
    unsigned char operator==(const type_point* arg);
    // Dreamcast S_PUB32 is ??8type_point@@QBA_NABU0@@Z: bool return,
    // const member, const-reference operand. Keep the pointer overload above
    // temporarily for older reconstructed callers while source-aligned sites
    // use the proven operator.
    bool operator==(const type_point& arg) const
    {
        return x == arg.x && y == arg.y && z == arg.z;
    }
    // Dreamcast retains this source helper out of line in
    // AI_AttemptMove; Complete VC6 expands the same three comparisons.
    bool operator!=(const type_point& arg) const
    {
        return x != arg.x || y != arg.y || z != arg.z;
    }
    unsigned char is_valid();
};
#if defined(HOMM3_PHILAI_OBJ_DECLS) || defined(HOMM3_AI_PLAYER_OBJ_DECLS)
#pragma pack(pop)
#endif

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
#ifdef HOMM3_DRAWING_UPDATE_GRID_DECLS
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
#endif
};
SIZE(SLimitData, 0x10);

#endif /* HOMM3_STRUCT_H */
