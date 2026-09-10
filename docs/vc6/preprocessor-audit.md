# Preprocessor audit

The 2026-09-10 review of the reconstruction-debt checklist started at
`182b7a26`. All **36 definitions in tracked `src/` C/C++** were reviewed:
**35 removed, one retained**. The accompanying common-code search covers
tracked C/C++ in both `src/` and `include/`; vendor files and generated build
copies are excluded. Counts refer to written definitions, with comments and
literals excluded. The header census includes both compiler arms of `va.h`.

## Disposition of the 36 source definitions

| Location | Definitions | Disposition |
| --- | ---: | --- |
| `smackmgr.cpp`: RAD import aliases | 13 | Removed; call the existing underscored SDK declarations directly. |
| `smackmgr.cpp`: video flags | 5 | Replaced with typed, file-local constants. |
| `binkmanager.cpp`: archive-open flag | 1 | Replaced with a typed, file-local constant. |
| `kb.cpp`: dialog and player constants | 8 | Replaced with typed, file-local constants, including the array extents and case labels. |
| `misc.cpp`: `READ_REG_PREF` | 1 | Expanded into its 36 registry calls, preserving shared `cbData` updates and query order. |
| `advmgr.cpp`: `APPEND_VISIT_TEXT`, `SET_VISITED_ROLLOVER`, `SET_KNOWN_VISITED_QUICKINFO`, `SET_VISITED_QUICKINFO` | 4 | Expanded into explicit statements and switch arms. |
| `townmgr.cpp`: `HOMM3_TTOWN_SCREEN_RELEASE_DIAGNOSTIC`, `HOMM3_THALL_RELEASE_TRACE` | 2 | Removed with their unreachable calls; these were synthetic inliner inputs. |
| `levelupwindow.cpp`: `HOMM3_LEVELUP_RELEASE_DIAGNOSTIC` | 1 | Removed with its nine repeated unreachable calls. |
| `palette.cpp`: `max` | 1 | Retained: the nested macro reproduces retail's repeated comparison; the standard selector does not. |

The removed RAD aliases were `SmackToBuffer`, `SmackToBufferRect`,
`SmackDoFrame`, `SmackGoto`, `SmackClose`, `BinkPause`, `BinkDDSurfaceType`,
`BinkGetRects`, `SmackWait`, `SmackNextFrame`, `SmackOpen`, `SmackUseMMX`,
and `SmackVolumePan`. Their actual import spellings and declarations remain
unchanged in the owning headers.

The replaced constant names were `SMACKBUFFER555`, `SMACKBUFFER565`,
`SMACKOPEN_FROM_ARCHIVE`, `SMACK_TRACK_MASK`, `SMACKOPEN_NO_FRAME_SKIP`,
`BINKOPEN_FROM_ARCHIVE`, `DIALOG_ICON_MAX_TEXT_WIDTH`, `GAME_PLAYER_COUNT`,
`DIALOG_RETURN_CLOSE`, `DIALOG_ICON_MAX_ROWS`, and
`DIALOG_ICON_ROW_SINGLE/PAIR/TRIPLE/QUAD`. Source comments retain these
spellings; the C++ names follow the project's `g_`/lowerCamelCase convention.
The two high-bit format flags use `unsigned int`; the other constants use
`int`, preserving the original literal types on VC6.

## Common code and source boundaries

The adventure-map macros were reconstruction abbreviations for caller-local
statements. Expansion preserves both `sprintf` branches, the `strcat` tail,
the flag stores, and each case's original scopes. In the rollover SIREN and
STABLES arms, only the assignment to `visited` is conditional; formatting and
appending remain outside that guard. The old multi-statement macro concealed
this retail behavior. Adding braces around its whole expansion would change
it.

The registry macro likewise captured `key`, `type`, and `cbData` from its
caller. Explicit calls preserve the API's in/out size state, including the
4-, 31-, and 350-byte resets. No loop, new temporary, or helper boundary was
introduced. The original and expanded `advmgr.cpp` and `misc.cpp`
preprocessed token streams agree with includes removed for that comparison;
the subsequent VC6 object comparison also verifies the real include state.

The Dreamcast/retail evidence pass was run for `setRolloverText` (0x40b150),
`quickInfo` (0x4137c0), `readPrefsFromRegistry` (0x50b7b0), `TPalette16::gray`
(0x522d50), and the three constructors below: `dreamcast show`,
`dreamcast asm --blocks`, `dreamcast inline-clues`, and `sema diff` with
`--summary`, `--structure`, and verified `--source`. The Dreamcast registry
counterpart is platform-specific and calls `CheckConfigFile`; it does not
supply the Windows registry implementation. Retail remains authoritative.
Existing recovered game helpers and their call boundaries are preserved.

### Retained palette macro

Dreamcast `palette.cpp:588` groups the nested maximum in one source statement;
retail 0x522d50 retests red versus green after comparing the selected maximum
with blue. The retained macro reproduces that control flow exactly. It is
defined after the three `numeric_limits<int>::max()` calls and undefined
immediately after `gray`, avoiding collisions with those member names.

A fresh negative control replaced it with
`std::_cpp_max(std::_cpp_max(red, green), blue)` using the VC6 standard header.
That emits operand-address selection and loses the repeated-comparison block:
eight CFG blocks against retail's ten, with a score of **73.97436%** instead
of **100%**. The control was reverted. The CFG contradiction, not merely the
lower score, supports retaining the macro.

### Removed diagnostic padding

The three macros expanded to constant-true conditionals with unreachable
`printf` calls. Those call candidates changed VC6's inline budget without
emitting diagnostics. Earlier comments acknowledged that their text, counts,
and placement were unattested. Line gaps and diagnostic imports elsewhere
cannot establish these statements, and better byte scores cannot turn
synthetic padding into recovered source. The calls were deleted without
replacement VERIFY expressions or inline-control pragmas.

| Constructor | Retail VA | Before | After removal | Current reconstruction |
| --- | --- | ---: | ---: | ---: |
| `TTownScreenWindow` | 0x005c34d0 | 98.94972% | 95.586754% | 100% |
| `THallWindow` | 0x005c9be0 | 99.66054% | 85.98953% | 100% |
| `TLevelUpWindow` | 0x004f8880 | 99.12995% | 98.00000% | 100% |

The level-up constructor is now exact using its named visibility/hotkey/text
helpers and natural widget-result lifetimes. Four Hall coordinate arrays
regained their Dreamcast-proven `const`; Complete's larger extents remain.
The town-screen constructor is also exact: retail proves an Escape hotkey
on its final button, absent from the older Dreamcast constructor. Restoring
that call and using the ordinary while-iterator attachment loop closed the
match. Hall is exact after restoring town-parameter indexing and the compact
construction/append expressions. Failed probes are recorded beside the functions;
source-family snapshots retain reproduced alternatives. No diagnostic padding
or new inline-control pragma was used. The checkpoints preserve prior peaks
in HIST while recording each authored source correction.

The follow-up covered all 24 functions whose source hashes changed in the
initial cleanup, including the 14 that were already non-exact. The ten
original exact functions stayed exact, and eight more reached 100%:

| Newly exact function | Retail VA | Initial score | Source recovery |
| --- | --- | ---: | --- |
| `TTownScreenWindow` | 0x005c34d0 | 98.9497% | Escape hotkey, real widget-result lifetimes and attachment loop. |
| `TLevelUpWindow` | 0x004f8880 | 99.1299% | Visibility/hotkey/text helpers and widget-result lifetimes. |
| `videoRealignBuffers` | 0x005971f0 | 88.9254% | Existing `Bitmap16Bit::getMap(x,y)` interface. |
| `SmackManager::nextSmackerFrame` | 0x00598eb0 | 90.7265% | Assign the actual dirty flag from the short-circuit predicate, then test it. |
| `eventWindowHandler` | 0x004f0fc0 | 99.3701% | DC OK-arm `if`/`else` and shared reply normalization. |
| `checkEndGame` | 0x004f2ce0 | 88.5571% | Player-record lifetime, handled/outcome statements, enemy-mask consumption and initialization order. |
| `THallWindow` | 0x005c9be0 | 99.6605% | Const coordinate arrays, town-parameter indexing, construction/append expressions and real exit-button lifetime. |
| `videoPlay` | 0x005972d0 | 87.1912% | Store both persistent screen coordinates before copying them into the deferred-update `POINT`. |

Matching all 24 to 100% was attempted but not achieved. The remaining work
is recorded below. Scores are observations, not permission to discard
supported source facts. The ordinary cell-adjuster constructor/restore,
player-loss/enemy-count/dialog getters, const seer references, message/dialog
reference parameters, and named dialog locals/helpers remain restored even
where a caller's score fell. Their earlier peaks remain in HIST.

| Remaining function | Current score | Evidence and attempted recovery |
| --- | ---: | --- |
| `type_dialog_icon::set` | 99.1667% | Nine string receiver/result states and twelve word-scan expression/lifetime states did not recover two nested `_Eos` call decisions; earlier de-inlining controls also failed. |
| `readPrefsFromRegistry` | 95.9319% | Retail's patched four-byte copy/jump, 17 NOPs and stranded copy tail differ from the canonical inline `strcpy`. A direct assignment control does not reproduce those bytes. |
| `setRolloverText` | 96.1491% | Canonical helpers and generator references restored. DC fountain query and the retail/DC four-term flag sum retained through a tail-merging score dip; quest-string inlining and switch-arm residuals remain. |
| `quickInfo` | 95.5740% | Generator evaluations, cell-type name indexes, fountain query, visited carriers and nested guards restored. Composed formatting tails now expand the quest-string destructor correctly; hero/cell inlining and tail scheduling remain. |
| `videoDrawRects` | 90.3379% | Individual DirectDraw object scopes improve the existing `BinkRect` bounds reconstruction. A retained 89.7123% scalar candidate reproduces every DirectDraw stack address; union-loop registers and scheduling remain unresolved. |
| `calculateNormalDialogSize` | 99.9762% | DC5430’s per-element cursor and ordered chained-zero initialization improve the match. Nine array-address operands and two exchanged field-load operands remain; array/center-element binding controls did not close them. |

Reproducible source-family generators are under `scripts/experiments/`.
They reject stale or ambiguous anchors; generated manifests, isolated
candidate trees, unchanged-source controls and reproduced elites are in this
worktree's `build/preprocessor-audit/` and `build/source-families/`. None of
these attempts uses new inline-control pins or synthetic release diagnostics.

The new [`homm3 dreamcast audit`](../source-facts.md) command checks positive
Dreamcast type, qualifier, reference, named-local, helper and source-order
facts against the authored C++ AST. It reports coverage gaps explicitly and
has independent tests, including a real Clang fixture for the missing Hall
qualifiers and level-up helper calls. The required evidence loop in
`AGENTS.md` now includes this check.

## Header census for the common-code search

The search found no remaining project-defined macro that generates game
switch arms or multi-statement common code. The remaining definitions in
`include/` fall into these categories:

| Definitions | Count | Disposition |
| --- | ---: | --- |
| Include guards | 202 | Retained for VC6-compatible header inclusion. |
| `va.h` annotations, `SIZE`, `OVERRIDE`, and release VERIFY | 18 written definitions, 9 names | Retained as the compiler/analysis contract. Individual VERIFY uses remain subject to the evidence requirements in `AGENTS.md`. |
| `dxplay_com.h` HRESULT constants and `HOMM3_MAKE_DPLAY_ERROR` | 58 | SDK compatibility surface; the constructor macro is a single typed constant expression, not hidden game control flow. |
| `soundmgr.h` Miles/RAD aliases | 34 | Shared SDK spelling adapters, not common-code macros. The source cleanup does not change this header surface. |
| `crt_stdio.h`: `SEEK_SET`, `SEEK_END` | 2 | CRT compatibility constants. |
| `pcx.h`: `NOMINMAX` | 1 | Controls the Windows header interface. |
| `advmgr.h`: `VIEW_WORLD_TILE_SCALE_FULL/MID/FAR` | 3 | Floating constants; the owning comment records VC6's failed `const float` folding control. |
| `town.h`: `NUM_RESOURCES` | 1 | Shared resource-count constant, not a common-code macro. |

This header search is a disposition of the common-code category, not a claim
that every header constant needs preprocessor syntax. The requested source
directive review and the tree-wide search for common-code macros are complete.

## Validation

The unchanged-source full build passed before editing. Immediately after macro cleanup, the five TUs affected
only by aliases, constants, or common-code expansion passed the strict
`scripts/experiments/compare-coff-layout.py` comparison against that baseline:

| TU | Identical sections | Identical relocation destinations |
| --- | ---: | ---: |
| `advmgr` | 308 | 4,091 |
| `misc` | 89 | 460 |
| `smackmgr` | 123 | 868 |
| `binkmanager` | 15 | 230 |
| `kb` | 238 | 3,063 |

This comparison checks section layout and bytes, relocation sites/kinds and
destinations, and function identities/locations. It does not substitute a
fuzzy score for byte equivalence. This is the macro-only checkpoint; later
source-fact restorations deliberately change some adventure-map and dialog
bytes. Full builds after the palette control was reverted and after the
level-up/town/helper restorations passed all gates. The palette remains exact.
The source-fact and Dreamcast suites initially passed 39 tests. After
integrating the updated default branch at `90ec3028`, both the new source-line
layout tool and this audit coexist. With the standard-selector boundary
controls and string-template default handling, the combined suites pass **53 tests**.
The new `lines` report was run for the 13 source-correlated functions in the
non-exact starting set; the retained metadata includes gaps and file changes
without inferring missing source text.

On the current default-branch tip, the final full build passes all gates at
**4,086 / 4,764 exact**, compared with incoming main's 4,078. All eight authored
recoveries in the table above are exact, and the unchanged
`CEnterNameEdit::onKeyPress` remains exact. All incoming retail rows and HIST
peaks survive; the only current-score losses against main are the two documented
adventure-manager source-fact corrections, while dialog sizing exceeds its
incoming peak. Whole-executable fuzzy is 96.42% (main: 96.40%).

The final positive source-fact report covers 13 selected functions and retains
**23 review findings and 22 coverage gaps**. Dialog event handling and the
level-up constructor have neither. Dialog sizing has one intentional standard
maximum boundary finding covering DC lines 5240, 5242 and 5245. The report includes
Complete/DC layout and interface differences, uncorrelated or shadowed locals,
minimal platform bodies and unrelated TU parse errors. These remain visible;
zero across the whole touched set has not been achieved. The extended
all-functions-at-100% target also remains unmet; the table above records the
matching limitations explicitly.

## Continued source-line audit

The continued pass reused the source-fact audit and inspected Dreamcast line
positions, spans, gaps and statement groups before generating source families.
Dialog sizing has an observed 229-line span with 88 recorded rows; Hall has
160 observed lines with 89 rows. These are partial observations, not recovered
total function lengths. The candidates retain meaningful declaration, scope
and statement boundaries without padding the source to equalize line counts.

Dialog's 72-state line-layout family produced 21 distinct objects and ten
reproduced elites (`28c51ed50a836767467e`). Restoring widest-icon-before-spacing
initialization, delaying the icon count to its loop, and separating line count
from pixel height improved 87.1948% to 93.6558%. DC's three direct
`std::max<long>` calls needed a retail boundary correction: the x86 expansions
copy both operands into fresh stack homes, as the canonical by-value wrapper
does. A direct reference selector instead selects the original objects. The
long local types remain intact. This is operand-identity evidence, not a
rejection based on similarity score.

The follow-up tested 48 row-array, width-floor and height-statement combinations
(`a442a1c9bff70897503e`), producing two objects and two reproduced elites.
The `else if` supported by DC 5317..5323 and retail's skip-to-join branch raised
the score to 93.8074%. Candidate `63bac3599581ad3e2f6be78f` was separately
reproduced before adoption; its byte-equivalent form preserves the distinct
maximum/rounding and base-height/text groups. All 43 exact `kb` siblings held
through both families. The four named font calls agree; the remaining call-view
disagreement is the internal jump-table addend. Array stack homes, the constant
128's register lifetime, width reloads and a height-loop join remain unresolved.

Hall's 72-state follow-up (`2fbf81be0e4da4d8673b`) tested compact construction
groups, actual vector bindings and counter/attachment lifetimes. It produced
25 objects and ten reproduced elites, with no improvement over 99.6436% and
all 79 exact `townmgr` siblings preserved. No Hall variant was adopted.

The audit previously excluded all standard-library helpers. It now checks
public `std::min`/`std::max`, correlates VC6's `_cpp_` names, and keeps direct
selectors distinct from by-value wrappers. Clang fixture controls reject both
flattening and wrapper substitution. Other standard-library groups remain
explicitly outside its documented coverage; a zero report never certified them.

The next event-handler family (`4d8bb7f50a2b0a3f5a2f`) exhausted 12 states,
producing ten objects and ten reproduced elites. DC2593..2603 supports an
explicit OK-arm `if`/`else`, followed by shared reply/deadline stores. That
form is exact; the earlier fallthrough retained a register mismatch.

Endgame recovery progressed through three source boundaries. The player-record
family (`ccc76af4cd8a00cd9e7e`, 24 states, four objects/elites) recovered the
record retained across DC2750..2776 and raised 88.5571% to 93.4214%. The outcome
family (`3936ec51d420edcfcd04`, nine states, six objects/elites) restored the
handled flag and DC3728/3735/3742/3747 outcome assignments. Only the complete
form reached 96.0524% and recovered all 25 named call decisions, including
GetTeamMask's expansion and retained GetTeam call.

The enemy lifetime family (`bde76ac1e8a8cc76c583`, 32 states, 18 objects, ten
reproduced elites) recovered retail's early counter initialization, receiver
reload and full-width mask consumption, reaching 99.7119%. Finally, the four
DC3628/3629 initialization states (`8932f891c289d51a4295`) produced four
objects and reproduced all four: separate outcome zero assignments followed
by the standard-victory initialization are exact. Chained zeros score
99.9952%; interleaving the initialization scores 99.7143%; putting it first
scores 99.7119%. All 44 prior exact `kb` siblings survived.

Dialog sizing's 27-state counter/minimum/field-lifetime follow-up
(`524b4976781b28b63bd4`) produced eight objects and eight reproduced elites
without improving 93.8074%. The icon's two-state empty-text return family
(`5bbe108e7462f601b2dd`) produced one object. Its opposite corner reproduced
separately; the adopted early return preserves DC5089/5090's lexical boundary
before the two text loops, but leaves both nested `_Eos` decisions unchanged.

Rollover's next audit found two `generator&` locals still spelled as pointers.
The four-state reference family (`15ac0ad9ea80313bdf7c`) produced one object;
both recorded references are restored with no byte change and all 93 exact
`advmgr` functions preserved. Their repeated names remain explicit audit scope
correlation gaps, rather than being inferred from stack-slot order.

DC3477 also records `infolevel = GetInfoFlag(...)` before the fountain's
independent DC3479 cell-knowledge test. DC3491 and retail +0x8b1 both sum four
masked flag terms. The six-state family (`22d70a3031c93d08c634`) produced four
objects and four reproduced elites. The retained DC operand order plus query
(`6a70d933045e709512b84839`) was reproduced separately and emits the same
object as the opposite-order elite. The query alone is byte-flat; the four-term
sum changes shared visited-format tail merging and scores 96.1491% against
96.2649% for the flattened mask. Both evidenced source operations stay, with
the earlier peaks retained in history. The previous source discarded the sum
solely for its score; that is not contrary retail evidence.

### Hall parameter indexing and exact closure

The constructor now matches all retail bytes and report-level relocations.
Its 369 CFG blocks and 178 branches agree; the full build preserves all 79
previously exact `townmgr` siblings. The call-name view still pairs folded
`vector` instantiations and an internal constructor label differently; these
are report-level equivalent, not missing helper boundaries.

DC 4344 at `0x16eafe..0x16eb28` multiplies the saved `which` parameter in R9
by 72 for both `hallX` and `hallY`. The switch at 4338 uses the same R9, and
the later construction group at 4419 repeats parameter-based indexing.
The authored cases had prematurely substituted their constant town rows.
Restoring either table alone was byte-flat at 99.6605%; restoring both fell
to 92.5610%, while correcting the first loop-update scheduling differences.
That dip was retained because the parameter use is positive source evidence.

From that corrected parent, the 162-state parameter/widget-result family
(`eab37789b1aec9d5d922`) reproduced several exact forms. The adopted simplest
form, `61d9353994ff116031c4dc5e` (`1-1-1-1-0`), directly appends each new widget
and the three status/title constructions. It retains the exit-button pointer
for `setHotkey`. These expressions correspond to DC's construction/append
statement groups at 4344..4347. No synthetic statements or inline pin are
needed. Earlier result scopes reached 99.6605%; sixteen induction spellings
produced one object without further improvement.

The three compiler traces reproduce their respective uninstrumented C2
objects. Case-specialized rows with four named widget results have caller
cost 7,627 and initial budget 15,254. Parameter-indexed rows with the same
results have cost 7,843 and budget 15,686. The exact parameter-indexed form
with direct construction arguments has cost 7,663 and budget 15,326.
These measurements demonstrate that an index later folded to a case constant
still changes the earlier inline budget, and that real expression boundaries
change it again. They do not establish a unique source spelling or authorize
adding statements to hit a chosen budget. Source facts must be composed before
judging their caller effects: the parameter-only score dip did not predict
the final result.

### Quick-info source composition

The continued pass raised `quickInfo` from 94.7326% to **95.5740%**, preserving
all 93 exact `advmgr` siblings and every incoming HIST peak. The full evidence
pass included `show`, `lines`, `asm --blocks`, `inline-clues`, `audit`, and the
retail summary/structure/source/call views. DC's observed source span is
7543..8813, with 525 recorded rows and 746 unrecorded lines; the total source
length and missing text remain unknown.

Unlike rollover's generator references, DC7739/7740 and 7757/7758 separately
evaluate the vector subscript for owner and type. Restoring those evaluations
improved an intermediate candidate to 95.2514%. The fountain queries its info
flag at 7890 before the independent cell-knowledge test at 7892; its four-term
sum belongs to `visited`. These facts remain in the source. The 64-state
family (`a4dd1dc5154e9b559b32`) produced 24 distinct objects. Its combined
source-fact form, `6c2c277241c20bfa10210105`, reproduced 93.3711%, so the
follow-up tested the related formatting tails together.

The 16-state visit family (`1a847cf1fe9f53f4b968`) produced eight objects and
eight reproduced elites. Candidate `1aaaba540484fe1a2c198a4f` reached 95.5740%
with Arena's conditional argument, explicit Buoy/fountain branches, five
nested trigger/hero guards, and the lean-to `visited` carrier. A conditional
format argument can produce two emitted `sprintf` calls; the source-line
and call evidence does not establish one unique C++ spelling. Neither equal
statement counts nor lower isolated scores decide source fidelity.

DC's object-name loads use the cell's type: the table reads at 0x1646c,
0x165b4, 0x16e00, 0x17804 and the other object-name sites load `cell+28`.
The name-index/format family (`361c6c8aefc244a5542c`) produced twelve objects
from sixteen states. Restoring all 32 constant subscripts to the cell type
is byte-flat on the adopted `e31207e77cf0828cc210dd1e` parent, but changes
some other format combinations. This provides another control for the Hall
finding: source-level case specialization can affect later optimization.

Eight flag-expression states (`344fc59b3f2df8289997`) produced two objects.
Direct Arena accumulation falls to 91.7578%; the garden expression and
Temple's DC low-to-high sum are byte-flat. The reproduced
`f1eaf19924e0cebfee4fbd75` retains the Arena carrier and adopts those latter
two forms. The final source restores the names `testCell`, `result`, `special`
and `debugText`, then passes the full build again at 95.5740%.

The selected source naturally expands the quest temporary's destructor as
retail does. The remaining named call difference is the nested `cell`/`zCell`
choice; the differing internal jump-table addend is not a different callee.
Formatting-tail sharing and the `getHero` arm layout remain under review.
Complete's retained mine and seer builders are retail-proven boundaries,
so their older DC in-caller operations stay review findings rather than being
pasted into this caller. The map-cell/extra-info ownership gap also remains.

Recovering the local names removed four coverage gaps and exposed a standard
string-type spelling issue in the audit. The comparison now expands omitted
`basic_string` defaults, with a real Clang alias fixture and negative controls
for different traits, allocators, character types and cv/ref layers. This
corrects the comparison without changing the source type or suppressing a
custom specialization. The 13-function report retains 23 findings and 22 gaps.

The final inline trace reproduces the uninstrumented C2 object: caller cost
6,720, initial budget 13,440, and 392 recorded tests. The ordinary empty-string
constructor/destructor sites reject `_Tidy` at budgets 119/122 against cost
152; the quest temporary accepts it at 157, and the seer temporary at 481.
The nested three-coordinate `cell` accepts at 116 against cost 47, then
rejects `zCell` at 34 against cost 59. This localizes the remaining boundary:
retail retains the outer `cell` call in that invalid-point arm. The trace
supplies a compiler-state lead, not permission to suppress the canonical body
or add mass to change its budget.


### Dialog placement cursor

The fresh DC5430 group at `0xe5ea8` increments the current icon index inside
its placement loop; a separate counter advances at `0xe5e74`. The source had
indexed `firstInRow + k` and advanced `firstInRow` after the row. Restoring
`m_icons[firstInRow]` and the per-element increment raised
`calculateNormalDialogSize` from **93.8074% to 99.9719%**. Both states in
`dfd00b78016955893f91` emitted distinct objects and reproduced; the adopted
`091ea12557bf81417b095134` preserves all 45 exact `kb` siblings. The full
checkpoint passed, retaining every historical peak.

Retail adds the row count to ECX before the inner loop. VC6 produces that
hoisted addition from the recovered per-element cursor. Reading it as a
literal bulk source increment concealed a lifetime difference that also
changed earlier register allocation. This is a useful example of source
line evidence resolving a much wider scheduling residual.

The raw comparison still has eleven row-array displacement differences and
two exchanged center-icon x/width load operands at `+0x435/+0x43b`. The 87
CFG blocks have equal sizes and flows, and all branches and the four-entry
jump table agree. Neither the skeleton's “exact” label nor the flat masked
instruction view establishes byte identity. Four declaration/addition-order
combinations in `23e4b355204a2880ec87` emitted one reproduced object; none
improved the residual. Keep the proven array initialization order and types.
DC5257 separately recomputes the per-row quotient; DC5433 uses the second
row-array elements. Those operations remain unchanged.

The icon recheck also exhausted eight morale/luck frame-before-name negative
controls (`3ac59c5f3dcb3ebfb84c`, eight objects, 96.3289..98.5486%) and eight
sprite-result/line-count lifetime states (`1c285acee8567a1431a9`, one reproduced
object, 99.1667%). DC5046/5051/5056 loads filenames; an initial interpretation
as frame loads was incorrect. The generated hypothesis was corrected, and
no candidate was adopted. The current name-before-frame order is supported
by the following shared string assignments and later frame stores. The
verified icon compiler trace has both EXPERIENCE `_Eos` sites at budget 47
against cost 46; the result-binding alternatives do not resolve those calls.


The dialog's subsequent 18-state array-element/center-icon/zero-assignment
family (`459a068b223a41e11e3b`) produced eight distinct objects and eight
reproduced elites. Chaining `labelHeight[i] = iconHeight[i] = 0` preserves the
DC5219/5220 store order and fixes the two height-sum operands at
`+0x2a0/+0x2a4`, reaching **99.9762%**. The adopted
`6f077b2f393d6480fff81e12` uses direct array subscripts and center-icon
accesses. Reference/pointer bindings to the height elements lost matching;
center-icon bindings did not help. Nine array-address differences and two
exchanged field-load operands remain. The seven recorded local facts remain
intact; the punctuation of the chained assignment is a tested hypothesis,
not recovered source text.

### Video rectangle lifetimes

`videoDrawRects` now reaches **90.3379%** with the existing SDK `BinkRect`
bounds, function-scope `POINT`, source `RECT` and `DDSURFACEDESC`, and an
overlay-scope destination `RECT`. The full DC evidence pass confirms that
its counterpart is a four-byte platform stub. All body, local-lifetime and
initialization hypotheses therefore require retail validation.

The SDK-field/geometry-lifetime family (`acc58cffad04bcf4a073`) exhausted
36 states, producing 18 objects and ten reproduced elites. Moving all four
DirectDraw locals to function scope reached 90.3333%, but reserved `0xa8`
bytes against retail's `0x9c`. The individual-scope family
(`51a5b6186d722e9c39df`) exhausted 16 states and reproduced eight objects.
Its retained `da341e8f602aa4dde9786718` uses the scopes above and reserves
`0x98`. Pure coordinate/full-rectangle captures did not fix either union
loop. All eleven named/indirect calls agree with retail.

The 25-state bound-type recombination (`a0b7ae8978754f63cbff`) produced
twelve objects and ten reproduced elites. It also yielded a useful lower
score: `db3d1eb678b25767ba0c04a1`, with signed `long` scalars, reaches
89.7123% while reproducing the frame and every DirectDraw object address:
`POINT -0x10`, destination `RECT -0x20`, source `RECT -0x30`, and surface
descriptor `-0x9c`. Registers, dimension-load scheduling and the union loops
still differ. This candidate remains available as a source-lifetime lead;
an exact frame alone does not establish the complete reconstruction.

Both parents fed the capture-order family (`972ac67c3210e041d2ac`):
73 states, 17 distinct objects, ten reproduced final elites. Unlike the
older coupled order trials, it varies initial Smack and Bink captures
independently, then tests coordinate/dimension initialization stages of the
actual source rectangle. It did not improve on 90.3379%. The retained source
keeps the original update order, including x/y updates before the extent
comparisons, and preserves all 28 exact `smackmgr` siblings. None of these
experiments adds a helper, synthetic object or inline-control pragma.

### Video playback coordinate lifetimes

`videoPlay` now matches all 669 retail bytes, with 41/41 exact CFG blocks,
27 matching branches, two matching returns and all 14 calls agreed. Its
Dreamcast counterpart is a platform stub, so the Complete instruction order
provides the body evidence: retail stores `gSmackX`, computes and stores
`gSmackY`, then spills the two `POINT` members before `_SmackToBuffer`.

The six-state immediate-global family reproduces three exact objects. The
adopted form computes both globals first, copies them into the `POINT` retained
for `updateScreen`, and passes the globals to `_SmackToBuffer`. This gives x,
width and height the retail EBX/ESI/EDI allocation and frees EBX for the loop's
shared zero. Computing the `POINT` first capped the row at 87.1912%; direct
mutable coordinates removed its homes, while volatile/address-exposed
diagnostics introduced extra loads. The broader declaration, scalar/aggregate,
helper, message, gate, SDK, parameter, TU-state and lifetime families supplied
negative controls without introducing source-false qualifiers or inline pins.
