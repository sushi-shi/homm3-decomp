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
`HOMM3_LOKI_BINUTILS`, `HOMM3_LOKI_GCC` and `HOMM3_LOKI_GTK` work as well. Everything is staged under ignored
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
`__rtti_class` receives (each base's access in the top bits: TMapDoc's
`TGameMap::TClient` base is private, its other bases public). Shared engine
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
- A constructor that owns heap state and reads it from a stream guards
  the reading with `try { ... } catch (...) { delete p; throw; }`
  (`__start_cp_handler`, `__uncatch_exception`; TSeersHut's streamed
  constructor).
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
