# RMG terrain and line shapes

The random-map terrain painter (`src/rmg_terrain.cpp`) gives each cell a
**shape** (`ERmgTerrainShape`) from the terrain of its eight neighbours, then
draws a frame of that shape from the cell's tileset. Each neighbour gets one
of three **edge kinds**, seen from the centre cell. A shape is described in one
canonical, unreflected orientation; the painter tries four reflections
(none, flipY, flipX, both), and the matching reflection becomes the sprite's
flipX/flipY. Each tileset also has plain **base** frames for shape 0. Some
tilesets also have **special** (decorated) base frames, chosen with a
per-terrain chance.

Diagram legend (north is up, `C` is the cell):

| Mark | Meaning |
| --- | --- |
| `.` | no edge |
| `b` | blend edge |
| `h` | hard edge |
| `o` | same terrain as the cell (read directly, not as an edge kind) |
| `x` | terrain differs from the cell |
| `?` | not tested by this rule |

## Neighbour kinds

`getRmgTerrainNeighbourKind(centre, neighbour)`:

1. Same terrain, or a sand centre: **no edge**. Sand never draws transitions;
   its neighbours draw them.
2. Both terrains blend (dirt, grass, snow, swamp, rough, subterranean, lava):
   **no edge** for a dirt centre, otherwise **blend edge**.
3. Anything else: **hard edge**. This covers water and rock centres, and a
   blending centre beside sand, water or rock.

So dirt, water and rock cells only ever see hard edges, and sand sees none.
Neighbour coordinates are clamped to the map: an off-map cardinal neighbour
is the cell itself (no edge), and an off-map diagonal repeats a cardinal
neighbour, or the cell itself at a map corner.

## Shape selection

`selectTerrainTransition` tries rule groups in priority order. It tries each
group under all four reflections before moving on, so group priority beats
reflection priority. Canonical orientations differ between families. The
outer corners 2, 8 and 23-26 face NW, while the mixed corners 17-22, 27 and
28 face SE. Groups, in order:

1. 28, 27
2. 23/25, 24/26
3. 21, 22 (or 8)
4. 17, 18
5. 2, 8
6. 17, 18, 21, 22, then the offset forms of 2 and 8
7. 19, 20
8. 4, 10, 3, 9
9. 14, 15, 16
10. 5, 11
11. otherwise 0

`?` cells can still be constrained, because an earlier group would have caught
them. In particular, no cardinal neighbour has an edge when groups 9 and 10 run.
`paintTransitions` then refines the four corner shapes. If either diagonal
beside an outer corner (NE or SW of the NW corner) has the cell's own terrain,
2/8 become 6/12. If a cell two steps along either side of an inner corner
(+2,0 or 0,+2 of the SE corner) has another terrain, 5/11 become 7/13.
Both tests clamp coordinates to the map.
Geometrically, both refinements mark a corner on a 45-degree boundary
running across the tile grid. This reading of the art is inferred from the
predicates.

### Single-kind shapes

Blend shapes 2..7 and hard shapes 8..13 are the same geometry; hard is blend + 6.

| Id | Enumerator | Neighbourhood | Meaning |
| --- | --- | --- | --- |
| 0 | `SHAPE_FILL` | `. . .`<br>`. C .`<br>`. . .` | interior; base or special frame |
| 2 / 8 | `SHAPE_N_W_BLEND` / `SHAPE_N_W_HARD` | `? b ?`<br>`b C ?`<br>`? ? ?` | outer corner: other terrain on the N and W sides. It also matches W + NE or N + SW (offset corner). |
| 3 / 9 | `SHAPE_W_BLEND` / `SHAPE_W_HARD` | `? . ?`<br>`b C ?`<br>`? ? ?` | straight west side |
| 4 / 10 | `SHAPE_N_BLEND` / `SHAPE_N_HARD` | `? b ?`<br>`? C ?`<br>`? ? ?` | straight north side |
| 5 / 11 | `SHAPE_SE_BLEND` / `SHAPE_SE_HARD` | `? . ?`<br>`. C .`<br>`? . b` | inner corner: the SE diagonal differs but no side does |
| 6 / 12 | `SHAPE_N_W_DIAG_BLEND` / `SHAPE_N_W_DIAG_HARD` | `? b o`<br>`b C ?`<br>`? ? ?` (or `o` at SW) | outer corner on a 45-degree edge |
| 7 / 13 | `SHAPE_SE_DIAG_BLEND` / `SHAPE_SE_DIAG_HARD` | `? . ? ?`<br>`. C . x`<br>`? . b ?`<br>`? x ? ?` (either `x`) | inner corner on a 45-degree edge |
| 14 | `SHAPE_NW_SE_BLEND` | `b . ?`<br>`. C .`<br>`? . b` | two opposite inner corners, both blend |
| 16 | `SHAPE_NW_SE_HARD` | `h . ?`<br>`. C .`<br>`? . h` | two opposite inner corners, both hard |
| 23 | `SHAPE_N_W_SE_BLEND` | `? b ?`<br>`b C ?`<br>`? ? b` | NW outer corner plus the opposite SE diagonal (a diagonal neck) |
| 24 | `SHAPE_N_W_SE_HARD` | `? h ?`<br>`h C ?`<br>`? ? h` | the same, all hard |

Shape 8 also covers two mixed SE corners, drawn with both flips inverted:
E hard + S blend + SW hard, and E blend + S hard + NE hard.

### Mixed blend/hard shapes

| Id | Enumerator | Neighbourhood | Meaning |
| --- | --- | --- | --- |
| 15 | `SHAPE_NW_BLEND_SE_HARD` | `b . ?`<br>`. C .`<br>`? . h` | opposite inner corners, NW blend and SE hard |
| 17 | `SHAPE_E_BLEND_SW_HARD` | `? ? ?`<br>`? C b`<br>`h ? ?` | east blend side with a hard SW diagonal. S may also blend. |
| 18 | `SHAPE_S_BLEND_NE_HARD` | `? ? h`<br>`? C ?`<br>`? b ?` | south blend side with a hard NE diagonal. E may also blend. |
| 19 | `SHAPE_E_BLEND_SE_HARD` | `? ? ?`<br>`? C b`<br>`? ? h` | east blend side with a hard SE diagonal |
| 20 | `SHAPE_S_BLEND_SE_HARD` | `? ? ?`<br>`? C ?`<br>`? b h` | south blend side with a hard SE diagonal |
| 21 | `SHAPE_E_HARD_SW_BLEND` | `? ? ?`<br>`? C h`<br>`b ? ?` | east hard side, with blend at SW (or along S with SW not hard) |
| 22 | `SHAPE_S_HARD_NE_BLEND` | `? ? b`<br>`? C ?`<br>`? h ?` | south hard side, with blend at NE (or along E with NE not hard) |
| 25 | `SHAPE_N_W_BLEND_SE_HARD` | `? b ?`<br>`b C ?`<br>`? ? h` | NW blend corner, hard SE diagonal |
| 26 | `SHAPE_N_W_HARD_SE_BLEND` | `? h ?`<br>`h C ?`<br>`? ? b` | NW hard corner, blend SE diagonal |
| 27 | `SHAPE_E_S_BLEND_SE_HARD` | `? ? ?`<br>`? C b`<br>`? b h` | SE blend corner whose diagonal is hard |
| 28 | `SHAPE_E_S_BLEND_NE_SW_HARD` | `? ? h`<br>`? C b`<br>`h b ?` | SE blend corner with hard NE and SW diagonals |

Id 1 is never returned and has no frames. The range arrays still reserve its slot.

## Frames and terrain tables

Each pattern rule builds frame ranges keyed by `shape * 2 + special`.
Transitions always draw from the non-special range of their shape.
If a cell's existing frame already shows the wanted shape, it is kept.
Rock uses a fixed table with the flips baked into the frames, keyed by shape
and both flips. The rule returns no sprite flip.

Frame ranges per shape. Land is grass, snow, swamp, rough, subterranean and
lava, which share one table.

| Shape | Land (79) | Dirt (46) | Sand (24) | Water (33) | Rock (48) |
| --- | --- | --- | --- | --- | --- |
| 0 base | 49-56 | 21-28 | 0-7 | 21-32 | 0-7 |
| 0 special | 57-72 | 29-44 | 8-23 | - | - |
| 2 `N_W_BLEND` | 0-3 | - | - | - | - |
| 3 `W_BLEND` | 4-7 | - | - | - | - |
| 4 `N_BLEND` | 8-11 | - | - | - | - |
| 5 `SE_BLEND` | 12-15 | - | - | - | - |
| 6 `N_W_DIAG_BLEND` | 16-17 | - | - | - | - |
| 7 `SE_DIAG_BLEND` | 18-19 | - | - | - | - |
| 8 `N_W_HARD` | 20-23 | 0-3 | - | 0-3 | 8-9, X 10-11, Y 12-13, XY 14-15 |
| 9 `W_HARD` | 24-27 | 4-7 | - | 4-7 | 16-17, X 18-19 |
| 10 `N_HARD` | 28-31 | 8-11 | - | 8-11 | 20-21, Y 22-23 |
| 11 `SE_HARD` | 32-35 | 12-15 | - | 12-15 | 24-25, X 26-27, Y 28-29, XY 30-31 |
| 12 `N_W_DIAG_HARD` | 36-37 | 16-17 | - | 16-17 | 32-33, X 34-35, Y 36-37, XY 38-39 |
| 13 `SE_DIAG_HARD` | 38-39 | 18-19 | - | 18-19 | 40-41, X 42-43, Y 44-45, XY 46-47 |
| 14 `NW_SE_BLEND` | 40 | - | - | - | - |
| 15 `NW_BLEND_SE_HARD` | 41 | - | - | - | - |
| 16 `NW_SE_HARD` | 42 | 20 | - | 20 | - |
| 17 `E_BLEND_SW_HARD` | 43 | - | - | - | - |
| 18 `S_BLEND_NE_HARD` | 44 | - | - | - | - |
| 19 `E_BLEND_SE_HARD` | 45 | - | - | - | - |
| 20 `S_BLEND_SE_HARD` | 46 | - | - | - | - |
| 21 `E_HARD_SW_BLEND` | 47 | - | - | - | - |
| 22 `S_HARD_NE_BLEND` | 48 | - | - | - | - |
| 23 `N_W_SE_BLEND` | 73 | - | - | - | - |
| 24 `N_W_SE_HARD` | 74 | 45 | - | - | - |
| 25 `N_W_BLEND_SE_HARD` | 75 | - | - | - | - |
| 26 `N_W_HARD_SE_BLEND` | 76 | - | - | - | - |
| 27 `E_S_BLEND_SE_HARD` | 78 | - | - | - | - |
| 28 `E_S_BLEND_NE_SW_HARD` | 77 | - | - | - | - |

The classifier could give water shape 24 and rock shapes 16 and 24, but their
tables have no frames for them. Water and rock disallow separated neighbours,
and both shapes separate the cell's matching neighbours, so terrain repair
probably removes them before painting. This has not been verified.

### Base and special frames

A shape-0 cell keeps its frame if that frame is already a base or special
frame. Otherwise a terrain with special frames rolls `rand() % 100` against
`chance * strength / 8`. Here `chance` is the rule's percentage at strength 8:
dirt 50, sand 70, grass 50, snow 80, swamp 80, rough 80, subterranean 60 and
lava 80. Water and rock have no special frames. `strength` is the painter's
strength, halved once for each same-terrain cardinal neighbour that already
shows a special frame. The generator's brushes all paint at strength 4
(`RMG_BRUSH_STRENGTH`), so they start at half these chances. The roll picks
a special frame on success and a base frame otherwise.

## Painting and repair

The painter first paints the requested rectangle with freshly rolled
shape-0 frames of the paint terrain. It then keeps two worklists:
paint-terrain cells that need repair, and other-terrain neighbours to
recheck. A rechecked neighbour that
needs repair is painted over with the paint terrain. `finish` drains both
lists until no repair is pending, then runs `paintTransitions`.

A cell **needs repair** when it is a one-cell gap: both W and E, or both N
and S, are other terrain. Water and rock also need repair when their matching
neighbours are separated. Separation is checked on the ring of eight
neighbours, clamped as above. A cardinal neighbour matches when it has the
same terrain. A diagonal matches only when it has the same terrain and an
adjoining cardinal also matches. The neighbours are separated when the ring
holds more than one run of matches.

Repairing a cell paints the paint terrain into its neighbours:

- One-cell gaps: it fills one of the two opposite cells. The negative side
  (N or W) is preferred, unless that side needs no repair and the other side
  either needs repair or is the only side without a perpendicular gap.
- Separated neighbours: it splits the nonmatching part of the ring into gaps,
  starting after a match. Each gap weighs 2 per cardinal and 1 per diagonal.
  It then paints the lightest gap's on-map cells and repeats until one gap is
  left. On a tie, the gap earlier in the ring is painted first.

`paintTransitions` counts terrain boundaries between neighbouring cells. A
cell with no differing neighbour is drawn as shape 0 with no flips. Every other
cell is classified as above. A frame or flip is written only when it changes.

## Line shapes (rivers and roads)

`selectRmgLinePattern` uses one bit per neighbour: whether it carries the same
line type. Diagrams use `#` for connected, `.` for not connected and `?` for
not tested. Shapes are again canonical, and the returned flips supply the
other orientations.

| Id | Enumerator | Neighbourhood | Meaning |
| --- | --- | --- | --- |
| 0 | `LINE_END_S` | `? . ?`<br>`. C .`<br>`? # ?` | end leading south; flipY for north. An isolated tile also uses this flipped. |
| 1 | `LINE_END_E` | `? . ?`<br>`. C #`<br>`? . ?` | end leading east; flipX for west |
| 2 | `LINE_NS` | `? # ?`<br>`. C .`<br>`? # ?` | straight north-south |
| 3 | `LINE_EW` | `? . ?`<br>`# C #`<br>`? . ?` | straight east-west |
| 4 | `LINE_SE` | `? . ?`<br>`. C #`<br>`? # ?` | bend joining E and S (reflections give the others) |
| 5 | `LINE_SE_VARIANT` | `? . #`<br>`. C #`<br>`# # ?` (either `#` diagonal) | bend whose NE or SW diagonal also connects. It is used only by tables with these frames; the art is not identified. |
| 6 | `LINE_NES` | `? # ?`<br>`. C #`<br>`? # ?` | T joining N, E and S; flipX for west |
| 7 | `LINE_ESW` | `? . ?`<br>`# C #`<br>`? # ?` | T joining E, S and W; flipY for north |
| 8 | `LINE_CROSS` | `? # ?`<br>`# C #`<br>`? # ?` | four-way crossing |

The priority is cross, then N+S (T or straight), then E+W (T or straight),
then a bend under four reflections, then ends. Tables without end frames, like
rivers, use straight pieces instead: EW when E or W connects, NS otherwise.

| Line shape | River frames (13) | Road frames (17) |
| --- | --- | --- |
| `LINE_END_S` | - | 14 |
| `LINE_END_E` | - | 15 |
| `LINE_NS` | 9-10 | 10-11 |
| `LINE_EW` | 11-12 | 12-13 |
| `LINE_SE` | 0-3 | 0-1 |
| `LINE_SE_VARIANT` | - | 2-5 |
| `LINE_NES` | 7-8 | 6-7 |
| `LINE_ESW` | 5-6 | 8-9 |
| `LINE_CROSS` | 4 | 16 |

The painter keeps a cell's frame when the frame already shows the selected
shape with the same flips. Otherwise it draws a random frame from the shape's
range.
