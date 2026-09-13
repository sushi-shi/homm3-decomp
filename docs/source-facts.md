# Reviewing Dreamcast source facts

## Member access and method properties

```sh
PYTHONPATH=scripts python scripts/experiments/verify-access-adherence.py --all --json
PYTHONPATH=scripts python scripts/experiments/verify-access-adherence.py --module adventuremapwindow
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

`apply-access-adherence.py` prints a declaration-only proposal by default;
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
variadic member ownership (see [VC6 member varargs](vc6/variadic-members.md)).
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
python scripts/experiments/recover-member-order.py --output build/member-order.json
python scripts/experiments/recover-member-order.py --apply-plan build/member-order.json
python scripts/experiments/recover-member-order.py --output build/member-order-after.json \
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

Each JSON finding has an ID derived from its kind, subject and compared facts.
The report also carries source identity, source hash, checked categories and
coverage gaps. It does not edit source, suppress findings, alter matching
scores, or automatically reject Dreamcast facts after a score decrease.

Exit status is **0** for no findings or gaps, **1** for review findings, and
**2** for coverage gaps or an input error. Zero describes only the checked
facts; it does not certify complete source recovery. Whole-corpus review can
take time because each selected definition is parsed under its owning profile.

The loop is: inspect a finding's dossier/line evidence, check retail semantics
and ABI, restore the supported source fact, compile with VC6, and rerun both
the source audit and retail diff. Record a proven platform difference beside
the function and leave it visible in the report. For example, Complete's hall
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
