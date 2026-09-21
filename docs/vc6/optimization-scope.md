# VC6 C2 optimization scope — how far effects reach, and what we can predict

Answers three recurring questions: (1) is the inline→everything cascade written
down; (2) can we predict whether something inlined; (3) how *non-local* are the
optimizations across a function. Grounded in what the `vc6` area reverse-
engineered (inliner, register allocator) plus the Ghidra atlas's module map
(`build/vc6/c2-tu-map.tsv`); passes we located but did not model are marked
**located-only**.

## 1. The C2 pass pipeline (image base 0x10700000; anchor RVAs from the atlas)

C1XX (front end) parses the TU and emits IL — symbol handles in parse order
(`docs/vc6/handle-order.md`), each callee's size estimate `cb`, candidacy flags.
C2 then runs, roughly:

| order | pass | module (anchor RVA) | modeled? |
|---|---|---|---|
| 1 | read IL | `reader.c` 0x84d9b | handle/cb extraction (il-format.md) |
| 2 | **inline expansion** | `inline.c` 0x94521 | **yes — `predict()`, 9/9** |
| 3 | flow graph | `fg.c` 0x86c8a | substrate (CFG) |
| 3 | loop graph | `lg.c` 0x91d26 | substrate (loops) |
| 4 | global dataflow (CSE, copy-prop, DCE) | `globdf.c` 0x8f5e3, `globopt.c` 0x90643 | located-only |
| 4 | single-def/single-use | `sdsu.c` 0x87418 | located-only |
| 4 | local DAG (block CSE/order) | `dag.c` 0x91bbf | located-only |
| 5 | **register allocation** | `color.c` 0x8e474, `regasg.c` 0x8b906 | **yes — first-fit preference** |
| 6 | scheduling | `schedmd.c` 0x7a7de | located-only |
| 7 | emit | `code.c`/`emit.c` | — |

**The one load-bearing ordering fact we proved:** inlining (step 2) runs *before*
register allocation (step 5), and its cost model uses the front-end `cb`, not the
allocated result. So the cascade is **one-way**: inline decisions feed regalloc /
CFG / scheduling; those never feed back into inlining.

## 2. How non-local each optimization is

The heavy passes reason over the **whole function** (or a whole loop), not per
statement — which is why a single local change is rarely local in effect.

- **Register allocation — WHOLE FUNCTION.** Live ranges span all basic blocks;
  the allocator walks pseudos in creation order and first-fits over
  `{EAX,ECX,EDX,ESI,EDI,EBX,EBP}`. One added value (or one call vs inline) shifts
  the ordering and **renames registers everywhere downstream** — the modal "B1
  swap" is a whole-function decision, not a local one (`docs/vc6/regalloc.md`,
  catalog B7).
- **Global dataflow — WHOLE FUNCTION, cross-block** (`globdf.c`/`globopt.c` over
  `fg.c`): common-subexpression elimination reuses a value across blocks; copy
  propagation (the `this` alias we measured); dead-store / dead-code elimination
  (a store in one block killed by a def three blocks later). located-only — we
  *observe* these via `sema diff`, we do not predict them.
- **Loop optimizations — WHOLE LOOP NEST** (`lg.c`): LICM hoists loop-invariant
  computation out (VideoClose, iconwdgt); strength reduction / induction-variable
  rewriting spans the loop. located-only.
- **CFG optimizations — WHOLE FUNCTION**: jump threading, tail-merging,
  cross-jumping operate over all of the function's exits (the merged-return /
  DUP-EXIT family, catalog D4–D7).
- **Inline budget — PER CALLER (whole function), sequential**: charged in tuple
  order, so an early inline starves a later site (A9); nested expansion divides
  `budget ÷ sites-remaining`.
- **Scheduling — mostly WITHIN A BLOCK** (`schedmd.c`/`dag.c`), but the schedule
  interacts with the global allocation, so it isn't cleanly isolated.

What *is* genuinely local: instruction selection per tuple, immediate/SIB
encoding (the B18 base/index tie-break), and window peepholes (`fppeeps`). These
are the residuals no whole-function reasoning reaches — and the ones the tools
correctly report as not source-addressable.

**Consequence for matching:** because 2, 4, 5, and the CFG passes are
whole-function, and handle numbering is whole-*TU*, a mismatch propagates. This is
the "butterfly": a declaration-count change moves a distant function (handle
order, TU-global), a call-vs-inline renames every register after it (allocation,
function-global). It is *why* the doctrine is **inline → control-flow → register**
(fix the upstream, function-global cause first) and why one wrong inline can drop
a function to 46%.

### Tiny setter calls can change allocation after expansion

`TCustomCampaignWindow::updateList` (`0x483330`) remained at 93.8038% with
four direct color-field assignments. Calling the canonical `textWidget::setColor`
inside the same two highlight arms restores 100%, including EDI for `this`,
EBX for the loop index, and the name-widget load before the branch. Both
source forms ultimately expand to the same four member stores; their compiler
input is still different. The fifteen-state search emitted ten objects and
reproduced ten retained states, with all thirteen sibling functions exact.
Caching the name widget or selected index fails to reproduce the result.

The setter itself is recorded at Dreamcast `TextWdgt.h:78..81`; this
Complete-only caller has no Dreamcast counterpart. Its use is supported by
the retail result, rather than claimed as a recovered source call. An earlier
comment attributing the register swap to the include set was incorrect.

In `TQuickHeroWindow`'s constructor (`0x52ead0`), binding the real widget
vector once by reference restores the retained mana-string `_Tidy` call.
The full-expression allocation/formatting/cleanup order remains unchanged;
111 CFG flows, 54 branches and 59 calls then agree with retail. This recovers
96.8447% from 94.1662% while preserving the recorded constructor scopes and
Complete's owning `ostrstream`. Coordinate captures and equivalent split
stream-output expressions did not improve it. A named mana string initialized
before the widget allocation was excluded: it would change the evidenced
call order, regardless of its possible effect on inlining.

### Source lifetimes can also change refusal epilogues

`advManager::summonBoat` (retail `0x41c8a0`) illustrates why a merged-return
diagnosis is provisional. After restoring its recorded interfaces and helper
calls, 72 refusal/search variants emitted one object at 91.5769%. Sixteen
variants of the two real `SLimitData` constructor expressions emitted nine
objects. Removing the invented coordinate caches matched the retail frame,
all 50 blocks, all 31 branches and all four returns. Correcting only the old
boat rectangle reached 94.9842%; only the destination rectangle reached
95.9570%; both reached 100%. The corresponding direct rectangle expression
also closed `skuttleBoat` (`0x41cdf0`), from 98.4651% to 100%.

These are measured effects of source expressions and lifetimes, not a trace
proving which C2 pass caused each change. No extra operation, inline pin or
alternate helper was needed. The retained source and controls are documented
beside the functions in `src/advspells.cpp`.

`townGate` (`0x41d360`) supplies a second control. Its recorded global
`int max(int,int)` wrapper owns argument copies before calling the
const-reference template selector. Calling that selector directly left the
caller near 91.81%; restoring the wrapper recovered 99.9914%. A separate text
accessor control was byte-flat. With that source boundary restored, declaring
the X difference before Y inside the canonical `DistanceSquared` helper
removed the last four stack-displacement differences and reached 100%.
The earlier X-first probe had failed under the incomplete caller model.

The helper's Dreamcast rows are nonmonotonic (121/124/122), with no named
locals. Its mixed X/Y loads and earlier completed Y subtraction are SH4
scheduling evidence, not proof of C++ declaration order. Both orders remained
valid hypotheses until VC6 distinguished them. Retest a justified earlier
probe when the canonical caller changes; preserve the helper and its source
calls throughout.

The Complete-only `type_text_scroller` constructor (`0x5b9fb0`) supplies
a separate argument-evaluation control. Its slider construction passes both
`max(1, textLines.size() - lineImages.size() + 1)` and a later page-size
argument. Direct `cppMax<int>` reference selection evaluates the maximum
operands first under VC6 and scores 93.1943%. The canonical by-value `max`
restores retail's page-size-first evaluation, all stack homes, and all 767
function bytes. Reversing the wrapper's operands reaches only 99.7809%.
All seven sibling scores hold across the four reproduced states. Equal
arithmetic results do not make the wrapper and selector interchangeable
when reconstructing the surrounding call expression.

`TCombatControlSubWindow` (`0x46bc30`) supplies an even smaller helper
boundary: `button::setDisabledFrame` is one field store. Flattening it in
the constructor changed scheduling inside the following `setHotkey`
expansion. Restoring the first setter raised 96.3990% to 100%; restoring only
the second setter was byte-flat. Both calls are positively identified in
Dreamcast, including the second indirect call through the retained callee
register. The 24-state joint setter/max family emitted sixteen objects and
reproduced ten retained candidates. The same family isolated the recorded
by-value `max` wrapper in creature Update: its first call recovered 90.3734%
to 94.8880%, while the second wrapper and attack-local integer spelling were
byte-flat. A direct field store or flattened template can preserve behavior
while losing the compiler state of the original source boundary.

A single emitted call can also come from two source calls merged after their
arguments are prepared. `TCombatCreatureSubWindow::update` proves this at
`0x46dc30`: DC lines 751 and 753 compute the icon receiver in separate arms,
then SH4 shares `SetIconFrame`. Retail pushes the nonzero frame and literal
zero in separate arms before one receiver load and call. A shared frame local
or a conditional expression inside one call reaches 94.8880%; calls authored
in both arms reach exact and recover the spell/icon register homes. Twenty
source states emitted fourteen objects; ten retained candidates reproduced,
and all twenty-four previously exact siblings survived. The loop increment
placement was flat once both calls were restored. Read argument placement
and positive line/scope evidence before treating a shared call as a source
call-count constraint.

The battle `TViewArmyWindow` constructor (`0x5f3360`) supplies another
shared-call control. Its background helper has two recorded palette-call
arms (DC 620/622/623), which SH4 merges. Restoring those arms from one call
with a conditional argument recovers the following widget insertion and
`GetName` schedules under VC6, raising 95.8012% to 97.2049%. The morale and
luck helpers also own their member stores: both DC and retail store before
allocation and reload the member afterward for `limit`. Caller-owned cached
values lose those reloads. Preserve the actual helper responsibilities when
diagnosing register differences in the expanded caller.

Recheck old invariant experiments against the current compiler context.
`createDamageWidget`'s retained `!Widgets.empty()` expression is now
byte-neutral when removed, independently of restoring its recorded traits
reference and text subscript. Nine states across six consumers distinguish
those effects. The historical one-site inliner explanation is no reason to
retain an unnecessary expression; the final source removes it.

Restore coupled boundaries before judging a caller budget. In the quick-town
constructor at `0x530120`, three `HasBuilding` calls had been flattened and
four `push_back` calls replaced by `insert(end(), value)` to hold 98.8368%.
Restoring only the building calls gave 94.9246%; restoring both groups held
98.8368%, including with direct `m_widgets` calls. Twenty-four source states
produced eight objects, all eight reproduced, with six exact siblings intact.
The earlier score-only rejection of the fort helper calls was a consequence
of the other substituted boundary. The constructor reaches exact when
the remaining scan is modelled with `resource[0]` as its induction lvalue,
with outputs in `resource[1]` and `[2]`. DC's `EGameResource[3]` local at
sp+0x5c is r14+24 after the recorded 68-byte stack adjustment: precisely
the loop counter address. Its true arm increments count before storing at
that base plus `4*count`; consumers load base+4/+8. Retail independently
agrees at ebp-0x28/-0x24/-0x20. The extra counter reload is array aliasing,
not an inexplicable register allocator decision. Four structured loop/count
forms emit the exact same object; the separate-local control holds 98.8368%.
The native scan fixture uses the authored enum, loop and consumer indices,
passes all nine pinned retail income rows plus 99 synthetic rows, and rejects
four controls that change output indexing, count timing, sign or gold bounds.

## 3. Can we predict whether something inlined?

**Yes — deterministically, not just probabilistically — for a source we control.**
`inline_model.predict(caller_cb, sites)` implements the RE'd rule and is validated
9/9 against the real compiler. Given the inputs it says expand-or-call exactly:

- **caller_cb / callee cb**: measurable with `inline_model --measure-cb` (titrates
  a callee's front-end estimate against the real compiler) or readable from the
  IL (`il handles` / the `sy` stream). With these, prediction is exact.
- **No-compile / probabilistic mode**: estimate `cb ≈ 14 × simple-statements`
  (`CB_PER_STMT`, measured) and candidacy from the ~13-statement front-end
  save-gate. That predicts expand/call from source structure alone, with the
  uncertainty of the estimate — good enough to answer "will this inline?" and to
  quantify the budget gap (`inline_model --gap`: "grow the caller ~N statements").

Two honest limits: (a) for **retail** we don't predict, we **read** — the
`predict-inline` diagnoser reads retail's actual out-of-line calls from the
delinked object, so prediction is for *our* reconstruction and for reasoning about
gaps; (b) the model omits the rarely-triggered post-substitution veto (inliner.md
§6). Register *bindings* are likewise predictable (first-fit in creation order,
why-reg v2) **when the divergence is handle ORDER**; when it is handle STATE
(values differ, order matches) it is C2-internal and not predictable from order
alone (`docs/vc6/handle-order.md`).

## 4. Is the cascade documented?

Now yes — here, plus the pieces it ties together: the inliner rule
(`inliner.md`), the allocator (`regalloc.md`), catalog **B7** ("register renaming
cascade after a call-vs-inline change") and the smackmgr worked example, and the
`diagnose` routing that encodes inline→flow→register.

The resource-display constructor (0x558ba0) supplies a second condition-merging
counterexample. Dreamcast lines 41 and 46 separately test size for subwindow
initialization and background allocation; retail has a merged branch graph.
Writing one combined if/else kept the size input in BL across the body and
transposed this/textX, measuring 95.0450%. The historical volatile workaround
only reached 99.3919%. Restoring the two source conditions produces exact VC6
bytes, including the parameter reloads and EBX/EDI assignment. All 30 split
states in a 45-state condition/id-counter/declaration family are exact; all
15 merged controls stay at 95.0450%. All four exact siblings survive. The
explicit text/border counters retain DC75/76 and86/87/88; their spellings and
layout-declaration placement are independently score-flat. A merged retail
CFG therefore does not justify collapsing positively evidenced source tests.

The canonical `hero::getPrimarySkill` (0x5bde40) demonstrates that identical
retained helper bytes do not imply identical expanded callers. Its signed-byte
temporary and direct `m_stats[skill]` expressions both emit the exact 49-byte
body. The latter closes `type_spellvalue::type_spellvalue` (64.3242% to exact)
and `swapManager::update` (89.3496% to exact), and restores the exact retained
`_Unguarded_partition<type_monster_data>` body in `ai_combat`. In the first
caller, the source change switches the two insertion-sort inline decisions
into retail's order; in the second, it restores register allocation. DC
Hero.h:672/673, 675 and 677 support the conditions and returns, but the missing
local record alone cannot choose between these source forms.

A six-state family measured nineteen consumer TUs, yielding four reproduced
objects. Const-byte is byte-flat; a byte reference loses four exact functions;
promoted int/const-int temporaries lose nineteen. Direct reads improve nine
callers, lower three non-exact callers (setupThievesGuild, setupDynamicStuff,
perDay), and lose no exact caller. The full 68-TU checkpoint confirms the
collateral and raises exact functions from 4036 to 4039. QuickHero improves
96.8447% to 97.5654%, but forty follow-up index/coordinate states (eight objects)
leave its remaining signed index induction unresolved. Inspect the canonical
helper's actual source expressions before treating a caller's register delta
or nested STL inline decision as an independent compiler limitation.

Mouse Update (0x50cd90) closes after restoring eight distinct RECT locals
and their branch scopes. CodeView's variable owners and frame-relative SH4
accesses distinguish the outer new_rect, two non-overlap rectangles, and
five overlap rectangles; the old reconstruction coalesced these into six
function-scope objects. DC663 copies the front rectangle back into new_rect,
664 offsets it, 685 reads the copy dimensions and 688 saves it. Recovering
those identities and reuse raises 99.842% to exact. Both source path orders
emit exact bytes: VC6 places the overlap path first even when the source
retains DC's non-overlap-first condition. Output block order alone therefore
does not prove source branch order. The full checkpoint reaches 4040 exact
functions, with all other callers unchanged.

GameTime's CodeView public symbols use namespace @@YA procedures, unlike
actual static members' @@SA symbols. Restoring the namespace, ElapsedSince's
Get/Elapsed calls, NextFrameTime's ElapsedSince call and DelayTil's IsPast
predicate preserves all three retained GameTime bodies at exact. Its shared
header state changes six consumer TUs, including an exactness exchange
between CEnterNameEdit's key and focus handlers; the 105-TU checkpoint still
has 4040 exact functions. The source facts remain authoritative despite
those collateral changes, with previous peaks retained in history.

Mouse CheckUpdate remains 96.1361% because its nested HidePointer expansion
inlines the TCSLock constructor that retail calls at 0x50d890; that retained
constructor is currently not emitted. The thirteen other mouse rows are
exact. Eight function-static declaration/initializer forms emit one object;
four timer guard forms emit two, and four HidePointer guard-scope forms emit
two, without restoring the call. Seven NextFrameTime clamp forms across nine
TUs emit four objects: the neutral conditional form holds all scores, while
split returns lose four exact functions. Scope and expression changes must
be checked through callers even when the retained helper remains exact.

Artifact traits' DC132..149 proxy assignment calls are compatible with
retail's retained bitset::set call: the Dinkumware reference assignment itself
delegates to set. Replacing the canonical proxy interface with a direct set
call had concealed another inline decision. Three source forms produce three
reproduced objects; the restored direct proxy expression improves the table
initializer from 81.3762% to 81.9921%, preserving seven exact siblings.
Naming the RHS bool gives 80.8594%. The remaining reference assignment,
_Tidy and equality expansions still differ from retail; a higher similarity
does not establish that their named call streams agree.

TGzInflateBuf supplies negative controls for the same helper/caller boundary.
Six getByte byte-binding/advance forms emit five objects. A postincrement
inside the byte read preserves both retained reader matches but lowers
underflow from 81.4346% to 66.1989%; a success-first readByte return layout
holds its own exact bytes yet lowers underflow to 73.1099%. The sixteen-state
reader/error-construction family also finds no gain. Retail's alternating
two expanded/two retained trailer reads remain unresolved; neither family
is adopted, and the exact helpers keep their original source expressions.

AI castSpell (0x425bd0) closes from 86.7910% when its pasted resurrection
valuation loop returns to the ordinary, const getSummoningValue helper
called at DC1033. This reduces the caller's expansion budget and restores
both unrelated mass-damage call decisions. The full restoration also keeps
DC988's SpellIsAvailable and DC1072's castSummoning calls. All 23 named
retail calls then occur at the same offsets, with every sibling score in
ai_combat and ai_player unchanged.

Eighteen source states produce eighteen objects and ten reproduced finalists.
Restoring only castSummoning lowers the caller to 79.8398% with a range
guard or 79.8008% with a switch; restoring the value helper alone or both
helpers reaches exact. Both complete DC switches and equivalent ranges
work, so retain the evidenced switches and all three calls. The earlier
mass-helper and Familiar-helper families had exhausted local alternatives
without finding this separate source boundary. A nonlocal pasted helper
can be the cause of apparently unrelated nested inline decisions.

AI initializeCreatures (0x424120) reaches exact by recovering two source
bindings and correcting one value. DC283 computes speed before the unit
stores and preserves it through the classifier call; DC284 captures the
attribute flags for the later ranged branches. Retail independently keeps
those same values live. Speed alone reaches 96.2279%, attributes alone
94.4426%, and both reach 99.9981% from 92.7119%. The remaining operand is
behavioral: combatValuePerHit divides the per-creature value by hit points,
not the whole stack value. Retail's unit+0x3c load and DC295 agree. Correcting
that numerator reaches exact and prevents population from multiplying the
per-hit ratio. Thirty-two source states emit eight reproduced objects;
statement order and the canonical armyTypes enum view are byte-flat. The
native fixture checks 3584 numeric records and rejects stack-total, base-HP
and missing-population-factor controls. All forty scored AI-combat functions
are exact. The separate coverage audit accounts for every retail body and
flanking gap, plus the expanded, dropped and library-only Dreamcast records;
the owning source records that function-coverage closure.

AI doAftermath (0x426ee0) demonstrates why linkage must accompany a recovered
helper boundary. Dreamcast marks do_eagle_eye static. Restoring that ordinary
static helper and its two SpellIsAvailable calls preserves the exact caller;
the older forced-inline failure did not justify a pasted body. Seven source
states emit five reproduced objects: ordered guards with either a break or
first-success return remain exact, while one combined condition gives
89.0061%. The helper stays static and has no retained retail body. Restoring
the reference-qualified AI interfaces and canonical getTotal, getMana and
valueOfExperience calls also preserves all current scores, including callers
in events, command and townmgr.

### System-options helper recovery and a nested reserve limit

`TSystemOptionsWindow` now calls the complete ordinary `updateSystemOptions`
helper from its constructor and handler. Dreamcast records the first-update
network path at 671..692 and later redraw at 696..697. Restoring that boundary
and the pointer-registration loop raises the constructor from 96.3370% to
99.3013%, with the standard vector owner preserved. The handler becomes exact
when its real exit flag (273,332,652), per-arm update/member-dirty assignments
(624/625,632/633,640/641), and retail checkbox arm order are restored. All 34
retained direct calls and all 76 CFG blocks match retail; the other three
tracked functions stay exact.

The constructor's named-coordinate form has root `cb=4467`, initial budget
8934 and 58 candidate sites remaining at `reserve` (cost 169). Its nested
budget is therefore `floor((8934-169)/58) = 151`. `capacity` costs 42 and
`_Ucopy` costs 60; the small allocation wrapper does not consume this budget.
With 49 left, `_Destroy` (49) expands and the final `size` (42) is retained.
Retail makes the opposite pair of decisions.

Passing `29 + slot * 19` directly in each volume icon constructor preserves
retail's slot induction and removes two single-use coordinate bindings.
The verified trace then has root `cb=4457`, budget 8914, the same 58 sites,
and nested budget 150. After `capacity` and `_Ucopy`, `_Destroy` fails at 48
and `size` expands. All 231 blocks match, and all 6,268 function bytes outside
relocation operands agree, with identical 299 relocation positions. The
standard library and both real `updateSystemOptions` boundaries stay intact.

The sixteen-state family independently crosses the two coordinate bindings,
member initialization placement and the registration widget binding. Eight
objects reproduce. Either direct-coordinate loop suffices for an exact
constructor; retaining both named coordinates leaves 99.3013%. This is a
source-supported simplification of actual arguments, not extra work inserted
to consume budget. DC61/63/64 and 66/68/69 show the two slot loops, with no
named local inventory; they do not prove the temporary-coordinate spellings.

### A bound helper result changes a later return-copy decision

`getLevelString` is a real ordinary static helper: DC117 initializes its five
text pointers and DC119 reads the spell level, subtracts one, and indexes the
table. Binding that index before the lookup makes its C1 size 117 instead of
112. The `getSpellDescription` body stays unchanged (`cb=388`, budget 1000),
but its later string copy constructor receives root budget 367 instead of 372.
Its nested `_Tidy`, cost 152, then sees budget 149 instead of 152 and remains
a call, exactly as in retail. Replaying the same C1 streams through the clean
and traced C2 reproduces the object; all 36 blocks and 19 direct calls match.

Twenty-four helper/caller lifetime combinations emitted nine objects. Binding
the actual level, index, trait pointer, or trait reference all makes the
unchanged caller exact without moving any other unit score. The adopted form
binds only the index; flattening that expression is the 96.8000% negative
control. This is a real lookup value consumed by the return, not an extra
operation added to the caller. The canonical helper, its static table, and
all three source calls remain intact.

### A shared real loop index can change a string SIB tie

`button::main` had an otherwise identical 94-block body and 30-call stream,
with `_Eos` in the `setText` expansion encoding `[eax+ecx]` where retail used
`[ecx+eax]`. Reusing one unsigned index across the mutually exclusive key-down
and key-up searches, each initialized to zero, reproduces the retail encoding.
The canonical header setter and its string assignment stay unchanged. All
1,743 function bytes outside relocation operands then agree, including the
embedded dispatch data; the 46 relocation positions also agree.

This is a function-wide lifetime effect from two actual searches, not a
reason to change the string implementation or insert unused declarations.
The 24-state caller family reproduced ten retained candidates; keeping
separate indices leaves the SIB mismatch, and an extra text-argument local
is unnecessary. Dreamcast proves both searches but records no named locals,
so the shared index remains a retail-tested source hypothesis.

The same real-lifetime distinction explains `combatManager::drawFrame`'s
remaining two-load exchange after the priority traversal. Sharing the column
counter between the earlier underlay walk and the priority walk recovers
retail's ECX/EDI reload order. All loop initializations, bounds, steps and
helper calls stay unchanged. Six states produce two objects; moving only
the priority declaration is neutral, and the shared-column form keeps all
28 exact siblings. The source keeps DC's five recorded local types and
states explicitly that the unrecorded counter sharing is a hypothesis.
