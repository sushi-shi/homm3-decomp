# HoMM2 ↔ HoMM3 symbol overlap

**Verdict: HoMM3 is a direct source-level descendant of HoMM2's engine.**
More than half of homm2's compilands recur by name in HoMM3, the NWC
`basewin.lib` UI framework and the game-core class roster carry over
nearly wholesale, and 611 functions share their exact (normalized) name
across the two games — 461 of them backed by homm2-decomp's
**exact-matched reconstructed source**. The reuse is source-level with
heavy evolution, NOT byte-level: cross-compiler/cross-era code identity
is infeasible (measured below), but homm2's matched C++ is a ready
translation-template library for homm3 matching, and rare-string anchors
name 50 of our still-unnamed functions.

Measured 2026-08-04 by `homm3.analysis.homm2_overlap` (one-shot; retired
to `scripts/archive/` — the generated tables live in
`evidence/homm2-overlap/`, regenerate from the archived script if ever
needed). Inputs: our pinned HEROES3.EXE (sha256 `057c9d88…`) + symbol
db + Dreamcast/NH3API corpora; the sibling homm2-decomp read-only
(HEROES2W.EXE sha256 `bc8f362d…`, MSVC 4.2 `/Gr`, CodeView-NB09
authoritative names, 1,154/1,514 functions exact).

## Lane results

| Lane | Result |
| :-- | :-- |
| TU stems | **53 of homm2's 95 units** recur as HoMM3 DC compilands (advmgr, army, armygrp, findpath, fly, game, hero, recruit, soundmgr, strip, swapmgr, townmgr, window, …) |
| Classes | **43 exact class-name matches**: `advManager`, `army`, `armyGroup`, `baseManager`, `border`, `button`, `combatManager`, `executive`, `font`, `game`, `hero`, `heroWindow`, `heroWindowManager`, `hexcell`, `mouseManager`, `resourceManager`, `soundManager`, `town`, `widget`, … (the whole basewin family + the game core) |
| Function names | **611 normalized-name pairs**; **466 with homm2 fuzzy = 100.0** (exact source exists); **144 with a known HoMM3 retail RVA** (via the DC name map) |
| String anchors | **57 rare-literal anchors** (literal referenced by ≤3 fns on each side); 50 land on `working-label` (unnamed) HoMM3 functions |
| Boost list | **198 rows** in `boost.csv`: 50 `string-anchor` + 148 `h2-source-template` |

## The two boosts

**1. Names for unnamed functions (`string-anchor`, 50 rows).** A shared
rare literal pins a fn↔fn pair with no name needed on our side. Flagship
example: HoMM3 `game_smove_82m_3d9f0` / `game_skill_82m_166c20`
(working labels) share the creature-animation resource format strings
`%sattk.82M` / `%smove.82M` / `%skill.82M` / … with homm2's
`army::LoadResources` (`SOURCE/ARMY`, 100% exact) — those HoMM3
functions are the army/creature resource loaders, and the `.82M`
extension surviving into HoMM3 is lineage evidence by itself.

**2. Source templates for matching (`h2-source-template`, 148 rows).**
HoMM3 functions we already NAME (DC evidence) whose homm2 twin is
exact-matched — homm2's C++ is the starting body for our
reconstruction. Examples: `combatManager::ResetLimitCreature`,
`combatManager::UpdateGrid`, `combatManager::ViewArmy`,
`Process1WindowsMessage` — each with homm2 fuzzy 100.0. Whole units at
homm2-100% with many HoMM3 counterparts: `BASE/WINDOW`,
`SOURCE/SWAPMGR`, `SOURCE/STRIP`, `BASE/soundmgr`, `SOURCE/RECRUIT`,
`SOURCE/ARMYGRP`.

## Calibration: why there is no byte-identity lane

20 name-paired functions compared across MSVC 4.2 → 6.0 and three years
of engine evolution: size ratios span **0.26x–2.10x**, call counts
drift freely. One striking outlier — `combatManager::GetCommand` is
1306 vs 1322 bytes with 11 vs 11 direct calls — but it is the
exception. Conclusion: do NOT build a masked-byte transfer lane against
homm2 (unlike the HD-Mod pressing, which is the same build); the value
is names via anchors and source via templates.

## Known false-positive mode

The name lane's negative control caught exactly one collision class:
homm2's `BASE/Bzip` exports the generic names `compress`/`uncompress`,
which pair with our **zlib** `compress.obj`/`uncompr.obj` — different
libraries, same generic C names. Units/anchors lanes are clean on all
negative controls (Bzip, Modem, Netbios). Treat short generic free-names
as untrusted without a second signal.

## Campaign status (2026-08-06)

Of the 466 exact-template pairs, **167 are located** and **299 remain
DC-attested but unlocated**. "Located" means the retail address is
proven either by `evidence/retail-dc-name-map.csv` (144) or by a live
`VA()` claim in `src/` (the rest) - the generator only knows the first
source, so its own `h3_retail_rva` column undercounts; measure against
both.

The bulk-join shortcut does NOT exist: joining through
`evidence/retail-game-tree.csv` adds exactly zero. What does work is
**link-order bracketing** (`homm3.analysis.dc_bracket`, decision log
2026-08-06): inside a TU the DC and retail sequences are the same
sequence, so proven addresses cut it into gaps, and an equal-count gap
forces the mapping. That yielded 37 locations, 21 promoted to claims
here. The gaps bracketing cannot decide go to the **call-graph lane**
(`homm3.analysis.dc_callgraph`): R(F) must be among the retail callees
of R(F's proven DC callers), which yields 24 more unique locations
under an injective-monotone soundness check. Both lanes proved
themselves end to end - `inputManager::Close`/`::Main` and
`heroWindowManager::RemoveWindow` were located by them and then
matched.

## Admission path (nothing admitted by this report)

Evidence grade for everything here: **external-candidate** (homm2 is
another game's build; no HoMM3 retail-byte proof). Recommended use, per
the supervised-review rule:

1. `string-anchor` rows → propose as *naming evidence* for the labeled
   functions (each needs a supervised look at both bodies; the anchor
   literal is the argument).
2. `h2-source-template` rows → when the owning TU comes up for
   reconstruction (P5.2+), start the body from homm2's matched source
   and adapt; record the template's origin in the TU's review.
3. The 43 shared classes → corroborating input for the class-modeling
   phase (a third oracle beside Dreamcast layouts and NH3API types —
   homm2's layouts are VC 4.2-proven and will differ, but member
   *rosters* and method sets transfer).
