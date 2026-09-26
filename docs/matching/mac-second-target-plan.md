# Mac PowerPC as a second byte target

This document records the target design and initial controls. For the expanded
worker campaign, implemented tooling and remaining coverage requirements, see
the [implementation report](../tooling/mac-matching-report.md).

## Objective

The product is the reconstructed **Windows game**. Mac compilation and byte
comparison exist only to recover its source: retained helper calls, clearer
control flow and local lifetimes inform the Windows implementation. Preserve
supported helper definitions and source calls in that implementation even when
VC6 expands them. No runnable Mac game or Mac OS implementation is required.

Match recovered C++ against both the pinned Windows VC6 executable and the
Classic Mac OS PowerPC PEF. `homm3 build --fast <TU>` should compile and compare
both targets for admitted Mac counterparts in that TU; a full `homm3 build`
should refresh and check every admitted counterpart. Keep separate scores and
provenance for the two targets. A Mac match is an exact byte claim about the
Mac build, not a substitute for the Windows retail verdict.

The Mac build becomes the preferred cross-reference for source spelling and
ordinary helper boundaries where a counterpart is paired. Dreamcast CodeView
remains evidence for names, types, locals, line positions and older source
facts that the stripped Mac executable cannot provide.

The ordinary-header migration and its current limitations are tracked in
[native header compilation](../tooling/mac-native-headers.md).

## Verified starting point

- `../macos/Heroes_III_raw.pef` is a PowerPC PEF, 3,418,835 bytes, SHA-256
  `650be8880cfda81ffa7704ce3bcdb9c5a6528f67afdf77c0c0c63e8259250d86`.
- Its first code section begins at file offset `0x6c00`. The paired
  `hero::getHighestSchool` starts at **section offset** `0x106188`, hence file
  offset `0x10cd88`. The observed 37 words occupy `0x94` bytes and end in
  `blr`. Confirm the function boundary independently before admitting it.
- The local CodeWarrior compiler is `../macos/cw/tools/MWCPPC.exe`, SHA-256
  `96377c7da7864e3fa067eda9da00e6c01505d2935425d21cfda93c48d9f892ac`.
  `MWLinkPPC.exe` has SHA-256
  `1fe73016f9d8f4ce8ea7ff07d09fd900422c08b818c19e281212b131ca52118e`.
  `-O1 -proc 750 -nolink` produces named `MWOBPPC` code hunks with the pinned
  compiler. The 750 scheduling profile is distinguished by the non-leaf
  patrol function's prologue and epilogue; both leaf controls remain exact.
  This `-O1` control is historical evidence; the current game-wide working
  comparison uses `-O3` pending broader source and compiler calibration.
- The source change in `src/hero.cpp` that removes the synthetic `else` and
  puts `bestLevel` before `bestSchool` is already in this worktree. The Windows
  full checkpoint reports that function exact. The earlier `-O1` Mac control
  reproduced it byte for byte; the shared `-O3` profile must be assessed
  separately.

## Labeling rule

The existing `VA(address, size)` in an owning source file is the function's
project identity. A `MAC_ADDRESS(offset, size)` claim beside it pairs that
Windows VA with one Mac PEF section-relative byte span. `homm3 mac show <Windows-VA>`
and `homm3 mac show mac:<section>:<offset>` resolve to the same pair and inherit
the source name and unit. Pairing evidence is mandatory; similar names or
ordinal positions alone cannot admit a Mac span. A Mac-only function needs a
separate documented disposition until it can be tied to an authored source
claim. This avoids pretending that section offsets are Windows VAs.
`homm3 mac labels` prints the scored section-offset/name mapping as TSV.

The current admitted control is `hero::getHighestSchool` at Windows VA
`0x004e51c0` and Mac section `0` offset `0x106188`. The target toolchain
originally compiled the admitted bodies in `src/hero.cpp` through temporary
declaration views. Those duplicate headers have now been removed: candidates
reuse the source include prefix and ordinary project headers. The following
scores describe the historical declaration-view controls, in authored source order. All `0x94` target bytes match. The adjacent `hero::getSpellSchoolLevel` is
paired at Windows VA `0x004e5100` and Mac section `0` offset `0x106090`;
its unique switch prefix, skill offsets and boundary at the next paired
function support the label. A natural `if/else` with one shared return resolved
the two differing instructions and now matches all 248 Mac bytes and the
Windows VC6 target from the authored source.
`hero::isInPatrolRadius` is paired at Windows VA `0x004e56e0` and Mac section
`0` offset `0x10671c`. Its full 208 bytes match, including two direct calls to
the reviewed four-instruction `abs` body at `0x288778`. The same authored
function expands `abs` in Windows VC6.

Two further tooling controls are paired: `hero::getExperience` at Windows
`0x004da3a0` / Mac `0:0xf6290` (184 bytes), and
`hero::getExperienceIncrement` at `0x004da420` / `0:0xf6348` (72 bytes).
The first exposes named table and floating-constant references; the second
exposes two calls between game functions. The existing shared source is
retained while tooling is developed. Five admitted pairs are not a Mac census.

Current implementation: pinned PEF/toolchain staging, bounded section reads,
source-VA pairing, CodeWarrior compilation, named MWOB code-hunk extraction,
exact comparison, `homm3 mac` navigation, build-loop integration and a separate
Mac score ledger are in place. Named `HUNK_XREF_24BIT` direct calls within a
PEF code section are resolved against admitted source pairs or hash-verified
runtime spans. The comparator reproduces MWLink's `-collapsereloads on`,
including internal branch adjustments after removing unused RTOC-reload NOPs.
Its output was checked against an actual PEF produced by the pinned linker.
The PEF pattern-data decoder and symbolic loader resolve 28,670 pointers and
363 imports in the pinned executable, with its TOC at data section
`1+0x8000`. TOC loads resolve through reviewed `DATA` identities or emitted
literal payloads. Named data definitions are extracted from owning source;
anonymous CodeWarrior `@number` names never establish identity. Supported
RW/RO address loads also reproduce the linker's in-range `lwz` to `addi`
relaxation. A separately linked CodeWarrior control validates packed data,
TOC pointers and all 184 instruction bytes.

CFM import glue resolves 362 stubs through exact instruction forms and
loader-proven transition-vector imports. The reviewed `__ptr_glue` has 3,237
incoming calls; these remain logically indirect in call reports. Imported and
indirect calls restore r2 from 20(r1), as verified with real MWLink controls
for imported, function-pointer and virtual calls. Their reserved slots are
never collapsed. Unknown call targets, unsupported relocation types, relocated
data payloads and calls across sections remain explicit comparison errors. A differing referenced data initializer currently produces an explicit
unsupported-data error; it is not silently accepted as an exact function.

`homm3 mac calls [selector]` compares static call-site counts and ordered
targets, including unresolved candidate calls before linking. Indirect targets
remain unknown; local linked branches and outgoing possible tail branches are
separate observations. Both builds and this command write
`build/mac/calls.json` and `build/mac/calls.tsv`.
See the [tooling guide](../tooling/mac-matching-roadmap.md) for how pairs are
scored from full-TU objects.

Ninja rebuilds a unit's full-TU object when its source or header closure
changes. A function's own source identity excludes neighboring bodies, so
context changes do not reset that function's MAX. Partial unit reports retain
other units' observations. Reports record source and target hashes plus each
resolved call/data reference and removed/restored reload slot.

## Implementation sequence

1. **Pin and parse the target.** Add `[inputs.mac]` to `config/project.toml`,
   `HOMM3_MAC_EXE` and `homm3 init --mac-exe`, staging the user-supplied PEF
   under ignored `build/orig/mac/`. Verify size, SHA-256, PEF magic, section
   table and bounds at every read. Represent Mac addresses as section index
   plus section-relative offset; never mix them with Windows VAs or raw file
   offsets. Pin the CodeWarrior executable/DLL hashes and use a separate Wine
   prefix from VC6. Do not commit proprietary executable or compiler bytes.

2. **Admit function pairs.** Add a small hand-reviewed Mac pairing inventory
   keyed by the function's existing Windows `VA(addr, size)` claim. Each row
   records Mac section, start, end, and concrete pairing/boundary evidence.
   Generate names from the existing source claim; do not maintain a second
   symbol-name ledger. Begin with `hero::getHighestSchool`, then pair a few
   functions with ordinary helper calls and relocations before scaling up.
   Unpaired functions have no Mac score.

3. **Compile the same authored body.** Extract each admitted function body
   from its owning project source using its `VA` claim. Supply a minimal,
   versioned Mac declaration/layout shim for dependencies that the Windows TU
   cannot pass directly to CodeWarrior. The shim must not duplicate or alter
   the function body. Record hashes of the body, shim, compiler binary, flags
   and target span with every result. Require an actual `MWOBPPC` output and
   the expected function symbol; `-c` syntax checking alone cannot count as
   a build.

4. **Compare exact bytes.** Parse the emitted MWOB function section and its
   relocations, and compare it with the admitted PEF span. First close the
   no-relocation `getHighestSchool` control, including all 37 words. Then
   resolve branch, glue, data and import references before declaring matches
   for functions containing relocations. Report raw mismatching words and
   relocations separately. Never mask a disputed call target into an exact
   result.

5. **Join the matching loop.** Add `homm3 mac show|disasm|diff` for navigation.
   Make `homm3 build --fast <TU>` compile and compare admitted Mac functions
   in that TU after the VC6 work; make the full build check every admitted Mac
   pair and refresh a separate Mac CUR/MAX/HIST ledger. Fail a build on missing
   pinned inputs, missing compiler output, invalid pairs or comparison-tool
   errors. Report score changes without making a legitimate source-supported
   dip fatal, following the Windows ledger policy. Keep the Windows summary
   and the Mac coverage denominator separate.

6. **Roll out and document.** Add the third executable's verified size/hash,
   `HOMM3_MAC_EXE` quickstart step and two-target score/coverage to README.
   Update `AGENTS.md` so an admitted Mac pair receives a Mac evidence and
   exact-compile pass before speculative rewrites, while Dreamcast positive
   source facts and the Windows retail verdict remain in force. Document
   pairing confidence and platform-specific divergences beside owning source.

7. **Expand to the complete shared game.** Inventory Mac code functions and
   account for every cross-platform game function in both directions: paired,
   Mac-only, Windows-only, folded, inlined, or platform-rewritten, with evidence
   for each exception. Use constants, strings, vtable slots, field accesses,
   neighboring functions and the paired call graph to grow the address map.
   The `getHighestSchool` pilot is infrastructure proof, not a reduced coverage
   goal. A module is closed only when its shared functions match both targets
   exactly and its unpaired code has an explained disposition.

## Acceptance checks before enabling the build gate

- A clean setup stages all three verified executables and a verified
  CodeWarrior toolchain without reading a sibling checkout implicitly.
- The admitted `getHighestSchool` pair reproduces the `0x94`-byte Mac span
  exactly from the current project function body, while the Windows target
  remains exact. A one-statement source perturbation must be detected by
  both comparisons and restored cleanly.
- At least one paired ordinary helper caller and one relocation-bearing
  function prove that symbol extraction, retained-call decisions and Mac
  relocation handling work. Compare named call sites, not aggregate call
  counts.
- Parser and gate tests reject a wrong PEF hash, wrong section offset or
  extent, stale/missing object, missing symbol, and a changed compiler/shim.
- A full build reports exact counts against **only admitted Mac functions**;
  it does not imply whole-game Mac coverage. The Windows score and existing
  source/evidence gates continue to run.

Long-term completion requires the bidirectional Mac/Windows function inventory
to be accounted for, all shared game functions to reach exact matches in both
targets, and platform-specific code to have documented source ownership. The
admitted-pair denominator must grow with that inventory; a 100% pilot score is
not a whole-game result.

## Open decisions to settle with the pilot

- Confirm the exact CodeWarrior 2.4 build and flags from the emitted object,
  then pin that profile. One function is insufficient to infer a global
  optimization setting or stable inlining policy.
- Extend shared Mac declarations and compatible library headers. Current TU
  profiles compile admitted bodies plus explicitly listed source helpers in
  original order; they do not claim that the whole Windows TU builds for Mac.
- Confirm each Mac function's boundary through code, call references and
  neighboring entries. A `blr` alone does not prove a span when a function
  has multiple returns or internal branch islands.
