# VC6 name-keyed compiler state

An identifier's spelling can change the bytes VC6 emits for other functions in
the same unit. The mechanism is not located yet. Until it is, we can't predict
which renames are byte-neutral and which are not, and `homm3 vc6 compile-m`
doesn't model this channel.

## Evidence

- **[behavior-catalog C12](../vc6/behavior-catalog.md):** renaming only
  `CSpriteFrame::draw` in `cspriteframe.cpp` changes instruction selection in
  later functions of that unit.
  - Under `Draw` (Dreamcast's original), `drawz`, `drawA` or `drawB`,
    `drawSpellEffect`'s blend loops emit retail's dword
    `mov ebx,[s_div2mask]; and edx,ebx`.
  - Under `draw` or `drawRle` they emit the word-sized
    `and dx,word [s_div2mask]`.
  - `drawCreatureImpl` (93.70 ↔ 95.88) and `drawAdvObjWithFlagAlpha` move with
    it.
- **It's the decorated name.** Retyping `draw`'s two flags from `bool` to
  `unsigned char`, which changes only the decorated name, has the same effect.
  Removing `drawSpellEffect`'s own call to `draw` does not, so it is not a
  call-site conversion.
- **It's usually neutral.** The 2026-10-07 DC-bool interface sweep changed the
  decorated names of about 340 functions with no Windows MAX movement apart
  from real type effects. Earlier, retyping `army::is` was byte-neutral
  tree-wide.

- **Original spellings alone don't reproduce retail.** On 2026-10-08, every
  `CSpriteFrame` method and static was renamed to its Loki-proven spelling
  (`Draw*`, `Clip`, `Crop`, `GetMap`, `div2mask`…). That lost all of `Draw`'s
  gains: `drawSpellEffect` went back to 99.34, and `DrawTileShadow` fell from
  its 99.90 MAX to 99.87. So the result depends on the unit's whole set of
  names, not on one name being original. The rename diff is not kept; it is
  reproducible from the Loki symbol table.

## Hypothesis

C1XX or C2 hashes decorated names into an internal table, and some decision
walks that table in hash order. A spelling that lands in another bucket then
changes the order. This parallels the period-64 handle effect: the
regalloc-setup constant hash `0x2c6ce` at `.bssbe 0x9d750`, documented in
`docs/vc6/handle-period.md` (#186). The scoped symbol table `0x14bf4` (1,024
buckets, key at `+0x28`) is a candidate. It was ruled out for the period-64
effect, but not for names.

## Plan

1. Locate where decorated names are hashed and which decision consumes the
   order. Use the in-process C2 shim and IL replay (#186), and diff C2 state
   at the affected functions' entry between `Draw` and `draw`.
2. From the mechanism, compute which functions any name change can affect,
   and which names they are sensitive to, without renaming at random.
3. Add the channel to `homm3 vc6 compile-m` and `homm3 vc6 impact`, if it is
   real carried state.
4. Feed it into the naming work. AGENTS.md now prefers proven original
   spellings, and this channel is one reason they matter for matching:
   original names reproduce the original hash layout.

## Status

Open. Recorded 2026-10-08 with PR #178 (`CSpriteFrame::draw` → `Draw`:
`drawSpellEffect` 99.34 → 100).
