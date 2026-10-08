# Recovering helpers from cross-platform size differences

The September 2026 review screened functions with a largest recorded extent
of at least 256 bytes and a largest/smallest ratio of at least four. It covered
134 Dreamcast-connected entries and 13 additional Windows/Mac entries. These
147 entries are leads for source recovery; this threshold does not clear
smaller differences or functions without a paired address.

## Recovered call

`CDPlayHeroes::sysMsgDestroyPlayerOrGroup` at Windows `0x552920` now calls
`handlePlayerDrop(message->m_dpId)` instead of repeating its logging, message
construction and queue operation. Dreamcast `0x11bb4a` calls the named
`HandlePlayerDrop` at `0x11c1f8`. Mac retains the helper at `0x21100c`.
Windows expands it: the log at `0x552946` and message construction at
`0x55294d..0x55296c` perform the same operation as the standalone helper at
`0x553580`.

Keep the ordinary helper in `remote.cpp` and its source call. Fresh VC6
comparisons before and after gave 534 bytes and 100% for the dispatcher,
496 bytes and 100% for the helper, and unchanged sizes/scores for every other
function in `remote`. This recovers source structure without adding a new
exact Windows match. The dispatcher's DC source audit has no findings or
coverage gaps.

## Corrected identities

Mac `0x46c1c` is a `deque<int>` copy constructor, not the implicit whole-army
copy constructor at Windows `0x437a00`. In Mac `getBacklashValue`, instructions
at `0x4359c` and `0x435a8` pass source/destination plus `0x420`; the call at
`0x435ac` copies `army::m_spellInfluenceQueue`. The surrounding whole-army
copy expands in that caller. Retain the natural implicit C++ copy operation.

Mac `0x118134` is owned by the existing `MAC_COMPGEN_ADDRESS` for
`type_dialog_icon`'s implicit copy constructor. The duplicate library mapping
is removed. Generated call binding now resolves both copy assignment and
copy construction from their source claims, checking the exact owner and
const-reference parameter ABI. A call binding does not establish a scored
body or an exact match.

## Other leads

The review accounted for 71 apparent missing direct Mac game-call anchors and
90 named DC helper-call leads. Besides the recovered player-drop call, the
reviewed operations are already represented through nested helpers, implicit
member construction/destruction, or corresponding desktop interfaces.
The 23 unclaimed direct Mac targets inspected were container/bitset operations
and a Smacker API routine. Unknown template spellings and the codec routine's
unverified complete extent do not establish missing game helpers.

`InitializeArtifactTraitsTable` already calls the canonical
`initializeArtifactTraits`. DC retains the helper separately at `0x50058`;
Windows and Mac expand it in the table initializer. A missing standalone
desktop address is not evidence that its source body is missing.

Recorded DC extents can include trailing literal pools: for example,
`DeletePlayerFromGroup` has a 288-byte record but only 38 bytes of instructions,
and `HighlightLocators` has a 64-byte record with a four-byte return body.
Keep the symbol-proven `DC_ADDRESS` extents; do not treat every size ratio as
an optimization or missing-helper difference. The broader raw source audit
still has type/order findings and coverage gaps; this helper review does not
claim to resolve them all.
