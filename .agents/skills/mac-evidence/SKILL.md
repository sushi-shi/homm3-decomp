---
name: mac-evidence
description: Read Classic Mac CodeWarrior code as evidence for HoMM3 source structure that VC6 erases - inline wrapper depth, helper boundaries, guard and branch shape, expression form - and test a spelling with a two-variant CodeWarrior probe. Use when a Mac pair disagrees with source that already matches Windows, or when choosing between spellings that compile identically under VC6.
---

# Mac evidence

Windows bytes remain the verdict (`AGENTS.md`). The Mac PEF is useful
because CodeWarrior keeps source distinctions that VC6 `/O2 /Ob2` folds away.
Read a Mac difference as a hypothesis about the original source, test it, and
keep Windows CUR in view. Use [helper-placement](../helper-placement/SKILL.md)
for where a helper body lives and [match](../match/SKILL.md) for the Windows loop.

## Which distinctions survive

Measured on 2026-10-07 with two-spelling probes. Mac is CodeWarrior at the Mac
profile; Loki is GCC 2.95.2 `-O2 -funroll-loops -fno-exceptions`.

| Source distinction | VC6 SP3/SP5 | Mac | Loki |
| :----------------- | :---------- | :-- | :--- |
| inline member call vs pasted body | erased | kept | kept |
| ordinary static helper vs pasted body | erased | kept | kept |
| split guards vs one joined `\|\|` guard | erased | kept | kept |
| redundant guard before an inline helper that already checks | erased | kept | kept |
| `bool` vs `unsigned char` parameter (simple use) | erased | erased | kept |
| early returns vs nested single exit | kept | kept | erased |
| `bool` local vs direct test | erased | erased | erased |
| assigned vs discarded call result | kept | kept | kept |

How the kept cases look in Mac code:

- **Inlined member.** CodeWarrior evaluates an inlined call once and reuses
  the value across a later call. A pasted body reloads the fields after that
  call.
- **Joined guards.** Split guards each emit their own `li r3,0; b exit`.
  Joined guards share one block, and several failure paths branching to one
  `li r3,0` means one condition or a single final `return 0`.
- **Redundant guard.** A guard the inlined helper repeats shows up as two
  consecutive tests of the same value.
- **Bool argument.** An `unsigned char` value passed to a `bool` parameter is
  normalized with `addic`/`subfe` or `clrlwi`. Retail passing the register
  unchanged means both sides were `bool`. Check the original Dreamcast decorated
  public (`_N` versus `E`, from the `?…Z` strings in `H3.EXE`) before changing
  an interface.

## Inline depth: the leftover call shows how many wrappers there were

CodeWarrior expands inline functions only to a fixed depth and leaves the next
level as a call. The wrapper itself disappears in both spellings. What
identifies the spelling is the call that remains, and it stays the same from
`-O1` to `-O4`.

Example: our `TTextResource::operator[]` calls `GetText`, which indexes a
`std::vector<char*>`.

| Caller spelling | Call left in the Mac code |
| :-------------- | :------------------------ |
| `g_generalText->GetText(i)` | `vector_pod<unsigned long>::data()` |
| `(*g_generalText)[i]` | `vector_pod<unsigned long>::operator[]` (retail 0x2a0c) |

Retail sites call 0x2a0c, so under our header model the original indexed
through `operator[]`. This only measures depth: if the original `GetText`
carried one more wrapper layer than ours, its calls would leave the same call.
State that caveat, and treat a conflict with an `AGENTS.md` rule as a decision
for the user rather than a silent rewrite.

## Probe a spelling

Compile two spellings with CodeWarrior and compare the emitted calls or words.
Do this in a scratch worktree (`homm3 worktree new`); paths must sit inside
`HOMM3_DIR`. Include the real project header so wrapper depth matches the game.

```sh
mkdir -p build/probes && cat > build/probes/p.cpp <<'EOF'
#include "textresource.h"
extern void sinkText(const char*);
void viaGetText(int i) { sinkText(g_generalText->GetText(i)); }
void viaIndex(int i) { sinkText((*g_generalText)[i]); }
EOF
HOMM3_DIR=$PWD PYTHONPATH=$PWD/scripts python3 - -O3 <<'EOF'
import sys
from homm3.mac import cc_wrap
level = sys.argv[1]
cc_wrap.flags_for = lambda unit: (level, "-proc", "750", "-nomapcr", "-nolink", *cc_wrap.MAC_DEFINES)
root = cc_wrap.ROOT
sys.exit(cc_wrap.compile_unit("victorylossconditions", root / "build/probes/p.cpp", root / f"build/probes/p{level}.o"))
EOF
grep -E "HUNK_GLOBAL_CODE|bl " build/probes/p-O3.dis.txt
```

Compare the result with `homm3 mac disasm <Windows-VA>` for the retail
caller. To see which distinctions VC6 keeps, compile the same pair with
`python3 -m homm3.core.cc_wrap` and compare the function bytes.

## Before trusting a Mac score

- **Profile.** `config/mac/units.toml` compiles every unit at `-O3`, which is
  only a working assumption. Retail hero code is unoptimised loop code: no CTR
  loops, and `which+1` is recomputed inside the loop. A per-unit sweep raised
  exact pairs from 588 to 674. Identify a unit's level from its loop
  signatures before reading its differences as source facts.
- **Score.** `homm3 mac build` counts equal bytes at equal offsets, so one
  early extra instruction collapses a pair. Use `homm3 mac shape` (aligned,
  relocations masked) and `homm3 mac diff` to judge a change.
- **Unavailable pairs.** "Unresolved references" is a linking gap, not a
  source verdict; `shape` still works.
- **Frame offset.** Many pairs keep every local about 0x18 higher, with a frame
  0x10 larger, even after the control flow matches. The cause is still open;
  do not chase it per function.

## Other source facts Mac has shown

- `ZeroMemory`/`CopyMemory` (retail `bzero` / `BlockMoveData`) where our source
  calls `memset`/`memcpy`.
- CodeWarrior places the vptr where the first virtual function is declared, so
  member order relative to the virtuals matters.
- 44 pairs contain Mac byte-swap reads and writes; `platform.h` has no
  byte-swapping store macro yet.
- Expression shape: `?:` versus arithmetic on a comparison, index formulas,
  separate locals, and operand order (which also drives register numbering).
- In `checkForArtifactWin` (0x5f1610) these facts took Mac from 41% to 97%:
  a discarded `erase` result, a removed redundant guard, joined guards, a
  nested `if (comboIdx != -1)` block and direct calls. Windows CUR held for
  all of them except the last.
