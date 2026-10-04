#ifndef HOMM3_MAP_DIMENSIONS_H
#define HOMM3_MAP_DIMENSIONS_H

// The four square map dimensions MAP_WIDTH/MAP_HEIGHT take, named so
// UpdateRadar's three `switch (MAP_HEIGHT)` bodies case on a domain rather
// than on literals. The values are retail's own switch labels, decoded out
// of the two 109-byte index tables at 0x4135e4 and 0x413714; the names are
// HoMM3's published map sizes. advmgr.h already carried 144 as
// ADVENTURE_XLARGE_MAP_WIDTH inside advManager's sound-extent enum - that
// enumerator is left alone, this is the domain's own home.
enum EMapDimension {
    MAP_DIMENSION_SMALL = 36,
    MAP_DIMENSION_MEDIUM = 72,
    MAP_DIMENSION_LARGE = 108,
    MAP_DIMENSION_EXTRA_LARGE = 144
};

#endif
