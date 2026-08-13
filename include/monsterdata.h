// MonsterData - per-instance adventure-map monster payload.
#ifndef HOMM3_MONSTERDATA_H
#define HOMM3_MONSTERDATA_H

#include <string>
#include "artifact.h"

// Dreamcast CodeView supplies the member names and order. Retail keeps the
// seven-dword resource array but widens the leading STL string from 12 to 16
// bytes, placing Artifact at +0x2c; saveMonsterData independently proves both
// the array base/extent and that final offset.
class MonsterData {
public:
    std::basic_string<char, std::char_traits<char>, std::allocator<char> > Message;
    int ResQty[7];
    TArtifact Artifact;
};
SIZE(MonsterData, 0x30);

#endif
