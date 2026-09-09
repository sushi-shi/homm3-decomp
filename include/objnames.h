// objnames.h - the adventure-object trait rows and the objnames.txt name
// table that fills them.
//
// Kept narrow deliberately: only objnames.cpp writes these rows. Every
// reader in the tree reaches them through the `unsigned char (*)[16]`
// pointer at 0x660428 that advmgr.h and mapcell.h already declare, and
// that pointer's value in the retail image IS 0x691698, this table.
#ifndef HOMM3_OBJNAMES_H
#define HOMM3_OBJNAMES_H

#include <va.h>

enum {
    // The loader's own bound: it zeroes 232 rows, walks 232 text rows
    // (`cmp edx,0x3a0` over a four-byte stride) and stops the name-copy
    // loop at 0x69251c, which is 0x69169c + 232 * 0x10.
    ADVENTURE_OBJECT_TRAIT_COUNT = 232
};

// 16-byte row, every offset written by the loader at 0x41b500. Only the
// first byte has an attested role - mapcell.h names it the trigger-object
// landing veto and cursor/findpath read byte +1 - so the remaining flags
// keep neutral spellings.
struct TAdvObjectTraits {
    // Before normalization: blocksLanding.
    unsigned char m_blocksLanding;  // +0x00
    // Before normalization: trait1.
    unsigned char m_trait1;         // +0x01
    // Before normalization: trait2.
    unsigned char m_trait2;         // +0x02
    // Before normalization: name.
    const char* m_name;             // +0x04
    // Before normalization: nameRow.
    int m_nameRow;                  // +0x08
    // Before normalization: trait3.
    unsigned char m_trait3;         // +0x0c
};
SIZE(TAdvObjectTraits, 0x10);

// One row of the loader's first .rdata override table: the object id and
// the objnames.txt line its name comes from.
struct TAdvObjectNameRow {
    // Before normalization: objectType.
    int m_objectType;
    // Before normalization: nameRow.
    int m_nameRow;
};

// Before normalization: gAdventureObjectTraitRows.
extern TAdvObjectTraits g_adventureObjectTraitRows[ADVENTURE_OBJECT_TRAIT_COUNT];

void initializeAdventureObjectNames();

#endif  /* HOMM3_OBJNAMES_H */
