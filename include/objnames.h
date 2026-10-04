// objnames.h - the adventure-object trait rows and the objnames.txt name
// table that fills them.

// Only objnames.cpp writes these rows. Readers share the const record pointer
// at retail 0x660428, whose initial value is the table address 0x691698.
#ifndef HOMM3_OBJNAMES_H
#define HOMM3_OBJNAMES_H

#include "va.h"
#include "adventure_object_data.h"

// 16-byte row, every offset written by the loader at 0x41b500 from the
// object-type id tables in objnames.cpp.
struct TAdvObjectTraits {
    // Trigger-object landing veto (mapcell.h).
    unsigned char m_blocksLanding;       // +0x00
    // The trigger cell can be entered from and left to its north side
    // (findpath, cursor); small objects such as pickups and monsters.
    unsigned char m_enterableFromNorth;  // +0x01
    // Stops blocking once visited: pickups, monsters, prisons, guards,
    // border gates, events and heroes.
    unsigned char m_clearedOnVisit;      // +0x02
    const char* m_name;                  // +0x04
    int m_nameRow;                       // +0x08
    // Decorative obstacle (object types 114-211); drawn by View World's
    // terrain view and written first by the random map generator.
    unsigned char m_isDecoration;        // +0x0c
};
SIZE(TAdvObjectTraits, 0x10);

extern TAdvObjectTraits g_adventureObjectTraitRows[ADVENTURE_OBJECT_TRAIT_COUNT];

extern const TAdvObjectTraits* g_adventureObjectTraits;

void initializeAdventureObjectNames();

#endif  /* HOMM3_OBJNAMES_H */
