#ifndef HOMM3_ADVENTURE_OBJECT_SUBTYPE_H
#define HOMM3_ADVENTURE_OBJECT_SUBTYPE_H

// The three cartographer variants, indexed by NewmapCell::objectIndex.
// DispatchEvent's cartographer arm shows one map plane per value, matching
// the three-wide cartographerMask/cartographerFlags arrays.
enum ECartographerType {
    CARTOGRAPHER_WATER = 0,
    CARTOGRAPHER_LAND = 1,
    CARTOGRAPHER_UNDERGROUND = 2
};

// Dreamcast CodeView supplies the names/order; retail's help-text selector
// bounds the player bit, then indexes this domain through a 400-byte traits
// record. Only the currently consumed name field is exposed.
enum type_creature_bank_type {
    CREATURE_BANK_CYCLOPS = 0,
    CREATURE_BANK_DWARF,
    CREATURE_BANK_GRIFFIN,
    CREATURE_BANK_IMP,
    CREATURE_BANK_MEDUSA,
    CREATURE_BANK_NAGA,
    CREATURE_BANK_DRAGONFLY,
    CREATURE_BANK_SHIPWRECK,
    CREATURE_BANK_DERELICT,
    CREATURE_BANK_SEPULCHER,
    CREATURE_BANK_DRAGON,
    CREATURE_BANK_COUNT
};

#endif
