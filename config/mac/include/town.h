// Mac declaration view for source-owned TOWN reward bodies.
// The KB view already carries the canonical dialog vector, string, text,
// creature-name, and town declarations used by these bodies.
#ifndef HOMM3_MAC_TOWN_H
#define HOMM3_MAC_TOWN_H

#include "kb.h"


// Mac readTownData places its fixed-spell bitset at TownExtra+0x70. The
// initializeSpells scan passes that address to bitset<70>::test.
class TownExtra {
    char m_beforeFixedSpells[0x70];
public:
    std::bitset<70> m_fixedSpells;
};

extern const signed char g_mageGuildBaseSpellCounts[5];
extern unsigned long long g_bitNumber[];
extern __int64 g_hierarchyMask[9][44];
extern int g_mapWidth;
extern int g_mapHeight;
void checkEndGame(int forceWin);
int random(int minimum, int maximum);
extern "C" void* memset(void* destination, int value, unsigned long count);

extern playerData* g_currentPlayer;
extern int g_netLocalGamePos;

enum {
    GENERAL_TEXT_LIST_AND = 142,
    GENERAL_TEXT_EVENT_CREATURES = 588
};

void extendedDialog(const char* text,
                    std::vector<type_dialog_resource>& resources,
                    long x, long y, long timeout);

#endif
