# Boolean contracts

This review follows the H1/H2 Boolean cleanup while retaining H3's native type
evidence. H1 commits `0d6fd211` and `3f404239` and H2 commits `fd66871fe` and
`86ed7e76a` distinguish Boolean values from general integers and preserve storage
widths. Their typedef-based cleanup is not proof that an H3 interface originally
used an integer alias: H3 has original decorated Dreamcast public symbols.

## File and CFindFile

The original public names in the pinned Dreamcast `H3.EXE` encode a `bool`
result with `_N`. The CodeView primitive `0x20` printed as unsigned char is a
lowered representation here. Restore `bool` and the original method spelling
together; generated Windows names are not independent type evidence.

| Owner | Original Boolean methods | Independent public evidence |
| --- | --- | --- |
| `File` | `Exists`, `Delete` | `?Exists@File@@SA_NPBD@Z`, `?Delete@File@@SA_NPBD@Z` |
| `File` | `Close`, `IsOpen` | `?Close@File@@UAA_NXZ`, `?IsOpen@File@@UAA_NXZ` |
| `File` | `Open` | `?Open@File@@UAA_NPBDW4FileMode@@@Z` |
| `File` | `Rename` | `?Rename@File@@QAA_NPAD0@Z` |
| `File` | `SetAttribute` | `?SetAttribute@File@@QAA_NPADW4FileAttribute@@@Z` |
| `CFindFile` | `FindFile` | `?FindFile@CFindFile@@QAA_NPBD@Z` |
| `CFindFile` | `FindNext`, `IsReadOnly`, `IsSystem`, `IsNormal`, `IsDots`, `IsDirectory`, `IsHidden`, `IsArchived` | Each corresponding public has suffix `@CFindFile@@QAA_NXZ` |

The seven `File` and nine `CFindFile` declarations change from unsigned char
to bool. The five authored `File` definitions use the same result type, with
`true`/`false` for their Boolean returns. `Delete`'s local receives the comparison
`DeleteFileA(filename) != 0`, so it is Boolean too. `Exists` stays inline at its
original source position before its callers. The only changed outside call
sites are `File` operations in `game.cpp`'s map transfer routines.

The existing retail evidence pairs four emitted predicates at `0x005ffb60`,
`0x005ffb70`, `0x005ffb90`, and `0x005ffbc0`; `Exists` expands into the last two.
`Rename`, `SetAttribute`, and the entire `CFindFile` method family remain
declaration-only because their retail bodies have not been identified. Restoring
their proven interfaces does not claim that Windows emits those methods.

## Boundaries and remaining work

- `File::m_open` stays an unsigned byte. Its initialization to zero and its
  CodeView byte type do not independently prove original Boolean storage.
- Win32 `BOOL` APIs retain their platform declarations. Their nonzero results
  are explicitly compared with zero when converted to a game Boolean result.
- Other byte-returning game interfaces need their own decorated public and
  retail/caller review. A name such as `isWinner` is only a lead.
- The other normalized `File` method and field names are outside this predicate
  review; changing a proven original name requires refreshing the affected
  compared symbols and consumers together.

## Verification

The starting ledger records all 14 emitted `winfile` functions at MAX 100%.
The SSD worktree compiled the owning units and refreshed their retail target
labels through its initialization delink. The final command
`homm3 build --fast winfile game multiplayerwindow` passed with no MAX changes:

| Target | Before MAX | After CUR / MAX |
| --- | ---: | ---: |
| All 14 emitted `winfile` functions | 100% each | 100% / 100% each |
| `game::transmitSaveGame`, `0x004cafd0` | 97.4264% | 97.4264% / 97.4264% |
| `game::receiveSaveGame`, `0x004cbd40` | 100% | 100% / 100% |

The renamed predicates now have `_N` result types in their generated VC6 names;
those generated names record the adopted declarations and are not the native
evidence that justified them. `multiplayerwindow` also compiles with the shared
header and reports no MAX change. There is no Mac function claim on the File
bodies, and this cleanup did not use a Mac comparison to infer their types.
The existing non-exact transmitSaveGame residual is unchanged.
`homm3 status update --write-readme` migrated the four emitted predicate
symbols, reported zero MAX resets, and preserved their 100% HIST values; the
README score block remained unchanged.
