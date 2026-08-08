// terrain.h - E:\gamedcs\terrain.h
//
// MODELLED FROM RETAIL BYTES + DC CODEVIEW (2026-08-08). Only the part
// of retail's terrain.h that is byte-proven is here: the ten file-scope
// terrain masks at its lines 70-79. Everything else the real header
// declares (a terrain roster, whatever else lives at lines 1-69) is NOT
// recoverable from the evidence and is deliberately absent.
//
// WHY THIS HEADER EXISTS AT ALL. The DC CodeView corpus attributes
// twenty static-initializer funclets in each of 78 game compilands to
// `E:\gamedcs\terrain.h` lines 70-79 - ten file-scope objects with
// dynamic initializers, one pair of funclets each, duplicated per TU
// (`awk -F, 'tolower($6) ~ /terrain\.h/' evidence/dreamcast/
// functions.csv`). Retail carries exactly that: every game obj ends in
// a cinit tail of ten near-identical ~95 B funclets, e.g. iconwdgt at
// 0xeb360..0xeb72f and initialize at 0xebd10..0xec0de.
//
// WHAT THE OBJECTS ARE (byte-proven). Each funclet is one inlined
// `std::bitset<10>` construction, in the shape of the VC6 Dinkumware
// header this toolchain ships:
//   bitset(unsigned long _X)                         // BITSET:57
//     {_Tidy();
//      for (size_t _P = 0; _X != 0 && _P < _N; _X >>= 1, ++_P)
//         if (_X & 1) set(_P); }
// - `xor eax,eax; mov [ebp-4],eax`      = _Tidy() with _Nw == 0, so the
//   whole bitset is ONE dword: _N == 10 (`cmp esi,0xa` is the _P < _N
//   test, and the out-of-range arm calls the shared thiscall _Xran at
//   0x404410, `lea ecx,[ebp-4]` = the bitset's own `this`).
// - `mov ebx,1` is the ctor argument: the value constructed is 1.
// - the tail `and eax,MASK; shl eax,K` is operator<<= (BITSET:89):
//   `_A[0] <<= _P, _Trim()` with _Trim's `_A[0] &= 0x3ff`, reassociated
//   by VC6 into `(x & (0x3ff >> K)) << K`. K runs 0,1,2,...,9 down the
//   ten funclets in emission (= declaration) order, and MASK tracks it
//   0x3ff/0x1ff/0xff/0x7f/0x3f/0x1f/0xf/7/3/1. The K == 0 funclet has
//   neither instruction, exactly as `if ((_P %= _Nb) != 0)` predicts.
// So object number K is `std::bitset<10>(1) << K`.
//
// WHAT THEY ARE CALLED, AND WHICH K IS WHICH (proven, not assumed).
// The DC corpus names ten per-module statics of type 0x2D65 =
// `std::bitset<10,unsigned long>`: k{Dirt,Sand,Grass,Snow,Swamp,Rough,
// Subterranean,Lava,Water,Rock}Mask (`grep Mask evidence/dreamcast/
// globals.csv`). Their DC .data emission order is identical in every
// module that carries them (ai.obj 0x20260, ai_combat.obj 0x20288,
// adventuremapwindow.obj 0x1fff4, remote.obj):
//     Lava, Rock, Dirt, Snow, Rough, Swamp, Subterranean, Water,
//     Grass, Sand
// Retail's iconwdgt copy occupies ten consecutive .bss dwords
// 0x6991e8..0x69920c, and the funclets' store addresses give the K of
// each slot: 7, 9, 0, 3, 5, 4, 6, 8, 2, 1. Laying that against the DC
// order gives Lava=7, Rock=9, Dirt=0, Snow=3, Rough=5, Swamp=4,
// Subterranean=6, Water=8, Grass=2, Sand=1 - every one of them equal to
// its own TTerrainType enumerator (eTerrainDirt=0 ... eTerrainRock=9,
// kNumTerrainTypes=10; dump.txt:59956-59974). initialize.obj repeats
// the identical permutation at 0x699210..0x699238 (with one unrelated
// dword at 0x699224 interleaved). A chance agreement of two independent
// ten-element orderings is 1 in 10!.
// Hence: mask K belongs to terrain K, and the declaration order at
// lines 70-79 is the TTerrainType order.
//
// NOT const: the DC dump types every one of them as the bare class
// 0x2D65, and it does emit LF_MODIFIER `const` on data it has (e.g.
// kMaxRunLength/kOpaqueRunCode in the sprite modules), so the absence
// is evidence. They are plain file-scope statics.
//
// NO DATA() CLAIMS: these are per-TU statics, so each including TU owns
// a different ten-dword .bss run. There is no single address to claim,
// and the initializer funclets are the cinit excluded class - never
// claimed as functions either.
//
// The shift amounts below are written as literals on purpose: the
// TTerrainType roster still lives in armygrp.h as a bootstrap stub with
// only the sentinel, and moving it here is a separate, wider change.
//
// WHICH TUs GET THIS HEADER - decided by retail bytes, not by the DC
// file column. Scanning config/retail-functions.tsv for the size run
// [89, 96, 97, 95, 95, 95, 95, 95, 95, 95] finds the ten-funclet tail in
// 89 places in the image; 72 of those are immediately preceded by a
// 32-byte row, the ctype<wchar_t>::id guard. A TU whose tail is guard-
// only did NOT include terrain.h; a TU whose guard is followed by the
// ten did. That test disagrees with the DC corpus in both directions
// (retail iconwdgt has the tail and is not even in the DC terrain.h
// list; retail armygrp/misc/strip have a bare guard although the DC
// build attributes terrain.h to them), which is exactly what the
// evidence ranking says to expect - so the retail run is what this tree
// follows. Of the units in config/units.toml the run says YES for
// border, castle, exec, game, hexcell, iconwdgt, initialize, kbwin,
// mousemgr, path, recruit, resource, sample, smackmgr, soundmgr, town
// (all included as of 2026-08-08) plus ai, ai_combat, ai_tactical,
// ai_player, cmbtmgr and findpath (NOT touched by that lane - owned
// elsewhere at the time), and NO for advmgr, armygrp, basemgr, button,
// font, hero, inputmgr, misc, monframeinfo, strip, textntry, textwdgt,
// widget, window, winfile, winmgr. Do not add it to a NO unit to chase
// a score.
//
// MEASURED EFFECT of the sixteen additions (each one measured on its
// own, `homm3 build --fast`): initialize_game_data 94.0741 -> 100.0000;
// every other function in every other widened unit unchanged to seven
// digits. The header is carried in the other fifteen because retail's
// bytes say it belongs there, not because it moved a number.
#ifndef HOMM3_TERRAIN_H
#define HOMM3_TERRAIN_H

#include <va.h>

#include <bitset>

// E:\gamedcs\terrain.h:70-79
static std::bitset<10> kDirtMask = std::bitset<10>(1) << 0;          // eTerrainDirt
static std::bitset<10> kSandMask = std::bitset<10>(1) << 1;          // eTerrainSand
static std::bitset<10> kGrassMask = std::bitset<10>(1) << 2;         // eTerrainGrass
static std::bitset<10> kSnowMask = std::bitset<10>(1) << 3;          // eTerrainSnow
static std::bitset<10> kSwampMask = std::bitset<10>(1) << 4;         // eTerrainSwamp
static std::bitset<10> kRoughMask = std::bitset<10>(1) << 5;         // eTerrainRough
static std::bitset<10> kSubterraneanMask = std::bitset<10>(1) << 6;  // eTerrainSubterranean
static std::bitset<10> kLavaMask = std::bitset<10>(1) << 7;          // eTerrainLava
static std::bitset<10> kWaterMask = std::bitset<10>(1) << 8;         // eTerrainWater
static std::bitset<10> kRockMask = std::bitset<10>(1) << 9;          // eTerrainRock

#endif  // HOMM3_TERRAIN_H
