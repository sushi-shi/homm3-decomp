// prefs.h - the preferences block misc.obj reads from and writes to the
// registry (retail .bss 0x698758).

// Original DC names: configStruct and gConfig (ReadPrefs, misc.cpp:526).
// Consumers include this header to access the one preferences object;
// its volume, display and multiplayer fields must not have separate storage.
#ifndef HOMM3_PREFS_H
#define HOMM3_PREFS_H

#include "va.h"

// Extent is byte-proven by ReadPrefs' own `mov ecx,0x35 / xor eax,eax /
// rep stosd` at 0x698758: 53 dwords = 212 bytes, so
// 0x698758..0x69882b inclusive is ONE object, and every interior
// address misc.obj touches lands inside it - which is what makes them
// MEMBERS and not neighbours.

struct configStruct {
    int m_computerWalkSpeed;        // +0x00  "Computer Walk Speed"
    int m_walkSpeed;                // +0x04  "Walk Speed"
    int m_musicVolume;              // +0x08  "Music Volume"
    int m_soundVolume;              // +0x0c  "Sound Volume"
    int m_lastMusicVolume;          // +0x10  "Last Music Volume"
    int m_lastSoundVolume;          // +0x14  "Last Sound Volume"
    int m_autosave;                 // +0x18  "Autosave"
    int m_showRoute;                // +0x1c  "Show Route"
    int m_moveReminder;             // +0x20  "Move Reminder"
    int m_quickCombat;              // +0x24  "Quick Combat"
    int m_videoSubtitles;           // +0x28  "Video Subtitles"
    int m_townOutlines;             // +0x2c  "Town Outlines"
    int m_animateSpellBook;         // +0x30  "Animate SpellBook"
    int m_windowScrollSpeed;        // +0x34  "Window Scroll Speed"
    int m_blackoutComputer;         // +0x38  "Blackout Computer"
    int m_combatAutoCreatures;      // +0x3c  "Combat Auto Creatures"
    int m_combatAutoSpells;         // +0x40  "Combat Auto Spells"
    int m_combatCatapult;           // +0x44  "Combat Catapult"
    int m_combatBallista;           // +0x48  "Combat Ballista"
    int m_combatFirstAidTent;       // +0x4c  "Combat First Aid Tent"
    int m_binkVideo;                // +0x50  "Bink Video"
    int m_mainGameShowMenu;         // +0x54  "Main Game Show Menu"
    int m_mainGameX;                // +0x58  "Main Game X"
    int m_mainGameY;                // +0x5c  "Main Game Y"
    int m_mainGameFullScreen;       // +0x60  "Main Game Full Screen"
    int m_showCombatGrid;           // +0x64  "Show Combat Grid"
    int m_showCombatMouseHex;       // +0x68  "Show Combat Mouse Hex"
    int m_combatShadeLevel;         // +0x6c  "Combat Shade Level"
    int m_combatArmyInfoLevel;      // +0x70  "Combat Army Info Level"
    char m_dosDigitalDriver[13];    // +0x74
    char m_dosMidiDriver[13];       // +0x81
    char m_dontTryRedbook;          // +0x8e
    unsigned char m_firstInstall;      // +0x8f  boolean checked/defaulted here
    char m_name[4];                 // +0x90  "Unique System ID"
    int m_combatSpeed;              // +0x94  "Combat Speed"
    char m_rcFile[13];              // +0x98  "RMT%sRC.BIN" destination
    char m_rdFile[13];              // +0xa5  "RMT%sRD.BIN" destination
    char m_scFile[13];              // +0xb2  "RMT%sSC.BIN" destination
    char m_networkDefaultName[21];  // +0xbf  "Network Default Name"
};
SIZE(configStruct, 212);

// Definition + DATA claim in src/misc.cpp.
extern configStruct g_config;

// Four dwords at 0x699524..0x699530, OUTSIDE the prefs block (it ends
// at 0x69882b), so they are separate globals and not members. Their names are
// published by Dreamcast CodeView and their roles/addresses are independently
// confirmed by the retail preference I/O and oldmain benchmark block.
// Definitions + DATA claims in src/misc.cpp.
extern int g_firstTimeThrough;  // 0x699524, "First Time"
extern int g_testDecomp;        // 0x699528, "Test Decomp"
extern int g_testRead;          // 0x69952c, "Test Read"
extern int g_testBlit;          // 0x699530, "Test Blit"

#endif  /* HOMM3_PREFS_H */
