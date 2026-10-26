#ifndef HOMM3_HEXCELL_H
#define HOMM3_HEXCELL_H

#include "va.h"

#include "struct.h"

class army;

// Head model from the retail ctor 0x4e7150 and GenerateMap 0x4642d0.
// The seven leading screen-geometry shorts use the names and offsets in
// Dreamcast CodeView; their x/y formulas and the remaining initialized
// fields are independently byte-proven by the retail GenerateMap body.
class hexcell {
public:
    // DC TAttributes: IS_OBSTACLE_ORIGIN, IS_BLOCKED, IS_QUICKSAND,
    // IS_LANDMINE, IS_FIREWALL, IS_STONEWALL, IS_OBSTACLIZED and
    // IS_MAGIC_OBSTACLE. Retail spell placement and
    // removal use these same bits; keep the integer field's existing ABI.
    enum TAttributes {
        obstacleOrigin = 0x01,
        blocked = 0x02,
        quicksand = 0x04,
        landMine = 0x08,
        fireWall = 0x10,
        stoneWall = 0x20,
        obstacleMask = obstacleOrigin | blocked | quicksand | landMine
                       | fireWall | stoneWall,
        magicObstacleMask = quicksand | landMine | fireWall | stoneWall
    };

    // Screen-space bounds initialized by combatManager::GenerateMap; the
    // seven geometry fields are shorts.
    short refX;
    short refY;
    short hexULX;
    short hexULY;
    short hexBRX;
    short hexBRY;
    short fullHexBRY;
    // Dreamcast ends its seven geometry shorts with fullHexBRY at +12
    // and starts attributes at +16, matching retail. No eighth semantic
    // short exists: these two bytes align the integer.
    short m_paddingBeforeAttributes;
    int attributes;
    int obstacleIndex;
    // Signed: get_army (0x4e7170) movsx-loads the pair and treats a
    // negative group as empty; the dead arrays hold 14 slots each and
    // get_dead_army (0x4e71b0) indexes both by the same i.
    signed char armyGroup;          // +0x18
    signed char armyIndex;          // +0x19
    // Signed like the pair above: the ctor feeds all four -1 byte
    // fields from the SAME register as the int -1 (cl); an unsigned
    // declaration splits (uchar)-1 = 0xff into a separately
    // materialized constant, which retail's bytes rule out.
    signed char partOfDouble;
    // Dreamcast armyGroup/armyIndex/partOfDouble occupy bytes 24..26;
    // retail preserves the gap before the corpse-count dword at +28.
    unsigned char m_paddingBeforeBodiesInHex;
    // DC hexcell.iBodiesInHex (members.csv hexcell@28), and
    // combatManager::remove_corpse (0x5a7320) proves the role on retail
    // bytes: it scans this many entries of the three parallel dead-army
    // rows, shifts the tail down over the match, and decrements it.
    int iBodiesInHex;              // +0x1c
    signed char deadArmyGroup[14];  // +0x20
    signed char deadArmyIndex[14];  // +0x2e
    // A THIRD parallel dead-army row: remove_corpse shifts +0x20, +0x2e
    // and +0x3c together,
    // one byte per slot per iteration, so the third one is 14 bytes wide
    // and indexed by the same i. The DC dump names it - members.csv
    // hexcell@60 deadPartOfDouble, right after deadArmyGroup@32 and
    // deadArmyIndex@46, which are this class's own +0x20 / +0x2e. The
    // whole DC record is unshifted against retail here (armyGroup@24 =
    // +0x18, bValidMove@74 = +0x4a, cloudLimitData@96 = +0x60), so the
    // three offsets corroborate each other. Only the first two rows take
    // the -1 "empty" sentinel at the tail; this one is left as it lies,
    // which is what a partOfDouble flag would want.
    signed char deadPartOfDouble[14];  // +0x3c
    // "This cell is reachable / interesting for the combat search":
    // ai_tactical's get_hypnotize_value (0x43a500) skips any stack
    // whose gridIndex cell has it clear, right after seeding the
    // search.
    unsigned char bValidMove;        // +0x4a
    // Set for the TAIL hex of a two-hex stack while its anchor hex gets
    // bValidMove: searchArray::SeedCombatPosition (0x4b2da0) marks the two
    // halves through the two different bytes.
    unsigned char front_move;        // +0x4b
    unsigned char mouse_shaded;
    // DC members.csv names +0x4d background_offset. Complete's
    // UpdateMouseGrid stores the clean-background atlas lane here before
    // shading a cell, then consumes lane*45 while restoring that cell.
    signed char background_offset;
    // Object stride is 0x70, byte-proven by ValidAttack's cell access
    // (0x523bb0: index*112 + 0x1c4 into combatManager).
    // Dreamcast hexcell type 0x6665 names
    // obstacleLimitData/cloudLimitData as SLimitData at +0x50/+0x60;
    // NH3API independently gives the same PC offsets. Retail confirms the
    // preceding background_offset byte at +0x4d and the 0x70 cell stride.
    // +0x4e..+0x4f align the first four-int bounds record.
    SLimitData obstacleLimitData;   // +0x50
    SLimitData cloudLimitData;      // +0x60

    hexcell();
    // Const on their own S_PUB32 publics (?get_army@hexcell@@QBAPAVarmy@@XZ,
    // ?get_dead_army@hexcell@@QBAPAVarmy@@H@Z) - the roster text at the
    // foot of this header cannot express that, and combatManager's own
    // const enemy_is_adjacent could not compile without it.
    army* get_army() const;
    army* get_dead_army(int i) const;

    // DC HexCell.h:85. The Complete UpdateGrid caller expands the returned
    // four-word rectangle and SLimitData::Include into one union loop.
    DC_ADDRESS(0x087300, 0x44)
    SLimitData limits() const
    {
        return SLimitData(hexULX, hexULY, hexBRX, fullHexBRY);
    }

    // The DC roster's hexcell::HasArmy (HexCell.h:90) - a
    // class-body inline on that build too, and retail carries no
    // out-of-line copy anywhere - the /Ob2 inline-away case.
    // army::ProcessDeath (0x444120) is the decoded consumer: the guard
    // folds to retail's sign test on armyGroup, and the call SITE is
    // itself load-bearing for ProcessDeath's inline budget (a free
    // candidate site in C2's sites-remaining divisor - measured there).
    // The DC const-this record also permits DoorCanBeLowered to call it.
    // Original public ?HasArmy@hexcell@@QBA_NXZ proves the bool result.
    DC_ADDRESS(0x04cc68, 0xa)
    bool HasArmy() const
    {
        return armyGroup >= 0;
    }
};
SIZE(hexcell, 0x70);

#endif  /* HOMM3_HEXCELL_H */
