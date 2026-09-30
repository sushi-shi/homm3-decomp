# Native Cove town

Cove is town 9, compiled into the ordinary game with no feature switch. Its
creatures, Captain/Navigator classes, twenty heroes, buildings, recruitment,
town selection, random-map templates, graphics and combat rules use the native
game systems. Sea Dogs share the Pirate population and require the Gunpowder
Warehouse. Legacy factions keep their IDs; legacy H3M hero fields retain their
original lengths. New saves use version 43 and require this executable.

The source is the [VCMI HotA port](https://github.com/vcmi-mods/horn-of-the-abyss/tree/40b6b9f317e40c3e928cf1add0bb11fcc9556a4d),
pinned to `40b6b9f317e40c3e928cf1add0bb11fcc9556a4d`. These are public faction
definitions and resources, not HotA's original C++ implementation. The native
integration is authored here. `definition.json` records the input hashes and
attribution; `scripts/homm3/expansions/cove.py` generates `src/cove_data.inc`.

Assets and definitions were created by HotA Crew; the port credits VCMI Team,
edeksumo, avatar, Loki Laufeyjarson, Ben and Karyoplasma. Upstream distributes
them under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).
The adapted definitions, generated data and converted Cove assets retain that
license. Changes include selecting Cove/Cannon, normalizing JSON, assigning
native IDs, and converting resources to native LOD/bitmap/interface layouts.
Original Complete assets are read from the user's installation and are not
included in the repository. Conversion output stays under ignored `build/`.

## Build and package

Use the repository's build environment and a licensed Complete installation.
Run from the worktree root:

```sh
git clone https://github.com/vcmi-mods/horn-of-the-abyss build/hota-reference
git -C build/hota-reference checkout 40b6b9f317e40c3e928cf1add0bb11fcc9556a4d
PYTHONPATH=scripts python3 -m homm3.expansions.cove
ninja candidate
```

Prepare a separate `build/cove-runtime` with the installation's DLLs, `Maps`,
`mp3`, and base `Data` archives. Keep its `Data` and `mp3` directories writable
and separate from the original installation; individual base files may be
symlinked. With Pillow available:

```sh
PYTHONPATH=scripts python3 -m homm3.expansions.cove_assets \
  --upstream build/hota-reference --base-data /path/to/original/Data \
  --output build/cove-runtime/Data
cp build/exe/HEROES3.candidate.EXE build/cove-runtime/Heroes3.exe
bash extensions/cove/run.sh
```

The converter verifies pinned definition hashes and writes `cove.lod`, loose
creature sounds and `mp3/CoveTown.mp3`. The resource manager loads that archive
before the original archives, including its extended interface sheets and
object templates. It is required for this build. Original resource frames are
preserved, and appended frames use distinct global cache names.

Select Cove in a scenario's Advanced Options, then begin the game. Maps that
permit random alignment can select it. Existing HotA-format maps are not
supported; this change retains the Complete map reader.

## Validation and limits

```sh
PYTHONPATH=scripts python3 -m unittest homm3.expansions.test_cove_assets
PYTHONPATH=scripts python3 -m homm3.expansions.cove
homm3 build
```

The executable was started under Wine with installed Complete assets, Cove
selected, a scenario loaded and the Cove town entered. The resource tests
cover old-frame preservation, offsets, global frame names and bitmap encoding.
This is a native town integration, not full HotA engine compatibility: Cannon
currently uses native Ballista targeting and cannot fire at siege walls.
Campaign transfer of new creatures/artifacts and multiplayer interoperability
have not been validated. The changed game layouts intentionally diverge from
the pinned retail binary; retail matching scores are not a feature pass/fail
criterion.
