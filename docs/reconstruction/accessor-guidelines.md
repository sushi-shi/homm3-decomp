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
| `TSubWindow`, `0x1ec6`, field list `0x6efa` | Public registration and drawing methods maintain the widget range | Geometry is public; `Widgets` / `ParentWindow` protected; `FirstWidgetID` / `LastWidgetID` / `Background` private | Preserve the proven mixed data boundary. The range endpoints are internal bookkeeping, not public configuration. |

The existing source annotations give the Windows and Mac corroboration for
these helpers. The current report does not turn a Dreamcast offset into a
Windows address claim. For example, `widget::getRealWidth` has an independently
claimed retail body at `0x004021e0`, while an inlined bitmap getter need not have
an independently retained Windows body.

## General code guidelines

1. Read the owner and complete caller flows before replacing a field access.
   Search for repeated groups of operations across setup, gameplay, loading,
   teardown and temporary simulation. A list of matching member names is a
   navigation aid, not a semantic review. Keep one canonical operation and use
   it throughout the project, including callers that are already byte-exact.
   An outer helper may provide it through a nested canonical call.
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
6. Keep mutation with the owner. First look for an existing reset, transfer,
   copy or assignment operation. Introduce a missing operation for a repeated
   state transition, rather than exposing each of its fields through a setter.
   Reuse it only when its semantics match: `setText`, palette replacement,
   reference counting and status messages do more than assignment. Partial
   updates, full resets and temporary overrides need distinct contracts.
   Preserve stream-read order and narrow/packed storage conversions; never
   combine sequential reads into arguments with unspecified evaluation order.
7. Preserve constness, value/reference/pointer returns, indexed interfaces,
   output parameters and lifetime boundaries. Add a mutable reference API only
   where the operation needs a borrowed mutable object, not to disguise public
   scalar storage or evade a missing setter.
8. Preserve data order, representation, packing, inheritance and virtual slots.
   Insert access labels in place. Do not move fields to group private storage,
   add padding, or use friendship/casts/macros to bypass the interface.
   Review the surrounding class, not just one accessor's field. Repeated
   public/private hops are a lead that neighboring state was never reviewed.
   Existing public spelling is not proof of original public access. Group a
   coherent owner boundary after reading its callers and native declarations;
   internal-only state needs no new getter. Preserve genuinely evidenced mixed
   access rather than minimizing labels for appearance alone.
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
matching compiler. All 139 project TUs were visited. Baseline and final Clang diagnostics
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

The pass adds **139 method definitions** for missing reads, writes, indexed
borrowing and owner operations. Examples include coordinate setters, indexed
campaign/scenario and network-player access, packed map-cell flag operations,
reward payload setters, and the mouse owner's wait operation. Existing helpers
remain canonical. In particular, game-position lookup is not substituted for
network-slot indexing, a rendered widget width is not substituted for stored
width, and the multi-field movement-cost/predecessor operation is not replaced
by a scalar setter.

### Caller implementation

The public-storage exceptions still use canonical property operations at
unrelated call sites. Game-owned world-map and indexed hero, town, mine,
garrison and boat access goes through the existing getters. Count, append
and clear operations now live with those collections; each clear retains its
place in the loader's destruction sequence. The returned append index comes
from the same owner that inserts the object.

Hero route-target setters own the coordinate group. Full-width coordinate
getters preserve X/Y sentinel comparisons and temporary save/restore without
packing through `type_point`; `clearTarget` invalidates X/Y and preserves Z.
The existing packed `getTarget` and inherited `getLocation` views handle
map-point consumers. `setSecondarySkillLevel` changes only the mastery byte,
so AI simulation and replacement of an existing campaign skill do not acquire
`setSS`/`giveSS` slot-order side effects.

Palette-copy setters preserve resource copy construction and delete-before-copy
ownership. The loader's named palette helpers delegate to those setters, and
the existing reference overload shares the canonical pointer-copy operation.
Manager status uses its native setter and a read counterpart. Player-human
state keeps the native normalized predicate alongside a byte-preserving copy
view and setter. Combat owning-side access uses its property interface.
The existing slider-state query now has a concrete body.

These additions are inferred project interfaces, labelled at their definitions;
they do not assert new native symbol identities. Positive native-public field
declarations remain public, and owning implementations can still use their own
storage. Resource payloads, serialization records and computed properties with
different semantics are not silently replaced by unrelated getters/setters.

Per the task instruction, this implementation continuation ran no builds,
tests or validation checks. The observations below belong to commit
`069a8b3b8` and do not validate the subsequent caller/owner-operation changes.

## Complete combat-window boundaries

`TCombatWindow` exposed the limitation of changing only accessor-backed fields:
its private chat editor and control panel were separated by public chat/log
state, followed by public information-panel arrays. Direct inspection of the
native type stream finds only forward declaration `0x43dc`, with no field list
or access levels. The message vector, counters and timestamp have no external
users. The window owns both chat widgets and all panels through construction,
placement replacement and destruction. Its entire data block is now private,
in the same order; this is a project inference, not recovered private keywords.
Typed panel accessors let callers use the existing `update`, `show`, `unShow`,
`isShown` and `draw` interfaces without replacing owned pointers. Callers still
reread the current combat window at each stage, including after quick view.
Chat background restoration owns the widget rectangle and existing bitmap
draw, preserving the unguarded widget use. It remains separate from chat-content
updates and widget drawing. The public panel methods and virtual slots are
unchanged. This is a class-wide ownership review, not a reason to blanket-hide
native-public fields elsewhere.

The same review extends through the combat subwindow family. Their native type
records (`0x5708`, `0x570f`, `0x5715`, `0x54c2`, `0x571f`) are also forward-only.
Hero and creature popup widget pointers, shown state and creature display mode
form private data blocks; the control bar's two scroll-button pointers are
private too. Their existing update/display methods already serve all callers.
The rollover pointer is protected because the derived control bar allocates
and updates it. The parent combat window now queries its presence and stored
width and requests visibility through the base interface. The visibility
operation keeps its null-widget guard and actual `widget::show()` / `hide()`
calls. Chat deactivation still redraws the control bar even when it has no
rollover widget; activation only hides the rollover. Message timing, wrapping,
constructor null policies and the separate unlink/delete versus delete-only
teardown paths remain intact. The empty native virtual overrides remain empty.

The shared `TSubWindow` base supplies a useful contrast: its complete native
field list explicitly makes the widget-range endpoints and background private,
while geometry is public and the widget vector/parent pointer are protected.
The range endpoints now follow that evidence. Registration grows their range,
draw substitutes them for the all-widgets sentinels, and `initialize()` retains
them. No external code needs an accessor for either endpoint. This mixed layout
is supported by native declarations; it should not be flattened merely because
some other classes had unjustified public/private hops.

These class reviews add seven private TCombatWindow members beyond the
initial accessor inventory, together with the combat-popup widget/configuration
blocks, control-button pointers and native-private TSubWindow range endpoints.
The original inventory counts below are historical, not a recount of this head.

### Earlier compiler observations

The declaration/body inventory visits all 139 project TUs. The implementation
has no runtime behavior fixtures: VC6 comparisons and native evidence are the
project's verdict. The affected-header closure selects 111 Windows TUs for
`homm3 build --fast`, with their paired full-TU CodeWarrior comparisons. The integrated score
freshness check also required rebuilding `widget` and `subwindow` (113 TUs
in the final validation set). This is
a source-interface recovery, so any observed byte-score changes are reported
and retain previous peaks in the score history.

After `homm3 status update --write-readme`, Windows weighted MAX is **97.76%**
(parent checkpoint: 98.18%); CUR is **97.19%**, and HIST is **98.71%**.
There are 4,381 exact current-source MAX implementations out of 4,785.
This is a source-boundary recovery with measured matching debt, not a
byte-neutral cleanup. The committed ledger preserves historical peaks and
keeps unchanged-source MAX separate from current compiler output.

The final affected Mac pass scores 1,478 pairs, of which 561 are exact;
1,691 emitted pairs have unresolved references and 77 claims lack an emitted
full-TU body. This is a partial comparison, not a full preservation checkpoint.
The newly lost campaign-completion and river-generation comparisons were
restored by retaining the existing owner getter body and locating the object
position getter beside the property interface. Some scored callers still lose
similarity, for example `getRmgPointOrientation` (100% to 85%) and
`getRmgSquaredDistance` (100% to 92.8571%) after replacing coordinate reads with
their established getters. Their prior Mac checkpoints remain in the ledger;
this pass does not claim Mac score preservation or overwrite that history.

Two previously checkpointed comparisons remain unavailable in the affected
set: `NewfullMap::readMonsterData` and `CChatWidget::draw`. Both are inherited
from the parent source. For the chat caller, a fresh full-TU compile of parent
`47ade15bf` produces exactly the same 284-byte body and named relocations as
this PR, including the unresolved `Bitmap16Bit::getPitch` call at `+0x6c`.
The earlier successful parent report used an object predating the nested pitch
accessor. Native Mac `0x186874` expands that load and retains raw grab, raw draw
and text-widget draw calls. Restoring the chat flag's public visibility in a
disposable diagnostic leaves the same call, so weakening access control does
not resolve it. Preserve the canonical bitmap helper path while investigating
that inherited inlining boundary.

Temporary extraction and review artifacts live in ignored
`build/accessor-audit/`. The owning C++ declarations remain the authority for
names and access; this report is a dated investigation, not a second symbol
ledger. The initial declaration review recorded 144 private and 17 protected
changes. Those counts precede the complete combat-window reviews above and
do not establish whole-codebase completion.
The added definitions exclude the reward-icon helper introduced by the
parent branch during integration.
