# The C2-slot shim - pass-through instrumentation of the back end

Shim v1 is a drop-in replacement `C2.DLL` that logs the argv the CL driver
hands to the back end and forwards, provably inertly, to the real `C2.DLL`.
It is the dynamic-analysis instrument the later phases extend (phase 3 hooks
the same slot in-process to read the inliner's budget globals); v1's job was
to prove the mechanism works and to fence it with a byte-identity gate.

Subjects (both hash-gated by `_toolchain.PINNED` before any step runs):

- `CL.EXE` driver 12.00.8168, sha256 `d9220fdf...`, 65,536 B, base 0x400000.
- `C2.DLL` back end 12.00.8447, sha256 `a0cc45f8...`, 737,329 B, base
  0x10700000, exporting exactly `_InvokeCompilerPass@12` (RVA 0x00068fd0)
  and `_AbortCompilerPass@4` (RVA 0x0007d4c1).

All addresses below are VAs inside the pinned `CL.EXE` unless noted.

## 1. The InvokeCompilerPass ABI (RE'd from the driver's call site)

```c
int __stdcall InvokeCompilerPass(int argc, char **argv, int fLastTU);
int __stdcall AbortCompilerPass(int code);
```

The driver's pass-runner function starts at **0x406009** (VC6 SEH prologue,
scope table 0x409a90, handler thunk 0x406620). Its evidence chain:

- **Pass descriptors** (static, `.data`): `0x40b134` "c1", `0x40b158` "c1xx",
  `0x40b17c` "c2", `0x40b1a0` "link.exe" - 0x24-byte records:
  `+0x00` name ptr (bare `"c2"` at 0x408518 - no extension; `LoadLibraryA`
  appends `.dll` and searches the driver's own directory first, which is why
  a same-named DLL next to CL.EXE slots in), `+0x08` ptr to
  `"MSC_CMD_FLAGS="` (env prepend), `+0x10` flags (statically 1),
  `+0x14` HMODULE, `+0x18` InvokeCompilerPass ptr, `+0x1c` AbortCompilerPass
  ptr. Flag bit 2 (in-process DLL pass vs `_spawnvp` EXE pass) and bit 3
  (below) are set at run time.
- **Resolution** (first call per pass): `LoadLibraryA(pass path)` at
  0x406130..0x406133 (IAT 0x407020 = KERNEL32!LoadLibraryA), then
  `GetProcAddress` (IAT 0x407038) twice at 0x40613f..0x406156 with the
  literal strings **`"_InvokeCompilerPass@12"`** (0x409a78) and
  **`"_AbortCompilerPass@4"`** (0x409a60); stored to desc `+0x18`/`+0x1c`
  at 0x406163..0x406169, `FreeLibrary` (IAT 0x40702c) on partial failure at
  0x40616f, error 0x7eb if unresolved (0x40617c).
- **argc**: counting loop 0x406195..0x4061a7 walks the NULL-terminated
  vector at `[ebp+0x18]` - argc = number of leading non-NULL entries.
- **The call**: 0x4061bc..0x4061bf
  `push edi; push edx; push ecx; call [esi+0x18]` with ecx=argc, edx=argv,
  edi=third arg; no caller stack cleanup follows, confirming stdcall @12.
  Return value lands in edi (0x4061c2); nonzero takes the pass-failure path.
- **Third arg = fLastTU**: computed at 0x4061b4..0x4061ba as
  `(desc->flags >> 3) & 1`. Bit 3 is written per source file at
  0x40449b..0x4044ae: set (`or [eax+0x10],8` at 0x4044a4) iff the current
  source-file list node's `next` pointer is NULL, cleared
  (`and [eax+0x10],~8` at 0x4044aa) otherwise - i.e. *this is the last TU
  of the driver invocation*. (A second setter of the same shape sits at
  0x404589.) The driver keeps the pass DLL loaded across a multi-TU batch
  and releases it - helper 0x405fe8: `FreeLibrary(desc+0x14)`, zero
  `+0x14/+0x18/+0x1c` - after the last TU or on failure (0x406200..0x40620b).
  Observed: single-TU compiles always get `fLastTU=1`.
- **argv is a `char**`, cross-proof**: the non-DLL branch of the same
  function passes the *same* `[ebp+0x18]` vector to MSVCRT `_spawnvp`
  (0x40621d..0x40622a, IAT 0x4070f8, cdecl `add esp,0xc`), whose argv
  parameter is a NULL-terminated `char*` vector by contract.
- **argv[0]** is the full Windows path of the pass binary (observed
  `Z:\...\msvc\bin\c2.dll`); the C2 option tokens follow, e.g. for the game
  profile `/c /O2 /Ob2 /Oy- /Op /ML /Gr /GX`:
  `-il <tmp> -f <src> -W 1 -G5 -Gs4096 -dos -Fdvc60.idb -Gy -ML -EHs -Fo<obj>`.
- **SEH**: the runner's filter (0x4061cd) special-cases 0x80000003; the
  current descriptor is published to 0x40b82c and an in-pass flag to
  0x40b830 around the call (that is how the driver's ctrl-C path finds
  `AbortCompilerPass`). v1 does not model this further.

Two independent oracles confirm the reading: `/Bd` makes the driver print
each pass's full command line, and `homm3 vc6 argv` models the same table -
the shim's logged vector matched both (see §3).

## 2. The overlay mechanism (`homm3.vc6.shim.build`)

The pinned toolchain is never modified in place. `build` creates
`build/vc6/toolchain-shim/msvc`:

- `bin/` is a real directory with every file **copied** (4.9 MB) - the
  driver's `LoadLibraryA` must resolve inside the overlay; all other
  top-level entries (`include/`, `lib/`, ...) are symlinks to the pinned
  tree.
- The copied `C2.DLL` is renamed **`C2_real.dll`** and re-verified against
  the pinned sha256/size after the copy.
- `shim/passthru.c` is compiled *by the pinned VC6 itself*
  (`CL /c /nologo /W3 /O1`, then `LINK /DLL /NOENTRY /DEF:passthru.def`
  against `kernel32.lib` only - C89, CRT-free, no entry point) and installed
  as the overlay's `C2.DLL`.
- The build hard-checks the export table: it must be exactly
  `{_InvokeCompilerPass@12, _AbortCompilerPass@4}`. The `.def` route
  works because the internal VC6-decorated stdcall symbols *are* the wanted
  export names, so LINK's exact-match lookup binds them; the resulting name
  table is byte-identical to the real C2.DLL's (same names, same ordinal
  order, same hints).

Compiling with `MSVC_DIR=build/vc6/toolchain-shim/msvc` through
`homm3.core.cc_wrap` puts the shim in the loop. On call it appends to the
log file named by **`HOMM3_VC6_SHIM_LOG`** (a Windows path; default
`c2shim_argv.log` in the cwd - the gate always passes the winepath of
`build/vc6/shim/argv.log`, which is `homm3.vc6.argv.SHIM_LOG`), resolves
`C2_real.dll` next to its own module once per process, forwards all three
arguments unchanged, and returns the real return value. Log format:

```
# c2shim call=1 export=InvokeCompilerPass fLastTU=1 argc=15 utc=2026-08-09T13:58:17 tick=...
Z:\...\toolchain-shim\msvc\bin\c2.dll -il C:\...\a00672 -f Z:\...\sample_tu.cpp -W 1 ... -Fo<obj>
# c2shim call=1 ret=0
```

The bare line is what `homm3 vc6 argv --verify` parses (last line containing
`c2.dll` wins). Note when hand-running `--verify`: its comparison keeps the
`-f <source>` pair on the model side while dropping it from the log side, so
an exact-flags run still reports the source token as "extra" - cosmetic,
owned by the argv tool.

## 3. The byte-identity gate (`gate`) - the inertness proof

`python3 -m homm3.vc6.shim.build gate` runs three checks (rc 0 green /
1 red / 2 harness error):

1. **/Bd-argv**: one overlay compile of the frozen `shim/sample_tu.cpp`
   with the game profile plus `/Bd`. The driver's own printed C2 command
   line and the shim's logged argv come from the *same* process, so they
   must be equal token for token including the `-il` temp path and the
   `-Bd` token itself. Measured: 16/16 tokens equal.
2. **identity**: the sample TU compiled twice via `cc_wrap` - real
   toolchain vs overlay - must be byte-identical **outside the COFF
   `TimeDateStamp` (file bytes 4..7)**. That mask is measured, not assumed:
   two back-to-back *real* compiles of the same TU differ in exactly one
   byte, at offset 4, by the elapsed seconds - the obj is otherwise fully
   deterministic (671-byte probe, 2026-08-09). Measured for the gate TU:
   804/804 bytes identical outside the stamp.
3. **argv-model**: the canonical run's logged tokens (argv[1:], `-il` value
   normalized to `<tmp>`) must equal `homm3.vc6.argv.expand()` of the exact
   flag list `cc_wrap` sent. Measured: 14/14 tokens agree.

A red gate means the shim is NOT inert and every conclusion drawn through
it is void - fix the shim before trusting any log.

**Negative control** (`negative`): rebuilds the shim with
`/DSHIM_NEGATIVE_CONTROL` - a variant that silently drops every `-Gy`
token before forwarding (the log still records the pristine vector) - and
requires the identity check to go **red**, then restores the clean shim and
requires green again. Measured: the mutated shim shrinks the obj 804 -> 652
bytes (function COMDATs collapse), 616 bytes differ - detected; restore
green. This is the gate's proof that it can fail.

Run book (inside `nix develop .#build` - the wine client must match the
running wineserver):

```sh
python3 -m homm3.vc6.shim.build build      # create/refresh the overlay
python3 -m homm3.vc6.shim.build gate       # the inertness + argv gates
python3 -m homm3.vc6.shim.build negative   # prove the gate can fail
python3 -m homm3.vc6.shim.build clean      # remove overlay + scratch
```

## 4. Phase 3 extension path (not in v1)

The shim already executes inside the compiler process at pass time, after
`C2_real.dll` is mapped and before/after every `InvokeCompilerPass`. The
planned extension keeps the byte-identity gate as the standing inertness
fence and adds, between the log and the forward:

- read C2 globals by `LoadLibraryA("C2_real.dll") + RVA` (the DLL prefers
  base 0x10700000 and carries a `.reloc` section, so use the returned
  HMODULE as the base, never the preferred base) - first target: the
  `/Ob2` inliner budget trajectory around each pass;
- optionally install IAT or hot-patch hooks inside `C2_real.dll` for
  per-decision tracing.

Any such build must keep a hook-free configuration that still passes the
gate, and every instrumented conclusion needs a gate-green control run of
the same source. v1 deliberately contains no hooks.

## 5. Files

| Path | Role |
|---|---|
| `scripts/homm3/vc6/shim/passthru.c` | the shim DLL source (C89, CRT-free) |
| `scripts/homm3/vc6/shim/passthru.def` | the two decorated exports |
| `scripts/homm3/vc6/shim/sample_tu.cpp` | frozen gate input - do not edit |
| `scripts/homm3/vc6/shim/build.py` | overlay builder + gates (CLI above) |
| `build/vc6/toolchain-shim/` | the overlay (gitignored) |
| `build/vc6/shim/argv.log` | canonical shim log (= `argv.SHIM_LOG`) |
| `build/vc6/shim/gate/` | gate scratch: ref/shim objs, /Bd log |
