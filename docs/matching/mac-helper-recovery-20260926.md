# Mac helper recovery follow-up — 2026-09-26

Branch: `codex/mac-helper-recovery-core-20260926`, based on `9594bf8ce`.
Windows is the rebuilt game. Mac is evidence for source calls and helper
boundaries; a Mac/Windows difference needs review rather than a forced call.

## Recovered source calls

- `LODFile::find` uses its two recursive self calls. VC6's tail-recursion
  optimization still produces the prior retail match (`lodfile` 9/9 exact in
  the focused build).
- Adventure callers now use the ordinary `updateRadar` and `completeDraw`
  wrappers at twelve Mac-retained sites. Both wrappers keep one source body.
- `heroWindow::widgetSetStatus` and `widgetClearStatus` use the existing
  four-argument `broadcastMessage` helper. The Mac graph now accounts for all
  95 retained calls to that helper.
- Mutable `town::getArmy()` calls replace artificial const casts in game,
  campaign, AI, overview and town-management callers. Corrected the two Mac
  overload address claims. Of 63 Mac mutable-getter sites, 59 now agree
  directly; four `townQuickView` calls are a documented source constness
  difference: Windows retail calls the const getter there.
- Six AI cost sites now call the existing player-ID `aiResourceCost` overload.
- `army::walkTo` again calls `cancelSpellType(AFTER_MOVE)`. Mac retains the
  call; VC6 emits byte-identical `walkTo` code with it because that helper arm
  has no effect in Windows.

The final spells review covered 98 remaining sites: 36 game calls are already
authored and 62 come from CodeWarrior string/vector operations in
`showSpellMessage`. Campaign, UI, systems and adventure follow-ups found
existing nested helper paths, implicit C++ lifetimes, or documented platform
differences at their remaining named sites.

## Audit tooling and coverage

`homm3 mac helper-audit` now joins reviewed Mac addresses to exact source
overloads even when Clang drops an annotation on a later redeclaration. Its
Clang-only parse view handles VC6 `min(long, …)`, RAD inline assembly, VC6
`functional`, and deque dependent-base syntax without changing game compiler
inputs. Authored `min` edges are normalized to the real int helper and marked
`analysis_compat`. Corrected VC6 loop scope and narrow type ambiguities in a
few owning sources so the graph can see their existing calls.

Full generated report: `build/mac/helper-audit.json` with
`helper-audit-calls.tsv` and `helper-audit-source.tsv`. Compared with the
prior handoff, directly agreeing Mac call sites rose **17,061 → 17,196** and
`missing_source_call` rows fell **546 → 506**. Clang-error units fell
**27 → 2**; the remaining units are `rmg` and `rmg_terrain`, deferred for
this pass. These graph states are leads, not byte or source verdicts.

The report still has **21,724 source-callee-unavailable** sites, **2,742
unresolved indirect destinations**, and **6,662 implicit-lifetime coverage
gaps**. They prevent a corpus-wide claim that every helper is accounted for.
The reviewed named non-deferred gaps are existing implicit operations, wrapper
paths, or platform differences; those exceptions remain visible in the queue.

## Build checkpoint

The integrated full build compiled all units and measured **4,287 / 4,782
exact MAX** functions (97.08% weighted). The generated README's filtered
executable scope is **4,274 / 4,769 exact MAX**, versus 4,307 / 4,768 at
branch start. We kept recovered helper calls through these score dips for the
later VC6 matching pass. A final focused rebuild of `advmgr` and
`multiplayerwindow` changed no MAX rows.

The full run first exposed a missing retail function carve: `sliderGames` at
VA `0x0050ee10`, 42 bytes. Retail disassembly shows a distinct function
between padded entries, so `config/retail/functions.tsv` now admits it. The
callback definition also now follows retail source order. Two other order
differences have reviewed backlog entries: `SavedGameHeader` needs its
original header ownership recovered, and `drawSmackerFrame` is currently
visible before its caller for VC6 expansion.

The full command still exits nonzero on wider repository gates: **40 / 73
admitted Mac pairs unavailable**, **62 source-ownership violations**, and
**64 source-inventory violations**. They are explicit comparison and source
coverage work, not a new helper-call verdict. The cleanliness gate now passes:
its linkage-block selftest was repaired, the audit-only sprite guard was moved
out of game source, and floors were reconciled with casts already present at
branch start. The combined source-graph and helper-graph contract suite passed
**23 tests**.
