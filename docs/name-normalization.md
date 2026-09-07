# Naming normalization

This pass starts from `4b0953c8bdab90558d543da09622ae8d697ab47a` and applies
the naming convention in `AGENTS.md`: preserve evidenced semantic names,
prefer Dreamcast over NH3API, remove Hungarian type prefixes, and use
lowerCamelCase with `m_`, `s_`, and `g_` for instance members, static members,
and globals. Locals, parameters, and ordinary functions have no scope prefix.
Types, required external ABI names, and the vendor tree retain their spellings.
Source comments retain previous spellings for reference lookup; a previous
spelling alone is not evidence that the retail source used it.

## Field audit

The initial declaration census found 3,685 active field locations, including
515 explicit placeholder declarations. The broader implementation AST census,
including additional template and facade declarations, covers 3,776 instance
field locations; every one has an `m_` prefix after the pass. These are source
declaration counts, not original-member counts or byte-coverage percentages.

| Initial placeholder category | Before | Semantic names admitted | Still unnamed |
| --- | ---: | ---: | ---: |
| Synthetic member names | 323 | 172 | 151 |
| Padding / opaque labels | 192 | 11 | 181 |
| Total | 515 | 183 | 332 |

The initial synthetic-name population contains 29 byte-array declarations and
5 void-pointer-based declarations; the other 289 have concrete types. Naming
does not establish those types' correctness. The 192 padding/opaque declarations
include actual padding, unresolved storage, and alternate union views. This
pass preserves all member types, extents, declaration order, and layouts,
including the 11 opaque declarations that now have semantic names.
The 151 still-synthetic names include 16 byte-array and 5 void-pointer-based
declarations; the other 130 use concrete types. There is no defensible single
count of semantically untyped original members from declaration syntax alone.

The reference audit used the local Dreamcast corpus and NH3API commit
`1ef91ced1eda61060e98104a0fa5bc58ac3bfa90`:

| Original 515 placeholders | NH3API | Dreamcast | Both | Either |
| --- | ---: | ---: | ---: | ---: |
| Equal-extent candidates | 172 | 158 | 145 | 185 |
| Broader interval overlaps | 200 | 182 | 167 | 215 |

Dreamcast counts include name bridges through NH3API; an equal-extent candidate
is a lead, not independent proof of x86 identity. Of the 332 still-unnamed
declarations, 29 have broader NH3API overlaps and 25 have Dreamcast overlaps
(22 both, 32 either). Only two retain equal-extent candidates in this audit;
both were rejected on source/layout review:

- `advManager::field_fc`: the proposed `animFrame` conflicts with the existing
  member and its documented location. It remains `m_fieldFc`.
- `CAdvPopup::field_50`: the proposed `exitId` belongs at a different location
  in the retail model. It remains `m_field50`.

The other unresolved declarations are not proven absent from the reference
corpora. Broader overlaps can describe several members inside one opaque span
and require layout work before admission. Source declarations and their evidence
comments own admitted names; this document is not a second symbol ledger.

## Lookup and compatibility

`m_` distinguishes members from locals, but cannot distinguish two members
whose Hungarian prefixes encode different representations. `SBolt::fX/fY`
become `m_x/m_y`; the documented rounded pixel coordinates `iX/iY` become
`m_pixelX/m_pixelY`. The troop-count flag becomes `m_showTroopCount`.

Normalization also merges some formerly case-distinct operation names.
Explicit `this->` or global qualification preserves lookup where local values
share an operation's spelling. `TViewWorldWindow` exposes the inherited
`drawWindow` overload with a using-declaration, preserving virtual dispatch.
`Delete` becomes `deleteElement` or `deleteFile` because `delete` is a keyword.
Required SDK imports, COM methods, and standard-library member spellings remain
unchanged at their boundaries. The compiler-function key recognizer accepts
both original and normalized names for the project-owned obstacle-vector
facade; the standard-library matching rules remain unchanged.
The cleanliness checker also distinguishes a control condition followed by
`this->method()` from an actual C-style cast of `this`, including nested
conditions. Its positive and negative selftests cover both forms; the cast
floor remains zero.

## Validation and island search

The pristine checkpoint had 3,730 / 4,370 exact functions. Full builds after
normalization and retail-label regeneration retain all 3,730 exact functions,
all banked addresses, and all MAX/history peaks. The label recognizer's selftest
covers both spellings of the obstacle-vector helpers. The final checkpoint
uses VC6 SP3 under Wine and the repository's full build gates.

One current score moves from 98.9251 to 98.8868: `army::doAttack` at
`0x00441610`. Comparing the pre-pass and normalized objects isolates four bytes:
two independent EDI/EBX reloads after `doMultiHeadAttack` exchange order.
The retail comparison retains 106 matching blocks, 58 matching branches, and
the same call sequence.

A bounded experiment reused Gruntz's deterministic declaration-forest generator:
16 forests plus the unperturbed state, crossed with two meaningful local-name
spellings, for 34 disposable whole-TU builds. It found two states; six forest
trials recovered the pre-pass relocation-masked instruction bytes. No synthetic
declarations were retained. The ordinary result-local order already documented
beside the function was also tested; 98.8829 was worse, so it was rejected.
The small current-score dip remains observable, with the previous peak preserved.
