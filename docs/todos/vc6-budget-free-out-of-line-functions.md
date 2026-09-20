# TODO: explain a budget-free function retained out of line

Status: open, measured 2026-09-01.

## Scope

The source currently has 27 semantic `#pragma auto_inline(off)` regions.
Exactly one reconstructed body among them is in VC6's strict budget-free
class (`cb <= 0x28`):

| Function | Retail body | Controlled VC6 result |
|---|---:|---:|
| `ResourceManager::TCacheMap::lower_bound_iterator` | `0x0055e740`, 23 B | 60/60 sites expanded |

For each control, only its definition-wide fence was disabled in a scratch
copy of the real TU, a 60-site caller was appended, and the TU was compiled
with its exact `config/units.toml` profile. Neither caller retained a `call` or
tail `jmp` to the tested callee. This is the `cb <= 40` behavior documented in
[`docs/vc6/inliner.md`](../vc6/inliner.md): the callee itself spends no inline
budget and is expanded at every eligible site, subject only to candidacy,
depth, the running cap, and the post-substitution veto.

This TODO does **not** cover the 329 statement-scoped
`#pragma inline_depth(0)` settings. A statement pin may suppress several
direct and nested candidates, so settings cannot be counted as functions.

## Hypothesis

The retail sources may have contained release-elided source mass such as
`ASSERT`, `TRACE`, or a `VERIFY`-like expression. Such statements can change
C1XX's front-end `cb` or body-save candidacy while contributing no retail
instructions. They are a useful hypothesis, not an admitted source fact.

Other live explanations include a different source body or scope, different
definition visibility or inline marking, an original pragma, C1XX's
body-save gate, the post-substitution veto, and the retail link's mixed
compiler generations. Dreamcast is an older cross-architecture revision and
cannot settle any of those by absence alone.

## `ResourceManager::TCacheMap::lower_bound_iterator`

Current source: [`src/resourcemanager.cpp`](../../src/resourcemanager.cpp),
retail `0x0055e740`. The two-statement wrapper calls `lower_bound`, stores the
returned node through the explicit result pointer, and returns that pointer.
Retail retains the exact 23-byte wrapper inside the nested `CSprite::Dispose`
path. There is currently no retail/Dreamcast bridge for this function, so the
leading-gap metric is unavailable.

This is the stronger one-statement-mass candidate: a byte-inert source
statement that raises `cb` above 40 can turn a formerly free late-site
expansion into a budgeted rejection without changing the wrapper's emitted
bytes.

- [ ] Capture `CSprite::Dispose`'s candidate order and remaining budget at
      the `lower_bound_iterator` site.
- [ ] Find the minimum byte-inert `cb` dose that restores the retail call
      after removing the fence.
- [ ] Test plausible release-shaped `ASSERT`/`TRACE`/`VERIFY` expressions and
      ordinary source-shape alternatives against that measured dose.
- [ ] Compare the wrapper with the pinned VC6 Dinkumware map/tree source; do
      not invent a game assertion if the library surface already explains
      the boundary.

## Resolved control: `type_artifact_effect::~type_artifact_effect`

This 7-byte body was part of the original two-function audit, but it no longer
uses an inline fence. Retail has two distinct deleting-destructor contexts:
the base-vtable copy at `0x004324d0` expands the base destructor, while the
shared deleting destructor at `0x00433080` calls the standalone body at
`0x00432500`. Correcting that attribution lets the current VC6 source preserve
both boundaries naturally. It remains a useful negative control, not an open
budget-free exception.

## Completion gate

A task above closes only when all of the following hold:

1. Positive retail, Dreamcast, sibling-source, or pinned-library evidence
   supports the replacement source shape.
2. Removing the corresponding `auto_inline(off)`/`auto_inline(on)` pair keeps
   the standalone body and every affected caller at their ratcheted maxima.
3. A negative control proves that flattening the recovered source or removing
   its release-elided mass returns the unwanted expansion.
4. The explanation is promoted to `docs/vc6/inliner.md` or
   `docs/vc6/behavior-catalog.md`; this TODO is then marked closed rather than
   silently deleted.
