# Mutable counted packed-plane writer

`writePackedBitsCount` in `packed_bits.h` now owns the repeated zero/pack/write
operation for all four `NewfullMap::saveObjectType` masks and the padded spell
mask in `town::save`. It is a distinct raw-count API, not a constness-dependent
overload of the existing const success-returning writer. That writer and the
reader helpers remain unchanged.

DC `mapcell.obj:0xf22cc` names one `outBuf[6]`, signed `int i` and `int count`;
lines 3684/3696/3708/3720 retain mutable bitset subscript/proxy conversions.
The four calls therefore share their existing caller-owned buffer. Mac
0x126a2c..0x126c88 repeats the corresponding clear-six/test-48/pack/write-six
operation, preserving the mutable reference path to `test<48>`.

DC `town.obj:0x165988` explicitly names `outBuf[70]`; Mac 0x1b2718 clears 70
bytes, 0x1b2778 bounds the loop at 70 bits, and 0x1b278c writes 70 bytes.
The 61 trailing zero bytes are part of the saved-game format. The helper takes
both the native bit bound and the caller's byte extent, preserving the signed
`hero::NUM_SPELLS` bound for town and unsigned `sizeof(packed) * 8` for mapcell.
This interface is tied to those observed expressions, not a speculative new
serialization feature. The actual int write result reaches the original
unsigned `< sizeof(buffer)` checks unchanged, including negative/overcount
conversion behavior. Each mask still performs one write.

The name, interface and grouping are inferred; the older DC declarations do
not prove a named template. Positive shared-buffer evidence argues against
inventing separate buffer lifetimes for each call. A deduced reference-to-array
extent was rejected by VC6 (C2265/C2783), so caller storage and extent are passed
explicitly. Native Mac expands the entire writer through the mutable proxy to
`test`; an ordinary template left a new writer call. `inline` reproduces the
observed expansion. No const conversion, platform fork, inline pin, dummy
statement, new linking manifest, or score waiver was introduced.

## Targeted results

Fresh `homm3 build --fast mapcell town` compares this source against retail:

| Function | Prior Windows MAX | Measured final score | Explanation |
| --- | --- | --- | --- |
| saveObjectType, 0x00503c40 | 100% | 94.09% | Shared writer expands, but shadow/trigger retain `bitset<48>::test` where retail expands it and retains `_Xran` |
| town::save, 0x005bd2f0 | 100% | 100% | Caller buffer, signed bound, mutable bit access and one 70-byte write preserved |

The final `vc6 predict-inline --no-build --trace` reproduced the compiled
object exactly (caller cb 289, initial budget 1000, 30 budget tests). This is
a nested library inlining residual, not a missing stream operation. No source
was added solely to move the budget, and the recovered helper calls remain.

Selected Mac builds before/after retain all **66 scored mapcell/town pairs**,
24 exact, with no loss of comparison availability. Of these, 65 scores are
unchanged. `town::save` moves **64.616402% -> 64.550265%** (977 -> 976 matching
bytes out of 1512); linked candidate extent remains 1496 bytes and the same
named direct-call offsets/targets remain: armyGroup::save, memset, test<70>.
Both baseline and candidate have the existing memset-versus-native-bzero
call difference. The remaining one-byte score movement is recorded without
claiming an exact causal instruction or adding a Mac waiver.
`saveObjectType` is unavailable in both states due to the existing unresolved
`writeScalar<char>` relocation; its shape/call report confirms four retained
`test<48>` calls, the mutable proxy setup and no new writer call. This is source
shape evidence, not an exact Mac preservation verdict for that function.

Evidence, native disassemblies, baseline/final logs and comparison snapshots
remain in ignored `build/packed-writer-audit/`. No full build or mock behavior
tests were run. The coordinator owns score banking and integration; previous
MAX/HIST evidence is untouched in this worker commit.
