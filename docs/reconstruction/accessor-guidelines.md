# Accessors and member visibility

This investigation starts at `faf64f874252f144a7a54b3f64503c5d30b24703`.
Accessors are source-model evidence: they identify operations the original
programmers chose to name, the state those operations expose, and the boundary
between an owner and its callers. Recovering that boundary matters even when
VC6 produces the same instructions for a direct member expression.

## Native cases

The following access levels come from the pinned Dreamcast NB11 type stream,
not from generated Windows names or the presence of a load instruction.

| Class / CodeView type | Accessor evidence | Backing storage | Consequence |
| --- | --- | --- | --- |
| `resource`, `0x1037` | Public `get_Name`, `get_resType`, `GetReferenceCount` | `Name`, `resType`, `ReferenceCount` are private | Preserve the small public interface and private storage. `addRef` / `release` own mutation of the count. |
| `Bitmap16Bit`, `0x103a` | Public `GetWidth`, `GetHeight`, `GetPitch`, two `GetMap` overloads; DC bodies at `0x01f100`, `0x01f10c`, `0x01f118`, `0x01f124`, `0x04ca7c` | Dimensions, pitch and map pointer are private | The map accessor owns address calculation and preserves const / mutable overloads. Its implementation also uses the pitch accessor. |
| `widget`, `0x170f` | Public `get_help_text`, `get_rclick_text`, `set_help_text`; header bodies at `0x06a3e8`, `0x05abe4` | `RollOver`, `RightClick`, `freeText` are protected | Derived widgets may implement the owner interface; unrelated callers use the methods. The right-click getter includes a fallback, so a direct field read is not equivalent. |
| `widget`, `0x170f` | Public virtual `GetRealWidth` / `GetRealHeight` | `width` / `height` are explicitly public | An accessor does not prove private storage. These virtual queries can differ from the stored widget rectangle. Preserve the proven public exception and distinguish stored dimensions from rendered dimensions. |
| `CSprite`, `0x17d3` | Public dimension, frame and palette accessors | Width, height, sequence array/count/mask are private; palette pointers `p` / `p24` are public | One class can combine encapsulated state with public borrowed resources. Do not infer one access level for the entire class. |
| `TPalette16`, `0x1083` | Public static `SetPixelFormat` | RGB masks are private; palette arrays are public | A setter can own several related fields. Static data has an access boundary too. |

The existing source annotations give the Windows and Mac corroboration for
these helpers. The current report does not turn a Dreamcast offset into a
Windows address claim. For example, `widget::getRealWidth` has an independently
claimed retail body at `0x004021e0`, while an inlined bitmap getter need not have
an independently retained Windows body.

## General code guidelines

1. Keep one canonical accessor and use it for its operation throughout the
   project, including callers that are already byte-exact. An outer helper may
   provide the operation through the canonical accessor; preserve that path.
2. Treat an established accessor as evidence for a non-public backing field
   when original visibility is unknown. Prefer private storage. Use protected
   storage where a derived implementation needs direct access. This default is
   a reconstruction inference, not a recovered CodeView fact.
3. Positive native public/private/protected declarations take precedence over
   the default. Preserve documented public fields such as widget dimensions;
   do not replace a raw width with a virtual rendered-width query merely because
   their names look related.
4. Apply the rule to the state actually accessed. A pointer traversed on the way
   to a property is not automatically that property's backing field. Inspect
   indexed access, packed flags, delegated helpers and computed properties.
5. Add missing accessors as caller review exposes the need. Preserve known
   original semantic names, types, qualifiers and owning class. Use a clear
   lowerCamelCase project name when the original name is unknown and label the
   inference in the owning source. Do not present a new convenience method as
   independently recovered original source.
6. Keep mutation with the owner. Reuse a setter only when its semantics match:
   `setText`, palette replacement, reference counting and status messages can
   do more than assignment. Do not bypass their invariants or introduce their
   side effects into a previously different operation.
7. Preserve constness, value/reference/pointer returns, indexed interfaces,
   output parameters and lifetime boundaries. Add a mutable reference API only
   where the operation needs a borrowed mutable object, not to disguise public
   scalar storage or evade a missing setter.
8. Preserve data order, representation, packing, inheritance and virtual slots.
   Insert access labels in place. Do not move fields to group private storage,
   add padding, or use friendship/casts/macros to bypass the interface.
9. Same-class implementation code may access its own storage. Derived code may
   access protected storage. Unrelated callers use the established operation.
   An accessor is not a reason to rewrite its own implementation recursively.
10. Recover helper placement and explicit inline status from source evidence and
    cross-TU expansion. An absent retained body does not prove an `inline`
    keyword. Keep ordinary definitions and natural call sites.
11. Validate affected Windows TUs with VC6 and retain the repository's Mac
    comparisons. If an interface changes a compared symbol, refresh the owning
    TU's delink targets. Track score changes honestly; do not flatten recovered
    calls or weaken visibility to recover a percentage.

## Counting and scope

The starting body review identifies **723 implemented accessor definitions**
(**568 getters and 155 setters**) across project-owned `include/` and `src/`.
This is the reviewed accessor set, not a count of every function with a `get`
or `set` prefix. It includes indexed reads, state predicates, computed property
views, delegated property access, const overloads and setters that maintain a
small related state group. Constructors, destructor bodies, declarations without
bodies, disabled carcasses, vendor code, allocation/search/AI algorithms and
unrelated commands are excluded. Overloads count separately; repeated header
appearances across translation units count once.

The inventory combines a lexical body review with Clang declaration identity
under each TU's configured Windows profile. Clang is an analysis aid, not the
matching compiler. All 139 project TUs were visited. Existing Clang diagnostics
in 24 TUs concern vendor inline assembly, legacy `min` overloads and temporary
reference binding; they must not be mistaken for a VC6 build verdict or a claim
that every expression was successfully typed. The source-wide compiler pass is
responsible for checking edited callers.

The body review associates these methods with 458 owning data members. After
correlating original names, 140 already-public members have positive native
public evidence; 131 already match native private/protected declarations.
Twenty-four public members have positive non-public evidence (15 private,
nine protected), and 137 have unresolved original visibility. The remaining
26 were already non-public and retain their existing boundaries.

The implementation tightens all **161** public backing members in the latter
two groups: **144 private and 17 protected**. All project callers were searched,
including template specializations and already-exact functions. Private access
is enforced by the compiler; owning implementations and protected derived
implementations retain direct access. This does not make every field of every
class private: native public exceptions and data with no established property
interface remain outside this inference.

The pass adds **115 method definitions** for missing reads, writes, indexed
borrowing and owner operations. Examples include coordinate setters, indexed
campaign/scenario and network-player access, packed map-cell flag operations,
reward payload setters, and the mouse owner's wait operation. Existing helpers
remain canonical. In particular, game-position lookup is not substituted for
network-slot indexing, a rendered widget width is not substituted for stored
width, and the multi-field movement-cost/predecessor operation is not replaced
by a scalar setter.

### Verification and limits

The declaration/body inventory visits all 139 project TUs. The implementation
has no runtime behavior fixtures: VC6 comparisons and native evidence are the
project's verdict. The affected-header closure selects 111 Windows TUs for
`homm3 build --fast`, with their paired full-TU CodeWarrior comparisons. This is
a source-interface recovery, so any observed byte-score changes are reported
and retain previous peaks in the score history.

Temporary extraction and review artifacts live in ignored
`build/accessor-audit/`. The owning C++ declarations remain the authority for
names and access; this report is a dated investigation, not a second symbol
ledger.
