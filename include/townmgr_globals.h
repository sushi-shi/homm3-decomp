#ifndef HOMM3_TOWNMGR_GLOBALS_H
#define HOMM3_TOWNMGR_GLOBALS_H

// Claimed by townmgr.obj and toggled by the adventure-map "nwczion"
// command; SetupExtraStuff is its reader. This narrow owner interface keeps
// consumers from importing townmgr.h's UI closure merely to name one byte.
extern unsigned char gBuildAllBuildings;  // retail 0x6aaa5c

// The thieves'-guild ranking pair is also consumed by game.cpp's special
// rumor builder. Keep this narrow interface from importing townmgr.h's UI
// closure into game.obj.
void GetCategoryStats(int whichCat, long* value, signed char* index);
void SortStats(long* value, signed char* index);

#endif  /* HOMM3_TOWNMGR_GLOBALS_H */
