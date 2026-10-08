// objnames.h - the adventure-object type traits (AdvObjectTypeTraits.cpp of
// the Loki port) and the objnames.txt names that fill them.
#ifndef HOMM3_OBJNAMES_H
#define HOMM3_OBJNAMES_H

#include "adventureobjecttype.h"

// 8-byte RoE record, indexed by TAdventureObjectType. The editor's map
// validation tests the first two flags; the three are the roles the
// Complete loader gives the same rows.
struct TAdvObjectTypeTraits {
    // Trigger-object landing veto.
    bool m_blocksLanding;
    // The trigger cell can be entered from and left to its north side.
    bool m_enterableFromNorth;
    // Stops blocking once visited.
    bool m_clearedOnVisit;
    const char* m_name;
};

#if defined(HOMM3_TARGET_LOKI)
// The Loki editor sizes the table: TObjectPaletteWnd::OnToolTipNeedText
// asserts "pObjType->getType() < ( sizeof( akAdvObjectTypeTraits ) /
// sizeof( ( akAdvObjectTypeTraits )[ 0 ] ) )" and compares with 164, so it
// sees a reference to the 165-row array (the exported 4-byte cell holds
// its address either way).
extern const TAdvObjectTypeTraits (&akAdvObjectTypeTraits)[MAX_EVENT_TYPE];
#else
extern const TAdvObjectTypeTraits* akAdvObjectTypeTraits;
#endif

void InitializeAdvObjectTypeTraitsTable();

#endif  /* HOMM3_OBJNAMES_H */
