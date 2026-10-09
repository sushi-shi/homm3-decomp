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
homm3 loki-game compile kb --scan    # one unit at the profile, bodies found in the image
homm3 loki-game diff 0x2e0b0         # compiled body beside its Loki body
homm3 loki-game score --readme       # exact / compiled / paired, README block
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
| `reviewed` | 4 | settled against the compiled body (config/loki/game.toml) |
| `fingerprint` | 56 | an engine unit compiled at the profile hits one body exactly |
| `string` | 207 | the only users of a shared C string on both sides |
| `vtable-signature` | 45 | the same set of classes holds it in its vtable |
| `vtable-slot` | 118 | equal-length slot runs between paired slots of a class |
| `call-graph` | 329 | equal-length runs between paired callees of a paired caller |
| `call-graph-intersection` | 44 | the only unpaired callee all paired callers share |
| `call-graph-alignment` | 169 | aligned in two or more paired callers' call sequences |

972 pairs, all ledger rows (58 of the non-exact Windows functions). Call-graph
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

## Compiling game units

`homm3 loki-game compile UNIT [--scan]` compiles a game unit at the profile
the way Loki's port must have: no Windows SDK is parsed.

- `include/loki/` (work/loki-game only) holds the Win32, DirectDraw,
  DirectSound, multimedia, shell and socket declarations the shared source
  names, opaque RAD (Bink, Smacker), Miles and Immersion handles, the
  Microsoft CRT headers (`direct.h`, `io.h`) and the library headers
  libstdc++ 2.95.2 lacks or spells differently (`<limits>`, `<sstream>`,
  `<streambuf>`, Dinkumware's `<strstream>` bringing in `<string>`).
  platform.h's `HOMM3_TARGET_LOKI` branch includes them; the VC6 AST and
  the cleanliness board skip the directory.
- `include/gcc_prefix.h` spells the Microsoft keywords and CRT names, and
  the exception keywords: the game has no project `.eh_frame`, and g++
  rejects `try`/`throw` under `-fno-exceptions`, so guarded blocks run
  unguarded, handlers are dead and throw expressions are discarded (the
  library's exception specifications are read first).
- `#if 0` regions are blanked before g++ reads a unit: their MSVC special
  names (`` `vbase destructor' ``) hold a lone quote that g++ 2.95's
  preprocessor rejects even while skipping.
- Header forms g++ 2.95 rejects keep a `HOMM3_TARGET_LOKI` arm on this
  branch only: mapcell.h's anonymous structs (the Dreamcast flag list,
  one declarator per bit, with read-only views of the word), the `i64`
  bit table, iterator typedefs for the two bitset iterators, a forward
  `enum TTownType`, TAutoArrayPtr's `throw()` and the forward Dinkumware
  `std::string` declarations. The headers name the string `std::string`
  for both compilers (VC6 bytes unchanged).

101 of the 117 units with a paired function compile; the rest are platform
units Loki replaced (wingraph, mousemgr, inputmgr, kbwin, misc, dxplay,
forcefeedback, smackmgr, soundmgr) and viewwrld (a using-declaration g++
2.95 rejects).

## Loki game score

`homm3 loki-game score [UNIT ...] [--below-100] [--all] [--readme]` and
`homm3 loki-game diff WIN_RVA` compare compiled bodies with their Loki
pairs (`homm3.loki_game.diff`): relocated fields and image addresses become
one sentinel, outgoing calls `call .+5`, and each scored function is
assembled in its own section at its image address modulo 16 (gas resolves
`.p2align 4,,7` from the section start). The census counts a body's
addresses, so a Loki body runs to the next function less gas's fill.
Unpaired image bodies are named by object functions found in the image.

At the merge of 54e3a8037: **130 of 972 pairs exact, 894 compiled**. Of the
58 paired functions below 100% on Windows none is exact; their differences
are era facts (layouts, vtables, record strides, counts, flag values; see
docs/todos/loki-evidence-pending.md). Reviewed pairs (call-graph pairs that
had taken a neighbour) are in config/loki/game.toml `[[pairs.reviewed]]`.

## Carry-back

- doCombat's Loki/Dreamcast stats-array form landed on decomp-complete-4.0
  (54e3a8037, 97.31 -> 97.82) once the helper audit made it neutral.
- clearOverviewWidget and clearCreatureSelection have no out-of-line Loki
  body; the helper audit had already folded both.
- applyHeaderToGame is expanded in Loki but kept by Mac (0x17b0b0): not
  carried.

## State (end of the second stint)

Next: an RoE layer would be needed to make the walls' Loki bodies exact
(class layouts, traits records, counts); the cheaper lead is to sweep the
894 compiled pairs for helper boundaries and statement shapes where RoE and
Complete agree, and to repair weak call-graph pairs with `homm3 loki-game
diff` against neighbouring bodies (nextArmy has none yet).
