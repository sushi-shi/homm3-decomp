# Data layout

How VC6 places globals and statics inside one object's `.data` and `.bss`,
measured with the pinned compiler (`game_o2_mt_gr_windows` flags) on
generated test units. Each unit placed hundreds of objects of every size
and kind after `char p[n]` arrays with n = 1..17, 259, 260 and 297, and
the placement was read from the COFF symbol table. The size-from-slot proof
(see [data matching](../tooling/data-matching.md#verified-game-bytes)) uses
these rules.

## Order

- `.data` (initialized) is emitted in definition order.
- `.bss` is not in source order. It is scrambled, apparently by a symbol
  hash, and independent C1 runs of one source can order it differently
  (see [inliner](inliner.md)). The retail order of a compiland's `.bss`
  therefore says nothing about its declarations; the retail addresses
  themselves give the neighbour of each object.

## Packing

Objects are packed without gaps beyond the alignment of the object that
follows. Object sizes and section sizes are not rounded up: a `char` or
`bool` directly follows a `char[n]` of any tested length, and 82 of the
tree's `.bss` sections have a size that is not a multiple of 4.

## Placement alignment

| Object | Placed on |
| --- | --- |
| 1-byte scalar or class (`char`, `bool`, `unsigned char`, a one-char struct) | 1 |
| 2-byte scalar or class (`short`, a two-char struct) | 2 |
| 4-byte scalar, pointer, enum, `float`, 4-byte class (`std::bitset<10>`, `struct { char x[4]; }`) | 4 |
| 8-byte scalar (`double`, `__int64`) | 8 |
| Class of 8 bytes or more, whatever its members (`struct { char x[9]; }`, a 212-byte `configStruct`) | 8 |
| Any array | max(4, the element's placement) |

Arrays are 4-aligned even when the element is a `char` and the array has
one or two elements; an array of 8-aligned elements (`double[2]`,
`struct { int a, b; }[2]`, `__int64[3]`) is 8-aligned. The rules hold in
`.data` and `.bss` alike.

Classes of 3, 5, 6 and 7 bytes are placed irregularly: a
`struct { char x[3]; }` after a one-byte array sits directly behind it, but
after a three-byte array it skips two bytes; a 5-byte struct skips four
bytes from some starts and none from others. The tooling does not derive a
bound from them.

## Sections and contributions

Game objects emit `.data` and `.bss` sections aligned to 4, or to 8 when
they hold an 8-aligned object (every object in the tree). LINK starts each
contribution on its section's alignment. An address that is not a multiple
of 4 therefore never starts a contribution; an address that is 4 but not 8
modulo 8 can only start a 4-aligned one.

## Consequence for a buffer's size

A buffer that ends before a verified neighbour in the same contribution is
shorter than its slot by less than the neighbour's placement alignment. A
neighbour on 1 byte fixes the size; one on 4 bytes leaves three sizes open.
Where the neighbour could open another compiland's contribution, the
contribution alignment bounds the gap instead (7 when the neighbour is
8-aligned).
