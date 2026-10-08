// GameResource.h - the editor's game resource (wood ... gold) type traits.
// The file name follows GameResource.cpp (Loki __FILE__); the enumerators
// other than wood (TAbandonedMine's asserts) and the count are not recovered
// yet.
#ifndef HOMM3_EDITOR_GAMERESOURCE_H
#define HOMM3_EDITOR_GAMERESOURCE_H

enum TGameResourceType {
    eResourceWood = 0,
    kNumGameResourceTypes = 7
};

// One record per resource type, filled from "restypes.txt".
struct TGameResourceTypeTraits {
    const char* m_name;
};

extern const TGameResourceTypeTraits* const akGameResourceTypeTraits;

void InitializeGameResourceTypeTraitsTable();

#endif  /* HOMM3_EDITOR_GAMERESOURCE_H */
