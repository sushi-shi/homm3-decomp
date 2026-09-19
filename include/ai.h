// ai.h - prototypes of ai.cpp (compiland ai.obj)
#ifndef HOMM3_AI_H
#define HOMM3_AI_H

class army;
struct type_AI_combat_parameters;

// The three shooters whose splash damage choose_shooter_target prices over
// both occupied hexes. Values and spellings are Dreamcast-attested; kept in
// this narrow header because adding members to the widely included creature
// enum perturbs VC6 code generation in initialize_game_data.
enum EAreaAttackCreature {
    CREATURE_MAGOG = 45,
    CREATURE_POWER_LICH = 65,
    CREATURE_DEMON = 0x30,
    CREATURE_PIT_LORD = 0x33,
    // The one pair choose_melee_target names: 0x421680 compares
    // current_army->creatureType against 0x48 and 0x49 back to back and
    // gives both the same arm - take the raw hex value instead of adding
    // it to the accumulated change, which is the no-retaliation flyer
    // rule. 0x48/0x49 are the two-slot Harpy run of the Dungeon roster,
    // immediately below Beholder 0x4a. They live here for the same
    // reason every enumerator above does: armygrp.h's roster is included
    // nearly tree-wide, and adding to it moved initialize_game_data off
    // exact.
    CREATURE_HARPY = 0x48,
    CREATURE_HARPY_HAG = 0x49,
    // The creature a besieged town's keep and towers shoot AS:
    // has_ranged_advantage (0x420a80) prices each of them off
    // akCreatureTypeTraits[2].AI_value (`[table + 0x128]`, i.e. record
    // 2's field +0x40). 2 is the Archer, third row of the Castle
    // dwelling run below Pikeman 0 and Halberdier 1. Here for the same
    // reason as every enumerator above.
    CREATURE_ARCHER = 0x2,
    // The rest of army::LoadResources' shooter roster (0x43d9f0): its
    // missile-sprite switch maps every one of these ids to a named
    // projectile .def (the archers above to the crossbow bolt, the
    // monks to the zealot beam, both elves plus armygrp.h's
    // SHARPSHOOTER to the elf arrow, and so on), and every pair lands
    // exactly where the Complete dwelling runs the wide enum's anchors
    // put it (WOOD_ELF/GRAND_ELF directly under PEGASUS 0x14/0x15,
    // POWER_LICH above beside armygrp.h's LICH 0x40, BEHOLDER between
    // HARPY_HAG 0x49 above and MEDUSA 0x4c). VAMPIRE/VAMPIRE_LORD are
    // proven by the same body's extra-sample gate (the ext1/ext2 pair
    // they share with the two devils). They live HERE and not in
    // armygrp.h for the measured reason every enumerator above does:
    // adding this block to the wide roster took initialize_game_data
    // 100.00 -> 90.16 (2026-08-20); in this narrow header the canaries
    // do not move. NH3API spellings, Complete numbering.
    CREATURE_MARKSMAN = 0x3,
    CREATURE_MONK = 0x8,
    CREATURE_ZEALOT = 0x9,
    CREATURE_WOOD_ELF = 0x12,
    CREATURE_GRAND_ELF = 0x13,
    CREATURE_MASTER_GREMLIN = 0x1d,
    CREATURE_TITAN = 0x29,
    CREATURE_GOG = 0x2c,
    // CREATURE_VAMPIRE now belongs to TCreatureType: the bank's
    // native guard table and this sample gate share the same domain.
    CREATURE_VAMPIRE_LORD = 0x3f,
    CREATURE_BEHOLDER = 0x4a,
    CREATURE_EVIL_EYE = 0x4b,
    CREATURE_ORC = 0x58,
    CREATURE_ORC_CHIEFTAIN = 0x59,
    CREATURE_LIZARDMAN = 0x64,
    CREATURE_LIZARD_WARRIOR = 0x65,
    // army::new_turn (0x446e30) regenerates exactly three creatures
    // unconditionally: WIGHT 0x3c, WRAITH 0x3d and this one - 0x90 is
    // the Troll, the one regenerating neutral, bracketed by the proven
    // ROGUE 0x8f and CATAPULT 0x91 in armygrp.h's roster.
    CREATURE_TROLL = 0x90,
    // army::compute_attacker_bonus (0x443320) proves the four: the
    // jousting bonus (CAVALIER/CHAMPION) is void against 0 and 1 - the
    // Castle root pair, Pikeman and Halberdier - and the hate ladder
    // pairs the plain Genie 0x24 and plain Efreeti 0x34 with the
    // MASTER_GENIE 0x25 / EFREET_SULTAN 0x35 armygrp.h already proves.
    CREATURE_PIKEMAN = 0x0,
    CREATURE_HALBERDIER = 0x1,
    CREATURE_GENIE = 0x24,
    CREATURE_EFREETI = 0x34,
    // army::do_post_attack's dispatch (0x440bc0) proves the four: the
    // Thunderbird's 20% lightning at 0x5d (Stronghold's upgraded Roc),
    // the Mighty Gorgon's death stare at 0x67, and the Serpent/Dragon
    // Fly dispel pair at 0x68/0x69 (only 0x69 adds the Weakness cast),
    // each id bracketed by the proven runs around it.
    CREATURE_THUNDERBIRD = 0x5d,
    CREATURE_MIGHTY_GORGON = 0x67,
    // CREATURE_DRAGON_FLY likewise moved to TCreatureType for the
    // native bank guard table; the on-attack evidence above still holds.
    CREATURE_SERPENT_FLY = 0x68
};

// --- globals ---
long getAreaAttackValue(const army* currentArmy, long hex, long ourGroup,
                           type_AI_combat_parameters* data);
// CODEVIEW(E:\gamedcs\ai.cpp:610, dc 0x24604) long get_move_order(const army* current_army);
// CODEVIEW(E:\gamedcs\ai.cpp:696, dc 0x248b4) long get_attack_value(const army* current_army, const army* enemy, long enemy_hit_points, type_AI_combat_parameters& data);
// CODEVIEW(E:\gamedcs\ai.cpp:1000, dc 0x250e0) long get_enemy_attack_limit(const army* our_army, const type_AI_combat_parameters* estimate);
// CODEVIEW(E:\gamedcs\ai.cpp:1121, dc 0x253a8) void find_attack_hexes(const army* our_army, const army* enemy, const searchArray* search_array, std::vector<long,std::allocator<long>* result);
// CODEVIEW(E:\gamedcs\ai.cpp:2397, dc 0x273d8) void simulate_simple_attack(army* current_army, army* target, long distance, unsigned char ranged, unsigned char breath_attack);

// --- army ---
// CODEVIEW(E:\gamedcs\ai.cpp:1786, dc 0x28068) void army::~army();

// --- combatManager ---
// CODEVIEW(E:\gamedcs\ai.cpp:43, dc 0x23450) int combatManager::ChooseBallistaTarget(int target_group, int attack_skill, int average_damage);
// CODEVIEW(E:\gamedcs\ai.cpp:162, dc 0x2389c) unsigned char combatManager::AICheckRetreat();
// CODEVIEW(E:\gamedcs\ai.cpp:1152, dc 0x2544c) void combatManager::mark_multiheaded_enemy(const army* our_army, const army* enemy, long* enemy_attacks, long limit_value, searchArray* search_array, const type_AI_combat_parameters* estimate);
// CODEVIEW(E:\gamedcs\ai.cpp:1357, dc 0x25998) unsigned char combatManager::choose_defense_hex(const army* current_army, const army* client, long* best_hex, long* open_hexes, const searchArray* search_array);
// CODEVIEW(E:\gamedcs\ai.cpp:1635, dc 0x2600c) unsigned char combatManager::choose_creature_spell(const army* current_army, long* best_value, type_AI_combat_parameters* estimate);
// CODEVIEW(E:\gamedcs\ai.cpp:1694, dc 0x26140) unsigned char combatManager::choose_resurrect_action(const army* current_army, long* best_value, type_AI_combat_parameters* estimate);
// CODEVIEW(E:\gamedcs\ai.cpp:1896, dc 0x266d4) unsigned char combatManager::choose_melee_target(const army* current_army, unsigned char teleport, long* action_value, type_AI_combat_parameters* estimate);
// CODEVIEW(E:\gamedcs\ai.cpp:2272, dc 0x27138) void combatManager::DoCompAI(int whichGroup);
// CODEVIEW(E:\gamedcs\ai.cpp:2608, dc 0x27888) void combatManager::find_AI_targets(long our_group, const army* current_army, unsigned char melee_only, const type_AI_combat_parameters* data, searchArray* search_array);

// --- func_moves_before ---
// CODEVIEW(E:\gamedcs\ai.cpp:597, dc 0x28024) unsigned char func_moves_before::operator()(const army* first, const army* second);

// --- type_spellvalue ---
// CODEVIEW(E:\gamedcs\ai.cpp:1626, dc 0x28050) void type_spellvalue::~type_spellvalue();

#endif  /* HOMM3_AI_H */
