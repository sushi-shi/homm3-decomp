// creature_bank.h - creature_bank.cpp (compiland creature_bank.obj)
#ifndef HOMM3_CREATURE_BANK_H
#define HOMM3_CREATURE_BANK_H

#include <string>
#include <vector>
#include "creature_bank_types.h"

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

// The three guard shapes initialize_creature_bank's splitter knows, named
// because they are switch labels and the tree cases on enumerators. Retail
// dispatches on armyGroup::GetNumArmies() and handles exactly these three -
// one stack becomes five groups, two become 2+3 and three become 2+2 - so
// the domain is real even though no external source names it. PROVISIONAL.
enum type_creature_bank_guard_shape {
    CREATURE_BANK_GUARDS_ONE_STACK = 1,
    CREATURE_BANK_GUARDS_TWO_STACKS = 2,
    CREATURE_BANK_GUARDS_THREE_STACKS = 3
};

// Retail's constructor at 0x47aad0 walks four records at a 0x60 stride and
// invokes armyGroup::armyGroup at the start of each. The still-unread tail
// is kept opaque until initialize_creature_bank names its reward fields.
struct type_creature_bank_level {
    armyGroup m_guards;
    int m_resources[7];
    TCreatureType m_rewardCreature;
    signed char m_rewardCreatures;
    signed char m_chance;
    signed char m_upgradeChance;
    signed char m_treasureArtifacts;
    signed char m_minorArtifacts;
    signed char m_majorArtifacts;
    signed char m_relicArtifacts;
    // The trait loader stores relicArtifacts as the last byte at +0x5e.
    // The retail table walks 0x60-byte records; this byte aligns their extent.
    char m_tailPadding;

    type_creature_bank_level();
};
SIZE(type_creature_bank_level, 0x60);

// The constructor first initializes the retail 16-byte Dinkumware string
// consumed by the help-text selector, then the four levels above. This
// independently closes the 0x190 stride; only the old anonymous tail is
// retired.
struct type_creature_bank_traits {
    std::string m_name;
    type_creature_bank_level m_levels[4];

    type_creature_bank_traits();
};
SIZE(type_creature_bank_traits, 0x190);

extern const type_creature_bank_traits* g_constCreatureBankTraits;

void initializeCreatureBank(type_creature_bank* bank,
                              type_creature_bank_type type);

#endif  /* HOMM3_CREATURE_BANK_H */
