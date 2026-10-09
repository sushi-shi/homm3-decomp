// GameResource.h - the editor's resource types (GameResource.cpp; Loki
// h3maped object 14).
#ifndef HOMM3_EDITOR_GAMERESOURCE_H
#define HOMM3_EDITOR_GAMERESOURCE_H

#include "va.h"

enum TGameResourceType {
    eResourceWood = 0,
    eResourceGold = 6,
    kNumGameResourceTypes = 7
};

// One row per resource type: its name (Loki's TGameResourceTypeTraits).
struct TGameResourceTypeTraits {
    const char* m_name;
};

// h3maped 0x584188: points at the rows (one per resource type).
extern const TGameResourceTypeTraits* akGameResourceTypeTraits;

void InitializeGameResourceTypeTraitsTable();

#endif  /* HOMM3_EDITOR_GAMERESOURCE_H */
