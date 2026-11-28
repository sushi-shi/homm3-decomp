#ifndef HOMM3_MISC_H
#define HOMM3_MISC_H

#include "includes.h"

#include <string>
#include <vector>

// Live prototypes (claimed misc.cpp bodies).
int safeRandom(int min, int max);   // 0x50b1d0
int random(int min, int max);       // 0x50b230
void sRand(int seed);              // 0x50c5f0
// Original SRandom, defined once in misc.cpp (DC source line 796).
int sRandom(int lower, int upper);
void checkConfigFile();             // 0x50b260
void setGameDefaults();             // 0x50b4d0
void setDefaultCombatOptions();     // 0x50b700
void writePrefsToRegistry();        // 0x50be10
void writePrefs();                  // 0x50c1b0
std::string formatString(const char* format, ...);  // 0x50c600
unsigned long getAvailableDiskSpace();            // 0x50c7a0

// SetupCDDrive's result, as the startup code keeps it. No Heroes III symbol
// names the domain; Heroes II's debug symbols call the same startup global
// iCDRomErr, and the values 1..4 keep their Heroes II meaning. The retained
// retail body behind the patched entry (0x50c1c6..0x50c599) returns
//   0 with the disc found,
//   2 when two rounds of MCI "open %c: type cdaudio" probes find no disc,
//   3 when _chdir(AppPath) fails,
//   4 when DATA\H3BITMAP.LOD cannot be opened from that directory;
// it never returns 1, which SetupCDRom still handles (the CD-ROM
// unavailable notice). EarlySetup then turns 0 into 5 or 6 when the disc's
// video archive lacks the Shadow of Death movie: 6 when it holds the
// Armageddon's Blade movie, 5 otherwise, and the main menu names that
// edition in its wrong-disc notice. 7 is the pinned image's patched result.
enum ECDRomSetupResult {
    CD_ROM_READY = 0,
    CD_ROM_DRIVE_UNAVAILABLE = 1,
    CD_ROM_DISC_MISSING = 2,
    CD_ROM_GAME_DIRECTORY_MISSING = 3,
    CD_ROM_DATA_FILES_MISSING = 4,
    CD_ROM_RESTORATION_OF_ERATHIA_DISC = 5,
    CD_ROM_ARMAGEDDONS_BLADE_DISC = 6,
    CD_ROM_NOT_REQUIRED = 7
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
