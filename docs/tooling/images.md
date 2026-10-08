# Images: more than one linked program

The repository reconstructs the game, `HEROES3.EXE`, and the programs built
from the same source tree. Each program is an **image**. The game is the
default image `game`. Another image is a pin in `config/project.toml` with
`image = true`; its key selects it on the command line:

```sh
homm3 --image h3maped census --write
homm3 --image h3maped placements --write
homm3 --image h3maped delink
```

`--image KEY` comes before the subcommand. It sets `$HOMM3_IMAGE`, which every
child process inherits (`homm3.core.images`). Without it every command works
on the game exactly as before.

| Image | Executable | Compiler profile |
| --- | --- | --- |
| `game` | GOG Complete 4.0 `HEROES3.EXE` | `/O2 /Ob2 /Oy- /Op /MT /Gr /GX` |
| `h3maped` | GOG Complete 4.0 `h3maped.exe` (map editor) | `/O1 /Ob2 /GR` for its own objects; `/O2 /GR` for the prebuilt Libraries archive; zlib and Victor reuse the game's objects |

## Where an image keeps its state

| What | Game | Another image |
| --- | --- | --- |
| Retail tables | `config/retail/` | `config/retail/<image>/` |
| Units and profiles | `config/units.toml` | `config/units.<image>.toml` |
| Score ledger | `config/match_baseline.tsv` | `config/match_baseline.<image>.tsv` |
| Generated state | `build/{gen,objdiff,delink,pdb}` | `build/<image>/{gen,objdiff,delink,pdb}` |
| Ninja graph | `build.ninja` | `build/<image>/build.ninja` |
| README block | `match-score` | `<image>-match-score` |

The toolchain, the Wine prefix, the staged executables under `build/orig/`
and the Dreamcast and Mac evidence are shared. `homm3.core.images.path()`
maps a game-relative per-image path (`build/gen/...`, `config/retail/...`,
the ledger, the manifest) to the selected image's spelling; for the game it
returns the path unchanged, so the game's files, scores and gates do not move.

## The census of another image

The game's census was carved once and is hand-owned. Another image derives
its census from its pinned executable (`homm3.census`):

- `functions.tsv`: function starts from recursive descent over seeds of
  decreasing strength (entry, EH funclets, call and tail targets, the code
  after a zero-displacement `jmp`, runs of code pointers in data, immediate
  code addresses, isolated data pointers, then the code after each decoded
  extent). Extents partition `.text` to the next start minus padding. A C++
  EH registration stub (`mov eax, offset FuncInfo; jmp ___CxxFrameHandler`)
  is never a start: as in the game's census it closes its parent's
  `.text$x` group, so the parent's `offset stub` reads as the last funclet
  plus its size, the form the comparison canonicalizes (also for the
  editor's `/O1` prologue `mov eax, offset stub; call __EH_prolog`).
- `vtables.tsv`: RTTI vtables (the dword before each is its Complete Object
  Locator, which names the class) and tables that code stores as a vptr.
- `relocs.tsv` and `reloc-evidence.tsv`: the vendored `find_relocs` channels.
  A data dword pointing into a function's interior is dropped.
- `funclets.tsv` and `init-thunks.tsv`: EH funclets with their parent
  functions (the function whose `__ehhandler` stub names their FuncInfo),
  and the `.CRT$XCU` initializer table (the larger table the CRT hands to
  `_initterm`). An `/O1` slot holds a `jmp $+5`, so the initializer body
  after it joins the table, and so do the cleanups the initializers
  register with `_atexit` (slot `-`): like the game's, they are
  compiler-generated and outside the scores.
- `runtime-map.tsv`: statically linked library functions, each the unique
  masked match of a LIBCMT, LIBCPMT or SP3 NAFXCW member function. They are
  named, not matched, and excluded from the scores.

`homm3 --image KEY census --check` fails when the committed tables no longer
match the derivation.

## Shared units and placements

A unit that the game also compiles is the same source file. Its `VA()`
claims spell game addresses, so another image never reads them. The image
compiles the file with its own profile and **places** its functions
(`homm3.census.placements`, `config/retail/<image>/placements.tsv`):

1. a compiled body that equals exactly one census function of the same size,
   relocation fields masked;
2. the callees and referents named by the relocations of each placed body,
   read from the retail bytes (where the layouts differ, the absolute
   operands pair by order when both bodies have the same number). A shared
   unit's `$E` initializers and static destructors take the game's
   compiler-function names (`__h3cg$...`), which the image's comparison
   claims unsized;
3. the slots of each RTTI-named vtable;
4. string literals: a `??_C@` literal whose bytes sit at exactly one retail
   address that code references anchors the compiled functions that use it.
   A function still unplaced is placed where retail references all of its
   anchored literals together, when that is one unclaimed census function;
5. masked prefixes: a function still unplaced whose masked bytes agree with
   the start of exactly one unclaimed census function for 32 fixed bytes,
   8 more than any other start, with a size within a factor of two (a body
   whose tail the image's compile changes).

Steps 2 and 3 follow every other step. A placed body names a referent only
where the opcode byte before its relocation field agrees with retail, so a
body that differs elsewhere still names its callees where it agrees. Steps 4
and 5 never contradict an earlier placement. A name that reaches two
addresses, or an address that receives two names, is dropped, except an
`/OPT:ICF` fold: when the names that reach only that address have compiled
bodies that agree byte for byte, the first of them places it, and the
comparison names the others' references after it (`vector<T*>::push_back`
for every pointer `T`). Names keep
their checkout-independent anonymous-namespace spelling. The label model
reads the table as the image's claims (channel `placement`).

Sources only the image compiles spell the image's own addresses in
`VA()`/`DATA()` and are extracted like game sources. They live in the
image's source directory, the pin's `sources` key (`editor` for h3maped, the
original's `Editor\` directory): `src/editor/` and `include/editor/`. The
game's claim, ownership, cleanliness and accounting scans skip those trees
(`homm3.core.images.foreign`); the image's scans read only them.

## The SP3 MFC overlay

The shared toolchain carries the RTM MFC (`mfc/LIB/NAFXCW.LIB`, members
stamped C++ 8168). The editors link SP3's library (members stamped 8447) and
compile against SP3's changed headers. `homm3 --image h3maped init --mfc-sp3
DIR` verifies the pinned files of the extracted `vc98/mfc` from VS6 SP3
(`config/project.toml [toolchain.mfc_sp3.files]`, the TechNet disc the
toolchain release already uses) and copies them to `build/mfc-sp3/`. The
image's units put `build/mfc-sp3/include` ahead of the toolchain's MFC headers,
and the census names MFC code from `build/mfc-sp3/lib/nafxcw.lib`.

## Building an image

A full `homm3 build` builds the game, then every other image whose
executable and SP3 MFC overlay are staged, each in its own process; a
failed image fails the build. `homm3 --image h3maped build [--fast TU]`
builds one image alone: configure, ninja, the placements check, delink,
report, ledger, the banked-rows and VA-claim gates and its README block.
The game's `--fast` loop never builds an image. The steps by hand:

```sh
homm3 --image h3maped init --exe /path/to/h3maped.exe --mfc-sp3 /path/to/vc98/mfc
homm3 --image h3maped census --write        # after a census-tool change
homm3 --image h3maped configure
ninja -f build/h3maped/build.ninja -j4 objects
homm3 --image h3maped placements --write    # after a compile of shared units
homm3 --image h3maped delink
homm3 --image h3maped status update --write-readme
```

The game's commands, ledger, README block and gates are unchanged by any of
these.
