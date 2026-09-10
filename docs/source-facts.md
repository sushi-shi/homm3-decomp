# Reviewing Dreamcast source facts

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
