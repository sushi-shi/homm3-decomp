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

## Mechanism (cspriteframe case, 2026-10-08)

The name effect in `cspriteframe` goes through **data layout**, not code paths:

- VC6 orders a unit's `.bss` objects by a name hash, so a spelling change can
  move a static.
- A `u16` global gets the dword form `and ebx, dword ptr [mask]` only when it
  sits 4-aligned in the object; otherwise VC6 emits `and bx, word ptr [mask]`.
- Retail has `div2mask` at `0x6968a4` (aligned) and `div4mask` at `0x6968aa`
  (2 mod 4). `Draw` happens to align ours like retail for `div2mask`; the full
  Loki spellings put both masks at 2 mod 4.
- Retail's unit `.bss` also has three unreferenced holes (`0x69689d–a3`,
  `a8–a9`, `ac–af`). These are statics our source doesn't define yet, and they
  are needed to reproduce retail's layout with the original names.
- The blend-term order VC6 picks also follows the names.

See [behavior-catalog C12](../vc6/behavior-catalog.md).

## Hypothesis for other units

Where a name change moves bytes, first check whether it moved a `.bss` or
`.data` object's alignment. Only otherwise suspect a symbol-table walk such as
the scoped table `0x14bf4` (1,024 buckets, key at `+0x28`). That parallels the
period-64 handle effect (`docs/vc6/handle-period.md`, #186).

## Plan

1. Recover the three missing `cspriteframe` `.bss` statics, then retry the
   Loki-proven spellings. Separately, locate the `.bss` name hash, and any other
   name-ordered decision. Use the in-process C2 shim and IL replay (#186), and diff C2 state
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
