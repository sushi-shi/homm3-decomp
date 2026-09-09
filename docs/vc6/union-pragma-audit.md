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
| Union definitions | 78 (47 source, 31 header) | 63 (37 source, 26 header) | Fifteen removed; classify the remainder below |
| Inline override regions | 289 | 217 | 72 removed, including six in disabled negative-example code |
| `inline_depth(0)` regions | 262 | 212 | Remaining overrides are matching debt |
| `inline_depth(1)` regions | 7 | 0 | All seven redundant |
| `auto_inline(off)` regions | 20 | 5 | Three redundant; eleven retired by recovered helpers, locals, types and meaningful release verifications; one retired after complete untracked-body review |
| Packing regions | 11 | 11 | Preserve layout contracts: eight pack-1, three pack-8 |
| All pragma directive lines | 600 | 456 | Each region includes its closing/reset directive |

The six disabled regions were in `army.cpp`'s rejected `drop_aura_links`
example under `#if 0`. Thus 66 active inline overrides were removed; counting
all 72 as active compiler interventions would overstate the cleanup.

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
| `game::load` / `game::save` | `TGatePairVectorPointerAlias` and its forced wrapper | Restore the common `loadVector` / `saveVector` templates with bool results and native vector references. The long-vector resize restores retail's zero-filled new elements; the point/long retained writers have identical bytes. Six Load fences disappear, both callers improve, and both claimed writers remain exact. |
| `game::randomizeUniversity` | `TUniversitySkillsPointerAlias` and its forced wrapper | Restore the DC-proven native aggregate. Retail's elemental-school initializer is Conflux-specific, not a generic record constructor. The map local remains 99.7464%, Load improves to 81.2284%, and explicit Conflux initialization preserves the retained body and all four call/expansion sites. Shared-header collateral is measured below. |
| Marketplace artifact state | `TMarketArtifactList` byte/enum/integer pointer views | DC `DoBlackMarket` takes `TArtifact*`; both actual producers already own seven-element `TArtifact` arrays. Correct the entry signature, pass `TBlackMarket::m_artifacts`, and use one native pointer throughout. All five header consumers preserve their whole raw COFF section bytes, function locations and relocation destinations under the evidenced signature rename. |
| `NewfullMap::readObjectType` | Full-width integer/enum `convertedType` | Read the four-byte wire value into its native enum local and commit only after the short-read guard. This is a Complete ownership hypothesis, not the DC `int_buffer` spelling. The diagnostic queries the committed record, positively supported by DC row 3619. Only 18 stack-displacement bytes change; the function retains 99.9633% and all other mapcell functions are byte-identical. |
| `initializeCreatureBankTraits` | Reward-table `creatureTypeFromInt` | DC proves both tables are mutable function-static `TCreatureType` arrays owned by this loader. Restore those owners, the enum's consumed names and the direct enum assignment. All 66 emitted dwords match writable retail data; the reader's canonical static helper also lifts the loader from 89.4550% to 97.5355%. |

The puzzle control is instructive: flattening row and piece into one short-array
index scored 96.4516% / 96.6598%. A 36-state family of actual pointer/value
lifetimes produced eight distinct objects; separating row selection from piece
indexing recovered both peaks without the union. This is an address-expression
choice, not evidence that the storage is a byte buffer. No casts, new inline
pins, alternate declarations or dummy operations were introduced.

### Remaining unions: complete accounting

| Role | Count | Disposition |
|---|---:|---|
| Scalar integer/enum adapters | 30 | Encoding/type-boundary debt; not established as necessary compiler interventions |
| Enum/raw views of record fields | 3 | Migrate readers and writers together before removing |
| Pointer adapters/views | 8 | Two intentional ABI views; six adapters requiring owner/call-boundary recovery |
| Numeric bit/width views | 7 | Intentional representations |
| Tagged, packed or external-layout unions | 15 | Preserve actual shared-storage representations |
| **Total** | **63** | **24 intentional representations; 39 reconstruction adapters/workarounds** |

The 30 scalar adapters are accounted for below. A local definition with two
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
| `src/events.cpp` | 1 | Creature event/input adapter |
| `src/game.cpp` | 4 | Creature, hero-class, creature-bank and secondary-skill adapters |
| `src/hero.cpp` | 1 | Drawn secondary-skill index |
| `src/mapcell.cpp` | 1 | Narrow serialized adventure-object type read |
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

The other 33 remaining definitions are exhaustively grouped here:

| Owner / member | Count | Why retained / removal condition |
|---|---:|---|
| `armygrp.h`: spell-trait `m_school`/`m_schoolBits`, roster `m_armies`/`m_armyTypes`; `mapcell.h`: `m_type`/`m_typeValue` | 3 | Shared enum/raw record fields. Loader flag accumulation, integer slot-writing APIs and serialized dword stores must be reconciled with typed consumers together. A same-size field edit alone leaves invalid users. |
| `Bitmap16MapPointer`, `Bitmap16ConstMapPointer` | 2 | Pixel reads use 16-bit elements while pitch and row stepping use bytes, including `winmgr::fizzle`. A canonical byte-pitch row-access model is needed before retiring these pointer adapters. |
| `message` payload and `TIPv4SocketAddress` | 2 | Intentional integer/text message ABI and same-record sockaddr_in/sockaddr API views. Keep shared storage, not two sequential fields. |
| Local pointer payloads: `advmgr::drawRolloverText`, two hero-screen portrait sends, `swapmgr::textPointerPayload` | 4 | Current retained broadcast overload takes an integer payload. Recover its real call/expansion decision before switching to the message-pointer overload; do not invent a text overload solely to hide the conversion. |
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
singleselectionwindow 24, soundmgr 1, spells 3. The subsequent native market
setup recovery retires tradpost's last region; the earlier deletion inventory
above remains a historical control, not the new-tree census.

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

At that checkpoint the seven remaining auto-inline regions had bounded next actions:
`convertVolume`, `checkDimNextHeroBut`, and `~CAnimatedDlg` have no leading
DC gap supporting an assertion; the destructor's sprite is explicitly
optional. `AppCommand` is a four-byte DC platform stub and `stopMouseThread`
is mostly PC-only teardown, so missing PC statements are not assertion proof.
`CNewPlayerUpdateMan::headerRequested` now has its source-proven concrete-proc
call and Complete's changed queue payload; its remaining caller dependency
is measured below.
The empty `CNewPlayerUpdateTask` destructor has the previously measured
untracked derived-destructor expansion, reviewed and removed below. These remain reconstruction debt,
not candidates for invented checks or declarations merely to alter inlining.
The later volume lifetime recovery below retires `convertVolume` without an
assertion or additional operation; the later task-destructor review leaves
five auto-inline regions.

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

### Shared combat helpers and drawing callers

From `e4650642`, the pathfinding pass removes one more depth fence, the
duplicated `markEnemySearched` body, and all three preamble helper extractions
whose source comments explicitly described them as inline-budget experiments.
DC publics establish ordinary private `build_combat_path`, `mark_enemy`, and
`check_enemy_armies` methods; their bodies retain the original source order.
The mark helper has the nested early-return scope at lines 1176-1179.
`checkEnemyArmies` owns its `ValidHex` guard and uses the real owning-side,
creature-flag and second-hex accessors. Its two callers share one mark body.

The 48-state helper family alone initially lowers the fully reconstructed
`FindCombatPath` to 60.0911%. A second 48-state family restores the positively
named `Is`, `get_owning_side`, `OffsetToFront(-1)`, `get_spell_time`, `ValidHex`
and const `get_hex` boundaries. Together these raise the old 87.9780% to
90.2669%, with every other tracked findpath score unchanged, including
`markTeleport` at 100%. The compound mark guard gives 88.6656%; omitting the
geometry accessors gives 84.7692%, and omitting the validity boundary gives
74.6845%. A temporary score dip did not refute the recovered helpers.
The final production object matches the selected family object in all 55
sections, 237 relocation destinations and function locations.

The drawing pass restores `advManager::getCell`'s scalar-invalid/point-valid
helper calls and retires `drawBoatCell` and `drawGroundCell`. Both boat
callers use `game::getBoat` and `getLocation` and improve 98.2619% to 100%;
ground drawing and every other tracked advmgr score are unchanged. Omitting
the location accessor while restoring the cell helper gives 90.8714% in the
boat twins. Restoring all hero/underlay calls as well gives
96.6095%/96.6415%/86.9011% and stops emitting the claimed scalar-cell body.
That remaining `drawHeroCell` copy is explicit recovery debt, not evidence
against DC's ordinary helper or its callers. The selected production object
matches all 308 family-object sections and 4,097 relocation destinations.

`type_point::isValid` is now `bool ... const`, as established by DC's public
`?is_valid@type_point@@QBA_NXZ`; the dossier's unsigned-char rendering does
not override the public's bool encoding. Its ordinary body stays in
findpath.cpp:36, not in a header. The renamed 59-byte body and the complete
pre-pathfinding findpath object are byte-identical under one explicit rename
(52 sections, 235 relocation destinations). Neither const-byte nor const-bool
rescues the remaining hero drawing expansion decision.

Historical generators and contexts:

| Generator suffix (`scripts/experiments/generate-...-family.py`) | Context | Scored states / distinct objects |
|---|---|---|
| `adventure-cell-call` | `76b71ba668f31dcdfd1d` | 32 / 32 |
| `adventure-cell-const` | `2675b9280570a49850e1` | 24 / 24 |
| `adventure-cell-sites` | `bc78bc047916d70eaef7` | 32 / 32 |
| `findpath-helper` | `94c8b2f5a3af5fa41569` | 48 / 27 |
| `findpath-accessor` | `e39aa0ed39b14fca4d84` | 48 / 40 |
| `university-record` | `eb059cf1c1273d8b1223` | 8 / 8 |

Each family reproduced its retained candidates (ten, except seven for the
eight-state university family). The drawing base is `e4650642`; sites runs
after the validity-signature correction. The pathfinding families additionally
include the adopted boat/ground changes. Use the frozen snapshots for exact
reproduction; source anchors intentionally reject the post-adoption tree.

The initial university family retained the then-modelled constructor boundary: a
typed local scores 97.8623% versus 99.7464%; typed local plus `push_back`
scores 95.6522% while the insert fence remains. Removing that fence expands
the insertion heavily and scores zero, not because the function disappears.
Every other tracked game function was unchanged. Those scores did not prove
generic constructor ownership. The later Conflux-initializer evidence below
resolves that mistake without a fabricated inline or no-initialization overload.

The Scholar bitfield widths/positions are readable, but NB11's embedded
type indices in LF_BITFIELD records 0x2f7e-0x2f81 do not resolve to legal
underlying field types in the global table (pointer, argument list, unrelated
structures). They cannot yet prove enum field ownership. Preserve the
retail signed loads and document the unresolved type references instead of
silently changing those fields to enums just to remove two unions.

### Pragma-free pathfinding insertion

`PushCombatPoint`'s remaining depth fence is removed too. The DC call is the
two-argument `vector::insert(begin() + middle, path_cell)`, not its count
overload. VC6 naturally expands that wrapper and retains the count-insert
child while expanding the separate `push_back` arm. This recovers retail's
decisions without controlling the outer statement's inline policy.

Re-reading the actual SH4 argument loads corrects an older source comment:
the stores at lines 1412-1417 are point.x, visited, point.y, direction, cost,
flight_cost. Those stores and the proper insertion boundary improve
98.9844% to 100%, preserving every tracked sibling. The DC midpoint-before-
break loop is retained: `if (last <= first)` is exact, whereas the equivalent
`if (first >= last)` gives 99.7819%. Swapping the midpoint sum's operands is
byte-flat. This is a local comparison-expression effect, not evidence to
discard the recovered loop scope.

Deleting the old fence around count-insert alone gives 87.0685%; with the
recovered stores it gives 88.0841%. Conversely, fencing the proper direct
two-argument insert gives 91.8318%. These negative controls explain why
deletion alone had hidden the removable dependency.

`generate-pushcombat-boundary-family.py`, context `aba7f0923bc87620316b`,
exhausted 64 states (20 distinct objects, ten reproduced elites in each of
two generations). The follow-up `generate-pushcombat-loop-family.py`, context
`34bab0471386d66e7211`, exhausted ten states (three distinct objects, all three
reproduced). The adopted fully recovered loop has its own production
recompile; all 55 sections and 237 relocation destinations match that
selected object. These are historical controls against the frozen input,
before adoption and removal of the duplicate accessor described next.

DC's const `get_hex` header body emits all 32 raw retail bytes at 0x4b3b90,
with no relocations. Its four calls inside the shared `mark_enemy` expansion
corroborate this identity beyond leaf shape. The old `getCellData` spelling
was an NH3API fallback, not a second source helper. It is removed; the
canonical header body is claimed through the owning TU's standard claim-only
carcass annotation. The full build migrates the source-owned name; no
hand-edited generated label or second symbol ledger is involved.

`findpath.cpp` now has **zero inline overrides**. The two removals here do not
establish that the remaining overrides elsewhere are necessary.

### Ordinary shipyard and movement helpers

A fresh deletion audit at `6d71f4c3` exhausts all 241 regions in 34 TUs:
231 isolated removals lose only, four have mixed effects, four improve only,
and two keep tracked scores but change emitted code. The four positive sites
are in `markShipyards`; this is a new lead after the shared map-accessor
recovery, not a repetition of the earlier deletion verdicts.

The matching pass restores `mark_shipyards`, `clear_shipyards`, `RestoreMouse`
and `check_for_town` as ordinary static helpers. DC source boundaries at
150/165/833/896 and retail's expanded copies supply the evidence. The resource
guard exits before the marking loops; the absent-dock and absent-boat guards
continue before the following cell work. `check_for_town` similarly returns
on a negative ID before its two calls. The helpers and all source calls remain
canonical; none needs a forced-inline declaration or any of the seven removed
shipyard depth fences.

| Reviewed family | Successful source states | Distinct code/relocation results |
|---|---:|---:|
| Seven-fence subsets and ordinary shipyard boundaries | 256 | 83 |
| Continue scopes, copied/reference/temporary points, flag type | 97 | 7 |
| Remaining ordinary movement helpers and town early return | 33 | 7 |

The raw highest score, 93.3304%, retains the old forced shipyard enclosure;
89.6719% still forces `checkForTown`. Neither score proves that declaration.
The adopted ordinary four-helper source removes all seven fences and improves
`moveHero` from 86.3549% to 86.6362%. Direct/reference point arguments in that
fully ordinary family score 86.2031%; keeping the copied point value avoids
that loss. No other tracked `philai` score changes. All 126 other emitted
function spans have identical raw bytes, with no added or removed functions.
The adopted production compile also exactly reproduces its chosen candidate's
229 sections and 1,442 relocation destinations, including untracked code.

The three generators are historical pre-adoption controls. Their frozen
contexts are `72462b8f9ffdd5d808be`, `40bf439a08b295f6421e`, and
`30bc95c403479c26cd58`; rerun them against the
`6d71f4c3` source snapshot, not by weakening the now-stale seven-fence anchors.

### Native saved-game vector helpers

DC `load_vector` instances at 0xc19e8/0xc1a68/0xc1ae8 share game.cpp:2698;
the three `save_vector` instances share line 2716. Their mangled declarations
prove native bool results and vector references. The shared readers use a
short count, public `resize(count)`, subscript zero and two guarded reads.
Complete uses `TAbstractFile` in place of the older stream handle. Its retained
writers use an int count slot, write its low two bytes, and sign-extend those
two bytes for the payload length.

The previous implementation pasted six reader expansions into Load and sent
the long-vector gate pairs through a point-vector pointer union. That loses a
real operation: retail zeroes the long fill before resize. The canonical
template restores that initialization and naturally expands all six calls,
without any of their six resize fences. Save uses the same native references;
the pointer union, forced wrapper and duplicate writer implementations are gone.

The helper family exhausts 33 source states / 33 code identities. A follow-up
crosses four reader parents, six writer return forms and all four deletion
subsets of the two existing Save fences: 97 source states / 73 code identities.
The adopted default-argument resize and guarded returns improve Load
78.2645% → 80.1570% and Save 96.5761% → 96.7588%. Named fill locals and direct
read-result expressions reach 82.2663% in Load, but do not displace the positive
DC default-argument and guard evidence. Removing either Save fence scores
90.8331% / 90.9878%, and removing both scores 89.7442%; those existing caller
fences remain debt. No override was added or moved into a helper.

The native bool writers at 0x4d2ac0 / 0x4d2b20 reproduce all 96 retail bytes,
including SETAE. Direct-expression and named-result alternatives emit SBB/INC;
that difference does not refute the bool ABI. Point/long writer, resize, size,
`_Ucopy` and `_Ufill` bodies have identical raw code, supporting the folded
retail identities without changing the native container element types. The
production object also reproduces the selected candidate's entire 822 sections
and 5,288 relocation destinations. This is candidate/adoption identity, not a
claim that Load or its whole TU matches retail.

The disappearing untracked `vector<CampaignScenarioInfo>` destructor was
referenced only by Load's expanded event-read failure cleanup and its unwind
handler. That exit now calls the outer `SavedGameHeader` destructor; it is not
an unrelated function loss. Retail partially expands that outer cleanup but
retains `SCampaign` destruction, so cleanup boundaries still differ. The
template-recovery checkpoint also called the university constructor where retail
passes an uninitialized fill record. The subsequent generic-record/Conflux
ownership correction below resolves that separate class-boundary error.

The two retained claims remain source-owned, inactive declarations of their
actual bool/reference instances. The old size/order label join could not
distinguish three equally sized emitted instances from two retail addresses.
`retail_labels/source.py` now binds this narrow helper family by its declared
stream and vector element types before any weak fallback. Missing, ambiguous,
wrong-ABI and already-taken instances cannot borrow an ICF twin. Eleven focused
tests cover the contract; all 169 retail-label tests pass. Both identity
migrations preserve the original RVAs and exact CUR/MAX/HIST.

The native I/O fixture extracts the actual helper templates and university
definition/initializer. It checks full round trips, empty/growing/shrinking
vectors, short headers/payloads, native long zero-fill and signed-short save
count boundaries. Six intentionally wrong width/count/failure variants fail.
It preserves retail's unsigned read-result comparison, including its negative
result quirk; this is behavioral evidence, not a replacement for the pinned
Dinkumware/VC6 verdict or a portability claim for `&vector[0]` on empty vectors.

Historical generators are `generate-game-vector-helper-family.py` and
`generate-game-vector-return-family.py`, frozen in contexts
`00e01d97ce77f329067a` and `5b633fb2e19799b0a4b3`. Their game.cpp anchors refer
to the pre-adoption `6d71f4c3` source; the independent movement-helper changes
do not alter that TU's inputs. Reproduce those contexts before rebasing a new
family onto the adopted helpers.

### Generic university records and Conflux initialization

The earlier default-constructor attribution at 0x5d2d80 mistook a Conflux-only
operation for generic initialization. Two independent source/retail facts
contradict it. Dreamcast CodeView type 0x1adf is a sixteen-byte struct with
field list 0x3521: its sole member is the four-element `TSecondarySkill`
array `skills`, with no constructor. Complete's Load passes an uninitialized
sixteen-byte fill to an opaque vector resize; automatic four-school stores
could not disappear across that call. Its randomizer also fills the native
record directly, with no elemental initialization.

All three retained calls to 0x5d2d80 are in Conflux branches: two in
`aiEnterTown` (0x5253d0), one in `valueOfTownBuildings` (0x52b1e0).
`townManager::doUniversity` expands the same four stores. The ordinary member
`initializeMagicSkills`, defined at the same townmgr source position and
explicitly called at those four sites, models that role. Its name and historical
helper kind remain provisional; neither DC nor the unused EAX result uniquely
proves a historical spelling. The pointer-return model preserves the observed
ECX input/EAX output and all thirty retail bytes. A void-return negative control
loses `mov eax,ecx` and changes every store operand. No alternate declaration,
special no-initialization constructor, new derived class or copied helper body
is introduced.

The insertion family covers count-insert, single-insert and push_back, raw/native
records, pointer/reference/member receivers and the existing fence. All 36
states compile (24 distinct objects), with ten reproduced retained candidates.
The 13-state ownership follow-up crosses void/pointer initializer results with
the three APIs and fence controls; all 13 distinct objects compile and ten elites
reproduce. The retained count-insert boundary remains debt: unpinning all three
APIs expands the child heavily (0% comparisons). DC's push_back is positive
evidence for the public wrapper; its inline count-insert child explains retail's
named call without proving that the wrapper was absent.

The selected native local removes the union at unchanged 99.7464%. Load rises
80.1570% → 81.2284%; all other common game function bodies remain byte-identical.
The entire philai/townmgr objects agree with their controls under the single
explicit initializer rename; mapcell and initialize agree without a rename.
The adopted five objects reproduce the selected candidate exactly. The source
claim migrates the initializer's existing RVA with 100% CUR/MAX/HIST.

Full shared-header verification also matters: the overall build remains 96.39%
linked / 96.13% whole executable, but two unchanged-source callers move slightly.
`CEnterNameEdit::onKeyPress` is 100% → 99.8868% (two spill slots exchanged), and
`army::doAttack` is 99.9424% → 99.9040% (address-register allocation). Their MAX
and HIST remain intact; the full build reports no source-caused MAX reset.
This is measured collateral, not a loss-free or whole-TU exactness claim.
Both states of the focused header-only control reproduce: restoring only the
old constructor declaration recovers those two scores, with all siblings flat.
Across all 149 raw objects, 141 compare strictly identical (including the two
explicit-rename Conflux objects). Besides Load and the two changed callers,
the remaining five differences are data placement or anonymous header-symbol
identities; all their executable section bytes agree. No untracked executable
body disappears in this ownership change.
The shared-header control and frozen contexts are recorded with the generators
in `docs/vc6/source-families.md`.

The native fixture now tests the actual initializer and generic default
initialization separately. Preseeded object bytes remain untouched by default
construction; explicit initialization sets Fire/Air/Water/Earth and returns the
record address. Wrong-school, wrong-return and automatic-default variants are
all rejected, in addition to the six stream-contract negative controls. Host
value-initialization during native vector tests does not assert the unspecified
bytes of VC6's retail-proven uninitialized fill.

### Ordinary volume helper with read-only setting bindings

`soundManager::convertVolume` at 0x5996c0 keeps its ordinary declaration,
original source order, duplicated setting/range/scale arms, and shared clamps.
Each arm now binds the selected setting as a `const int&` rather than copying
it. This is a supported lifetime hypothesis, not a claimed DC reference local:
the debug inventory records none, while rows 125/136 read the selected
configuration field. No call or write occurs between those reads. No leading
gap or positive inline clue supports manufacturing an assertion.

The 12-state lifetime/guard family exhausts all options, gives five emitted
identities and reproduces all five elites. The selected unfenced-reference
form has the entire old object's section layout, bytes, function positions
and 699 relocation destinations unchanged (110 sections). All four callers
still call the retained exact body: `setMusicVolume`, `modifySample`,
`memorySample`, and the PC `processStopAndPlayMP3` worker. In contrast, simple
deletion with the old value locals expands it at all four named sites; their
scores become 0%, 57.3125%, 81.0864%, and 83.8794%. Nested value guards also
preserve the calls, but do not preserve the whole raw object as the reference
form does. The actual production object independently reproduces the selected
candidate. The arithmetic oracle checks 461,700 valid-input combinations per
optimization level and rejects five faulty selection/range/scale/clamp controls.

This retires soundmgr's last intervention. The current census is **226 inline
overrides** (220 depth-zero, six auto-inline-off) in **32 TUs**, with **64 unions**.

### Post-integration deletion recheck

The fresh `d7f7be28` audit exhausts every single-region deletion and each
whole-TU removal across 226 regions in 32 units. Individual results are
219 loss-only, four mixed, two code-identical and one score-identical with
different emitted code. The last is the already known untracked derived
destructor consequence of removing the `CNewPlayerUpdateTask` fence; score
identity alone does not validate its removal.

Both newly byte-identical regions belong to `game::load`: the creature-bank
`loadObjectVector` call and the final successful return. A separate four-state
family tests unchanged, each individual deletion and their combination; all
four produce one identical game object. Strict comparison confirms all 822
sections, 5287 relocation destinations and function locations unchanged.
Both directives are removed without changing any helper call, C++ statement
or cleanup scope. The intervening `isLocalHuman` fence remains. The new census
is **224 inline overrides** (218 depth-zero, six auto-inline-off) in **32 TUs**.

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
delinking pass. That checkpoint's census is 248 inline overrides and 68 union definitions.

The shared-combat/drawing checkpoint is 4059/4752 exact, 96.29% linked fuzzy
and 96.03% whole-image. The full affected-TU build, retail delinking and all
gates pass, with no source edit lowering MAX. The subsequent insertion
recovery raises the exact count to 4060/4752. That checkpoint's census is 246
inline overrides (239 depth-zero and seven auto-inline-off) and 68 unions.

Integration with main's `265bc0f7` control-flow recovery preserves both sets
of source changes. Its canonical black-market, generator and town-pool
serialization boundaries remove five additional depth fences; see the
[goto audit](goto-audit.md#further-structured-reductions). The combined
full build passes at 4064/4752 exact, 96.39% linked fuzzy and 96.12%
whole-image, with no MAX reset or lost banked RVA. That combined census is
**241 inline overrides** (234 depth-zero, seven auto-inline-off), across 34
TUs, and **68 unions**. The independent controls above retain their exact
pre-integration snapshots rather than presenting those scores as new-tree
measurements.

The ordinary movement-helper checkpoint keeps 4064/4752 exact functions,
96.39% linked fuzzy and 96.12% whole-image. Full retail delinking and all
repository gates pass, with one raised function checkpoint and no MAX reset.
The updated census is **234 inline overrides** (227 depth-zero, seven
auto-inline-off) and **68 unions**.

The native-vector checkpoint keeps **4064/4752 exact functions**, **96.39%
linked fuzzy** and improves whole-image coverage to **96.13%**. No scored
function loses its current score. Full retail delinking and all gates pass,
with two raised checkpoints, two typed claim migrations and no MAX reset.
That checkpoint's census is **228 inline overrides** (221 depth-zero, seven
auto-inline-off) and **67 unions**.

Native marketplace ownership removes another union without changing any of
the five affected objects' section bytes or relocation destinations. Restoring
the DC-proven artifact getters and four ordinary `setupNewTrade` calls also
retires tradpost's final depth fence. Its handler moves 90.6792% to 81.9322%
while every other tradpost score stays fixed; the retained ratio body is exact
and the old handler peak remains in HIST. The missing resource-arm call is a
measured nested-inline budget residual, not a semantic change or a reason to
flatten the helper again. The ordinary `countMarkets` and its recovered
`HasBuilding` query are whole-object byte-neutral. See the
[source families](source-families.md#marketplace-ratio-accessor-and-setup-boundaries)
for complete controls and native contract tests. The full build passes at
4063/4752 exact, 96.39% linked and 96.12% whole-image. That checkpoint's census is
**227 inline overrides** (220 depth-zero, seven auto-inline-off) in 33 TUs and
**65 unions**.

The native full-width object-type read removes one more local union. The
8-state ownership/query family and 9-state field-lifetime follow-up are
exhausted; a separate enum local with the original generic integer buffer
otherwise preserved is adopted. Reading directly into the record is rejected:
it would partially commit a short read. The actual block passes native tests
for reports 0..4 across six valid type values at `-O0` and `-O2`, with five
deliberate faults rejected. The adopted VC6 object reproduces its selected
candidate; all 419 section layouts, 318 function-symbol locations and 2077
relocation destinations agree with the old implementation. Only the target's
18 stack-displacement bytes differ; all other 259 emitted functions are
byte-identical. Full retail delinking and all gates pass at **4062/4752 exact,
96.39% linked and 96.12% whole-image**. The one MAX reset is 100% to 99.9633%,
with HIST held at 100%. That checkpoint's census is **227 inline overrides** and
**64 unions** (38 source, 26 header). The neighboring narrow wire fields are
not candidates for this four-byte read without changing their format contract.

Integration with main's `06f2f4f7` widget, spell-immunity and control-flow
recovery passes the full build at **4063/4752 exact, 96.40% linked and 96.13%
whole-image**, with no further MAX reset or lost banked RVA. The 22 focused
tests cover the native enum/market contracts and the newly integrated spell,
source-family and CodeView behavior. Census and the native read's local
99.9633% residual are unchanged.

The volume reference-binding checkpoint preserves **4063/4752 exact, 96.40%
linked and 96.13% whole-image**. Full retail delinking and all gates pass;
no score changes or MAX resets occur. That checkpoint's census is **226 inline
overrides** (220 depth-zero, six auto-inline-off) in **32 TUs**, and **64 unions**.

The later integration with main's `7e4a9aa8` preserves its Victor, victory
condition and support-TU recovery. Full delinking and all gates pass at
**4073/4764 exact, 96.38% linked and whole-image**, with all 4764 functions
now in 152 linked units and no additional MAX reset. This expanded denominator
is not the older 149-unit linked score. The pragma/union census is unchanged.

The joint Load fence removal preserves **4073/4764 exact and 96.38% linked
and whole-image**. Full delinking and all gates pass, with no changed score,
MAX reset or lost banked RVA. Production independently matches the complete
game control object. The current census is **224 inline overrides** and
**64 unions**; the depth-zero cleanliness bound is ratcheted to **218**.

The text-dialog special-member correction recovers the DC-proven implicit
destructor and resolves CAnimatedDlg's previously wrong named base-cleanup
target without changing any of the twelve header consumers' 1129 scores.
The full checkpoint remains **4073/4764 exact, 96.38% linked and whole-image**,
with no MAX reset, migration or new row. The four-state declaration/fence
control and twelve-state nullable-sprite follow-up show that this correction
alone does not remove the animated-destructor override: the unfenced options
still lose the retained network-copy body and lower the readiness caller.
The census stays **224 overrides and 64 unions**. See the
[source-family controls](source-families.md#compiler-generated-text-dialog-teardown)
for the positive compiler-generated-member evidence and precise residual.

The bank-resource checkpoint retires one further depth override and restores
two canonical source calls: the ordinary player-ID resource-cost overload
forwards to the pointer overload, and valueOfBank calls the ID overload as
Dreamcast proves. The coupled boundary recovery itself preserves the complete
philai object. Removing the shared size fence makes the retained bank exact;
only the first dispatcher expansion now expands size where retail calls it.
The second expansion and final bank call retain their proper boundaries.
The caller's CUR moves 98.0336% to 97.4610%; MAX and HIST stay at 98.0336%.
All other 125 emitted philai bodies remain byte-identical to the control.
Sixty lifetime states and eight boundary controls are exhausted, and the
actual-body native oracle passes both optimization levels and seven deliberate
faults. Full delinking and all gates pass at **4074/4764 exact, 96.38% linked
and whole-image**, with one raised checkpoint, no MAX reset and no lost banked
RVA. The current census is **223 overrides** (217 depth-zero, six auto-inline-off)
and **64 unions**. See the
[bank controls](source-families.md#creature-bank-and-resource-cost-boundaries).

The task-destructor follow-up removes another auto-inline region without
changing any tracked score. Its two reproduced states are not whole-object
identical: the generated Proc destructor grows from a jump thunk to the exact
retained Task teardown, adding 32 padded bytes. All other 395 emitted bodies
and relocation destinations stay fixed; all 737 non-debug sections preserve
their order/attributes and all other bytes. Genuine VC6 linker controls refute
the tempting ICF explanation: the ordinary non-COMDAT Task body remains
separate. No source interface or declaration is changed to force a fold.
The independent 36-state mouse-handle family finds no safe removal and keeps
that override. Full delinking and all gates preserve **4074/4764 exact and
96.38% linked/whole-image**, with no score or history changes. The current
census is **222 overrides** (217 depth-zero, five auto-inline-off) and
**64 unions**. See the
[task and mouse controls](source-families.md#mouse-thread-lifetimes-and-inherited-task-teardown)
for the explicit untracked-code and linker limitations.

The fresh full deletion audit at `c78bb3c8` exhausts all **222 regions across
32 TUs**: 219 loss-only and three mixed. There are no code-neutral, score-neutral
or gain-only deletions. The mixed cases are the already bounded animated-dialog
and mouse-thread helpers, and `readObject`'s seer insertion (a small caller gain
but four exact retained STL bodies disappear). This bounds simple deletion at
that source state, not further source recovery.

That distinction matters in `type_sacrifice_window::updateSlot`: restoring
Dreamcast's `getArtifact` accessor and first ordinary `updateArtifactWidget`
call removes the override embedded in a pasted helper copy. All 55 scores stay
fixed, and the **entire raw object** stays identical: 200 sections, 1678
relocation destinations and every function position. The copied-body deletion
control loses 100% to 99.1368%; the canonical boundary restores retail's nested
`setVisible` call naturally. The six-state family and production both reproduce
the byte-neutral result. That checkpoint has **221 overrides** (216 depth-zero,
five auto-inline-off) and **64 unions**. See the
[sacrifice-slot controls](source-families.md#sacrifice-slot-helper-boundaries).
Full delinking and all gates preserve **4074/4764 exact and 96.38%
linked/whole-image**, with no matching-score or history changes.

The bank-table owner recovery removes the reward-creature adapter: NB11
proves both loader-local tables are mutable `TCreatureType` arrays, and all
66 emitted dwords match retail's writable data. All 95 consumers of the
corrected creature enum are measured. Three RMG rows move slightly while
retaining their unchanged-source MAX/HIST; the other 4114 scores do not move.
Restoring the loader's canonical static reference-taking level reader then
raises it from **89.4550% to 97.5355%**, with all fifteen named calls and
twelve branches matching retail. Production reproduces the chosen object's
28 raw sections and 100 relocation destinations; the native two-body oracle
and table-owner verifier pass with their negative controls. See the
[bank ownership and reader controls](source-families.md#creature-bank-table-owners-and-ordinary-level-reader).
Full build passes at **4073/4764 exact, 96.39% linked fuzzy and 96.38%
whole-image**, with no MAX reset or lost banked RVA. The current census is
**221 overrides** (216 depth-zero, five auto-inline-off) and **63 unions**
(37 source, 26 header): 24 intentional representations and 39 reconstruction
adapters.

The fresh deletion audit at `3423ece1` checks all **221 regions across 31 TUs**
after the bank-header and sacrifice-slot recoveries. Every isolated compile
finishes: **218 loss-only and three mixed**, with no neutral or gain-only
deletion. The mixed cases are `readObject`'s seer insertion (97.4369% →
97.5099%, but four exact retained STL bodies disappear), animated-dialog
teardown (network copy 100% → missing and wait caller 90.6522% → 75.0683%,
while its implicit destructor rises 86.3333% → 100%), and the mouse helper
(setup 100% → 90.1470%, generation 92.6386% → 97.9759%). This audit bounds
simple deletion at that snapshot, not subsequent source recovery.

The skill-quest proposal's **36-state input family, exhaustive 64-subset fence
family and 48-state string-lifetime follow-up** also find no removable fence.
All nonempty deletion subsets lower the caller while leaving its 118 siblings
fixed. The native signed-byte binding and real const-reference string lifetimes
do not recover a deletion. See the
[proposal controls](source-families.md#skill-quest-proposal-lifetimes-and-six-fence-boundary)
for the signed-loop residual and shared-COMDAT qualification.

Native AI combat ownership retires all **three ai_combat overrides**: the
general-melee call replaces a fenced pasted copy, and one correct ordinary
mass-damage helper replaces two fenced caller-specific versions. The native
vector copy/size claims now have their actual STL owners; `getTotal` returns
combat value instead of a falsely identified vector length. The ordinary
Familiar predicate, const valuation interfaces and canonical value-returning
min/max are restored too. See the
[container/melee controls](source-families.md#ai-combat-container-ownership-and-canonical-melee)
and [mass/value controls](source-families.md#ai-mass-damage-familiar-predicate-and-value-wrapper-boundaries).

chooseMelee, doGeneralMelee and two-side getEnchantmentValue become exact.
getResurrectionValue's initial min-wrapper dip is recovered to 100% with a
named capped value. Real residuals remain: initializeCreatures is 83.4275%
against its old 91.9548% HIST, its retained `_Unguarded_partition` body is
currently absent, and corrected/unpinned castSpell is 86.7910% against the
incorrect-dataflow version's 93.0273% HIST. No body/claim disappearance is
hidden by the banked-RVA gate. The seer signed-loop correction costs one
additional small MAX reset, 83.2252% → 82.9730%; its old HIST is retained.

Full delinking/build passes at **4075/4764 exact and 96.38% linked/whole-image**.
The updated inventory has **218 overrides across 30 TUs** (213 depth-zero,
five auto-inline-off), **63 unions**, and no inline override left in ai_combat.
Relative to the user's 253/72 checkpoint, this removes **35 overrides and
nine unions**. The remaining union classifications are unchanged.

The post-AI deletion audit at `a6ac8f7e` exhausts all **218 regions across
30 TUs**, plus each TU's all-removed control. Individual deletions yield
**215 loss-only and three mixed** results; none is code-neutral, score-neutral
or gain-only. The three mixed cases remain seer insertion, animated-dialog
teardown and the mouse helper, with the same affected scores described above.
The all-removed controls are 28 loss-only and two mixed. These isolated,
reproduced controls describe the pre-RMG snapshot, not a rerun after integration.

Integrating main's `b4de5f57` RMG recovery passes full retail delinking and all
build gates at **4081/4764 exact and 96.42% linked/whole-image**, with no new
MAX reset, checkpoint change or lost banked RVA. The sole merge conflict was
the generated README score table, regenerated by this combined build. Census
remains **218 inline overrides and 63 unions**; the RMG gains are preserved
concurrent work, not attributed to this override reduction.

The tactical mass/summon recovery removes the final ai_tactical depth override
and its forced-inline mass declaration. Restoring the summon mastery accessor
preserves all 347 scores across five consumers without that fence. Naming the
group loop's target then raises **considerSpell from 98.1927% to 100%**, with
all other 91 tactical scores fixed. The 24-state boundary and 16-state lifetime
families, strict raw-object comparisons and actual-body native tests are
documented in the [tactical controls](source-families.md#tactical-masssummon-boundaries-and-exact-spell-dispatch).
The current census is **217 overrides across 29 TUs** (212 depth-zero, five
auto-inline-off), **63 unions**, and **434 source inline-pragma directives**.
The 22 header packing directives remain separate layout contracts.
Against the user's **253 overrides / 72 unions**, this removes **36 overrides
and nine unions**; auto-inline-off regions fall from seven to five.

The final tactical checkpoint passes full retail delinking and all gates at
**4082/4764 exact and 96.42% linked/whole-image**. One checkpoint rises to 100%;
no source edit lowers MAX and no banked RVA is lost. This is a verified stopping
checkpoint requested by the user, not evidence that further improvements have
been exhausted.

This audit does not claim TU closure, all remaining unions as original source,
or all inline debt solved. The remaining reconstruction classes above identify
what must be recovered; they are not permissions to add new suppression pins.
