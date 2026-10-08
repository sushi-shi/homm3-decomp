// creaturegenerator4.h - Creature Generator 4 sites.
#ifndef HOMM3_CREATUREGENERATOR4_H
#define HOMM3_CREATUREGENERATOR4_H

// The two sites of the Creature Generator 4 object class. When
// generator::m_genClass is CREATURE_GENERATOR_4, m_genType selects the row
// of g_creatureGenerator4Types: the four elementals, or the stone, iron,
// gold and diamond golems that mapcell.cpp's random-dwelling pass places as
// type 1 for CREATURE_STONE_GOLEM.
enum ECreatureGenerator4Type {
    CREATURE_GENERATOR_4_ELEMENTAL_CONFLUX = 0,
    CREATURE_GENERATOR_4_GOLEM_FACTORY = 1
};

#endif
