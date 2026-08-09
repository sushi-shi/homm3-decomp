#ifndef HOMM3_ADVMGR_OBJECTS_H
#define HOMM3_ADVMGR_OBJECTS_H

#include <bitset>
#include <vector>
#include "mapcell.h"

class CSprite;
class textWidget;

enum ECompleteDrawFps {
    COMPLETE_DRAW_FPS_FRAME_COUNT = 100
};

// Narrow view of the retail chat singleton used by CompleteDraw's optional
// FPS overlay. Both method targets and their calling conventions are proved
// by the retail call sites; the rest of CChatManager stays unreconstructed.
class CChatManager {
public:
    void ClearChat();
    void UpdateWidget(textWidget* widget, unsigned char killOld, int numLines);
};

DATA(0x0069d7b0) extern CChatManager chatMan;
void __cdecl UpdateCompleteDrawFps(CChatManager* manager, const char* text);

DATA(0x0065f690) extern int gCompleteDrawFpsFrame;
DATA(0x00691240) extern unsigned long gCompleteDrawFpsLastTime;
DATA(0x0069136c) extern int
    gCompleteDrawFpsTimes[COMPLETE_DRAW_FPS_FRAME_COUNT];
DATA(0x006912ec) extern char gCompleteDrawFpsText[];
DATA(0x00660388) extern char gCompleteDrawFpsFormat[];

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

struct AdvGroundCellView {
    char pad_00[0xc];
    unsigned char cellFlags;
};

struct AdvRoadCellView {
    char pad_00[0xc];
    unsigned short cellFlags;
};

// Retail mine records are 0x40 bytes. GetSoundId reaches the type and the
// one-byte abandoned marker without widening game.h's broad mine layout.
struct AdvMineSoundView {
    signed char owner;
    signed char type;
    unsigned char abandoned;
};

enum EGetSoundObjectIndex {
    GET_SOUND_BANK_0 = 0,
    GET_SOUND_BANK_1,
    GET_SOUND_BANK_2,
    GET_SOUND_BANK_3,
    GET_SOUND_BANK_4,
    GET_SOUND_BANK_5,
    GET_SOUND_BANK_6,
    GET_SOUND_GARRISON_0 = 0,
    GET_SOUND_GARRISON_1,
    GET_SOUND_GENERATOR4_0 = 0,
    GET_SOUND_GENERATOR4_1,
    GET_SOUND_MINE_0 = 0,
    GET_SOUND_MINE_1,
    GET_SOUND_MINE_2,
    GET_SOUND_MINE_3,
    GET_SOUND_MINE_4,
    GET_SOUND_MINE_5,
    GET_SOUND_MINE_6
};

// Only the creature ids selected by GetSoundId's retail switch are named.
// Ordinal spellings avoid importing an external semantic creature roster.
enum EGetSoundCreatureId {
    GET_SOUND_CREATURE_000 = 0,
    GET_SOUND_CREATURE_002 = 2,
    GET_SOUND_CREATURE_004 = 4,
    GET_SOUND_CREATURE_008 = 8,
    GET_SOUND_CREATURE_010 = 10,
    GET_SOUND_CREATURE_012 = 12,
    GET_SOUND_CREATURE_014 = 14,
    GET_SOUND_CREATURE_016 = 16,
    GET_SOUND_CREATURE_018 = 18,
    GET_SOUND_CREATURE_020 = 20,
    GET_SOUND_CREATURE_022 = 22,
    GET_SOUND_CREATURE_024 = 24,
    GET_SOUND_CREATURE_026 = 26,
    GET_SOUND_CREATURE_028 = 28,
    GET_SOUND_CREATURE_030 = 30,
    GET_SOUND_CREATURE_034 = 34,
    GET_SOUND_CREATURE_036 = 36,
    GET_SOUND_CREATURE_038 = 38,
    GET_SOUND_CREATURE_040 = 40,
    GET_SOUND_CREATURE_042 = 42,
    GET_SOUND_CREATURE_044 = 44,
    GET_SOUND_CREATURE_046 = 46,
    GET_SOUND_CREATURE_048 = 48,
    GET_SOUND_CREATURE_050 = 50,
    GET_SOUND_CREATURE_052 = 52,
    GET_SOUND_CREATURE_054 = 54,
    GET_SOUND_CREATURE_056 = 56,
    GET_SOUND_CREATURE_058 = 58,
    GET_SOUND_CREATURE_060 = 60,
    GET_SOUND_CREATURE_062 = 62,
    GET_SOUND_CREATURE_064 = 64,
    GET_SOUND_CREATURE_066 = 66,
    GET_SOUND_CREATURE_068 = 68,
    GET_SOUND_CREATURE_070 = 70,
    GET_SOUND_CREATURE_072 = 72,
    GET_SOUND_CREATURE_074 = 74,
    GET_SOUND_CREATURE_076 = 76,
    GET_SOUND_CREATURE_078 = 78,
    GET_SOUND_CREATURE_080 = 80,
    GET_SOUND_CREATURE_082 = 82,
    GET_SOUND_CREATURE_084 = 84,
    GET_SOUND_CREATURE_086 = 86,
    GET_SOUND_CREATURE_088 = 88,
    GET_SOUND_CREATURE_090 = 90,
    GET_SOUND_CREATURE_092 = 92,
    GET_SOUND_CREATURE_094 = 94,
    GET_SOUND_CREATURE_096 = 96,
    GET_SOUND_CREATURE_098 = 98,
    GET_SOUND_CREATURE_100 = 100,
    GET_SOUND_CREATURE_102 = 102,
    GET_SOUND_CREATURE_104 = 104,
    GET_SOUND_CREATURE_106 = 106,
    GET_SOUND_CREATURE_108 = 108,
    GET_SOUND_CREATURE_110 = 110,
    GET_SOUND_CREATURE_112 = 112,
    GET_SOUND_CREATURE_113 = 113,
    GET_SOUND_CREATURE_114 = 114,
    GET_SOUND_CREATURE_115 = 115
};

DATA(0x0063d570) extern int gCreatureGenerator1Types[];

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
