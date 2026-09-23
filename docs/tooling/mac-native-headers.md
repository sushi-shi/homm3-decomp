# Ordinary headers for Mac source comparisons

Windows is the product. Mac is evidence for reconstructing Windows functions,
including helpers VC6 expanded into callers. Compile the same game declarations
and header bodies with CodeWarrior; use its real MSL library. Do not maintain
parallel game classes, copied standard-library definitions, or marked/extracted
method fragments. This does not require a runnable Mac port.

## Native library inputs

```sh
homm3 mac sdk /path/to/extracted/codewarrior-pro6
homm3 mac sdk                       # verify the staged headers
```

`HOMM3_MAC_SDK` can supply the extracted archive root. Paths, file counts and
tree hashes are pinned in `config/mac/sdk.toml`. The four unmodified header
trees are staged under ignored `build/mac/sdk/`; no SDK header is committed.
Both byte contents and relative names participate in each tree hash, preserving
the original encodings and line endings. Sources are all verified before any
staged tree is replaced. No sibling installation is discovered implicitly.

## Unit profiles

An ordinary-header profile uses the existing paired-body compiler mode:

```toml
mode = "paired_bodies"
native_headers = true
preamble = "include/hero.h"
extra_headers = [] # Other ordinary project headers needed by selected bodies.
include_dirs = ["include"]
flags = ["-O1", "-proc", "750", "-nolink", "-char", "unsigned"]
helpers = []
source_helpers = []
```

The example describes configuration, not a claim that all hero bodies currently
compile with these includes. Source-owned functions and data are still selected
in original source order. Header methods are read directly by the compiler,
including helpers that were absent from the former minimal declaration views.
They require no per-method manifest or extraction marker.

`include/compiler.h` supplies compiler spelling compatibility. Native profiles
also enable CodeWarrior's documented `-msext on` to parse Microsoft anonymous
structs. These settings do not turn an ordinary helper into an inline helper.
The SDK include paths are added automatically after project include paths.

The compiler performs preprocessing. Tooling snapshots and fingerprints the
entire declared project-header trees and the verified SDK, so conditional and
macro includes do not need a Python approximation of preprocessing. This is
conservative: an unrelated header edit can invalidate a native comparison.
Staged inputs are checked before cached output is reused and compilation inputs
are checked again after compilation. Missing SDKs, compiler errors and unbound
symbols remain unavailable comparisons, never exact results.

## Migration status

Native input staging and the profile/compiler path are implemented. Existing
unit profiles have not yet switched: removing their declaration views and the
legacy marker extractor depends on reviewing the ordinary-header blockers.
Do not interpret the earlier declaration-view scores as validation of the new
header path.

The original MSL headers compile with the pinned compiler. An object-data probe
measured `sizeof(std::string) == 4`, `sizeof(std::vector<int>) == 12`, and
`sizeof(std::bitset<48>) == 8`. These sizes are supplied by the library; game
classes do not need Mac substitutes for those members.

A compiler probe using the new input snapshot compiled the unchanged
`include/recruit.h` after its existing creature-type prerequisite header.
Another object-data probe confirmed that `#pragma pack(push, 1)` gives a
`char`/`int` record size of 5 and `#pragma pack(pop)` restores its size to 8.
These pragmas need no target-specific replacement. No test suites or full
matching checkpoint were run for this migration step.

With compiler spelling compatibility and Microsoft extensions enabled,
`include/hero.h` reaches five errors in `include/mapcell.h`: integer-backed
fields returned implicitly as `TCreatureType`, `ArtifactPrices`, or
`EGameResource`. Explicit casts in those five getters have been proposed for
user review. No game-class edits have been made in this migration yet.

The proposed returns are:

| Getter | Proposed expression |
| --- | --- |
| `getArtifactDefender` | `static_cast<TCreatureType>(m_artifactInfo.m_guard)` |
| `getArtifactPrice` | `static_cast<ArtifactPrices>(m_artifactInfo.m_price)` |
| `getGardenResource` | `static_cast<EGameResource>(m_gardenInfo.m_resource)` |
| `getWagonResource` | `static_cast<EGameResource>(m_wagonInfo.m_resource)` |
| `getWindmillResource` | `static_cast<EGameResource>(m_windmillInfo.m_resource)` |

Further layout, packing and OS dependencies must be examined through the real
headers. Reuse existing declarations/helpers where possible; show game-class
changes before applying them. Keep the approved ballista multiply/divide
conditional. Byte-order work must preserve the observed file format and source
helper boundaries, rather than inventing a generic new serialization layer.
