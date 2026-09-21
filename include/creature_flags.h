#ifndef HOMM3_CREATURE_FLAGS_H
#define HOMM3_CREATURE_FLAGS_H

enum creatureFlags {
    creatureDoubleWide            = 0x00000001U,  // CF_DOUBLE_WIDE
    creatureFlyingArmy            = 0x00000002U,  // CF_FLYING_ARMY
    creatureShootingArmy          = 0x00000004U,  // CF_SHOOTING_ARMY
    creatureHasExtendedAttack     = 0x00000008U,  // CF_HAS_EXTENDED_ATTACK
    creatureAlive                 = 0x00000010U,  // CF_ALIVE
    creatureCatapult              = 0x00000020U,  // CF_CATAPULT
    creatureSiegeWeapon           = 0x00000040U,  // CF_SIEGE_WEAPON
    creatureKing1                 = 0x00000080U,  // CF_KING_1
    creatureKing2                 = 0x00000100U,  // CF_KING_2
    creatureKing3                 = 0x00000200U,  // CF_KING_3
    creatureImmuneToMindSpells     = 0x00000400U,  // CF_IMMUNE_TO_MIND_SPELLS
    creatureShootsRay             = 0x00000800U,  // CF_SHOOTS_RAY
    creatureNoMeleePenalty        = 0x00001000U,  // CF_NO_MELEE_PENALTY
    creatureUnused                = 0x00002000U,  // CF_UNUSED
    creatureImmuneToFireSpells     = 0x00004000U,  // CF_IMMUNE_TO_FIRE_SPELLS
    creatureTwoAttacks            = 0x00008000U,  // CF_TWO_ATTACKS
    creatureFreeAttack            = 0x00010000U,  // CF_FREE_ATTACK
    creatureNoMorale              = 0x00020000U,  // CF_NO_MORALE
    creatureUndead                = 0x00040000U,  // CF_UNDEAD
    creatureMultiHeaded           = 0x00080000U,  // CF_MULTI_HEADED
    creatureFireballAttack        = 0x00100000U,  // CF_FIREBALL_ATTACK
    creatureImmobilized           = 0x00200000U,  // CF_IMMOBILIZED
    creatureSummoned              = 0x00400000U,  // CF_SUMMONED
    creatureClone                 = 0x00800000U,  // CF_CLONE
    creatureMorale                = 0x01000000U,  // CF_MORALE
    creatureWaiting               = 0x02000000U,  // CF_WAITING
    creatureDone                  = 0x04000000U,  // CF_DONE
    creatureDefending             = 0x08000000U,  // CF_DEFENDING
    creatureSacrificed            = 0x10000000U,  // CF_SACRIFICED
    creatureRedColoring           = 0x20000000U,  // CF_RED_COLORING
    creatureGreyColoring          = 0x40000000U,  // CF_GREY_COLORING
    creatureDragon                = 0x80000000U,  // CF_DRAGON
};

#endif
