// heroclass.h - the hero classes, which the map editor's dialogs also use
// without the game's hero.h.
#ifndef HOMM3_HEROCLASS_H
#define HOMM3_HEROCLASS_H

#include "domains.h"

// Hero-class ids. Dreamcast CodeView supplies the original 0..15 ladder
// and its spellings; retail GetNewHeroId extends it with the two Conflux
// classes, indexes all eighteen class-traits rows, and uses 18 as the
// no-class sentinel.
H3_ENUM_BEGIN(THeroClass)
    eClassKnight = 0,
    eClassCleric = 1,
    eClassRanger = 2,
    eClassDruid = 3,
    eClassAlchemist = 4,
    eClassWizard = 5,
    eClassPagan = 6,
    eClassHeretic = 7,
    eClassDeathKnight = 8,
    eClassNecromancer = 9,
    eClassOverlord = 10,
    eClassWarlock = 11,
    eClassBarbarian = 12,
    eClassBattleMage = 13,
    eClassBeastmaster = 14,
    eClassWitch = 15,
    eClassPlanesWalker = 16,
    eClassElementalist = 17,
    kNumHeroClasses = 18
H3_ENUM_END(THeroClass)
H3_ENUM_STEPPED(THeroClass)

#endif  // HOMM3_HEROCLASS_H
