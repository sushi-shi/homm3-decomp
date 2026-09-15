// cursor.h - prototypes of cursor.cpp (compiland cursor.obj)
#ifndef HOMM3_CURSOR_H
#define HOMM3_CURSOR_H

class CMapChange;

// Before normalization (type): ECursorMoveTuning.
enum CursorMoveTuning {
    CURSOR_INSTANT_WALK_SPEED = 4,
    CURSOR_IRREGULAR_STEP_PIXELS = 10,
    CURSOR_IRREGULAR_MIDDLE_STEP_PIXELS = 12,
    CURSOR_TILE_PIXELS = 32
};

// --- globals ---
void sendMapChange(CMapChange* mapChange);  // 0x482390, dc 0x7c9f8

// --- advManager ---
// CODEVIEW(E:\gamedcs\cursor.cpp:52, dc 0x79a48) void advManager::StartCursor(int direction);
// CODEVIEW(E:\gamedcs\cursor.cpp:490, dc 0x7a7f4) void advManager::animate_move(hero* curr, int direction, int xInc, int yInc);
// CODEVIEW(E:\gamedcs\cursor.cpp:570, dc 0x7aa54) NewmapCell* advManager::MoveHero(int direction, unsigned char standEnd, type_point* trigger_point, int* bNoMove, unsigned char bComputerMove, int* bFoughtBattle, unsigned char bIsRemoteMove);
// CODEVIEW(E:\gamedcs\cursor.cpp:961, dc 0x7bbbc) void advManager::CheckAdjacentMon(int* bFoughtBattle);
// CODEVIEW(E:\gamedcs\cursor.cpp:1158, dc 0x7c2e0) void advManager::OnTeleportHero(CMapChange* pMapChange);
// CODEVIEW(E:\gamedcs\cursor.cpp:1167, dc 0x7c328) void advManager::OnClaimMine(CMapChange* pMapChange);
// CODEVIEW(E:\gamedcs\cursor.cpp:1177, dc 0x7c390) void advManager::OnClaimTown(CMapChange* pMapChange);
// CODEVIEW(E:\gamedcs\cursor.cpp:1186, dc 0x7c3c8) void advManager::OnBuildBoat(CMapChange* pMapChange);
// CODEVIEW(E:\gamedcs\cursor.cpp:1196, dc 0x7c420) void advManager::OnEraseObject(CMapChange* pMapChange);
// CODEVIEW(E:\gamedcs\cursor.cpp:1208, dc 0x7c470) void advManager::OnDeadHero(CMapChange* pMapChange);
// CODEVIEW(E:\gamedcs\cursor.cpp:1224, dc 0x7c51c) void advManager::OnRecruitHero(CMapChange* pMapChange);
// CODEVIEW(E:\gamedcs\cursor.cpp:1245, dc 0x7c584) void advManager::OnDeadPlayer(CMapChange* pMapChange);
// CODEVIEW(E:\gamedcs\cursor.cpp:1253, dc 0x7c5e0) void advManager::OnClaimGenerator(CMapChange* pMapChange);
// CODEVIEW(E:\gamedcs\cursor.cpp:1260, dc 0x7c648) void advManager::OnClaimGarrison(CMapChange* pMapChange);
// CODEVIEW(E:\gamedcs\cursor.cpp:1267, dc 0x7c664) void advManager::OnClaimShipYard(CMapChange* pMapChange);

#endif  /* HOMM3_CURSOR_H */
