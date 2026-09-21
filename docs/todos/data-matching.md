# Data matching

Function scores do not establish that every initializer, pointer or byte in the
executable has been reconstructed. Extend data matching with independent retail
extents and explicit pointer ownership, keeping enrollment separate from match
quality. Candidate object sizes alone cannot prove retail boundaries.

The independent whole-file/image accounting layer is implemented in
`homm3 sema coverage --output build/retail-accounting`; start with its
`backlog.tsv`. See [complete retail byte accounting](../tooling/data-coverage.md)
for the measured totals, evidence policy and remaining identification work.
Source/candidate binding, enrollment and pointer/access-width verification
remain separate unfinished tasks.

## Implementation checklist

1. **Make data enrollment explicit.** Display disabled data comparison as such.
   Add a strict relocation report using explicit project options shared by the
   GUI and CLI. Preserve the existing checkpoint history with its scoring policy;
   do not silently compare maxima recorded under different policies. Classify
   strict differences by resolved identity before treating them as code defects.

2. **Complete data claim extraction and binding.** Extend the existing source
   claim channel with declaration identity, type, linkage, candidate size and
   alignment, confirmed against VC6 output. Keep source annotations authoritative
   for names and produce manifests from the model. Track retail extent evidence
   independently. Handle extern-only declarations, static locals, COMMON guards,
   pooled strings and identical-content copies explicitly.

3. **Add an independent retail coverage report.** Account for complete PE region
   ranges, including their leading/trailing unknown areas. Partition bytes into
   proven objects, system structures, provisional extents, reviewed padding and
   unknowns. Report overlaps and retail-touched gaps; keep undecoded accesses and
   unknown pointer flows visible. Count unique retail bytes once, regardless of
   how many COMDAT copies objdiff compares. Do not infer absence from zeroes.

4. **Enroll a bounded first set and compare data.** The cursor constant arrays
   are a straightforward pilot, followed by ordinary globals, reviewed vtables
   and independently identified zlib/CRT data. Emit correct per-unit definitions
   and section topology, materializing initialized target bytes only from retail.
   Candidate topology is a comparison arrangement, not retail source evidence.
   Preserve unknown gap bytes as explicit provisional target data instead of
   making them disappear. Fail on enrolled data in a unit objdiff never opens.

5. **Add referent and extent gates alongside objdiff.** Check pointer-table slots
   and symbol-relative addends against independently resolved retail addresses;
   audit access widths and bounded array accesses against declared storage.
   Require negative controls for a changed initializer, wrong pointer, changed
   addend, undersized array, overlapping claim and silently unscored data unit.
   Apply the BSS scoring relaxation only once genuine extent defects are caught
   independently. Adopt strict relocation handling consistently in both binaries.

6. **Extend to the complete image and real link.** Cover library data, EH and
   initializer tables, resource/import structures and embedded code-section
   tables. A successfully forced link does not prove closure or correct placement.
   Later compare link-assigned storage, alignment, initialization order and
   pointer referents, then exact RVAs when the full layout is reconstructed.

Maintain separate measurements for **retail bytes identified**, **bytes enrolled
in comparison**, **enrolled data matched**, and **pointer fields verified**.
This prevents a shrinking comparison denominator or a large zero-filled object
from appearing to establish complete data correctness.
