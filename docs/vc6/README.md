# The `vc6` area — a white-box model of the VC6 SP3 compiler

Binary-matching decomp plateaus on two wall classes that no source spelling
reaches: **inliner divergence** (the `/Ob2` budget is positional/sequential;
depth-2 stops go both directions vs our CL) and **register-allocation
tie-breaks** (whole-body ESI/EDI role swaps, memory-homing, spill-to-dead-
parameter-slot, bidirectional constant-CSE). This area stops grinding those by
*modelling the compiler itself*: reverse-engineering the pinned CL / C1XX / C2
binaries and turning their inliner and allocator into predictions, with solvers
that plug into the match loop.

Shape copied verbatim from the sibling homm2 `/Od` stack-slot model
(`~/Projects/homm2/homm2-decomp/docs/od-stack-layout.md`): **model doc /
predictor / real-compiler oracle / census gate**. The predictor is pure and
compiler-free; the oracle compiles known source with the real toolchain and
reads the answer back; the census gate re-checks every matched function so the
model cannot rot.

## Provenance hygiene (hard rules)

- **Binary-only RE.** Never consult leaked MSVC source trees. The admissible
  evidence is bytes and strings *inside our own pinned binaries* (they carry 48
  back-end source-file paths and the option tables in the clear) plus the
  infinite ground truth of compiling known source and reading the result.
- **The pinned pressing is the only subject.** `_toolchain.PINNED` hard-gates
  CL/C1/C1XX/C2/LINK by sha256+size; a wrong-hash binary aborts. The model is
  only valid against those exact bytes (SP3: C1/C1XX 12.00.8472,
  C2/LINK 12.00.8447). The one RTM addition (C2 12.00.8168, Track R) is
  admitted separately, hash-pinned, and §5-logged.
- **The Wine VC6 build stays the sole verdict.** A model prediction is a
  hypothesis; a compile settles it. Solvers propose edits — the human applies
  them after checking the evidence.

## Layout

| Path | Role |
|---|---|
| `scripts/homm3/vc6/` | the area package (`homm3 vc6 <verb>`) |
| `scripts/homm3/vc6/_toolchain.py` | hash-gated PE reader over the compiler binaries |
| `scripts/homm3/vc6/argv.py` | CL spec-table decoder → per-pass argv model |
| `scripts/homm3/vc6/passes.py` | run C1XX / C2 as separate steps (IL persistence) |
| `scripts/homm3/vc6/oracle.py` | real-compiler ground-truth runners |
| `scripts/homm3/vc6/{inline_model,reg_model,il}.py` | the predictors + solvers |
| `scripts/homm3/vc6/{diagnose,report,queue}.py` | one-function routing, plateau report, and recoverable-byte wall census |
| `scripts/homm3/vc6/_source.py` | the solvers' source-body locator (demangle + definition grammar + `#if 0` masking) |
| `scripts/homm3/vc6/_eh.py` | the EH cleanup transcript (`[ebp-4]` state stores) — object lifetimes, the one signal the three solvers do not read |
| `scripts/homm3/vc6/census.py` | the gates (each with a negative control) |
| `scripts/homm3/vc6/test_locator.py` | the `locator` gate's cases (`homm3 vc6 check --locator`) |
| `scripts/homm3/vc6/test_report_resolution.py` | negative controls for shared public-symbol routing and unclaimed flat names |
| `scripts/homm3/vc6/test_queue.py` | negative controls for MAX-first ordering and banked-exact dip exclusion |
| `scripts/homm3/vc6/shim/` | the C2-slot pass-through/instrumentation DLL |
| `scripts/homm3/vc6/ghidra_scripts/` | in-Ghidra headless scripts (no `__init__`) |
| `scripts/homm3/vc6/probes/` | one probe TU per catalogued behaviour |
| `docs/vc6/behavior-catalog.md` | the model's spec: ~80 byte-verified behaviours |
| `docs/vc6/driver-passes.md` | the CL spec-table mini-language + argv model |
| `docs/vc6/{inliner,regalloc,il-format,c2-atlas}.md` | one model doc per subsystem |
| `docs/vc6/eh-cleanup.md` | the EH cleanup-count rule + the tree-wide transcript divergences |
| `evidence/vc6/*.tsv` | generated tables (regenerate, never hand-edit) |
| `build/re/vc6/` | the Ghidra project (gitignored scratch) |

## Residual-routing contract

`homm3 vc6 queue` ranks actionable rows hardest-first by ascending effective
MAX (`max(current score, banked MAX)`). A banked-exact function is excluded
even when its current score has dipped; those collateral dips are reported
separately and may only be reopened by an evidence/source-gate failure. The
census still records recoverable bytes as wall-mass accounting, not as its
priority key. It includes functions whose retail address is known but whose
source body is still inactive or absent. Those
unclaimed rows are reconstruction work, not solver failures. Before invoking
disassembly, the router now requires one unique public text symbol shared by
the compiled base object and the delinked target object. A retail/synth flat
label with no compiled counterpart is recorded as `unclaimed (no source
binding)` without printing an objdump error; a genuine diagnosis failure stays
visible as an `unclassified` row with its reason.

The shared-symbol preflight is asymmetric in the useful direction: exact
mangled identities and uniquely resolved source basenames may enter the
register/control-flow/inliner solvers, while ambiguous substrings and symbols
present on only one comparison side may not. `homm3 vc6 check --locator`
includes the measured flat-label defect and the one-side-only case as negative
controls, so the census cannot silently regress into treating missing source as
a compiler wall.

## Status

Phase 0 (driver ground truth + probe rig) in progress. See the approved plan
at `~/.claude/plans/good-now-onto-the-lexical-allen.md` for the full phase
sequence and `docs/gruntz-script-port.md` §5 for the decision log.
