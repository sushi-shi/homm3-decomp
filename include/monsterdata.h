// MonsterData - per-instance adventure-map monster payload.
#ifndef HOMM3_MONSTERDATA_H
#define HOMM3_MONSTERDATA_H

#include <string>
#include "artifact.h"

// Dreamcast CodeView supplies the member names and order. Retail keeps the
// seven-dword resource array but widens the leading STL string from 12 to 16
// bytes, placing Artifact at +0x2c; saveMonsterData independently proves both
// the array base/extent and that final offset.
// The map file's five wandering-monster quantity presets.  PROVISIONAL
// NAMES: no Dreamcast enum covers this domain, so each is named for the roll
// readMonsterData performs on it (0x5013b0's five-arm jump table).  Grade 0
// stores the sentinel -4 and is resolved elsewhere.
enum EMonsterQuantityPreset {
    MONSTER_QTY_UNRESOLVED = 0,
    MONSTER_QTY_RANDOM_1_7 = 1,
    MONSTER_QTY_RANDOM_1_10 = 2,
    MONSTER_QTY_RANDOM_4_10 = 3,
    MONSTER_QTY_FIXED_10 = 4
};

class MonsterData {
public:
    std::basic_string<char, std::char_traits<char>, std::allocator<char> > Message;
    int ResQty[7];
    // Spelled int, not TArtifact, for the reason armyGroup::armies is
    // spelled int: readMonsterData deserializes it from a one- or two-byte
    // stream field and saveMonsterData narrows it back to a byte, so an
    // enum here would put a cast on every crossing.  ARTIFACT_NONE still
    // assigns.  The Dreamcast declarator's enum is preserved in the name.
    int Artifact;

    // loadMonsterList's resize temp proves a header-inline constructor: the
    // default argument `_Ty()` that Dinkumware's resize materializes stores
    // -1 into Artifact right after the base string is tidied, and nothing in
    // resize can be doing that.  readMonsterData's own temp shows the same
    // single store, so the assignment it used to spell by hand is this
    // constructor's and has been removed there.
    MonsterData() { Artifact = ARTIFACT_NONE; }
};
SIZE(MonsterData, 0x30);

#endif
