# Loki h3maped image (GCC 2.95.2, ELF i386)

The Loki Linux map editor 1.0 (`h3maped`, 4,970,572 bytes, SHA-256
`0d5614c4…2138559d`, pinned as `[inputs.loki_h3maped]` in
`config/project.toml`) is the first map-editor image. It is Loki's GTK+ port of
the RoE-era editor source. Its retail bytes are the verdict for this image;
the game's verdict stays `HEROES3.EXE`. Nothing here changes `homm3 build`,
the game ledger or the README score block.

## Setup

```sh
homm3 loki init --exe /path/to/h3maped --debs DIR --sgi-stl DIR --binutils DIR --gcc DIR --gtk DIR
homm3 loki census --check      # retail facts are current
homm3 loki build -v            # compile, delink, canonicalize, objdiff
homm3 loki disasm _getC__13TGzInflateBuf   # references named as compared
homm3 loki diff Error __11TDebugBreak      # one function, base | retail
```

`--debs` holds the eight Debian 2.2 "potato" i386 packages pinned in
`config/loki/toolchain.toml` (gcc, g++, cpp 2.95.2-13.1; binutils
2.9.5.0.37-1; libc6 and libc6-dev 2.1.3-20; libstdc++2.10 and -dev).
`--sgi-stl` holds SGI STL 3.2's `stl32.tar.gz` (members dated 1999-04-23).
`--binutils` holds Slackware 7.1's `binutils.tgz` (2.9.1.0.25), whose `as`
replaces potato's: Loki's objects never use the byte `moffs` encodings
(`a0`/`a2`); all 29 absolute byte loads and stores are `8a 05`/`88 05`,
which as 2.9.1.0.25 emits and 2.9.5.0.37 does not. `--gcc` holds Slackware
7.1's contrib `gcc.tgz`, the unpatched 2.95.2 release, whose `cc1` and
`cc1plus` replace Debian's 2.95.2-13.1 (a 20000220 branch snapshot): the
release compiler emits a class's in-class inline members as strong `.text`
globals in the unit that defines its vtable, as Loki's objects show
(`resource::AddRef` and the other accessors in `resource.o`), where Debian's
emits weak linkonce copies. Debian's driver and `cpp` stay. `--gtk` holds
Slackware 7.1's `gtkglib.tgz`, whose GTK+ 1.2.8 and GLib 1.2.8 headers are
the versions the image links statically: its `gtk_major/minor/micro_version`
read 1.2.8 with `gtk_binary_age` 8 and `gtk_interface_age` 3, and
`glib_*_version` 1.2.8 (potato ships 1.2.7). The environment
variables `HOMM3_LOKI_H3MAPED`, `HOMM3_LOKI_DEBS`, `HOMM3_LOKI_SGI_STL`,
`HOMM3_LOKI_BINUTILS`, `HOMM3_LOKI_GCC`, `HOMM3_LOKI_GTK` and `HOMM3_LOKI_LIBS` (see Link) work as well. Everything is staged under ignored
`build/`: the image at `build/orig/loki/h3maped` and the toolchain at
`build/loki/toolchain/`.

The 2000-era binaries run unmodified. Each program the driver spawns (`cpp`,
`cc1plus`, `as`) is a small wrapper that starts it through the packaged
`ld-2.1.3.so` with `--library-path`, and the driver runs under a clean
environment, so the Nix `COMPILER_PATH` and `LD_LIBRARY_PATH` cannot leak in.
The 2.95 driver prepends each `-B` prefix, so the wrapper directory is given
last. Neither Wine nor patchelf is involved.

## Compiler and flags

| Fact | Evidence |
| :--- | :------- |
| GCC 2.95.2 release | `.comment`: 146 objects `GCC: (GNU) 2.95.2 19991024 (release)`. The release `cc1plus` (Slackware 7.1 contrib) is staged; Debian's 2.95.2-13.1 (`20000220`) differs in where it emits in-class inline members. |
| `-O0` | Out-of-line accessors, `jmp` to the next instruction, `mov %eax,%eax` after calls, locals reloaded from the frame. A sweep over 12 game units paired by mangled name gives 0 exact at `-O1 -fno-inline` and `-O2`. |
| `-mcpu=pentiumpro` | Epilogues are `mov %ebp,%esp; pop %ebp`, not `leave` (the i386 default emits `leave`, so 0 functions match). `Bitmap16Bit::Bitmap16Bit(int,int)` sign-extends with `cltd` only under pentiumpro. The Loki compiler was i686-configured, whose default this is. |
| exceptions and RTTI on | `.eh_frame` (21,301 FDEs) and `.gcc_except_table`; `-fno-exceptions` loses half of the exact bodies (419 to 210). |
| non-PIC, vtable thunks | Absolute addressing; `virtual function thunk` symbols. |
| `-g` | Undecidable from code (stripped image); irrelevant to bytes. |
| SGI STL 3.2 headers first | `string` is `basic_string<char, char_traits<char>, allocator<char> >` with `_String_base`; `runtime_error` objects are 0x104 bytes (`__Named_exception`'s 256-byte buffer); `vector<T>::_M_fill_insert` instantiations are exact. 3.2, not 3.3: the per-object static `__get_c_string` is non-inline (3.3's inline copy loads its parameter into `%ebx`) and `fill_n<char*, unsigned, char>` is the generic template (3.3 adds char overloads). libstdc++ 2.95's own iostream (libio) stays. |

`config/loki/units.toml` records the profile: `-O0 -mcpu=pentiumpro
-fpermissive`, `-DHOMM3_TARGET_LOKI=1` and `-include include/gcc_prefix.h`.
`-fpermissive` only admits the VC6 dialect of the shared source.
`include/gcc_prefix.h` is the GCC counterpart of `codewarrior_prefix.h`: it
spells the Microsoft keywords. VC6 never sees it. No Windows SDK header is on
the Loki include path: the port defines the few Windows names it spells
itself (`UINT` and `IDOK` in the editor's MFC shim, `stdafx.h`; `tagRGBQUAD`
in `palette.cpp`).

## Census

`homm3 loki census` writes `config/retail/h3maped-loki/`:

- `objects.tsv`: one row per compiled object. g++ 2.95 emits one `.eh_frame`
  CIE per object and one FDE per function, so CIE order is link order and FDEs
  give exact starts and sizes, including file-static functions. 103 project
  objects (0–102) precede the frameless C libraries (GTK+ 1.2, gdk, glib,
  libglade, libxml, zlib; egcs 1.1.2), then 19 libstdc++/libgcc objects.
  86 objects are named by their `__FILE__` strings (assert/`__assert_fail`
  arguments), one by its anonymous namespace, the rest by class and marked
  "file name inferred".
- `functions.tsv`: 10,754 starts with object, placement, binding, GNU v2
  mangled name and the era's `c++filt` demangling. `linkonce` marks kept
  `.gnu.linkonce.t` copies (templates and inline members), which the default
  linker script places after every object's `.text`; their owner is the first
  object that defined them. Static init/fini functions are named by role from
  `.ctors`/`.dtors`.

Project code: 3,120 `.text` functions (421 file-static) and 4,312 kept linkonce
functions, plus 364 frameless virtual-function thunks.

## Comparison

The linked ELF has no relocations. `homm3.loki.delink` decodes every function
of an object and names each rel32 branch and absolute address through the
census, the PLT (`.rel.plt`) and the exported symbols. The compiled object goes
through the same canonical form (`homm3.loki.cmpobj`) before objdiff sees it:

- every call or jump that leaves its function and every absolute address is a
  relocation against a named symbol, with the addend in the field. GAS resolves
  calls to file-static functions in the same section; the canonical form
  relocates them too;
- a string or constant in `.rodata` is named by its content: the bytes a
  memory operand loads, or the C string whose address is taken. Literal
  identity is therefore checked independently of the object's `.rodata`
  layout;
- a jump table is `<function>$jt<n>`;
- `_GLOBAL_.N.<file><6 random chars>` anonymous-namespace components become
  `5_ANON` (append_random_chars draws on gettimeofday and the pid), and
  `_GLOBAL_.I.*`/`_GLOBAL_.D.*` keep their role.

Nothing is masked: an unnamed reference keeps a distinct name and differs.
The image has no names for file-static functions. g++ 2.95 at `-O0` emits an
object's `.text` in source order, so `homm3.loki.delink.pair_statics` uses the
functions both sides name as anchors and gives the k-th retail static between
two anchors the name of the k-th compiled local function there, when both gaps
hold the same number. Other statics stay `sub_<address>` and unpaired.
Function-local statics (`<name>.<uid>`, their `_.tmp_<n>` guards and the
`__tcf_<n>` cleanups registered with `atexit`) belong to one function:
`homm3.loki.delink.pair_locals` aligns that function's two relocation lists,
unnamed retail references and compiled file-local data standing as
placeholders, and pairs the k-th of an aligned run with the k-th, when every
aligned use agrees one to one. A pointer table in `.rodata` is named by its
entries (`$P<n>[...]#digest`); an entry that points to file-static data is
`data_<address>` in the image and the object it falls in on the compiled
side, and `pair_data` pairs those entries position by position, like the
function's own `data_` references, then renames the table from them
(Town.cpp's generator tables).

Units compile from their source's directory with the bare file name, as Loki
did: `__FILE__` in assert text and `TRuntimeError(__FILE__, __LINE__, ...)`
sites is `"GzBuf.cpp"`, never a path.

## Data comparison

`homm3 loki build` also compares every built object's data with its slice
of the image (`homm3.loki.datacmp`). GNU ld 2.9.5's default script
concatenates input sections in link order: `.rodata` holds each object's
`.rodata` and then the kept `.gnu.linkonce.r*`, `.data` each `.data` and
then the kept `.gnu.linkonce.d*` (vtables, SGI allocator statics), `.bss`
the copy-relocated `.dynbss`, each `.bss` and then COMMON (g++ 2.95's
`__ti` type_info nodes); `.gcc_except_table`, `.ctors` and `.dtors` follow
link order too.

- A data section's image base is voted by the absolute fields that relocate
  against it from code and frames whose place is known (object `.text`,
  kept linkonce bodies, the object's `.eh_frame` at its CIE) and by the
  exported symbols it defines. A section nothing refers to is found by its
  resolved bytes (`.ctors`), or follows the previous object's, aligned.
- A slice runs from the object's base to the next object's base; the
  compiled bytes are zero-padded to it, as ld pads. Missing or extra data,
  order and alignment therefore show as differing bytes.
- A relocated field matches when the image word is the address of the same
  target: the paired symbol, or the same offset of the object's own
  section. A discarded linkonce section sits at address 0 (ld 2.9.5), so
  exception ranges into it keep their bare offsets, as in the image.
- A `.bss` byte matches while every code reference into its symbol votes
  for the section's base. Kept linkonce data is compared in the first
  project object that defines it; a COMMON symbol must exist with the same
  size. Jump tables count with their functions and are left out.
- The type name of a class in an anonymous namespace spells the
  namespace's six random characters; where both sides spell the same
  `_GLOBAL_.N.<file>` at the same place, they compare as the symbols do.

Each unit reports matching data bytes over the image's bytes, and `-v`
lists the differing symbols and anonymous gaps (`.rodata+0x40`). `--bank`
records one `$data` row per unit in the ledger.

Most remaining data differences come from what each object emitted: header
inlines that are never called still leave their assert and type-name
strings, and a polymorphic class without an out-of-line virtual function
gets a `__tf` type_info function wherever it is used. `homm3 loki
emitorder [UNIT ...]` compares each built object's `.eh_frame` function
list, in emission order, with the image's frame block of that object. ld
keeps a frame block whole, so a linkonce copy that another object's copy
displaced still shows as an FDE at address 0 with its size: exported
functions compare by name, discarded slots and file-static functions by
size. Naming units (or `-v`) prints the aligned lists, image-only lines
`-`, ours-only `+`.

An object's `.rodata` keeps its strings in parse order, so two string
sequences follow the original source too. `homm3 loki emitorder --headers`
compares the `__FILE__` names of the assert and `TRuntimeError` sites (the
include order); `--types` compares the g++ type names of the `__ti` nodes
(the order in which classes are defined and first used). The image's
sequence runs from the end of the previous object's compiled `.rodata` to
the next object's base, so a string before the voted base still counts.

## Emission order (g++ 2.95.2)

What an object emits, and in which order, decides most of its data:
`.rodata` strings, type names and `.gcc_except_table` entries are written
with the functions. The rules below come from `cp/decl2.c`
(`finish_file`, `mark_inline_for_output`), `cp/pt.c` (`instantiate_decl`,
`instantiate_pending_templates`), `cp/rtti.c` and `toplev.c`
(`wrapup_global_declarations`), and were checked with a cc1plus that logs
the queue, every emitted function and every string constant.

1. An out-of-line function is written as it is parsed, in source order.
2. Every inline function, implicitly declared member and `__tf` type_info
   function enters one queue (`saved_inlines`) and is written only at the
   end of the file:
   - a non-template inline enters when its body is compiled: a free inline
     at its definition, in-class bodies at the end of the *outermost* class
     in textual order (so a nested class's bodies wait for the enclosing
     class; a nested class defined outside, `class A::B { ... };`, has its
     bodies queued after A's);
   - the implicit members (copy constructor, `operator=`, destructor,
     default constructor) enter when their class is completed, before its
     in-class bodies, whether or not they are ever used;
   - an inline template instance used inside a function body is
     instantiated on the spot, depth first: its callees enter before it,
     and before the function that used it;
   - a polymorphic class's `__tf` enters when the class is completed (its
     vtable initializer names it, which also marks it used); a
     non-polymorphic class's `__tf` enters at its first use (a throw).
3. A template instance that is not inline (out-of-class SGI members such as
   `_S_chunk_alloc`), or an inline one first used outside a function body
   (a default argument), goes to the pending list in first-use order. At
   the end of the file the pending list is instantiated first, and such a
   non-inline instance is written at once.
4. Then the used artificial functions are synthesized in queue order: every
   `__tf` of a class defined in the object whose vtable is emitted here is
   written now, and its type-name string with it. The type names in
   `.rodata` therefore follow class completion order, which is what
   `emitorder --types` compares.
5. The queue is written in passes. Each pass walks it in order and writes
   every function some written function already refers to, including
   those written earlier in the same pass: a callee queued after its
   caller follows it in the same pass, one queued before waits for the
   next pass. The `.text`/FDE order is pass 1 in queue order, then pass 2,
   and so on; new instantiations made while writing go to the end of the
   queue and of the next round.
6. A queued inline that nothing refers to is never written, but its body
   was still compiled when it entered the queue: its string constants are
   already in `.rodata` and everything it instantiated is already queued.
   An uncalled header inline therefore moves every later queue entry.

Applied:

- The image's type names give the class definition order and therefore
  the include order where the assert file names do not (a header without
  asserts leaves no file name). Most fixes are a header that included what
  it only needed to forward-declare, a source that included its own header
  after the object headers, or a nested class the original defined after
  its enclosing class (`TGameMap::TLayer`, `TLayer::TCell`, the map
  windows' controllers).
- A function-local static constant is written when its declaration is
  compiled (`cp_finish_decl`), a namespace-scope `const` with internal
  linkage only at the end of the file. A table the image places between
  two functions' strings is therefore a local static of the later function
  (LinePlacement's and TerrainPlacement's `akFlippedDir`). An 8-byte local
  static constant (a non-BLKmode one) is first built in `%eax:%edx` and
  written to `.rodata` only when its address is taken.
- Namespace-scope `.bss` is written at the end of the file namespace by
  namespace, the first-created namespace last: MapEditorText.cpp opens its
  anonymous namespace before including MapEditorText.h, so
  akGeneralStringImp follows every `S...Text` namespace's akStringImp.
- A namespace-scope non-const variable is written at its definition, its
  string literals with it (GUIGameObject's unreferenced check-mark rows,
  SpellDefs' akSpellEffectTraits after aSpellTraitsImp's sounds).
- GameMap.h includes exceptions.h and RefCountingPtr.h after
  VictoryCondition.h, so every GameMap.h includer names TRuntimeError after
  the condition classes; units that name it right after `<stdexcept>`'s
  (Event, Hero) reach exceptions.h first.
- Namespace-scope constants with internal linkage are written at the end of
  the file in declaration order, header by header: an object's `.rodata`
  ends with them, so the tail proves which headers define constants (and in
  which include order) and which names are enumerators instead
  (`kNumPlayers`, `kNumCreatureTypesPerTown` is a constant). The map record
  readers' reserved byte counts are such file constants.
- Inline SGI helpers that are not templates (`destroy(char*, char*)`,
  `uninitialized_copy(const char*, const char*, char*)`, `min<unsigned>`
  through `lexicographical_compare`) and the `_Base_bitset<1>` and
  `allocator<char>` specializations' members enter the queue where their
  header is first parsed: whether `<string>` (and with it `<stdexcept>`)
  comes before `<algorithm>`, `<vector>` (`stl_range_errors.h`) or `<bitset>`
  is visible in every object's emission order.
- The class members follow the original declaration order: the map
  objects declare their setters (and non-const accessors) before their
  getters and their accessors before their inline virtuals; inline members
  defined after a class (TObjectType's setters) or nested classes defined
  outside it (TSeersHut::TReward) are queued after the class's own bodies.
- A namespace-scope template use outside a function body (an array
  initializer calling `vector::operator[]`) and the non-inline members a
  function instantiates go to the pending list in first-use order; the
  image's pass-0 template order therefore fixes the order of such
  definitions in the file (Hero.cpp's TCopiedProto before the class traits
  table).
- `build/` tooling note: an instrumented cc1plus (the release source with
  `fprintf` at mark_inline_for_output, instantiate_decl, rest_of_compilation,
  assemble_start_function and output_constant_def_contents, configured
  i686-pc-linux-gnu with the gas alignment features) produces the same
  objects and logs the queue, the instantiations, the passes and every
  string constant. Configure does not detect the alignment features with
  the staged tools: define `HAVE_GAS_MAX_SKIP_P2ALIGN` and
  `HAVE_GAS_BALIGN_AND_P2ALIGN` in `gcc/auto-host.h` before `make cc1plus`,
  or labels lose their `.p2align 4,,7` and the objects differ. Logging the
  return-value state at `finish_function` (decl.c, next to
  `no_return_label`) shows which bodies get the two extra jumps.
- Non-inline templates (SGI's `lexicographical_compare`, the allocator's
  `_S_chunk_alloc`) are instantiated at the end of the file in first-use
  order, after the last out-of-line body: their code takes the return state
  that body left (GzBuf's 100- and 132-byte `lexicographical_compare` slots
  need `underflow` last, so `_mustGetC`/`_mustGetLong` are `inline` members
  defined before `_getC`; they still come out strong in `.text`, queued at
  their definition, with `this` reloaded from the frame).
- A class template's static member functions are instantiated at the end
  of the file; an explicit specialization's in-class bodies are queued where
  it is parsed. `TDigits<9>` ends the recursion with `N < 10 ? 1 : ...`,
  not a specialization.
- The first header that parses `<vector>` (its range-error helper builds a
  `string`) instantiates the string and allocator helpers; units place
  `<vector>`, `<memory>`, `terrain.h` (bitset<10> tables) and their own
  header relative to it as the image's first-use order shows (FindDlg,
  RiverPlacement, RoadPlacement, TerrainPlacement, Colors, cppbridge).
- An implicit member is synthesized at the end of the file; a user-declared
  in-class one is queued with the class's bodies (`TRumor()`, the GUI
  specialized objects' inline virtual destructors, written between `edit`
  and `clone` as the vtable instantiates them).
- Line directives order a class's parts: `_TImpl::getNextObjectID`'s assert
  (line 5375) precedes `_TObjectLink`'s (5435 on), so TLayer::_TImpl
  declares its interface before its nested classes; TCell likewise defines
  its shared vector holders after its accessors, and TPlayerInfo and TCell
  declare their setters before their getters.
- exceptions.h's never-called `TRuntimeError()` (`runtime_error(string())`)
  is inferred from rule 6: every includer queues `allocator<char>()`, the
  `__default_alloc_template::allocate` chain, `~basic_string` and
  `~allocator` right after TRuntimeError's implicit members.
- Rule 6 again: GameMap's `_TImpl::_TProperties` declares an uncalled
  `operator=` after its destructor, whose member assignments instantiate
  `vector<TRumor>` and `vector<TTimedEvent>`'s `operator=` (pass 0) right
  after the copy constructor's vector copies.
- The first use of a non-inline template member fixes its pass-0 place:
  `string::assign(const char*, const char*)` comes from Town.h's timed event
  assignment, so the four objects that parse both Town.h and BlackBox.h
  (GameMap, MapView, MapValidation, GUIGameObject) include Town.h first.
- The editor's bitmap formats are explicit specializations
  (`T8bppBitmapBase<unsigned char>`, `T16bppBitmapBase<unsigned char>`):
  completed where T16bppBitmap.h defines them, with the 16-bit accessors
  queued right there (cppbridge.o's pixel-format masks precede CPoint).
- Inline members defined after their class are queued after its bodies:
  `TGameMap::getLayer`, `TCell::TCell()`, `_TImpl`'s condition visitors
  (defined as `class TGameMap::_TImpl::_TVictoryConditionWriter`) and
  `_TImpl::isValidPlacement` (defined between getNumObelisksOnMap and save).
- `.gcc_except_table` records where each `try` region starts, so a
  statement before or inside a `try` shows even though the code is the
  same: MapView's OnInitialUpdate creates the frame window before its `try`,
  onEditPlaceObject backs up the map before it, and the find commands
  assert the find type before it.

## Link

```sh
homm3 loki toolchain --libs DIR   # link media of config/loki/toolchain.toml [link]
homm3 loki link                   # build/h3maped-loki/link/h3maped (+ .map, link.log)
homm3 loki link-diff [-v]         # the linked file against retail, section by section
```

`--libs` (or `HOMM3_LOKI_LIBS`) stages the link media under
`build/loki/toolchain/link/` and builds every static library from its era
source (`homm3.loki.linklibs`; configure's test programs link and run through
the staged loader, never the kept objects). `homm3 loki link` compiles every
unit and runs binutils 2.9.1.0.25's `ld` directly, with what the image proves:

| Fact | Evidence |
| :--- | :------- |
| link order | `.comment` (one entry per linked object) runs 2 egcs, 122 GCC 2.95.2, 155 egcs, 23 GCC 2.95.2, 1 egcs, 1 GCC 2.95.2, 1 egcs: crt1.o and crti.o (glibc 2.1.3, egcs 1.1.2), crtbegin.o, the 103 project objects, libglade and libxml (GCC 2.95.2 C, no `.eh_frame`), GTK+, GDK, GModule, GLib and zlib (egcs), libstdc++ and libgcc, libc_nonshared, crtend.o, crtn.o. `.text` agrees: `glade_init` follows the last project object at 0x08202730, then libxml, `gtk_accel_group_new` (0x0823ead0), GDK, GModule/GLib, zlib (0x083206b0), libstdc++ (0x083278e0). All 2,481 exported C-library functions of the image are linked, in its order. |
| shared libraries | DT_NEEDED is libdl, libXi, libXext, libX11, libm, libc in that order. The link host's are Red Hat 6.0's glibc 2.1.1-6 (every imported libc/libm/libdl `st_size` matches) and X libraries ordered as Red Hat 6.2's libX11/libXi and XFree86 4's libXext (ld records imports in each library's `.dynsym` order). Everything else is static. libm is first named by g++'s own `-lstdc++ -lm` tail, after libstdc++.a: the image exports libstdc++'s `clog` stream unversioned, where an earlier `-lm` makes ld 2.9.1 give it libm's `clog@@GLIBC_2.1` version and a `.gnu.version_d`. |
| flags | `-export-dynamic` (12,052 defined `.dynsym` entries; libglade connects handlers by name), interpreter `/lib/ld-linux.so.2`, no `.symtab` (`-s`). |
| ld 2.9.1 | `.gcc_except_table` precedes `.eh_frame`, both after `.data`: ld 2.9.1's default script has neither, and an orphan goes right after `.data`, so the later one lands first. 2.9.5's script names `.eh_frame` first. The assembler is already 2.9.1.0.25. |
| GCC runtime | crtbegin.o, libgcc and libio/libstdc++ 2.10 come from a plain `make` (no bootstrap) of an i686-pc-linux-gnu 2.95.2 tree on the Red Hat link host: its stage-1 cc1/cc1plus, compiled by egcs 1.1.2 at configure's default `-g -O2` with the host assembler's alignment features, build the target libraries at `-g -O2` (default `-mcpu=pentiumpro`, libgcc `-fPIC` per `config/t-linux`, so its type_info objects go through the 63 `R_386_GLOB_DAT` GOT entries; threads off: no pthread imports). 2.95.2's `build_x_typeid` reads an uninitialized `nonnull` (`fixed_type_or_null` returns early for `*this` without setting it), so whether `typeid(*this)` tests `this` depends on the stack the compiling cc1plus leaves: this stage-1 cc1plus omits the test in `exception::what()`, as the image does, where a bootstrapped (stage 3), a release or an egcs `-g` cc1plus emits it. The recipe reproduces crtbegin's `__do_global_dtors_aux` and every linked libgcc and libstdc++ member. |
| libglade 0.14, libxml 1.8.9 | Built by Loki with the same GCC 2.95.2 at `-O2`, against Red Hat 6.0's glibc 2.1.1-6 headers (its `<bits/string2.h>` turns every `memset(p, 0, n)` into a `__bzero` call and leaves `memset(p, -1, 12)` a call, as the image's libxml does at all 12 and 5 sites; glibc 2.1.1 final and later inline both): the five libglade members are byte-exact in `.text`, and so are libxml's parser, SAX, entities, encoding and error. `xmlParserVersion` is "1.8.9"; libglade exports 0.14's `glade_set_custom_handler`. |
| GLib/GTK+ 1.2.8, zlib 1.0.8 | Red Hat 6.2's egcs 1.1.2 (egcs-1.1.2-30; it emits `.p2align 4,,7` for jump targets, Slackware's `.align 16`) and assembler (binutils-2.9.5.0.22-6: the image's GDK uses the `a0`/`a2` moffs byte moves and its nop fills) at Red Hat's `-O2 -m486 -fno-strength-reduce`, GTK+ with Red Hat's ahiguti i18n patch (the word breaks of GtkEntry, GtkLabel and GtkText call `iswpunct`/`iswcntrl`, which no GTK+ 1.2.x release does), GTK+ `--with-xinput=xfree` (libXi) and NLS on (the image imports `bindtextdomain` and `_nl_msg_cat_cntr`; Slackware 7.1's gettext 0.10.35 is on `PATH` while GTK+ configures). zlib at `-O2 -fno-strength-reduce`: all 14 members exact (`zlibVersion()` "1.0.8"). GLib 18 of 21 members, GModule 1/1, GDK 18 of 20, GTK+ 92 of 98. |

The linked file has the image's size, sections, PLT, `.dynsym` order,
`.comment` runs and every byte of `.text`. ObjectPaletteWnd.o's `.rodata`
ran 1 to 32 zero bytes past ours (GzBuf's 32-byte aligned `.rodata`
started 0x20 later); file constants are written last, in declaration order,
so the source ends its includes with a 4-byte zero constant,
`kInitialScrollPos`, whose name and type are not proven (MapObjectRef.h's
`kNullObjectID` is the same shape).

Two differences remain, stated as reviewed retail facts in
`config/retail/h3maped-loki/link-differences.toml` (region, byte count,
cause, evidence and the media that would close each):

- zlib's `deflate_copyright` is " deflate 1.0.4 Copyright 1995-1996
  Jean-loup Gailly " in `.data` (non-const, before deflateInit2_'s
  `my_version`), while deflate.o's code is 1.0.8's and inflate_copyright
  is 1.0.8's const string: Loki's zlib 1.0.8 carried 1.0.4's copyright
  line. No zlib.net release from 1.0.4 to 1.0.9 has that combination
  (1.0.4 and 1.0.5 declare a non-const string of their own version, 1.0.6
  on a const one); the tree Loki used is not found. The 1.0.8 string
  lengthens `.rodata` by 0x40 and shortens `.data` by 0x40: 135 bytes.
- The imported X symbols' `st_size` values (202 bytes): 170 of 194 match
  Red Hat 6.0's XFree86 3.3.3.1-49 libraries, whose `.dynsym` order does
  not match; the order matches Red Hat 6.2's libX11/libXi and XFree86 4's
  libXext (only it references XGetVisualInfo). The link host's X libraries
  are a build between those, presumably a Red Hat 6.0 erratum
  (XFree86-libs-3.3.3.1-52 or 3.3.5-0.6.0), of which no copy is found.

`homm3 loki link-diff` reads the linked file (or a path) and reports the
total differing bytes over the file; the ELF header and program headers; every
section's address, offset, size and differing bytes (each at its own file
offset); `.text` by census object at the image's addresses (start files,
each project object, the C libraries, libstdc++/libgcc); `.dynsym` order,
the PLT's import order; the `.comment` runs; and the C libraries' exported
functions in the image's address order (longest run kept in order). With the
link map beside the file it also sizes each `.text` group (start files,
project objects, C libraries, libstdc++/libgcc, linkonce) against the image's.

It then runs the link gate, which `homm3 loki link` and a full `homm3 loki
build` (when the link media are staged) also run; a failing gate fails the
command. A raw comparison would count one displaced object once for every
byte after it, so the gate aligns each allocated section with the image
piece by piece: the exported `.dynsym` symbols anchor the pieces, and
between two anchors with different displacements the split points (with at
most one intermediate displacement) that leave the fewest differing bytes
are chosen. A differing 4-byte word is accepted only when it is the image's
word translated by that map: an absolute address, or a PIC GOT-relative
offset, of a displaced byte. Every byte that still differs (headers and
padding included), and every linked byte that no image byte maps to, must
lie in a stated region and within its byte count; anything else fails as
UNSTATED.

Anonymous-namespace names are random per compile: `append_random_chars`
(gcc/tree.c) adds `(tv_usec << 16) ^ tv_sec ^ getpid()` to a static sum and
spells the sum in six base-62 digits, least significant first, which hold
all 32 bits. 34 units export `_GLOBAL_.N.<file><6 chars>` names through
`.dynsym`, `.dynstr` and their type-name strings; `homm3 loki census`
records each file's characters and the sum they spell in
`config/retail/h3maped-loki/anonymous.tsv`. The staged cc1plus runs with
`anonseed.so` preloaded (`scripts/homm3/loki/anonseed.c`, built with the
staged gcc into `build/loki/toolchain/libexec/`), and `homm3 loki build`
gives a listed unit `HOMM3_LOKI_TIMEOFDAY=<sum>.0` and `HOMM3_LOKI_PID=0`:
the names prove the sum, not its split between the clock and the process
id, so the whole sum is fed as seconds. Every compiled suffix equals the
image's. cppbridge.cpp's anonymous namespace holds only local data, which
the image strips, so it has no row and stays random. The comparison still
canonicalizes the names (`5_ANON`).

Launch (2026-10-08): headless only, `xvfb-run -a -s "-screen 0 1280x1024x24"`
with a run directory holding `heroes-iii-level-editor.glade`, a `heroes3`
stub on `PATH` and `data/h3bitmap.lod`, `data/h3sprite.lod` (GOG Complete),
under potato's `ld-2.1.3.so` with the staged X libraries. The linked editor
initializes GTK+, reads the Glade file and both LODs, then stops where retail
stops with the same data (re-run after the source-built libraries, same log): `Artifact.cpp:163: InitializeArtifactTraits:
Assertion '*resource[20] == 'S'' failed` (Complete's ARTRAITS.TXT is not
RoE's). Retail and linked logs are identical.

## Ledger and README block

`homm3 loki build --bank` records every built unit's functions in
`config/loki/match_baseline.tsv` (`homm3.loki.ledger`) with the game's
CUR <= MAX <= HIST schema. The fingerprint standing in for a source hash is
the compiled function's comparison form: g++ 2.95 at `-O0` compiles a
function from its own source, so MAX holds while that body is unchanged and
resets to CUR when it changes. The README's `loki-match-score` block renders
from that ledger and the census, never from a build report, on both of the
game's README edges (`homm3 build` and `homm3 status update
--write-readme`) and after `--bank`. It counts project functions only (the
`.text` and owned linkonce bodies of the 103 project objects) and splits out
the engine phase listed in `config/loki/units.toml` `[phases]`.

## Shared engine source

Engine units compile the game's own `src/` file, so one source serves both
programs. The first run (`homm3 loki build`, twelve game units, no new source)
pairs 378 retail functions and finds 101 exact, among them
`Bitmap16Bit::Bitmap16Bit(int,int)`, `Bitmap816::zBufferDraw`, `LODFile::sort`,
`CSequence::CSequence(int)`, `resource::resource(char const*, EResourceType)`,
`TTextResource::~TTextResource` and the SGI `vector<LODEntry>` instantiations.
Differences seen so far are real: for example `Bitmap16Bit(char const*, char
const*)` builds its path in `char[4096]` (Linux `PATH_MAX`) where Windows uses
`MAX_PATH`.


## Access specifiers

Member access (`public`, `protected`, `private`) in the editor-only
classes is inferred: from the naming conventions (`_m_`/`_` for private
members), the call sites, the friends a member needs and the pImpl split.
GCC 2.95 mangling and the stripped image do not record it. Only base-class
access and virtual bases are proven, by the RTTI base lists that
`__rtti_class` receives (each base's access in the top bits, 0x40 public,
0xc0 private: TMapDoc inherits all four of its clients privately, TMapView
everything but CWnd, the line operations their line clients). Shared engine
classes take member access from the game's Dreamcast CodeView where it is
available.

## Code-generation patterns (g++ 2.95.2 -O0)

- A `const` local is initialized through a pseudo register. The value is
  computed into a fresh register or a spilled temporary and then stored, a
  float goes through `fstps tmp; flds tmp`, and the extra pseudo changes
  register choice around it (callee-saved `%ebx/%esi/%edi` for successive
  const initializers, `this` spilled). The Loki palette, bitmap and font
  bodies use `const` for channel, scale, norm, colour and HSV locals.
- Calls inside an expression are expanded first. A function that returns a
  named local switches the C++ front end to a single jump at each later
  `return`; a `switch` switches it back to the three-jump epilogue (two for
  `void`). The state persists from one function to the next, and the
  release compiler makes a class's inline members' RTL when the class is
  parsed, so function order and header order are visible in the bytes
  (`Font.cpp` includes `font.h` first).
- Variables of sibling blocks reuse frame slots, last allocated first.
- A clip of the form `width -= -x` emits `neg; sub`, distinct from
  `width += x`.
- A `const` local with a constant initializer gets an ordinary frame slot
  (`movl $229,-4(%ebp)`), in declaration order with the other locals. Loki's
  `animateTilesets` instead keeps such constants in `%ebx/%esi/%edi` and
  spills the rest after every other local: they are `register const int`,
  the only way -O0 allocates a pseudo to a variable with a constant
  initializer. Their uses still fold to immediates.
- One loop variable declared at the top of a function and reused by several
  `for` loops takes one slot; a `for (unsigned int i = ...)` per loop takes
  one slot each (`loadTilesets`).
- An address-taken table of string pointers in `.rodata` is compared by the
  strings its entries point to (`$t<n>[...]#digest`), on both sides.
- An inline function keeps its parameters in registers. At `-O0` GCC 2.95
  homes every parameter in its stack slot (`obey_regdecls`) unless the
  function is `DECL_INLINE`, so the linkonce bodies of inline members and
  inline templates load `this` and their arguments into `%ebx/%esi/%edi`
  once (`writeFromIter`, `readToIter`, `writeContainer`, `TArray::end`,
  the raw stream member templates), while out-of-line functions reload
  them from `8(%ebp)`. A helper whose parameters live in registers was
  declared `inline`; a non-inline spelling of it never matches. Locals
  stay on the stack either way.
- Functions are only `.align 4`, but labels after a jump get
  `.p2align 4,,7`: the padding inside a function depends on where it sits
  modulo 16 in its object's `.text`, so it follows the sizes of every
  function before it. A body that differs from retail only in `leal
  (%esi),%esi`/`movl %esi,%esi` padding and the jump offsets around it is
  usually right; it converges once the functions before it in the object
  are complete (GameMap.cpp's TLayer bodies while the file is partial).
- A call result stored into a variable of the call's own type goes through
  a pseudo (`movl %eax,%eax` before the store); a converting store
  (`size_t` to `int`, `gchar*` to `const gchar*`) writes `%eax` directly
  (cppbridge.cpp's `on_h3path_ok_clicked` keeps the entry text in a
  `const gchar*`).
- A `volatile bool` is loaded into `%al` and tested (`movb; testb`) where a
  plain one is compared in memory (`cmpb $0`), and a volatile store takes
  `%al` without the `movb %al,%al` copy. The bridge's modal loops spin on
  volatile flags their signal handlers set.
- `b = !b` on a `bool` in memory is `xorb $1`; `b = b ? false : true` is a
  `cmpb` and two stores.
- `if (f(s) || g(t))` whose operands build temporaries with cleanups (the
  by-value `string` of `_isspace`) is evaluated into a byte temporary
  (`movb $0`, `movb $1`, `cmpb $0`) before the branch; a named flag
  assigned in the `if` gives a second slot.
- GTK+ 1.2.8's checked casts are macros: `GTK_OBJECT(_widget("x"))`
  evaluates `_widget` four times (the `GTK_IS_OBJECT` test, then the
  cast), `GTK_TOGGLE_BUTTON(w)` calls the type function before its
  argument.
- The GTK+ types beyond `gtkwidget.h` are `{anonymous}::` in the editor's
  dialogs and bridge (`<gtk/gtk.h>` included inside `namespace { }` after
  stdafx.h): it shows in mangled names (`PQ2..._GLOBAL_.N.ArmyDlg.cpp..9_GtkCombo`)
  and `__PRETTY_FUNCTION__` texts, while `GtkWidget` stays global.
- `fold` reassociates a subtraction of a difference with a constant:
  `x - (w - 1)` becomes `(x + 1) - w` (`incl`/`leal 1` on `x`), while
  `x + 1 - w` becomes `x - (w + -1)` (`decl` on `w`). Pick the spelling
  whose folded shape the image shows (`_TImpl`'s streamed constructor,
  `_computeObjExtent`). Additions associate the same way: `a + 1 + b`
  becomes `a + (b + 1)`, while `a + (b + 1)` becomes `(a + 1) + b`
  (TileVRuler's ones-digit offset).
- A cast of an lvalue binds a `const T&` parameter without a temporary,
  even across modes: `stream << (long) _s_akDimension[_m_size]` pushes the
  element's address (`movl $sym,%edx; addl %edx,%eax`), and
  `(signed char) teamInfo._m_numTeams` pushes `leal 4(%eax),%edx`; a
  reference cast `(const long&) x` computes the address in place
  (`leal sym(%eax)`, `addl $4,%eax`).
- `begin_while_stmt` emits a `nop` when the last insn is a label (the end
  of a preceding loop or `if`); a `for` statement first opens its own
  scope and never does. A `nop` before a loop's head therefore marks a
  `while` (`_isValidPlacement`'s height scans).
- VC6 for-scoping survives: a second loop that reuses the first loop's
  `for (unsigned int i = ...)` variable (`for (i = 0; ...)`, accepted with
  a warning) shares its slot, where a fresh declaration takes a new one.
- Declarations in `if (T* p = ...)` conditions are block-scoped temporaries;
  a dispatch that declares each pointer at function scope allocates them
  all before any expression temporary (`placeObject`'s frame).
- `return` of a declaration (a named local, a parameter, or the result
  slot of a class-returning function) at statement level sets the C++
  front end's `current_function_return_value`; a user label or a `case`
  label clears it, and nothing else does, so the state crosses function
  boundaries. When it is clear at `finish_function`, g++ adds a
  `goto no_return_label` and a jump to the return label: two extra `jmp`s
  before the epilogue, also in the compiler-generated bodies at the end
  of the object (static initialization, `_GLOBAL_.I`, type_info
  functions). MapValidation.cpp's eight misses are this state.
- A constructor that owns heap state guards only what follows its first
  owned allocation with `try { ... } catch (...) { delete p; throw; }`
  (`__start_cp_handler`, `__uncatch_exception`): the try block emits no
  code, but `.gcc_except_table` records where its region starts. TSeersHut's
  streamed constructor guards the reserved bytes read after the reward;
  the property sheets, TMapFrameWnd and TToolkitWnd create their first page
  or child before the `try`.
- The strong in-class inline members a class emits with its vtable come
  out in declaration order: TBlackBox's virtual `isCustomized`/`hasText`
  follow its content accessors.
- `while (n > 0) { --n; ... }` tests then decrements (`cmpb $0`, `decb`
  in the body); `while (n-- > 0)` copies, decrements and tests the copy.
- `-O0` uses stupid register allocation (`stupid.c`): a `register` local
  lives for its whole scope, a compiler temporary from first to last
  mention. A dead `register` local whose block closes before a call takes
  call-clobbered registers (`%eax:%edx` for an 8-byte table); one whose
  scope crosses the call takes `%ebx:%esi`, and the extra saved register
  shrinks the frame by 4 (`computeLineShape`'s unused flip table).
- `if (...) { ...; return; }` jumps straight to the epilogue, where an
  `if/else` arm jumps to the end of the enclosing `if` first; a `return`
  in an `else if` arm still leaves the jump over the following `else`.
- `-O0` never deletes a label (`delete_insn` only marks it), so a switch's
  exit label survives unreferenced and aligns the jump around the EH
  handlers after a switch whose cases all return. The generic case after
  the switch, not a `default:` arm, leaves that jump unaligned
  (`createObject`/`_createObject`).
- An unsigned `x % 8` and a source-level `x & 7` emit the same `andl $7`
  but allocate its result differently (the modulus goes through
  `expand_divmod`): `(dir + 1) & (kNumDirs - 1)` takes `%ecx` where
  `(dir + 1) % kNumDirs` takes `%edx` (`_validateTile`'s span loop).
