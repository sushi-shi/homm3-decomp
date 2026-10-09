// ObjectTypeTable.h - the map editor's object types and palette slots
// (Loki h3maped objecttype.h): the rows of objects.txt, which ObjectType.cpp
// loads at start-up into the table at 0x5a2398 and binds kObjectTypeTable
// to (0x49291b), and the slots of apObjectSlotTraits the palette lists them
// in.
#ifndef HOMM3_EDITOR_OBJECTTYPETABLE_H
#define HOMM3_EDITOR_OBJECTTYPETABLE_H

#include <functional>

#include "va.h"
#include "objecttype.h"

DATA(0x005a23a8) extern const TObjectTypeTable& kObjectTypeTable;

// The map editor's object palette slots: one per terrain but rock, the
// all-terrain slot and one per non-generic category, in the order of
// apObjectSlotTraits (Loki h3maped objecttype.h; the enumerator names
// other than kNumObjectSlots and eSlotHeroes are not proven).
enum TObjectSlot {
    eSlotDirt,
    eSlotSand,
    eSlotGrass,
    eSlotSnow,
    eSlotSwamp,
    eSlotRough,
    eSlotSubterranean,
    eSlotLava,
    eSlotWater,
    eSlotAllTerrain,
    eSlotTowns,
    eSlotMonsters,
    eSlotHeroes,
    eSlotArtifacts,
    eSlotTreasures,
    kNumObjectSlots
};

inline bool objectTypeInSlot(const TObjectType& objType, TObjectSlot slot)
{
    return apObjectSlotTraits[slot]->contains(objType);
}

// objectTypeInSlot as an adaptable predicate: the palette binds the slot
// (h3maped 0x48d4c4, binder2nd's call operator).
struct TObjectTypeInSlotPred : public std::binary_function<TObjectType, TObjectSlot, bool> {
    bool operator()(const TObjectType& objType, TObjectSlot slot) const
    {
        return objectTypeInSlot(objType, slot);
    }
};

#endif  /* HOMM3_EDITOR_OBJECTTYPETABLE_H */
