# The VC6 /Ob2 inliner — the reverse-engineered decision rule

The `/Ob2` auto-inline decision of the pinned back end, read out of the
binary: **C2.DLL 12.00.8447** (sha256 gated by `_toolchain.PINNED`, image
base `0x10700000`; all addresses below are image-relative rvas). Phase 3 of
the `vc6` area: this document is the model spec + address ledger + validation
record; the executable model is `predict()` in
`scripts/homm3/vc6/inline_model.py` (`--predict --selftest` replays the
validation), and the Ghidra-side queries that produced the evidence are
`scripts/homm3/vc6/ghidra_scripts/inline_probe.py` (read-only over the
persisted `build/re/vc6/` atlas project; `dump` / `refs` / `callers`).

Provenance: binary-only RE of our own pinned compiler + oracle compiles with
that same compiler (behavior-catalog method). No external source consulted.
Shape per the homm2 `od-stack-layout.md` doctrine: methodology, ledger,
validated predictor, and what the model does NOT cover, in one place.

## 0. The headline: the STL-inline verdict

> **CORRECTED 2026-08-19 — the ~65% figure below was a measuring artifact and
> the `game::Load` example was backwards.** `predict-inline` compared the two
> sides' call multisets BY SYMBOL NAME, and the sides do not name a callee the
> same way: our obj emits the real mangled symbol, the delinked target carries
> the synth-PDB label for anything unclaimed (`sub_f6570`,
> `game_8a310_sub09_d4aa0`, `hero_load`, `exe_new`). Every such call booked
> twice — an under-inline on our side and an over-inline on retail's — and
> because `_route` puts the inliner upstream of registers and blocks, the
> phantom buried the true diagnosis. With the names paired off
> (`inline_model.divergence`, negative controls in `test_inline_names.py`) the
> inliner class falls from **135 of 211 plateaus to 46**, and
> **register-homing (108) is the dominant class**, not the inliner.
>
> `game::Load` measured again with names resolved: 92 out-of-line calls
> against retail's 80, of which **30 pair off by name**. What actually
> survives is three rows — `~NewSMapHeader` 2 vs 1, `IsLocalHuman` 0 vs 1, and
> `_Tidy` **38 vs 39**. Retail calls `_Tidy` *more often than we do*; the
> claim below that "retail inlines 37 `_Tidy` … our compile emits them all as
> calls" is the opposite of what the objects say, and the 5 `vector::insert`
> it cites are `game_8a310_sub09_d4aa0` under another name. The §E6 standalone
> experiment is untouched by this — it never used the multiset comparison —
> so the budget-dynamics mechanism stands; only its claimed REACH was inflated.

The plateau diagnoser found ~65% of the residual walls are inline
divergence dominated by STL (`game::Load`: retail inlines 37
`basic_string::_Tidy`, 5 `vector::insert`, `size`/`erase` and small dtors —
our compile emits them all as calls). **Verdict: this is a C2 budget-dynamics
divergence, not a front-end marking or header difference.** Proven standalone
(E6 below): with the SAME pinned toolchain and the SAME `<string>`, `_Tidy`
flips from fully-inlined to fully-called purely as a function of the
*caller's* front-end size estimate and the number of candidate sites:

| caller | `_Tidy` sites | `_Tidy` out-of-line |
|---|---|---|
| 1 string local, no padding | 2 | 0 (all inlined) |
| 12 string locals | 24 | 23 |
| 24 string locals | 48 | 47 |
| 12 string locals + 300 pad statements | 24 | **0 (all inlined)** |

Mechanism (§2): the per-caller budget is `clamp(2 × caller_cb, 1000, 35000)`
where `caller_cb` is the **front end's size estimate of the caller itself**,
and the budget handed to nested (depth-2) expansions is the remaining budget
**divided by the number of candidate sites still ahead**. A big caller gets a
big budget; a small caller with many sites starves every nested `_Tidy`.

Matcher guidance:

* **Under-inlined STL in a big caller means the caller reconstruction is
  lighter than retail's source.** Retail's `game::Load` presents a large
  `cb` → budget up to 35000 → everything small inlines. Our partial/leaner
  reconstruction presents a small `cb` → the 1000 floor → everything starves.
  Finish the body; the inlining follows. Do not chase `_Tidy`/`vector`
  spellings, pragmas, or header variants — they are not the input.
* The budget lever is *statement mass*, not bytes: dead stores and other
  byte-inert statements move `cb` (this is A6's real mechanism). On a
  byte-plateaued function whose only residual is an under-inline (A9,
  `do_general_melee`), the honest fix is raising `caller_cb` past the
  knife-edge or slimming an earlier callee's `cb` — quantified by the model
  (§5.8).
* A15 ("leaf spelling is a global variable") now has a mechanism: a leaf's
  `cb` enters every caller's sequential budget arithmetic, so respelling a
  leaf re-decides inline structure at every call site in the image.

## 1. Where the inliner lives (and why the atlas pointed one region over)

The C2 atlas anchors `inline.c` at `0x94521` (span `0x93c23`–`0x95e9f`).
That region turns out to be the **cold half**: C2.DLL 8447 is block-reordered
(BBT-style) — hot paths of many TUs are packed into the string-less front
region (`0x1000`–`0x6a9fc`), while each TU's anchored region keeps its cold
blocks and ICE sites. Concretely, `0x93f71` is two instructions —
`mov [0x107ac094],ecx; jmp 0x10719dea` — a cold stub jumping into the hot
driver at rva `0x19dea`. The working inliner is:

| function | rva | role |
|---|---|---|
| `inl_main` | `0x1994f` | per-function pass entry: budget init, calls the expander |
| `inl_expand` | `0x199fa` | the recursive sequential accept loop (the rule) |
| `inl_collect` | `0x1a27c` | candidate-site collector over the tuple stream |
| `inl_candidate_ok` | `0x16f04` | callee flag gate (bit 0x40 body-saved, /Ob mode, 0x200) |
| `inl_fetch_body` | `0x1b973` | fetch the callee's stored front-end tuple stream |
| `inl_veto` | `0x94964` | post-substitution size veto (cold; option-gated) |
| option-bit unpack | `0x1bd89` | per-invocation flags → `0xac0**` dword bits |
| budget clamp stub | `0x93d28` | `mov eax,0x88b8` — the 35000 cap (cold) |

Correction to the atlas's §3 hunting list: the `.databe` cluster
`0xac094`–`0xac0d4` is **not** budget state — `0x1bd89` shows those dwords
are unpacked option BITS (`0xac094`=bit25, `0xac098`=derived, `0xac0a0`=bit18,
`0xac0b0`=bit23, `0xac0d4`=bit24, `0xac054`=bit21 = the `/Ob` auto-inline
enable). The real mutable state is one global (`0x9f234`, below) plus stack
locals of `0x199fa`.

## 2. The rule

All of it, byte-proven (ledger in §3), implemented 1:1 in `predict()`:

```
inline_pass(caller):                                   # 0x1994f
    running  = cb(caller)          # DAT_1079f234 := caller's IL size estimate
    budget   = 2 * cb(caller)
    if budget < 1000:  budget = 1000                   # floor
    if budget > 35000: budget = 35000                  # cap (0x93d28; NOT a bail)
    expand(caller_body, depth=1, budget)

expand(body, depth, budget) -> spent:                  # 0x199fa, recursive
    sites, n = collect(body)                           # 0x1a27c, tuple order,
    #   candidates only: flags&0x40 (body saved by C1XX), /Ob2-auto or
    #   inline/forceinline-marked, !(flags&0x200), cb < 1000 [DAT_10799280],
    #   recursion guard flags&0x10 unless inline_recursion (state & 0xf00);
    #   each site records the lexical #pragma inline_depth value (low byte
    #   of the opcode-0x1b8 state tuple)
    for k, site in sites:                              # n counts DOWN per site
        cb = int16(callee.cb)                          # SIGNED 16-bit (movsx)
        reject if arg-tuple count != formals + hidden  # 0x19f63
        reject if depth > site.inline_depth            # 0x19f7c
        unless callee.forceinline (flags & 0x2000):    # 0x19f87
            reject if budget < cb and cb > 0x28        # 0x19f97 + 0x19a8a
            reject if running > 35000                  # 0x19f9f
        on reject: C4710/C4714 if callee marked inline # 0x2c6/0x2ca
        # accept:
        unless forceinline:
            if cb > 0x28: budget -= cb                 # 0x19bac; cb<=40 is FREE
            running += cb                              # 0x19fec
        C4711 if callee NOT marked inline              # 0x2c7
        body' = fetch_stored_body(callee)              # 0x1b973
        spent' = expand(body', depth+1,
                        budget / (n - k))              # 0x1a0cc: idiv by the
        #                                                sites REMAINING (incl.
        #                                                current); trunc division
        unless forceinline:
            budget -= spent'; running += spent'        # 0x1a0f9-0x1a10a
        # option bit23 clear: post-substitution veto 0x94964 may still
        # revert the copy (the cb charge is NOT refunded)
    return budget0 - budget                            # 0x19a51-0x19a55
```

Consequences worth naming:

* **Sequential, positional exhaustion** (A9): the budget is spent in tuple
  order; the last sites lose. Confirmed to the instruction in E1/E5.
* **Nested budgets shrink fast**: `budget / sites-remaining` at every level.
  "Depth-2 stops" (A8/A10) are *not* a depth limit — they are this division.
  The depth check proper is the per-site `#pragma inline_depth` byte
  (default 8), which nothing in the corpus ever hits.
* **Small callees are free** (`cb <= 0x28`): inlined regardless of budget
  and site count (E2: 60/60), bounded only by `running <= 35000`.
* **The caller's own size RAISES its budget** — the single most
  counter-intuitive prediction, confirmed in E3: padding a caller flips
  rejected sites to expanded.
* **`cb` is a signed 16-bit field**: estimates past 32767 wrap negative and
  the budget collapses to the 1000 floor (E3 pad=4500).
* `DAT_1079f234` is *per-caller* running post-inline size (reset at pass
  entry), not a module accumulator.

## 3. Address ledger

| claim | rva | bytes / instruction |
|---|---|---|
| pass entry; `DAT_1079f234 := caller cb` | `0x19962` | `a3 34 f2 79 10` after `movsx eax, word [eax+0x6d]` (`0x1995e`) |
| budget = 2×cb | `0x19967` | `add eax,eax` |
| floor 1000 | `0x19969`/`0x19970` | `cmp eax,0x3e8` / `mov eax,0x3e8` |
| cap 35000 (cold clamp, not a bail) | `0x199da` → `0x93d28` | `cmp eax,0x88b8; jg` → `mov eax,0x88b8; jmp 0x10719975` |
| top call: depth=1, flag=0 | `0x19976`–`0x19986` | `mov edx,1; push 0; push eax; call 0x107199fa` |
| expander frame; depth stored | `0x19a08` | `mov [esp+0x28],edx` |
| collector call (&site-count out) | `0x19a2c`–`0x19a30` | `lea edx,[esp+0x28]; call 0x1071a27c` |
| per-site counter decrement | `0x19eb7`/`0x19ebe` | `dec edx; mov [esp+0x30],edx` |
| arg-count check | `0x19f59`–`0x19f6a` | `movsx edx,word [edi+0x6b]; add; cmp esi,edx` |
| depth vs site allowance | `0x19f70`–`0x19f7e` | `mov eax,[ebx+8]; and 0xff; cmp; jg reject` |
| forceinline bypass | `0x19f87` | `test ah,0x20` on `[edi+0x73]` |
| budget test (signed) | `0x19f8c`–`0x19f99` | `mov ax,[edi+0x6d]; movsx; cmp esi,edx; jl` |
| small-free escape | `0x19a8a` | `cmp ax,0x28; jle` (back to accept) |
| running cap | `0x19f9f` | `cmp dword [0x1079f234],0x88b8; jg reject` |
| budget charge iff cb>0x28 | `0x19fd6`–`0x19fde`, `0x19bac` | `cmp ax,0x28; jg` → subtract |
| running += cb | `0x19fec` | `add [0x1079f234],ecx` |
| C4710/C4714 (reject, marked) | `0x19a94`, pattern | `test [edi+0x73],0x2080` … `0x2c6 + (forceinline ? 4 : 0)` |
| C4711 (accept, unmarked) | via `0x93dd2` cold | warn id `0x2c7` |
| fetch stored body | `0x1a000` | `call 0x1071b973` |
| recursion: depth+1, budget/remaining | `0x1a0cc`–`0x1a0dd` | `mov eax,[esp+0x4c]; cdq; idiv [esp+0x34]; inc edx; call 0x107199fa` |
| spent charge after recursion | `0x1a0f9`–`0x1a10a` | `sub ecx,esi; add eax,esi` on budget slot + `0x9f234` |
| return spent | `0x19a51`–`0x19a55` | `mov eax,[esp+0x30]; sub eax,ecx` |
| collector: `#pragma inline_depth` state tuple | `0x1a27c` body | tuple class `0x16`, opcode `0x1b8` → carried word; site node `[2]` |
| collector: candidacy cb filter | `0x1a27c` body | `cb < DAT_10799280` unless option bit23 |
| `DAT_10799280` default 1000 | `0x7d6a7` (main.c) | `mov dword [0x10799280],0x3e8` |
| candidate flag gate | `0x16f04` | flags&0x40 && (bit21 \|\| flags&0x2080) && !(sym+0x14&0x1400) && !(flags&0x200) |
| option-bit unpack | `0x1bd89` | bit21→`0xac054`, bit23→`0xac0b0`, bit24→`0xac0d4`, bit25→`0xac094` |
| post-substitution veto | `0x94964` | limit `(nargs+2) * DAT_107ae244`; classes `0x0c`:+1, `0x0e`/`0x12`:+2 |
| `DAT_107ae244` = 3 | `.databe` init | file bytes (no code writer) |
| cb/flags are FRONT-END fields | `0x1d23d` | `mov [ebp+0x6d],ax` — the only writer in the DLL, inside the IL symbol-record reader `0x1ce0b`; `0x1c8a3` is a varint reader over the IL FILE stream |

Symbol-record fields used (offsets on C2's symbol records): `+0x6d` int16
size estimate (`cb`), `+0x6b` int16 formal count, `+0x73` flag dword
(`0x40` body-saved/inline-eligible, `0x80` inline-declared, `0x200`
never-inline, `0x2000` `__forceinline`, `0x10` on-expansion-stack guard).

## 4. The cost input `cb` — front-end owned, measured empirically

`cb` is **computed by C1XX and shipped in the IL**; C2 only compares it.
Its formula was not reverse-engineered (C1XX is a separate phase); it is
*measured* through the budget rule itself: with a small caller the budget is
exactly 1000, so `expanded = floor(1000 / cb)` and counting rejected sites
brackets `cb`. `inline_model --measure-cb TU --fn CALLEE --caller CALLER
--sites N` automates one such titration (**count rejected = out-of-line
`call`s PLUS the tail `jmp`** — VC6 tail-jump-optimizes a rejected final
site; forgetting the jmp cost this investigation an off-by-one and is also
why the a06 catalog entry reads "7 expansions + 2 calls" for what is really
6 + 3).

Calibration staircase (25-site harness, statements `gA[i] = gA[i+1] + row;`):

| statements S | expanded | cb bracket |
|---|---|---|
| 1–2 | 25 | ≤ 40 (free) |
| 3 | 20 | [48, 50] |
| 4 | 16 | [59, 62] |
| 6 | 11 | [84, 90] |
| 8 | 9 | [101, 111] |
| 12 | 6 | [143, 166] |
| 13 | 5 | [167, 200] |
| 14+ | 0 | **not a candidate** |

Roughly `cb ≈ 15 + 11.5 × S` for simple statements; heavier statements cost
more (a06's subscript-XOR loop body: 5 source statements ≈ cb 143–166; a
plain call statement ≲ 13). Measured game shapes: `get_total` (ternary form)
cb ∈ [46, 47]; `kill` cb ∈ [77, 83].

**The save-gate cliff.** At S=14 the callee stops being expanded anywhere —
a *binary* candidacy drop, not a cost jump: either C1XX stops saving the
body (flags bit `0x40`) or ships a sentinel `cb ≥ 1000` (the collector's
`DAT_10799280` filter); the two are indistinguishable without an IL tap.
This cliff — not the budget — is a06/A6's "wildly disproportionate"
`fill_storedec` collapse: one dead pre-loop store pushes the body over the
save threshold and ALL nine sites become calls. The threshold is
shape-dependent (13→14 simple statements; 5→6 statements for the a06 loop
shape) and stays a front-end unknown; the model takes it as the boolean
`candidate` input.

## 5. Validation record (2026-08-10, pinned SP3 CL under Wine)

Every case below is a real-compiler measurement; `--predict --selftest`
replays all of them through the pure model (9/9 PASS; during development the
gate demonstrably failed on wrong parameters, so it can fail). "Rejected"
always counts `call` + tail `jmp`.

1. **E1 / a06 `fill_plain`** — 9 sites, small caller: 6 expanded + 3
   rejected. Model: `floor(1000/cb) = 6` for the whole measured bracket
   cb ∈ [143,166].
2. **a06 `fill_storedec`** — 0 expanded + 9 rejected: the candidacy cliff
   (§4), reproduced by `candidate=False`.
3. **E2 small-free** — `gAcc += a;` callee at 60 sites: 60/60 expanded.
4. **E3 caller-size coupling** — padding the *caller* flips 6→8→9
   expansions (pad 40→45→50); the model's `budget = 2×cb` coupling. At
   pad≈3000 the running-cap zone appears (6 expanded), at pad≈4500 the
   int16 wrap reverts to exactly the 1000-floor answer (6) — both shapes
   the model reproduces qualitatively (the pad statements' own cb is not
   modelled; the wrap and cap are).
5. **E5 nested division (the A9/do_general_melee shadow)** — `ff` calls
   `gg`×6, `gg` calls `hh`×3, cb(hh) ∈ [143,166]: model predicts every `gg`
   expands, and *exactly one* `hh` per copy (nested budgets
   1000/6=166 … 250/1=250 each fit one hh) = 6 expanded + 12 rejected.
   Measured: 11 calls + 1 tail jmp = 12 rejected. Exact hit.
6. **E6 STL flip** (§0 table) — same header, `_Tidy` fully inlined or
   fully called purely by caller size / site count.
7. **E7/E8** — the staircase and the S=14 cliff (§4).
8. **A9 `do_general_melee` (in-tree)** — candidate sites in tuple order:
   `kill`, `inflict_damage`, `kill`, `inflict_damage` (cb: get_total
   [46,47], kill [77,83] measured). Our observed structure — `get_total`
   expanded inside kill copy 1, a CALL inside copy 2 while kill itself
   expands both times — holds in the model iff the budget left at kill#2
   is in [80, 172), i.e. the `inflict_damage`#1 subtree spends 708–787 of
   the 1000 floor (window derived by running the model backwards from the
   observed decision; a full forward derivation needs cb of the
   `inflict_melee_damage` tree). Retail's side of the knife-edge
   (`get_total` expanded in BOTH copies) appears as soon as `caller_cb`
   rises (e.g. ≥ ~590: budget 2×cb clears the same site) — consistent with
   our reconstruction being statement-mass-lighter than retail's source, and
   with every callee-respelling attempt at this site failing (the body's
   bytes were never the input).
9. **`TBottomViewKingdom` (in-tree, 2026-08-14) — the site-count lever,
   isolated.** The sharpest confirmation of `budget / sites-remaining`
   from a real matching row, because the knob is the SITE COUNT alone
   with the caller's bytes held fixed. Retail keeps
   `vector<widget*>::size()` (`0x423110`) out of line inside `reserve()`
   where our compile expands it; `capacity()` is inlined on both sides,
   so retail's nested budget died between two 19-byte callees.
   Introducing exactly one extra **free** candidate (cb ≤ 0x28, so it
   takes no budget charge and emits no bytes — `Widgets.size()`,
   `capacity()` and `empty()` are interchangeable) moves the function
   94.06 → 98.52 and the residual from ten divergent blocks to two
   size-only ones. The rule's position semantics fall out exactly:

   | extra free sites | placed BEFORE `reserve(8)` | placed at/after it |
   |---|---|---|
   | 1 | 94.06 (inert, 3 placements) | **98.52** (8 placements) |
   | 3 | — | 95.88 |
   | 8 | — | 95.88 |

   A site before `reserve` cannot help because it raises the loop index
   and the count together, leaving `sites-remaining` at reserve
   unchanged; only a site at or after it raises the divisor. Sibling
   rows in the same TU show the same lever with their own thresholds
   (`TBottomViewTown` peaks at +3, `TBottomViewResourceMessage` at +5,
   `TBottomViewHero` is already at retail's count and only loses), which
   makes "the reconstruction is a few candidate sites lighter than
   retail's source" a *measurable* per-function quantity.

   **Negative control on the rival hypothesis.** The same four rows were
   swept with 45 include-set / handle-order probes (six declaration
   kinds at counts 1..256, two localities, plus eight mutations of the
   include list itself). All byte-flat, while `il-diff` proves each
   probe reached the front end (gl high-water +9, ex divergence across
   392 function spans; adding a header moved 118700 ex bytes). Inline
   structure in this TU is a C2 budget quantity and is not reachable
   from C1 handle numbering.

10. **The lever reaches the MEMBER-INITIALIZER prologue too (2026-08-14)** —
    and this retracts a published negative. `viewarmywindow`'s
    `TViewArmyWindow(int,int,int,unsigned char)` had been recorded as
    "97.1049 at +0,+1,+2,+3,+5,+8 — the body's site count does not reach a
    mem-init expansion". Re-run with real free candidates
    (`Widgets.capacity();`, cb ≤ 0x28, emitting no bytes) it goes
    **97.1049 → 99.9352 at +1**, flat at +2 and +3, and 92.8920 at +6. The
    divergent decision is the SECOND `std::string` member's constructor,
    which sits one index after the first in the same site list: their
    allowances differ only by `(n-1)/(n-2)`, and `cb` of
    `basic_string(const allocator&)` falls inside that gap, so the window is
    exactly one site wide. The prologue is therefore NOT accounted separately
    — it is simply at the head of the list, which is also why any body site
    helps there while §5.9's placement rule still binds for divergences
    further down. Whatever the earlier probe appended was not a candidate.

11. **The `viewarmywindow` +1 is LANDED and the row is EXACT (2026-08-14).**
    The site the model demanded is real: it is the elemental/version gate the
    Dreamcast build of that file does not have at all (`dc 0x19148c` line 284
    hands `traits->townType` straight to the portrait builder), spelled as a
    call to a free predicate. Every other statement of retail's body has a DC
    line of its own carrying the same call, so the post-DC edit was the only
    place left for a candidate site to hide — `docs/dc-line-tables.md` is the
    instrument. Worth 97.1049 → 99.9352 alone; a second finding off the same
    table (the three `Influence[i] = -1` stores are a counted `for` loop,
    which VC6 unrolls back into retail's three stores and which recovers
    retail's EDX for the −1) took it to 100.0000. **A free candidate site is
    an instrument, not a fix** — it says the deficit is exactly one; the line
    table says which statement carries it.

12. **Per-row deficits measured with the same probe (2026-08-14)**, the other
    two still unlanded: `bottomviewsubwindow:??0TBottomViewTown` +2
    (95.6295 → 97.4523; re-measured 2026-08-14 after that row's
    `game::GetCurrTown` landing it is STILL +2, 97.3638 → 98.8631, flat at
    +3 — the accessor bought a site but not this one),
    `armygrp:?get_luck_description` +4
    (82.5689 → 90.1916). Every one of them is the same direction — **our
    reconstruction lets the allowance run one level deeper than retail's**
    — which is the mirror image of §0's under-inline guidance and the
    dominant residual class behind the EH-transcript rows
    (`docs/vc6/eh-cleanup.md`).

13. **A zero-byte real statement can select the exact nested frontier
    (2026-09-01).** `TSingleSelectionWindow::~TSingleSelectionWindow` first
    reached 96.98% with 33/34 exact CFG blocks. Its only residual was the
    final `HeadersA` member teardown: our caller retained
    `~GameSelectionHeadersStruct`, while retail expands that outer destructor
    and retains its nested `~SavedGameHeader` call. A cleanup-region
    `inline_depth(1)` probe overshot in the opposite direction (two outer
    expansions), proving the nested frontier but not supplying a fix.

    Dreamcast's final line rows supplied the missing source fact instead:
    `del_Spr_from_Cache()` is guarded by `!m_flag65`. The Complete helper
    itself folds to no instructions, but restoring the real conditional moved
    C1 to retail's exact midpoint: 100% bytes and 34/34 exact blocks. This is
    the operational order for a reciprocal outer-under/inner-over report:
    inspect the Dreamcast line/scope rows for a missing real guard, helper,
    RAII boundary, or release-elided operation immediately before cleanup;
    use depth probes only to classify the frontier. Never retain synthetic
    free calls, expose a nested library internal, or apply TU-wide
    `auto_inline(off)` to manufacture the midpoint. `predict-inline` now emits
    this repair path when it can prove that the under-inline outer callee calls
    the over-inline inner callee.

14. **A tail-site probe does not identify a missing tail statement
    (2026-09-06).** `VWDrawGround` (`0x5fa1e0`) expanded its last clipped
    scaler where retail calls it. A temporary free candidate after the scale
    call could flip that decision, but Dreamcast supplied no tail statement.
    The natural correction came from two existing source relationships:

    - `CSprite::DrawTile`'s bitmap overload calls `GetMap`, `GetWidth`,
      `GetHeight`, and `GetPitch` (DC `CSprite.h:393`). Restoring those
      flattened accessors selects retail's clipped-scaler calls and raises
      57.8477% to 96.8937%, with one extra `GetMap` call remaining.
    - DC `viewwrld.cpp:1194-1258` puts the border arm before the normal-tile
      `else` arm. The reconstruction inverted them and returned early.
      Restoring the source order removes that final extra call: **100%**,
      64/64 exact blocks and six matching calls, with the original inline
      declarations retained.

    The probe isolated sensitivity to the site's allowance. It did not prove
    where the missing source fact belonged: earlier helper costs and branch
    order were sufficient. These measurements preserve one canonical scaler
    and bitmap accessor implementation; no synthetic tail operation remains.

15. **A flattened nested helper can make a natural outer expansion seem
    impossible (2026-09-06).** `HandleLowLevelMsg` (`0x552db0`) was capped at
    50.5525% behind a synthetic `RemoteCleanupInline` force-inline wrapper.
    Removing the wrapper while retaining its flattened chat loop leaves
    `RemoteCleanup` out of line and scores 32.7626%. Dreamcast provides the
    missing nested boundary: `RemoteCleanup` calls `CChatManager::ClearChat`
    at `remote.cpp:1412`. Restoring that ordinary call makes the ordinary
    outer cleanup expand and selects both retail deque `_Buyback` calls:
    **99.9543%**, all 26 blocks exact. No switch reordering or caller carrier
    is required. The standalone cleanup and ClearChat remain exact.

    The remaining ten differences are stack/frame operands. Passing
    `&CPingResponseMsg(...)` directly into the send call, instead of naming
    a local, restores retail's separate `[ebp-0x38]` response slot and
    `0x138` frame: **100%**. Dreamcast line 310 passes the constructor's result
    directly to `TransmitRemoteDataDPID`; only the chat buffer has a recorded
    local name. This is a VC6-accepted temporary-address expression, with
    lifetime through the call. The named-local control remains 99.9543%.

    All callers now use one ordinary `RemoteCleanup` definition.
    `LobbyLaunchConnect` and `HandlePlayerDead` retain their current exact
    bodies; `HandleMPlayerLaunch` falls to 84.5139% while its 100% MAX remains
    banked. That collateral does not justify restoring the synthetic wrapper.

## 6. What the model does not cover

* **The veto (`0x94964`)**: post-substitution re-walk, limit
  `(nargs+2)×3` over statement-class tuples, active only with option bit23
  clear; vetoed sites keep their budget charge. Under our `/O2 /Ob2` profile
  no experiment required it (E7's straight-line rejections are explained by
  candidacy), so `predict()` omits it; if a divergence ever needs it, the
  charge-without-refund asymmetry is the fingerprint to look for.
* **C1XX's cb formula and the save-gate measure** — front-end territory
  (phase for the IL tap / C1XX RE); the model takes `cb` and `candidate`
  as inputs, measured via `--measure-cb`.
* `#pragma inline_depth` default byte is taken as 8 (documented VC6
  default; a05 proves the per-site mechanism, no experiment pinned the
  default since nothing in the corpus reaches depth 8).
* The `hidden-args` term of the arg-count check (`0x18d54` jump table) is
  assumed satisfied — the front end emits matching IL for legal calls.

## 6b. The library-accessor DEPTH lever (measured 2026-09-06, polish 29)

The `/Ob2` budget is spent per call site, so the SPELLING of a library
accessor - which is to say how many inline levels stand between the caller's
statement and the leaf the budget runs out on - is a source lever with no
pragma involved. `std::bitset<N>` is the cleanest instance in this tree
because Dinkumware layers it exactly:

    operator[](size_t) const  ->  test(size_t)  ->  _Xran()  ->
        out_of_range(const string&)  ->  basic_string(const char*, alloc)

so writing `b[i]` instead of `b.test(i)` costs the leaf one level of budget
and pushes whatever was marginal back OUT of line, which is where retail
frequently has it. Swept over every `.test(` / `.set(` site whose owning row
sits below 100 at its banked MAX (36 rows):

| row | before -> after |
| --- | --- |
| `town::initialize_spells` | 97.7386 -> **100.0000** |
| `NewfullMap::GenerateHeightMap` | 96.7484 -> **100.0000** |
| `TSingleSelectionWindow::SetNewPlayerSlot` | 63.0729 -> 68.8219 |
| `TCampaignBrief::ScenarioStruct::GiveCrossoverArtifacts` | 72.5726 -> 73.0000 |
| `mark_spells` (`.set(i,v)` -> `[i] = v`) | 93.9578 -> 94.5148 |
| `TSingleSelectionWindow::MakeHeroFilter` | 87.3429 -> 87.5476 |

It is NOT a general improvement, and the losers are as informative as the
winners: `armyGroup::get_morale_description` 93.06 -> 89.04,
`NewSMapHeader::Save` 87.00 -> 80.36, `AI_attempt_puzzle_guess` 97.16 ->
95.60, `town::GiveSpells` 99.92 -> 99.70, `hero::HeroFn_004DC100`
87.27 -> 79.24 on the `.set` form, and eleven rows byte-flat. Read it as a
per-site fact about which level retail's budget ran out on, and MEASURE both
spellings; the flat rows are the ones where the leaf was never marginal.

The same ladder runs through the sequence containers and `basic_string`, and
two more rows moved on it:

| row | change | before -> after |
| --- | --- | --- |
| `InitializeSeerHutText` | `push_back(x)` -> `insert(end(), x)` | 79.8841 -> **100.0000** |
| `exchange_spells` | `s += x` -> `s.append(x)` (13 sites) | 88.6905 -> 92.1640 |

And the widest one, `basic_string::operator=` -> `assign`, swept over all 37
sub-100 rows that assign to a `std::string` local:

| row | change | before -> after |
| --- | --- | --- |
| `TViewArmyWindow::WindowHandler` | `text = X` -> `text.assign(X)` (7 sites) | 92.5744 -> 99.1520 |

One winner out of 37, three losers (`QuickInfo` 94.87 -> 94.55,
`CreatureBankEvent` 91.59 -> 91.41, `TSpellbookWindow::WindowHandler`
99.90 -> 98.81), three non-compiling and thirty byte-flat. The hit rate is
low; the payoff when it lands is 6.6 points on a row 97 of whose 98 blocks
were already exact, so sweep it, do not reason about it.

`clear()` is the fourth mass-carrying forwarder (`clear()` is literally
`erase(begin(), end())`, and the erase is the mass). Swept over 29 sub-100
rows: `TCampaignStartHeroOption::Read` 88.9802 -> 92.6089 and
`NewSMapHeader::Load` 92.5118 -> 92.6763; two byte-flat, one non-compiling,
and TWENTY-FOUR losers, several catastrophic - `army::HeroFn_00445490`
92.52 -> 14.41, `readMapObjects` 92.20 -> 27.51, `readBlackBox` 93.01 -> 66.11,
`TTextScroller::SetText` 99.44 -> 73.40. This is the lowest hit rate of the
four and the most dangerous; it is worth sweeping only because the sweep is
mechanical and each row is measured on its own.

**THE INTERMEDIATE LEVEL MUST CARRY MASS.** This is the bound, and it is what
separates the levers above from the ones that do nothing. `bitset::test` holds
a range check, `push_back` holds an `insert` call, `operator+=` holds an
`append` call - each is a real basic block the budget can run out on. A
one-line forwarder that only renames its argument is FREE, and adding or
removing it is byte-flat at every site measured:

* `.length()` -> `.size()` (`length()` is literally `return size();`) - twelve
  rows swept, **all twelve byte-flat to the digit**.
* `.resize(n)` -> `.resize(n, T())` (`resize(n)` is literally
  `resize(n, T())`) - four rows swept, **all four byte-flat**.

So do not sweep a forwarder; sweep an accessor that does work. And measure -
the sign is per-site, never per-lever (`push_back` -> `insert` LOSES on five
of the eleven rows it was tried on, up to -9.7).

**AND THE LADDER RE-OPENS CLOSED ROWS.** Twenty rows whose residual notes had
been closed against every lever that existed before this one were re-measured
with it, one measurement each. Three moved, two materially:
`game::LoadMap` **70.6990 -> 75.4768** on the six `clear()` calls in its pool
reset, and `TCampaignBrief::TCampaignBrief` **85.7661 -> 86.6820** on five
`push_back`s (`TCampaignBrief::CompleteCurrentMap` gained 0.16 and was left
alone as noise). Neither row's standing note was wrong - both predate the
lever. This is the "a local-maximum verdict expires when a new lever lands"
rule paying out, and it is cheap: the sweep is mechanical.

Two riders:

* `TSingleSelectionWindow::OnBeginGame` shows the ladder has a floor. It is
  already spelled `[...]` through a `const bitset<4>&` and retail is STILL one
  level less inlined - it CALLS `bitset<4>::_Xran()` - and there is no deeper
  legal spelling, so that one needs caller mass, not a respelling.
* The lever can RETIRE a pin. `mark_spells` carried a statement
  `#pragma inline_depth(0)` around one `.set`; with the subscript form the pin
  is worth -0.19 (94.32 pinned against 94.51 unpinned), so it came out and the
  tree's pin count fell 354 -> 353.

### The checked bitset accessor can recover another boundary

`GiveCrossoverArtifacts` (`0x487900`, Complete-only) reaches 99.41% from
94.50% by replacing the non-const subscript with `bitset::at`. In the pinned
Dinkumware header, `at` checks the index before constructing the reference
proxy; converting that proxy to bool checks it again through `test`. VC6
merges the range tests into retail's single branch while retaining the
`basic_string(const char*, allocator&)` constructor in the throw path.
The subscript control expands that constructor to `_Tidy` plus `assign`.
The checked spelling also restores EDI as the shared zero across the
artifact and recipient loops. Neither spelling changes the library's
out-of-range exception semantics.

Thus the accessor ladder also includes `at`; the earlier observation that
subscript leaves no deeper spelling should not be generalized to other
callers. Measure the actual overload and call site. This function still
has separate artifact stack homes and the wrong first vector insertion
boundary, so the remaining 0.59 points are not an established allocator wall.

### Constructed return values and local return objects differ after inlining

`SelectTerrainTransition` (retail `0x005b3e80`, 1,887 bytes) reaches 100%
with an ordinary static helper returning `TRmgTerrainFlip(x, y)`. Returning
a named flip local after assigning its two fields instead moves the caller's
two-byte temporary from `ebp-2` to `ebp-8` and its saved output pointer from
`ebp-8` to `ebp-4`. The total frame size remains eight bytes, so frame size
alone misses the difference. Both helpers expand under `/Ob2`.

Replacing the helper calls with direct construction is another negative
control: it keeps the temporary layout but changes the fourth reflection
loop's register allocation, scoring 99.7991%. Adding an explicit empty
destructor prevents the value-return helper from auto-inlining; the retail
array's registered empty cleanup therefore does not itself prove a
user-declared destructor. No inline pragmas or artificial caller operations
are needed. These are retail-supported source hypotheses; this Complete-only
function has no Dreamcast counterpart.

### Source arm order controls nested inlining before cold-code placement

`ReadRmgTemplateZones` (`0x00538480`, 1,671 bytes) demonstrates that retail's
physical arm order does not fix C1's source statement order. Its invalid-player
cleanup lies after the large parsing/insertion arm in x86. Writing the accepted
arm first, followed by `else { delete slot; }`, lets VC6 expand the connection
vector destructor; retail retains that call at `0x00538ab0`.

Writing the same filter as `if (invalid) { delete slot; } else { ... }`
restores the retained vector destructor naturally. With the diagnostic
three-argument `vector::insert` spelling, this moves 96.94682% to 98.08062%.
Restoring ordinary `push_back(slot)` then reaches 100%: all 1,671 raw bytes
match after resolving 33 relocations, including the embedded switch tables.
The extra STL boundaries also recover the retail growth temporaries and
registers; selecting the count-insert overload had only compensated for the
incorrect arm order. No new inline pragma or release-elided assertion remains.

A cleanup-only pin is an insufficient diagnosis: temporarily pinning the
implicit member teardown retains the vector destructor but leaves the growth
mismatch and introduces an unwanted spreadsheet subscript call. Inspect both
the ordered named call stream and the insertion expansion after restoring
source control flow. This function has no Dreamcast counterpart; the source
order is established by the VC6/retail controls, not a recovered line table.

## A missing helper can alter an earlier expansion

`game::InitNewGame` (`0x513320`) reached 100% by restoring the ordinary
`pick_alignment` call named at Dreamcast `newgame.cpp:284` and correcting
`TPlayerSlotAttributes::legalAlignments` to an unsigned word, as retail's
zero extension requires.

The helper call alone raises the current 18.7059% (prior MAX 26.5235%) to
95.0853% and removes all 39 surplus CFG blocks from the preceding
`NewSMapHeader` copy assignment. Its members now reach retail's nested
inline boundaries without a pragma. C1's caller-size estimate is made
before expansion: hand-flattening a later helper can change the initial
budget for earlier calls. Inspect positive source helper evidence throughout
the function, including after the first divergent statement.

The signed mask is an independent negative control: with the real helper
restored, it still chooses the wrong extension, registers, and temporary
homes. The unsigned declaration closes all 70 blocks. Both corrections
preserve one canonical helper and the normal compiler profile.

`TCampaignBrief::ScenarioStruct::Read` (`0x487e40`) gives a second case.
Retail's legacy-artifact loop compares both the source bitset pointer and its
position before dereferencing proxies: the existing `bitset_iterator` range
and `std::copy` restore this shape, raising 39.42% to 55.04%. The three packed
planes then share a value-returning reader, suggested by their repeated
temporary-to-member copies. Recovering that helper removes 17 surplus blocks
and reaches 70.55%; its name remains provisional because this Complete code
has no Dreamcast counterpart.

The remaining scalar evidence matters independently. Retail puts the absent
text-record cases first and retains each allocated record across the virtual
file reads. Those corrections reach 73.36% and 80.95%. Default zero construction
inside the packed-bit helper reaches 83.29%, while the unsigned-long `(0)`
constructor is the 80.95% control. The reader remains unfinished; these gains
do not establish that every remaining call boundary is correct. In particular,
its prerequisite append still expands the single-element insert that retail
retains. The explicit `VECTOR_INSERT_SINGLE` claim now leaves `0x48bf00`
unpaired until that body emits, preserving its historical MAX without borrowing
the count overload's identity.

### Canonical video cleanup helpers (2026-09-06)

`VideoClose` (`0x5975f0`) reaches 100% from 95.9231 when its copied resume and
Smacker teardown are replaced by ordinary calls to `VideoResume` and
`SmackManager::CloseSmacker`. `VideoResume` in turn calls `VideoSoundOnOff`.
The retained retail helpers and `ShowVideo`'s different expansions expose the
chain; Dreamcast's four-byte video stubs prove declarations and order only.

The earlier loop-spelling and cached-count probes could not recover the missing
top test in the flattened body. Restoring the helper chain does, with no inline
keyword or pragma. Its callers remain a separate checkpoint: `ShowVideo` now
auto-inlines the ordinary close at all three sites, but still expands nested
helpers that retail calls. Restoring the other teardown callers is byte-flat;
call-site count alone does not explain that remaining decision.

### Campaign hero lookup and the packed-point accessor (2026-09-06)

`PlaceCrossoverHeroes` (`0x487290`) improves from 67.9283% to 98.0287% by
recovering its reverse-loop tests and two helper boundaries. Retail uses each
reverse loop's pre-decrement count as the condition, matching `for (i = size(); i--;)`.
That change alone reaches 74.5681%.

The all-pool search snapshots its hero-ID argument after the preceding erase,
keeps the current pool across the search, and retains both vector `size()`
calls. An ordinary `SCampaign::FindCrossoverHero(int)` reproduces that pattern
and reaches 87.2742%; the name remains provisional because this Complete-only
caller has no Dreamcast counterpart. Flattening the same loops with a cached
ID and pool reference reaches 75.0108% and expands both nested size calls.

Using the existing `cell(type_point)` overload instead of exposing its three
fields then reaches 98.0287%, restoring the retained `cell(int,int,int)` call
and the final vector-destructor calls. All 71 block flow shapes, 40 branches,
and three returns agree. This is further evidence that a small wrapper can
change later cleanup decisions as well as its own nested call. The remaining
stack/register differences are recorded beside the source function.

### Shared town-selection helpers in a new caller (2026-09-06)

`SetNewPlayerSlot` (`0x58e700`) had copied the bodies of `GetDisplayTown` and
`UpdateTown`. Restoring only the first call raises 68.8219% to 71.87% but
leaves `HasMultipleTowns` called and `CheckFaces` expanded. Restoring both
calls reaches 98.8866%, with retail's repeated `HasRandomAlignment` test,
expanded `HasMultipleTowns`, and retained `CheckFaces` call. The only remaining
instruction-sequence difference is a two-instruction loop-tail detour.

Dreamcast retains both helpers and their nested calls even though the older
`SetNewPlayerSlot` has a different ABI and much smaller body. The Complete
caller's own bytes establish where those shared helpers belong. No new
helper, pragma, or artificial branch is needed. NB11 enum `TTownType` also
supplies `eTownNeutral = -1`, which lets the reset use the existing enum ABI.

### Aggregate membership changes earlier STL inlining (2026-09-06)

`playerData::operator=` (`0x58f750`) reaches 100% from 70.2441% after
restoring its `AI ai` member. Dreamcast NB11 type `0x3591` supplies the
six-member, 120-byte record; Complete copies 30 dwords from `+0xf0` and
skips the preceding four-byte alignment pad. `playerData::Init` independently
clears that same record. The old flattened members generated separate copy
loops and copied the alignment pad as data.

This also restores both retained `_Construct<type_point>` calls inside the
earlier `shipyards` assignment, without changing that vector or its source
operations. All 51 blocks and 25 branches agree. Retail folds the nine-byte
construction helper onto `_Construct<widget*>`. Before diagnosing an STL
inline boundary inside a compiler-generated special member, recover its
complete member structure, including nested POD records and implicit padding.

### Time-helper argument evaluation and exit placement (2026-09-06)

`NormalDialogHandler` (`0x4f08d0`) reaches 99.58% from 71.3624% by
restoring both `15000 - GameTime::ElapsedSince(giNormalDialogStart)` calls.
Dreamcast kb.cpp:2377/2403 reuse the preceding ElapsedSince target register;
Complete snapshots the argument before `GameTime::Get`. Flattening this to
`giNormalDialogStart - GameTime::Get() + 15000` moves that load after Get.
The helper restoration also moves both ExitNormalDialog expansions and
restores the three return sites. Earlier early-return rewrites targeted a
symptom of the lost source call and could not recover that layout.

Restoring the helper's zero-first random-choice arm, visible in Dreamcast
kb.cpp:2345/2346 and retail, closes the remaining branch to 100%. All 31
blocks, 17 branches and 15 calls agree. No inline pragma or extra return
statement is required.

### Folded failure guard and implicit message cleanup (2026-09-06)

`TSingleSelectionWindow::BeginNewGame` (`0x58c570`) reaches 100% from
74.9022% by restoring two source facts. Dreamcast singleselectionwindow.cpp:
7871/7872 records `if (!SendPlayerPositions(0)) return false;`, even though
that helper returns true at line 6986. VC6 removes the runtime test, but
retaining the source guard and its cleanup path restores the earlier header
construction: `NewSMapHeader` stays a call and its assignment expands through
base assignment, two string assigns, and the bitset copy. The guard alone
reaches 94.0226%, with the original 17 blocks and nine branches unchanged.

The remaining difference comes from a written empty message destructor.
Dreamcast attributes `~CNewMapHeaderInfoMsg` to line 7886, the caller's closing
line; retail's standalone body and caller cleanups lack a derived-vptr store.
Removing the explicit declaration/definition and retaining the existing claim
as `IMPLICIT_DTOR` closes both the caller and the destructor (`0x58a300`,
97.0588% to 100%). Cleanup now retains the map destructor and expands only the
normal-exit vector teardown, exactly as retail does.

Negative controls are the original omitted guard and written destructor:
74.9022% together, 94.0226% with only the guard restored. Restoring the named
`CLaunchingGameMsg` and `iReturn` header-send local alone is byte-flat. There
is no inline pragma or invented assertion. A failure branch proven in the
older source must survive reconstruction even when constant-return inlining
makes it disappear from retail instructions.

### A fully expanded helper still determines register lifetimes (2026-09-06)

`combatManager::CalculateGainedExperience` (`0x46a350`) reaches 100% from
75.0110% by restoring the ordinary cpp helper `ExperienceValueOfStack(1-side)`.
Dreamcast cmbtmgr.cpp:4949 records that call; the helper at lines 2738..2752
owns the casualty loop, both `army::Is` calls and the defeated-hero bonus.
The caller adjusts retreat/surrender and town experience, applies Learning,
and assigns the result after the conditional at line 4963.

The flattened version had the same retail call multiset and branch flow,
but spilled the accumulated experience instead of `this` and the loop
counter. Type/order sweeps had left it at 75.0110% and misclassified the
residual as an allocator tie-break. Restoring the helper and field-accessor
boundaries reproduces all 17 blocks without an inline keyword or pragma.
A matching call multiset does not prove the source helper structure is
complete: a helper expanded on both sides can still delimit register lifetimes.

## A callee defined LATER in the TU still inlines

Measured 2026-09-06 (polish lane 44), pinned SP3 CL under Wine, on the real
tree. A pasted helper body is **never forced by definition order**: `/Ob2`
expands a callee whose *definition* stands below the call site, as long as a
declaration precedes it. C1 hands C2 the whole TU's IL before C2 chooses, so
the "define it above the caller" folklore does not apply to this back end.

The clean control is `binkmanager.cpp`. `NextBinkFrame` (`0x44daa0`,
`binkmanager.cpp:214`) carried the thirteen statements of `CloseBinkVideo`
(`0x44dcc0`, defined at `binkmanager.cpp:285`, seventy lines BELOW it)
written out longhand. Replacing them with the call is byte-flat -- 92.9245
before and after -- and `sema diff --calls` still shows the two
`_BinkPause`/`_BinkClose` pairs standing inline at `+0x173..+0x192`, i.e.
VC6 reached down the file, took the body, and emitted retail's expansion.
The only prerequisite was the declaration already in `binkmanager.h:124`.

So the helper-boundary rule in CLAUDE.md is enforceable everywhere, and
"the definition comes later" is not a reason to keep a longhand copy.

### The census, and where the caller_cb lever actually bites

The tree-wide sweep for claimed helper bodies copied into callers (short
claimed bodies, normalised modulo identifier renames, matched against every
window of every other body) found six live sites. Every one is byte-flat
once restored:

| caller | helper | score, before = after |
|---|---|---|
| `advManager::Open` `0x406fd0` | `ForceNewHover` | 97.9312 |
| `advManager::DoAdvCommand` `0x407b80` | `ForceNewHover` | 94.3953 |
| `advManager::ProcessKeyPress` `0x408c40` | `ForceNewHover` | 97.6091 |
| `advManager::SetHeroContext` `0x417b20` | `DeactivateCurrHero` | 99.2746 |
| `NextBinkFrame` `0x44daa0` | `CloseBinkVideo` | 92.9245 |
| `VideoClose` `0x5975f0` | `CloseSmacker` | 95.9231 |

Retail expands the helper at all six; the call is the source fact and the
bytes do not care. The sixth row carries the census's only positive retail
proof, and it is worth the pattern: `ShowVideo` (`0x598af0`) expands
`VideoClose` three times, and its THIRD expansion at `+0x284` runs
`VideoSoundOnOff / service_sounds / CALL CloseSmacker / CALL CloseBinkVideo`.
A call to `CloseSmacker` standing *inside* an expansion of `VideoClose` can
only come from a `CloseSmacker()` call in `VideoClose`'s own source -- a
longhand copy there would have been expanded with everything else. **When a
suspected paste has a caller that retail expands, read that caller's call
stream: a helper call surviving inside the expansion proves the boundary.**
Its cost is `ShowVideo` 48.6988 -> 41.0154, TU collateral kept under the
"preserve proven helpers through score dips" rule; `VideoClose` itself is
flat and MAX is unmoved. That flatness is itself the model's prediction and
sharpens the polish-42 result (`CampaignHeaderStruct::Load`, +4.44 for the
same edit). The budget is `clamp(2 x caller_cb, 1000, 35000)`, so moving
mass out of `caller_cb` can only change an expansion decision while
`caller_cb` sits inside `[500, 17500]`. Below it the 1000 floor absorbs the
change; above it the 35000 ceiling does. `game::NextPlayer` (`0x4c6fe0`, 142
statements) is the ceiling control: restoring its pasted
`game::CancelComputerScreen` body is byte-flat at 81.3343, because that
caller is saturated. Do not expect a pasted-helper restoration to pay on a
very large or a very small caller -- take it for the source fact, and look
for the mid-band callers when hunting score.

## Ordinary definitions later in the same TU can inline

Complete retains the 91-byte fastcall guard-value helper at `0x545e00` and
expands its four threshold/scale table accesses inside several RMG connections.
The recovered `GetRmgGuardValue(int value, int strength)` definition follows
`CreateGroundConnection` and `CreateSubterraneanGate` in `rmg.cpp`, in retail
address order. Both earlier callers inline it under the normal RMG profile,
without an `inline` keyword or a pragma; its standalone body matches all 91
raw bytes after resolving four data references.

A declaration followed by a later definition in the same TU therefore does
not establish an out-of-line boundary. Verify the actual caller expansion
before moving a body or changing a declaration. Restoring this shared helper
alone does not settle the callers' remaining STL and map-accessor decisions.

### An exact accessor can hide a different nested call

The by-value `type_random_map::GetMapItem` at 0x5378e0 retains the same
39 raw bytes whether it computes the index directly or delegates to the
three-scalar overload. The delegation leaves `RepairWaterZoneBorders`
unchanged, but changes `CreateGroundConnection`'s first clear from expanded
`copy`/`_Destroy` calls to a retained range erase, as in retail
(76.75134% to 77.31306%).

The same change moves `CreateRiver` from 39.066925% to 33.67439%: final
map cleanup calls the vector deleting destructor where retail directly
invokes the array iterator, and a trailing vector `_Destroy` is retained.
These named sites show why an exact standalone body does not settle the
source call boundary. The candidate keeps one indexing formula through
delegation; its original source spelling remains provisional because the
DC corpus has no RMG compiland. Caller-specific residuals and prior peaks
remain recorded rather than being hidden by an inline directive.

### Repeated tile operations expose shared helper boundaries

`PaintPoint` (0x5b4b20) and both update paths in `PaintTransitions` (0x5b5a70)
write the adapter, then refresh the packed cache with validity first and four
field setters. An ordinary shared `SetTile(point, tile)` preserves that
operation and raises `PaintPoint` from 78.4213% to 92.2405%. Flattening the
body into its callers changes later set and gap-predicate expansions.

The same callers compute transition strength before loading the base-frame
rule's virtual receiver. A shared `SelectBaseFrame(point, terrain, oldFrame)`
captures the terrain index across that call and preserves this evaluation
order. With a named frame result and scoped neighbour points, `PaintPoint`
reaches 95.3146%. `PaintTransitions`, which initially fell to 38.6271% when
the tile writer was recovered, returns to 74.7320% with its proven unsigned
grid interface intact; its 74.7623% historical peak remains banked.

These are retail-derived boundaries with provisional names and no Dreamcast
counterpart. The remaining `PaintPoint` expansions are documented beside the
function. No inline keyword, pragma, or unused operation is added. Recovering
one common operation can expose another missing boundary in a different
caller; preserve the stronger interface while checking that collateral.

### Coordinate construction affects later nested calls

The grid translation used by `PaintPoint` can initialize its working value
with the retained two-reference coordinate constructor before applying the
offset. This leaves the arithmetic expansion unchanged but restores the final
`GetPackedCell` call at retail 0x5b509e, raising the caller from 95.3146% to
97.0506%. The first neighbour read still expands `GetPackedCell`, and the
inner set erase still expands the three-argument distance wrapper. Its frame
and original-x temporary remain different. This supports the constructor
boundary, without establishing original local names or a free/member addition
interface: a free addition taking both operands by reference is byte-neutral.

Named coordinate values, named cache indices, tighter tile scopes, separate
nearby assignment, and early-continue loop guards are also byte-neutral. An
explicit grid copy constructor instead introduces a retained call absent from
retail. Giving the shared tile writer a value argument adds an entry copy;
adding an aggregate packed-cell writer retains that method where retail has
field stores. Neither is evidence for replacing the existing writer interface.
Recovering the retained neighbour-queue body is neutral for `PaintPoint`, so
its former declaration-only state does not explain these remaining decisions.

### Measured budget comparisons in the terrain painter

The gated [C2 shim trace](shim.md#4-gated-inline-budget-observations) reads
actual candidate costs and budgets from the configured terrain compile.
For `rmgTerrainPainter::paintPoint` (prior role `TRmgTerrainPainter::PaintPoint`,
retail 0x5b4b20) at the earlier 97.0506% checkpoint, the front-end caller
estimate is 933 and the initial budget
is 1,866. At the first eight-neighbour terrain comparison, `getPackedCell`
has cost 90 and budget 106 at depth 2. At the inner set erase, the three-argument
`_Distance` has cost 41 and budget 45 at depth 3; its four-argument child
has cost 45 but only 4 budget units. Those readings explain the two observed
unwanted expansions. The final rule read gives `getPackedCell` only 73 units
and correctly retains the call.

Both compiled objects agree outside the COFF timestamp. The painter's lowerCamelCase
method/type names and `m_` field prefixes leave all 70 emitted code sections
unchanged. Source-owned comments preserve the earlier provisional role names;
retail labels and checkpoint rows are regenerated from the new declarations.
The trace measures candidate compiler state, not missing retail source tokens.

A scratch counterfactual separates the two unwanted inline copies from the
storage residual. At the existing `0x19f8c` hook, reject the first depth-two
`getPackedCell` and the following three-argument `_Distance` by returning to
the compiler's rejection path at `0x19a94`. Charge their original costs (90
and 41) to the current budget before rejecting: their baseline child
expansions are all free, so this preserves the later budget decisions.
This is deliberately a modified-compiler experiment, outside the passive
trace command and the matching build. Its normal-shim restoration is mandatory.

The diagnostic reproduces all 56 named/virtual retail call sites in order,
including the correct distance overload, yet still has a 0x54-byte frame
(retail 0x50) and omits the original-x store at 0x5b4e3f. Thus neither storage
delta can be attributed solely to those inline copies. The ordinary byte-checked
`/Z7` object records the tile at EBP-0x54, the neighbour mask at EBP-0x38,
and all five scoped nearby points at EBP-0x28; it omits the optimizer's
unnamed temporaries, so those records do not identify the extra allocation.
A lexical `inline_depth(1)` at the outer terrain read is byte-neutral because
the nested call retains its own lexical allowance. Flattening just this read
and pinning its cache call changes the caller's budget and later calls, so it
is not an equivalent control. Both source pragma probes were removed.

### Recover the tile constructor and terrain predicate together

`paintPoint` now constructs a base tile from terrain/frame and tests matching
terrain through an ordinary `isPaintTerrain(point)` helper. That helper calls
both `getTerrain(point)` and `getPaintTerrain()`. Each operation has a meaningful
value; no dummy call, assertion, or inline control is present. Names and the
interface remain retail-derived hypotheses, since this TU has no DC counterpart.

The individual controls explain why a lower intermediate score did not reject
these boundaries. Before the point-copy recovery, the two-accessor predicate
alone restored the first cache call but freed enough budget to expand the final
one (96.6293%). The tile constructor alone prevented the inner tree find from
expanding (85.7667%). Together they retain both desired cache calls and expand
the tree find (98.2893%). The constructor's two-argument zero-flip form, explicit
default flip arguments, and the existing flip-value factory leave the score unchanged.

After recovering point-copy initialization and the base tile's lifetime, the
full checkpoint is 99.5570%. The gated trace now reads caller cost 920, initial
budget 1,840, and base-tile constructor cost 52. The first cache read gets 48
units at depth 3, so the cost-90 `getPackedCell` stays out of line. The two
interior rule reads get 122 and 109 and expand it; the final rule read gets 77
and retains it. The inner distance wrapper still receives 47 for cost 41 and
expands, leaving only 6 for its cost-45 fourth-argument child. Its unwanted
expansion is still a real residual, despite report-level relocation agreement.

The trace object matches all 56,600 reference bytes outside the COFF timestamp.
Renaming the recovered terrain-tile type and its fields leaves all 74 named
function sections byte-identical. Full build and raw checks preserve the cache
reader/initializer, gap predicates, coordinate constructor/comparator, both
worklist destructors, and the exact 1,516-byte water-border repair.

## 7. Using it

```sh
# the validated rule, replayed:
python3 -m homm3.vc6.inline_model --predict --selftest

# what does the model say for a caller? (cb values from --measure-cb)
python3 -m homm3.vc6.inline_model --predict --spec sites.json
#   {"caller_cb": 235, "sites": [{"name": "kill", "cb": 80,
#      "sites": [{"name": "get_total", "cb": 46}]}, ...]}

# bracket a callee's front-end size estimate with the real compiler:
python3 -m homm3.vc6.inline_model --measure-cb harness.cpp \
    --fn callee --caller caller25 --sites 25

# the diagnoser (v1) is unchanged:
homm3 vc6 predict-inline src.cpp --fn F --against UNIT:FN
```

The Ghidra evidence regenerates with
`python3 scripts/homm3/vc6/ghidra_scripts/inline_probe.py dump|refs|callers`
against the persisted atlas project (never re-analyze; `atlas --regen
--reimport` owns that).
