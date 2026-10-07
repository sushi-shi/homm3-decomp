// hero_type.h - shared hero-class and hero-id domains.
#ifndef HOMM3_HERO_TYPE_H
#define HOMM3_HERO_TYPE_H

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

// Dreamcast names the hero-ID domain THeroID; Complete uses 156 hero slots
// (Dreamcast kNumHeroes, 128 there).
// The -1 sentinel is serialized as0xff; hero.cpp independently identifies
// Xeron as0x9b. Other admitted IDs retain their existing corpus constants.
// The remaining original enumerator names are not yet recovered.
enum HeroId {
    heroIdNone = -1,
    heroIdXeron = 0x9b,
    kNumHeroes = 156
};

#endif  // HOMM3_HERO_TYPE_H
