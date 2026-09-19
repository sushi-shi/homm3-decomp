# RMG Voronoi provenance

Resolution of [issue #5](https://github.com/sushi-shi/homm3-decomp/issues/5),
investigated 2026-09-19; documentation based on upstream `e57986b0`.

**The evidence supports adaptation of the Graphics Gems IV implementation,
possibly through a derivative, but does not establish direct provenance.**
The algorithm family is identifiable: incremental Delaunay insertion with
Guibas–Stolfi edge operations and walking point location. More distinctive
choices agree with Dani Lischinski's published implementation, beyond those
already present in the 1985 paper. No named third-party library, original game
helper declaration, or author-to-game attribution has been confirmed.
Independent implementation of the published algorithm remains possible.

## Method and evidence limits

The investigation separates three questions: which algorithm is used, whether
implementation choices suggest source adaptation, and whether historical
evidence identifies an actual supplier or author. It compares retail behavior
first, checks the published implementation against its antecedent, then looks
for corroborating notices. Similar C++ reconstructed with that implementation
as a guide would otherwise be circular evidence.

Retail means English GOG Complete 4.0, fixed base `0x00400000`, 2,732,032 bytes,
SHA-256 `057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274`.
Addresses below are virtual addresses; `+` ranges are function-local and
end-exclusive. The owning reconstruction is in
[rmg_support.cpp](../src/rmg_support.cpp), [rmg.cpp](../src/rmg.cpp), and
[rmg.h](../include/rmg.h). Its role-derived class/helper names are not recovered
original spellings. Names printed by delinked disassembly come from those
annotations, not from symbols shipped in the retail executable.

No Dreamcast counterpart was found for the selected Voronoi procedures:
`dreamcast find Voronoi`, `find Delaunay`, `find TRmgBoundaryVertex`, and
`show 0x005fd790` / `show 0x005fd6b0` provide no original declaration or
source-line layout. This is a limit of the available bridge/search, not proof
that every older source file lacked related code. No speculative C++ rewrite
or declaration promotion is part of this investigation.

## Pinned external references

- **Gems implementation:** [quadedge.C][gg-c], [quadedge.h][gg-h],
  [geom2d.h][gg-geom], and [the accompanying README][gg-readme] at
  `erich666/GraphicsGems` revision
  `ca4898517a93b6b45444a9cda5fd50132694b864`. The README attributes the code
  to Dani Lischinski's *Incremental Delaunay Triangulation*, *Graphics Gems IV*
  (1994). This repository snapshot is not itself a 1994 artifact.
  Comparing `quadedge.C` with the repository's
  [initial 2013 import][gg-initial] shows only include/graphics-declaration
  changes; the geometry and insertion bodies compared here are unchanged.
- **Algorithm antecedent:** Guibas and Stolfi,
  [*Primitives for the Manipulation of General Subdivisions and the
  Computation of Voronoi Diagrams*][gs], *ACM TOG* 4(2), 1985, pp. 74–123.
  Relevant printed pages: 96–98 (splice), 107 (incircle determinant),
  113 (side predicates), 120–121 (insertion and location).
  The inspected scan has SHA-256
  `b46073f5c1657d53765ef42d38f88bc461381c2e025d5570c1580dd8acecbe42`.
- **Independent correction record:** [Guibas–Stolfi errata hosted by
  Jonathan Shewchuk][errata], especially the post-flip predecessor correction
  and the warning about walking on the convex hull.

## Retail comparison

The navigation correspondence is `Sym = twin`, `Onext = next`,
`Oprev = previous`, `Lnext = twin->previous`,
`Lprev = next->twin`, and `Dprev = twin->previous->twin`.
These describe equivalent traversal roles, not identical storage layouts.

| Operation | Independently observable retail evidence | Published comparison and significance |
| --- | --- | --- |
| Edge representation | `0x005FCEF0` allocates a twin; each half is `0x24` bytes, with embedded integer site coordinates, zone pointer, twin, forward/backward links, and cached dual position. | [quadedge.h][gg-h], lines 8–62: four records per quad-edge, numbered navigation, point pointers. **Different representation.** |
| Splice and detach | `0x005FCF60` exchanges the two next links and their previous links; `0x005FCFA0` splices each half against its predecessor. | [quadedge.C][gg-c], lines 16–45: corresponding topology operations. **Common family evidence.** |
| Initial subdivision | `0x005FD010` builds four corners at `(-200,-200)`, `(400,-200)`, `(400,400)`, `(-200,400)`, four perimeter edges and a diagonal. | [quadedge.C][gg-c], lines 49–64: caller-supplied triangle. **Different boundary policy.** |
| Point location | `0x005FD6B0`, length `0xD7`: endpoint checks followed by side tests on the edge, next edge, and destination predecessor. A destination hit returns the twin (`+0xCB`), putting the query at the result's origin. | [quadedge.C][gg-c], lines 148–168, and paper p. 121: same walk, but either endpoint returns the current edge. **Shared walk; meaningful behavioral difference.** |
| Point on an edge | `0x005FD790 +0x40:+0xD0`: three calls to squared distance `0x005FDB10`; reject either distance exceeding edge length squared; evaluate an integer implicit line and test exact zero. On success, move to predecessor and remove its next edge. | [quadedge.C][gg-c], lines 128–143, 180–183: distance rejection, line evaluation, and predecessor/deletion sequence. **Suggestive implementation choice.** |
| Insertion fan | `0x005FD790 +0xD0:+0x186`: create/splice first spoke, save it at the diagram root; repeat connection and predecessor traversal until `twin->previous` equals that root. | [quadedge.C][gg-c], lines 185–195: root-edge termination. **More specific than the original pseudocode.** |
| Legalization | `0x005FD790 +0x186:+0x348`: predecessor's opposite site must pass the side test and incircle test; flip, then reload the edge's predecessor; otherwise stop at `next == root` or advance `next->next->twin`. | [quadedge.C][gg-c], lines 199–210: same loop choices. **Suggestive in combination.** |
| Numeric predicates | `0x005FDAE0` computes a signed 32-bit orientation. Insertion makes four further orientation calls and uses signed 64-bit products/additions for the incircle determinant (`+0x1EE:+0x2AB`). | [quadedge.C][gg-c], lines 94–125: corresponding area/determinant algebra. **Mathematical family evidence, with different arithmetic.** |
| Dual construction and ownership | `0x005FDB40` computes an integer circumcenter and caches it on three incident halves. `0x005FD390` registers both allocated halves in a vector; `0x005FD5B0` erases both; `0x005FD330` destroys owned halves. | [quadedge.h][gg-h], lines 37–55: different ownership/interface; no corresponding dual-cache fields. **Game integration, not proof of original authorship.** |

### What distinguishes the implementation from the paper

The 1985 paper already supplies the walk, splice/connect/swap operations,
orientation-side relation, determinant test, duplicate rejection, and
insertion/legalization structure. Those similarities alone cannot select
Graphics Gems over another implementation of the same algorithm.

The closer correspondence is a *combination* of choices:

| Choice | Paper, p. 120 | Gems implementation | HoMM3 retail |
| --- | --- | --- | --- |
| Fan completion | Compare destination with saved first site. | Compare left-next with saved starting edge. | Compare twin's previous with root at `0x005FD912`. |
| Legalization completion | Compare origin with first site. | Compare next with starting edge. | Compare next with root at `0x005FDABD`. |
| Resume after flip | Reuse saved predecessor `t`. | Read the modified edge's predecessor. | Reload predecessor at `0x005FDA9D`. |
| Segment membership | Left abstract. | Distance guards plus a line equation. | Squared-distance guards plus an unnormalized integer line equation. |

The post-flip change is also independently documented in the [errata][errata],
so it is not a unique fingerprint. The two root-edge comparisons and segment
test make adaptation a better-supported hypothesis than lookup exactness
alone. They still cannot distinguish direct use of Lischinski's code from an
intermediate derivative or independently chosen equivalent implementation.

### Numerical and edge-case differences

[geom2d.h][gg-geom], lines 21–23, 65–68, 126–129, and 145–159, uses `float`
coordinates, a `1e-6` tolerance, Euclidean norms, approximate point equality,
and a normalized line. Retail uses integer coordinates, exact coordinate
equality, squared distances, and no line normalization or epsilon here.
Duplicate insertion returns without creating edges; a point beyond an
endpoint is rejected by the distance guards even if collinear.

Retail requires a strictly positive incircle determinant: a zero determinant
does not flip that edge. The 64-bit final computation does **not** make the
predicates arbitrary-precision: orientation and squared-norm intermediates
are still 32-bit. No general overflow guarantee is inferred.

Both walks retain the non-strict negated side tests discussed in the
[errata][errata]. That does not establish a reachable game hang: HoMM3's
artificial outer square changes which hull inputs normal map generation
encounters. Outside-domain and arbitrary degenerate inputs were not executed
in the game during this investigation.

The lookup's [earlier source-family experiment](vc6/source-families.md)
found multiple exact C++ forms. Exactness of its 215 bytes verifies a candidate
against retail; it neither recovers the original helper spelling nor identifies
an external author. The incomplete match of other functions likewise does not
invalidate the specific retail loads, branches, calls and stores cited above.

## Other historically plausible sources

[Fortune's Netlib package][netlib] is explicitly a sweepline implementation.
The retail insertion walk and local edge flips do not identify that package.
This excludes an unchanged implementation of that core, not every possible
reuse of utility code.

Shewchuk's [1996 Triangle implementation discussion][triangle] describes
triangle-based storage, several triangulation algorithms, and elimination of
duplicate input points. It also discusses earlier quad-edge implementations
of the same algorithm family. Retail's paired halves and fixed-width integer
predicates do not identify Triangle; neither duplicate rejection nor the
Guibas–Stolfi ancestry is a package fingerprint. No exhaustive comparison of
all pre-1999 geometry packages is claimed.

## Historical corroboration searched

The [1999 Armageddon's Blade manual][manual] identifies the random map
generator as a new feature (printed pp. 4–5; use on p. 8). Its copyright/technology
notice on p. 1 names Bink and Miles but supplies no geometry attribution.
The full 28-page text contains no `Voronoi`, `Delaunay`, `Graphics Gems`,
`Lischinski`, `Guibas`, `Stolfi`, or `Shewchuk` attribution. This manual has no
staff-credits section; it is not a substitute for examining every shipped
credit asset. Inspected PDF SHA-256:
`aacfd727d04fab46e19fd701d5ab20dc02f5f070365e4b667031cf7247875dfe`.

The pinned retail executable's printable ASCII and UTF-16LE strings were
searched for the same names, plus `Triangle`, `Fortune`, and `copyright`.
There were no geometry names; `Fortune` hits concern game content, while zlib
copyright strings remain visible. This is a bounded negative result, not
evidence against statically incorporated source: notices can be absent from
compiled code, and game resources were not all searched.

Targeted public searches combining Heroes III / New World Computing with
Graphics Gems, Lischinski and Voronoi yielded no attributable developer
statement connecting this code to the publication. No developer was contacted.
There is no recovered game-source notice, original geometry header, licensed
package record, or first-person account establishing the transmission path.

## Reproduction and closure

From an initialized checkout with the pinned images, inspect the retail
functions, not just the reconstructed C++:

```sh
homm3 sema disasm 0x005FCEF0
homm3 sema disasm 0x005FCF60
homm3 sema disasm 0x005FCFA0
homm3 sema disasm 0x005FD010
homm3 sema disasm 0x005FD330
homm3 sema disasm 0x005FD390
homm3 sema disasm 0x005FD5B0
homm3 sema disasm 0x005FD6B0
homm3 sema disasm 0x005FD790
homm3 sema disasm 0x005FDAE0
homm3 sema disasm 0x005FDB10
homm3 sema disasm 0x005FDB40
homm3 sema diff 0x005FD6B0 --summary
```

Validation on this documentation branch: the full `homm3 build` passed all
gates across 152 units, with zero source-ownership violations and no source-edit
MAX regression. The lookup comparison reported 100%, all views agreeing,
including no divergent source statement. These checks validate the retail
comparison and unchanged reconstruction; they do not validate authorship.

Compare the pinned reference files and printed paper pages cited above.
Neither C++ names nor declarations should be changed on the strength of this
resemblance. Issue #5's requested investigation can close with **a supported
adaptation hypothesis, a confirmed algorithm family, and unresolved direct
attribution**. An original source notice or a corroborated account from someone
who worked on this code could upgrade that conclusion; another equivalent
triangulation or matching helper spelling alone could not.

[gg-c]: https://github.com/erich666/GraphicsGems/blob/ca4898517a93b6b45444a9cda5fd50132694b864/gemsiv/delaunay/quadedge.C
[gg-h]: https://github.com/erich666/GraphicsGems/blob/ca4898517a93b6b45444a9cda5fd50132694b864/gemsiv/delaunay/quadedge.h
[gg-geom]: https://github.com/erich666/GraphicsGems/blob/ca4898517a93b6b45444a9cda5fd50132694b864/gemsiv/delaunay/geom2d.h
[gg-readme]: https://github.com/erich666/GraphicsGems/blob/ca4898517a93b6b45444a9cda5fd50132694b864/gemsiv/delaunay/README
[gg-initial]: https://github.com/erich666/GraphicsGems/blob/3f85c80f5bae3ac347a94db2c9ce482583976d6d/gemsiv/delaunay/quadedge.C
[gs]: https://www.wias-berlin.de/people/si/course/files/GuibasStolfi85-QuadEdge.pdf
[errata]: https://people.eecs.berkeley.edu/~jrs/meshpapers/GSflaws
[netlib]: https://www.netlib.org/voronoi/
[triangle]: https://www.cs.cmu.edu/~quake/tripaper/triangle2.html
[manual]: https://heroes3wog.net/download/%5BHeroes%203%5D%20Armageddons%20Blade%20Manual.pdf
