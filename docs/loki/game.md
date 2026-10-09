# Loki Linux game 1.3.1a as evidence for the Windows game

`heroes3.dynamic` from Loki's 1.3.1a update (GCC 2.95.2, ELF i386, stripped;
provenance in `config/retail/heroes3-loki/image.toml`) is RoE-era Loki source
with Windows bugfixes. It is evidence for `HEROES3.EXE` (Complete, VC6 SP3),
never a target of `homm3 build`. Era differences are expected and are named as
such: Armageddon's Blade's random map generator and SoD's quest classes
(`type_quest` and its subclasses), combination artifacts and the Complete
loaders have no Loki counterpart.

```sh
homm3 loki toolchain --debs DIR --sgi-stl DIR --binutils DIR --gcc DIR --gtk DIR
homm3 loki-game init --exe PATH      # stage and verify heroes3.dynamic
homm3 loki-game census               # headless Ghidra: functions, calls, data refs
homm3 loki-game profile              # compiler-flag proof (config/loki/game.toml)
homm3 loki-game vtables              # RTTI -> vtables.tsv
homm3 loki-game pair                 # functions.tsv: Loki <-> Windows pairs
homm3 loki-game calls 0x2e0b0        # callees of a paired function, both sides
```

## Compiler profile

`-O2 -mcpu=pentium -funroll-loops -fno-exceptions`, RTTI on, frame pointers
kept, GCC 2.95.2 release `cc1plus` with libstdc++ 2.95's own headers (the
h3maped editor put SGI STL 3.2 first; the game's `std::string` is
libstdc++'s reference-counted bastring, `lock xadd` on the count).

- The game's project objects have no `.eh_frame` FDEs: all 11 CIEs describe
  the C++ runtime from 0x0820ce00 on. Type names and `__tf` functions exist.
- `homm3 loki-game profile` compiles the h3maped branch's engine units (exact
  for h3maped at `-O0`) under each variant and counts bodies found exactly in
  the game image with only link-decided fields open (config/loki/game.toml).
  On the 15 units that compile against libstdc++'s headers: 141 for the
  profile, 117 with SGI STL first; 116 with exceptions, 114 with
  `-funroll-all-loops`, 107 without unrolling, 80 at `-O3`, 30 at `-O1`,
  64 with `-mcpu=pentiumpro`, 71 with `i486`, 0 with the i386 default
  (`leave` epilogues). With SGI STL first (all 22 units) `-ffast-math` loses
  (192 against 199). `-fno-strength-reduce` and `-march=pentium` give the
  same count and stay undecided.

## Pairing

The image has no project symbols, so pairs come from retail evidence
(`homm3.loki_game.pair`; the first evidence that pairs a function wins):

| Evidence | Pairs | Meaning |
| :-- | --: | :-- |
| `fingerprint` | 56 | an engine unit compiled at the profile hits one body exactly |
| `string` | 207 | the only users of a shared C string on both sides |
| `vtable-signature` | 45 | the same set of classes holds it in its vtable |
| `vtable-slot` | 118 | equal-length slot runs between paired slots of a class |
| `call-graph` | 328 | equal-length runs between paired callees of a paired caller |
| `call-graph-intersection` | 43 | the only unpaired callee all paired callers share |
| `call-graph-alignment` | 168 | aligned in two or more paired callers' call sequences |

965 pairs, all ledger rows (59 of the 281 non-exact Windows functions). Call-graph
evidence pairs project code only: Loki's linkonce and runtime bands hold
libstdc++ templates with no Dinkumware counterpart, and bodies identical up
to addresses (file-static copies) stay unpaired.
Holdout check: without the fingerprint seeds the other evidence recovers 17
of the 56 fingerprint pairs and contradicts one (an alignment pair that chose
the wrong one of two near-identical Loki copies of `RGBToHSV`). A Windows vtable holds the
scalar deleting destructor; GCC's single destructor slot is paired with the
class's `??1` instead.

GCC 2.95 at `-O2` inlines only functions declared `inline` (and in-class
members); VC6 `/Ob2` also expands non-inline helpers. A Loki call to a
function whose Windows counterpart the Windows caller expands therefore
proves a helper boundary, and a body in the kept linkonce band (after
libgcc's `__umoddi3`) proves an inline or template definition.

## Compiling game units (not yet working)

`homm3 loki-game compile UNIT [--scan]` compiles a game unit at the profile:
`include/gcc_prefix.h` spells the Microsoft keywords, platform.h's
`HOMM3_TARGET_LOKI` branch imports the Windows SDK declarations (VC6's
include tree as `homm3 mac sdk` stages it, exposed in lower case after the
Linux headers), and va.h's branch keeps the claim macros empty. No game unit
compiles yet; the first blockers (kb.cpp, army.cpp) are:

- the SDK's `_VARIANT_BOOL` member (objidl.h, oaidl.h), which wtypes.h spells
  `/##/` for every compiler but MSVC;
- anonymous structs inside unions (mapcell.h's cell flags), which g++ 2.95
  does not merge into the enclosing scope;
- Dinkumware spellings (armygrp.h forward-declares `char_traits`);
- try/catch in headers (game.h's SavedGameHeader::load) under
  `-fno-exceptions`;
- RAD's Bink/Smacker headers' MSVC inline assembly; Loki played video
  through smpeg, so those units differ by port anyway.

Matching Loki bodies under GCC therefore starts with a Loki port layer on
this branch only.
