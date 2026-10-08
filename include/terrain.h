// terrain.h - E:\gamedcs\terrain.h

// MODELLED FROM RETAIL BYTES + DC CODEVIEW (2026-08-08). Only the part
// of retail's terrain.h that is byte-proven is here: the ten file-scope
// terrain masks at its lines 70-79. Everything else the real header
// declares (a terrain roster, whatever else lives at lines 1-69) is NOT
// recoverable from the evidence and is deliberately absent.

// WHY THIS HEADER EXISTS AT ALL. The DC CodeView corpus attributes
// twenty static-initializer funclets in each of 78 game compilands to
// `E:\gamedcs\terrain.h` lines 70-79 - ten file-scope objects with
// dynamic initializers, one pair of funclets each, duplicated per TU
// (NB11 procedure/line records). Retail carries exactly that: an including
// compiland opens with ten near-identical ~95 B funclets, compiled where the
// header is included and so ahead of the TU's first function, e.g.
// initialize at 0xeb360..0xeb72f and drawing at 0x491f20..0x4922ef.

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

// WHAT THEY ARE CALLED, AND WHICH K IS WHICH (proven, not assumed).
// The DC corpus names ten per-module statics of type 0x2D65 =
// `std::bitset<10,unsigned long>`: k{Dirt,Sand,Grass,Snow,Swamp,Rough,
// Subterranean,Lava,Water,Rock}Mask (NB11 global records).
// Their DC .data emission order is identical in every
// module that carries them (ai.obj 0x20260, ai_combat.obj 0x20288,
// adventuremapwindow.obj 0x1fff4, remote.obj):
//     Lava, Rock, Dirt, Snow, Rough, Swamp, Subterranean, Water,
//     Grass, Sand
// The copy stored by the run at 0xeb360 occupies ten consecutive .bss dwords
// 0x6991e8..0x69920c, and the funclets' store addresses give the K of
// each slot: 7, 9, 0, 3, 5, 4, 6, 8, 2, 1. Laying that against the DC
// order gives Lava=7, Rock=9, Dirt=0, Snow=3, Rough=5, Swamp=4,
// Subterranean=6, Water=8, Grass=2, Sand=1 - every one of them equal to
// its own TTerrainType enumerator (eTerrainDirt=0 ... eTerrainRock=9,
// kNumTerrainTypes=10; dump.txt:59956-59974). The run at 0xebd10 repeats
// the identical permutation at 0x699210..0x699238 (with one unrelated
// dword at 0x699224 interleaved). A chance agreement of two independent
// ten-element orderings is 1 in 10!.
// Hence: mask K belongs to terrain K, and the declaration order at
// lines 70-79 is the TTerrainType order.

// NOT const: the DC dump types every one of them as the bare class
// 0x2D65, and it does emit LF_MODIFIER `const` on data it has (e.g.
// kMaxRunLength/kOpaqueRunCode in the sprite modules), so the absence
// is evidence. They are plain file-scope statics.

// These per-TU statics have no single DATA address. The source-initializer
// verifier pairs each emitted CRT body with retail, including its named
// _Xran call and destination store. It accounts for the individual copies
// without assigning an unproven retail TU or adding independent VA claims.

// WHICH TUs GET THIS HEADER - decided by retail bytes. Scanning
// config/retail/functions.tsv for the size run [89, 96, 97, 95 x 7] finds
// it in 89 places, almost always directly after a 32-byte row, the
// ctype<wchar_t>::id guard. VC6 compiles a header's dynamic initializers
// at the include point and defers template static-member initializers to
// the end of the TU, and the object's code order is the link order: our
// initialize.obj opens with these ten funclets and ends with its 32-byte
// guard. So a run belongs to the TU it PRECEDES, after the previous TU's
// guard. Attributed that way, 58 units begin with the run at their first
// retail function; 51 of them carry the terrain.h initializers in the DC
// corpus too, while 6 of the 7 units the preceding-TU reading had
// selected (border, iconwdgt, mousemgr, resource, sample, smackmgr) have
// none there. Those six still include this header: removing it loses the
// bitset<10>::_Xran COMDAT border's claim pairs and sample's retail
// destructor shape, which another standard header must supply first.
// Runs that sit inside one of our units' retail ranges (advmgr, game,
// town, kb, mapcell, ...) mark TU boundaries this tree does not draw yet.

// MEASURED EFFECT (`homm3 build --fast`, delink pairings refreshed): the
// first function of an including TU is no longer the first one C2
// compiles, so it receives the phase flag as 1 (docs/vc6/phase-flag.md):
// showCreatureSpellError 99.4556 -> 100. army doAttack 99.9232 ->
// 99.9616; overview setupDynamicStuff 92.4357 -> 92.2165 (its residue is
// elsewhere; retail's run precedes it too). Every other function of the
// widened units is unchanged.

#ifndef HOMM3_TERRAIN_H
#define HOMM3_TERRAIN_H

#include "va.h"

#include <bitset>

// E:\gamedcs\terrain.h:70-79
static std::bitset<10> g_dirtMask = std::bitset<10>(1) << 0;          // eTerrainDirt
static std::bitset<10> g_sandMask = std::bitset<10>(1) << 1;          // eTerrainSand
static std::bitset<10> g_grassMask = std::bitset<10>(1) << 2;         // eTerrainGrass
static std::bitset<10> g_snowMask = std::bitset<10>(1) << 3;          // eTerrainSnow
static std::bitset<10> g_swampMask = std::bitset<10>(1) << 4;         // eTerrainSwamp
static std::bitset<10> g_roughMask = std::bitset<10>(1) << 5;         // eTerrainRough
static std::bitset<10> g_subterraneanMask = std::bitset<10>(1) << 6;  // eTerrainSubterranean
static std::bitset<10> g_lavaMask = std::bitset<10>(1) << 7;          // eTerrainLava
static std::bitset<10> g_waterMask = std::bitset<10>(1) << 8;         // eTerrainWater
static std::bitset<10> g_rockMask = std::bitset<10>(1) << 9;          // eTerrainRock

#endif  // HOMM3_TERRAIN_H
