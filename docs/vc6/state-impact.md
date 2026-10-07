# State impact: from a source edit to the functions it moves

Three C2 states let an edit change functions that it does not touch.
[phase-flag.md](phase-flag.md), [handle-period.md](handle-period.md) and
[unstable-state.md](unstable-state.md) describe them:

* the phase flag: only the first body C2 compiles in a TU receives 0;
* the live-range numbering keyed by handles modulo 64 (the handle offset);
* which inline callees C2 has already compiled (callee compile order).

This page derives, from those models, which functions an edit affects and
through which state. `homm3.vc6.state_impact` implements it as a predictor
that uses no trial compiles (§5). It was validated on targeted real compiles
(§6).

## 1. Does include order matter?

**Rarely, and only through handle numbers.** Include order never changes
the compile order of the TU's own functions, so it cannot move the phase
flag or the callee-compiled bit for them. It renumbers handles, so it
affects only the 18 period-sensitive functions in the corpus
(handle-period.md §4). Even for those, it changes code only when the swap
moves the creation point of a data symbol whose address the function uses.

Reasons, each measured (§3):

* **Headers emit almost no code at include point.** Every project header
  was compiled alone, to see which bodies its inclusion emits.
  * Only `terrain.h` emits code: ten namespace-scope `std::bitset<10>`
    masks, whose initializers are compiled where they are included.
  * Every header that pulls in `<string>`, `<bitset>` or `<istream>` also
    produces a library `_$E` initializer and `ctype<wchar_t>::id`. Both are
    deferred to the end of the TU (they appear last in every unit).
  * No header contains a non-inline function definition.

  Whether the TU's first function receives phase 0 therefore depends on
  whether `terrain.h` is in the **include set**, not on the order. It is
  only affected if a header with initializers is included *after* a
  function definition.
* **Inline helpers are emitted by first use, not by declaration.** Two
  headers were swapped while the use order in function bodies stayed fixed:
  * explicit inline functions came out in the same order;
  * template members came out in the same order.
* **Templates are deferred to the end of the TU** in first-use order, after
  every function they could be inlined into.

**The user's hypothesis, refined.** "Include order matters only if inline
helpers were used in functions above" describes the callee-compiled state,
not include order. Whether helper `h` is compiled before function F depends
on **which functions above F needed an out-of-line copy of h**. That is
definition order plus each earlier function's inline decisions. Include
order does not enter. Include order acts only through handle values,
independently of helpers.

## 2. The three maps

| state | what moves it | which functions it affects |
| --- | --- | --- |
| phase flag | whichever body is compiled first: the TU's first non-inline definition, or a namespace-scope initializer above it (only `terrain.h` provides one) | only the first and second bodies; the effect is real only for a function with a known-outcome branch in its opening sweep (phase-flag.md §6) |
| callee compiled | definition order; which earlier function first needs an out-of-line copy of an inline helper (a non-inlined call, address taken, EH unwind, vtable) | functions that inline that helper and lie between its old and new emission points |
| handle offset | any handle-consuming item before the creation point of an address-constant data symbol: declarations, includes (header cost), template instantiations, locals, blocks and literals in earlier bodies (handle-order.md) | the 18 period-sensitive functions, and only through the data symbols they use as immediate addresses (`push offset x`, `mov r, offset x`) |

Per edit kind:

| edit | phase | callee compiled | handles |
| --- | --- | --- | --- |
| swap or reorder includes | no change (headers emit no code before own functions) | no change | the swapped headers' symbols, plus headers included for the first time inside them, shift by the other header's cost. Symbols after both keep their numbers (the total is unchanged). |
| add or remove a declaration (header or .cpp) | no change unless it is a namespace-scope object with a dynamic initializer placed above the first function | no change | every symbol created after it shifts by its cost (handle-order.md table) |
| add a function | it receives 0 if it becomes the first body; the old first function then gets 1 | inline helpers it needs out of line are emitted after it, so they become compiled for the functions below it | parameters, `this`, block, locals and first-use literals shift everything after it |
| move a function | the first body may change | its group moves with it: the copies and implicit members emitted after it. Inline helpers it can need out of line are emitted after it when it now precedes their old emission point. | the functions between its old and new positions, and the symbols created there, shift by its cost |
| remove a function | as for a move | copies it alone needed are emitted later or not at all | as for a move |
| change which helpers an earlier function uses or inlines | no change | the helper's emission point moves to the first function that still needs it out of line | literals or locals in the edited body shift later symbols |
| edit inside the function itself | not covered here: that is a related edit | | |

## 3. Emission-order rules (measured)

C1XX emits function bodies to C2 in this order, which is also the COFF
section order and the link order (§6 has the identity test). The probes
are scratch files (`emit/t1…t11`), not committed.

1. Non-inline definitions come in source order.
2. A namespace-scope object with a dynamic initializer gets its `_$E` body
   at its definition point. The atexit funclet of a function-local static
   follows its function.
3. Non-template inline bodies are emitted right after the first function
   that needs them out of line: explicit `inline`, in-class definitions,
   inline virtual functions (emitted after the first constructor that
   references the vtable), implicit special members, `??_G`/`??_E` deleting
   destructors, and explicit specializations such as
   `char_traits<char>::copy`.
   * An inline function that is inlined everywhere is never emitted.
   * Address-of (t1, t8), recursion (t7) and an EH unwind call (t2) all
     trigger emission after that user, not after an earlier function that
     inlined it.
4. Template instantiations needed by the unit's own functions are
   **deferred** to the end of the TU, in first-use order, independent of
   include order (t10). Library statics such as `_$E` and
   `ctype<wchar_t>::id` come last.
5. Template members needed only by an inline copy may appear in that
   copy's group (observed in kb: `set<int>::~set` after
   `~combatManager`). The predictor keeps each such group intact.

**The compile order of a unit can be read off its object.** The predictor's
group model makes each own definition lead a group: its initializers above
it, and the copies and implicit members emitted after it. Rebuilding the
order from the own-definition sequence reproduces the actual object order
for **15/15 units tested**, including game, hero, events and
singleselectionwindow.

## 4. Handle-sensitive functions and their address constants

A function's relevant handles are those of data symbols that appear as
**immediate operands** in its code. The pseudo table hashes only those
operands; memory displacements are not hashed. This object-level rule
selected exactly the 22 symbols that the C2 trace logged for
`initializeGameData` and the 3 for `doAttack`. For example:

Where each of the 18 functions' address constants are created:

| function | address constants (creation point) |
| --- | --- |
| `initializeGameData` | `town::s_includedBuildings`, `g_townEligibleBuildMask`, `g_hierarchyMask` (`town.h`); 19 file statics (`initialize.cpp`) |
| `combatManager::getCommand` | `s_wallTargets` (`cmbtmgr.h`) |
| `CEnterNameEdit::onEnter`/`onKeyPress`/`onKillFocus` | `g_config` (`prefs.h`, the first include); `basic_string::_Nullstr`'s local static (instantiated at the end of the TU) |
| `aiEnterTown`, `town::initializeSpells` | `g_bitNumber` (and `g_mageGuildBaseSpellCounts`) (`town.h`) |
| `doAttack`, `monstersSellOut`, `recruitUnit::update` | `g_text` (`kb.h`) plus string literals |
| `transmitSaveGame`, `oldmain`, `loadTemplates`, `writeMapHeader`, `createArtifactWidgets` | mostly string literals (created at first use, often in the function itself) |
| `doCombat` | five globals from `remote.h`, `events.h`, `cmbtmgr.h`, `advmgr.h` |
| `giveArtifact`, `TRmgQuestCreatureDef::generate` | template statics, vtables, exception-info symbols (end of TU / unit) |

An edit shifts these symbols uniformly or not at all. A uniform shift by Δ
selects the table's offset class Δ mod 64. A mixed shift (some move, others
do not) is a new configuration that the table does not contain. The
predictor reports it as unknown.

**Consequence for include order.** A pure reorder keeps the total handle
count, so symbols created after the reordered includes keep their numbers.
A reorder therefore shifts a function uniformly only when **all** its
address constants are created inside the reordered headers. `getCommand`
(one symbol, in `cmbtmgr.h`) qualifies. `initializeGameData` (file statics),
the three `CEnterNameEdit` functions (the end-of-TU `_Nullstr` static) and
every literal-heavy function do not: a reorder gives them a mixed shift.

## 5. The predictor

```sh
python3 -m homm3.vc6.state_impact report UNIT            # compile-order groups
python3 -m homm3.vc6.state_impact predict UNIT EDIT [--verify]
#  EDIT: swap-include:A.h:B.h | insert:LINE:TEXT | move:VA:before:VA | remove:VA
```

`predict` combines three analytic inputs:
* the current object, for compile order, references and address constants;
* one C1XX front-end capture each of the current and the edited text, for
  handle deltas by name;
* the unit's state→bytes table from `homm3 vc6 compile-m`, which maps each
  single-axis state to an assembly and says which axes the function is
  sensitive to.

For each function it prints the states the edit changes and, where only one
axis changes, the predicted assembly. `--verify` compiles the edited text
once and compares every function.

## 6. Validation

Every case is a real compile of the edited unit, compared function by
function with the prediction. Each prediction was made before its compile.
"Unknown" means a mixed shift.

| unit | edit | predicted | result |
| --- | --- | --- | --- |
| initialize | `enum` of 12 enumerators at line 1 (+13 handles) | `initializeGameData` → offset class 13 (retail) | **changed as predicted**; 33 others unchanged as predicted |
| initialize | struct after the last function | no function changes | 34/34 as predicted |
| initialize | swap `terrain.h` / `town.h` | `initializeGameData` mixed shift (−209 / 0): unknown | it did not change; 23 others as predicted |
| initialize | typedef between the headers and the file statics | mixed (0 / +1): unknown | it did not change; 33 as predicted |
| drawing | swap `drawing.h` / `ai_tactical.h` | no function changes (no sensitive function moved) | 47/47 as predicted |
| drawing | move `combatMessage` before `showCreatureSpellError` | SCE phase 0→1 | **changed as predicted** (the retail bytes); 46 unchanged |
| drawing | add a small function above SCE | SCE phase 0→1 | **changed as predicted**; 46 unchanged |
| command | swap `cmbtmgr.h` / `combatwindow.h` | `getCommand` +4594 ≡ 50: same class | 67/67 as predicted |
| command | swap `cmbtmgr.h` / `game.h` | `getCommand` +26392 ≡ 24: class 1 | **changed as predicted**; 66 unchanged |
| mapcell | move `readBlackBoxData` above `readBlackBox` | `readBlackBoxData` prefix 2→1, same bytes | 305/305 as predicted |
| mapcell | move `readEventData` above `readBlackBoxData` | `readBlackBoxData` prefix 2→3 (`~BlackBoxData` now emitted after `readEventData`); `readEventData` 3→2 | **both changed as predicted**; 303 unchanged |
| singleselectionwindow | swap `prefs.h` / `<algorithm>` | 3 `CEnterNameEdit` functions mixed (+3461 / 0): unknown | none changed; 433 as predicted |

Across 12 edits, 1,447 function comparisons hold with no misprediction:
* 6 predicted changes all occurred;
* every predicted "no change" held;
* in 5 comparisons the prediction was unknown (mixed shifts), and none of
  those functions changed.

Two earlier model versions mispredicted. Both mispredictions changed the
model:
* `onKillFocus`: the immediate-operand test missed `mov [ebp-4], offset
  _Nullstr`, and an unresolved handle was silently treated as unmoved. Both
  are fixed; an unresolved handle now counts as a mixed shift.
* `readBlackBoxData` (second mapcell move): a pure group model missed that
  the moved `readEventData` becomes the first user of the implicit
  `~BlackBoxData`. Fixed by the candidate re-anchoring rule (§3, rule 3).

Contract tests: `scripts/homm3/vc6/test_state_impact.py`. The group model
reproduces the object order of 15/15 units (§3).

## 7. Gaps

* **Mixed handle shifts are unresolved.** To decide them, the predictor
  needs the constant pseudos' buckets (the integer constants hashed beside
  the addresses, handle-period.md §1). They are visible only through the
  C2 trace (`HOMM3_VC6_PK_LOG` in the scratch shim), not in the object. All
  5 mixed cases tested were unchanged, but that is not a rule.
* **The emission trigger is approximate.** An inline copy is emitted after
  the first function that needs it out of line. The object shows only users
  whose final code references the copy. The re-anchoring rule uses the
  inliner's candidate list, which over-approximates "needs it out of line".
  Where a function would inline the candidate everywhere, the rule may
  predict a prefix change that does not happen. This was not observed in
  the tests.
* **Combined states are not in the table.** `compile-m` sweeps the three
  states separately, so an edit that changes two of them for one function
  is reported without an assembly. A function move that also shifts its
  handles is an example.
* **Phase is assumed to apply to every body.** A function under
  `#pragma optimize("g", off)` does not run the global optimizer and does
  not change the flag (phase-flag.md §6). The predictor does not read
  pragmas.
* **Some function bodies have no VA annotation.** Unannotated definitions
  in the .cpp (for example `saveBlackBoxList`, which shares
  `readBlackBoxData`'s VA block) travel with the preceding group. A move
  edit moves a whole VA block.
* **Retail places header initializers last; our builds compile them
  first.** The `terrain.h` mask initializers are compiled first in our
  builds, and in units that include `terrain.h` they make the first own
  function receive phase 1. Retail places every unit's mask funclets at the
  object's tail (for example iconwdgt and initialize, terrain.h's own
  comment). Either the
  original header's objects were initialized in a deferred form, or retail
  compile order differs here. This is open, and it matters for the phase
  of the first function in 78 units.
