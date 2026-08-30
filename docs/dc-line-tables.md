# The Dreamcast line/addr table — reading the original statement layout

*Opened 2026-08-14, on the round that closed
`viewarmywindow:??0TViewArmyWindow@@QAE@HHHE@Z`.*

The Dreamcast dump is usually consulted for **names, types and layouts**
(`docs/…`, `CLAUDE.md`'s evidence ranking). It carries two more records that
nothing in this tree was using, and they are the only artefact anywhere that
speaks about retail's **source text**:

* a **line/addr table** per compiland — `(source line, section-1 offset)`
  pairs, sorted by address; and
* an **S_BLOCK32 scope tree** per function — one record per lexical `{ }`,
  each with a start address and a byte count.

Neither says anything about x86 codegen. What they say is *which statement
each run of instructions came from*, which is exactly the question a
reconstruction is answering. Two divergence kinds fall out that no x86-side
lens can see:

* **a statement we split or merged** — three straight-line stores where the
  line table shows a counted loop, four assignments where it shows one
  `memset` call;
* **a call we spelled as a field read** — the DC compiland calls
  `town::HasBuilding` where our body tests `active & bitNumber[id]`.

Both change the /Ob2 candidate-site list (`docs/vc6/inliner.md` §2) even when
they are byte-neutral, so this table is the natural partner of the
site-count probe: **the probe measures how many sites are missing, the line
table says which statement carries them.**

## Reading it

The dump is `../homm3-symbols/HoMM3-Dreamcast-Dump/dump.txt`.

```
  E:\gamedcs\viewarmywindow.cpp, 0001:00190ABC-00192C87, line/addr pairs = 372

     55 00190ABC     57 00190B48     58 00190B54     61 00190B64
     …
    274 0019148C    278 001914F0    282 00191506    283 0019150C
```

`0001:XXXXXXXX` is a **section-1 offset** — the same number our `VA(...)`
comments carry as `dc 0x…` and `evidence/dc-xref-graph.tsv` as
`src_offset`. Two conversions:

| want | from `dc` offset |
|---|---|
| the next line's start (statement size) | the following entry, or the S_GPROC32 `Cb` |
| the **raw file offset** in `../orig/dreamcast/H3.EXE` | **`+ 0x400`** |
| the **linear VA** the literal pools hold | **`+ 0x11000`** |

**Address-base correction, 2026-08-14.** The first draft of this table said
"an RVA into H3.EXE = `dc + 0x1000`". Read off the DC PE header: the image
base is **0x10000** and `.text` is va 0x1000 / raw 0x400, so `dc + 0x1000` is
neither an rva nor a VA — it happened to work only because the same draft
subtracted it straight back to reach the file offset. The two numbers that
matter are in the table above, and the second one is load-bearing: literal
pools hold **linear VAs**, so decoding a call target needs `− 0x11000`, not
`− 0x1000`. `.rdata`/`.data`/`.pdata` are sections 2..4 at va 0x19e000 /
0x1a8000 / 0x1e1000 (add the image base for the VA), which is what lets a
pool entry that is a GLOBAL rather than a callee be named too.

The scope tree sits in the `S_GPROC32` record for the same address, with the
parameter and local `S_REGREL32` records ahead of it — those name the
**original parameter list**, which is independently useful (they proved
`get_luck_description`'s DC signature had no `creature` parameter at all).

## What the artifact proves about optimization

It does **not** prove a `/Od`-equivalent build. The embedded object paths name
the configuration `Win32__WCE_SH4__Release_with_debug`, and `S_COMPILE` names
Microsoft's SH compiler, but CodeView does not preserve the command-line
optimization switch. Existing corpus provenance therefore classifies this as
an optimized release build with full debug information. The defensible claim
is narrower and more useful: this older SH compiler/build preserved many more
helper calls, scopes, lifetimes, and statement boundaries than retail VC6
`/O2 /Ob2` did. Tool output exposes those surviving facts without assuming
that every non-inlined helper was caused by optimization being completely off.

## The tool

The public entry point is `homm3 dreamcast`. It joins the roster, retail
bridge, parameters/locals, lexical scopes and statement call/branch stream:

```sh
homm3 dreamcast show 0x00403ee0
homm3 dreamcast asm dc:0x1190
homm3 dreamcast asm dc:0x1190 --blocks
homm3 dreamcast find ClearBottomView
homm3 dreamcast gaps game::GetHero
homm3 dreamcast gaps --minimum 2 --limit 50
homm3 dreamcast gaps --exact 1 --retail-only --limit 0
homm3 dreamcast stats
```

Every view labels the result as cross-pressing analysis rather than retail
evidence. `show` and `asm` accept `module.obj:0xOFF` and an exact or
unambiguous function name. Use `--json` for machine-readable dossiers and
assembly/CFG records. Assembly line-program entries are labelled `bp`,
following Vostok's debugger-breakpoint terminology. `scope` labels are
CodeView `S_BLOCK32` lexical scopes; `B0`, `B1`, ... are separately inferred
SH4 basic blocks.

### Vostok-style empty source-line gaps

NB11 preserves the same clue Vostok's PDB carcass uses. The first line-program
row at a procedure boundary supplies the **procedure-frame line**, and the next
lexical row supplies the **first body line**. If those are lines 20 and 22, line
21 had no emitted line-program row. `homm3 dreamcast gaps SELECTOR` reports that
leading hole plus every other source-line hole in the selected function;
without a selector it ranks the whole Dreamcast corpus by leading-hole size.
`homm3 dreamcast show` includes the leading measurement in the dossier.

**Corpus snapshot, 2026-08-28.** The current line-table scan contains **2,365
Dreamcast functions with a reliable positive leading gap**, spanning 4,948
missing source-line numbers. Of those, 1,073 have the strongest simple shape:
exactly one missing line between the procedure-frame row and the first body
row. Restricting the scan to functions joined to retail gives **799 Dreamcast
functions mapping to 800 unique retail VAs** (801 mapping edges), spanning
1,845 missing entry-line numbers. The exact-one-line subset contains **297
Dreamcast functions mapping to 298 unique retail VAs**. Those 298 retail
functions are the first assertion-clue worklist:

```sh
homm3 dreamcast gaps --exact 1 --retail-only --limit 0
```

This records 298 candidates, not 298 assertions. The count is deliberately
defined in terms of the line-table shape so it remains reproducible as the
retail bridge grows; rerun the command rather than copying a hand-maintained
function list.

This is a candidate generator, not a source-text decoder. A missing row may be
a blank, comment, declaration, brace, preprocessor-only line, or a statement
the optimizer folded away. A release `assert` macro is therefore one important
possibility, especially when adding a zero-emission assertion changes VC6's
front-end inline budget while leaving emitted bytes unchanged, but the gap does
not identify that spelling by itself. Corroborate it with an assertion callee or
string, a sibling build/source, a source-line operand, or a controlled compiler
experiment. Minimal four-byte SH4 `rts; nop` bodies are excluded from leading-gap
inference so a closing-frame row cannot become a large false candidate.

Two vendored-zlib controls define the limits. `deflateInit_@16` has Dreamcast
boundary line 194 and first body line 196; zlib 1.1.3 line 195 is the opening
brace, so even a one-line leading hole is not intrinsically an assertion. In the
other direction, `_tr_flush_block`'s known release-disabled
`Assert(buf != (char*)0, "lost buf")` at `trees.cpp:964` **does retain an NB11
line row** at dc `0x18dc76`. The row owns 12 bytes of neighbouring branch/setup
code but contains no conditional assertion path or call; the assertion string
and `z_error` symbol are absent from the executable. This proves that a
compiled-out assertion can leak as a **ghost line attribution**, rather than as
an absent row or runtime assertion. Gap candidates and zero/borrowed statement
rows must therefore both be considered.

A full Dreamcast CodeView name/public/string scan found no generic `assert`,
`_wassert`, `DebugBreak`, or assertion-failure runtime signature. The surviving
`File`/`Line` format strings are adjacent to named DirectPlay and DirectDraw
error messages. There is currently no evidence that the Dreamcast game kept a
generic runtime assertion implementation; the proven leak is line metadata.

Unlike Vostok's per-symbol `lines_for_symbol()` view, the dumped NB11 line table
is flat per compiland. A preceding procedure's closing row can land exactly on
the next procedure's address. The scanner rejects a boundary row when it is
source-line-coherent with the preceding contiguous procedure rather than the
new body, and reports the leading gap as unavailable; this prevents those
borrowed braces from becoming high-ranked false assertion candidates.

The lower-level statement renderer remains available for development:

`scripts/homm3/analysis/dc_lines.py` does the whole join — line table +
scope tree + capstone SH4 + pool-symbol resolution — so no round of this
should be hand-rolled again:

```sh
cd scripts
python3 -m homm3.analysis.dc_lines 0x55df4          # statement listing
python3 -m homm3.analysis.dc_lines 0x55df4 --asm    # + SH4 disassembly
python3 -m homm3.analysis.dc_lines --find GetCurrTown
```

Each statement prints as `line N  dc 0xX  <bytes>  br=<conditional
branches>  { }` followed by the calls it makes, with `{`/`}` marking the
S_BLOCK32 scope opens and closes. That triple — statement size, branch
count, call list — is what a reconstruction is compared against.

Three practicalities the tool already handles, listed because they bite when
reading the raw dump:

* **literal pools sit inside functions.** SH4 materialises constants and call
  targets with `mov.l @(disp,pc)`, and the pool is emitted mid-body; capstone
  stops at it. Disassemble around the gap rather than assuming the function
  ended (`TViewArmyWindow`'s pool is 0x1925d8–0x192617 and code resumes at
  0x192618). `--asm` steps over pool halfwords and resumes.
* **a call is `mov.l <pool>,rN; jsr @rN`** (or `bsr` when it is near), so the
  pool entry is the callee — decode the `mov.l` halfword rather than trusting
  a disassembler's operand text, and track the last pool value loaded into
  each register (`jsr @r11` three times in a loop shares one pool entry).
  `evidence/dc-xref-graph.tsv` resolves the same edges by `src_offset` but
  gives a per-function SET, not the per-statement sequence; its `pool_refs`
  column counts literal-pool references, not calls, so it is a lower bound.
* **a `bsr` inside a literal pool is not a call.** Pool data decodes as
  instructions; treat a call whose target is mid-body as pool noise.

## The load-bearing caveat: the DC source is an OLDER REVISION

This is what makes the instrument sharp rather than merely corroborative.
`dc 0x19148c` line 284 is `mov.l @r10,r6 / bsr create_portrait_widget` — it
hands the portrait builder `traits->townType` straight through, with **no
version gate and no elemental test**, where retail's x86 body has both. The
same shows up on `armygrp::get_luck_description` (a `creature` parameter and
a whole clover-field arm retail has and the DC build does not) and on
`TBottomViewKingdom` (`town::HasBuilding` calls that retail spells as an
inline 64-bit mask test).

So the two directions are asymmetric, and both are useful:

* **DC has a structure, retail's bytes agree with it** → that is retail's
  source too, and the DC line table is proof of the SPELLING (the
  `for (i = 0; i < 3; i++) Influence[i] = -1;` case).
* **retail's bytes have code DC has no line for** → that code is a later
  edit, and it is exactly where a source-level element retail has and DC does
  not must live. On `TViewArmyWindow(int,int,int,unsigned char)` that
  argument is what identified the one missing /Ob2 candidate site: every
  other statement in retail's body has a DC line carrying the same call, so
  the post-DC version gate was the only place left for it.

Never read a *missing* DC call as evidence that retail has no call there.
`dc 0x19148c` does not reference `operator new` at all even though its body
allocates a `textWidget`.

## Running the table the other way, seven rows (2026-08-14)

The table was applied to every remaining row of the EH transcript
(`docs/vc6/eh-cleanup.md`) plus `TBottomViewKingdom`, cataloguing for each
the retail code that has **no DC line**. It answers a COUNT-class residual in
one of two ways and it is worth knowing which before spending a round on it:

| row | what the no-DC-line pass named |
|---|---|
| `bottomviewsubwindow:??0TBottomViewTown` | the whole quantity-text block (DC 477–492 have no code): the DC revision's army loop pushes the creature icon and nothing else. **Also a positive hit:** DC line 359 is a call to `game::GetCurrTown`, not `GetTown(currTownId)` — landed, 95.63 → 97.36 |
| `bottomviewsubwindow:??0TBottomViewHero` | DC line 229 is `game::GetCurrHero` — the twin. Landed, 96.52 → 97.77 |
| `armygrp:?get_luck_description` | two post-DC blocks and only two: the clover-field arm (DC 1482→1485) and the halfling arm (DC 1502→1505); retail's four `format_string` groups against DC's three corroborate the second exactly. **Positive hit:** DC line 1482 uses `operator=`, not `+=`, and retail's call there is `assign` — landed byte-flat |
| `bottomviewsubwindow:??0TBottomViewKingdom` | **NO post-DC region at all** — all 41 DC statements account for retail's body. Its missing site must therefore be a statement DC SPELLS DIFFERENTLY, and the table names the three candidates: line 524 is a `memset` where we write four assignments, lines 531/533/535 call `town::HasBuilding` where we test `active & bitNumber[id]`, lines 554/557 call `TTextResource::operator[]` where we call `GetText` |
| `quickherowindow:??0TQuickHeroWindow` | the DC revision formats army counts with `memset`+`sprintf` into `gText` (DC 166/174/177/188); retail uses `ostrstream`, so that whole block is post-DC. Retail's and our call sequences are otherwise 1:1 across all 59 calls |
| `campaignbrief:??1TCampaignBrief` | the DC dtor is line-complete; nothing post-DC to search |
| `ai_combat:?choose_melee` | line-complete, and the S_REGREL32 list names the two `type_AI_combat_data` locals whose copy ctors the EH transcript is arguing about |

Two rules fall out of doing it seven times.

* **Do the cheap positive read first.** Every landing above came from the
  *forward* direction — a DC statement that calls something our source
  spelled out by hand. That read costs one `dc_lines` run; the negative
  direction costs a full statement-by-statement alignment against retail.
* **An accessor the DC calls by name is worth trying even when the general
  accessor is already byte-close.** `game::GetCurrTown`/`GetCurrHero` had
  been recorded in `bottomviewsubwindow.cpp` as an unresolvable conflict with
  `GetTown`/`GetHero`, because the two call sites want opposite branch
  polarity and opposite compare widths from "the one header inline". They are
  two DIFFERENT header inlines, and the DC body says how the current-object
  pair is written: it re-reads the id (`GetCurrTown` calls `GetCurrTownId`
  twice) instead of taking a widened `int` parameter, which is exactly what
  holds retail's compare at char width.

### The forward read scales: `town::HasBuilding`, seven compilands (2026-08-15)

The row that named the instrument was the one it then closed.
`TBottomViewKingdom` had NO post-Dreamcast region, so its missing /Ob2
candidate site had to be a differently-spelled statement, and the table
named three. The live one was `town::HasBuilding`: dc 0x563b8 lines
531/533/535 are `mov #13/#12/#11,r5 / mov #1,r6 / jsr @r11` where our
body tested `active & bitNumber[HALL_*_ID]` by hand. Retail's own
out-of-line copy at 0x4305a0 supplies both arms - `check_included != 0`
reads `[ecx+0x158]` = active, `== 0` reads `[ecx+0x150]` = built - and
its inline expansion is byte-identical to the mask test, so the bytes
never arbitrate. Only the candidate-site count does, and three real
sites landed exactly on the +1 number the free probe had measured:
**94.0575 -> 98.5213**.

Sweeping the same call by `dc_lines` across the rest of the tree found
seventeen more sites in six compilands, every one of them a mask test we
had spelled by hand, and two more rows moved:

| row | before -> after |
|---|---|
| `bottomviewsubwindow:??0TBottomViewKingdom` | 94.0575 -> **98.5213** |
| `bottomviewsubwindow:??0TBottomViewTown` (all 7 source sites; peak retained) | **98.7476 peak -> 94.0054 current** |
| `quicktownwindow:??0TQuickTownWindow` (7 sites) | 96.3088 -> **98.4193** |
| `armygrp:?get_morale_description` (2 sites) | 67.5649 -> **74.4820** |
| `armygrp:?GetMorale`, `?GetLuck`, `?get_luck_description`, `game:?calculate_production`, `?HasCapitol`, `ai_player:?end_turn`, `ai_combat:?check_wall_archery_penalty` | byte-flat |

Two rules the round adds.

* **A header inline's VISIBILITY is a measurement, not a given.**
  Declaring `HasBuilding` unconditionally cost `initialize_game_data`
  100.0 -> 90.1620 (the include-set canary, one more town.h declarator)
  and `town::get_growth_rate` 100.0 -> 88.4737 (our /Ob2 budget expands
  the inline where retail's `0x5bfb60` plainly emits `push 0 / push 8 /
  call town_HasBuilding`). Scoping the declaration to the compilands
  that expand it, and the BODY to those same compilands only, gives the
  full-tree diff **one mover** - the intended one. `#pragma
  inline_depth(0)` around `get_growth_rate` was tried first and is
  worse (70.4210): it suppresses expansions that row needs.
* **When the expansion is byte-identical to the hand-written test, the
  percentage cannot arbitrate the source boundary.** `TBottomViewTown` has
  seven DC calls; all seven score 94.0054 while the two ladders alone peak at
  98.7476 (subsets: hall 96.8134, fort 97.3638, silo 97.1557, hall+silo
  96.0953, fort+silo 97.1960). The former reconstruction refused the silo
  site solely on that measurement. That was a source-shape regression, not a
  proved Complete edit: DC line 402 passes `MARKETPLACE_SILO_ID, 1`, and raw
  NB11 places the pointer local `resource` inside the resulting lexical
  block. Both facts are restored as of 2026-08-30. The 98.7476 peak remains
  history while a fatal asymmetric contract now rejects six calls, the wrong
  silo flag, a direct `active` mask, or loss of the scoped `resource` local.

  The restored call is itself instruction-identical at the silo test, but its
  extra `/Ob2` candidate site changes earlier inlining across the function:
  the candidate moves from 90 blocks / 41 branches to 88 / 39 while retail
  remains 90 / 41. Replacing either post-Dreamcast `std::ends` site with a
  direct `put(0)` was a bounded negative control: it restores the 90-block,
  41-branch sequence but reaches only 95.5920%, and the exact sibling uses the
  standard `std::ends` idiom. This residual therefore stays open; it is not a
  license to delete the seventh source fact again.

### The negative bound is a real deliverable

On `get_luck_description` the table cannot say what the row's measured +4
candidate sites are, because they live in code the DC build does not have.
What it CAN say is that they live in one of two named blocks and nowhere
else in a 968-byte body — and that the clover arm's own bytes (including its
longhand four-way elemental compare, the `is_base_elemental` shape) already
match retail exactly, so it is a site count and not a spelling. That narrows
the search from "the body" to "two blocks, neither of which is byte-wrong".
