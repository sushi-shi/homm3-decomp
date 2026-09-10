# Artificial address arithmetic review

Reviewed 2026-09-10 from `182b7a26`, in the isolated
`codex/address-arithmetic-20260910` worktree. The inspiration was the local
King's Field project's `docs/patterns/open-address-arithmetic-review.md`:
distinguish meaningful address calculations from expressions that merely
reconstruct an already-known address or imitate an optimizer's induction
variables. Pointer arithmetic is not itself reconstruction debt.

This report records the arithmetic pass's checkpoint. The subsequent
[owner-pointer review](owner-pointer-audit.md) adds complete SDK owners and
palette/diff payload fixes, records the remaining retail pointer exceptions,
and supplies the newer aggregate match and cast-count result.

This pass removes the identified artificial field offsets, duplicate flat
creature-record view, cross-member army reads, cross-row scalar walks, and
diff-header base recovery. It does **not** certify buffer bounds, object
lifetimes, all casts, or all pointer provenance; those remain separate README
review categories. It adds no inline pins, compiler flags, surrogate layouts,
dummy operations, or alternative helper declarations.

## Scope and evidence

The lexical review covers all **352 tracked project C/C++ files** in `src/`
and `include/`. Comments, literals, preprocessor directives and literal
`#if 0` carcasses were masked with `homm3.vc6._source.mask`; vendor files and
generated build copies were excluded. Nonliteral preprocessor conditions are
conservatively retained by that masker, not evaluated as a full preprocessor.
Macro definitions were separately inspected for address calculations; the
extra `.cxx` file (`DC_input.cxx`) contains only Dreamcast stub declarations.

A supplementary Clang type-based census parsed the 138 project TU entries in
`compile_commands.json`. Its initial deduplicated navigation inventory had
**1,004 candidate sites**: 434 pointer arithmetic expressions, 555 pointer
increments/decrements and 15 subscripts containing casts. These are candidate
counts, **not defect counts or an exhaustive inventory of address expressions**.
In particular, scalar indexing through an already-erased `int*` needed the
separate lexical/member-layout review to expose the creature-record view.

125 TUs parsed without errors. The remaining 13 were also examined lexically:
`hiscore`, `game`, `townmgr`, `bitmap16`, `bitmap24`, `customcampaign`,
`drawing`, `palette`, `army`, `singleselectionwindow`, `sacrifice_window`,
`rmg`, and `forcefeedback`. The Clang limitations include old for-loop scopes,
VC6 library declarations and missing SDK/header spellings; they are not VC6
build failures. No source was changed to accommodate this navigation parser.

Review signals included byte-pointer casts and nonzero field displacements,
manual record strides, first-element pointers to multidimensional arrays,
negative/cross-member subscripts, integer address carriers, pointer-difference
expressions, and cancellation-like buffer calculations. The source and owning
declarations were inspected to distinguish these from pixels, serialized data,
flat allocated tables and iterators.

The scratch census scripts and outputs remain under `build/` in the isolated
worktree: `audit_address_arithmetic.py`, `audit_lexical_address.py`,
`address-census-all.json` and `address-lexical.txt`. They are navigation aids,
not new build gates or a second symbol ledger. Reusable review searches include:

```sh
rg -n 'static_cast<[^>]*\*|\b(?:uintptr_t|intptr_t|ptrdiff_t)\b' src include
rg -n '&[^;]*\[0\]\[0\]|\b(?:stride|offset|delta|base)\b' src include
rg -n '\[[^]]*(ARMY_GROUP_SLOT_COUNT|CREATURE_RECORD)[^]]*\]' src include
```

These broad searches include comments and require source review/masking; zero
hits in a particular search never proves absence of this semantic debt.

Dreamcast dossiers, block assembly and inline clues were consulted for the
changed game functions. Non-exact counterparts received the required retail
summary/structure/source pass before rewriting. Candidate `/Z7` labels were
used only to locate candidate statements, not as recovered retail source.
All adopted edits were checked with the per-TU VC6 SP3 profiles and then a
full `homm3 build`, including regenerated retail targets and shared-header
collateral. The relevant source comments retain the specific evidence.

## Adopted changes

| Site | Replacement and evidence | CUR before → after |
| --- | --- | --- |
| `combatManager::resetLimitCreature`, `0x493710` | Seven raw offset accesses become the existing effect members and `m_drawbridgeBounds`; remove the five-offset enum. DC drawing.cpp:468–478 and retail member offsets agree. | 100 → 100 |
| `checkShipyardSquare`, `0x5c0c90` | Remove `cellFlagsWord` and its fabricated `unsigned short*` index 6; use the existing `m_cellFlags` union member at +0xc. No new union or layout. | 100 → 100 |
| `getUpgradeCost`, `0x54e750`; `getMonsterCost`, `0x54e7c0` | Use `g_creatureTypeTraits[creature].m_cost`. The canonical 116-byte record owns the seven costs at +0x20. | 100 → 100, both |
| AI war-factory, town-entry and siege purchase, `0x524fc0`, `0x5253d0`, `0x525ca0` | Use the same canonical cost member in the original helpers/callers. Together with recruitment, remove seven raw record accesses, `g_creatureRecords`, and the two dword-stride constants. | 100 → 100, all three |
| `town::initializeBuildingCostsTables`, `0x5c14c0` | Index all dimensions of the special, neutral and dwelling tables. DC `0x168c3c` supplies signed indexed loops; retail has nine towns rather than DC's eight. | 91.0465 → **100** |
| `town::initializeHordes`, `0x5bdf60` | Select `s_constHordeEffects[townType][entry]` within its four-entry row. Restore DC's short town/even-entry indices, creature snapshot and upgraded-entry pointer; retain the proven creature/dwelling/bonus store order. | 100 → 78 → **100**, CUR/MAX/HIST 100 |
| `TBottomViewTown` constructor, `0x4521f0` | Index the two-dimensional coordinate table by occupied display slot; skip empty army slots without advancing display position. DC lines 452/457/476 distinguish the two indices. | 94.0054 → 94.8000 |
| `TQuickHeroWindow` constructor, `0x52ead0` | Index the coordinate rows and both named army arrays; remove the negative read from `m_numTroops` into `m_armies`. DC lines 144/159/163 support these accesses. | 94.1662 → 94.1662 |
| `TQuickTownWindow::initializeArmyDisplay`, `0x530990` | Index coordinate rows and `m_armies`/`m_numTroops` separately; remove the `currentArmy[7]` cross-member read. DC lines 187/192/202/215/248 preserve packed display positions. | 100 → 100 |
| `CDiffFile::getBase`, expanded in `CDiffMaker::makeDiff`, `0x491140` | Return the byte representation of `this` instead of subtracting the size word from `m_data`. Keep the canonical inline helper and its calls. | 83.9244 → 83.9244 |
| `CDiffHeader::getData` | Express the payload boundary as the byte address after the header (`this + 1`), not the end of its padding member. The 12-byte serialized header is retail-proven. This accessor has no emitted caller in the current tree. | No emitted body |

The indexed building-cost loops are a useful compiler finding: VC6 performs
the bias and strength reduction itself, including the signed address bound.
Earlier attempts that indexed only the outer dimension did not establish
that the fully indexed source would fail.

The horde initializer also reaches 100% without restoring its cross-row
cursor. DC's town and even-entry backedges both sign-extend a **short**;
using `int` for the town index had let VC6 eliminate retail's stack-homed
countdown and frame. Restoring the evidenced type produces the countdown
naturally. Creature snapshots and the separate upgrade pointer preserve
DC rows 946/949 and are independently score-neutral.

`scripts/experiments/generate-horde-row-family.py` exhausts 24 ordinary
row-traversal/type/value-binding forms in context `5bf7482ca2df1b6da9dd`:
24 successfully scored states, 12 distinct search identities and ten
reproduced elites. Indexed forms score 78% with an int town index and 100%
with a short; within-row cursor forms score 81.4222% / 88.0667%, respectively.
All 53 sibling scores stay unchanged throughout the family. The selected
DC-backed even-entry form (`2fbc63c12ecdd8a6214dc549`) was additionally
recompiled independently because it was not an elite representative.
Production reproduces its **159 raw sections, 947 relocation destinations
and function locations**. All eleven retail CFG blocks and instruction rows
agree. The three displayed data-name differences are existing unclaimed
references, not changed scoring rules.

`scripts/experiments/test-horde-row-family.py` checks the actual record
declaration and function body at native `-O0` and `-O2`, with 486 cases per
form across both pairs and all nine towns. It covers matches, misses,
duplicate dwelling IDs, bonuses and unchanged dwelling storage; four
deliberately wrong controls are rejected. All 24 trial forms and the adopted
body pass. This bounded oracle excludes a final-town slot-seven match, whose
existing upgrade lookup would exceed the flat dwelling table. It does not
claim general bounds safety or native-host proof of the VC6 ABI.

There are two fewer written named casts overall: six removed from raw member
accesses and four added at the two explicit byte-representation boundaries
in `diff.h`. The existing README cast inventory is adjusted by that delta;
this pass does not replace the separate cast review.

## Retained arithmetic

| Family | Disposition |
| --- | --- |
| Bitmap, sprite, font, Bink/Smacker and Victor pixel paths | Byte pitch, bytes-per-pixel, row orientation, encoded line/cell offsets and RLE runs are real layout/format operations. A typed pixel pointer cannot replace byte pitch without a unit conversion. Keep the existing canonical bitmap interfaces. |
| Network messages, resource definitions and binary diff records | `sizeof(header)` skips a serialized header; byte offsets and encoded lengths navigate variable payloads. Removing this arithmetic would change the format, not simplify an artificial member view. |
| `TGzInflateBuf::underflow`, `0x4d6920` | `next_out + avail_out - 512` recovers the beginning of the live **z_stream output window**. Retail +0x9b..+0xb3 explicitly loads those two stream fields and calculates that address before `crc32`. Substitution with the separately stored `m_outBuffer` would remove a real stream-state dependency. This is not the King's Field dead base-plus-difference artifact. Keep it, with an evidence comment. |
| Flat allocated maps and tables | `x + y * width + z * width * height`, flat puzzle-coordinate blocks, and the five-string quest groups are offsets in declared flat storage. Quest groups remain inside the selected 52-string row. No member type is bypassed. |
| Containers and arrays | End pointers, iterator differences, vector insertion positions, neighboring elements, and paired sentinel spell tables retain their genuine element-count semantics. This review is not a blanket replacement of pointer iteration with indexing. |
| Palette bytes and packed text | RGB byte streams, packed strings and text edits operate on byte-oriented interfaces; their strides and lengths describe the format rather than a fabricated game-record view. |

No newly identified artificial-address site is parked unfixed. The retained
families above are deliberate classifications, not claims that all their
buffer bounds or provenance have been audited.

## Final checkpoint

The full build passes with **4,084 / 4,764 exact functions across 152 units**,
one more exact function than the fresh starting checkpoint. Normalized aggregate
similarity improves from **96.426770% to 96.429930%**. The executable-wide
display is now **96.43%**.

Only two current function scores change from the starting checkpoint:
building-cost tables and the bottom-town constructor improve.
All other current scores are unchanged, including shared-header consumers.
The horde initializer's generated CUR, MAX and HIST are all 100 after the
source-backed restoration; its temporary 78% score is not left as MAX debt.

Banked-RVA retention, VA claims, single-view, cleanliness and status invariant
gates pass. `vendor/`, compiler profiles and target inventories are unchanged.
The only generated tracked change is `config/match_baseline.tsv`; it was
updated by the build, not edited by hand. No executable gameplay test was
performed: the validation here is source/layout evidence plus the pinned
compiler, retail comparison and repository gates.
