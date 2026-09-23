# Strict matching for VC6 exception metadata

Full builds now compare typed records inside independently code-anchored,
per-function VC6 `.xdata$x` contributions. This extends
[code-derived data matching](code-data-matching.md) to FuncInfo headers, unwind
maps, try maps and catch maps. It also resolves local cleanup labels through
checked code offsets, while preserving their object-local identity.

## Address and extent proof

`analysis/compiler_eh.py` recognizes the compiler's `mov eax, FuncInfo; jmp
___CxxFrameHandler` sequence only inside an independently matched code
contribution. Its jump must target the admitted runtime identity. An arbitrary
magic value in data, an unproved code root, or a conflicting source/root address
cannot establish the record. Both stub instructions must begin at decoded
instruction boundaries; opcode-like bytes inside another instruction do not
establish an EH root.

That root fixes the base of one private compiler contribution. COFF symbol
offsets within the same `.xdata$x` contribution supply map addresses; retail
pointer fields supply none. Ordinary game data sections retain allocation-level
binding and do not inherit this layout rule. A synthetic pinned VC6 LINK control
confirms that internal offsets survive linking and that associative contributions
follow their selected parent in both input orders.

The version supported here is the pinned x86 FuncInfo prefix with magic
`0x19930520`: 28-byte headers, 8-byte unwind entries, 20-byte try entries and
16-byte catch entries. The existing retail EH reader and emitted VC6 objects
establish those extents; the modern
[LLVM Microsoft EH emitter](https://llvm.org/doxygen/WinException_8cpp_source.html)
also documents these record fields, but its later header extensions are not
assumed to exist in VC6 records.

Candidate counts bound each map. Every map needs a unique DIR32 relocation to a
private symbol in that same contribution, an emitted allocation starting at its
target, and enough physical storage for the typed extent. Unsupported versions,
oversized counts, interior pointers, external targets and IP-to-state maps remain
explicit issues. An unsupported header version retains the independently typed
28-byte prefix comparison rather than silently hiding its differing bytes.
Inter-record spacing and trailing contribution bytes are excluded. Compiler
record extents do not become source array-bound proofs.

Changing a retail map pointer leaves all expected addresses unchanged and yields
a strict pointer difference. Changing a map entry leaves it enrolled and yields
a byte difference. The matcher never follows the retail pointer to relocate the
candidate map and then declares that same pointer correct.

## Folded copies and code labels

An associative COMDAT can share an identity only through its actual parent
selection record. The parent must be a supported named code COMDAT with one
public symbol at its beginning; the private EH contribution must be the unique
such child of that parent. The identity also includes the record kind and local
offset. Every enrolled copy still receives comparison, so a good copy cannot
hide a bad copy. Equal metadata alone does not establish coalescing.

Local code labels use exact raw-COFF symbol indices and offsets inside checked
code contributions. They do not become external function identities, and labels
in the trailing NOP region excluded by code comparison remain unproved. Ordinary
source function claims still prove only their entries; nonzero code addends are
not inferred from those claims.

## Generated evidence

The normal checkpoint and coverage command add two literal TSV files:

| File | Evidence |
|---|---|
| `code-compiler-records.tsv` | Root and child allocation identities, kinds, typed lengths, local offsets, code paths and associative identities |
| `code-compiler-issues.tsv` | Unsupported or malformed records, with owning root and reason |

These join `code-data-bindings.tsv`, `code-data-anchors.tsv`, raw object hashes
and the existing strict verdict and relocation tables. Complex cells contain
JSON. Both module freshness and compiler issues participate in the strict gate.
Raw candidate objects remain unchanged.

## Measured checkpoint

The pass finds 427 FuncInfo headers, 427 unwind maps, 16 try maps and 16 catch
maps. Their typed candidate spans total 34,900 bytes inside 36,672 bytes of
emitted contributions; 1,772 bytes of spacing remain excluded. Repeated emitted
copies are retained, while the retail denominator counts each byte once.

| Distinct retail data bytes | PR #88 | With EH records and labels |
|---|---:|---:|
| Denominator | 470,624 | 470,624 |
| Enrolled | 232,920 | 265,924 |
| Matched initialized bytes | 106,595 | 140,599 |
| Zero-fill agreement | 104,395 | 104,395 |
| Unenrolled | 237,704 | 204,700 |
| Fixed-byte differences | 65 | 72 |
| Resolved pointer differences | 16 | 16 |
| Zero-fill differences | 1,051 | 1,051 |
| Missing relocation evidence | 1,754 | 1,755 |
| Unresolved pointer identity | 16,864 | 15,856 |
| Conflicting bindings | 2,180 | 2,180 |
| Unexplained ownership | 114,335 | 114,335 |

The union gains 33,004 enrolled bytes and 34,004 matched initialized bytes,
without losing prior enrollment or matches. Newly enrolled metadata contributes
20,572 fixed-byte matches and 12,412 pointer-verified bytes; another 1,020
previously unresolved pointer bytes now match through checked code labels.
Seven new differing bytes and one missing-relocation byte remain explicit.
The catch records expose missing or different catch-type metadata even where
the associated handler code matches.

Combined strict comparison finds 4,048 of 4,609 projections statically exact.
Objdiff compares 4,070 projections across 229 units, withholding 539, for
202,909 packed bytes. Initialization and consumer totals remain unchanged:
22 of 1,149 registrations paired, 230 bytes of proved constant effects, and
5,680 consumer entry pairs with 138,663 of 291,996 address expressions known.

Validation includes 127 targeted tooling tests, 151 sema tests, the full VC6
build and the pinned linker control in both object orders. The function ledger
is unchanged. Independent audits reconstruct all 886 records and their rooted
code addresses, validate 3,030 local-label relocation references, recheck existing
source/vendor bindings, and rebuild the objdiff projections from raw COFF.
Consumer audits verify raw operands, call targets and bounded ranges. Generic
negative controls cover incorrect map pointers and entries, bad counts and
versions, missing roots, cross-contribution targets, conflicting source anchors,
incorrect associative ownership, differing folded copies and excluded code
alignment labels. The exactness CLI correctly exits nonzero on the remaining
backlog, including a negative control whose supported prefix bytes match but
whose compiler record version remains unsupported. Exhaustive coverage audits
verify all 2,732,032 file bytes and 2,842,624 mapped-image bytes; compiler, static
and consumer TSV exports agree with the full-build checkpoint.

These measurements separate actual equality from metadata attribution and
zero-fill agreement. They do not prove that exception handling, initialization
or all consumers behave correctly. Remaining unsupported source extents and
the refreshed PR #78 audit continue in the [stack plan](data-matching-stack-plan.md).
