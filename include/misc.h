#ifndef HOMM3_MISC_H
#define HOMM3_MISC_H

#include <string>
#include <vector>
#include "includes.h"

// Live prototypes (claimed misc.cpp bodies).
int safeRandom(int min, int max);   // 0x50b1d0
int random(int min, int max);       // 0x50b230
void sRand(int seed);              // 0x50c5f0
// Original SRandom, defined once in misc.cpp (DC source line 796).
int sRandom(int lower, int upper);
void checkConfigFile();             // 0x50b260
void setGameDefaults();             // 0x50b4d0
void setDefaultSystemOptions();
void setDefaultCombatOptions();     // 0x50b700
void readPrefsFromRegistry();
void writePrefsToRegistry();        // 0x50be10
void writePrefs();                  // 0x50c1b0
std::string formatString(const char* format, ...);  // 0x50c600
unsigned long getAvailableDiskSpace();            // 0x50c7a0

// SetupCDDrive result domain. Retail proves all six non-default arms in
// oldmain's inlined SetupCDRom and separately singles out 5/6 in the main
// menu. The original enumerator spellings did not survive, so keep the names
// deliberately numeric rather than inventing error semantics for them.
enum ECDDriveNumber {
    CD_DRIVE_NUMBER_1 = 1,
    CD_DRIVE_NUMBER_2 = 2,
    CD_DRIVE_NUMBER_3 = 3,
    CD_DRIVE_NUMBER_4 = 4,
    CD_DRIVE_NUMBER_5 = 5,
    CD_DRIVE_NUMBER_6 = 6
};

// Registry-path state owned by misc.cpp. AppPath's 351-byte extent is
// byte-proven by SetGameDefaults' _getcwd bound; registry reads cap both
// paths at 350 bytes, which also proves CDDrive's extent.
extern char g_regAppPath[351];       // .bss 0x6985c4
extern char g_regCdRomPath[350];     // .bss 0x698838
extern int g_showIntro;              // .bss 0x6993c0

// --- globals ---
long fileSize(char* filename);

// --- std ---

// DC misc.cpp SRandom; consumed by adventure spell probability checks.
int sRandom(int lower, int upper);

#endif  /* HOMM3_MISC_H */
