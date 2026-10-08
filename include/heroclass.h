// heroclass.h - the hero classes, which the map editor's dialogs also use
// without the game's hero.h.
#ifndef HOMM3_HEROCLASS_H
#define HOMM3_HEROCLASS_H

// Hero-class ids. Dreamcast CodeView supplies the original 0..15 ladder;
// retail GetNewHeroId extends it with the two Conflux classes, indexes all
// eighteen class-traits rows, and uses 18 as the no-class sentinel.
// Before normalization (Dreamcast enumerators): eClassKnight, eClassCleric,
// eClassRanger, eClassDruid, eClassAlchemist, eClassWizard, eClassPagan,
// eClassHeretic, eClassDeathKnight, eClassNecromancer, eClassOverlord,
// eClassWarlock, eClassBarbarian, eClassBattleMage, eClassBeastmaster,
// eClassWitch, eClassPlanesWalker, eClassElementalist.
enum THeroClass {
    classKnight = 0,
    classCleric = 1,
    classRanger = 2,
    classDruid = 3,
    classAlchemist = 4,
    classWizard = 5,
    classPagan = 6,
    classHeretic = 7,
    classDeathKnight = 8,
    classNecromancer = 9,
    classOverlord = 10,
    classWarlock = 11,
    classBarbarian = 12,
    classBattleMage = 13,
    classBeastmaster = 14,
    classWitch = 15,
    classPlanesWalker = 16,
    classElementalist = 17,
    kNumHeroClasses = 18
};

#endif  // HOMM3_HEROCLASS_H
