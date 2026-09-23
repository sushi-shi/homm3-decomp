# PR #78: generic data checks against the runtime repairs

The generic checks detect several repaired defects, but they do **not yet cover
every case in PR #78**. Missing startup bindings, conflicting town declarations,
wrong quest vtable targets and missing link definitions have concrete diagnostics.
The complete hero-table bound, minimap stride, aggregate-copy initializer and
some control-flow relationships still need stronger general checks. A non-exact
function score or an unproved expression is not a specific diagnosis of those
defects. This audit remains open.

## Revisions and method

PR #78 was refreshed before the audit. Its base is
`0dbc798f804317c176c014ac28936ac59dbd1b49`; its head is
`6e854110c776eccf6f6e962ba23d4550779ebcdf`. Both use identical matching tooling
through PR #93, `675101a669b6de4fd4854ec0f8736f44f8c5527f`. The base is the
verified [gap-review checkpoint](data-gap-review.md). The head was freshly
compiled in an isolated worktree with the same scripts and retail extent
inventory. All 512 tooling/input files in that overlay were checked for equality.
Authored game source, vendor source and source-admission manifests were left at
their respective revisions; no case-specific rule was added.

Both full builds and their existing gates pass. Both run actual objdiff data
comparisons. Independent audits reconstruct the strict comparison views from
raw COFF and retail, replay source/vendor code paths, declaration bindings,
compiler records, compiler-copy identities, startup effects and consumer operands.
The head compares 4,168 projections across 233 units, totaling 218,818 packed
bytes, and withholds 561 projections. Packed bytes include repeated copies;
the distinct retail denominator below is unchanged.

Both exhaustive exports independently cover all 2,732,032 file bytes and
2,842,624 image bytes without holes or overlaps. The head's consumer, parse-region
and reference-role exports agree byte-for-byte with its build checkpoint.
Unexplained data ownership falls from 114,175 to 112,573 bytes. Both strict
exactness controls fail on the visible backlog, with no unavailable analysis.

The ordinary link command is also part of this audit. The base fails with the
single unresolved identity `??0TDebugBreak@@QAE@XZ`; three callers report it.
The head links with zero unresolved externals and zero duplicate-symbol warnings.
The linked candidate's actual CRT slots were reconciled with the VC6 map and
emitted initializer records. In particular, the game-context binding precedes
the network-player initializer. Unsupported tables and unresolved map symbols
remain separate; this does not claim all startup effects are understood.

## Distinct-byte comparison

| Measurement | Base | Head |
|---|---:|---:|
| Retail data denominator | 470,624 | 470,624 |
| Enrolled | 289,593 | 291,591 |
| Initialized matches | 157,951 | 159,265 |
| Static zero-fill agreement | 109,192 | 109,772 |
| Unresolved pointer bytes | 17,048 | 17,164 |
| Pointer mismatch bytes | 16 | 4 |
| Binding conflict bytes | 2,508 | 2,508 |
| Unenrolled | 181,031 | 179,033 |

The complete byte transition is: 1,110 unenrolled bytes become fixed matches,
204 become pointer matches, 104 become unresolved pointers and 580 become
zero-fill agreements. Twelve pointer-mismatch bytes become unresolved pointers.
No other byte verdict changes. That last transition is **not** pointer-match
credit. Fixed differences (72), zero-fill differences (1,051), and missing
relocation bytes (1,755) remain unchanged.

Paired startup registrations rise from 24 to 28, exact effect pairs from 14 to
18, and proved constant-effect bytes from 326 to 374. These effects are separate
from static zero-filled storage. Consumer entry pairs rise from 5,680 to 5,691;
known address expressions are 139,889 of 292,064 on the head. Both versions still
report zero proved scale changes. Their unsupported paths prevent interpreting
that zero as evidence that all strides agree.

## Case matrix

Addresses below navigate the audited evidence. They are not matching-policy
exceptions or addresses added to the checker.

| Repair | Generic base evidence | Head evidence | Coverage and remaining work |
|---|---|---|---|
| Game-context startup binding | The four-byte cell at `0x69923c` agrees as static zero, but its retail initializer is unpaired and its startup write is absent from candidate evidence. The backing dword is unenrolled. | The backing dword matches; the binding initializer is `effects-exact` for four bytes. | Missing initialization is diagnosed independently of zero agreement. Actual linked order puts this binding before the player initializer; the player's full effects remain unproved. |
| Archive search lists and contexts | All 100 list bytes are unenrolled. The 96-byte context array agrees as static zero; retail initializer `0x559320` is unpaired. | All 100 list bytes match. The context initializer remains unpaired despite its emitted body. | Partial: four `rep movsd` operations prevent the effect analysis from recovering the context writes. `no-observed-startup-write` is a coverage gap, not successful initialization. Add bounded aggregate-copy effects. |
| Live sound descriptors | The 36-byte descriptor array agrees as static zero, with retail startup writes absent from candidate evidence. | Its initializer is `effects-exact` for all 36 bytes, including independently resolved addresses of live cells. | The repaired initializer is covered; whole-program loading and later mutations are separate obligations. |
| Sprite decoder constants | The one-byte run marker has a binding conflict; its initializer is unpaired. The four-byte maximum is unenrolled. | The maximum's initializer proves value 256 exactly. The marker's initializer remains unpaired because its address identity is conflicted. | Partial: the four-byte `TBlendMask` projection at `0x6968a4` overlaps the marker at `0x6968a6`. Preserve that layout conflict and improve initializer diagnostics without pretending the identities are consistent. |
| Complete hero-table extent and portrait pointers | The emitted table ends after 156 rows; the final 644 retail bytes are unenrolled. All 312 portrait pointer fields remain unresolved. The selection consumer has no proved outside-source-extent diagnostic. | The added tail contributes 588 fixed matches and 56 unresolved pointer bytes. Across all 163 rows, 13,692 fixed bytes match and 1,304 pointer bytes remain unresolved. | Incomplete: the gap is visible, but the checker does not yet prove the consumer's complete loop domain or independently bind all 326 portrait referents. A correct prefix is not a complete-table verdict. |
| Town names: shared identity and row dimensions | The declarations disagree in size and shape, `[9][16]` versus `[9][17]`; the 576-byte backing range is withheld. | One shared definition supplies 576 zero-fill agreement bytes; that shape/size conflict disappears. | The declaration defect is diagnosed generically. Zero agreement still does not prove text parsing; the reader's retail code match is separate evidence. |
| Minimap byte pitch versus pixel stride | The consumer differs, but three indirect branches stop analysis before most pixel writes. No scale-change diagnostic is emitted. | The broad consumer comparison remains incomplete. | Incomplete: add independently proved switch-table edges and address-flow evidence for row updates. Raw inspection shows the changed coefficients, but the current generic report does not isolate them. |
| CD setup result overwritten | Ordinary code/CFG comparison reports differences in `earlySetup`; static data does not establish preservation of a call result. | The function still has code/CFG differences. | Incomplete: isolate the call-result overwrite and guarding predicate through general data flow; a non-exact whole function does not verify this fix. |
| Optional Host widget | The multiplayer constructor differs in code/CFG and consumer observations. | It remains non-exact after the guard is added. | Incomplete: relate the optional pointer, null guard and later use; existing aggregate differences do not prove that relationship. |
| Missing out-of-line `TDebugBreak` constructor | The ordinary linker reports the unresolved constructor. | The ordinary link succeeds with no unresolved or duplicate symbols. | Link closure detects this case directly. No data-byte rule is needed to disguise a missing function definition. |
| Seer-name reference initialization | Static zero agreement hides a missing binding; retail initializer `0x56c3d0` is unpaired and its write is not observed in candidate startup. | Its four-byte binding is `effects-exact`. | The missing reference initialization is covered independently of its initial zero bytes. |
| Quest loader virtual slots and payload reads | Slot 12 in three independently placed vtables points to the base loader instead of the respective retail targets: twelve `pointer-mismatch` bytes. | The new override targets have no independent address anchors, so those twelve bytes become `pointer-unresolved`. | Wrong slots are diagnosed. Further generic referent-body/call-effect evidence is needed to verify the replacement payload readers; do not turn an unresolved target into a match. |
| Town animation timer min/max | Code/CFG and consumer comparisons report differences in `townManager::main`. | The function remains non-exact after the selector changes. | Incomplete: recover the timer update's comparison/select relationship. Static data equality and a whole-function difference do not verify the intended lower bound. |

## Code comparison limits

The audited current function scores retain regressions instead of assuming every
runtime repair improves instruction similarity:

| Function | Base CUR | Head CUR |
|---|---:|---:|
| `earlySetup` | 99.01462% | 98.429825% |
| `TMultiPlayerWindow` constructor | 83.76605% | 83.79068% |
| `townManager::main` | 91.15452% | 91.04056% |
| `advManager::updateRadar` | 91.2477% | 90.54954% |

All four code/CFG comparisons remain non-exact. This is evidence for continued
analysis, not proof that the runtime fixes are wrong. The raw comparison used
the last completed builds. Optional candidate `/Z7` source labeling was unavailable
in the read-only summary invocation; no claim here relies on those labels.

## Required generic follow-ups

1. Model bounded repeated copies/stores, including known stack temporaries,
   direction and count, without hiding unknown writes or pointer identities.
   Recover generated initializer anchors from their actual compiler/source
   evidence, keeping value comparison separate from conflicting storage.
2. Prove supported indirect switch targets, preserve reachable instruction
   boundaries and trace address relationships through the resulting branches.
   Extend loop-domain evidence across supported calls without assuming arbitrary
   calls preserve memory. This must diagnose table-tail access and byte scaling.
3. Compare supported indirect referent bodies and calls without deriving strict
   pointer identity solely from the pointer being tested. Extend call-result,
   guard/use and comparison/select diagnostics for the behavioral cases.
4. Replay the same checks on both revisions after each extension. Add generic
   negative controls with unrelated names, counts, addresses and layout choices.
   Refresh PR #78 again before the final completion audit.

The byte-accounting stack remains useful and the failures above are explicit,
but this matrix does not satisfy the goal's completion criteria yet.
