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
tree hashes are pinned in `config/mac/sdk.toml`. The four unmodified CodeWarrior
header trees and Windows SDK declarations from the existing pinned VC6 toolchain
are staged under ignored `build/mac/sdk/`; no SDK header is committed.
Both byte contents and relative names participate in each tree hash, preserving
the original encodings and line endings. Sources are all verified before any
staged tree is replaced. No sibling installation is discovered implicitly.

## Unit profiles

An ordinary-header profile uses the existing paired-body compiler mode:

```toml
mode = "paired_bodies"
flags = ["-O1", "-proc", "750", "-nolink", "-char", "unsigned"]
helpers = []
source_helpers = []
```

Candidate input starts from the original source file. The existing Clang source
inventory identifies definition spans using the project's Windows declaration
profile. The generator preserves includes, globals, constants, namespaces,
source-local classes, prototypes and source order. Selected function bodies,
source helpers, templates and authored inline bodies remain present. Other
free-function bodies become declarations using their own written declarators;
out-of-line class methods already declared in headers are omitted. CodeWarrior
then preprocesses the candidate and reads the ordinary game headers itself.
A source-inventory error stops preparation; it cannot yield an empty candidate.

Project include paths come from `config/units.toml`. There is no separate Mac
header roster or game-class definition. Header methods require no extraction
markers, location lists or copied bodies. `mac_symbol` in a pair is the expected
**candidate** linkage name, not a claim that the stripped executable retains
that name. Native MSL vector specializations include their allocator argument.

`include/compiler.h` supplies compiler spelling compatibility. Native profiles
also enable CodeWarrior's documented `-msext on` to parse Microsoft anonymous
structs and define `HOMM3_TARGET_MAC=1` on the compiler command line. These
settings do not turn an ordinary helper into an inline helper.
The SDK include paths are added automatically after project include paths.

The compiler performs preprocessing. Tooling snapshots and fingerprints the
entire declared project-header trees and the verified SDK, so conditional and
macro includes do not need a Python approximation of preprocessing. This is
conservative: an unrelated header edit can invalidate a native comparison.
Staged inputs are checked before cached output is reused and compilation inputs
are checked again after compilation. Missing SDKs, compiler errors and unbound
symbols remain unavailable comparisons, never exact results.

## Migration status

All 16 duplicate Mac declaration headers have been removed. All profiles use
ordinary project headers and native library inputs. The 73-entry body manifest,
146 marker comments, generated method fragments and fallback declaration-view
compiler path are gone. Original game helper bodies remain in their owning
headers/source. Profile loading rejects the retired header-view settings.

All 66 admitted pairs still load. All twelve admitted units now produce native
objects through ordinary headers. Under `HOMM3_TARGET_MAC`, `mapcell` uses the
existing `ExtraInfoUnion` view of `m_extraInfo` for its bitfield writes, replacing
accesses to a member supplied only by the deleted alternate declaration.
The shared game-class layout is unchanged. Compilation success does not imply
that all byte comparisons are available or exact.

The integrated build before the final Mac-only `mapcell` repair rebuilt 129
Windows units and passed the Windows/source gates: 4,304 / 4,781 exact MAX,
97.20% weighted MAX. Mac comparisons were available for 28 / 66 pairs; the
remaining 38 prevented a complete Mac checkpoint. The subsequent `mapcell`
compiler check passed, but the full comparison sweep has not been repeated.
No test suites were run for this checkpoint.

A real comparison through this path reproduced all 148 bytes of
`hero::getHighestSchool`. This control does not validate all pairs or older
scores. Native-library linkage names and references need current validation;
missing mappings remain unavailable comparisons.

`include/platform.h` isolates the remaining Windows SDK dependency. Windows
uses its usual SDK. For the Mac compiler, a scoped compatibility import reads
the genuine SDK declarations and Miles headers; it does not implement Windows
APIs on Mac or establish native Mac layouts for platform classes. MSL stays
first on the C/C++ library path. Opaque Bink and Smacker handles suffice for the
shared declarations without importing their platform-specific UI APIs.

Other fixes add missing owning includes, use native MSL for the VC6 min/max
fallback and CRT spelling differences, and keep VC6-only delete annotations in
the VC6 branch. No game fields, base classes or virtual rosters were changed.
The unsupported `giveExperience` parameter fork was removed: neither the raw
PEF nor any expanded section contains the alleged retail symbol. Both compiler
candidates now use the existing Windows/DC unsigned-char declaration.

MWLink's listing truncates large initialized data. The object reader records
these payloads as unavailable, with their declared extent, instead of filling
the missing bytes. A function requiring one of those payloads cannot receive a
byte verdict. Unrelated function comparisons can still run.

The original MSL headers compile with the pinned compiler. An object-data probe
measured `sizeof(std::string) == 4`, `sizeof(std::vector<int>) == 12`, and
`sizeof(std::bitset<48>) == 8`. These sizes are supplied by the library; game
classes do not need Mac substitutes for those members.

A compiler probe using the new input snapshot compiled the unchanged
`include/recruit.h` after its existing creature-type prerequisite header.
Another object-data probe confirmed that `#pragma pack(push, 1)` gives a
`char`/`int` record size of 5 and `#pragma pack(pop)` restores its size to 8.
These pragmas need no target-specific replacement. No test suites were run.

With compiler spelling compatibility and Microsoft extensions enabled,
`include/hero.h` now compiles with the native SDK. Its five enum-bitfield
return errors are resolved with the existing `H3_ENUM_DECODE` convention from
`codex/name-disguise-creature-bounds` (`693a49b0`). That branch already used
this macro in `getArtifactDefender`. Its `include/domains.h` was reused without
modification; the broader typed-domain migration was not imported.

CodeWarrior presents these bitfield reads as narrow integer values. The shared
macro converts them to the declared enum return type while preserving member
types and layout. The five getter returns now use:

| Getter | Expression |
| --- | --- |
| `getArtifactDefender` | `H3_ENUM_DECODE(TCreatureType, m_artifactInfo.m_guard)` |
| `getArtifactPrice` | `H3_ENUM_DECODE(ArtifactPrices, m_artifactInfo.m_price)` |
| `getGardenResource` | `H3_ENUM_DECODE(EGameResource, m_gardenInfo.m_resource)` |
| `getWagonResource` | `H3_ENUM_DECODE(EGameResource, m_wagonInfo.m_resource)` |
| `getWindmillResource` | `H3_ENUM_DECODE(EGameResource, m_windmillInfo.m_resource)` |

This verifies header compilation, not Windows byte matching or successful compilation of
all admitted hero bodies with their complete include prefix.

Further layout, packing and OS dependencies must be examined through the real
headers. Reuse existing declarations/helpers where possible; show game-class
changes before applying them. Keep the approved ballista multiply/divide
conditional. Byte-order work must preserve the observed file format and source
helper boundaries, rather than inventing a generic new serialization layer.

## Windows verification of enum decoding

A full comparison run after `95c00a90` rebuilt 78 affected VC6 units and
refreshed all retail targets. Windows retains 4,304 / 4,781 exact MAX and
97.20% weighted MAX. No exact function was lost and no MAX decreased;
`hero::giveArtifact` MAX increased from 95.3644% to 95.4534%. All 4,781
reported function sizes stayed unchanged; two already-nonexact current scores
moved. This is not a claim that every emitted instruction stayed identical.
The existing Mac profiles compared all 66 admitted functions, with 13 exact.

Banked rows, VA claims, single-view, source ownership and source inventory
passed. The full command exited unsuccessfully on the cleanliness gate because
`HOMM3_TARGET_MAC` had been defined in `include/compiler.h`. Moving that selector
to native compiler flags fixed the violation; the failed gate was rerun and
passed. This correction changes no active Windows or legacy Mac compilation
input. Native hero-header compilation was checked with the corrected flags.
The full command was not repeated after that gate-only correction.
