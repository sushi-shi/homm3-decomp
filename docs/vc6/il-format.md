# The C1XX->C2 intermediate language - tap, framing, and the C1 killer result

Phase 1 of the vc6 area: make the front-end/back-end handoff visible and use
it to settle the include-set-sensitivity question (behavior catalog C1).
Subjects: the pinned CL.EXE 12.00.8168 / C1XX.DLL 12.00.8472 / C2.DLL
12.00.8447 (`_toolchain.PINNED` hash-gates all three before any run).
Every byte cited below was measured 2026-08-09/10 on those exact binaries
under wine; commands are reproducible via the tools at the end.

## TL;DR

* **Tap**: `/d1il<prefix>` redirects the front end's IL to `<prefix>{in,gl,
  sy,ex}` and the files SURVIVE (C2 dies on the untouched seed tmp path -
  a capture run produces no obj by design). `/d2il<prefix>` feeds a captured
  set back into C2.
* **Round-trip oracle**: obj(C2 over a captured IL) equals a plain compile
  byte-for-byte outside the COFF TimeDateStamp - 418/418 B (trivial TU),
  24,547/24,547 B (initialize.cpp, game profile). The capture is exactly and
  completely what C2 consumes.
* **Framing**: `in` = 3-byte function records `09 <u16 handle>` (exact,
  round-trip-bounded, simple TUs); `gl`/`sy` = named symbol records around
  one global u16 handle counter (heuristic scan, three record forms); `ex` =
  opaque tuple stream segmented per function by offsets recovered from `gl`.
* **KILLER VERDICT: FRONT-END.** One unused `struct probe0_t { int a; };`
  appended to town.h (the byte-proven 100.0 -> 96.09 perturbation) changes
  all four IL streams: the front end's symbol-handle counter advances by
  **+9** and every later-created symbol's numeric handle shifts by +9 while
  the symbol NAMES and record structure stay identical. Feeding the two ILs
  through the SAME C2 reproduces an 11,900-byte obj delta from the IL alone.
  The include-set sensitivity is upstream of C2; the "C2-internal optimizer
  state" hypothesis class is retired.

## 1. The tap - how capture works

The driver seeds `-il <tmp>` to every pass (spec record 0x7eec `^il!>+` ->
`1P2=*:*`, docs/vc6/driver-passes.md section 6) and appends the suffixes of
its temp table (file 0x7510: `ex sy gl in st db`) to the prefix. The `^`
makes `/il` non-user-typable (D4002), but `/d1<text>` reaches both front
ends verbatim (record 0x7c30 `d1!+` -> `1PM=!*`) and `/d2<text>` reaches C2
(record 0x7c44 `d2!+` -> `2M=!*`) - so the -il VALUE is injectable per pass:

| run | observed (probe.cpp, 2026-08-09) |
|---|---|
| `/d1il<P>` alone | C1XX honours the LAST `-il`: writes `<P>{ex,gl,in,sy}` (1093/72/3/43 B); C2 still reads the seed tmp, dies `C1083 ...<tmp>in`; the four files SURVIVE (the driver only deletes its own temp names). **This is the capture route.** |
| `/d2il<P>` alone | C2 honours the LAST `-il` too: dies `C1083 ...<P>in` (nothing was written there). With `<P>` holding a captured COPY, C2 compiles from it - **the feed route** - and leaves the files intact on success. |
| `/d1il<P> /d2il<P>` in ONE invocation | self-destructs: C1XX writes `<P>*`, then C2's in-process handling deletes them and re-opens `<P>in` -> C1083, empty dir. Mechanism unmodelled (OPEN); capture and feed therefore stay separate invocations. |

Capture properties, measured:

* **Stable**: two captures of the same TU are byte-identical in all four
  streams (unlike the obj, which carries a timestamp).
* **Path-independent**: captures under two different prefix paths are
  byte-identical - the IL does not embed its own -il path.
* **Complete**: the round-trip oracle (section 3).
* What DOES embed: the `-f` source spelling and every include path **as
  resolved** (absolute `Z:\...\include\terrain.h` for INCLUDE-env hits,
  relative `town.h` for includer-dir hits). `homm3.vc6.il.capture` therefore
  copies the TU to a common basename (`tu.cpp`) in equal-length work dirs
  and compiles with cwd there, so neither the -f token nor a resolved path
  can pollute an A/B comparison.
* `st`/`db` streams: reserved in the driver's suffix table, never observed
  written by C1XX (trivial TUs through initialize.cpp, game profile).
* The injected token itself is the one argv difference vs a plain compile;
  its inertness on pass-1 OUTPUT is bounded by path-independence plus the
  round-trip oracle.

## 2. The four streams

For `int add(int a, int b) { return a + b; }` (plain `/O2`, sizes
ex/gl/in/sy = 1093/72/3/43 B); initialize.cpp under the game profile gives
109,524 / 45,224 / 11,097 / 33,929 B.

* **`in`** - the pass-2 function index: which functions C2 compiles, in
  order. Simple TUs are exactly `09 <u16 handle>` per function (probe:
  `09 d7 00`; two-function TU: `09 d7 00 09 dc 00`).
* **`gl`** - the global symbol table: 16-byte header, then name-bearing
  records (source files, functions, data, string literals), then trailer
  `0a 00 00 00`. Function records embed the function's ex-stream offset.
* **`sy`** - per-function local-symbol blocks, in `in`-list order
  (parameters listed in reverse source order).
* **`ex`** - the expression/tuple stream: header `5b 80 f4 03`, a zero hole
  to 0x3f4, then per-function tuple spans (opcodes 0x4f/0x53/0x41/... with
  u16 handle operands).

**Handles.** Every stream refers to symbols by a u16 handle from ONE global
creation counter in the front end. Probe proof (`int add(int a, int b)`):
a=0xd5, b=0xd6, add=0xd7, source file=0xd8, block=0xd9; two-function TU
continues c=0xda, d=0xdb, sub=0xdc, block=0xdd. The gl header carries the
counter's high-water mark. Symbol creation ORDER is therefore directly
visible in the numbering - the leading observable for the B-family
register-tie-break hypothesis.

## 3. The round-trip oracle

```
capture:  cl /c <flags> /d1il<P> tu.cpp          (IL at <P>*, no obj)
feed:     cp <P>* <Q>*; cl /c <flags> /d2il<Q> /Foq.obj tu.cpp
compare:  q.obj vs a plain-compile obj, masking COFF bytes 4..7
```

Measured: trivial TU 418/418 B identical **including** the timestamp (same
second); initialize.cpp (game profile) 24,547/24,547 B identical outside the
mask. This bounds every capture claim: the four files are byte-exactly the
front end's complete output, and C2 is deterministic given them.

## 4. Record framing achieved (and its limits)

Byte evidence from probes p2 (add+sub), p3/p3n (struct/no-struct), p4
(global + static + caller) and initialize.cpp; parser in
`scripts/homm3/vc6/_il.py`, annotations wired into `homm3 vc6 il-diff`.

### 4.1 `in` - exact for the simple form

`09 <u16 fn-handle>` x N; `_il.parse_in`/`serialize_in` round-trip
byte-identically or refuse (the framing bound). Rich TUs interleave OTHER
record types after the leading 09-run - observed in initialize.cpp at
offset 15: `09 b0 04 | 00 c7 04 00 02 cc 04 00 04 01 03 04 ...` - that
grammar is OPEN; the tool falls back to byte-level for such streams.

### 4.2 `gl` - header plus three named-record forms (heuristic overlay)

* Header: constant leader `11 86 03 96 ba 30 01` (the pinned C1XX's stamp;
  all observed C++ captures), then **u32 handle high-water** at offset 7,
  then `01 02 00 00 00 0c 00 80 18`.
* Source file: `21 12 <u16 handle> <path-as-resolved cstr>`.
* Function: `0e <u16 handle> 00 <mangled cstr>` + attribute tail containing
  `... 00 00 80 <u32 ex-offset> ...` (tails observed: `01 05 04 00 00 00 80
  <u32>` plain /O2, `01 05 04 04 00 00 80 <u32>` game profile /Gr; data
  points 0x3f4/0x43d/0x3f4 for add/sub/helper - each matches where that
  function's tuple bytes begin in `ex`). Terminator byte `02`.
* Data symbol: `01 00 <u16 handle> 00 <mangled cstr>` + 8-byte tail.
* Embedded-handle form (wide handles): `<00> <u16 handle> <name>` with NO
  separator - e.g. initialize.cpp `02 00 49 27 24 6b...` = handle 0x2749 +
  `$kTown0Buildings`.
* Trailer: `0a 00 00 00`.

The name SCAN (`_il.scan_names`) walks printable 0-terminated runs preceded
by a 00 byte and tries strongly framed function/data/local records before
embedded-handle and generic symbol/file forms, gated by handle plausibility
(<= high-water) and a name charset. Rich-TU `sy` locals use
`01 <scope> <u16 handle> 02 00 00 <name>`. Strong framing must win: otherwise
an ordinary mangled name beginning `?P` is a plausible embedded handle
`0x503f` and loses its first two characters. It is an OVERLAY: it annotates
the byte diff (names in order, handle-shift summaries, per-function ex spans),
never replaces it.
Known residual noise: junk runs from attribute bytes can scan as records;
both sides of a diff see the same noise, so comparisons stay meaningful.

### 4.3 `sy` - block spans

Per function, in `in`-order: header `03 01 <u16 block-handle> 1f 00 01 01
0d 01`, parameter records `01 01 <u16 handle> 00 <name cstr>` + 8-byte type
tail, terminator `0d 02 06`. The tool splits on the terminator for block
counts; the type-tail semantics are OPEN.

### 4.4 `ex` - opaque, segmented from `gl`

No tuple grammar is claimed. Per-function segmentation uses the ex-offsets
recovered from gl (423 spans for initialize.cpp), which lets the diff name
the first diverging function and count affected functions.

## 5. The killer experiment - catalog C1 settled at the IL boundary

**Question.** initialize_game_data's exactness moves non-monotonically with
the COUNT of user-defined types visible in its TU (0..8 dummy-struct sweep:
100/96/96/26/97/94/100/100/96) with no semantic change. Is the mechanism in
the front end (then the IL must differ) or in C2-internal state (IL
identical)?

**Setup** (`python3 -m homm3.vc6.il killer`): initialize.cpp, game profile
(`/c /nologo /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS`), two captures
in equal-length work dirs `build/vc6/il/killer/{a,b}`:
A = shadow `town.h` == include/town.h verbatim; B = same + one appended
unused `struct probe0_t { int a; };` (the byte-proven 100.0 -> 96.09
perturbation). town.h resolves from the shadow dir in both, so no resolved
path differs.

**Result (2026-08-10)** - all four streams differ, same sizes:

| stream | size | differing bytes | first divergence |
|---|---|---|---|
| in | 11,097 | 60 in 28 clusters | 0xc2a |
| gl | 45,224 | 61 in 60 clusters | 0x7 (the high-water dword) |
| sy | 33,929 | 794 in 533 clusters | 0x4d57 |
| ex | 109,524 | 2,868 in 1,989 clusters | 0xf532 |

The annotated diff pins the mechanism:

* gl high-water 0x2bb8 -> 0x2bc1: the struct consumed **+9 handles**
  (identical to the micro-probe: p3 vs p3n, `struct probe_t { int a; }`
  moved file/function handles d8/d7 -> e1/e0 and high-water da -> e3).
* 657 gl symbol names IDENTICAL in order; 58 named-record handle fields
  shifted, ALL by +9 (e.g. `$kTown1IncludeList` 0x2754 -> 0x275d).
* 481 sy blocks on both sides; ex divergence touches 218 of 423 recovered
  function spans, first `?add_to_included_mask@@YIXPBHPA_J@Z`.

**Causation, not correlation** (same run): obj(C2 over IL A) vs the plain
compile = 0 bytes outside the COFF timestamp (capture faithful); obj(C2
over IL A) vs obj(C2 over IL B) = **11,900 bytes** (24,547 vs 24,659 B) -
the entire include-set codegen delta reproduces from the IL bytes alone,
same C2, same source text, same flags.

**VERDICT: FRONT-END.** The perturbation is upstream of C2: C1XX's
symbol/type-table population determines the numeric handles baked into
every IL stream, and C2's output is a deterministic function of those
bytes. What changes under a dummy struct is (at least) the handle
NUMBERING, names and structure held fixed. Retired: the hypothesis class
"C2-internal optimizer state keyed on something outside the IL" (idb
state, allocation nondeterminism, ...). Not yet proven: WHICH C2-side
structure turns renumbered-but-identically-named symbols into different
codegen - a hash table keyed on handle values (p2symtab.c) would explain
the non-monotonicity (bucket collisions), but that is the next phase's RE
target, not a measured fact.

## 6. Boundary to the deeper reader.c RE

The atlas (`evidence/vc6/c2-tu-map.tsv`) places C2's IL reader: the pure
`reader.c` block is RVA **0x84d9b..0x864ce** (17 functions, 5 ICE-string
anchors: 0x84d9b/0x8502d/0x8563c/0x85bd6/0x864bc), with ambiguous flanks
`p2symtab.c|reader.c` (0x8411f, 0x84258), `reader.c|ehexcept.c`
(0x8808b..0x882fa), and a `getattr.c|reader.c` bracket run near 0x14cb0.
The phase-2+ path: disassemble the reader's record dispatch to replace the
heuristic gl/sy framing with the true grammar, and walk `p2symtab.c`'s
handle->symbol mapping to test the hash-collision explanation of C1's
non-monotonicity. The open items from section 4 (rich-TU `in` records, gl
attribute tails, sy type tails, ex tuple grammar) are exactly what that
dispatch encodes.

## 7. Tools and commands

| command | role |
|---|---|
| `homm3 vc6 il-diff <srcA> <srcB> [--flags "..."] [--fn NAME] [--json]` | capture both TUs, byte-diff per stream with framing annotations; rc 0 identical / 1 differs / 2 error; verdict on stdout line 1 |
| `python3 -m homm3.vc6.il killer [--structs N] [--json]` | the C1 experiment: shadow-town.h A/B capture + diff + C2 feed oracle + verdict |

Controls (measured 2026-08-10): two identical sources ->
`IL IDENTICAL (in gl sy ex: 1361 bytes)`, rc 0; the same pair with one
`+ 1` added to one statement -> `IL DIFFERS: first divergence gl@0x5f(1B);
2/4 streams differ` with the ex divergence attributed to the edited
function's span (`?mul@@YIHHH@Z`, 0x43d..0x48e), rc 1.

| path | role |
|---|---|
| `scripts/homm3/vc6/il.py` | tap (capture/feed) + il-diff + the killer |
| `scripts/homm3/vc6/_il.py` | stream framing: parse/serialize `in`, gl/sy scans |
| `build/vc6/il/` | captures (gitignored scratch): `diff/{a,b}`, `killer/{a,b}`, `feed/` |
