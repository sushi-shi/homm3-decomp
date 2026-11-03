# Alternate builds and compiler oracles

Research notes from 2026-10-07. They record which other HoMM3 executables
exist, which compilers built them, and what each one can and cannot prove
about the Windows source. Nothing here is a byte target. Retail Windows bytes
remain the verdict, as AGENTS.md requires.

## Inventory

| Build | Where | Compiler | Optimisation | Symbols |
| :---- | :---- | :------- | :----------- | :------ |
| GOG Complete 4.0, 2000-09-08 (target) | pinned | VC6 SP3: CL 8168, C1XX 8472, C2 8447 | `/O2 /Ob2 /Oy-` | none |
| Windows RoE demo, March 1999 | preserved `h3demo.exe` installer, SHA-256 `83cbb631…9c315` | Rich: 145 C++ objects stamped 8168 | lobby body omits frame pointer; exact TU options unknown | no PE debug directory |
| Buka «Полное собрание» 4.0, 2003-04-16 | archive.org `homm-antologiya-platinum-buka` (`autorun/launch/Setup3`) and `geroi-mexa-i-magii-novogodnee` (`Heroes3Setup`); identical `Heroes3.exe`, sha256 `2b777dcf…b0f0` | VC6 SP5: CL 8804, C1XX 8964, C2 8966, the same toolchain as the homm1/homm2 Buka targets | `/O2` with a frame pointer, like GOG | none |
| Loki Linux, RoE 1.2, 1999-11-15 | archive.org `heroes3-linux` (`heroes3_linux.bin`, MODE1/2352) | GCC 2.95.2 19991024; libraries egcs-2.91.66 | `-O2 -funroll-loops -fno-exceptions`, default `-mcpu=pentiumpro` | stripped; g++ RTTI kept |
| Classic Mac PEF | pinned | CodeWarrior Pro 6 | see `config/mac` | stripped |
| Dreamcast | pinned | SH4 | — | CodeView |

The same Buka discs also carry the homm1 and homm2 executables. They are
byte-identical to the `homm1-decomp-buka` and `homm2-buka` targets.

## Buka (VC6 SP5)

- The Rich header shows 145 C++ objects stamped C2 8966. The SP5 toolchain is
  at `~/Projects/homm1/homm1-decomp-buka/build/toolchains/vc6`. The STL and CRT
  headers match ours; only the DirectX and Platform SDK headers differ.
- With relocations masked, 69% of GOG's code bytes appear unchanged in Buka.
  Almost all of the rest comes from one localisation change:
  `TCreatureTypeTraits` gains a 4-byte field at or after +0x1c (116 → 120
  bytes). The change ripples into `army` (+4) and `combatManager` (+0xa8,
  which is 42 armies × 4).
- We compiled all 138 TUs with SP5 plus that one field. 558 functions
  (261 KB) then match Buka only, and just 138 (57 KB) still match GOG only.
- `game::getRandomMonster` (GOG MAX 99.02) is byte-exact against Buka under
  SP5. Its source is therefore right, and the GOG gap comes from SP3 or TU state.
- Stability: SP5 is no steadier than SP3 on this `/O2` code. Between each
  CUR≠MAX function's MAX commit and HEAD, functions whose source hash was
  unchanged moved bytes 41 times under SP3 and 44 under SP5 (of about 1,580).
  The steadiness of homm1/homm2 comes from `/Od`, not from the compiler build.

`CDPlayLobby::TestLobbied` gives a focused network control. Buka's body at
`0x004982f0` agrees with all 110 GOG bytes at `0x00498b70` after masking only
the two absolute IAT operands: `GlobalAlloc` (`0x63a0b8` → `0x63b0bc`) and
`GlobalLock` (`0x63a0b4` → `0x63b0b8`). Both import identities were checked;
the final `jge` and four returns are identical. The executable was re-extracted
from [the documented Platinum disc](https://archive.org/details/homm-antologiya-platinum-buka),
`autorun/launch/Setup3` → `Program_Executable_Files/Heroes3.exe`, and has SHA-256
`2b777dcf02706adf6aaa98ed02f906ebc2f0ef3dd6ab00d434257b6dce60b0f0`.

Compiling the direct-store `dxplay` model with the pinned HoMM1 SP5 passes, while keeping
the SP3 SDK and unit profile, reproduces all 86 SP3 comparison scores.
That model's `TestLobbied` section bytes and relocations are identical under both
compilers: both still emit `setge` and score 88.4210%. The baseline and SP5
objects were each reproduced independently. This isolates a source/context
distinction that changing from SP3 to SP5 alone does not repair; it does not
prove a unique original C++ spelling. See the conditional-select evidence in
[behavior-catalog.md D13](../vc6/behavior-catalog.md#d13-bool-materialization).
Subsequent per-path HRESULT recording restores the four returns under SP3
and reaches 96.4912%; only the final member-store/EDI-restore order differs.

## Windows RoE demo (1999)

The [preserved demo installer](https://github.com/GeorgeK1ng/heroes3demo/blob/main/h3demo.exe)
is 98,265,058 bytes, SHA-256
`83cbb6319386d6deba65273f53a60395fda8cce74cc24177b2ac3560ded9c315`,
matching the independently recorded [VCMI size/hash](https://github.com/vcmi/vcmi/issues/2126).
Its outer MSZip cabinet contains InstallShield `data1.hdr`/`data1.cab`;
`Program Files/h3demo.exe` extracts to 2,068,480 bytes, SHA-256
`fd9e66162d6632abdcfa686188bde15a1fd2a0fba8324da92a25be6cbaf754e3`.
Neither installer nor game was executed for this comparison.

`TestLobbied` is identified at `0x004810f0`, 102 bytes: the same two lobby
slot-`0x20` calls, buffer-size error guard, allocation/lock and status stores
as Complete. Its IAT operands resolve to `GlobalAlloc` at `0x5b40f4` and
`GlobalLock` at `0x5b40f0`. Two direct calls at `0x505e03` and `0x505eb9`
belong to the registry/bootstrap caller at `0x505d90`, independently anchored
by the DirectPlay application key and `RegOpenKeyExA`/`RegCloseKey` imports.

The demo omits the EBP frame but retains three guards and four returns.
At `0x481144` its tail is `test eax,eax; mov [esi+40h],eax; pop edi; jge`,
the same store-before-restore order required by Complete. Rich records
145 C++ objects at build 8168 and none at 8447. This is aggregate RTM-era
evidence, not proof of one TU's exact compiler options.

An isolated `/Oy` control of the adopted per-arm source emits the same lobby
section under verified SP3 and full RTM passes, with reproduction controls
passing. Both still put `pop edi` before the member store; query and fetch
argument staging differ from the demo as well. The earlier native body confirms
the operation and ordering without supplying a missing source statement or
justifying a change to Complete's compiler profile. Small evidence files and
the extracted executable are retained under ignored `build/network-exact/roe-demo/`;
the installer and temporary cabinets were removed after verification.

## Loki Linux (GCC 2.95.2)

- **Hierarchy:** the g++ type-info functions encode every polymorphic class
  and its bases, including access and virtual flags. 190 class/base pairs agree
  with our headers. The game uses only single public inheritance. Classes
  present only in RoE: `Bitmap8Bit`, `TGenericResource` (no Windows ctor
  callers), `CAdvancedOption` (a `widget`) and `CMPInputDlg1` (a
  `CHeroWindowEx`). In RoE, `type_necromancy_artifact` derives directly from
  `type_combat_artifact`; `type_base_necromancy_artifact`,
  `type_shooter_bonus_artifact` and `type_spell_artifact` do not exist yet.
- **Compiler:** the Debian potato i386 debs from `archive.debian.org` (gcc,
  g++ and cpp 2.95.2-13.1; binutils 2.9.5.0.37-1; libc6 and libc6-dev
  2.1.3-20; libstdc++2.10 and -dev) run on NixOS. Unpack each with `ar x` and
  `tar`, point the binaries at the potato `ld-2.1.3.so` with
  `patchelf --set-interpreter`, and pass `LD_LIBRARY_PATH` (an rpath trips the
  2000-era loader). Run the driver under `env -i` so the Nix `COMPILER_PATH`
  does not hijack cpp, and give `-B` a directory that holds only `as` and `ld`.
  Debian's build reports `2.95.2 20000220`, not Loki's `19991024 (release)`;
  the exact matches below suggest game codegen is unchanged.
- **Our source under GCC:** 35 of 139 TUs compile unchanged. The main blocker
  is code that uses Dinkumware STL internals; Loki used SGI STL. In those 35
  TUs, 227 of 473 functions are byte-exact against Loki with offsets masked.
- **Stability:** a neutral edit at the top of every TU changed 0 of 765
  functions. An unused class added to a shared header changed 2. GCC has no
  cross-function register allocation or auto-inlining, so it is far steadier
  than VC6.
- **Inlining oracle:** `-O2` inlines only functions declared `inline` or
  defined in the class body.
  - Loki calls these out of line, so they are ordinary helpers:
    `font::getCharacterWidth`, `TPalette16::convert24to16`,
    `CSprite::isValidSeq`, `iconWidget::setIconFrame`/`setIconSequence`,
    `CDiffFile::getData`.
  - Loki expands these, so they were declared `inline` (unless the port
    changed them): `adjustPaletteComponent`, `adjustPaletteHue`, `ftol`,
    `checkSpreadsheetResource`, `widget::initializeLinks`/`initializeHelpText`.
- **Verdict:** GCC is a secondary oracle, not a replacement target. Loki is
  RoE 1.2 with port edits, and its class layouts and STL differ from Complete.

The lobby path is not a Windows-body oracle. In all three checked Linux
executables, the caller of the unique lobby-launch log string reaches a
nine-byte false-return stub corresponding to `testIfLobbyLaunched`:

| Binary | Call site | Stub |
| :----- | :-------- | :--- |
| CD 1.2 `bin/x86/heroes3` | `0x812a233` | `0x81682cc` |
| 1.3.1a `heroes3` | `0x812e3e7` | `0x816d69c` |
| 1.3.1a `heroes3.dynamic` | `0x8133687` | `0x817293c` |

Each stub is `55 89 e5 b0 00 89 ec 5d c3`; none calls `TestLobbied`.
Their SHA-256s, in the same order, are
`6ac14c00be1704b9cb729b0eabc6100e3f566190e2c5ac5b5dd5f857450eed5c`,
`745a465d19d5f54fd80bba1a9dcdc482371ecabc92e3527724da4acfb5e5f30f`
and `6fd0db595138cedacd2bcf03e3cb324d3e9807958fd9a06c592a10119d475925`.
The stripped symbols do not independently locate the lobby member; this
caller-level result does not establish absence of every unreferenced stub.

## Mac evidence for the unstable (CUR≠MAX) functions

The committed ledger has 34 CUR≠MAX rows. Five have scored Mac pairs; two of
them are already exact on Mac (`updateButtons`, `initializeGameData`).

Case study: `VictoryConditionStruct::checkForArtifactWin` (0x5f1610) went from
Mac 41.4% to 96.8%. Every step was a source fact visible in the Mac code:

1. `pieces.erase(it);` discards the result.
2. The redundant `team >= 0 &&` goes; `isHumanTeam` already guards.
3. `!g_currentPlayer || playerDisabled[...]` is one guard.
4. The combination search sits inside `if (comboIdx != -1) { … }`.
5. `components.test(i)` and `h->hasArtifact(i)` are tested directly, with no
   local variables.

Steps 1–4 hold Windows CUR at 99.75%. Step 5 costs Windows 6%, so the
`bool carriesComponent` local looks like a VC6 shaping device.

Remaining on Mac: retail's frame is 0x10 larger, every local sits 0x18
higher, and two long-lived registers are swapped. `completeCurrentMap` shows
the same +0x18 pattern, so it is probably systematic (a compiler setting or a
header) rather than per-function source.

## Why Mac pairs lag behind Windows

1. **Optimisation level.** `config/mac/units.toml` sets `-O3` for every unit;
   commit db6df82dc made that a "working assumption". Retail hero code is
   unoptimised loop code: no CTR loops, and `which+1` is recomputed inside the
   loop. A sweep over 1,495 pairs:

   | Setting | Mean | Exact |
   | :------ | ---: | ----: |
   | `-O1` everywhere | 51.7 | 321 |
   | `-O2` everywhere | 61.4 | 408 |
   | `-O3` everywhere (current) | 73.5 | 588 |
   | `-O4` everywhere | 75.0 | 639 |
   | best level per unit | 77.9 | 674 |

   The best per-unit levels are `-O1` for hero, townmgr, tradpost and
   swapmgr, `-O2` for rmg and singleselectionwindow, and `-O4` for most
   others. Confirm each level from retail loop signatures, not from the score
   alone.
2. **Scoring.** The Mac score counts equal bytes at equal offsets, so a single
   extra instruction collapses it. On the per-unit-best build the positional
   mean is 78.2. Aligning the two instruction sequences gives 84.6, and also
   ignoring register numbers gives 92.6. Windows uses objdiff's alignment-based
   fuzzy score, so the two percentages are not comparable.
3. **Hidden source facts.** Each item below was verified with Windows staying
   at 100%.
   - Text lookups go through `(*g_generalText)[i]`, one inline wrapper deeper
     than our calls: 114 sites in 41 functions. This conflicts with the
     AGENTS.md rule to keep `getText(...)`, so it needs a policy decision.
   - The original used `ZeroMemory`/`CopyMemory`: 25 functions call `bzero`
     and 7 call `BlockMoveData`.
   - Callees that retail expands were declared `inline`, for example
     `type_event_record::load/save` and `isHumanTeam`.
   - `getLastBackpackIndex` reads `m_backpack[slot]` directly instead of
     calling `getBackpack(slot)`. This conflicts with the helper rule and
     needs review.
   - Expression shapes: `getIconIndex` uses `?:` (55.6 → 100);
     `getRouteArrayPtr` computes `x + y*W + z*W*H` (40.4 → 100);
     `town::getBuildCost` has a separate `cost` local (50 → 100);
     `valueOfPowerSchool` writes the `&` operands in the other order (85 → 100).
   - Class layout: CodeWarrior places the vptr where the first virtual
     function is declared. Moving `TProgressSink`'s data members after its
     virtuals took two pairs to 100%.
   - 44 pairs contain Mac byte-swap code that we lack; `platform.h` has no
     byte-swapping store macro yet.
4. **Open.** Frame size differs in 184 pairs, mostly by ±16 bytes; this is the
   same +0x18 pattern seen above, and extra or inlined-helper locals are
   suspected. 52 pairs differ only in register numbers and 61 only in
   instruction order, which points to expression- and declaration-order facts.

## Dreamcast decorated names prove `bool`

The original DC decorated names (`strings` of `H3.EXE`, demangled with
`llvm-undname`) show 167 HEAD functions where DC proves `bool` (`_N`) and our
source declares `unsigned char`, as a parameter or a return. The top classes
are CDPlay (39), advManager (15), CDPlayLobby (11), TSingleSelectionWindow (10),
VictoryConditionStruct (9) and TAdventureMapWindow (7).

We applied the TAdventureMapWindow and VictoryConditionStruct groups, then
ran a whole-tree Windows fast build. Every change was Windows-neutral except
`isGrailTarget`'s bool return (100 → 90.1). Mac `adventuremapwindow` went
from 16 to 17 exact pairs. `highlightLocators` went from 76 to 91 of 93
identical shape instructions; the remaining two differences are glue TOC
reloads.

The network follow-up restores 78 member interfaces across CDPlay (52),
CDPlayLobby (16), CDPlayHeroes (6), CDPlayMsg (2) and CDPlaySession (2), plus
the free `GetRemoteData(bool, bool*)` wrapper. Original NB11 public names
provide `_N` and native method spelling, for example
`?Send@CDPlay@@UAA_NPAXKKK_N@Z` and
`?GetRemoteData@@YAPAVCNetMsg@@_NPA_N@Z`. The latter's output-pointer contract
also types `CSingleSelectionNetMsgHandler::m_wasCompressed` as bool. The
unqualified byte field records for `m_connected` and `m_inSession` are not
enough to change them.
`GetSendQueueSize` and `GetReceiveQueueSize` also recover the two output
references encoded by `AAK0`; their bodies take addresses only at the SDK
pointer boundary. Complete's five-argument `RegisterApp` remains distinct
from Dreamcast's older four-argument version because retail passes and stores
the additional application-launcher name.

Changing the return declarations alone initially regressed 31 wrappers:
their authored `unsigned char ok = m_hRes >= 0` temporaries made VC6 use
extra result-register moves when returning bool. Keeping the relational
result in a `bool` local restores every preceding exact wrapper. Owning-TU
symbol refresh and all affected Windows consumers preserve MAX; all 102
previously available affected Mac comparisons retain scores and availability.
The network count remains 279/280 exact. This corrects source interfaces;
it does not resolve `TestLobbied`'s conditional-select residual.

## Compiler probes: which source differences each compiler preserves

Each probe compiles two spellings of one small function. The compilers are
VC6 SP3 and SP5 (the game profile), CodeWarrior (`-O3 -proc 750`, the Mac
profile) and GCC 2.95.2 (`-O2 -funroll-loops -fno-exceptions`). A row reads
`same` when the two spellings produce identical code, meaning the compiler
erased the distinction, and `DIFFERS` when the distinction stays recoverable
from the binary.

| Probe | SP3 | SP5 | CW | GCC |
| :---- | :-- | :-- | :- | :-- |
| inline member call vs pasted body | same | same | DIFFERS | DIFFERS |
| ordinary static helper vs pasted | same | same | DIFFERS | DIFFERS |
| `bool` vs `unsigned char` parameter | same | same | same | DIFFERS |
| early returns vs nested single exit | DIFFERS | DIFFERS | DIFFERS | same |
| `bool` local vs direct test | same | same | same | same |
| assigned vs discarded call result | DIFFERS | DIFFERS | DIFFERS | DIFFERS |
| split vs joined guards | same | same | DIFFERS | DIFFERS |
| redundant guard before inline helper | same | same | DIFFERS | DIFFERS |

How the differences show up:

- CodeWarrior evaluates an inlined member call once and reuses the value
  across a later call. The pasted body reloads the fields after that call.
- Split guards each keep their own `li r3,0; b`, while joined guards share
  one exit.

These are the reversible traces that make Mac and Loki valuable. Where VC6
gives the same output for two spellings, the Mac or Loki code can still
choose between them. The [mac-evidence skill](../../.agents/skills/mac-evidence/SKILL.md)
describes how to rerun a CodeWarrior probe.
