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

All 66 admitted pairs still load. All twelve admitted units produce native
objects through ordinary headers. `CObject` now inherits the ordinary
`ExtraInfoUnion` base, as recorded independently in Dreamcast CodeView records
0x30aa and 0x6401: base at +0, coordinates at +4/+5/+6, TypeID at +8,
frameOffset at +0xa, total size 12. Complete's readers/writers corroborate these
offsets. This replaces the reconstruction's partial union and makes the monster
fields inherited members; no new union arm or payload cast is required.
Compilation success does not imply that all byte comparisons are available or exact.

The integrated checkpoint has 4,305 / 4,781 Windows functions at exact
MAX and 28 / 66 available Mac comparisons. Missing native-library and TOC
mappings still prevent a complete Mac checkpoint. Windows/source gates, including
cleanliness, pass with no MAX losses. The full command exits unsuccessfully
because 38 Mac comparisons remain unavailable. No test suites were run.

### Source conditionals

The audit removes compiler-specific source spellings rather than treating an
instruction-order difference as proof of a platform difference:

| Owner | Shared source |
|---|---|
| `game::loadMinePool` | One unsigned byte count and the existing scalar-reader helpers; Windows stays exact. |
| `valueOfWarFactory` | One cost-pointer traversal for both compilers. |
| `philAI::getTurnAIVars` | One bonus formula; signed-byte difficulty makes the quarter arithmetic exact. |
| `NewfullMap::rebuildObjectTypeIndex` | One declaration order, comparison and unsigned-short conversion. |
| `NewfullMap::readMonsterData` | One signed-short quantity, inherited monster fields, and switch without an invented default assignment. Windows rises from 97.3333% to 98.4017%. Both Complete builds explicitly clear bits 27..30. |
| `hero::getLevel` | One ordinary helper. VC6 chooses to expand it without a compiler-specific `inline` keyword. |

This removes thirteen `HOMM3_TARGET_MAC` conditionals and one `_MSC_VER` inline
fork. The seven remaining Mac guards in source files cover four little-endian
file conversions, the two explicitly reviewed ballista multiply/divide
expressions, and the platform CRT include selection. Existing Windows resource
error reporting and compiler-specific SDK syntax are separate dependencies.
The Mac sprite retry path at code0+0x153ff0..0x154014 contains two archive
lookups and no reporter call, supporting the Windows-only diagnostic calls.
The Clang palette-temporary branch handles VC6's extension that binds a
temporary to a mutable reference; it is an editor compatibility case.
No compiler switches or false inline declarations were added to improve scores.
With shared source, the Mac comparisons for `valueOfWarFactory` and
`getTurnAIVars` fall to 51.7045% and 79.2857%, respectively; their named call
sequences still agree. Their Windows scores remain 99.4198% and 97.8723%.

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
the VC6 branch. The platform boundary does not change game fields or virtual
rosters. The source-evidenced CObject base restoration is described above.
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
