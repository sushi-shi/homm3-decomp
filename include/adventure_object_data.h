#ifndef HOMM3_ADVENTURE_OBJECT_DATA_H
#define HOMM3_ADVENTURE_OBJECT_DATA_H

enum {
    // The loader's own bound: it zeroes 232 rows, walks 232 text rows
    // (`cmp edx,0x3a0` over a four-byte stride) and stops the name-copy
    // loop at 0x69251c, which is 0x69169c + 232 * 0x10.
    ADVENTURE_OBJECT_TRAIT_COUNT = 232
};

// One row of the loader's first .rdata override table: the object id and
// the objnames.txt line its name comes from.
struct TAdvObjectNameRow {
    int m_objectType;
    int m_nameRow;
};

#endif
