// prefs.h - the preferences block misc.obj reads from and writes to the
// registry (retail .bss 0x698758).
// HAND-OWNED after admission.
//
// This is a SEPARATE header rather than a section of misc.h on purpose.
// misc.h is included by soundmgr.cpp, game.cpp, kbwin.cpp and
// ai_combat.cpp as well as misc.cpp; a struct DEFINITION added there
// would enter four more TUs' include closures, and the include-set
// sensitivity class (initialize_game_data precedent, re-measured
// 2026-08-08) makes that a real codegen risk for bodies that are
// already exact. Keeping the definition here, included only by the one
// TU that owns the block, gives the type its home in include/ with the
// same closure misc.obj had when its bodies were matched.
#ifndef HOMM3_PREFS_H
#define HOMM3_PREFS_H

#include <va.h>

// Extent is byte-proven by ReadPrefs' own `mov ecx,0x35 / xor eax,eax /
// rep stosd` at 0x698758: 53 dwords = 212 bytes, so
// 0x698758..0x69882b inclusive is ONE object, and every interior
// address misc.obj touches lands inside it - which is what makes them
// MEMBERS and not neighbours.
//
// The FIELD NAMES below are not guesses: WritePrefsToRegistry hands
// each one to RegSetValueExA next to the registry value name it is
// stored under, so the name/offset pairing is read straight off the
// image. The slicing closes EXACTLY - the last field ends at +0xd4 =
// 212 - which is the arithmetic check that no field was invented:
//   ...+0x70 the dword run, +0x74..+0x8f untouched by this TU,
//   +0x90 name[4] ("Unique System ID", written REG_SZ with cbData 4),
//   +0x94 combatSpeed, +0x98/+0xa5/+0xb2 the three 13-byte RMT keys,
//   +0xbf networkDefaultName[21] ("Network Default Name", cbData 0x15).
// The earlier reading of name as char[8] was wrong: 'Combat Speed'
// sits at +0x94, inside that span, and pins name at four bytes - which
// is also what makes the 13-byte RMT buffers exactly big enough for
// "RMT" + a 3-char name + "RC.BIN" + NUL.
// +0x74..+0x8f keeps a pad: nothing in this TU proves a field there.
struct SUnnamed698758 {
    int computerWalkSpeed;        // +0x00  "Computer Walk Speed"
    int walkSpeed;                // +0x04  "Walk Speed"
    int musicVolume;              // +0x08  "Music Volume"
    int soundVolume;              // +0x0c  "Sound Volume"
    int lastMusicVolume;          // +0x10  "Last Music Volume"
    int lastSoundVolume;          // +0x14  "Last Sound Volume"
    int autosave;                 // +0x18  "Autosave"
    int showRoute;                // +0x1c  "Show Route"
    int moveReminder;             // +0x20  "Move Reminder"
    int quickCombat;              // +0x24  "Quick Combat"
    int videoSubtitles;           // +0x28  "Video Subtitles"
    int townOutlines;             // +0x2c  "Town Outlines"
    int animateSpellBook;         // +0x30  "Animate SpellBook"
    int windowScrollSpeed;        // +0x34  "Window Scroll Speed"
    int blackoutComputer;         // +0x38  "Blackout Computer"
    int combatAutoCreatures;      // +0x3c  "Combat Auto Creatures"
    int combatAutoSpells;         // +0x40  "Combat Auto Spells"
    int combatCatapult;           // +0x44  "Combat Catapult"
    int combatBallista;           // +0x48  "Combat Ballista"
    int combatFirstAidTent;       // +0x4c  "Combat First Aid Tent"
    int binkVideo;                // +0x50  "Bink Video"
    int mainGameShowMenu;         // +0x54  "Main Game Show Menu"
    int mainGameX;                // +0x58  "Main Game X"
    int mainGameY;                // +0x5c  "Main Game Y"
    int mainGameFullScreen;       // +0x60  "Main Game Full Screen"
    int showCombatGrid;           // +0x64  "Show Combat Grid"
    int showCombatMouseHex;       // +0x68  "Show Combat Mouse Hex"
    int combatShadeLevel;         // +0x6c  "Combat Shade Level"
    int combatArmyInfoLevel;      // +0x70  "Combat Army Info Level"
    unsigned char pad74[0x1b];    // +0x74  otherwise untouched by misc.obj
    unsigned char unnamed8f;      // +0x8f  boolean checked/defaulted here
    char name[4];                 // +0x90  "Unique System ID"
    int combatSpeed;              // +0x94  "Combat Speed"
    char rcFile[13];              // +0x98  "RMT%sRC.BIN" destination
    char rdFile[13];              // +0xa5  "RMT%sRD.BIN" destination
    char scFile[13];              // +0xb2  "RMT%sSC.BIN" destination
    char networkDefaultName[21];  // +0xbf  "Network Default Name"
};
SIZE(SUnnamed698758, 212);

// Definition + DATA claim in src/misc.cpp.
extern SUnnamed698758 gUnnamed698758;

// Four dwords at 0x699524..0x699530, OUTSIDE the prefs block (it ends
// at 0x69882b), so they are separate globals and not members. Their names are
// published by Dreamcast CodeView and their roles/addresses are independently
// confirmed by the retail preference I/O and oldmain benchmark block.
// Definitions + DATA claims in src/misc.cpp.
extern int gbFirstTimeThrough;  // 0x699524, "First Time"
extern int giTestDecomp;        // 0x699528, "Test Decomp"
extern int giTestRead;          // 0x69952c, "Test Read"
extern int giTestBlit;          // 0x699530, "Test Blit"

#endif  /* HOMM3_PREFS_H */
