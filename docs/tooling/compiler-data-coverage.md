# Compiler and linker structures in DATA gaps

`homm3 sema coverage --output build/compiler-accounting` now adds a structural
accounting channel for validated VC6 EH metadata, admitted vtables and the final
PE import-directory descriptor. This follows the vendor layer in PR #81.
The original DATA and vendor measurements remain intact.

`sema/compiler_data.py` reuses the retail claims' validated extents. It associates
FuncInfo records with their handler stubs and unwind/try/catch maps, retaining
every association for shared maps. Function ownership requires an admitted
retail reference at a decoded `push imm32` instruction in an admitted function.
A nearby label or an embedded `0x68` byte cannot establish ownership. Generated
function names/source paths are navigation snapshots; the instruction and RVA
association supplies the ownership evidence.

Vtable class identities come only from the reviewed class column in
`config/retail/vtables.tsv`; every slot must point into backed retail code.
Unnamed classes remain explicit. The final zero descriptor is classified only
as part of the already validated, bounded PE import walk; an arbitrary zero run
does not become linker padding or an import record.

New artifacts are `compiler-data.tsv` (extents, family, owners, class, FuncInfo/
handler relationships, vtable targets and evidence) and `compiler-issues.tsv`
(invalid structures and missing ownership). The full map gains `compiler_status`,
IDs, families, owner/class fields, ownership gaps and candidate status.
`data-unaccounted.tsv` excludes validated structures as well as verified vendor
contributions; `data-gaps.tsv` still contains every absent DATA declaration.

Every structural row says `candidate_status=not-compared`. Both candidate compared
and matched byte totals are **zero** in this layer. This is not data enrollment,
source recovery, initialization proof or a candidate/retail byte verdict. Those
are subsequent stages in the [stack plan](data-matching-stack-plan.md).

## Measured pinned-image result

| Measurement | Bytes |
| --- | ---: |
| Structurally accounted data, including prior coverage | 88,244 |
| Additional gaps accounted after DATA and vendor layers | **84,612** |
| Remaining unaccounted data | **142,147** |
| Structural storage with unresolved owner/class | 15,864 |
| Candidate bytes matched by this layer | **0** |

The additional gap coverage comprises 72,572 bytes of EH metadata, 12,020
vtable bytes and the 20-byte import terminator. Of 1,147 FuncInfo records,
1,002 have decoded function owners. Of 364 vtables, 83 have admitted class
identities. All 573 unresolved-owner findings remain exported. Missing ownership
does not invalidate an independently proven structural extent.

The independent export audit compares every parent byte-map row, all DATA/vendor
totals and retail categories, checks the structural interval join and emitted
candidate-status fields, verifies vtable slots and input hashes, and reconstructs
the remaining gap table. Regression controls cover embedded false pushes,
unadmitted references, unnamed classes, invalid vtable targets, shared EH maps,
arbitrary zero runs, conflicting extents, denominator preservation and TSV output.
