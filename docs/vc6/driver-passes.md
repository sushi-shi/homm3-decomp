# CL driver → per-pass argv — the option-spec table model

How the pinned CL.EXE (driver 12.00.8168, sha256 `d9220fdf…`, see
`_toolchain.PINNED`) turns a user command line into the argv it hands to each
pass — the C front end **C1.DLL**, the C++ front end **C1XX.DLL**, the back end
**C2.DLL**, and **LINK.EXE**. The driver hardcodes almost nothing: it walks a
table of 0x14-byte records in `.rdata` whose strings are a small sigil
language. This document is the decoded model; it is the ground truth for every
later vc6 phase (it is how we know `/Ob2` reaches pass 1 only, and how `/d2`
flags are injected straight into C2).

Tools: **`homm3 vc6 argv`** (`--flags` / `--unit` / `--pass` / `--json` /
`--verify`), reusable `homm3.vc6.argv.decode_table()` / `expand()`, and the
generated table `evidence/vc6/cl-option-spec.tsv`
(regenerate: `python3 -m homm3.vc6.argv`).

Method note (provenance hygiene): everything here comes from two admissible
sources only — (a) the bytes of the pinned CL.EXE, and (b) running that exact
binary under wine and reading what it prints. The decisive instrument is in
the table itself: record 0x7adc `Bd` → `D=bv,1P2=*` puts the driver in verbose
mode, printing every pass's full command line. Every "run:" citation below is
a `wine CL.EXE /Bd …` invocation of the pinned pressing (2026-08-09).

---

## TL;DR

* The option-spec table: **192 records of 0x14 bytes** at `.rdata` file
  0x7550..0x8450 (VA 0x407550, referenced from `.text` 0x4020b1/0x4020c3).
* Record = `pattern, action, incompat, override, slot4` (5 string VAs; slot4
  is a requires-list, a handler function pointer, or 0).
* Pass-selector letters in action strings — all proven by /Bd runs:
  **`1`**=C1.DLL, **`P`**=C1XX.DLL, **`2`**=C2.DLL, **`C`**=LINK, **`D`**=driver
  state. (`S` unproven, `M`/`m` are modifiers.)
* The driver *decomposes* `/O2` into component tokens per pass — C1XX receives
  `-Gf -Og -Oi -Ot -Oy -Ob1`, C2 receives `-Gy`; the literal string `O2` never
  reaches a pass.
* Game profile `/O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS`:
  * **C1XX** ← `-Gf -Og -Oi -Ot -Ob2 -Op -Gr -EHs -EHc -D_CPPUNWIND -D_WINDOWS`
    (plus the seeded preamble, §6); `/Oy-` and `/GR-` emit nothing (they only
    cancel), `/ML` emits nothing to the front end.
  * **C2** ← `-Gy -ML -EHs` (plus seeded preamble). `/Ob2`, `/Op`, `/Gr`,
    `/D_WINDOWS` never reach C2.
* `/d2<text>` → C2 receives `-<text>` verbatim (`/d2Loop3` → `-Loop3`);
  `/d1<text>` → both front ends, verbatim.

---

## 1. The table

### 1.1 Location and record layout

The table was located by shape (scan `.rdata` at 4-byte alignment for the
record shape, group by offset mod 0x14 to reject shifted overlaps, take the
dominant phase's maximal contiguous run) and then anchored: the run base VA
**0x407550** appears as an immediate in `.text` at file 0x20b1 and 0x20c3.
The run is 0x7550..0x843c inclusive — **192 records**, no gaps, no headers.
`decode_table()` in `scripts/homm3/vc6/argv.py` re-derives this every run and
aborts if the anchor fails.

| off | dword | content | diagnostic when violated |
|---|---|---|---|
| +0x00 | pattern | switch-match string (`"Ob2"`, `"d2!+"`, `"Gs:$x[0,10-4294967295,4096]"`) | D4002 unknown option |
| +0x04 | action | per-pass emission string (`"1PM=*"`, `"D=*,…,2M-Gy"`) or 0 | — |
| +0x08 | incompat | mutually-exclusive switches | **error D2016** |
| +0x0c | override | switches this one supersedes/removes | **warning D4025** |
| +0x10 | slot4 | requires-one-of list (VA→`.rdata`), handler fn (VA→`.text`, only `@` patterns), or 0 | **warning D4007** |

Slot proofs (run vs record):

* incompat — record 0x7898 `O2` slot2 `=GZ,=ZI`; run `/ZI /O2` →
  `Command line error D2016 : '/ZI' and '/O2' command-line options are incompatible`.
* override — record 0x75dc `EHa` slot3 `=EHa-,=EHs`; run `/EHs /EHa` →
  `Command line warning D4025 : overriding '/EHs' with '/EHa'`, and the /Bd
  argv shows `-EHs`/its `-D_CPPUNWIND` gone, `-EHa -D_CPPUNWIND` emitted.
* requires — record 0x7e24 `Gm` slot4 `=Zi,=ZI`; run `/Gm` →
  `warning D4007 : '/Gm' requires '/Zi or /ZI'; option ignored`. Likewise
  0x7bcc `C` slot4 `=E,=EP,=P` → `'/C' requires '/E, /EP or /P'`.
* handler — slot4 VAs into `.text` occur exactly on the `@` patterns:
  0x7c1c `D!@`→0x4030ce, 0x8108 `Tc!@`→0x4031e6, 0x811c `Tp!@`→0x403200,
  0x8130 `To!@`→0x4031cc, 0x7f14 `link:@`→0x403057.

### 1.2 The compound-prefix table (file 0x74e8)

Immediately below the spec table sits a `(prefix, grammar)` pair list,
terminated by (0,0), referenced from `.text` 0x2332/0x2339/0x235f. Only four
switch families combine:

| prefix | suboption grammar |
|---|---|
| `EH` | `a-:c-:s-` |
| `G`  | `3:4:5:6:d:e:f:h*:i-:m-:p#:r:s*:t#:x-:y:z:A:B:D:E*:M:R-:X-:Z` |
| `O`  | `1:2:a-:b#:d:g-:i-:p-:s:t:w-:x:y-:V#` |
| `Z`  | `7:a:d:e:g:i:l:m#:n:p#:s:B*:I:M-:X*` |

Grammar items: single letter, `-` = optional trailing `-`, `#` = digits
follow, `*` = rest of token. A switch that matches no record is split here and
each component re-matched. Run proof: `/EHsc` → C1XX `-EHs -D_CPPUNWIND -EHc`
(records 0x7604 + 0x762c fired); `/Ogtb2` → C1XX `-Og -Ot -Ob2` (0x8068,
0x7938, 0x78d4).

### 1.3 Adjacent structure: temp-file suffixes (file 0x7510)

`('ex',1)('sy',1)('gl',1)('in',1)('st',1)('db',1)('lk',0)` — the suffixes the
driver appends to its temp basename (the `-il <tmp>` prefix files and the link
response file). Observed: a link run used response file `…\a01692lk`
(`link.exe -link @…lk`). Not part of the argv model; recorded for the phase-1
IL-persistence work (the `-il` files are the C1XX→C2 handoff).

---

## 2. Pass-selector letters — proof

An action string is `selector-letters op text, …` (letters persist across
commas until replaced, e.g. 0x7c80 `E` → `1P=E,D=bE,=br,=bd:.,…` — the
letterless `=br` continues under `D`).

| letter | pass | proving record | run evidence |
|---|---|---|---|
| `1` | C1.DLL (C front end) | 0x7c44 `d2!+`→`2M=!*` vs 0x7c30 `d1!+`→`1PM=!*` | compiling `probe.c`: driver prints `c1.dll …`; `1`-selected tokens (`-Ze`, `-Gr`, `-Og…`) appear there |
| `P` | C1XX.DLL (C++ front end) | 0x77bc `GX`→`D=*,P2M-EHs,PM-EHc,-D!_CPPUNWIND` | `/GX` on `probe.c`: C1 gets **no** EH token, C2 still gets `-EHs`; on `probe.cpp` C1XX gets `-EHs -EHc -D_CPPUNWIND` |
| `2` | C2.DLL (back end) | same `GX` record; 0x7898 `O2`→`…,2M-Gy` | C2 line shows `-EHs`, `-Gy`; C1XX shows neither `-Gy` |
| `C` | LINK.EXE | 0x785c `o!>+`→`…,C=out\:!(*|%b)<%X`; 0x7ff0 `nologo`→`1P=*,C=nologo` | non-`/c` run prints `link.exe -link @…lk` whose response file holds `/out:pmain.exe` |
| `D` | driver-internal state | 0x7974 `Oy-`→`D=*` | `/Oy-` emits **no** token to any pass; it only cancels (§4 slot3) — observed: `-Oy` absent from C1XX under `/O2 /Oy-` |
| `S` | **unproven** | only 0x7ac8 `^bS!*`→`S=!*` | never observed to fire |

`M` / `m` are modifiers, not passes (they never appear alone): **`m`** =
repeatable/accumulating — 0x7ed8 `I!>+`→`1Pm=*:*`, run `/I foo /I bar` → C1XX
`-I foo -I bar` (both survive). **`M`** — no behavioural difference found;
distribution: `M` sits on compile-mode switches (`O*`, `EH*`, `ML`, `W`…) and
is absent from file/driver switches (`Fo`, `FC`, `nologo`, `Bd`) — meaning
unproven (§8).

Which front end runs is the driver's language decision: source extension, or
`TC`/`TP`/`TO` records (0x8144/0x816c/0x8158, `D=bt:C|P|O`). A C++ TU gets
C1XX and receives the `P`-selected tokens; a C TU gets C1 and the
`1`-selected ones. Every `1`-action in the table is also `P`-selected except
via `d1`, so in practice the front ends differ only where C++-only records
(`EH*`, `GR`, `vm*`, `noBool`, `vd`) are involved.

---

## 3. The sigil grammar — pattern side (proven subset)

| sigil | meaning | proof (record → run) |
|---|---|---|
| bare text | exact match | 0x7898 `O2` matches `/O2` only |
| trailing `-` | the `-`-suffixed switch is its own record | 0x7974 `Oy-` vs 0x7960 `Oy` |
| `name!…` | required attached argument | 0x7c44 `d2!+`: `/d2Loop3` → arg `Loop3` |
| `name:…` | optional argument | 0x7794 `Gs:$x[…]`: `/Gs` and `/Gs4096` both legal (both observed as defaults) |
| `>` | argument may be the next argv token | 0x7ed8 `I!>+`: run `/I foo` → `-I foo` forwarded |
| `[…,last]` | validation spec; **last element = default** | 0x825c `W!*d[0-4,1]` → seeded `-W 1`; 0x8428 `Zp:$d[1,2,4,16,8]` → seeded `-Zp8`; 0x8324 `ZB:$d[32,128,64]` → seeded `-ZB64` |
| `@` | handler function consumes (slot4 = `.text` fn) | 0x7c1c `D!@`, 0x7f14 `link:@` (eats the rest of the command line) |
| `^` prefix | driver-internal switch, not user-typable | 0x75c8 `^dos`: seeded `-dos` appears in every C2 argv, but typing `/dos` → `D4002 : ignoring unknown option` |

## 4. The sigil grammar — expansion side (proven subset)

Action text, after the selectors and the op:

| sigil | meaning | proof |
|---|---|---|
| `=` / `-` (op) | **both emit a token** (`-`-prefixed) to the selected passes | 0x8310 `Ze`→`1PM=*,-D!_MSC_EXTENSIONS` → C1XX `-Ze -D_MSC_EXTENSIONS`; 0x7898 `O2`'s `1PM-Gf,-Og,…` → C1XX `-Gf -Og …` |
| `*` (first, in name position) | the record's switch name | 0x7794 `Gs`→`1P=*` → `-Gs` (arg dropped) |
| `*` (after `!`/`:`/in `(…)`) | the user's argument | 0x81a8 `V!*`→`1PM=*:*`; 0x7eec `^il!>+`→`1P2=*:*` → `-il <path>` |
| leading `!*` | argument alone, name suppressed | 0x7c44 `d2!+`→`2M=!*`: `/d2Loop3` → C2 `-Loop3` (observed, not `-d2Loop3`) |
| `!` | concatenate, no separator | 0x7744 `Ge`→`D=*,2M=Gs!0` → C2 `-Gs0` |
| `:` | argv word break (space) | `W`→`1P2M=*:*` → `-W 1` (two argv words) |
| `(a\|b)` | `a` if non-empty else `b` | 0x7d70 `Fo:>#`→`D1P2=*!(*\|%b)<obj` → `-Foprobe.obj` with no `/Fo` given |
| `%b` | source basename | same `Fo` proof |
| `%f` | source filename as given | 0x7ca8 `^f!>+`→`1P2=f:%f` → `-f probe.cpp` |
| `<ext` | append default extension if none | 0x7d0c `Fd:>#`→`1P=*!(*\|vc60)<pdb` → C1XX `-Fdvc60.pdb` |
| `<<ext` | force-replace extension | same record, `D2=*!(*\|vc60)<<idb` → C2 `-Fdvc60.idb` |
| `\c` | escape (literal `:` etc.) | 0x785c `o!>+`→`C=out\:!(*\|%b)<%X` → link `/out:pmain.exe` |
| pattern default | empty arg falls back to the `[…]` default | `W` seeded argless → `-W 1`; `Gs`'s `2M=*!(*\|4096)` → `-Gs4096` |

Override-list (slot3) entry forms: `=X` / `-X` remove active instances of
switch `X` and any pass token named `X` (warning D4025 when a *user-typed*
switch is removed by `=X`; seeded defaults die silently); `X:?` matches any
argument (0x7744 `Ge` slot3 `=Gs:?` — run `/Ge` removes both the seeded C1XX
`-Gs` and C2 `-Gs4096`); `X!arg` matches that exact argument (0x77d0 `GX-`
slot3 `=GX,=D!_CPPUNWIND,…` removes the define token). Re-firing the *same*
record replaces its earlier instance — user `/ML` moves the seeded `-ML` from
the preamble to the user position (observed in the game-profile run).

---

## 5. What a switch does end-to-end

1. Tokenize; `/link` (0x7f14) consumes the remainder for LINK.
2. Match against patterns (exact literal wins, else longest name); on failure
   try the compound split (§1.2); on failure warn D4002 and ignore.
3. Check slot2 against active switches → error D2016.
4. Apply slot3 removals → warning D4025 for user-typed victims.
5. Apply the action: emit tokens to the selected passes in action order;
   driver (`D`) entries mutate driver state (`b*` variables: `bt` source type,
   `bd` disable-link, `bp`/`bx` pass binary overrides for `/B1 /B2 /Bx`, `bv`
   verbose for `/Bd`, …).
6. At the end, slot4 requires-lists are checked: unmet ⇒ the switch's
   emissions are dropped — warning D4007 if user-typed, silently for seeds
   (the seeded `EHc` vanishes from a plain compile but survives any `/GX`,
   `/EHs`, `/EHa` run; observed both ways).

## 6. The seeded defaults (observed, not table-decoded)

The driver seeds a default switch list before user switches; the seeding
itself lives in driver code, so the model carries it as an **observed** list
(`DEFAULT_SEEDS` in argv.py), reproduced from the /Bd print of a bare
`cl /c probe.cpp`:

```
C1XX: -il <tmp> -f probe.cpp -W 1 -Ze -D_MSC_EXTENSIONS -Zp8 -ZB64
      -D_INTEGRAL_MAX_BITS=64 -D_M_IX86=500 -G5 -Gs -Ot -Ob0 -Foprobe.obj
      -pc \:/ -Fdvc60.pdb -D_MSC_VER=1200 -D_WIN32
C2:   -il <tmp> -f probe.cpp -W 1 -G5 -Gs4096 -dos -Foprobe.obj -ML
      -Fdvc60.idb
```

i.e. seed order `il f W Ze Zp ZB G5 Gs dos Ot Ob0 Fo pc ML Fd EHc
D:_MSC_VER=1200 D:_WIN32`, every entry expanded through the same table
machinery (which is what makes `-Gs` vs `-Gs4096` and `-Fdvc60.pdb` vs
`-Fdvc60.idb` come out right). `-D_MSC_VER`/`-D_WIN32` go through the `/D`
handler path (front ends only — C2 never sees defines).

## 7. Worked example — the game profile

`/O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS` (units.toml
`game_o2_ml_gr_windows`, minus the driver-only `/nologo /c`). Model output =
the /Bd-observed pass lines **token-for-token** (2026-08-09 run; `-Bd` itself
elided):

| user switch | record | C1XX receives | C2 receives |
|---|---|---|---|
| *(seeds)* | §6 | preamble as above, `-Ot -Ob0` cancelled below | preamble; `-ML` re-fired below |
| `/O2` | 0x7898 | `-Gf -Og -Oi -Ot` (its `-Oy`, `-Ob1` die below) | `-Gy` |
| `/Ob2` | 0x78d4 | `-Ob2` (slot3 `=Ob0,=Ob1` kills seed `-Ob0`, O2's `-Ob1`) | — |
| `/Oy-` | 0x7974 | — (slot3 `=Oy` kills O2's `-Oy`) | — |
| `/Op` | 0x78fc | `-Op` | — |
| `/ML` | 0x7f64 | — | `-ML` (moved to user position) |
| `/Gr` | 0x7780 | `-Gr` | — |
| `/GX` | 0x77bc | `-EHs -EHc -D_CPPUNWIND` | `-EHs` |
| `/GR-` | 0x7e60 | — (driver state only; nothing to cancel) | — |
| `/D_WINDOWS` | 0x7c1c | `-D_WINDOWS` | — |

Final (user-switch region): **C1XX** `-Gf -Og -Oi -Ot -Ob2 -Op -Gr -EHs -EHc
-D_CPPUNWIND -D_WINDOWS`; **C2** `-Gy -ML -EHs`.

The `/Ob2` claim that motivated this model: `Ob2` (0x78d4, `1PM=*`) is
front-end-only — the inliner budget knob never reaches C2. Conversely
`/d2<x>` (0x7c44, `2M=!*`) reaches only C2, verbatim: `homm3 vc6 argv --flags
"/d2Loop3"` → C2 `…-Loop3`.

## 8. Unproven / open

* `S` selector (only `^bS!*` 0x7ac8) — target pass unknown.
* `M` modifier — no behavioural difference found; hypothesis: marks
  mode-switches recorded for the .idb build-state, untested.
* `!+` vs `!*` argument forms — both behave as rest-of-token in every observed
  case; the distinction (if any) is unproven. Trailing `x` in `F!*x`,
  `H!*x`, `nl!*x` (hex-allowed numeric?) unproven.
* `:?#` patterns with all-zero expansions (`FP:?#` 0x76a4, `Fs:?#`, `GE:?#`,
  `Gp:?#`, `Gt:?#`, `OV:?#`) — accepted-and-ignored relics; the `?` sigil
  meaning is unproven.
* `$d` vs `$x` validators, and the exact `[…]` range grammar
  (`0,10-4294967295,4096`) beyond "last = default".
* Exact list-mutation semantics of `=` vs `-` inside slot3: both remove; the
  model additionally recurses `-X` entries one level into X's own slot3
  (needed to reproduce the seeded `-Ot` dying under `/O2`, whose slot3 has
  `-Os` and Os's slot3 has `=Ot`) — reproduces every observed run but the
  mechanism is a hypothesis, not disassembly-proven.
* `%X`/`%x`/`%m` escapes (target-extension/-basename/map-basename by context)
  — only exercised in link-side expansions.
* Driver `b*` state variables' full meaning (`ba:1`/`ba:P`, `bd:.` vs `bd:C`,
  `bo:…`); `Bk`'s bare `m=*:*` selector carry-over.
* The seed list §6 is observed, not decoded from driver code; a driver-code
  walk would pin where `-pc \:/`, `-D_MSC_VER=1200` come from.
* `--verify` against the C2-shim argv log is designed but pending the shim
  (`build/vc6/shim/argv.log` does not exist yet); all verification so far is
  via `/Bd`.
