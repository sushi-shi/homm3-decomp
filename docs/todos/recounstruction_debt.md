# Reconstruction debt

Manually maintained source-cleanup checklist. Counts are recorded snapshots,
not live totals. Unchecked items need review; they are not necessarily defects.

## Remaining work

- [ ] Remove avoidable casts: **3,047 named casts**
  (**2,830 `static_cast`**, **217 `const_cast`**).
- [ ] Simplify avoidable union views: **63 union definitions**.
- [ ] Review **31 goto statements** in **12 functions across 10 files**;
  existing dispositions are in the [goto audit](../vc6/goto-audit.md).
- [ ] Review manual varargs.
- [ ] Review unrelated variable reuse.
- [ ] Review stack aggregates and unused members.
- [ ] Review unresolved buffer bounds.
- [ ] Review [compiler warnings](../compiler-warnings.md), including
  potentially uninitialized locals and missing returns.
- [ ] Review inlining pragmas: **388 directives**, including resets,
  across **190 `inline_depth(0)` and 4 `auto_inline(off)` regions**.
  See the [union and pragma audit](../vc6/union-pragma-audit.md).
- [ ] Search for inline functions.

## Completed reviews

- [x] Artificial address arithmetic:
  [audit and retained arithmetic](../vc6/address-arithmetic-audit.md).
- [x] Owner recovery from member pointers:
  [owner-pointer audit](../vc6/owner-pointer-audit.md).
- [x] Out-of-object pointers:
  [repairs and allocation proofs](../vc6/pointer-boundary-repairs.md).
- [x] Preprocessor cleanup and shared-code macro search:
  **36 → 1 `#define` directive** in `src/`;
  [audit and retained macro](../vc6/preprocessor-audit.md).

## Scope and validation

Unless stated otherwise, counts cover tracked project C/C++ in `src/` and
`include/`. They exclude comments, literals, disabled `#if 0` bodies,
generated build copies, and vendor code. Cast counts refer to written named
conversions; the cleanliness gates separately check C-style and
`reinterpret_cast` conversions. Items without counts still need a tree-wide
census.

Use source and retail evidence to recover helpers, macros, types, and
lifetimes. Validate changes with VC6, measure effects on other functions,
and preserve MAX/HIST tracking. The linked audits document retained code
as well as repairs; a completed review does not mean every occurrence was
removed.
