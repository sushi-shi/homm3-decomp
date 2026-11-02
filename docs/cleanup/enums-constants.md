# Enum and constant cleanup

This review follows the HoMM1/HoMM2 constant-reuse pass: values are merged
only when their producers and consumers represent the same quantity. Equal
integers alone do not establish one domain. Native spellings and the Windows
ABI remain authoritative. This is a scoped review, not a claim that every
HoMM3 domain or literal has been audited.

## Reviewed domains

| Domain | Decision | Evidence and disposition |
| --- | --- | --- |
| `include/artifact.h`'s `ArtifactClass*` | Canonical | `TArtifactTraits::m_class` stores the ARTRAITS.TXT `S/T/N/J/R` masks. The parser (`0x0044cd50`, DC helper `0x050058`) writes these masks; the sacrifice offering (`0x0055fc30`) reads the same field; `game::getRandomArtifactId` intersects that field with its class-mask argument. Preserve the existing names, including Loki-proven `ArtifactClassSpecial`. |
| `ESacrificeArtifactClass` | Reuse | Its four members duplicate the treasure/minor/major/relic artifact masks. Retire the enum and use `ArtifactClassTreasure/Minor/Major/Relic` in the offering switch. This is a shared trait encoding, not a window-local classification. |
| Creature-bank level count | Retain separately | The retail constructor at `0x0047aad0` walks four `0x60` level records inside a `0x190` traits record. Name this array extent and use it for the loader's level countdown and the random selection's final-level fallback. Four levels are unrelated to four possible guard types. |
| Creature-bank guard-type table width | Retain separately | Each loader row contains up to four creature types followed by `CREATURE_NONE`, hence five entries. This is not the seven-slot `armyGroup` capacity or the number of generated guard stacks. |
| `type_creature_bank_guard_shape` | Retain | Its 1/2/3 labels dispatch on the number of occupied army stacks. They are not creature IDs, level ordinals, or resource IDs even where the integer values coincide. |

## Source changes

- Remove four redundant sacrifice-window class enumerators and reuse the
  canonical artifact classes in the four switch arms.
- Replace five numeric class assignments in `InitializeArtifactTraits` with
  the corresponding existing names. Preserve the `R/J/N/T/S` branch order,
  the integer `m_class` field and the helper's signature.
- Replace the four creature-bank reward masks with the same canonical names.
  Preserve `getRandomArtifactId(int)` and its ability to accept combined masks.
- Give the four-level bank extent one owner and couple its two traversal
  bounds to that declaration. Give the four guard-type spreadsheet entries
  one local count; use that count plus the sentinel for the lookup table and
  its copy loop. Keep arithmetic identities and explicit split recipes as
  numbers where they express quantities rather than domain IDs.

## Remaining leads

`src/game.cpp` still passes raw class masks to `getRandomArtifactId`: single
classes 2/4/8/16 and combined masks 6/14. The consumer proves these are
artifact-class masks. A later caller-owned pass can replace single classes
and express the combinations with `|`; it must also review the random level
producer before replacing a variable mask. Other callers should be found by
the call identity, not by a global search-and-replace of those integers.

The source's canonical creature sentinel `CREATURE_NONE` and army group's
legacy `m_armies` integer view merit a separate typed-boundary review. This
pass does not change that union representation or collapse absence markers
from unrelated domains.

`homm3 constants --name ArtifactClass` also identifies
`g_rmgQuestArtifactClass = 2` in `src/rmg.cpp`. Both uses mask
`TArtifactTraits::m_class` in `placeSeerHutForArtifact`; the adjacent retail
evidence (`0x0054b4db`, `0x0054b536`) explicitly identifies the treasure
class. This is an actionable reuse of `ArtifactClassTreasure`, deferred to
the RMG owner to avoid overlapping concurrent work. Conversely,
`g_allRandomArtifactClasses = 0x1e` is the combined random-eligible mask;
its role remains distinct even when its initializer is later expressed as
an OR of the four ordinary artifact classes.

The bank loader's minimum spreadsheet row count (`13`) is retained. Its
four-level traversal does not establish a simple relationship between that
guard and `CREATURE_BANK_COUNT`; replacing it with an attractive arithmetic
formula would assert an unproven file-format invariant. The splitter's `5`,
`2+3`, and `2+2` quantities are explicit distribution recipes, not the guard
table's capacity. The `random(1, 100)` endpoints express the percentage roll
used by chance fields. Spreadsheet column offsets remain local format
positions; this pass has not reviewed all of them.

## Existing enum branch

`cleanup/strict-enums` at `388baae83` is separate from this review's starting
commit `eb0115861`. It contains strict-enum audit tooling plus primary and
secondary skill, mastery, spell-school, artifact-slot, town, terrain, ground
shape, road/river, hero-class and resource-type changes. Its resource type
commit reports Dreamcast spellings and unchanged VC6 function code in all
three images. These are recorded claims from that branch, not validation of
this pass. The associated worktree also contains unfinished edits; do not
copy its working tree or describe those changes as landed here.

## Validation

`homm3 build --fast artifact creature_bank sacrifice_window` passed with the
pinned VC6 profiles after the SSD worktree's objects and retail targets were
prepared. All three units reported **no MAX change**:

- `creature_bank`: its three compared functions remain 100%.
- `sacrifice_window`: all 55 compared functions remain at MAX 100%; the
  changed offering helper at `0x0055fc30` also measures CUR 100%.
- `artifact`: nine of ten functions remain at MAX 100%. The existing
  `InitializeArtifactTraitsTable` residual at `0x0044cd50` remains
  MAX 90.2020% (CUR 90.20198%, HIST 91.8812%); no new mismatch is claimed as
  solved by this naming cleanup.

The command log is `build/enum-cleanup-build.log` (ignored). The constant
index (`homm3 constants --name ArtifactClass`) confirms the shared five
class masks and exposes the two follow-up constants discussed above.
The parser's Dreamcast line layout was inspected with
`homm3 dreamcast lines dc:0x050058`; no expression, branch, declaration
lifetime or helper boundary was changed to imitate that layout.

`homm3 status update --write-readme` banked the measurements with no MAX
reset or lost historical peak. The unchanged-source `createArtifactWidgets`
row moved from CUR 100% to 99.5747%; its source hash, MAX 100% and HIST 100%
are unchanged. This is recorded TU/compiler-state noise, not a reason to
introduce source changes to steer that row back.

These are game comparisons. Shared-source editor checks belong to the
coordinator's integration gate; this lane does not claim independently
verified editor or Mac scores.
