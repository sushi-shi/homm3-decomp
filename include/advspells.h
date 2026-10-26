#ifndef HOMM3_ADVSPELLS_H
#define HOMM3_ADVSPELLS_H

#include "va.h"

#include "struct.h"

// Retail .data 0x691250, the adventure map's on-screen viewport rectangle -
// ONE SLimitData, not four loose ints. Its dynamic initialiser at 0x405db0
// (an excluded cinit) is the whole writer and builds it through the
// four-argument constructor as (8, 8, 0x267, 0x227): the map view's inclusive
// pixel bounds on the 800x600 adventure screen.

// What proves it is one object rather than four is the CONSUMER shape, not
// the addresses. advManager::SkuttleBoat (0x41cf47) and
// advManager::SummonBoat (0x41cb61, 0x41cc57) each read the four dwords in
// the order iMinX / iMinY / iMaxX / iMaxY with `<` on the two minima and `>`
// on the two maxima, storing back only on the losing side - which is
// SLimitData::Clip's body verbatim - and then take iMaxX-iMinX+1 and
// iMaxY-iMinY+1 to feed heroWindowManager::SaveFizzleSourceX, i.e. Width()
// and Height(). advspells.obj's own Dreamcast roster carries all three of
// those helpers (dc 0x23014 / 0x23020 / 0x2302c) and nothing else in the
// compiland would need them.

// Exactly four references apiece exist in the image (retail-reloc-evidence):
// the cinit at 0x5db7..0x5dcb plus those three consumer sites, so the object
// is declared HERE, in advspells.obj's own narrow header, rather than in
// advmgr.h - which twenty-odd translation units include. Name is a role
// description; nothing attests a spelling.
extern SLimitData g_advMapViewLimits;

#endif  /* HOMM3_ADVSPELLS_H */
