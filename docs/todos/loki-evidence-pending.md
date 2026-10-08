# Loki evidence not yet taken into the game

The Loki Linux map editor `h3maped` (RoE, g++ 2.95, `-O0`, asserts kept) and
Dreamcast CodeView show original names, signatures, helpers and body shapes
for the shared engine classes. The `pr/loki-engine` branch took the ones that
VC6 SP3 holds or improves. This page lists every Loki-derived item it did not
take, so each can be retried once VC6's name-keyed state
([vc6-name-keyed-state.md](vc6-name-keyed-state.md)) is better understood.

The source notes are `loki-flowback-a.md` to `-d.md` (lanes A to D) beside the
checkouts. Scores are CUR/MAX from the full build of `pr/loki-engine`. A
"fast" score comes from a per-unit `homm3 build --fast` after a standalone
`homm3 delink`; that mode reads slightly lower than a full build for some
functions, so compare a fast probe only against another fast probe.

Each item gives the evidence, the reason it wasn't taken, and what would
likely unblock it.

## Tried and dropped: a score fell

- **`InitializeArtifactTraitsTable` (0x44cd50), 89.40.**
  - *Loki:* RoE assigns the loader's buffer through `operator=` from a
    temporary: `static TAutoArrayPtr<char> aNameBuffer(0); aNameBuffer =
    TAutoArrayPtr<char>(new char[size]);`. This is the
    `InitializeAdvObjectTypeTraitsTable` shape.
  - *Why not:* doing that for both artifact buffers drops it
    89.40 → 68.96 (fast). Complete constructs both statics directly from
    `new char[...]`.
  - *Unblock:* none expected. It is an era difference in this loader.
- **`InitializeArtifactTraitsTable`, 89.40.**
  - *Loki:* `int row = 0; int id = 0; row += 2; for (i = 0; i < N; i++) {
    ...; id++; row++; }`.
  - *Why not:* 89.40 → 85.02 (fast).
  - *Unblock:* Complete's single-buffer loader with its combination pass
    has no RoE counterpart. Retry only together with a recovered shape for
    Complete's two passes.
- **`initializeArtifactTraits`, the static row helper.**
  - *Loki:* spelled `InitializeArtifactTraits`, with asserts
    `id >= 0 && id < kNumArtifacts`.
  - *Why not:* renaming only the helper moves
    `InitializeArtifactTraitsTable` 89.40 → 89.36 (fast). This is
    name-keyed state: the helper is expanded inline, so the call itself
    does not change.
  - *Unblock:* the missing artifact.cpp statics or a located name-keyed
    mechanism, as the EncodeGeneral statics did for cspriteframe.
- **artifact.cpp has no missing `.bss` statics (2026-10-08).** Retail's
  artifact `.bss` runs from 0x693898 to 0x694ca0 with no unclaimed byte.
  - *Retail order:* `g_artifactSlotMasks` (0x693898), the
    `artifactStrings` guard (0x6938d4), `g_combinationArtifactTable`,
    the traits storage (0x6939f8), the slot-traits storage (0x694bf8),
    then `artifactStrings` (0x694c90) and `artifactSlotStrings`
    (0x694c98).
  - *Ours:* the same 0x1408 bytes and the same 8-byte alignment classes,
    but VC6's name-hashed order differs: `artifactStrings` first, then
    masks, guard, combination table, `artifactSlotStrings`, traits and slot
    traits.
  - *So:* the cspriteframe lever does not apply. No unreferenced hole
    exists, and no object changes its alignment class.
  - *Name probe:* Loki's `aArtifactTraitsImp`/`aArtifactSlotTraitsImp` for
    the two storage arrays still leaves a non-retail order, and gives 89.31
    (from 89.40).
  - *Also:* the 89.40 ↔ 89.36 movements follow the declaration offset
    (period 64, docs/vc6/handle-period.md), not `.bss` layout. On one tree
    the row also flipped between full rebuilds with byte-identical
    candidate and delinked target objects.
  - *Real residual:* inlining. Retail expands bitset<19>'s `_Tidy`,
    `reference::operator=` and the first `operator==`, which we call, and
    calls `assign` where we copy-construct. That is source budget, not
    `.bss`.
- **`ResourceManager::RedMask`…`BlueBits` declarations.**
  - *Loki:* exports nine `ResourceManager::` objects.
  - *Why not:* declaring the nine in `resourcemanager.h` (what the
    cleanliness ratchet asks for) costs three functions:
    - `advManager::doCombat` 95.53 → 95.36;
    - `CEnterNameEdit::onKillFocus` 100 → 99.87;
    - `InitializeArtifactTraitsTable` 89.40 → 89.36.

    So the branch defines them inside `namespace ResourceManager` in
    resourcemanager.cpp, with no extern declarations.
  - *Unblock:* the name-keyed mechanism. The header spelling changes other
    units' bytes, not resourcemanager's.
- **`TObjectType::setImageName` (0x514610), 99.21.**
  - *Loki:* `maskName.append(".msk")` in place of `+=`.
  - *Why not:* setImageName itself does not move, but the inlined
    `basic_istream` constructor (0x5151b0) falls 100 → 99.91 (fast).
  - *Unblock:* the `.rdata`/inline order in objecttype.cpp. The other two
    candidates (copy-init `maskName`, `find_last_of`) were taken.

- **`TAutoArrayPtr<T>::operator=`** (inline; Dreamcast 0x05b228).
  - *Evidence:* Dreamcast AutoArrayPtr.h:54..67 and Loki both guard with
    `if (this != &rhs)` and end with `m_ptr = rhs.release();` (Dreamcast
    calls `release` at line 65). The game has neither.
  - *Why not:* in a full build, `InitializeArtifactTraitsTable` falls
    89.40 → 89.36, although no artifact.cpp code assigns a
    `TAutoArrayPtr`. This is name-keyed state.
  - *Unblock:* the missing artifact.cpp statics or a located mechanism.
    Dreamcast proves the body, so retake it then.
- **`TResourcePtr<T>::operator=` and `release()`.**
  - *Evidence:* Loki's GUIGameObject.o keeps them as linkonce members.
    Dreamcast's ResourcePtr.h leaves lines 45..67 free between the
    destructor (44) and `get` (68).
  - *Why not:* Complete never instantiates either, but adding the two
    unused template members moves `CEnterNameEdit::onKillFocus`
    100 → 99.87 in a full build. This is name-keyed state.
  - *Unblock:* same as `TAutoArrayPtr`.

## Era differences (RoE versus Complete)

- **`TAllocationFailure::TAllocationFailure` (0x4d6b80), 100.** Loki's form
  is `TAllocationFailure(const char* file, unsigned line)`. The Complete
  constructor takes no arguments.
- **`TRuntimeError` constructors.** Loki's `Error.cpp` has
  `(const char*)`, `(const string&)` and the file/line forms built by
  `formatDebugMessage`. Complete retains only `(const char*)` at 0x49a0c0,
  in the dxplay band. The file name `Error.cpp` is not proven for
  Complete's layout.
- **`LODFile::clear` (0x4fa590), 100.** RoE does not free `dataBuffer` in
  clear; Complete does.
- **`ResourceManager::getBitmapResourceSize` (0x55d070), 96.77.** RoE reads
  one bitmap LOD; Complete walks the archive list. Only the final
  `->size` read is comparable.
- **`CSequence::~CSequence`, 100.** RoE deletes each frame; Complete frees
  only the array, because frames are cache-owned.
- **The `TTimedEvent` binary form.** RoE has 8 player bits, 16-bit
  first/interval and 16 reserved bytes. Its accessors are
  `getBApplyToPlayer`, `getFirstOccurence` and so on.
  - *Not compared:* Complete's `Read`/`Save`/`Load` against these.
  - *Unblock:* compare the field widths first.
- **`TObjectType` image info.** RoE keeps it outside the record and reads
  masks through `PointToSpriteResource(a) || PointToSpriteResource(b)`
  and `ReadFromSpriteResource`. Complete keeps a 0x4c inline record and
  passes the `LODFile*`. `push_back(_TImageInfo())` versus Complete's
  size constructor: not tried.
- **`kMaxObjWidth`/`kMaxObjHeight`.** Loki builds the no-trigger location
  from these constants. Complete reads a `.rdata` `TPoint` at 0x640278
  (`g_noTriggerCell`). The constants could still name the 8 and 6 in
  setImageName's `bit < 48` and `[6]` arrays: not tried.
- **`initializeSpellTraits`.** RoE's `SSpellTraits` is 132 bytes with
  `m_townProbability[8]`. That is not Complete's record.

## Port-only differences (never for the game)

- Loki's libio `streambuf` in GzBuf.
- 4096-byte paths.
- One-byte `BOOL`.
- GTK code.
- `convert555to565`.
- The destructor-only resource vtable.
- The `this == NULL` + `g_warning` guard in `TTextResource::GetText`.

## Not tried

Unless noted, each target is exact (100) in the game, so these are fidelity
items.

### TAutoArrayPtr and TResourcePtr

- Loki's `throw()` specifications on their constructors, `get`, `release`,
  `operator=` and `operator->`. Measure VC6's unwind tables first.

### TGzInflateBuf

- **Flag names.** Loki's `_m_bGzip`, `_m_bSrcEOF` and `_m_bInflating` do
  not appear in assert text, so they are unproven. The `bool` type was
  taken.
- **Constructor (0x4d6050), 98.92.** Two Loki candidates: a named
  `int result = inflateInit2(...)` local, and
  `assert(_m_pSrcBuf != NULL)` after the magic check (release-elided).
- **`TDataError`.** Loki defines it inline in the class body.
- **`g_gzMagic` (0x63e6fc).** Loki keeps zlib's `gz_magic`. This is a data
  rename, so it carries name-keyed risk.

### Text resources

- **`TTextResource` constructor (0x5bbba0).** Loki's `dd` cursor, the
  `count` prologue, the shared `end` local and the quote-collapse loop.
- **`TSpreadsheetResource` constructor (0x5bbe70).** Loki's column-count
  loop and its `*dd++ = 0` terminators.
- **`TTextResource::GetText`.** Loki has it out of line, with
  `assert((r >= 0) && (r < Text.size()))`, and `operator[]` calls it.
  Complete expands it.

### LODFile

- **`LODFile::getErrorString`** (Dreamcast only): Loki's `break;` after each
  `return`.

### ResourceManager (lane A)

- **`ResourceManager::GetSprite` (0x55c7b0), 88.94.** Loki's `-O0` body
  differs widely:
  - `char (*)[13]` frame-name rows;
  - `sequences[i]` indexing;
  - a NULL-initialized frame from `GetFromCache`;
  - RoE's `croppedHeader.m_encoding` bug;
  - an int `AddFrame` result.

  Worth a dedicated session.
- **`TCacheMapKey` (0x55ac20).** Loki uses `char Name[13]`,
  `strncpy(Name, name, 12); Name[12] = 0;` and an in-class `operator<`.
- **`AddToCache`/`GetFromCache`.** Loki's names, and `AddToCache`'s
  assert on `*r->get_Name() != 0`.
- **The `GetText`/`GetSpreadsheet` reports.** Loki uses the copy-pasted
  "GetResource" label and an `m_attrib != RESOURCE_TYPE_TEXT` check.
- **`ResourceManager::Dispose(CSprite*)`.** RoE's sequence walk; Complete
  uses the virtual `CSprite::dispose`.

### Pixel classes (lane B)

- **Project-inferred helpers that Loki shows are not original.**
  `TPalette16::convert24to16WithMasks`, `selectHSVChannels`,
  `adjustPaletteHue` and `adjustPaletteComponent`. Loki spells each body
  inline, so removing them changes the structure of exact functions.
- **Palette shapes.**
  - `(red_mask << 1) & ~red_mask` scales;
  - `/ 256` channel division;
  - `p16` destinations;
  - `Cycle`'s countdown loops and `const` saved locals;
  - the `h += amount * delta` operand order;
  - `RGBToHSV`'s `const float delta` and literal `+ 0.0f`.
- **Copy constructor and assignment by `const TPalette16&`/`TPalette24&`
  (Loki).** Era check pending.
- **Bitmap shapes.**
  - `sw -= -dx` clips with early returns;
  - `if (!w || !h) return;`;
  - `Colorize`'s top-of-function locals;
  - `Remap`'s named intermediate;
  - `Bitmap816` raw-draw source cursor and palette local;
  - `import`'s `TPalette16(p16)` temporary.
- **Font shapes.**
  - `DrawCharacter`'s `dst += 2 * abcA` second statement;
  - `DrawStringExecute`'s loop condition and local order;
  - `DrawBoundedString`'s first-use locals;
  - `GetCharacterWidth`'s `const myABC&`;
  - `LineLength`/`LineWidth`/`longest_word_length` loop shapes.

### CSpriteFrame (lane C)

- The `for (;;)` skip and draw loops with `runLength`.
- `div2`-first shadow terms.
- `const T* const` locals bound after the size return.
- The `switch` flag test in `DrawAdvObjWithFlagAlpha`.
- `drawTile`'s Duff-style `switch ... do`.

All 14 cspriteframe functions are exact on this branch. Retry these against
the now-exact bytes.

### Traits tables and ObjectType (lane D)

- **Local-static names `aNameAutoStrs`/`aDescAutoStrs`.** From Dreamcast
  and Loki. The loaders' `int i; int row = 0; int id = 0; row += 2;`
  counters, and `T& traits = a…Imp[id]` bindings.
- **`InitializeHeroClassTraits`.** Its `for (i = 0; i < 4; i++)` field
  loops.
- **`TSpellTraits::m_flags`.** `unsigned long` in Dreamcast `T_ULONG`
  (game `unsigned int`).
- **The object-traits loader.** `objnames`' function-local `nameBuffer`
  becomes Loki's `aNameBuffer`, and its text-resource local becomes
  `pTextResource`.
- **`TUniqueSet<T>` (UniqueSet.h).** `add`/`numItems`/`get` for the
  provisional `TObjectImageNameTable`, and `TUniqueSet<TObjectType>` in
  the GameMap code.
- **`TObjectType::_setTriggerMask`.** Loki's `goto found` scan with
  `_getBitPos(x, y)`, and the `setBCell*` setters.
- **`TObjectTypeTable::load`.** Loki's `TRuntimeError` message and its
  `try { ... } catch (...) { Dispose; throw; }`.
- **`less<TObjectType>`.** Loki's explicit specialization.
