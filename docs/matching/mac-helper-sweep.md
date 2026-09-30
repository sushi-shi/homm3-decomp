# Mac retained-helper sweep

The current pass restores source helper boundaries and calls across all Windows
game functions, including byte-exact callers. A lower byte score does not reject
a helper supported by the available source and compiler evidence. Byte polishing
follows the broad pass.

Use clear project names when Mac shows an operation but not its original C++
spelling. Restore the helper and its call sites promptly, including when Mac
inlines the operation too; an original symbol or immediate score gain is not
a prerequisite. Keep one canonical body and refine names or placement later if
new evidence warrants it.

During subsequent byte recovery, keep these helper calls. Do not replace them
with direct fields, array indexing, or pasted statements to recover a score.
An outer helper is permitted when it contains the recovered helper call; review
that complete source path. Tune natural argument evaluation, local lifetimes,
body visibility and source order around the preserved calls.

Use `homm3 mac calls <unit>` and `homm3 mac disasm <Windows-VA>` on claimed
callers. Direct Mac branches resolve to source claims and runtime labels; an
unnamed destination needs an identity review. These are leads for human
review, not proof of an inline qualifier or even a game helper. A Mac inlined
operation may have no retained call and therefore needs a body-shape review
too. The current whole-corpus request supersedes the earlier module deferrals:
RMG is included, and library/runtime/generated destinations must be identified
as such rather than silently dropped from the call inventory. Keep vendor code
pristine; identifying a library destination does not require recreating it as
a game helper.

For each caller, inspect Mac disassembly and direct targets; use Dreamcast
source facts when they are already available. Restore one canonical helper body
and source call in its plausible owning TU/header. The name may be ours when
Mac shows the operation but does not preserve a source symbol. A field load,
array index, or sequence of stores in Mac can be an expanded helper. Do not
remove an authored helper call solely because the PEF has no retained call,
including in Windows functions already at 100%. Check Mac body order
separately from cross-TU VC6 expansion when deciding header placement.
Record unresolved destinations instead of silently counting them as absent
helpers. Batch the helper edits without per-function score tuning or
per-helper builds; compilation and byte comparisons follow the broad
restoration pass.

## Whole-corpus review

The working inventory covers 139 game translation units, 5,549 canonical source
definitions, 9,684 DC procedure records and 6,921 known Mac function spans.
Native call destinations without a recorded extent are also retained as leads.
These are coverage totals, **not a claim that every semantic review is closed**.
The inventory includes exact Windows functions and source functions without a
DC/Mac address. Source annotations remain the function-name authority.

The generated working files are under `build/all-function-calls/`:
`function-inventory.tsv` contains every canonical source function;
`native-source-edges.json` records individual native calls and source identities;
`review-with-paths.json` retains possible nested and implicit lifetime paths.
Clang diagnostics, unresolved indirect calls, unpaired targets, and ambiguous
template instances are coverage gaps, not evidence of absent source calls.
In particular, a reachable nested helper is a review candidate: an unrelated
call elsewhere in the caller does not discharge the native operation.

### Recovered structure

* RMG now uses canonical helpers for line/terrain neighbour masks, frame
  selection, first-object lookup, guard valuation, treasure replacement and
  disposal, occupied-cell counting, per-zone distance initialization, bounds
  accumulation, map-cell object removal and active-zone counting. Native
  offsets and caller evidence accompany the owning bodies.
* Mac's reward constructors derive treasure value internally. Experience boxes
  take one reward argument and compute `experience * 12 / 10`; quest rewards
  compute `2 * (reward - 2000) / 3`. The roster no longer supplies independently
  precomputed values. Prison constructor argument order and the intermediate
  dwelling constructor's four-argument interface are restored too.
* Multiplayer IPX setup calls the shared member `initRemote`; TCP setup calls
  `refreshSessions`. Both use the ordinary existing member definitions.
* Resource disposal, cache hooks, text access, bitmap access, timing, network
  setup, widget visibility and town-name operations again use their existing
  helpers where native evidence supports those calls. This includes exact
  callers; a zero byte-score change is not a reason to leave pasted logic.
* `TSeerData` inherits the five-byte `TQuestGuard` prefix. Mac's seer constructor
  calls that base constructor and then performs its additional field writes;
  the packed Windows layout is preserved.
* `CDPlayGroup` inherits `CDPlayPlayer` and delegates construction to it.
  DC class records 0x3d83/0x5447 prove the public base at offset zero, and
  SH4 0x8be7a calls its constructor. The former duplicate name/DPID fields
  hid that relationship. The 0x104-byte Windows layout and VC6 scores hold.
* Bitmap and sprite drawing again use their bitmap-taking overloads in window
  backgrounds, fades, borders, buttons, pointer loading, radar/puzzle drawing,
  combat animation, campaign subtitles, credits, RMG progress and text scrolling.
  This restores 37 calls whose map/width/height/pitch forwarding had been pasted
  into callers. DC retains the wrappers; Mac retains the raw calls with those
  same forwarding expansions. The DirectDraw locked-surface path still supplies
  its actual surface pointer, dimensions and pitch.
* Adventure redraw, screen update and combat victory again call the five
  DC-proven mouse enable/disable hooks. Complete's release helpers only return
  the counter; discarded results disappear under VC6, so these calls are
  byte-neutral.
* Seer log, rollover and quick-info text builders call `TSeerHut::getName`
  instead of repeating the name-vector lookup. DC retains that call in the
  older callers; Mac expands it inside the Complete text builders. This
  restores the complete nested path without changing Windows MAX scores.
* Combat mouse highlighting calls `validHex` and `inInvisibleColumn` again.
  DC retains both calls; Mac expands both, including the nested bounds check.
  `army::getMirrorEffect` calls `getSpellTime` and `cppMax` instead of spelling
  out their field access and reference selection. Complete moved the older
  spell-duration check into that helper. All four calls preserve Windows MAX.
* Army spell-state reads now use `getSpellTime` in 25 ordinary methods and
  three shared predicates, including damage, luck/morale, animation and spell
  expiration. This restores 44 duration reads and one `getSpellLevel` read;
  mutation stays in the owning lifecycle methods. Mac retains the indexed
  reads at army+0x198+4*spell and expands the same accessors inside controlling
  side, incapacitation and retaliation. The header records representative
  native sites. Recognizable expansions need review even when no native call
  survives.
* Eight network-message constructors default-construct their `type_point`
  member before assigning payload fields, as the DC calls show. Explicit
  point copy initialization had removed that lifetime. Restore the original
  construction followed by the observed assignment order.
* Mac's river/road painter destructors identify the existing line-painter base
  destructors. Their source definitions now carry the corresponding Mac claims;
  the C++ base destruction paths already represented those calls.

### Call-extraction correction

The DC pass exposed stale-register call labels in the SH4 analysis tooling.
It now invalidates literal targets after register writes and after a call's
delay slot, preserves saved registers, and follows register copies. A load
through a global pointer no longer labels the subsequent indirect call as a
call to that global. The assembly renderer uses the same analysis.

Regenerating all 9,684 DC procedure records changed 1,014 unsupported target
claims to unresolved indirect calls; no named game-procedure target changed.
Most were import-slot addresses, which are also data rather than direct code
targets. Keep these unresolved instead of counting them as reviewed callees.
Pointer-slot provenance is reported separately from a direct target, including
in rendered assembly. The working Mac join also preserves the runtime map's
`indirect_tvector` classification: 2,750 calls through `__ptr_glue` have unknown
dynamic destinations. The stub's runtime identity does not discharge the game
operation it invokes. These are explicit review gaps. The
Dreamcast/structure/source-facts tests pass (60 tests, eight skipped).

### Identity corrections

DC `game::get_alignment(int)` selects a **player's** setup alignment and handles
a negative player index. The separately retained Windows creature-alignment
lookup is now named `getCreatureAlignment`; it no longer claims that DC body.
The setup array and network message pointer use the symbol-proven `TTownType`.

DC `ExtraInfoUnion::getCustomIndex` extracts the 12-bit resource/treasure index.
Its annotation had been attached to an 8-bit monster accessor. Treasure lookup
now calls the correct helper, and the distinct monster accessor retains its
8-bit semantics. Town-manager swap/garrison identities and the two freelancers'
guild overloads likewise carry their corrected DC pairings.

### Validation limits

VC6 checks distinguish semantic restoration from byte-score movement. Some
restorations improve matching, some are byte-neutral, and some alter inlining.
The RMG roster's recovered constructor interfaces currently lower its Windows
score from 97.22% to 96.25%; the previous peak remains in HIST. Keep the source
interfaces and investigate natural compiler context during byte recovery.

The shared `hasBuilding` to `getBuildingMask` path changes PPC allocation in
20 scored callers. Their annotations document the one-checkpoint abstraction
exception required by the Mac preservation gate. This does not waive later
regressions or turn an unresolved call identity into a verified pairing.

The bitmap-overload batch makes `viewPuzzle` exact and improves
`fizzleForwardX` to 99.94%. It lowers `creditsWait` from 100% to 98.28% and
`shootBallisticMissile` from 94.19% to 91.24%; HIST retains their previous
peaks. Three Mac callers (`updateProgressBar`, `saveFizzleSourceX`, and
`fadeToBlack`) carry reviewed abstraction exceptions for the same forwarding
operations. Their score changes do not justify pasting the wrappers back into
the callers.

The full checkpoint passes source ownership (5,549 definitions, no violations),
source inventory, layout/order and cleanliness gates, and links with zero
unresolved externals. Windows records 4,448/4,814 exact functions and 98.21%
weighted MAX. The Mac gate scores 1,527 pairs with no unreviewed regression;
1,741 emitted pairs remain unavailable because of unresolved references, and
83 Windows-VA claims have no full-TU Mac body. These comparison gaps are not
call-review completion. This checkpoint includes the bitmap wrappers,
DirectPlay base constructor, mouse hooks, seer-name and combat accessors,
spell-duration accessors and the eight network point lifetimes;
598 scored Mac pairs are exact.

The spell-accessor batch keeps the supported calls through Windows score dips:
`getUnitCombatValue` moves from 100% to 93.42% and `resetRound` from 100% to
97.40%. Their previous peaks remain in HIST. Ten Mac callers carry the narrowly
scoped abstraction checkpoint for nested `isIncapacitated -> getSpellTime`
reads, with each native expansion site recorded on its `MAC_ADDRESS` line.

The constructor review verifies default member/base initialization separately
from explicit initializer lists. In particular, a member assignment inside a
constructor does not replace its earlier construction, while explicit point
copy initialization must not be counted as a default-constructor call. The
working review records 183 existing native constructor sites under that rule,
separately from the eight restored point-construction sites.

Manual inspection also verifies 64 terrain-cache calls through `getTerrain`
or `getFrame`, `getPackedCell`, and `initializePackedCell`. The operation at
each site is already represented; the working notes distinguish terrain
queries, frame queries and the two whole-tile reads. Those reviewed paths do
not discharge unrelated calls in the same caller.

The creature-name pass restores eleven `getArmyName` calls: seven army-command
messages, the recruitment title, both monster-quest text methods and the
campaign creature bonus. Dreamcast retains the seven town calls and the
recruitment call; Mac expands the same range guard and singular/plural lookup
in all eleven operations. The full checkpoint preserves Windows scores and
improves one Mac pair to exact. An unrelated path from `selectArmy` to
`getArmyName` did not represent the town messages; nested reachability alone
must not close a call-site review.

A further 57 inspected lifetime/copy sites and 33 movement/terrain-predicate
sites are already represented. The notes identify the particular member,
base, local or predicate at each address. For example, campaign previews
construct their own setup member, selection messages construct header members
before assigning them, and the water-walking artifact query belongs to
`canWalkOnWater`, not an arbitrary reachable `isFlying` call. These are
call-site dispositions, not claims that the enclosing functions have completed
the whole-source review.
