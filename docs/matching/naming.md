# Naming conventions

## Proven original spellings win

The decomp shows how the original code was, so a spelling proven by native
symbols overrides the conventions below. That includes original case,
Hungarian prefixes and member styles such as `_m_pInBuf` or `DrawTile`. The
proof sources are:

- the Loki Linux editor `h3maped` (g++ 2.95 mangled names,
  `__PRETTY_FUNCTION__` and assert strings);
- the Loki game;
- Dreamcast decorated and CodeView names;
- Mac symbols.

Decided 2026-10-08. Renaming back to proven originals is planned soon. The
cross-platform port later normalizes names to its own consistent format; that
is not done in the decomp.

**Names can change bytes.** Identifier spelling is occasionally VC6 compiler
state. In `cspriteframe.cpp`, renaming `CSpriteFrame::draw` to Dreamcast's
`Draw` changes instruction selection in later functions of the unit, taking
`drawSpellEffect` from 99.34 to 100 (#178, [behavior-catalog
C12](../vc6/behavior-catalog.md)). Most renames are byte-neutral. The
mechanism is an open item: [VC6 name-keyed compiler
state](../todos/vc6-name-keyed-state.md). After a rename, refresh targets with
`homm3 delink --unit <TU>` and compare every function of the unit.

## Conventions where no original spelling is proven

Preserve evidenced semantic names,
prefer Dreamcast over NH3API, remove Hungarian type prefixes, and use
lowerCamelCase with `m_`, `s_`, and `g_` for instance members, static members,
and globals. Locals, parameters, and ordinary functions have no scope prefix.
Types, required external ABI names, and the vendor tree retain their spellings.
Source comments retain previous spellings for reference lookup; a previous
spelling alone is not evidence that the retail source used it.

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
