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
    short x : 10;
    short y : 10;
    short z : 4;

    type_point() {}
    type_point(short new_x, short new_y, short new_z);
    unsigned char operator==(const type_point* arg);
    unsigned char is_valid();
};

#ifdef HOMM3_SLIMITDATA_VIEW
// The shared inclusive rectangle used by the adventure and combat drawing
// code. Dreamcast CodeView fixes the four names, offsets, and 16-byte extent;
// retail's combat drawing wrappers read the same four dwords in this order.
// Keep the concrete view consumer-scoped: introducing a class into every TU
// that includes struct.h perturbs VC6's type-handle state.
struct SLimitData {
    int iMinX;
    int iMinY;
    int iMaxX;
    int iMaxY;
};
SIZE(SLimitData, 0x10);
#endif

#endif /* HOMM3_STRUCT_H */
