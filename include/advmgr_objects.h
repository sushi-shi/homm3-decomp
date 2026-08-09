#ifndef HOMM3_ADVMGR_OBJECTS_H
#define HOMM3_ADVMGR_OBJECTS_H

#include <bitset>
#include <vector>
#include "mapcell.h"

class CSprite;

// Narrow renderer views of the retail map object pools. They live in the
// adventure renderer's header rather than changing the broad game.h include
// closure before the rest of NewfullMap is reconstructed.
struct AdvObjectCellView {
    unsigned short objectIndex;
    unsigned char offsets;
    signed char layer;
};
SIZE(AdvObjectCellView, 4);

struct AdvMapCellObjectsView {
    char pad_00[0xc];
    std::vector<AdvObjectCellView> objects;
};

class CObject {
public:
    char pad_00[8];
    unsigned short typeIndex;
    unsigned char animationOffset;
    unsigned char pad_0b;

    void FindTrigger(int* resultX, int* resultY);
};
SIZE(CObject, 0xc);

class CObjectType {
public:
    char pad_00[0x10];
    signed char width;
    signed char height;
    char pad_12[2];
    std::bitset<48> drawCells;
    char pad_1c[8];
    std::bitset<48> shadowCells;
    char pad_2c[0xc];
    TAdventureObjectType objectType;
    char pad_3c[4];
    unsigned char suppressDraw;
    char pad_41[3];
};
SIZE(CObjectType, 0x44);

struct AdvFullMapObjectsView {
    char pad_00[4];
    CObjectType* objectTypes;
    char pad_08[0xc];
    CObject* objects;
    char pad_18[0xc];
    CSprite** sprites;
};

#endif /* HOMM3_ADVMGR_OBJECTS_H */
