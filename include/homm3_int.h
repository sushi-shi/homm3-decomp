#ifndef HOMM3_INT_H
#define HOMM3_INT_H

// Width-named spellings of the exact integer types they replace. Macros, so
// each use expands to the original type; RAD's rad.h spells u32/s32 as long.
#undef u8
#undef s8
#undef u16
#undef s16
#undef u32
#undef s32
#undef u64
#undef s64
#define u8 unsigned char
#define s8 signed char
#define u16 unsigned short
#define s16 signed short
#define u32 unsigned int
#define s32 int
#define u64 unsigned __int64
#define s64 signed __int64

#endif
