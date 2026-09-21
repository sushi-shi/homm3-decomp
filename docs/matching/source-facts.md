# Reviewing Dreamcast source facts

## Retail body coverage is not method completeness

Matching progress counts inventoried retail function bodies. Identical COMDAT
folding can give distinct C++ methods the same address, and a body already
matched under one name does not make its other source definitions complete.
An empty unmatched-body list is therefore not proof that every required
method, vtable definition or data object has been recovered. Compiling
individual objects also permits unresolved references; it does not prove
the candidate can link as a complete program.

The RMG vtables provide concrete examples in the pinned Complete executable
(slot numbers below are zero-based):

| RMG use | Vtable / slot | Retail body | Existing matched name |
| --- | --- | --- | --- |
| `TRmgTableTerrainRule::isSpecialFrame(int)` | 0x642cb0 / 2 | 0x5543f0, false / ret 4 | `CChatEdit::ignoreKey` |
| `type_object::unknownOperation()` | 0x640a74 / 1 | 0x5bc690, ret | `textWidget::dim` |
| `type_object::isWritable()` | 0x640a74 / 2 | 0x484620, true / ret | `TCampaignBuildingBonus::isBuildingBonus` |
| `type_treasure_def::isTerrainDependent()` | 0x640b64 / 2 | 0x484d50, false / ret | `TCampaignBonus::isBuildingBonus` |

The creature, experience and gold quest vtables at 0x640c00, 0x640c0c and
0x640c18, and key-tent vtable at 0x640c30, use the same true body in slot 2.
These are verified shared addresses and behaviors; the method names come
from the reconstructed interfaces. Sharing does not create additional unique
retail bytes to match. Separately, the seven creature-reward dwords at
0x6824e0 are data and fall outside the function denominator altogether.

Source-definition and alias coverage need separate accounting from byte
coverage. The current matching totals do not perform that completeness
check, and Dreamcast coverage cannot fill the gap for Complete-only RMG.

## Member access and method properties

```sh
PYTHONPATH=scripts python -m homm3.analysis.access_facts --all --json
PYTHONPATH=scripts python -m homm3.analysis.access_facts --module adventuremapwindow
```

This read-only audit uses libclang and each TU's manifest profile to compare
authored access and ordinary/static/virtual properties with the NB11 class
records. Owning `Before normalization` comments supply renamed member aliases.
It deduplicates repeated DC type definitions and authored declarations across
TUs. Overloads are correlated separately: a unique name/arity is a review
correlation, and equal-arity alternatives require matching parameter and const
facts. A retail-only overload does not inherit another overload's access.

Text and JSON use the same exit status: **0** for checked facts without findings
or gaps, **1** for findings, and **2** for incomplete coverage (including Clang
errors, failed parses, unmatched or ambiguous members). An adherence percentage
describes only correlated facts. Missing DC members and authored members that
cannot be correlated remain explicit; they are not proof of platform changes.
The audit does not compare SH4 and candidate statement or scope counts.

`python -m homm3.analysis.access_edit` prints a declaration-only proposal by default;
`--apply` tightens unambiguously correlated public or protected declarations
without moving members. It shares the auditor's owning-name aliases and preserves
nested records and conditional access sections while removing redundant labels.
It aborts on Clang errors and has no keep-public exclusion list.
Review both the proposal and its callers before applying it. Access changes
can change VC6 mangled symbols, so run the full `homm3 build` to regenerate
source-owned bindings and delinked targets.

A C2248 access error is a source-recovery lead. Inspect public wrappers and
ordinary member helpers before concluding that retail changed the interface.
For example, MoveHero's DC line table switches to `hero.h:645` for a `can_land`
call: that call belongs to the expanded `IsFlying` wrapper. Likewise, the
garrison status handler calls `townManager::ArmyCommand`, whose expansion
reaches private `select_army`. A missing retained helper body does not make the
helper a free static function.

Friend declarations need their own evidence. The combat-options and DirectPlay
callbacks are already free procedures in DC and directly call the restricted
methods there; retail preserves those callback relationships. This supports
specific friends. An unexplained external access alone does not. Preserve the
recovered relationships through compiler-score dips, record the remaining call
decisions beside their owners, and keep historical peaks in the normal ledger.

The completed access pass restores all 312 remaining correlated field-access
mismatches. Rendering, map/path searches, inventory, resource tables and UI
callers now use their recorded interfaces. Chat formatters retain their
variadic member ownership (see [VC6 member varargs](../vc6/variadic-members.md)).
The text-button constructor calls the ordinary `button::initialize` helper;
backpack removal and saved-screen drawing likewise retain their owning helpers.
The toolchain mirror fixes let the audit parse every TU without Clang errors.

Five method-property findings remain visible after review:

| Declaration | Evidence for the authored property |
| --- | --- |
| `widget::open` | Complete widget vtable 0x643c90 slot 1 is 0x5fe4d0; DC's method is ordinary. |
| `hero::getExperienceIncrement() const` | DC's LF_METHOD marks both overloads static, but the nullary LF_MFUNCTION and dc 0x2e60 have a const receiver and read its level. The one-argument overload is static. |
| `sample::~sample` | Complete vtable 0x6416d0 has the deleting destructor in slot 0. |
| `Bitmap816::zBufferDraw` with eleven arguments | Complete's four-slot vtable 0x63ba14 contains only the eight-argument screen wrapper at 0x44fdf0; the raw-buffer body 0x44fba0 has no slot. |
| `CNetMsgHandlerPause::~CNetMsgHandlerPause` | Complete adds the virtual base destructor and the derived deleting wrapper 0x557eb0 in vtable 0x640f04 slot 0. |

DC-supported friendships include
`textButton` reading private `button::Text`, the high-score free callbacks,
and `CNewPlayerUpdateProc` reading the lobby version for its outgoing message.
They are documented at the owning declarations, not placed in an exclusion list.
The specific `army::rangeAttack` friend is a separate retail-only hypothesis:
Complete adds a tower arm calling private `keepAttack`, absent from DC's caller.
The call supports this narrowly scoped reconstruction, but cannot distinguish
friendship from an unrecorded inline wrapper; the owning comment keeps that
limit explicit.

Three additional hero boundaries have **provisional names**: `clearSpells`,
`copyPrimarySkills`, and `setPrimarySkills`. The retail campaign reset and network
skill copies establish their operations; no DC declaration proves these helper
names or an `inline` keyword. Their ordinary definitions precede the relevant
callers. Network copies preserve raw signed skill bytes; `getPrimarySkill` is
reserved for gameplay values because it clamps to 0/1..99. These hypotheses
remain explicit for source review.

Full-build matching differences are retained in MAX/HIST. Adherence to correlated
access declarations does not certify complete DC source recovery: uncorrelated
names, overloads and missing older-build declarations remain separate coverage
gaps, and the whole-corpus audit continues to return 2 for that reason.

## Class declaration order

The access transformer preserves the tree's previous declaration order. Its
inserted access labels are not evidence of the original header's organization.
Dreamcast's NB11 `LF_FIELDLIST` supplies a separate recorded sequence: `hero`
has 185 public member/method entries followed by 14 private entries;
`advManager` has 231 public entries, 104 private entries, then three public
special/generated methods. `DoEventArtifact`, `DoArtifactSkillRequirement`,
`DoEventFreeArtifact`, `FightForArtifact` and `DoCustomArtifact` are consecutive
private entries. These are observations about the type records, not recovered
literal access labels or header line numbers.

```sh
python -m homm3.analysis.member_order --output build/member-order.json
python -m homm3.analysis.member_order --apply-plan build/member-order.json
python -m homm3.analysis.member_order --output build/member-order-after.json \
  --compare-layouts build/member-order.json
```

The proposal follows the recorded order for unambiguously correlated header
declarations, retaining owning comments. Duplicate complete DC class records
must agree on the name-group sequence. An overload group supplies one position;
it does not establish where individual overload declarations originally sat
among other names. Authored overload order is therefore retained.

Retail instance-field order and authored virtual-method order are hard
constraints. DC and Complete can lay out the same class differently; grouping
all private fields at the bottom would change offsets and constructor order.
Nested types, friends, uncorrelated declarations, directives and unparsed text
are fixed boundaries. The tool does not assign invented positions to them or
move a declaration across a conditional branch. Some repeated access sections
therefore remain. Their presence alone is neither source evidence nor an error.

Plans record the input hashes, changed declaration sequences, unresolved
boundaries and Clang layout/interface snapshots. Applying a stale or overlapping
plan fails before writing files. Compare a subsequent scan's snapshots to
verify unchanged fields, offsets, class sizes, access, signatures and virtual
order. Clang is only this structural check: finish with the full retail VC6
build, the access audit, and a matching-ledger comparison. Reordering declarations
can change optimizer state without changing a function's source hash; MAX/HIST
must retain those prior observations.

The access-review ordering pass changes 115 classes in 68 headers and removes
123 access switches (685 to 562 across the changed headers). Every other source
and comment line is conserved. The full VC6 build retains all 4,764 matching
rows, with 4,111 exact; the CUR/MAX/HIST ledger is byte-for-byte unchanged.
All 753 captured class layout/interface snapshots agree after reordering, and
a second complete scan proposes zero edits. The independent access audit still
parses all 152 TUs with zero errors and reports 4,217 access matches, zero access
mismatches, and the same explicit property findings and coverage gaps.
This is recovery within the stated constraints, not a claim that the remaining
uncorrelated or platform-specific declaration positions are known.

## Function declarations and calls

`homm3 dreamcast audit` finds disagreements between positive Dreamcast debug
records and declarations/calls in the authored C++ AST:

```sh
homm3 dreamcast audit 0x005c9be0 0x004f8880
homm3 dreamcast audit --module levelupwindow --json
homm3 dreamcast audit --all --json > build/source-facts.json
homm3 dreamcast audit --all --no-suppressions --json > build/source-facts-raw.json
homm3 dreamcast audit --all --suppressions path/to/reviewed.tsv --json
```

Module and whole-corpus selections include source-claimed Dreamcast functions.
An uncompiled claim or ambiguous definition produces a coverage gap. Clang
uses the TU's manifest profile and the existing VC6 header mirror; it supplies
source facts only. VC6 under Wine and pinned retail bytes remain the verdict.

Use `homm3 dreamcast lines` alongside the audit to inspect observed source-line
positions, spans, gaps, repeated attributions and file switches. It supplies
additional evidence for lifetime and statement hypotheses; it is not a
candidate line-count comparison. See [source-line geometry](dc-line-tables.md#source-line-geometry-across-functions).

The review checks:

- `const`/`volatile` at each pointer layer, references, base types and array
  extents for formal parameters and uniquely named locals; return and member
  qualifiers are included when their declarators can be read reliably.
  Known `std::basic_string` defaults are expanded when Clang omits them
  from a desugared type; explicit custom traits/allocators remain distinct.
- Parameter order when complete, unique names correlate both declarations.
  Formal types come from the NB11 function argument list; optimized local
  parameter records alone are not assumed to be a complete signature.
- Named helper calls whose Dreamcast source rows belong to the function's
  owning file. A missing authored call is a lead for an expanded helper or a
  platform difference, not proof that retail must call it out of line.
  Automatic objects and bound temporaries supply destructor boundaries;
  heap pointers and static locals do not stand in for scope-exit destruction.
  Their declaration locations are never used as destructor-order anchors.
  Public `std::min`/`std::max` are checked, including VC6's
  `std::_cpp_min`/`std::_cpp_max` spellings and explicit template arguments.
  A call to the project's by-value `max` wrapper remains a distinct boundary
  from a direct standard reference selector. Other `std::` helper groups are
  excluded pending correlation of the ports' library implementation types;
  a zero result does not cover those boundaries or helper overload signatures.
- Relative source order for unique helper anchors on different Dreamcast
  lines and different authored statements. Repeated calls and argument
  evaluation order are excluded; SH4 address order is never the comparison.

Project case/underscore normalization correlates semantic names. A local
renamed beyond that convention needs its existing owning evidence comment:

```cpp
// Before normalization: bb.
bitmapBorder* background = new bitmapBorder(...);
```

No second name ledger, regex function roster, machine-shape comparison, or
equal-count requirement is involved. Shadowed/absent local names, unsupported
function-pointer declarators, unresolved calls, partial parameter names and
parse errors remain coverage gaps. When Clang recovers a selected definition
despite errors elsewhere in its TU, findings are still useful, but the report
remains incomplete. Invalid/recovery nodes in the selected body reject it.

A raw formal type can itself reflect lowering. For example, Dreamcast's
`TCombatHeroSubWindow::Update` records `on_cursed_ground` as `T_UCHAR(0020)`,
but its public symbol `?Update@TCombatHeroSubWindow@@QAAXABVhero@@PBV2@_N@Z`
encodes native `bool`. `town::HasBuilding` has the same disagreement between
its byte record and `_N` public ABI. The hero-update `bool` and `unsigned char`
forms emit identical retail instructions, so byte equality cannot decide the
source type. Retain the `bool` declaration established by the public symbol
and explain the record conflict beside the function; an unsigned-byte finding
alone does not justify changing that source interface. Check the public
symbol before acting on this kind of primitive-type disagreement.

The same correction applies to `town::IsCastle` and `town::IsCapitol`:
their public symbols end in `QBA_NXZ`, while the lowered return records say
`T_UCHAR`. Restoring both canonical header declarations to `bool` is byte-flat
across all 92 recorded header consumers. Exact code alone could not expose
these incorrect source types; the public ABI supplies the deciding evidence.

The same distinction matters across callers. CSprite Draw/DrawCreature/
DrawSpellEffect use public `_N` flags; preserving unsigned-char flip locals
in three missile callers adds a `test`/`setne` conversion which retail lacks.
Native Boolean locals remove that conversion and recover the prior caller
scores. A byte-sized field forwarded directly to a proven Boolean parameter
can supply the same evidence, as iconWidget IsFlipped and combatManager
SaveBiggestExtent do. The public signature proves the callee type; the actual
retail forwarding instructions are needed to infer the local or field type.
Do not infer every byte field is bool from its name or zero/one values.

Each JSON finding has an ID derived from its kind, subject and compared facts.
The report also carries source identity, source hash, checked categories and
coverage gaps. Reviewed older-version or retail-ABI exceptions live in
`config/dreamcast-audit-suppressions.tsv`, keyed by module, Dreamcast function
offset and finding ID. The default audit moves matching rows from `findings` to
`suppressed_findings`; `--no-suppressions` restores the raw report. A suppression
for a selected function that no longer matches is stale and makes the audit
exit 2, so fixed or changed findings cannot silently disappear. The audit does
not edit source, alter matching scores, or automatically reject Dreamcast facts
after a score decrease.

The TSV header is `module`, `dc_offset`, `finding_id`, `confidence`, `reason`.
Confidence is an integer from 1 (tentative) through 10 (directly compelled by
retail ABI/body evidence), making lower-confidence exceptions the first
re-review queue. Offsets accept decimal or `0x` notation. Module and offset
scope IDs that can legitimately repeat for the same type disagreement in
several functions; one row suppresses all identical occurrences within its one
function. Missing/out-of-range confidence, empty reasons, duplicate keys and
malformed rows are rejected. JSON exposes the retained review metadata as
`suppression_confidence` and `suppression_reason`.

Exit status is **0** for no unsuppressed findings, gaps or stale suppressions,
**1** for review findings, and **2** for coverage gaps, stale suppressions or an
input error. Zero describes only the checked facts; it does not certify complete
source recovery. Whole-corpus review can take time because each selected
definition is parsed under its owning profile.

The loop is: inspect a finding's dossier/line evidence, check retail semantics
and ABI, restore the supported source fact, compile with VC6, and rerun both
the source audit and retail diff. Record a proven platform difference beside
the finding in the suppression TSV and leave it visible in JSON's
`suppressed_findings`. For example, Complete's hall
arrays have nine town rows and a seven-by-five layout; Dreamcast has eight
town rows and a three-by-three layout. Preserve retail's extents while
restoring the Dreamcast-proven `const`. Reducing unexplained disagreements is
useful; forcing cross-platform declarations to be identical is not.

Validation includes a real Clang fixture that detects four removed array
qualifiers and three flattened helper boundaries, then becomes clean when
those facts are restored. Independent controls cover pointer constness,
references, member/return qualifiers, shadowed locals, parameter inventories,
source-order exclusions, owning aliases, missing definitions and exit status.
The standard-selector fixture accepts both VC6 spellings and explicit template
arguments, and rejects flattened expressions or substitution with a by-value
wrapper, even when that wrapper calls the standard selector internally.

The string-alias fixture reproduces Clang's abbreviated `std::string` type
and accepts its explicit standard defaults. Negative controls retain findings
for custom traits/allocators, a different character type, and cv/ref changes.

```sh
PYTHONPATH=scripts python -m unittest homm3.analysis.test_source_facts \
  homm3.analysis.test_dreamcast homm3.analysis.test_dc_source_layout
```

## Check lookup destinations as well as instructions

A matching visible CFG can still hide wrong switch results.
`TSpellbookWindow::windowHandler` initially scored 99.9011%, with every
instruction in the displayed CFG aligned. Both inlined `convertID2HelpID`
pools nevertheless selected the wrong help rows: the reconstruction numbered
rows by widget ID instead of the retail display order. Decoding the actual
pool destinations and their returned constants recovered Previous/Next 0/1,
Adventure/Combat 2/3, school tabs 4..8, spell points 9, and cancel 10. Repairing
the canonical helper made both right-click and rollover lookups exact.

The current default sema view for this handler ends at its first physical
epilogue, +0x657; rollover code continues through +0xabb. An explicit
`--base-range +0x65a:+0xabc --target-range +0x65a:+0xabc` covers that tail.
Check the complete claimed carve and internal relocation addends as well:
in this case all 54 direct calls and 38 internal references now agree.
Identical unrelocated instruction bytes do not establish correct dispatch
semantics, and an incomplete displayed range does not establish full coverage.


## Recover reference parameters across virtual interfaces

The earlier CodeView rendering flattened message& to message* in the popup
hierarchy. The richer records retain message& in CHeroWindowEx::WindowHandler
(window.cpp:1036), CAdvPopup::WindowHandler (advmgr.cpp:11539) and
CAdvPopup::ExitDialog (advmgr.cpp:11528). Retail's x86 address passing alone
cannot distinguish those source declarations.

Restore the base, every override and the forwarding call together. Here the
coordinated edit covers 41 implemented methods and fifteen address-valued
calls to other helpers. The full VC6 build preserves all current scores and
all 42 emitted vtables, including their extents and every method relocation.
A normalized code comparison, allowing only the explicit method-signature
rename, finds identical executable sections in all 152 units. Source audit
coverage remains separate: existing Clang errors or ambiguous local names do
not become checked facts merely because the reference declaration is repaired.

## Distinguish lowered Boolean parameters from record padding

`CDiffHeader` has a complete 12-byte class record (type 0x54d4) with three
fields at offsets 0, 4 and 8. Its public constructor symbol,
`??0CDiffHeader@@QAA@H_NH@Z`, proves a native `bool` argument where the formal
record renders `T_UCHAR`. Restore that parameter without assuming that its
one-byte destination field must also be Boolean. The class's final three
bytes are ABI padding; a fourth, named padding-array member was unsupported.

Sixteen constructor/layout/local-name controls preserve all diff scores and
all function instructions. The restored model's 64 non-debug raw sections,
including data and relocation destinations, agree after checking equivalent
compiler-local symbol ordinals. The other 151 normalized objects remain
byte-identical. This is source-model evidence, not a new exact match:
`MakeDiff` remains 437 bytes against retail's 447 at 83.9244%.

The three scoped `diffHeader` locals retain their original names, but the
current audit cannot correlate their repeated names to individual scopes.
The constructor itself has no standalone retail claim. Both are explicit
audit coverage gaps; the public symbol, complete class record and compiled
layout supply the evidence for this correction.

`homm3 source-ownership` reports reviewed declaration-only bodies separately
from located definitions. A source annotation `// @dc-declaration-only: 0xTYPE`
can retain a recovered accessor whose exact CodeView declaration survives but
whose body has no procedure or foreign-header line attribution. The adjacent
comment must identify the retail field/caller evidence and state that header
ownership is provisional. This annotation supplies no source line and cannot
establish definition order. The gate requires the exact name, field-list type,
argument types, constness, return type, and one in-class inline header body; it
rejects duplicate bodies, retained VA claims, and annotations hiding located
procedures. These entries remain explicit coverage gaps in the build output,
not Windows-only exemptions or recovered source locations.
