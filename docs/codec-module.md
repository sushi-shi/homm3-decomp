# Codec module

The `codec` module is tracked by semantic input/output agreement. A 100% VC6
byte match is useful evidence, but is not its completion criterion. Module
membership is declared on units in `config/units.toml`; the README groups their
existing scores without changing the executable denominator or score history.

The game-owned units are `cspriteframe` (DEF decoding integrated with blitting),
`lodfile` (LOD directory and member access), `gzfile` (gzip file I/O) and
`gzinflatebuf` (stream inflation). The existing translation-unit boundary is
retained; classifying `cspriteframe` does not move general bitmap rendering into
this module. Victor and zlib remain separate vendor-library modules. Audio/video
playback managers stay in `game`; their actual codecs reside in external DLLs.

Validation compares an implementation with the pinned retail decoder or encoder
at an explicit boundary. Record the input corpus and parameters, return codes,
metadata, decoded pixels/palettes or uncompressed bytes, and state changes that
are visible to callers. Require repeatability where outputs depend on allocation
or process state. For drawing decoders, clipping, pitch, orientation, palette
selection and transparency are inputs too. Encoder equivalence may mean an
identical decoded result rather than identical compressed bytes; document which
contract is required by the caller and checked by the oracle.

Run the complete installed corpus and keep every archive occurrence in the
coverage report. Supplement missing encoding modes and boundary conditions with
explicit generated cases. A finite corpus does not prove equivalence for every
possible input: record unsupported cases and malformed-input/error coverage.
Rust parser acceptance or a round trip alone is not retail differential evidence.

Existing independent implementations include `homm3-lod`, `homm3-archive`,
`homm3-def` and `homm3-resource` under `tools/`. The
[resource-format matrix](resource-format-matrix.md) distinguishes parser/corpus
coverage from comparison with reconstructed C++ and retail. The
[Victor oracle](victor-oracle.md) executes the recovered VC6 library beside
retail with full installed-bitmap coverage after explicit lossless PCX encoding.
Rust is a suitable implementation language at an independently validated codec
boundary; matching VC6 instruction selection is not a reason to duplicate a
working, validated implementation.

Keep the existing build, ABI, source-ownership and score-accounting gates. This
classification does not mark an untested implementation complete, remove its
bytes from the denominator, or alter its measured scores. Vendor sources remain
pristine. Game logic outside the codec boundary retains its normal matching
workflow.

Rendering and playback managers are not currently a separate semantic-completion
module. Matching decoded media alone would not cover presentation timing, frame
skips, stream loops, pause/resume, mixer controls, device interaction or resource
lifetimes. Moving those managers to such a module requires an oracle for their
observable behavior, not merely for the vendor decoder's output. Renderer checks
must likewise cover drawing inputs and framebuffer output, not just asset decoding.
