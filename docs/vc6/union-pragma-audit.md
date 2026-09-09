# Union and pragma audit

Scope: project-owned game C++ in `src/` and `include/`, against baseline
`231248aa`, English GOG Complete 4.0 and the pinned per-TU VC6 SP3 profiles.
Counts are definitions/directive regions, not keyword hits in comments or
the number of variables instantiated from a union definition. Forward
declarations, generated IDA types, generated build trees and pristine `vendor/`
are excluded. The two compiler probes `a04`/`a05` retain four diagnostic pragma
lines outside game source; they are experiments, not shipped workarounds.

## Census and decisions

| Construct | Baseline | After cleanup | Decision |
|---|---:|---:|---|
| Union definitions | 78 (47 source, 31 header) | 68 (42 source, 26 header) | Ten removed; classify the remainder below |
| Inline override regions | 289 | 248 | 41 removed, including six in disabled negative-example code |
| `inline_depth(0)` regions | 262 | 241 | Remaining overrides are matching debt |
| `inline_depth(1)` regions | 7 | 0 | All seven redundant |
| `auto_inline(off)` regions | 20 | 7 | Three redundant; ten retired by recovered helpers, locals, types and meaningful release verifications |
| Packing regions | 11 | 11 | Preserve layout contracts: eight pack-1, three pack-8 |
| All pragma directive lines | 600 | 518 | Each region includes its closing/reset directive |

The six disabled regions were in `army.cpp`'s rejected `drop_aura_links`
example under `#if 0`. Thus 35 active inline overrides were removed; counting
all 41 as active compiler interventions would overstate the cleanup.

“Retain” does **not** mean that the original source contained a union or an
inline pragma. Retail establishes behavior, layout and call/expansion choices,
not the spelling of a compiler workaround. In particular, none of the remaining
inline overrides is certified as original source. Their deletion controls locate
the current matching dependencies; natural declarations, helper visibility,
lifetimes and TU state still need recovery before removing those dependencies.

### Removed unions

| Owner | Removed representation | Evidence and replacement |
|---|---|---|
| `CObjectType` in `advmgr_objects.h` | Single-member `m_objectType` union and unused `s_objectTypeValue` | One real dword at +0x38; replace with that scalar. The static member's own comment admitted it existed solely to perturb VC6. No substitute compiler mass was added. |
| `NewmapCell::TObjectCell` in `mapcell.h` | `m_objectIndex` / `m_objectIndexAlias` | Same unsigned-short field and same offset; retain canonical `m_objectIndex`, update renderers. |
| Same record | `m_layer` / `m_height` | Same signed-byte layer; retain `m_layer`, update renderer. Packed nibble/byte offsets remain a real union. |
| `File` in `winfile.h` | HANDLE / integer `m_fileValue` | Canonical `void*` handle plus `return 0` in seven null-handle guards reproduces the whole TU's code and named relocations. The old claim that explicit zero could not match was false for the current source state. |
| `game::giveTroopsToNeutralTown` | `TNeutralWeightAddress` and its address-to-integer helper | DC game.cpp:3967/3969 positively establishes a signed level index, six-entry bound and indexed weight subtraction. VC6 strength reduction recreates retail's signed pointer/end comparison from that loop; both indexed guard formulations stay 100%. |
| Puzzle coordinate tables | `TPuzzleCoordinatePointer` byte/short pointer alias | Retail 0x52c6c0/0x52c9b0 reads signed words with a 96-short puzzle-row stride. Declare short tables and use typed row pointers. `updatePuzzle` remains 100%; `aiAttemptPuzzleGuess` preserves 97.1621%. |
| `SavedGameHeader::reset` | Byte/dword `isHuman` local | Store the bool result directly in the integer flag array. Removing this matching-only representation costs 100% → 98.2888% in this one function; the old DC Reset predates Complete's loop. |
| `TViewArmyWindow` | Three local `shownType` enum adapters | DC CodeView proves `ArmyType` is `TCreatureType`. Restore that member type and use it directly; retain the proven int constructor parameter and int `Upgrade` boundaries. Both affected constructors improve when their canonical name-helper calls are restored too. |

The puzzle control is instructive: flattening row and piece into one short-array
index scored 96.4516% / 96.6598%. A 36-state family of actual pointer/value
lifetimes produced eight distinct objects; separating row selection from piece
indexing recovered both peaks without the union. This is an address-expression
choice, not evidence that the storage is a byte buffer. No casts, new inline
pins, alternate declarations or dummy operations were introduced.

### Remaining unions: complete accounting

| Role | Count | Disposition |
|---|---:|---|
| Scalar integer/enum adapters | 32 | Encoding/type-boundary debt; not established as necessary compiler interventions |
| Enum/raw views of record fields | 3 | Migrate readers and writers together before removing |
| Pointer adapters/views | 11 | Two intentional ABI views; nine adapters requiring owner/call-boundary recovery |
| Numeric bit/width views | 7 | Intentional representations |
| Tagged, packed or external-layout unions | 15 | Preserve actual shared-storage representations |
| **Total** | **68** | **24 intentional representations; 44 reconstruction adapters/workarounds** |

The 32 scalar adapters are accounted for below. A local definition with two
declarators (for example `building, bestBuilding`) counts once. Each connects
an integer, loop ordinal, serialized value or packed field to a recovered enum
consumer. They should not be advertised as original union declarations.

| Owner | Definitions | Domain/boundary |
|---|---:|---|
| `include/advmgr.h` | 2 | Scholar award and secondary-skill packed accessors |
| `include/artifact.h` | 1 | Serialized/map/UI artifact ordinal |
| `include/primaryskill.h` | 1 | Packed/random primary-skill ordinal and sentinel |
| `include/town.h` | 1 | Resource/reward ordinal |
| `src/advmgr.cpp` | 4 | Creature/building helpers and two creature-bank extra-info reads |
| `src/ai_player.cpp` | 4 | Building/resource enumeration and selected-building values |
| `src/army.cpp` | 1 | Wall-target ordinal |
| `src/creature_bank.cpp` | 1 | Reward-creature input |
| `src/events.cpp` | 1 | Creature event/input adapter |
| `src/game.cpp` | 4 | Creature, hero-class, creature-bank and secondary-skill adapters |
| `src/hero.cpp` | 1 | Drawn secondary-skill index |
| `src/mapcell.cpp` | 2 | Serialized adventure-object type reads |
| `src/objecttype.cpp` | 1 | Stream object-type input |
| `src/philai.cpp` | 3 | Two secondary-skill values and one creature value |
| `src/seerhut.cpp` | 1 | Serialized quest creature value |
| `src/townmgr.cpp` | 2 | Creature and building ordinals |
| `src/viewarmywindow.cpp` | 2 | Proven int constructor/upgrade inputs; the displayed member is now typed |

The tree also has memcpy-based versions of some of these adapters, outside the
union census. Replacing every union with a memcpy, `void*` detour or an admitted
cast would make the keyword count smaller without recovering the original type
model. This audit deliberately does not do that. The zero ordinary enum-cast
floor is not proof of union necessity; nor is its narrow retail-revision
admission a blanket waiver for every integer-to-enum conversion. The next step
is canonical ownership and real input-domain recovery, preserving the attested
public ABI rather than changing parameter types solely to satisfy callers.

The other 36 remaining definitions are exhaustively grouped here:

| Owner / member | Count | Why retained / removal condition |
|---|---:|---|
| `armygrp.h`: spell-trait `m_school`/`m_schoolBits`, roster `m_armies`/`m_armyTypes`; `mapcell.h`: `m_type`/`m_typeValue` | 3 | Shared enum/raw record fields. Loader flag accumulation, integer slot-writing APIs and serialized dword stores must be reconciled with typed consumers together. A same-size field edit alone leaves invalid users. |
| `Bitmap16MapPointer`, `Bitmap16ConstMapPointer` | 2 | Pixel reads use 16-bit elements while pitch and row stepping use bytes, including `winmgr::fizzle`. A canonical byte-pitch row-access model is needed before retiring these pointer adapters. |
| `message` payload and `TIPv4SocketAddress` | 2 | Intentional integer/text message ABI and same-record sockaddr_in/sockaddr API views. Keep shared storage, not two sequential fields. |
| Local pointer payloads: `advmgr::drawRolloverText`, two hero-screen portrait sends, `swapmgr::textPointerPayload` | 4 | Current retained broadcast overload takes an integer payload. Recover its real call/expansion decision before switching to the message-pointer overload; do not invent a text overload solely to hide the conversion. |
| `TGatePairVectorPointerAlias` | 1 | DC retains both long-vector and point-vector serializers; retail folds their compatible bodies. Canonical element ownership, load/resize behavior and retained template claims need joint recovery, not another pointer cast. |
| `TUniversitySkillsPointerAlias` | 1 | `RandomizeUniversity` passes a four-int default to the record constructor/copy boundary. Directly declaring the record currently introduces the out-of-line Complete constructor where retail has no call (prior control 97.86% vs 99.75%). DC lacks this Complete constructor; its absence there does not establish an inline body. |
| `TMarketArtifactList` | 1 | The black-market entry accepts bytes; other entry points supply artifact arrays; panels use raw IDs/sentinels and typed artifact APIs. Reconcile entry-point/element ownership, then remove the adapter. |
| `TFloatLongBits` / `TDoubleLongBits` in bitmap16 and palette; the two `viewwrld::ftol` locals | 6 | Actual float/double bit reinterpretation and low-word extraction for the magic-constant conversion. Numeric casts are not equivalent. Keep the representation unless an equally evidenced implementation replaces it. |
| `TBlendMask` | 1 | Actual word/dword views used by blend-mask loads, not two independent values. |
| DirectPlay `DPNAME`, `DPSESSIONDESC2`, `DPCHAT` narrow/wide string slots | 5 | External ABI alternatives occupy the same fields. |
| `ExtraInfoUnion`, CObject extra-info, NewmapCell extra-info, `LegacyUpgradeExtraInfo`, `CurrentUpgradeExtraInfo` | 5 | Tagged map-object payloads and legacy/current serialized bit layouts. |
| NewmapCell flags and TObjectCell packed X/Y offset byte | 2 | Real bitfield/raw views of the same allocation units. |
| Hero-specific ability payload, seer reward payload, palette array/object view | 3 | Kind-dependent payloads and an actual common backing representation. |

Packing is separate from inline suppression. The pack-1 regions cover retail's
unaligned world cells, hero data, campaign legacy records, path cells, movement
network message, seer data, and the two narrow bands in HeroExtra/playerData.
The three pack-8 scopes explicitly preserve the natural HeroExtra, AI and
playerData layout around those bands. Their offsets/copy behavior are documented
at the owning declarations. Removing layout contracts because the current
ambient default happens to match is not a source-recovery improvement.

## Inline deletion controls

Every one of the original 289 regions was removed individually, with no C++
body rewrite. Every TU also received unchanged and all-overrides-removed
controls. All 35 finite families were exhausted. The repository source-family
runner compiled isolated source/header snapshots, used each exact TU profile,
checked the unchanged result against the ledger, checked the opposite corner,
and reproduced its retained distinct candidates. Whole-TU score vectors include
exact siblings and disappearance of retained bodies, not just the caller being
improved. Generated records also compare emitted code and named relocations.

| Individual deletion result on baseline | Regions |
|---|---:|
| Code and named-relocation identity unchanged | 18 |
| Tracked scores improve with no tracked loss | 8 |
| Both gains and losses | 6 |
| Tracked scores unchanged, other emitted code changes | 1 |
| Tracked losses only | 256 |

The first 18 include the six inactive `army.cpp` directives. All 18 were removed.
Seven of the eight individually improving regions were removed after testing
their combinations; the eighth conflicts with the selected `loadMap` pair.
A second complete sweep against the cleaned-up headers tested the remaining
264 regions: 257 loss-only, five mixed, and two score-flat/code-different.
That sweep exposed one further removable emission anchor, described below.
No gain-only or score-flat result is treated as proof of equivalent code.

The following baseline line numbers identify the first 26 adopted deletions. They
refer to `231248aa`, not moving line numbers in the edited files.

| TU | Regions before → after | Removed opening-directive lines |
|---|---:|---|
| `ai_player` | 6 → 4 | 4892, 5329 |
| `army` | 15 → 9 | 4722, 4728, 4739, 4746, 4752, 4763 (disabled example) |
| `findpath` | 3 → 2 | 1426 |
| `game` | 67 → 58 | 880, 3918, 6826, 6873, 7803, 7926, 7997, 8492, 18764 |
| `hero` | 17 → 15 | 3144, 6363 |
| `philai` | 44 → 43 | 1773 |
| `resourcemanager` | 17 → 16 | 961 |
| `town` | 5 → 3 | 1115, 1150 |
| `viewarmywindow` | 4 → 3 | 518 |
| `viewwrld` | 2 → 1 | 1374 |

Unchanged TU counts in that first pass: adventuremapwindow 2, advmgr 8, ai_combat 3, ai_tactical 1,
campaignbrief 2, cmbtmgr 8, command 1, cursor 1, event_record 2, events 8,
kb 3, kbwin 1, mapcell 13, mousemgr 1, multiplayerwindow 8, puzzlewindow 2,
remote 3, rmg 1, sacrifice_window 1, scenarioinfo 1, seerhut 10,
singleselectionwindow 24, soundmgr 1, spells 3, tradpost 1.

Important baseline exceptions and interactions:

- `game.cpp:6850`: removing any one of the three `loadMap` bitset pins helps
  on the original baseline, but removing all three gives 62.3643% versus
  63.8664%. All eight subsets were compiled. The selected two deletions give
  63.9902% and remove more overrides than the best single-deletion score.
- `mapcell.cpp:4293`: a small `readObject` improvement loses four exact
  STL bodies. Retain until the caller and retained-template boundaries agree.
- `game.cpp:3800` and `3925`: small Load gains delete exact black-market erase
  and bitset-reference bodies respectively. Caller score alone is misleading.
- `singleselectionwindow.cpp:271`: removing auto-inline suppression improves
  `generateRandomMap` from 92.6386% to 97.9759%, but lowers
  `setupScenarioOptions` from 100% to about 90.15%.
- `philai.cpp:817`: `valueOfBank` reaches 100%, but `aiValueOfEvent` falls from
  about 98.03% to 97.46%.
- `remote.cpp:1842`: a destructor reaches 100%, but an exact copy body
  disappears and `wait` falls from about 90.65% to 75.07%.
- `singleselectionwindow.cpp:3979`: the tracked score vector is unchanged,
  but the untracked `CNewPlayerUpdateProc` destructor changes from a 5-byte
  tail jump to a 38-byte expansion of task destruction and deletion. Retain
  pending boundary recovery. Some other object differences only renumber
  local labels; this one has actual changed instructions. Scores alone would
  incorrectly classify it as redundant.

The additional removal is `game.cpp:18764`, the pin inside the explicit
`bitset<8>::reset` specialization. On the original baseline its deletion loses
the exact `_Tidy<8>` body. After the real Load constructor pin is removed,
`game::load` itself calls `_Tidy<8>` at +0xb76 and keeps the exact body emitted.
Only the **untracked** reset wrapper changes (14-byte call wrapper versus
9-byte inlined zero store). A three-state control compared unchanged source,
pin removal, and whole-specialization removal: every tracked game score agrees.
The whole specialization was removed so the canonical vendor reset is used;
vendor files themselves are untouched. This is a demonstrated obsolete
emission workaround, unlike the unresolved UI destructor boundary above.

After that last edit, all 58 remaining game regions were retested against its
new snapshot (60 alternatives including unchanged/all-removed; 60 distinct
objects): 57 loss-only and one mixed. Together with the unchanged other TUs'
second-sweep results, the then-remaining 263 regions were **257 loss-only, five mixed,
and one score-flat with an untracked destructor change**. No further individually
code-identical or gain-without-tracked-loss deletion remains in this tested
source state. This is a single-deletion result, not proof that a larger,
evidence-backed reconstruction cannot retire more overrides.

### Small helpers: release-verification recovery

The deeper pass starts from merged checkpoint `8472fc36` (4062 exact rows).
Ten of those previously loss-only overrides are now removed. The first five
are listed here; serialization and the map accessor are treated separately
below because they establish different source facts. Each function
received the Dreamcast dossier/line blocks/inline clues and retail
summary/CFG/source pass before its bounded source family was authored.
Missing CodeView rows identify possible source locations, **not recovered
ASSERT text**. The retained expressions check actual array or input
preconditions, and compile to no runtime assertion branch or call.

| Helper | DC gap | Retained precondition | Deletion-only negative control |
|---|---|---|---|
| `hero::giveSS` (0x4e22d0) | hero.cpp:4628..4631 | Skill index within `m_skillLevel` | `setSS` 100 → 3.8; `checkLevel` 87.1338 → 49.8180; two other callers lose |
| `advManager::getNormalCursor` (0x40e280) | advmgr.cpp:4531..4532 | Non-null cell | `processHover` 91.6263 → 63.9466 |
| `type_AI_creature_purchaser::doPurchase` (0x42d690) | ai_player.cpp:2627..2628 | Non-null army and funds; adjacent army remains optional | `markTown` 100 → 0; `buyCreatures` 97.7723 → 52.4409; `valueOfHiring` 99.9522 → 79.2183 |
| `CDPlayHeroes::compressMsg` (0x5532b0) | remote.cpp:426 | Non-null message, size at least its wire header | Four member/free transmit rows 100 → 0 |
| `hero::getHeroSpellBonus` (0x4e5ff0) | hero.cpp:6429..6430 | Hero ID within the ability array | `modifySpellDamage` 100 → 32.6226 |

For this first five-function stage, the retained bodies, callers, and **all
other raw COFF section bytes** agree
with their respective pinned controls. Section layout/flags and function
names/locations agree too. Defined relocation destinations match by proven
identical section/offset; undefined targets retain their names. This checks
3439 relocations in hero, 4105 in advmgr, 1962 in ai_player and 1743 in remote,
including untracked emitted code. Only compiler-private label identities vary.
`compare-coff-layout.py` performs this read-only stronger comparison; it fails
on the deletion-only controls and never changes the normal scoring rules.

The five finite families scored 7/3/5/5/4 source states respectively, with
4/3/5/5/4 distinct named-object identities. All distinct retained elites were
reproduced. Combined versus separate checks matter: `giveSS` needs the combined
range predicate for TU-wide byte identity, while `getHeroSpellBonus` uses two
separate bounds. The other spelling retains the intended call but perturbs
`giveArtifact` from 79.8138 to 79.7247. DC also proves an outer else and an
inner capacity test in `giveSS`; that scope was restored. Flattening it is
byte-neutral, unlike removing the verification.

The source-family generators accept the inspected pre-edit or retained body,
refuse an unreviewed boundary, and can regenerate current-source controls:

```sh
python scripts/experiments/generate-givess-boundary-family.py build/givess-family.json
python scripts/experiments/generate-small-helper-verification-family.py cursor build/cursor-family.json
PYTHONPATH=scripts python -m homm3.vc6.source_families build/cursor-family.json --keep 3 --generations 1
```

Use `purchase`, `compress`, or `spellbonus` for the other individual families.
Historical contexts, in the table's order, are `2ff41da5e8b0e579044c`,
`192a25c02fd05fa8e2d1`, `51f3cf10f7743637ddfb`, `8845e9f7b12b05bd6fd7`, and
`faf7fbb799ec071bd270`. Fresh source controls intentionally get new contexts.

### Serialization locals and interface recovery

DC gives positive `int count` locals and separate I/O/result-test statements
for `game::loadString` (a7414), `game::saveString` (a750c), and
`NewSMapHeader::readString` (b1110). Restoring those statements removes the
reader fences without assertions. The writer additionally needs the meaningful
`outfile != 0` verification, permitted by the gap at game.cpp:2532. Count
alone or verification alone still expands the writer into `saveSignPool` and
`saveRumours`, taking those exact callers to 6.5% and 18.895% respectively.
The map reader's apparent leading gap belongs to the preceding function;
no assertion is inferred there.

All 75 combinations of the three bounded families were scored; the selected
2/4/2 combination was checked again in an eight-subset family. Both runs
completed their control/reproduction gates. The adopted source preserves all
809 raw game-object sections and 5,402 relocation destinations. Historical
contexts are `bbb0ea4d101dfcdc2fe2` and `36d10952015e84cc3ae4`.
An earlier run whose live headers changed
failed the snapshot guard and is **not** counted as a completed experiment.

The declarations were also corrected independently: Dreamcast proves static
class membership and string-reference arguments. Retail's two-register `/Gr`
calling convention does not make them free functions or pointer interfaces.
The obsolete `saveAbstractString` and `readMapString` names are gone; owning
source claims regenerate the labels. With precisely those three evidenced
symbol renames, every game-object byte, function location, and relocation
destination still agrees. The comparison tool requires an explicit bijective
function-rename map; swapping the loader and writer names fails its negative
control. This does not change the repository's score normalization.

`NewfullMap::saveObject` (0x503640, DC f1b1c) supplies another distinct result:
its plain `char` temporary **and** `int count` local are jointly required.
Deleting the fence, restoring count alone, or restoring char alone expands
the writer into `saveMapObjects` (100% → 55.4453%). Both recovered details
together preserve all 419 raw mapcell-object sections and 2,077 relocation
destinations. Five alternatives and four distinct objects passed reproduction
in context `41f7062873689b4033e9`. Its CObject-reference argument was restored
separately with the same whole-object proof and one explicit symbol rename.
Its boundary row is also borrowed: no assertion was added.

```sh
python scripts/experiments/generate-string-boundary-family.py build/string-family.json
python scripts/experiments/generate-string-boundary-family.py build/object-save-family.json --object-save
PYTHONPATH=scripts python -m homm3.vc6.source_families build/object-save-family.json --keep 5 --generations 1
```

These generators accept both the inspected pre-edit source and the adopted
source. Current controls do not reintroduce old pragmas.

### Map accessor: canonical helper retained through caller collateral

DC MapCell.h:847/850 proves const/mutable private `zCell`; scalar
wrappers follow at 889/895, and the packed-point wrapper at 906 calls `zCell`
directly. The original three families (6, 10 and 6 states across eight TUs)
tested that boundary, coordinate checks and method order. A final six-state
storage-precondition family (`2ecfcdb04594fd1e2ec7`) resolves the earlier
retained-body problem without renaming a claim: `m_cellData != 0` in the
mutable scalar wrapper makes both `cell` and `zCell` emit the identical
49-byte body. With no check, only `zCell` retains that body. The wrapper's
line 896 gap permits this real precondition, not historical ASSERT text.
Neither the single-line private helper nor the const wrapper receives an
unsupported check. All six states and their distinct retained elites were
reproduced.

The canonical helper pair, wrapper calls and original method order are now
adopted. The `HOMM3_NEWFULLMAP_CELL_OUTOFLINE` fork and its auto-inline fence
are removed; both per-TU scaffold sites disappear. The proof does **not** claim
whole-TU byte identity here. `processOnMapTowns` rises 94.3642% → 98.6258%,
monster quest text 93.002% → 98.3426%, and `searchArray::pushPoint` rises
98.2622% → 99.5758%. Header collateral lowers four exact drawing functions:
boat part/shadow to 98.2619%, hero part to 99.75%, and hero shadow to 99.7524%.
`CEnterNameEdit::onKillFocus` also moves 100% → 99.8710%. Other changed
non-exact callers are recorded in the generated ledger; MAX/HIST preserve
their banked peaks. Score dips do not refute this positive helper evidence.

The four drawing dossiers and retail source/CFG diffs localize follow-up:
the boat invalid-point arm wrongly expands scalar `cell(0,0,0)`; the hero
arms retain separate scalar/private calls instead of the retail merged call.
Their existing `drawHeroCell`/`drawBoatCell`/`drawGroundCell` copies are
canonical-helper debt. DC `advManager::GetCell` actually calls scalar `cell`
at 7028 and packed-point `cell` at 7029; the old comment claiming a direct
DC `cellData` return was incorrect. Do not restore the TU fork or paste more
arithmetic into callers to recover their scores.

```sh
python scripts/experiments/generate-cell-boundary-family.py build/cell-family.json --current
```

The other cell-generator modes explicitly replay the historical forked source
and refuse the adopted boundary. `--current` preserves the canonical helpers.

Map-access checkpoint census: 253 overrides (246 depth-zero, seven auto-inline-off).
This does not reclassify them as necessary. The assertion controls directly
demonstrate why a pragma-deletion-only census cannot establish necessity.

The seven remaining auto-inline regions have bounded next actions:
`convertVolume`, `checkDimNextHeroBut`, and `~CAnimatedDlg` have no leading
DC gap supporting an assertion; the destructor's sprite is explicitly
optional. `AppCommand` is a four-byte DC platform stub and `stopMouseThread`
is mostly PC-only teardown, so missing PC statements are not assertion proof.
`CNewPlayerUpdateMan::headerRequested` now has its source-proven concrete-proc
call and Complete's changed queue payload; its remaining caller dependency
is measured below.
The empty `CNewPlayerUpdateTask` destructor has the previously measured
untracked derived-destructor expansion. These remain reconstruction debt,
not candidates for invented checks or declarations merely to alter inlining.

### Further reduction from `ab315284`

This stage removes five more depth overrides and four union definitions.
Seven auto-inline-off regions remain; none is newly certified necessary.

`CNewPlayerUpdateMan::headerRequested` at 0x5892b0 now calls the ordinary
`CNewPlayerUpdateProc::headerRequested` defined in the original TU. DC line
1496 names the call, and the earlier helper at 0x148348 owns queue insertion.
Complete's retail expansion queues an eight-byte flag/number value instead
of the older allocated integer. The adjacent `headerConfirmed` call and
out-of-class ordinary `getProc` definition are restored from their DC
boundaries too. The 16-state family raises the manager from 88.6223% to
100%, preserving every other tracked score in its TU. Deleting the manager's
auto-inline override still expands it into `handleNetMsg`, lowering that
caller from 89.7408% to 85.9113%; no fabricated assertion replaces the fence.

`SavedGameHeader::reset` uses the real implicit campaign/map-header assignments,
replacing the pasted member walks and their three depth fences. Its empty
`resetAssignmentSurface` helper and call are removed, not replaced by other
compiler work. The 54-state family separates both assignment spellings,
human-flag values and the empty helper. Real assignments with the old union
remain 100%; the adopted direct bool-to-int store gives 98.2888%. Every other
tracked game score is unchanged. This small explicit tradeoff removes a
byte/dword workaround without pretending DC proves Complete's flag loop.

`TViewArmyWindow::ArmyType` is an enum in DC CodeView; `Upgrade` and the third
constructor's `army_type` parameter remain int. Restoring the field type
removes three local union definitions and one redundant declarator from the
remaining upgrade union. The battle constructor calls `army::getName` (DC
line 70) and appends both help strings (98/107); the group constructor calls
`getArmyName(m_armyType, 2)` (159) and retains its own help-string assignments.
These are canonical helper/operator boundaries, not pasted lookup code.

The 32-state popup family crosses field ownership, both name calls, the battle
append operations and all four deletion subsets of its two description fences.
It checks `viewarmywindow`, `cmbtmgr`, `game`, `recruit`, `sacrifice_window` and
`hillfortwindow`. With all source corrections and both fences removed, the
battle constructor improves 91.2989% → 92.6780%, and the group constructor
93.7569% → 97.4452%; every other tracked score across all six TUs is unchanged.
Keeping only the luck fence reaches 93.5871% in the battle constructor, but
the two-deletion state removes more debt and still improves over the baseline.
Deleting both fences without restoring the name calls gives only 87.4391%
with the typed field. The ordinary source boundaries matter jointly.

These families are reproduced by their generators under `scripts/experiments/`:
`generate-header-request-boundary-family.py` (`--adjacent`, context
`775af75f16447393e9f2`), `generate-saved-header-reset-family.py`
(`8eb0ac9d4548f228b82f`), and `generate-viewarmy-ownership-family.py`
(`36638968a9b0a5d650da`). They are historical pre-adoption controls: run in a
prepared worktree at `ab315284` for the header/reset sources. The popup family
was run after their adoption, so its whole-six-TU score vector also includes
the lower reset score. Generators reject changed anchors rather than silently
testing different source. Frozen input snapshots retain the exact experiments.
All 16/54/32 states scored, with 16/48/32 distinct objects and ten reproduced
elites in each family. The selected all-corrections corners were independently
recompiled as well. Final production objects match the selected family objects
under the strict whole-COFF check, including named external relocations and
all untracked emitted sections, for both edited TUs and all six popup consumers.

## Reproduction and verification

The audit scripts generate analysis only; they do not adopt source, adjust the
ledger, edit vendor code, or rewrite Dreamcast source structure. Run from a
prepared owning worktree (pinned toolchain installed and Wine prefix initialized).
Set the root explicitly: an inherited `HOMM3_DIR` can point at another worktree.

```sh
export HOMM3_DIR="$PWD"
export HOMM3_TOOLCHAIN="$HOMM3_DIR/build/homm3-toolchain-vc6-sp3"
export WINEPREFIX="$HOMM3_DIR/build/wineprefix"
homm3 build
python3 scripts/experiments/audit-inline-pragmas.py --run
python3 scripts/experiments/summarize-inline-pragmas.py
```

Use `--output-dir build/pragma-audit-current` for a distinct follow-up inventory
and pass that directory to the summarizer. Unit arguments restrict compilation;
the full inventory still requires all unit contexts before a complete summary
can be emitted. Context IDs record the exact snapshot/profile/target identity.
The summarizer joins options by rendered source identity, not cached labels:
single-region and all-removed alternatives share an identity in one-pin TUs.
It rejects missing/unscored alternatives and changed score-vector coverage.

The initial measurements live in `build/pragma-audit/{inventory,results,
all-removed-results,contexts}.json`; complete object comparisons and original
snapshots live in `build/source-families/`. These are generated, ignored
artifacts, not a second hand-maintained symbol ledger. To repeat the historical
numbers, run the scripts against the baseline source/headers in a separate
worktree; running on the cleaned tree is intentionally a different experiment.
The second full sweep lives in `build/pragma-audit-current/`; the last game-only
follow-up after removing its specialization lives in `build/pragma-audit-final-game/`.

The required Dreamcast dossier/assembly/inline-clue and retail summary/CFG/source
passes preceded non-exact source reconstruction. Function-local comments retain
the new negative controls and correct stale claims about now-removed pins.
Full builds refresh retail delinking, generated labels, CUR/MAX/HIST and gates;
`--fast` alone is not the final verdict.

“Exact” here uses the repository's existing report metric (relocation-name
comparison disabled), not a claim that every symbol identity is resolved.
Named-call review still finds the pre-existing const/non-const `town::getArmy`
overload discrepancy in the neutral-town routine and `__chkstk` versus
`__alloca_probe` in the puzzle AI routine. The deletion work does not resolve
or hide those earlier boundary/label questions; unclaimed puzzle data labels
also remain unclaimed. Before/after whole-object identity controls are a
separate, stronger check for the removals described as code-identical.

Initial audit checkpoint: 4061/4752 exact functions, versus 4059 before cleanup;
linked fuzzy score 96.29% versus 96.28%; whole-image score 96.03% versus 96.02%.
All source/layout/claim/cleanliness/banked-row gates pass. Besides the six
improving callers, shared-header collateral recovers `initializeGameData`
(90.1620% → 100%) and `CEnterNameEdit::onKillFocus` (99.8710% → 100%).
The only lower current score is unchanged-source header collateral in
`TCampaignBrief::CampaignHeaderStruct::load`: 53.9868% → 53.9715%; its MAX/HIST
remain banked. No authored function finishes below its preceding MAX.

First five-helper follow-up checkpoint: full `homm3 build` remains 4062/4752 exact,
96.30% linked fuzzy and 96.04% whole-image, identical to its `8472fc36` starting
checkpoint. Every tracked CUR/MAX/HIST score is preserved; only the five edited
function source hashes change. Retail delinking and all repository gates pass.

After canonical map access and the serialization recovery, the full checkpoint
is 4057/4752 exact, 96.28% linked fuzzy and 96.02% whole-image. The five lost
exact rows are the explicitly measured shared-map-header collateral above,
not losses hidden by the byte-neutral serialization controls. Source identity
migrations preserve the original RVAs and histories; no historically banked
row is lost. Retail delinking and all repository gates pass.

The further-reduction checkpoint is 4057/4752 exact, 96.29% linked fuzzy and
96.03% whole-image, versus `ab315284`'s 4057/4752, 96.28% and 96.02%.
Only four current function scores change: headerRequested and both popup
constructors improve; reset takes the documented 1.7112-point local reduction.
Its HIST remains 100%; no banked RVA is lost. All repository gates and retail
delinking pass. The final census is 248 inline overrides and 68 union definitions.

This audit does not claim TU closure, all remaining unions as original source,
or all inline debt solved. The remaining reconstruction classes above identify
what must be recovered; they are not permissions to add new suppression pins.
