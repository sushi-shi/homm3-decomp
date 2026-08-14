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
