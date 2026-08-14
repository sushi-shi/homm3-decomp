# The Dreamcast line/addr table — reading the original statement layout

*Opened 2026-08-14, on the round that closed
`viewarmywindow:??0TViewArmyWindow@@QAE@HHHE@Z`.*

The Dreamcast dump is usually consulted for **names, types and layouts**
(`docs/…`, `CLAUDE.md`'s evidence ranking). It carries two more records that
nothing in this tree was using, and they are the only artefact anywhere that
speaks about retail's **source text**:

* a **line/addr table** per compiland — `(source line, section-1 offset)`
  pairs, sorted by address; and
* an **S_BLOCK32 scope tree** per function — one record per lexical `{ }`,
  each with a start address and a byte count.

Neither says anything about x86 codegen. What they say is *which statement
each run of instructions came from*, which is exactly the question a
reconstruction is answering. Two divergence kinds fall out that no x86-side
lens can see:

* **a statement we split or merged** — three straight-line stores where the
  line table shows a counted loop, four assignments where it shows one
  `memset` call;
* **a call we spelled as a field read** — the DC compiland calls
  `town::HasBuilding` where our body tests `active & bitNumber[id]`.

Both change the /Ob2 candidate-site list (`docs/vc6/inliner.md` §2) even when
they are byte-neutral, so this table is the natural partner of the
site-count probe: **the probe measures how many sites are missing, the line
table says which statement carries them.**

## Reading it

The dump is `../homm3-symbols/HoMM3-Dreamcast-Dump/dump.txt`.

```
  E:\gamedcs\viewarmywindow.cpp, 0001:00190ABC-00192C87, line/addr pairs = 372

     55 00190ABC     57 00190B48     58 00190B54     61 00190B64
     …
    274 0019148C    278 001914F0    282 00191506    283 0019150C
```

`0001:XXXXXXXX` is a **section-1 offset** — the same number our `VA(...)`
comments carry as `dc 0x…` and `evidence/dc-xref-graph.tsv` as
`src_offset`. Two conversions:

| want | from `dc` offset |
|---|---|
| the next line's start (statement size) | the following entry, or the S_GPROC32 `Cb` |
| an **RVA into `../orig/dreamcast/H3.EXE`** | **`+ 0x1000`** (section 1 `.text` is at va 0x1000) |

The scope tree sits in the `S_GPROC32` record for the same address, with the
parameter and local `S_REGREL32` records ahead of it — those name the
**original parameter list**, which is independently useful (they proved
`get_luck_description`'s DC signature had no `creature` parameter at all).

## Disassembling the SH4 side

`capstone` ≥ 5 has an SH backend, so the DC bytes are readable directly and
the line table becomes a statement-by-statement listing:

```python
import struct, capstone
data = open("/home/sheep/Projects/homm3/orig/dreamcast/H3.EXE", "rb").read()
# section 1 (.text) is va 0x1000 / raw 0x400, so rva = dc_offset + 0x1000
md = capstone.Cs(capstone.CS_ARCH_SH, capstone.CS_MODE_SH4 | capstone.CS_MODE_LITTLE_ENDIAN)
for i in md.disasm(data[0x400 + (rva - 0x1000):][:size], rva):
    print("%08x  %-10s %s" % (i.address, i.mnemonic, i.op_str))
```

Two practicalities:

* **literal pools sit inside functions.** SH4 materialises constants and call
  targets with `mov.l @(disp,pc)`, and the pool is emitted mid-body; capstone
  stops at it. Disassemble around the gap rather than assuming the function
  ended (`TViewArmyWindow`'s pool is 0x1925d8–0x192617 and code resumes at
  0x192618).
* **a call is `mov.l <pool>,rN; jsr @rN`** (or `bsr` when it is near), so the
  pool entry is the callee. `evidence/dc-xref-graph.tsv` already resolves
  those to names — join on `src_offset` instead of decoding the pool by hand.
  Note its `pool_refs` column counts **literal-pool references, not calls**:
  three `jsr @r11` in a loop share one entry, so the column is a lower bound.

## The load-bearing caveat: the DC source is an OLDER REVISION

This is what makes the instrument sharp rather than merely corroborative.
`dc 0x19148c` line 284 is `mov.l @r10,r6 / bsr create_portrait_widget` — it
hands the portrait builder `traits->townType` straight through, with **no
version gate and no elemental test**, where retail's x86 body has both. The
same shows up on `armygrp::get_luck_description` (a `creature` parameter and
a whole clover-field arm retail has and the DC build does not) and on
`TBottomViewKingdom` (`town::HasBuilding` calls that retail spells as an
inline 64-bit mask test).

So the two directions are asymmetric, and both are useful:

* **DC has a structure, retail's bytes agree with it** → that is retail's
  source too, and the DC line table is proof of the SPELLING (the
  `for (i = 0; i < 3; i++) Influence[i] = -1;` case).
* **retail's bytes have code DC has no line for** → that code is a later
  edit, and it is exactly where a source-level element retail has and DC does
  not must live. On `TViewArmyWindow(int,int,int,unsigned char)` that
  argument is what identified the one missing /Ob2 candidate site: every
  other statement in retail's body has a DC line carrying the same call, so
  the post-DC version gate was the only place left for it.

Never read a *missing* DC call as evidence that retail has no call there.
`dc 0x19148c` does not reference `operator new` at all even though its body
allocates a `textWidget`.
